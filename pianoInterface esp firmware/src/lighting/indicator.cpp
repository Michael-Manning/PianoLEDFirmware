// This is for the simple external LEDs being used for debugging purposes. Most likely temporary

#include <Arduino.h>
#include "lighting.h"

namespace
{
    float blinkLength = 0.150f;
    float blinkTime = 0.0f;
} // namespace

namespace lights
{
    void setRedLED(bool state)
    {
        digitalWrite(redLEDPin, state);
    }
    void setGreenLED(bool state)
    {
        digitalWrite(greenLEDPin, state);
    }
    void setBlueLED(bool state)
    {
        digitalWrite(blueLEDPin, state);
    }

    void BlinkBlueLED()
    {
        digitalWrite(blueLEDPin, HIGH);
        blinkTime = 0.0f;
    }

    void updateBlueLED(float deltaTime)
    {
        blinkTime += deltaTime;
        if (blinkTime >= blinkLength)
        {
            digitalWrite(blueLEDPin, LOW);
        }
    }
} // namespace lights