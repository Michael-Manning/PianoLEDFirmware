#ifndef LIGHTING_H
#define LIGHTING_H

//#include "constants.h"
#include "m_constants.h"

#ifndef _KEYCOUNT
#define _KEYCOUNT 53
#endif

#ifndef _PIXELPIN
#define _PIXELPIN 13
#endif

#define noColor {0, 0, 0}

/**
 * Add more feedback animations to indicate things such as
 * a song being loaded in or a setting being changed successfully
 */

struct color
{
    byte r, g, b;
};

struct colorF
{
    float r, g, b;
};

//color operator+(color &a, color &b) { return {a.r + b.r, a.g + b.g, a.b + b.b};};


namespace lights
{
constexpr float fadeSpeed = 0.1f;
constexpr color ambiantColor = {1, 1, 1};
constexpr color indicateColor = {255, 0, 0};
constexpr color inFrameColor = {0, 0, 150};
constexpr color errorCodeColor = {0, 0, 255};

extern colorF indicateColorF;
extern colorF inFramecolorF;

enum class AnimationMode{
    PulseError,
    PulseColor,
    BlinkSuccess,
    ColorfulIdle,
    ProgressBar,
    None
};

namespace AnimationParameters{
    extern colorF PulseColor; 
    void setProgressBarValue(float value);
}

namespace Colors{
    constexpr color Off = {0, 0, 0};
    constexpr color Red = {255, 0, 0};
    constexpr color Blue = {0, 0, 255};
    constexpr color Green = {0, 255, 0};
    constexpr color Purple = {0, 255, 255};
}

void displayErrorCode(byte error);
void hideErrorCode();
void setAnimationMode(AnimationMode mode);
void updateAnimation();
bool animationCompleted();
void setIndicate(uint8_t key, bool state);
void setInFrame(byte key, bool state); // Sets to indicate if false!
void setFadOut(uint8_t key, colorF col);
void allOff();
void init();
void updateLEDS();
};

#endif