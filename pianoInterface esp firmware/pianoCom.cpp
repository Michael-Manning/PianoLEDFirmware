#include <Arduino.h>
//#include "M_Assert.h"
#include "pinaoCom.h"

namespace
{
    bool noteStates[_PIANOSIZE];
    MIDI::NoteEvent recentEvents[_PIANOSIZE]; // Very unllikely that every key is pressed at the exact same time.
    byte midiReadBuffer[3]; // MIDI messages are 3 bytes
}

namespace MIDI
{

unsigned int pollMIDI(){
    int stateChanges = 0;
   // Serial.readLine();

    while(Serial.available()){
        int read = Serial.readBytes(midiReadBuffer, 3);

        // If the status byte is not a note event, we don't care about it.
        if(midiReadBuffer[0] != 0x8 && midiReadBuffer[0] != 0x9){
            continue;
        }
        // 0x9 == note on. 0x8 == note off.
        NoteEvent note {midiReadBuffer[0] == 0x9, midiReadBuffer[1], midiReadBuffer[2]};
        //assert(read == 3);

        if(note.number < noteNumberOffset || note.number - noteNumberOffset > _PIANOSIZE)
        {
            // Note number out of range. (such as buttons other than piano keys)
            continue;
        }

        // Everything OK
        note.number -= noteNumberOffset;
        noteStates[note.number] = note.state;
        recentEvents[stateChanges] = note;
        stateChanges ++;
    }
    return stateChanges;
}

NoteEvent * getRecentEvents(){
    return recentEvents;
}

bool getNoteState(byte noteNumber){
    return noteStates[noteNumber];
}
bool connectMIDI(){
    Serial.begin(31250);
                 
}

}   