#include <Arduino.h>
#include <usbh_midi.h>
#include <usbhub.h>
#include <SPI.h>

#include "pinaoCom.h"
#include "m_error.h"

//#define MICROBRUTE_DEBUG

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

bool noteStates[_PIANOSIZE];
} // namespace

namespace MIDI
{

bool initUSBHost()
{
    if (!assert_fatal(Usb.Init() == 0, ErrorCode::USB_HOST_INITIALISATION))
    {
        return false;
    }

    #ifdef MICROBRUTE_DEBUG
        Serial.println("USB init success");
    #endif
    
    vid = pid = 0;
    USBInit = true;
    return true;
    
}

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

    #ifdef MICROBRUTE_DEBUG
    if (Midi.RecvData(&rcvd, midiBuf) == 0)
    {

        if (midiBuf[2] != 0)
        {
            bool state = midiBuf[0] == 9;
            byte noteNumber = midiBuf[2] - noteNumberOffset;
            if(noteNumber > 52){
                noteNumber = 52;
            }
            noteStates[noteNumber] = state;

                #ifdef MICROBRUTE_DEBUG
        Serial.println("note pressed");
    #endif
        }
    }

#else
    //if (assert_fatal(Midi.RecvData(&rcvd, midiBuf) == 0, ErrorCode::USB_TIMEOUT))
    if (Midi.RecvData( &rcvd,  midiBuf) == 0 )
    {
        // If there was a note change
        if(midiBuf[0] == 9){
            // For now, velocity is ignored and just used to get the note state.
            bool state = midiBuf[3] != 0;
            byte noteNumber = midiBuf[2] - noteNumberOffset;
            noteStates[noteNumber] = state;
        }
    }
    #endif
}

bool getNoteState(byte noteNumber)
{
    return noteStates[noteNumber];
}

} // namespace MIDI