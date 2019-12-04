#include <Arduino.h>

#include "../m_error.h"
#include "animator.h"
#include "color.h"
#include "LEDCom.h"
#include "lighting.h"

namespace
{

float animationStartTime = 0;
bool animationFirstFrame = false;
bool fullRefresh = false; // general use flag (mostly for waiting animation)
lights::AnimationMode animationMode = lights::AnimationMode::None;

// <AnimationParameters>
float progressBarValue = 0.0f;
// </AnimationParameters

} // namespace


namespace lights
{

namespace AnimationParameters
{

void setProgressBarValue(float value)
{
    progressBarValue = value;
}

} // namespace AnimationParameters

// initialise the LED strip
void init()
{
    LEDCom::stripInit();
}

// Sets the animation mode and starts running it
void setAnimationMode(AnimationMode mode)
{
    animationMode = mode;
    animationFirstFrame = true;
    animationStartTime = micros() / 1000000.0f;
    animator::resetAnimation();
}

// Skips less steps in certain animation modes
void forceRefresh(){
    fullRefresh = true;
}

float lastTime = 0;
void updateAnimation()
{
    if (animationCompleted())
    {
        animationMode = AnimationMode::None;
        return;
    }

    const float time = (float)micros() / 1000000.0f - animationStartTime;
    const float deltaTime = time - lastTime;
    lastTime = time;

    switch (animationMode)
    {
    case AnimationMode::Startup:
        animations::startUp(time);
    break;
    case AnimationMode::PulseError:
        animations::pulseError(time);
    break;
    case AnimationMode::BlinkSuccess:
        animations::blinkSuccess(time);
    break;
    case AnimationMode::ColorfulIdle:
        animations::colorfulIdle(time);
    break;
    case AnimationMode::ProgressBar:
        animations::progressBar(progressBarValue);
    break;
    case AnimationMode::KeyIndicate:
        animations::keyIndicate();
    break;
    case AnimationMode::KeyIndicateFade:
        animations::keyIndicateFade(deltaTime);
    break;
    case AnimationMode::Waiting:
        animations::waiting(deltaTime, animationFirstFrame, fullRefresh);
    break;
    default:
        fatalError(ErrorCode::IMPOSSIBLE_INTERNAL);
        break;
    }
    animationFirstFrame = false;
    fullRefresh = false;
    LEDCom::updateLEDS();
}

void displayErrorCode(byte error)
{
    LEDCom::setErrorCode(error);
    LEDCom::updateLEDS();
}

bool animationCompleted()
{
    return animator::getAnimationComplete();
}

} // namespace lights
