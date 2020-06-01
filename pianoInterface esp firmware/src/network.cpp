#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUdp.h>

#include "circularBuffer.h"
#include "lighting/lighting.h"
#include "lighting/color.h"
#include "m_constants.h"
#include "m_error.h"
#include "music.h"
#include "pinaoCom.h"
#include "network.h"
#include "settings.h"
#include "serialDebug.h"

namespace
{
    unsigned long lastMessageMillis = 0;
    byte messageBuffer[8];
    byte songStreamBuffer[music::maxNoteCount];
    char songConversionBuffer[music::maxNoteCount];

    unsigned int loaderFrameIndex;      // What frame of the song was last loaded
    byte frameNoteIndex;                // How many notes for the currently loading frame have been loaded
    byte currentFrameNotes[_PIANOSIZE]; // Storage for loading notes in the frame before being memcopied by ::music

    WebServer webServer(80);
    WiFiClient client; // persistant accross poll calls.

    // combine to bytes into the unsigned short they make together
    uint16_t concatBytes(uint8_t A, uint8_t B)
    {
        uint16_t combined = A;
        combined = combined << 8;
        combined |= B;
        return combined;
    }

    int intArg(const char * name){
        return webServer.arg(name).toInt();
    }
} // namespace

namespace network
{
    void handleGetStatus();
    void handleGetIndex();
    void handleSetIndex();
    void handleUploadSong();
    void handleChangeSetting();
    void handleUpdateLoopSetting();
    void handleRestoreSettings();
    void handleGetSettings();
    void handleSetAnimationMode();
    void handleSaveSettings();

    // Starts connecting to the WIFI network
    void beginConnection()
    {
        //WiFi.setHostname(hostName);
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(_SSID, _NETWORKKEY);
    }

    // Blocks until WIFI connection has been established.
    // Returns success.
    bool waitForConnection()
    {
        unsigned int attempts = 0;
        wl_status_t status;
        do
        {
            status = WiFi.status();
            delay(100);
            attempts++;
            if (attempts == 60)
            {
                switch (status)
                {
                case WL_NO_SSID_AVAIL:
                    fatalError(ErrorCode::WIFI_NO_SSID_AVAIL);
                    break;
                case WL_CONNECT_FAILED:
                    fatalError(ErrorCode::WIFI_CONNECTION_FAILED);
                    break;
                case WL_CONNECTION_LOST:
                    fatalError(ErrorCode::WIFI_CONNECTION_LOST);
                    break;
                case WL_DISCONNECTED:
                    fatalError(ErrorCode::WIFI_DISCONNECTED);
                    break;
                default:
                    fatalError(ErrorCode::WIFI_CONNECTION_TIMEOUT);
                    break;
                }
                return false;
            }
        } while (status != WL_CONNECTED);

        if (status != WL_CONNECTED)
        {
            fatalError(ErrorCode::IMPOSSIBLE_INTERNAL);
            return false;
        }

        // OTA stuff
        ArduinoOTA.onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
            {
                type = "sketch";
            }
            else
            { // U_SPIFFS
                type = "filesystem";
            }
        });

        ArduinoOTA.begin();

        return true;
    }

    // Starts the TCP server
    void startServer()
    {
        webServer.on("/getStatus", handleGetStatus);
        webServer.on("/getSongIndex", handleGetIndex);
        webServer.on("/setSongIndex", handleSetIndex);
        webServer.on("/uploadSong", HTTP_POST, handleUploadSong);
        webServer.on("/changeSetting", handleChangeSetting);
        webServer.on("/updateLoopSetting", handleUpdateLoopSetting);
        webServer.on("/restoreSettings", handleRestoreSettings);
        webServer.on("/getSettings", handleGetSettings);
        webServer.on("/setAnimationMode", handleSetAnimationMode);
        webServer.on("/saveSettings", handleSaveSettings);
        webServer.begin();
    }

    // Whether the network is currently connected
    bool isConnected()
    {
        return false;
    }

    void pollOTA()
    {
        ArduinoOTA.handle();
    }

    // Checks for incoming messages and handles them
    void pollEvents()
    {
        webServer.handleClient();
        return;
    }

    void handleGetStatus()
    {
        // formattedClientReply("Status: Error lock code: %d", static_cast<uint8_t>(getCurrentError()));
        webServer.send(200, "text/plane", "OK");
    }

    void handleGetIndex()
    {
        webServer.send(200, "text/plane", String(music::currentFrameIndex()));
    }

    void handleSetIndex()
    {
        music::setFrame(intArg("index"));
        Event e;
        e.action = []() {
            lights::forceRefresh();
        };
        PushEvent(e);
        webServer.send(200, "text/plane", "OK");
    }

    void handleUploadSong()
    {
        if (!webServer.hasArg("plain") || !webServer.hasArg("frames") || !webServer.hasArg("notes"))
        {
            debug::println("no content");
            webServer.send(400, "text/html", "no content");
            return;
        }

        unsigned int expectedSongLength = intArg("frames");
        unsigned int expectedNoteCount = intArg("notes");

        Serial.println("expecting " + String(expectedSongLength) + " frames and " + String(expectedNoteCount) + " notes");

        frameNoteIndex = 0;
        loaderFrameIndex = 0;
        
        String data = webServer.arg("plain");

        data.toCharArray(songConversionBuffer, music::maxNoteCount);
        auto temp = reinterpret_cast<unsigned char *>(songConversionBuffer);

        int byteCount = expectedNoteCount + expectedSongLength;
        uint16_t lastFrameIndex = 0;
        byte frameLength = 0;
        uint16_t frameCount = 0, noteCount = 0;
        for (size_t i = 0; i < byteCount; i++)
        {
            if (temp[i] == 250U)
            {
                music::loadFrame(temp + lastFrameIndex, frameLength);
                lastFrameIndex = i + 1;
                frameCount++;
                frameLength = 0;
            }
            else
            {
                noteCount++;
                frameLength++;
            }
        }

        if (expectedNoteCount != noteCount || expectedSongLength != frameCount)
        {
            webServer.send(400, "text/plain", "Failed");
            fatalError(ErrorCode::SONG_LOAD_UNEXPECTED_FRAME_COUNT);
            return;
        }

        webServer.send(200, "text/plain", "Upload ok");

        Event e;
        e.action = []() {
            lights::setAnimationMode(lights::AnimationMode::BlinkSuccess);
            while (!lights::animationCompleted())
            {
                lights::updateAnimation();
            }
            lights::setAnimationMode(lights::AnimationMode::Waiting);
        };
        PushEvent(e);

        music::setLoopingSettings(true, 0, expectedSongLength - 1);
    }

    void handleChangeSetting()
    {
        int setting = intArg("setting");
        if(setting > 7)
        {
            webServer.send(400, "text/plane", "Invalid setting number");
            fatalError(ErrorCode::INVALID_SETTING);
            return;
        }
        int argA = intArg("A");
        int argB = intArg("B");
        int argC = intArg("C");
        settings::saveColorSetting(static_cast<unsigned int>(setting), {argA, argB, argC});
    }

    void handleUpdateLoopSetting()
    {
        bool enabled = intArg("enabled");
        int loopStart = intArg("start");
        int loopEnd = intArg("end");
        music::setLoopingSettings(enabled, loopStart, loopEnd);
        webServer.send(200, "text/plane", "OK");
    }

    void handleRestoreSettings()
    {
        settings::restoreDefaults();
        webServer.send(200, "text/plane", "OK");
    }

    void handleGetSettings()
    {
        // if (messageBuffer[1] > 7)
        // {
        //     fatalError(ErrorCode::INVALID_SETTING);
        //     ClientReply("ER:Invalid setting");
        //     disconnectClient();
        //     return;
        // }

        // color col = settings::getColorSetting(static_cast<unsigned int>(messageBuffer[1]));
        // {
        //     // formattedClientReply("OK:%c%c%c", col.r, col.b, col.b);
        //     formattedClientReply("OK:%d,%d,%d", col.r, col.g, col.b);
        // }
    }

    void handleSetAnimationMode()
    {
        int mode = intArg("mode");
        if (mode > 5)
        {
            webServer.send(400, "text/plane", "Invalid mode");
            return;
        }
        switch (mode)
        {
        case 0:
            _pushModeSwitchEvent(lights::AnimationMode::None);
            break;
        case 1:
            _pushModeSwitchEvent(lights::AnimationMode::Ambiant);
            break;
        case 2:
            _pushModeSwitchEvent(lights::AnimationMode::ColorfulIdle);
            break;
        case 3:
            _pushModeSwitchEvent(lights::AnimationMode::KeyIndicate);
            break;
        case 4:
            _pushModeSwitchEvent(lights::AnimationMode::KeyIndicateFade);
            break;
        case 5:
            _pushModeSwitchEvent(lights::AnimationMode::Waiting);
            break;
        default:
            fatalError(ErrorCode::IMPOSSIBLE_INTERNAL);
            break;
        }
        webServer.send(200, "text/plane", "OK");
    }

    void handleSaveSettings()
    {
        settings::commitSettings();
        webServer.send(200, "text/plane", "OK");
    }
} // namespace network
