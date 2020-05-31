#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

#include "m_constants.h"

namespace music
{

// Describes where in the note data array describes a step in a song 
struct songFrame
{
    uint8_t *firstNote;
    uint8_t *lastNote;
};

constexpr unsigned int maxSongLength = 5000;
constexpr unsigned int maxNoteCount = 8192;

void resetSongLoader();

void loadFrame(uint8_t *notes, uint8_t noteCount);

bool getFrame(unsigned int frameIndex, songFrame *frame);

songFrame currentFrame();

int currentFrameIndex();

void nextFrame();

void setFrame(unsigned int index);

void setLoopingSettings(bool enabled, unsigned int start, unsigned int end);

constexpr unsigned int noteNumberInOctive(unsigned int note)
{
    return note <= 2 ? note + 9 : (note - 3) - ((note - 3) / 12) * 12;
};

constexpr bool isBlackNote(unsigned int note)
{
    return (noteNumberInOctive(note) == 1 ||
            noteNumberInOctive(note) == 3 ||
            noteNumberInOctive(note) == 6 ||
            noteNumberInOctive(note) == 8 ||
            noteNumberInOctive(note) == 10);
};

}
#endif
