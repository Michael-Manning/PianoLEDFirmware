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
    colorF IFB = settings::getColorSetting(settings::Colors::InFrameBlack);
    colorF AMB = settings::getColorSetting(settings::Colors::Ambiant);

    setAll(Colors::Off);

    using namespace music;
    songFrame frame = currentFrame();

    bool allInFrame = true;

    uint8_t *notes = frame.firstNote;

    unsigned int index = 0;
    while (true)
    {
        // get rid of last bit which is used to specify the hand
        uint8_t note = notes[index] & 0b01111111;   
        uint8_t hand = notes[index] & 0b10000000;
        
        unsigned int keyIndex = (note - MIDI::ledNoteOffset);
        if (note >= MIDI::ledNoteOffset && note < _KEYCOUNT + MIDI::ledNoteOffset)
        {
          //  if (music::isBlackNote(keyIndex))
           // {
                //keyFadeTargets[keyIndex] = WB;
                //keyFadeTargets[keyIndex] = hand == 0 ? Colors::Red : Colors::Blue;

                if( notes[index] > 100){
                    keyFadeTargets[keyIndex] = Colors::Red;
                }
                else{
                    keyFadeTargets[keyIndex] = Colors::Blue;
                }
          //  }
           // else
           // {
                //keyFadeTargets[keyIndex] = WW;
           // }
            if (pressedThisFrame[note - MIDI::ledNoteOffset] && MIDI::getNoteState(note))
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

            colorF inFrameCol = allInFrame ? color{230, 255, 230} : Colors::Green;

            colorF col = mix(inFrameCol, keyFadeTargets[i], 1.0f - t);

            //colorF col = mix(black ? IFB : IFW, keyFadeTargets[i], 1.0f - t);
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