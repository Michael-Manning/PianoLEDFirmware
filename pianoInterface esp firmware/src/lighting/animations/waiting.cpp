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
            keyFadeTargets[keyIndex] = Colors::Red;
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
        //memset(pressedThisFrame, false, sizeof(pressedThisFrame));
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
                keyFadeTargets[i] = settings::getColorSetting(settings::Colors::Ambiant);
            }
        }
    }

    // colors
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        // setColor(_KEYCOUNT - 1 - i, keyFadeTargets[i]);
        if (keyTimers[i] == 0)
        {
            setColor(_KEYCOUNT - 1 - i, keyFadeTargets[i]);
        }
        else
        {
            float t = ((float)keyTimers[i] / inFrameFadeTime);
            colorF col = mix(Colors::Green, keyFadeTargets[i], 1.0f - t);
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