#include <stdint.h>
#include <cmath>

#include "../../m_constants.h"
#include "../../settings.h"
#include "../color.h"
#include "../animator.h"

using namespace animator;

namespace animations
{

void blinkSuccess(const float time)
{
    float brightness = sin((float)time * 20.0f) * 0.5f + 0.5f;
    setAll({0.0f, brightness, 0.0f});
    if (time >= 1.0f)
    {
        setAnimationComplete();
        setAll(settings::getColorSetting(settings::Colors::Ambiant));
    }
}

} // namespace animations