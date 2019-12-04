#include <stdint.h>
#include <cmath>

#include "../../m_constants.h"
#include "../../settings.h"
#include "../color.h"
#include "../animator.h"

using namespace animator;

namespace animations
{

void startUp(const float time)
{
    float filledInKeys = _KEYCOUNT * (time / 2.0f);
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        // full On
        if (i <= filledInKeys)
        {
            setColor(i, Colors::Red);
        }
        // Partially lit
        else if (i < filledInKeys + 1)
        {
            float opacity = filledInKeys - i;
            setColor(i, colorF{opacity, 0.0f, 0.0f});
        }
        // Off
        else
        {
            setColor(i, Colors::Off);
        }
    }

    if (time >= 2.0f)
    {
        setAnimationComplete();
        setAll(settings::getColorSetting(settings::Colors::Ambiant));
    }
}

} // namespace animations