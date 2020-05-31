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
    bool connected = false; //message was received within the last 2 seconds
    byte messageBuffer[8];
    byte songStreamBuffer[music::maxNoteCount];
    char songConversionBuffer[music::maxNoteCount];

    // these are for song loading
    unsigned int expectedSongLength = 0;
    unsigned int expectedNoteCount = 0;
    // expecting the next message to be song data
    bool expectingSong = false;
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

    void ClientReply(const char * message){
        client.println(message);
    }

    void formattedClientReply(char *fmt, ...)
    {
        char buff[128];
        va_list va;
        va_start(va, fmt);
        vsprintf(buff, fmt, va);
        va_end(va);
        sprintf(buff, "%s", buff);
        client.println(buff);
    }

    void disconnectClient(){
        connected = false;
        lights::setGreenLED(false);
        client.flush();
        client.stop();
        debug::println("disconnected from client");
    }

    constexpr int intArg(const char * name){
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
        return connected;
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

        expectedSongLength = intArg("frames");
        expectedNoteCount = intArg("notes");

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
           // Serial.printf("Data[%d]: %d", i, temp[i]);
           // Serial.println("");
            if (temp[i] == 250U)
            {
                music::loadFrame(temp + lastFrameIndex, frameLength);
                lastFrameIndex = i + 1;
                frameCount++;
                frameLength = 0;
              //  Serial.printf("Framecount: %d" , frameCount);
              //  Serial.println("");
            }
            else
            {
                // songStreamBuffer[i] -= MIDI::noteNumberOffset;
                noteCount++;
                frameLength++;
            }
        }
     //   Serial.printf("Framecount: %d" , frameCount);
      //  Serial.println("");
        if (expectedNoteCount != noteCount || expectedSongLength != frameCount)
        {
         //   Serial.printf("ER: Unexpected song length. %d / %d frames, %d / %d notes", frameCount, expectedSongLength, noteCount, expectedNoteCount);
           // Serial.println("");
            webServer.send(200, "text/plain", "Failed");
            fatalError(ErrorCode::SONG_LOAD_UNEXPECTED_FRAME_COUNT);
            return;
        }

        //debug::println("uploaded successfully");
        webServer.send(200, "text/cool", "Upload ok");

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
        expectedSongLength = 0;
        expectedNoteCount = 0;
    }











    void changeSetting(byte messageBuffer[8])
    {
        if (messageBuffer[1] > 7)
        {
            fatalError(ErrorCode::INVALID_SETTING);
            ClientReply("ER:Invalid setting");
            disconnectClient();
            return;
        }

        settings::saveColorSetting(static_cast<unsigned int>(messageBuffer[1]), {messageBuffer[2], messageBuffer[3], messageBuffer[4]});
        ClientReply("OK");
    }

    void updateLoopSetting(byte messageBuffer[8])
    {
        bool enabled = messageBuffer[1];
        uint16_t loopStart = concatBytes(messageBuffer[2], messageBuffer[3]);
        uint16_t loopEnd = concatBytes(messageBuffer[4], messageBuffer[5]);
        music::setLoopingSettings(enabled, loopStart, loopEnd);
        ClientReply("OK");
    }

    void beginSongLoading(byte messageBuffer[8])
    {
        //When a song is done loading, expected length is reset. If it is 0 here then something went wrong.
        if (!assert_fatal(expectedSongLength == 0, ErrorCode::SONG_LOAD_DISCONTINUITY))
        {
            ClientReply("ER:Song length already set");
            disconnectClient();
            return;
        }

        expectedSongLength = concatBytes(messageBuffer[1], messageBuffer[2]);
        expectedNoteCount = concatBytes(messageBuffer[3], messageBuffer[4]);

        frameNoteIndex = 0;
        loaderFrameIndex = 0;

        if (!assert_fatal(expectedSongLength <= music::maxSongLength && expectedSongLength != 0, ErrorCode::SONG_LOAD_DISCONTINUITY))
        {
            formattedClientReply("ER:Song length out of range: %d", expectedSongLength);
            disconnectClient();
            return;
        }

        expectingSong = true;

        // reply OK
        formattedClientReply("Song header OK. Length: %d", expectedSongLength);

        // update progress bar
        music::resetSongLoader();
        Event e;
        e.action = []() {
            lights::setAnimationMode(lights::AnimationMode::ProgressBar);
            lights::AnimationParameters::setProgressBarValue(0.0f);
        };
        PushEvent(e);
    }

    void songData(byte messageBuffer[8])
    {
        uint16_t incommingFrameIndex = concatBytes(messageBuffer[1], messageBuffer[2]);

        if (!assert_fatal(incommingFrameIndex <= expectedSongLength, ErrorCode::INVALID_SONG_FRAME_INDEX))
        {
            ClientReply("ER:Frame index out of expected range");
            disconnectClient();
            return;
        }

        // If there are more notes being pressed at once than there are notes on the piano, we have an issue.
        if (!assert_fatal(frameNoteIndex <= 88, ErrorCode::SONG_LOAD_DISCONTINUITY))
        {
            ClientReply("ER: Invalid note index");
            disconnectClient();
            return;
        }

        if (incommingFrameIndex - loaderFrameIndex == 1)
        {
            // End of frame, so send what was loaded to the song loader
            music::loadFrame(currentFrameNotes, frameNoteIndex);

            // Expect the next frame and reset the note counter for the current frame
            loaderFrameIndex++;
            frameNoteIndex = 0;
        }
        else if (incommingFrameIndex - loaderFrameIndex != 0)
        {
            // ERROR: frame was skipped
            fatalError(ErrorCode::SONG_LOAD_DISCONTINUITY);

            formattedClientReply("ER:Frame skip detected. Last: %d, Recieved: %d", loaderFrameIndex, incommingFrameIndex);
            disconnectClient();
            return;
        }

        // read note values
        for (size_t i = 3; i < 8; i++)
        {
            // 255 means no notes left in the message.
            // This message contains the whole frame, or the end of a large frame
            if (messageBuffer[i] == 255)
            {
                // Emtpy message. Should be impossible
                if (i == 3)
                {
                    fatalError(ErrorCode::SONG_LOAD_DISCONTINUITY);
                    disconnectClient();
                    return;
                }
                break;
            }
            // Add note to the current frame
            currentFrameNotes[frameNoteIndex] = messageBuffer[i] - MIDI::noteNumberOffset;
            frameNoteIndex++;
        }

        // Reply with OK + debug info
        //formattedClientReply("OK: %d notes loaded for frame %d", frameNoteIndex, loaderFrameIndex);
        ClientReply("OK");

        // Update the progress bar display
        Event e;
        e.action = []() {
            float progress = (float)loaderFrameIndex / (float)expectedSongLength;
            lights::AnimationParameters::setProgressBarValue(progress);
        };
        PushEvent(e);
    }

    void songEnding(byte messageBuffer[8])
    {
        if (loaderFrameIndex != expectedSongLength - 1)
        {
            formattedClientReply("ER:Expected %d notes, but only received %d", expectedSongLength - 1, loaderFrameIndex);
            fatalError(ErrorCode::SONG_LOAD_DISCONTINUITY);
        }
        else
        {
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
            expectedSongLength = 0;
            expectedNoteCount = 0;

            ClientReply("OK: Song loaded");
        }
        expectingSong = false;
    }

    void setSongIndex(byte messageBuffer[8])
    {
        uint16_t newIndex = concatBytes(messageBuffer[1], messageBuffer[2]);
        music::setFrame(newIndex);
        //formattedClientReply("OK: index set to [%d]", music::currentFrameIndex());
        ClientReply("OK");

        Event e;
        e.action = []() {
            lights::forceRefresh();
        };
        PushEvent(e);
    }

    void getStatus(byte messageBuffer[8])
    {
        if (isErrorLocked())
        {
            formattedClientReply("Status: Error lock code: %d", static_cast<uint8_t>(getCurrentError()));
        }
        else
        {
            ClientReply("Status: OK");
        }
    }

    void getCurrentFrameIndex()
    {
        formattedClientReply("OK:%d", music::currentFrameIndex());
    }

    void restoreSettings()
    {
        settings::restoreDefaults();
        ClientReply("OK");
    }

    void getSetting(byte messageBuffer[8])
    {
        if (messageBuffer[1] > 7)
        {
            fatalError(ErrorCode::INVALID_SETTING);
            ClientReply("ER:Invalid setting");
            disconnectClient();
            return;
        }

        color col = settings::getColorSetting(static_cast<unsigned int>(messageBuffer[1]));
        {
            // formattedClientReply("OK:%c%c%c", col.r, col.b, col.b);
            formattedClientReply("OK:%d,%d,%d", col.r, col.g, col.b);
        }
    }

    void setAnimationMode(byte messageBuffer[8])
    {
        if (messageBuffer[1] > 5)
        {
            formattedClientReply("ER: Invalid animation number: %d", messageBuffer[1]);
            disconnectClient();
            return;
        }
        switch (messageBuffer[1])
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
            formattedClientReply("ER:Internal %d", __LINE__);
            fatalError(ErrorCode::IMPOSSIBLE_INTERNAL);
            break;
        }
        ClientReply("OK");
    }

    void saveSettings()
    {
        settings::commitSettings();
        ClientReply("OK");
    }
} // namespace network
