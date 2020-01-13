#include <Arduino.h>
#include <WiFi.h>

#include "circularBuffer.h"
#include "lighting/lighting.h"
#include "lighting/color.h"
#include "m_constants.h"
#include "m_error.h"
#include "music.h"
#include "pinaoCom.h"
#include "network.h"
#include "settings.h"

/**
 * PROTOCOL SPECIFICATION:
 * 
 * Message length : 8 bytes
 * 
 * byte 1: message type
 * 
 * message type: 
 *      1 = change setting
 *      2 = update loop setting
 *      3 = begin loading song
 *      4 = song data
 *      5 = end song loading
 *      6 = set song index
 *      7 = get status
 *      8 = get live frame index
 *      9 = restore default settings
 *      10 = get setting
 *      11 = change animation mode
 *      12 = commit settings
 * 
 * ------------------------------
 * 
 * -------- 1
 * 
 * change setting:
 * byte 2: setting number
 * byte 3: data
 * byte 4: data
 * byte 5: data
 * byte 6: data
 * 
 * -------- 2
 * 
 * update loop setting:
 * byte 2: looping enabled 
 * byte 3&4: loop start index
 * byte 5&6: loop end index
 * 
 * -------- 3
 * 
 * begin loading song:
 * byte: 2&3 song index length;
 * 
 * -------- 4
 * 
 * song data:
 * byte 2&3: frame index
 * byte 4: note number
 * byte 5: note number
 * byte 6: note number
 * byte 7: note number 
 * byte 8: note number
 * NOTE: if there are less than 5 notes, empy bytes must be 255 NOT NULL
 * 
 * -------- 5
 * 
 * end song loading:
 * none
 * 
 * -------- 6
 * 
 * set song index:
 * byte 2&3: song index 
 * 
 * -------- 7
 * 
 * get status
 * none 
 * 
 * -------- 8
 * 
 * get live frame index:
 * none 
 * 
 * -------- 9
 * 
 * restore default settings:
 * none 
 * 
 * -------- 10
 * 
 * get setting:
 * byte 2 setting number
 * 
 * 
 * -------- 11
 * 
 * change animation mode:
 * byte 2 animation number
 *      animation number:
 *          0: None
 *          1: Ambiant
 *          2: ColorfulIdle
 *          3: KeyIndicate
 *          4: KeyIndicateFade
 *          5: Waiting
 *          
 * -------- 12
 * 
 * commit settings:
 * none 
 * 
 */

namespace
{
bool connected;
byte messageBuffer[8];

// these are for song loading
unsigned int expectedSongLength = 0;
unsigned int loaderFrameIndex;      // What frame of the song was last loaded
byte frameNoteIndex;                // How many notes for the currently loading frame have been loaded
byte currentFrameNotes[_PIANOSIZE]; // Storage for loading notes in the frame before being memcopied by ::music

WiFiServer server(80);
WiFiClient client; // persistant accross poll calls.

// combine to bytes into the unsigned short they make together
uint16_t concatBytes(uint8_t A, uint8_t B)
{
    uint16_t combined = A;
    combined = combined << 8;
    combined |= B;
    return combined;
}

void formattedClientReply(char *fmt, ...){
    char buff[128];
    va_list va;
    va_start (va, fmt);
    vsprintf (buff, fmt, va);
    va_end (va);
    sprintf(buff, "%s", buff);
    client.print(buff);
}

} // namespace

namespace network
{

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
    connected = true;
    return true;
}

// Starts the TCP server
void startServer()
{
    server.begin();
}

// Whether the network is currently connected
bool isConnected()
{
    return connected;
}

// Checks for incomming messages and handles them
void pollEvents()
{
    // Client may still be connected since last poll call
    if (!client)
    {
        // listen for incoming clients
        client = server.available();

        // Nobody trying to connect at this time
        if (!client)
        {
            return;
        }
    }

    // At this point we know there is client connected

    // check if the client has any data it's trying to send
    if (!client.available())
    {
        return;
    }

    // At this point we know the client has data it's trying to send us

    // Wait for the correct number of bytes to become available
    int attemps = 0;
    while (client.available() != 8)
    {
        attemps++;
        delay(10);

        // A full message is not available. Something went wrong.
        if (attemps > 10)
        {
            // Might not need to be fatal...
            fatalError(ErrorCode::TCP_MESSAGE_INCOMPLETE);
            return;
        }
    }

    //At this point we know the connected client has a message of valid length
    client.readBytes(messageBuffer, 8);

    switch (messageBuffer[0])
    {
    // change setting
    case 1:
    {
        if(messageBuffer[1] > 7)
        {
            fatalError(ErrorCode::INVALID_SETTING);
            client.print("ER:Invalid setting");
            client.stop();
            return;
        }

        settings::saveColorSetting(static_cast<unsigned int>(messageBuffer[1]), {messageBuffer[2], messageBuffer[3], messageBuffer[4]});
        client.print("OK: setting updated");
    }
    break;
    // update loop setting
    case 2:
    {
        bool enabled = messageBuffer[1];
        uint16_t loopStart = concatBytes(messageBuffer[2], messageBuffer[3]);
        uint16_t loopEnd = concatBytes(messageBuffer[4], messageBuffer[5]);
        music::setLoopingSettings(enabled, loopStart, loopEnd);
        if (!enabled)
        {
            client.print("OK: Looping disabled");
        }
        else
        {
            formattedClientReply( "OK: Looping from %d to %d", loopStart, loopEnd);
        }
    }
    break;
    // begin song loading
    case 3:
    {
        //When a song is done loading, expected length is reset. If it is 0 here then something went wrong.
        if (!assert_fatal(expectedSongLength == 0, ErrorCode::SONG_LOAD_DISCONTINUITY))
        {
            client.print("ER:Song length already set");
            client.stop();
            return;
        }

        unsigned short songLength = messageBuffer[1];
        songLength = songLength << 8;
        songLength |= messageBuffer[2];
        expectedSongLength = songLength;

        frameNoteIndex = 0;
        loaderFrameIndex = 0;

        if (!assert_fatal(expectedSongLength <= music::maxSongLength && expectedSongLength != 0, ErrorCode::SONG_LOAD_DISCONTINUITY))
        {
            formattedClientReply( "ER:Song length out of range: %d", expectedSongLength);
            client.stop();
            return;
        }

        // reply OK
        formattedClientReply( "Song header OK. Length: %d", expectedSongLength);

        // update progress bar
        music::resetSongLoader();
        Event e;
        e.action = []() {
            lights::setAnimationMode(lights::AnimationMode::ProgressBar);
            lights::AnimationParameters::setProgressBarValue(0.0f);
        };
        PushEvent(e);
    }
    break;
    // song data
    case 4:
    {
        uint16_t incommingFrameIndex = concatBytes(messageBuffer[1], messageBuffer[2]);

        if (!assert_fatal(incommingFrameIndex <= expectedSongLength, ErrorCode::INVALID_SONG_FRAME_INDEX))
        {
            client.print("ER:Frame index out of expected range");
            client.stop();
            return;
        }

        // If there are more notes being pressed at once than there are notes on the piano, we have an issue.
        if (!assert_fatal(frameNoteIndex <= 88, ErrorCode::SONG_LOAD_DISCONTINUITY))
        {
            client.print("ER: Invalid note index");
            client.stop();
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
            client.stop();
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
                    client.stop();
                    return;
                }
                break;
            }
            // Add note to the current frame
            currentFrameNotes[frameNoteIndex] = messageBuffer[i] - MIDI::noteNumberOffset;
            frameNoteIndex++;
        }

        // Reply with OK + debug info
        formattedClientReply("OK: %d notes loaded for frame %d", frameNoteIndex, loaderFrameIndex);

        // Update the progress bar display
        Event e;
        e.action = []() {
            float progress = (float)loaderFrameIndex / (float)expectedSongLength;
            lights::AnimationParameters::setProgressBarValue(progress);
        };
        PushEvent(e);
    }
    break;
    // song ending
    case 5:
    {
        if (loaderFrameIndex != expectedSongLength - 1)
        {
            formattedClientReply("ER:Expected %d notes, but only recieved %d", expectedSongLength - 1, loaderFrameIndex);
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

            client.print("OK: Song loaded");
        }
    }
    break;
    // set song index
    case 6:
    {
        uint16_t newIndex = concatBytes(messageBuffer[1], messageBuffer[2]);
        music::setFrame(newIndex);
        formattedClientReply("OK: index set to [%d]", music::currentFrameIndex());

        Event e;
        e.action = []() {
            lights::forceRefresh();
        };
        PushEvent(e);
    }
    break;
    // get status
    case 7:
    {
        if (isErrorLocked())
        {
            formattedClientReply("Status: Error lock code: %d", static_cast<uint8_t>(getCurrentError()));
        }
        else
        {
            client.print("Status: OK");
        }
    }
    break;
    // get current frame index
    case 8:
    {
        formattedClientReply("OK:%d", music::currentFrameIndex());
    }
    break;
    // restore default settings
    case 9:
    {   
        settings::restoreDefaults();
        client.print("OK: Default settings restored");
    }
    break;
    // Get setting
    case 10:
    {
        if(messageBuffer[1] > 7)
        {
            fatalError(ErrorCode::INVALID_SETTING);
            client.print("ER:Invalid setting");
            client.stop();
            return;
        }

        color col = settings::getColorSetting(static_cast<unsigned int>(messageBuffer[1]));
        {
            // formattedClientReply("OK:%c%c%c", col.r, col.b, col.b);
            formattedClientReply("OK:%d,%d,%d", col.r, col.g, col.b);
        }
    }
    break;
    // Set animation mode
    case 11:
    {
        if(messageBuffer[1] > 5)
        {
            formattedClientReply("ER: Invalid animation number: %d", messageBuffer[1]);
            client.stop();
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
        client.print("OK: Mode updated");
    }
    break;
    // commit settings
    case 12:
    {   
        settings::commitSettings();
        client.print("OK: settings commited");
    }
    break;
    default:
    {
        char buffer[128];
        sprintf(buffer, "ER:Unrecognized command header: %d", messageBuffer[0]);
        client.print(buffer);
    }
    break;
    }
}
} // namespace network
