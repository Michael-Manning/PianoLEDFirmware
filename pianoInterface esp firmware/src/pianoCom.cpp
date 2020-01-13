#include <Arduino.h>
#include <usbh_midi.h>
#include <usbhub.h>
#include <SPI.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "pinaoCom.h"
#include "m_error.h"
#include "m_constants.h"

/**
 * 
 * NOTES:
 * The MIDI protocol seems to have the first byte be an indicator of wether
 * a note was pressed or released, but my portable grand is all sorts of weird,
 * so here is my best understanding of what is going on
 * 
 * Byte 0: Whether something happened. 15 = nothing, 9 = something
 * Byte 1: Same as byte 0, but with numbers 354 and 144
 * Byte 2: Note number
 * Byte 3: note velocity. This is the only indicator of wether the note was pressed or released.
 */

namespace
{
USB Usb;
USBH_MIDI Midi(&Usb);
bool USBInit = false;
uint16_t pid, vid;
SemaphoreHandle_t xMutex; // for copying the logical state buffer
bool logicalLayerEnabled = false;

// This array contains the states of notes as they are in REAL TIME. This is written to
// as midi notes are pressed and released. Nothing else should be writing to this. When reading
// from this array, it can be assumed that these are the true state of what notes are currently being pressed.
bool noteStates[_PIANOSIZE];

// This array is written to by the MIDI in realtime, but with true values only. That is to say the piano com thread only sets when
// notes are pressed, but doesn't clear the values when they are released. When the com thread writes to this array, a dirty flag is set.
// A slow frequency poll from the main thread occasionally coppies this buffer and clears all the values. A mutex is used when this operation
// is executed.
bool logicalStateBuffer[_PIANOSIZE];

// This array serves the purpose of replacing the functionality of note pressed events. It is ocasionlly coppied from the logicalStateBuffer 
// and uses no mutex so it may be read from at any frequency. this is a way of differentiating the diference between
// multiple note presses and a note just being held down without using a circular event buffer. When a note is pressed, the array
// is written to and a flag is set. The LED logic layer can then set the note value back off without the note having to be released.
// This makes subsequint reads from the array apear that the note is not pressed which means that any true value in the array is
// in effect a note pressed "event". This can only cause errors if a note is pressed more often than the array is polled.
bool logicalStateLayer[_PIANOSIZE];

} // namespace

namespace MIDI
{

// Starts up communications with the MAX3421E module.
// Returns success
bool initUSBHost()
{
    if (!assert_fatal(Usb.Init() == 0, ErrorCode::USB_HOST_INITIALISATION))
    {
        return false;
    }
    xMutex = xSemaphoreCreateMutex();

    vid = pid = 0;
    USBInit = true;
    return true;
}

// THREAD 1: Gets new note events from the MIDI device
void pollMIDI()
{

    assert_fatal(USBInit, ErrorCode::USB_HOST_INITIALISATION);

    Usb.Task();
    if (Usb.getUsbTaskState() != USB_STATE_RUNNING)
    {
        // Nothing to do right now
        return;
    }

    char buf[24];
    uint8_t midiBuf[64];
    uint16_t rcvd;

    // I have no idea what this does, but it was in the example?
    if (Midi.vid != vid || Midi.pid != pid)
    {
        vid = Midi.vid;
        pid = Midi.pid;
    }
    Serial.println("Recieved ");

#ifdef MICROBRUTE_DEBUG
    Midi.RecvData(&rcvd, midiBuf);
    //   if (Midi.RecvData(&rcvd, midiBuf) == 0)
    //rcvd = Midi.RecvData(midiBuf);
    //Midi.RecvRawData(midiBuf);
    if (rcvd != 0)
    {
        if (midiBuf[0] != 15)
        {
            bool state = midiBuf[0] == 9;
            byte noteNumber = midiBuf[2] - noteNumberOffset;
            if (noteNumber > 52)
            {
                noteNumber = 52;
            }
            noteStates[noteNumber] = state;

            //Serial.print("Rec3333d ");
            Serial.print("Recieved ");
            Serial.print(rcvd);
            Serial.print(" : ");
            for (int i = 0; i < rcvd; i++)
            {
                sprintf(buf, " %d", midiBuf[i]);
                Serial.print(buf);
            }
            Serial.println("");
        }
    }

#else
    //if (assert_fatal(Midi.RecvData(&rcvd, midiBuf) == 0, ErrorCode::USB_TIMEOUT))
    Midi.RecvData(&rcvd, midiBuf);
    //if (Midi.RecvData(&rcvd, midiBuf) == 0)
    if(rcvd != 0)
    {
        //Serial.print("Recieved ");
        //Serial.print(rcvd);
        //Serial.print(" : ");
        // for (int i = 0; i < rcvd; i++)
        // {
        //     sprintf(buf, " %d", midiBuf[i]);
        //     Serial.print(buf);
        // }
        // Serial.println("");

        if(logicalLayerEnabled){
            xSemaphoreTake(xMutex, portMAX_DELAY);
        }
        for (size_t i = 0; i < 64 / 4; i+= 4)
        {
            if (midiBuf[i] == 9)
            {
                // For now, velocity is ignored and just used to get the note state.
                bool state = midiBuf[i + 3] != 0;
                byte noteNumber = midiBuf[i+2] - noteNumberOffset;
                noteStates[noteNumber] = state;
                if (logicalLayerEnabled)
                {
                    logicalStateBuffer[noteNumber] |= state;
                }
            }
        }
        if(logicalLayerEnabled){
            xSemaphoreGive(xMutex);
        }
    }
#endif
}

// Gets the pressed state of a note in real time
bool getNoteState(byte noteNumber)
{
    return noteStates[noteNumber];
}

// THREAD 0: Gets the logical 'event' state of a note from the third note layer.
// Note: Getting the logical state note value clears the value in this layer.
// Note: The notes in this layer are only updated 100 times per second (see firm.ino).
bool getLogicalState(byte noteNumber)
{
    bool val = logicalStateLayer[noteNumber];
    logicalStateLayer[noteNumber] = 0; // reset to simulate event logic
    return val;
}

// THREAD 0: Thread safe copy from the real time logical state buffer to the led managed logical state layer.
// Copy this a few hundred times per second.
void copyLogicalStateBuffer(){
    xSemaphoreTake(xMutex, portMAX_DELAY);
    memcpy(logicalStateLayer, logicalStateBuffer, sizeof(byte) * _PIANOSIZE);
    memset(logicalStateBuffer, 0, sizeof(logicalStateBuffer));
    xSemaphoreGive(xMutex);
}

// Enabled or disables the logical layer functionality which can effect performance costs due to mutex use
void setLogicalLayerEnable(bool enabled){
    logicalLayerEnabled = enabled;
}
bool getLogicalLayerEnabled()
{
    return logicalLayerEnabled;
}

} // namespace MIDI