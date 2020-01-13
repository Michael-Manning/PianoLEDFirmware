#include <stdint.h>
#include <cmath>

#include "../../m_constants.h"
#include "../../pinaoCom.h"
#include "../../music.h"
#include "../../settings.h"
#include "../color.h"
#include "../animator.h"

using namespace animator;

namespace
{
constexpr float inFrameFadeTime = 0.26f; // seconds
}

namespace animations
{

void waiting(float deltaTime, bool firstFrame, bool fullRefresh)
{
    // update notes pressed down during this frame
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        if (MIDI::getLogicalState(i + MIDI::ledNoteOffset))
        {
            pressedThisFrame[i] = true;
        }
    }

    colorF WW = settings::getColorSetting(settings::Colors::WaitingWhite);
    colorF WB = settings::getColorSetting(settings::Colors::WaitingBlack);
    colorF IFW = settings::getColorSetting(settings::Colors::InFrameWhite);
    colorF IFB = settings::getColorSetting(settings::Colors::WaitingBlack);
    colorF AMB = settings::getColorSetting(settings::Colors::Ambiant);

    setAll(Colors::Off);

    using namespace music;
    songFrame frame = currentFrame();

    bool allInFrame = true;

    uint8_t *notes = frame.firstNote;

    unsigned int index = 0;
    while (true)
    {
        unsigned int keyIndex = (notes[index] - MIDI::ledNoteOffset);
        if (notes[index] >= MIDI::ledNoteOffset && notes[index] < _KEYCOUNT + MIDI::ledNoteOffset)
        {
            if (music::isBlackNote(keyIndex))
            {
                keyFadeTargets[keyIndex] = WB;
            }
            else
            {
                keyFadeTargets[keyIndex] = WW;
            }
            if (pressedThisFrame[notes[index] - MIDI::ledNoteOffset] && MIDI::getNoteState(notes[index]))
            {
                keyTimers[keyIndex] = inFrameFadeTime;
            }
            else
            {
                allInFrame = false;
            }
        }
        if (notes[index] == *frame.lastNote)
        {
            break;
        }
        index++;
    }

    if (allInFrame)
    {
        nextFrame();
    }
    if (allInFrame || firstFrame || fullRefresh)
    {
        for (size_t i = 0; i < _KEYCOUNT; i++)
        {
            pressedThisFrame[i] = false;

            if (music::isBlackNote(i + MIDI::ledNoteOffset))
            {
                keyFadeTargets[i] = Colors::Off;
            }
            else
            {
                keyFadeTargets[i] = AMB;
            }
        }
    }

    // colors
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        bool black = music::isBlackNote(i + MIDI::ledNoteOffset);

        // skip needless division
        if (keyTimers[i] == 0)
        {
            setColor(_KEYCOUNT - 1 - i, keyFadeTargets[i]);
        }
        else
        {
            float t = ((float)keyTimers[i] / inFrameFadeTime);
            colorF col = mix(black ? IFB : IFW, keyFadeTargets[i], 1.0f - t);
            setColor(_KEYCOUNT - 1 - i, col);
        }
        keyTimers[i] -= deltaTime;
        if (keyTimers[i] < 0)
        {
            keyTimers[i] = 0;
        }
    }
}

} // namespace animations