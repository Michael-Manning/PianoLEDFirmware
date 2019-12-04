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
}

namespace animations
{

void keyIndicateFade(const float deltaTime)
{
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        // Note event: reset the timer
        if (MIDI::getLogicalState(i + MIDI::ledNoteOffset))
        {
            keyTimers[i] = indicateFadeTime;
        }

        if (music::isBlackNote(i + MIDI::ledNoteOffset))
        {
            setColor(_KEYCOUNT - 1 - i, settings::getColorSetting(settings::Colors::IndicateBlack) * ((float)keyTimers[i] / indicateFadeTime));
        }
        else
        {
            colorF col = settings::getColorSetting(settings::Colors::IndicateWhite) * ((float)keyTimers[i] / indicateFadeTime);
            col = colorMax(col, settings::getColorSetting(settings::Colors::Ambiant));
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