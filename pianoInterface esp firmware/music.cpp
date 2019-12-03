#include <Arduino.h>
#include "m_error.h"
#include "m_constants.h"
#include "music.h"
#include "circularBuffer.h"

//#include "pinaoCom.h"

namespace
{
music::songFrame frameData[maxSongLength];
byte noteData[maxNoteCount];

unsigned int frameLoaderIndex = 0;
unsigned int liveFrameIndex = 0;
unsigned int notesLoaded = 0;

bool looping = false;
unsigned int loopStart = 0;
unsigned int loopEnd = 0;
} // namespace

namespace music
{

void resetSongLoader()
{
    frameLoaderIndex = 0;
    notesLoaded = 0;
}

songFrame Temp(int i){
    //frameData[i] = {noteData, noteData};
    return frameData[i];
}

void loadFrame(byte *notes, byte noteCount)
{
    memcpy(noteData + notesLoaded, notes, noteCount);
    frameData[frameLoaderIndex] = {noteData + notesLoaded, noteData + notesLoaded + noteCount -1};
    frameLoaderIndex++;
    notesLoaded+= noteCount;
}

bool getFrame(unsigned int frameIndex, songFrame *frame)
{
    if(!assert_fatal(frameIndex < notesLoaded, ErrorCode::INVALID_SONG_FRAME_INDEX))
    {
        return false;
    }

    *frame = frameData[frameIndex];
    return assert_fatal(frame != nullptr, ErrorCode::NULL_SONG_FRAME);
}

songFrame currentFrame()
{
    songFrame frame;
    getFrame(liveFrameIndex, &frame);
    return frame;
}
int currentFrameIndex()
{
    return liveFrameIndex;
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
        if (liveFrameIndex >= loopEnd)
        {
            liveFrameIndex = loopStart;
        }
        else if(liveFrameIndex < loopStart){
            liveFrameIndex = loopStart;
        }
    }
    if (liveFrameIndex >= notesLoaded)
    {
        liveFrameIndex = 0;
    }
}


void setLoopingSettings(bool enabled, unsigned int start, unsigned int end){
    if (start >= end || end > frameLoaderIndex)
    {
        fatalError(ErrorCode::INVALID_LOOP_SETTING);
    }
    
    looping = enabled;
    loopStart = start;
    loopEnd = end;

    // set the frame to the current frame to check if the current frame
    // is now out of looping range
    setFrame(liveFrameIndex);
}

} // namespace music