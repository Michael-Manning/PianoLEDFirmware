#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <stdint.h>

#include "../m_constants.h"
#include "color.h"

namespace animator
{

// Interpolates between two colors with a t value from 0.0 - 1.0.
// Similar to GLSL mix()
constexpr colorF mix(const colorF &A, const colorF &B, float t)
{
    return {
        (1.0f - t) * A.r + t * B.r,
        (1.0f - t) * A.g + t * B.g,
        (1.0f - t) * A.b + t * B.b};
};

constexpr float HSLRange_Over_KeyCount = 1530.0f / (float)_KEYCOUNT;

extern float keyTimers[_KEYCOUNT]; // general use per-key timers for animations
extern colorF keyFadeTargets[_KEYCOUNT]; // general use colorLayer
extern bool pressedThisFrame[_KEYCOUNT]; // keeps track of notes that have been pressed this frame (not just held down from the last frame)

void setColor(uint8_t led, colorF col);

void addColor(uint8_t led, colorF col);

void setAll(colorF c);

color sweepHSL(unsigned int index);

void resetAnimation();

void setAnimationComplete();
bool getAnimationComplete();

}

namespace animations
{
    void startUp(const float time);
    void pulseError(const float time);
    void blinkSuccess(const float time);
    void colorfulIdle(const float time);
    void progressBar(float progress);
    void keyIndicate();
    void keyIndicateFade(const float deltaTime);
    void waiting(const float deltaTime, bool firstFrame, bool fullRefresh);
    void wave(const float deltaTime, bool firstFrame);
    void rainbowFade(const float deltaTime, const float time);
}

#endif