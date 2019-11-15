#include <Arduino.h>
//#include "M_Assert.h"
#include "music.h"

#include "pinaoCom.h"

namespace
{
byte noteData[maxNoteCount];
music::songFrame frameData[maxSongLength];
unsigned int frameLoaderIndex = 0;
unsigned int frameLoaderNoteIndex = 0;
unsigned int liveFrameIndex;
} // namespace

namespace music
{
//Extern
 unsigned int songLength = 0;
 bool looping = false;
 unsigned int loopStart = 0;
 unsigned int loopEnd = 0;

void resetSongLoader()
{
    frameLoaderIndex = 0;
    songLength = 0;
}

void loadFrame(byte *notes, byte noteCount)
{
    memcpy(noteData, notes, noteCount);
    frameData[frameLoaderIndex] = {noteData + frameLoaderIndex, noteData + frameLoaderNoteIndex + noteCount};
    frameLoaderIndex++;
    frameLoaderNoteIndex += noteCount;
    songLength++;
}

bool getFrame(unsigned int frameIndex, songFrame *frame)
{
    if (frameIndex >= songLength)
    {
 //       assert(false);
        return false;
    }

    frame = frameData + frameIndex;
    return true;
}

songFrame *currentFrame()
{
    songFrame *frame;
    getFrame(liveFrameIndex, frame);
}

void nextFrame()
{
    setFrame(liveFrameIndex + 1);
}

void setFrame(unsigned int index)
{
    liveFrameIndex = index;
    if (looping)
    {
        if (liveFrameIndex >= loopEnd || liveFrameIndex > songLength)
        {
            liveFrameIndex = loopStart;
            return;
        }
    }
    if (liveFrameIndex > songLength)
    {
        liveFrameIndex = 0;
    }
}

bool checkFrameCompletion()
{
    songFrame *frame = currentFrame();
    byte *note = frame->firstNote;
    while (note != frame->lastNote)
    {
        if(!MIDI::getNoteState(*note))
            return false;
        note ++;
    }
    return true;
}

} // namespace music