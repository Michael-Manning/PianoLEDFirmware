#include <Arduino.h>
#include "network.h"
#include <WiFi.h>
#include "music.h"
#include "m_error.h"
#include "m_constants.h"
#include "circularBuffer.h"
#include "pinaoCom.h"

/**
 * PROTOCOL SPECIFICATION:
 * 
 * Message length : 8 bytes
 * 
 * byte 1: message type
 * 
 * message type: 
 *      1 = change mode
 *      2 = update loop setting
 *      3 = begin loading song
 *      4 = song data
 *      5 = end song loading
 *      6 = set song index
 *      7 = get status
 *      8 = get live frame index
 * 
 * ------------------------------
 * 
 * -------- 1
 * 
 * change mode:
 * byte 2: mode number
 * 
 * mode number:
 *      1 = indicate
 *      2 = wait
 *      3 = fade
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
 */

namespace
{
bool connected;
byte messageBuffer[8];

bool networkBusy = false; // not implimented
unsigned int expectedSongLength = 0;
unsigned int loaderFrameIndex;      // What frame of the song was last loaded
byte frameNoteIndex;                // How many notes for the currently loading frame have been loaded
byte currentFrameNotes[_PIANOSIZE]; // Storage for loading notes in the frame before being memcopied by ::music

WiFiServer server(80);
WiFiClient client; // persistant accross poll calls.

uint16_t concatBytes(uint8_t A, uint8_t B)
{
    uint16_t combined = A;
    combined = combined << 8;
    combined |= B;
    return combined;
}

} // namespace

namespace network
{

const char *hostName = "PianoESP";

void beginConnection()
{
    WiFi.setHostname(hostName);
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(_SSID, _NETWORKKEY);
}
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
void startServer()
{
    server.begin();
}
bool isConnected()
{
    return connected;
}
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
    // change mode
    case 1:
    {
        PlayMode mode;
        switch (messageBuffer[1])
        {
        case 1:
            mode = PlayMode::Idle;
            break;
        case 2:
            mode = PlayMode::Indicate;
            break;
        case 3:
            mode = PlayMode::Waiting;
            break;
        default:
            client.write("ER:Unknown PlayMode");
            return;
            break;
        }
        Event e;
        e.action = []() {
            //  globalMode =
        };
        PushEvent(e);
    }
    break;
    // update loop setting
    case 2:
    {
        bool enabled = messageBuffer[1];
        uint16_t loopStart = concatBytes(messageBuffer[2], messageBuffer[3]);
        uint16_t loopEnd = concatBytes(messageBuffer[4], messageBuffer[5]);
        music::setLoopingSettings(enabled, loopStart, loopEnd);
        if(!enabled){
            client.print("OK: Looping disabled");
        }
        else{
            char buffer[128];
            sprintf(buffer, "OK: Looping from %d to %d", loopStart, loopEnd);
            client.print(buffer);
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

        if (!assert_fatal(expectedSongLength <= maxSongLength && expectedSongLength != 0, ErrorCode::SONG_LOAD_DISCONTINUITY))
        {
            char buffer[128];
            sprintf(buffer, "ER:Song length out of range: %d", expectedSongLength);
            client.print(buffer);
            client.stop();
            return;
        }

        music::resetSongLoader();
        Event e;
        e.action = []() {
            globalMode = PlayMode::SongLoading;
            lights::setAnimationMode(lights::AnimationMode::ProgressBar);
            lights::AnimationParameters::setProgressBarValue(0.0f);
        };
        PushEvent(e);

        {
            char buffer[128];
            sprintf(buffer, "Song header OK. Length: %d", expectedSongLength);
            client.print(buffer);
        }
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

            char buffer[128];
            sprintf(buffer, "ER:Frame skip detected. Last: %d, Recieved: %d", loaderFrameIndex, incommingFrameIndex);
            client.print(buffer);
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

        // Update the progress bar display
        Event e;
        e.action = []() {
            float progress = (float)loaderFrameIndex / (float)expectedSongLength;
            lights::AnimationParameters::setProgressBarValue(progress);
        };
        PushEvent(e);

        // Reply with OK + debug info
        {
            char buffer[128];
            sprintf(buffer, "OK: %d notes loaded for frame %d", frameNoteIndex, loaderFrameIndex);
            client.print(buffer);
        }
    }
    break;
    // song ending
    case 5:
    {
        if(loaderFrameIndex != expectedSongLength -1){                  
            char buffer[128];
            sprintf(buffer, "ER:Expected %d notes, but only recieved %d", expectedSongLength - 1, loaderFrameIndex);
            fatalError(ErrorCode::SONG_LOAD_DISCONTINUITY);
            client.print(buffer);      
        }
        else{
            Event e;
            e.action = []() {
                lights::setAnimationMode(lights::AnimationMode::BlinkSuccess);
                while (!lights::animationCompleted())
                {
                    lights::updateAnimation();
                }
                // globalMode == PlayMode::Waiting;
                lights::setAnimationMode(lights::AnimationMode::Waiting);
            };
            PushEvent(e);

            music::setLoopingSettings(true, 0, expectedSongLength - 1);
            expectedSongLength = 0;

            client.print("OK: Song loaded");
        }
        // char bbuffer[128];
        // for (size_t i = 0; i < 10; i++)
        // {
        //     music::songFrame frame;
        //     if (!music::getFrame(i, &frame))
        //     {
        //         return;
        //     }
        //     //music::songFrame frame = music::Temp(i);
        //     Serial.print("first: ");
        //     Serial.print(*frame.firstNote);
        //     Serial.print(", ");
        //     sprintf(bbuffer, "%p", frame.firstNote);
        //     Serial.print(bbuffer);
        //     Serial.print(" last: ");
        //     Serial.print(*frame.lastNote);
        //     Serial.print(", ");
        //     sprintf(bbuffer, "%p", frame.lastNote);
        //     Serial.println(bbuffer);
        // }

    }
    break;
    // set song index
    case 6:
    {
        uint16_t newIndex = concatBytes(messageBuffer[1], messageBuffer[2]);
        music::setFrame(newIndex);
         char buffer[128];
        sprintf(buffer, "OK: index set to [%d]", music::currentFrameIndex());
        client.print(buffer);

        Event e;
        e.action = []() {
            lights::forceRefresh();
        };
        PushEvent(e);
    }
    break;
    case 7:
        client.print("Status: OK");
        break;
    case 8:
    {
        char buffer[128];
        sprintf(buffer, "OK:%d", music::currentFrameIndex());
        client.print(buffer);
    }
    break;
    default:
        char buffer[128];
        sprintf(buffer, "ER:Unrecognized command header: %d", messageBuffer[0]);
        client.print(buffer);
        break;
    }
}
} // namespace network