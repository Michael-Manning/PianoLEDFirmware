#include <Arduino.h>
#include "network.h"
#include <WiFi.h>
#include "music.h"
#include "m_error.h"
#include "m_constants.h"

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
 */

namespace
{
    byte messageBuffer[8];

    unsigned int expectedSongLength;
    unsigned int loaderFrameIndex; // What frame of the song was last loaded
    byte frameNoteIndex; // How many notes for the currently loading frame have been loaded
    byte currentFrameNotes[_PIANOSIZE]; // Storage for loading notes in the frame before being memcopied by ::music

    WiFiServer server(80);
}

namespace network
{

const char * hostName = "PianoESP";

bool connect()
{
    WiFi.setHostname(hostName);
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(_SSID, _NETWORKKEY);
    unsigned int attempts = 0;
    wl_status_t status;
    do{
        status = WiFi.status();
        delay(100);
        attempts++;
        if(attempts == 60){
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

    if(status != WL_CONNECTED){
        fatalError(ErrorCode::IMPOSSIBLE_INTERNAL);
        return false;
    }
    server.begin();
    
    return true;
}

void pollEvents()
{

#if 0

        // Assuming at this point that the message buffer has been loaded with a message
        switch (messageBuffer[0])
        {
        // change mode
        case 1:
            switch (messageBuffer[1])
            {
            case 1:
                mode = PlayMode::indicate;
                break;
            case 2:
                mode = PlayMode::waiting;
                break;
            case 3:
                mode = PlayMode::fade;
                break;
            default:
                break;
            }
            break;
        // update loop setting
        case 2:
            music::looping = messageBuffer[1];
            unsigned short loopStart = messageBuffer[2];
            loopStart << 8;
            loopStart &=  messageBuffer[3];
            unsigned short loopEnd = messageBuffer[4];
            loopEnd << 8;
            loopEnd &=  messageBuffer[5];
            music::loopStart = loopStart;
            music::loopEnd = loopEnd;
            break;
        // begin song loading
        case 3:
            unsigned short songLength = messageBuffer[2];
            songLength << 8;
            songLength &= messageBuffer[3];
            expectedSongLength = songLength;
            music::resetSongLoader();
            break;
        // song data
        case 4:
            unsigned short frameIndex = messageBuffer[2];
            frameIndex << 8;
            frameIndex &= messageBuffer[3];

            if(frameIndex >= expectedSongLength){
                // Error: frame was out of range
            }
            // Apending current frame with more simultaneous notes
            if(frameIndex - loaderFrameIndex == 0){
                for (size_t i = 3; i < 7; i++)
                {
                    if (messageBuffer[i] == 255)
                        break;
                    currentFrameNotes[i - 3] = messageBuffer[i];
                    frameNoteIndex++;
                }
            }
            // Starting new frame
            else if(frameIndex - loaderFrameIndex == 1){
                // First complete that last frame being worked on
                music::loadFrame(currentFrameNotes, frameNoteIndex);
                loaderFrameIndex++;

                // Start loading the next frame
                frameNoteIndex = 0;
                for (size_t i = 3; i < 7; i++)
                {
                    if(messageBuffer[i] == 255)
                        break;
                    currentFrameNotes[i - 3] = messageBuffer[i];
                    frameNoteIndex ++;
                }
            }
            else{
                // ERROR: frame was skipped
            }
            break;
        // song ending
        case 5:
            // Maybe do some sort of validation?
            break;
        // set song index
        case 6:
            unsigned short index = messageBuffer[2];
            index << 8;
            index &= messageBuffer[3];
            music::setFrame(index);
            break;
        default:
            break;
        }

        #endif
    }
}