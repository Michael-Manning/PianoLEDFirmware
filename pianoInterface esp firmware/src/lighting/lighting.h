#ifndef LIGHTING_H
#define LIGHTING_H

#include <stdint.h>

#include "../m_constants.h"
#include "color.h"


namespace lights
{

enum class AnimationMode{
    Startup,
    PulseError,
    BlinkSuccess,
    ColorfulIdle,
    ProgressBar,
    KeyIndicate,
    KeyIndicateFade,
    Waiting, // (Learning Mode)
    None,
    Ambiant,
    Wave
};

namespace AnimationParameters
{
    void setProgressBarValue(float value);
}

void init();
void setAnimationMode(AnimationMode mode);
void forceRefresh();
void updateAnimation();
void displayErrorCode(uint8_t error);
bool animationCompleted();


// Regular LEDs
constexpr int redLEDPin = 15;
constexpr int greenLEDPin = 2;
constexpr int blueLEDPin = 4;

void setRedLED(bool state);
void setGreenLED(bool state);
void setBlueLED(bool state);
void BlinkBlueLED();
void updateBlueLED(float deltaTime);

};

#endif