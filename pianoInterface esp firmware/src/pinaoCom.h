#ifndef PIANOCOM_H
#define PIANOCOM_H

#include <stdint.h>

#include "m_constants.h"

/**
 * NOTE: This system does not keep logical track of the state of notes.
 * Instead, it relies on the MIDI device itself to send events for whenever
 * the note state changes and simply keeps track of the events.
 */

namespace MIDI
{

constexpr uint8_t noteNumberOffset = 21; // MIDI note number for the first note
constexpr uint8_t ledNoteOffset = 0 ; // First note on the piano which has an LED 

bool initUSBHost();

void pollMIDI(); 

bool getNoteState(uint8_t noteNumber); 

bool getLogicalState(uint8_t noteNumber);

void copyLogicalStateBuffer();

void setLogicalLayerEnable(bool enabled);
bool getLogicalLayerEnabled();

}

#endif
