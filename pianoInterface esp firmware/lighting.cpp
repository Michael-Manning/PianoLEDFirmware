#include <Arduino.h>
#include <NeoPixelBus.h>

#include "m_error.h"
#include "lighting.h"
#include "music.h"
#include "pinaoCom.h"
#include "color.h"

namespace
{

////////////////////////////////////////// ANIMATION SETTINGS
constexpr float indicateFadeTime = 0.6f; // seconds
constexpr float inFrameFadeTime = 0.26f; // seconds

//////////////////////////////////////////

color colors[_KEYCOUNT];
colorF colorsF[_KEYCOUNT];

float keyTimers[_KEYCOUNT]; // general use per-key timers for animations
colorF keyFadeTargets[_KEYCOUNT]; // general use colorLayer

bool stripDirty = true;
bool showErrorCode = false; // Display the error code on top of everything else;
byte errorCode = 0;
float animationStartTime = 0;
bool animationComplete = false; // Not all animations have an end
bool animationFirstFrame = false;
float progressBarValue = 0.0f;
bool fullRefresh = false; // general use flag (mostly for waiting animation)
constexpr float HSLRange_Over_KeyCount = 1530.0f / (float)_KEYCOUNT;

constexpr colorF indicateColorF = colorToColorF(lights::indicateColor);
constexpr colorF inFramecolorF = colorToColorF(lights::inFrameColor);

lights::AnimationMode animationMode = lights::AnimationMode::None;

////////////////////////////////////////// song learner stuff
bool pressedThisFrame[_KEYCOUNT]; // keeps track of notes that have been pressed this frame (not just held down from the last frame)

} // namespace

NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(_KEYCOUNT, _PIXELPIN);

constexpr colorF mix(colorF A, colorF B, float t)
{
    return {
        (1.0f - t) * A.r + t * B.r,
        (1.0f - t) * A.g + t * B.g,
        (1.0f - t) * A.b + t * B.b
        };
}

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
        return {255 - (byte)(index - 255), 255, 0};
    }
    else if (index <= 255 * 3)
    {
        return {0, 255, (byte)(index - 255 * 2)};
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

typedef unsigned int size_t;

namespace lights
{

namespace AnimationParameters
{
void setProgressBarValue(float value)
{
    progressBarValue = value;
}
} // namespace AnimationParameters

void allOff()
{
    for (size_t pix = 0; pix < _KEYCOUNT; pix++)
    {
        setColor(pix, colorF{0, 0, 0});
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
       // pixelStates[0] = PixelState::indicate;
    }
}

// void setIndicate(uint8_t key, bool state)
// {
//     assert_fatal(key < _KEYCOUNT, ErrorCode::INVALID_LED_INDEX);
//     if (state)
//     {
//         colors[key] = indicateColor;
//         colorsF[key] = indicateColorF;
//         stripDirty = pixelStates[key] != PixelState::indicate;
//         pixelStates[key] = PixelState::indicate;
//     }
//     else if (pixelStates[key] == PixelState::indicate)
//     {
//         colors[key] = noColor;
//         colorsF[key] = noColor;
//         stripDirty = true;
//     }
// }

// void setInFrame(byte key, bool state)
// {
//     assert_fatal(key < _KEYCOUNT, ErrorCode::INVALID_LED_INDEX);
//     if (state)
//     {
//         colors[key] = inFrameColor;
//         colorsF[key] = inFramecolorF;
//         stripDirty = pixelStates[key] != PixelState::inFrame;
//         pixelStates[key] = PixelState::inFrame;
//     }
//     else
//     {
//         colors[key] = indicateColor;
//         colorsF[key] = indicateColorF;
//         stripDirty = true;
//         pixelStates[key] = PixelState::indicate;
//     }
// }

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
    animationFirstFrame = true;
    animationStartTime = micros() / 1000000.0f;
    animationComplete = false;
    memset(keyTimers, 0, sizeof(keyTimers));
    allOff();
}

void forceRefresh(){
    fullRefresh = true;
}

float lastTime = 0;
void updateAnimation()
{
    if (animationComplete)
    {
        return;
    }

    const float time = (float)micros() / 1000000.0f - animationStartTime;
    const float deltaTime = time - lastTime;
    lastTime = time;

    switch (animationMode)
    {
    case AnimationMode::PulseError:
    {
        float brightness = sin((float)time * 2.0f) * 0.5f + 0.5f;
        setAll({brightness, 0.0f, 0.0f});
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
    }
    break;
    case AnimationMode::ColorfulIdle:
    {
        for (unsigned int i = 0; i < _KEYCOUNT; i++)
        {
            float index = i * HSLRange_Over_KeyCount + time * 1000;
            while (index > 1530)
            {
                index -= 1530;
            }
            setColor(i, sweepHSL((unsigned int)(index)));
        }
    }
    break;
    case AnimationMode::ProgressBar:
    {
        float filledInKeys = _KEYCOUNT * progressBarValue;
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
    break;
    case AnimationMode::KeyIndicate:
    {
        for (size_t i = 0; i < _KEYCOUNT; i++)
        {
            bool state = MIDI::getNoteState(i + MIDI::ledNoteOffset);
            if (state)
            {
                if (music::isBlackNote(i + MIDI::ledNoteOffset))
                {
                    setColor(_KEYCOUNT - 1 - i, Colors::Blue);
                }
                else
                {
                    setColor(_KEYCOUNT - 1 - i, indicateColor);
                }
            }
            else if (!music::isBlackNote(i + MIDI::ledNoteOffset))
            {
                setColor(_KEYCOUNT - 1 - i, ambiantColor);
            }
            else
            {
                setColor(_KEYCOUNT - 1 - i, Colors::Off);
            }
        }
    }
    break;
    case AnimationMode::KeyIndicateFade:
    {
        for (size_t i = 0; i < _KEYCOUNT; i++)
        {
            // Note event: reset the timer
            if (MIDI::getLogicalState(i + MIDI::ledNoteOffset))
            {
                keyTimers[i] = indicateFadeTime;
            }

            if (music::isBlackNote(i + MIDI::ledNoteOffset))
            {
                setColor(_KEYCOUNT - 1 - i, Colors::Blue * ((float)keyTimers[i] / indicateFadeTime));
            }
            else
            {
                colorF col = Colors::Red * ((float)keyTimers[i] / indicateFadeTime);
                col = colorMax(col, ambiantColor);
                setColor(_KEYCOUNT - 1 - i, col);
            }
            keyTimers[i] -= deltaTime;
            if (keyTimers[i] < 0)
            {
                keyTimers[i] = 0;
            }
        }
    }
    break;
    case AnimationMode::Waiting:
    {

        // ADD GLOBAL ANIMATION INIT BOLLEAN

        // update notes pressed down during this frame
        for (size_t i = 0; i < _KEYCOUNT; i++)
        {
            if(MIDI::getLogicalState(i + MIDI::ledNoteOffset)){
                pressedThisFrame[i] = true;
            }
        }


        allOff();

        using namespace music;
        songFrame frame = currentFrame();


        bool allInFrame = true;
    
        byte *notes = frame.firstNote;


        unsigned int index = 0;
        while (true)
        {
            unsigned int keyIndex = (notes[index] - MIDI::ledNoteOffset);
            if (notes[index] >= MIDI::ledNoteOffset && notes[index] < _KEYCOUNT + MIDI::ledNoteOffset)
            {
                keyFadeTargets[keyIndex] = Colors::Red;
                if (pressedThisFrame[notes[index] - MIDI::ledNoteOffset] && MIDI::getNoteState(notes[index]))
                {
                    keyTimers[keyIndex] = inFrameFadeTime;
                   // setColor(_KEYCOUNT - 1 - (notes[index] - MIDI::ledNoteOffset), Colors::Red);
                }
                else
                {
                    allInFrame = false;
                }
            }
            if (notes[index] == *frame.lastNote)
            {
                break;
            }
            index++;
        }

        if (allInFrame)
        {
          //memset(pressedThisFrame, false, sizeof(pressedThisFrame));
          nextFrame();
        }
        if(allInFrame || animationFirstFrame || fullRefresh)
        {
            for (size_t i = 0; i < _KEYCOUNT; i++)
            {
                pressedThisFrame[i] = false;

                if (music::isBlackNote(i + MIDI::ledNoteOffset))
                {
                    keyFadeTargets[i] = Colors::Off;
                }
                else
                {
                    keyFadeTargets[i] = ambiantColor;
                }
            }
        }

        // colors
        for (size_t i = 0; i < _KEYCOUNT; i++)
        {
            // setColor(_KEYCOUNT - 1 - i, keyFadeTargets[i]);
            if (keyTimers[i] == 0)
            {
                setColor(_KEYCOUNT - 1 - i, keyFadeTargets[i]);
            }
            else
            {
                float t =  ((float)keyTimers[i] / inFrameFadeTime);
                colorF col = mix(Colors::Green, keyFadeTargets[i], 1.0f - t);
                setColor(_KEYCOUNT - 1 - i, col);
            }
            keyTimers[i] -= deltaTime;
            if (keyTimers[i] < 0)
            {
                keyTimers[i] = 0;
            }
        }
    }
    break;

    default:
        break;
    }
    animationFirstFrame = false;
    fullRefresh = false;
    updateLEDS();
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

void displayFrame(music::songFrame frame)
{
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        //if(frame.firstnote)
    }
}

} // namespace lights
