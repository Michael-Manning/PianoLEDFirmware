#include <Arduino.h>
#include "network.h"
#include <WiFi.h>
#include "music.h"
#include "m_error.h"
#include "m_constants.h"
#include "circularBuffer.h"

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
 */

namespace
{
bool connected;
byte messageBuffer[8];

unsigned int expectedSongLength = 0;
unsigned int loaderFrameIndex;      // What frame of the song was last loaded
byte frameNoteIndex;                // How many notes for the currently loading frame have been loaded
byte currentFrameNotes[_PIANOSIZE]; // Storage for loading notes in the frame before being memcopied by ::music

WiFiServer server(80);
WiFiClient client; // persistant accross poll calls.
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
    if(!client)
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
    
    if(!client.available()){
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
        e.action = [](){
            globalMode = 
        };
        PushEvent(e);
    }
        break;
    // update loop setting
    case 2:
    {
        music::looping = messageBuffer[1];
        unsigned short loopStart = messageBuffer[2];
        loopStart << 8;
        loopStart &= messageBuffer[3];
        unsigned short loopEnd = messageBuffer[4];
        loopEnd << 8;
        loopEnd &= messageBuffer[5];
        music::loopStart = loopStart;
        music::loopEnd = loopEnd;
        client.print("OK");
    }
        break;
    // begin song loading
    case 3:
    {
        //When a song is done loading, expected length is reset. If it is 0 here then something went wrong.
        if(!assert_fatal(expectedSongLength == 0, ErrorCode::SONG_LOAD_DISCONTINUITY)){
            client.print("ER:Song length already set");
            client.stop();
            return;
        }

        unsigned short songLength = messageBuffer[1];
        songLength = songLength << 8;
        songLength |= messageBuffer[2];
        expectedSongLength = songLength;
        
        if(!assert_fatal(expectedSongLength <= maxSongLength && expectedSongLength != 0, ErrorCode::SONG_LOAD_DISCONTINUITY)){
            char buffer[128];
            sprintf(buffer, "ER:Song length out of range: %d", expectedSongLength);
            // client.println("ER:Song length out of range");
            client.print(buffer);
            client.stop();
            return;
        }

        music::resetSongLoader();
        Event e;
        e.action = [](){
            globalMode = PlayMode::SongLoading;
            lights::setAnimationMode(lights::AnimationMode::ProgressBar);
            lights::AnimationParameters::setProgressBarValue(0.0f);
        };
        PushEvent(e);
        client.print("Song header OK");
    }
    break;
    // song data
    case 4:
    {
        unsigned short incommingFrameIndex = messageBuffer[1];
        incommingFrameIndex = incommingFrameIndex << 8;
        incommingFrameIndex |= messageBuffer[2];

        if(!assert_fatal(incommingFrameIndex <= expectedSongLength, ErrorCode::INVALID_SONG_FRAME_INDEX)){
            client.print("ER:Frame index out of expected range");
            client.stop();
            return;
        }
        

        // Apending current frame with more simultaneous notes
        if (incommingFrameIndex - loaderFrameIndex == 0)
        {
            for (size_t i = 3; i < 7; i++)
            {
                if (messageBuffer[i] == 255)
                    break;
                currentFrameNotes[frameNoteIndex] = messageBuffer[i];
                frameNoteIndex++;
            }
            // If there are more notes being pressed at once than there are notes on the piano, we have an issue.
            if(!assert_fatal(frameNoteIndex <= 88, ErrorCode::SONG_LOAD_DISCONTINUITY)){
                client.print("ER:Oh fuck");
                client.stop();
                return;
            }
            client.write("OK: Notes loaded");
        }
        // Starting new frame
        else if (incommingFrameIndex - loaderFrameIndex == 1)
        {
            // First complete that last frame being worked on
            music::loadFrame(currentFrameNotes, frameNoteIndex);
            loaderFrameIndex++;

            // Start loading the next frame
            frameNoteIndex = 0;
            for (size_t i = 3; i < 7; i++)
            {
                if (messageBuffer[i] == 255)
                    break;
                currentFrameNotes[frameNoteIndex] = messageBuffer[i];
                frameNoteIndex++;
            }

            // Update the progress bar display
            Event e;
            e.action = [](){
                float progress = (float)loaderFrameIndex / (float)expectedSongLength;
                lights::AnimationParameters::setProgressBarValue(progress);
            };
            PushEvent(e);
            client.write("OK: Frame loaded");
        }
        else
        {
            // ERROR: frame was skipped
            fatalError(ErrorCode::SONG_LOAD_DISCONTINUITY);

            char buffer[128];
            sprintf(buffer, "ER:Frame skip detected. Last: %d, Recieved: %d", loaderFrameIndex, incommingFrameIndex);
            client.print(buffer);
            client.stop();
            return;
        }
    }
    break;
    // song ending
    case 5:
        // Maybe do some sort of validation?
        expectedSongLength = 0;
        Event e;
        e.action = []() {
            lights::setAnimationMode(lights::AnimationMode::BlinkSuccess);
            while (!lights::animationCompleted())
            {
                lights::updateAnimation();
            }

            globalMode == PlayMode::Idle;
            lights::setAnimationMode(lights::AnimationMode::None);
        };
        PushEvent(e);
        client.print("OK");
        break;
    // set song index
    case 6:
    {
        unsigned short index = messageBuffer[2];
        index << 8;
        index &= messageBuffer[3];
        music::setFrame(index);
        client.print("OK");
    }
    break;
    case 7:
        client.print("Status: OK");
        break;
    default:
        char buffer[128];
        sprintf(buffer, "ER:Unrecognized command header: %d", messageBuffer[0]);
        client.print(buffer);
        break;
    }
}
} // namespace network