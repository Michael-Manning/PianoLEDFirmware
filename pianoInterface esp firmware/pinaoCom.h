#ifndef PIANOCOM_H
#define PIANOCOM_H

#include "m_constants.h"

/**
 * NOTE: This system does not keep logical track of the state of notes.
 * Instead, it relies on the MIDI device itself to send events for whenever
 * the note state changes, and simply keeps track of the events.
 */

namespace MIDI
{

#ifdef MICROBRUTE_DEBUG
constexpr byte noteNumberOffset = 21; // MIDI note number for the first note
constexpr byte ledNoteOffset = 15 ; // First note on the piano which has an LED 
#else
constexpr byte noteNumberOffset = 21; // MIDI note number for the first note
constexpr byte ledNoteOffset = 15 ; // First note on the piano which has an LED 
#endif

struct NoteEvent{
    bool state;
    byte number;
    byte velocity;
};

void pollMIDI(); 

NoteEvent * getRecentEvents();

bool getNoteState(byte noteNumber); 
bool getLogicalState(byte noteNumber);
bool initUSBHost();
void copyLogicalStateBuffer();
void setLogicalLayerEnable(bool enabled);

}

#endif
