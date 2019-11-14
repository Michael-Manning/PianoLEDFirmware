#ifndef PIANOCOM_H
#define PIANOCOM_H

#include "m_constants.h"

#ifndef _PIANOSIZE
#define _PIANOSIZE 88
#endif

/**
 * NOTE: This system does not keep logical track of the state of notes.
 * Instead, it relies on the MIDI device itself to send events for whenever
 * the note state changes, and simply keeps track of the events.
 */

namespace MIDI
{
constexpr byte noteNumberOffset = 16;

struct NoteEvent{
    bool state;
    byte number;
    byte velocity;
};


// Call this very often. Returns number of note state changes. Also updates the recent events
unsigned int pollMIDI(); 

NoteEvent * getRecentEvents();

bool getNoteState(byte noteNumber); 
bool connectMIDI();

}

#endif
