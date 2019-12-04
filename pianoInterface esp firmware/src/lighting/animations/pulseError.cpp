#include <stdint.h>
#include <cmath>

#include "../../m_constants.h"
#include "../color.h"
#include "../animator.h"

using namespace animator;

namespace animations
{
    void pulseError(const float time)
    {
        float brightness = sin((float)time * 2.0f) * 0.5f + 0.5f;
        setAll({brightness, 0.0f, 0.0f});
    }
}