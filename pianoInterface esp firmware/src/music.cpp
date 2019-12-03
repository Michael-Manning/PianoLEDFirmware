#include <Arduino.h>

#include "circularBuffer.h"
#include "m_error.h"
#include "m_constants.h"
#include "music.h"

namespace
{
music::songFrame frameData[music::maxSongLength]; // all the frames in the song
byte noteData[music::maxNoteCount];               // all the actual notes in the  song

unsigned int frameLoaderIndex = 0; // tracks what frame is being loaded in
unsigned int notesLoaded = 0;      // how many notes have been loaded in so far
unsigned int liveFrameIndex = 0;   // the current frame being played

bool looping = false;       // whether or not to perform looping
unsigned int loopStart = 0; // first frame of the song loop
unsigned int loopEnd = 0;   // last frame of the song loop
} // namespace

namespace music
{

// Clears loading data and gets ready to load a new song
void resetSongLoader()
{
    frameLoaderIndex = 0;
    notesLoaded = 0;
    liveFrameIndex = 0;
    loopStart = 0;
    loopEnd = 0;
}

// Adds a new frame to the end of the song
void loadFrame(byte *notes, byte noteCount)
{
    memcpy(noteData + notesLoaded, notes, noteCount);
    frameData[frameLoaderIndex] = {noteData + notesLoaded, noteData + notesLoaded + noteCount - 1};
    frameLoaderIndex++;
    notesLoaded += noteCount;
}

// Retrieves a previously loaded frame
bool getFrame(unsigned int frameIndex, songFrame *frame)
{
    if (!assert_fatal(frameIndex < notesLoaded, ErrorCode::INVALID_SONG_FRAME_INDEX))
    {
        return false;
    }

    *frame = frameData[frameIndex];
    return assert_fatal(frame != nullptr, ErrorCode::NULL_SONG_FRAME);
}

// Retrieves the frame at the live frame index of the loaded song
songFrame currentFrame()
{
    songFrame frame;
    getFrame(liveFrameIndex, &frame);
    return frame;
}

// Retrieves live frame index of the loaded song
int currentFrameIndex()
{
    return liveFrameIndex;
}

// Advances the song to the next frame
void nextFrame()
{
    setFrame(liveFrameIndex + 1);
}

// Sets the live frame index.
// Note: values may be adjusted due to looping settings
// Note: animation forced refresh may be required after setting the frame index manually
void setFrame(unsigned int index)
{
    liveFrameIndex = index;
    if (looping)
    {
        if (liveFrameIndex >= loopEnd)
        {
            liveFrameIndex = loopStart;
        }
        else if (liveFrameIndex < loopStart)
        {
            liveFrameIndex = loopStart;
        }
    }
    if (liveFrameIndex >= notesLoaded)
    {
        liveFrameIndex = 0;
    }
}

// Updates the settings used to automatically loop a portion of the song
void setLoopingSettings(bool enabled, unsigned int start, unsigned int end)
{
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