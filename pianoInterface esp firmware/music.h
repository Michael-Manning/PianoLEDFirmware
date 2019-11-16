#ifndef MUSIC_H
#define MUSIC_H

#include <arduino.h>
#include "m_constants.h"

#define maxSongLength 1000
#define maxNoteCount 10000

namespace music
{

extern byte globalNoteStates[_PIANOSIZE];

// Get only
extern unsigned int songLength;

// Get and set
extern bool looping; // If end is reached, song will always loop
extern unsigned int loopStart;
extern unsigned int loopEnd;

struct songFrame
{
    byte *firstNote;
    byte *lastNote;
};

// Resets the song loader so that newly loaded frames will start from frame zero
void resetSongLoader();

// Adds a new frame to the end of the song
void loadFrame(byte *notes, byte noteCount);

// Retrieves and alreader loaded frame
bool getFrame(unsigned int frameIndex, songFrame *frame);

// Advances the song to the next frame
void nextFrame();

// sets the current frame
void setFrame(unsigned int index);

bool checkFrameCompletion();

songFrame *currentFrame();

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
