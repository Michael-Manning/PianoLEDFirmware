#include <Arduino.h>
#include <NeoPixelBus.h>

#include "m_error.h"
#include "lighting.h"

namespace
{
color colors[_KEYCOUNT];
colorF colorsF[_KEYCOUNT];

enum class PixelState
{
    indicate,
    inFrame,
    fading
};

PixelState pixelStates[_KEYCOUNT];

bool stripDirty = true;
bool showErrorCode = false; // Display the error code on top of everything else;
byte errorCode = 0;
float animationStartTime = 0;
bool animationComplete = false; // Not all animations have an end
float progressBarValue = 0.0f;
constexpr float HSLRange_Over_KeyCount = 1530.0f / (float)_KEYCOUNT;

constexpr colorF colorToColorF(color c)
{
    return {(float)c.r / 255.0f, (float)c.g / 255.0f, (float)c.b / 255.0f};
}
constexpr color colorFToColor(colorF c)
{
    return {static_cast<byte>(c.r * 255), static_cast<byte>(c.g * 255), static_cast<byte>(c.b * 255)};
}

constexpr colorF indicateColorF = colorToColorF(lights::indicateColor);
constexpr colorF inFramecolorF = colorToColorF(lights::inFrameColor);

lights::AnimationMode animationMode = lights::AnimationMode::None;
} // namespace

NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(_KEYCOUNT, _PIXELPIN);

inline void setColor(byte led, uint8_t r, uint8_t g, uint8_t b)
{
    //strip.SetPixelColor(led, RgbColor(r, g, b));
    colors[led] = {r, g, b};
    colorsF[led] = colorToColorF({r, g, b});
    stripDirty = true;
}
inline void setColor(byte led, color c)
{
    //strip.SetPixelColor(led, RgbColor(r, g, b));
    colors[led] = c;
    colorsF[led] = colorToColorF(c);
    stripDirty = true;
}
inline void setColor(byte led, colorF c)
{
    //strip.SetPixelColor(led, RgbColor(r, g, b));
    colors[led] = colorFToColor(c);
    colorsF[led] = c;
    stripDirty = true;
}
inline void setAll(colorF c)
{
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        colors[i] = colorFToColor(c);
        colorsF[i] = c;
    }
    stripDirty = true;
}

// Index between 0 - 1530
color sweepHSL(unsigned int index){

    if(!assert_fatal(index <= 1530, ErrorCode::INVALID_LED_INDEX)){
         return {255, 255, 255};
    }
    if(index <= 255 * 1){
        return {255, index, 0};
    }
    else if(index <= 255 * 2){
        return {255 - (byte)(index - 255), 255, 0};
    }
    else if(index <= 255 * 3){
        return {0, 255, (byte)(index - 255 * 2)};
    }
    else if(index <= 255 * 4){
        return {0, 255 - (index - 255 * 3), 255};
    }
    else if(index <= 255 * 5){
        return {index - 255 * 4, 0, 255};
    }
    else if(index <= 255 * 6){
        return {255, 0, 255 - (index - 255 * 5)};
    }
    fatalError(ErrorCode::IMPOSSIBLE_INTERNAL);
    return {255, 255, 255};
}

typedef unsigned int size_t;

namespace lights
{

namespace AnimationParameters
{
    void setProgressBarValue(float value){
        progressBarValue = value;
    }
}

void allOff()
{
    for (size_t pix = 0; pix < _KEYCOUNT; pix++)
    {
        setColor(pix, 0, 0, 0);
    }
    // strip.Show();
}

void allIdle()
{
    setAll(colorToColorF(ambiantColor));
}

void init()
{
    strip.Begin();
    strip.Show();

    // Show initialization animation
    for (size_t pix = 0; pix < _KEYCOUNT; pix++)
    {
        for (size_t i = 0; i < 255; i += 30)
        {
            setColor(pix, i, 0, 0);
            updateLEDS();
        }
        pixelStates[0] = PixelState::indicate;
    }
}

void setIndicate(uint8_t key, bool state)
{
    assert_fatal(key < _KEYCOUNT, ErrorCode::INVALID_LED_INDEX);
    if (state)
    {
        colors[key] = indicateColor;
        colorsF[key] = indicateColorF;
        stripDirty = pixelStates[key] != PixelState::indicate;
        pixelStates[key] = PixelState::indicate;
    }
    else if (pixelStates[key] == PixelState::indicate)
    {
        colors[key] = noColor;
        colorsF[key] = noColor;
        stripDirty = true;
    }
}

void setInFrame(byte key, bool state)
{
    assert_fatal(key < _KEYCOUNT, ErrorCode::INVALID_LED_INDEX);
    if (state)
    {
        colors[key] = inFrameColor;
        colorsF[key] = inFramecolorF;
        stripDirty = pixelStates[key] != PixelState::inFrame;
        pixelStates[key] = PixelState::inFrame;
    }
    else
    {
        colors[key] = indicateColor;
        colorsF[key] = indicateColorF;
        stripDirty = true;
        pixelStates[key] = PixelState::indicate;
    }
}

void updateLEDS()
{
    if (!stripDirty)
        return;

    for (size_t pix = 0; pix < _KEYCOUNT; pix++)
    {
        strip.SetPixelColor(pix, RgbColor(colors[pix].r, colors[pix].g, colors[pix].b));
    }
    // Display the errorCode on top of everything else;
    if (errorCode)
    {
        for (size_t i = 0; i < 8; i++)
        {
            if (errorCode >> i & 0x01)
            {
                strip.SetPixelColor(i, RgbColor(errorCodeColor.r, errorCodeColor.g, errorCodeColor.b));
            }
        }
    }
    strip.Show();
    stripDirty = false;
}

void setAnimationMode(AnimationMode mode)
{
    animationMode = mode;
    animationStartTime = millis() / 1000.0f;
    animationComplete = false;
}

void updateAnimation()
{
    if (animationComplete)
    {
        return;
    }

    float time = millis() / 1000.0f - animationStartTime;

    switch (animationMode)
    {
    case AnimationMode::PulseError:
    {
        float brightness = sin((float)time * 2.0f) * 0.5f + 0.5f;
        setAll({brightness, 0.0f, 0.0f});
        updateLEDS();
    }
        break;
    case AnimationMode::BlinkSuccess:
    {
        float brightness = sin((float)time * 20.0f) * 0.5f + 0.5f;
        setAll({0.0f, brightness, 0.0f});
        if (time >= 1.0f)
        {
            animationComplete = true;
            animationMode = AnimationMode::None;
            allIdle();
        }
        updateLEDS();
    }
        break;
    case AnimationMode::ColorfulIdle:
    {
        for (unsigned int i = 0; i < _KEYCOUNT; i++)
        {
            float index = i * HSLRange_Over_KeyCount + time * 1000;
            while(index > 1530){
                index -= 1530;
            }
            setColor(i, sweepHSL((unsigned int)(index)));
        }
        updateLEDS();
    }
        break;
    case AnimationMode::ProgressBar:
    {
        int filledInKeys = _KEYCOUNT * progressBarValue;
        for (size_t i = 0; i < _KEYCOUNT; i++)
        {
            if(i <= filledInKeys){
                setColor(i, Colors::Red);
            }
            else{
                setColor(i, Colors::Off);
            }
        }
        updateLEDS();
    }
        break;
    default:
        break;
    }
}

void displayErrorCode(byte error)
{
    showErrorCode = true;
    errorCode = error;
    updateLEDS();
}
void hideErrorCode()
{
    showErrorCode = false;
}

bool animationCompleted()
{
    return animationComplete;
}

} // namespace lights
