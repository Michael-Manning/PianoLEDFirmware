#include <stdint.h>
#include <cmath>

#include "../../m_constants.h"
#include "../color.h"
#include "../animator.h"

using namespace animator;

namespace animations
{
void progressBar(float progress)
{
    float filledInKeys = _KEYCOUNT * progress;
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        // full On
        if (i <= filledInKeys)
        {
            setColor(_KEYCOUNT - i - 1, Colors::Red);
        }
        // Partially lit
        else if (i < filledInKeys + 1)
        {
            float opacity = filledInKeys - i;
            setColor(_KEYCOUNT - i - 1, colorF{opacity, 0.0f, 0.0f});
        }
        // Off
        else
        {
            setColor(_KEYCOUNT - i - 1, Colors::Off);
        }
    }
}
} // namespace animations