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
    constexpr float indicateFadeTime = 0.6f; // seconds
    colorF memes[_KEYCOUNT];
}

namespace animations
{

void rainbowFade(const float deltaTime, const float time)
{
    for (unsigned int i = 0; i < _KEYCOUNT; i++)
    {
        float index = i * HSLRange_Over_KeyCount + time * 1000;
        while (index > 1530)
        {
            index -= 1530;
        }
        memes[i] = sweepHSL((unsigned int)(index));
    }
     
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        // Note event: reset the timer
        if (MIDI::getLogicalState(i + MIDI::ledNoteOffset))
        {
            keyTimers[i] = indicateFadeTime;
        }


        colorF col = col = memes[i] * ((float)keyTimers[i] / indicateFadeTime);

        if (!music::isBlackNote(i + MIDI::ledNoteOffset))
        {
            col = colorMax(col, settings::getColorSetting(settings::Colors::Ambiant));
        }

        setColor(_KEYCOUNT - 1 - i, col);

        keyTimers[i] -= deltaTime;
        if (keyTimers[i] < 0.0f)
        {
            keyTimers[i] = 0.0f;
        }
    }
}

} // namespace animations