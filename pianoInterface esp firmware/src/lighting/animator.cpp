#include <Arduino.h>

#include "../m_error.h"
#include "LEDCom.h"
#include "animator.h"

namespace
{
bool animationComplete = false;
}

namespace animator
{

float keyTimers[_KEYCOUNT];
colorF keyFadeTargets[_KEYCOUNT];
bool pressedThisFrame[_KEYCOUNT];

void setColor(uint8_t led, colorF c)
{
    LEDCom::setColor(led, c);
}

void setAll(colorF c)
{
    LEDCom::setAll(c);
}

// Index between 0 - 1530
color sweepHSL(unsigned int index)
{

    if (!assert_fatal(index <= 1530, ErrorCode::INVALID_LED_INDEX))
    {
        return {255, 255, 255};
    }
    if (index <= 255 * 1)
    {
        return {255, index, 0};
    }
    else if (index <= 255 * 2)
    {
        return {255 - (uint8_t)(index - 255), 255, 0};
    }
    else if (index <= 255 * 3)
    {
        return {0, 255, (uint8_t)(index - 255 * 2)};
    }
    else if (index <= 255 * 4)
    {
        return {0, 255 - (index - 255 * 3), 255};
    }
    else if (index <= 255 * 5)
    {
        return {index - 255 * 4, 0, 255};
    }
    else if (index <= 255 * 6)
    {
        return {255, 0, 255 - (index - 255 * 5)};
    }
    fatalError(ErrorCode::IMPOSSIBLE_INTERNAL);
    return {255, 255, 255};
}

void resetAnimation()
{
    animationComplete = false;
    memset(keyTimers, 0, sizeof(keyTimers));
}

void setAnimationComplete()
{
    animationComplete = true;
}

bool getAnimationComplete()
{
    return animationComplete;
}

} // namespace animator