#include <Arduino.h>
#include <NeoPixelBus.h>

#include "../m_constants.h"
#include "color.h"
#include "LEDCom.h"

namespace
{
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(_KEYCOUNT, _PIXELPIN);

color colors[_KEYCOUNT];
colorF colorsF[_KEYCOUNT];
bool stripDirty = true;

bool overlayError = false;
uint8_t errorCode = 0;
} // namespace

namespace LEDCom
{

void stripInit()
{
    strip.Begin();
    strip.Show();
}

void setColor(uint8_t led, uint8_t r, uint8_t g, uint8_t b)
{
    colors[led] = {r, g, b};
    colorsF[led] = colorToColorF({r, g, b});
    stripDirty = true;
}

void setColor(uint8_t led, colorF c)
{
    colors[led] = colorFToColor(c);
    colorsF[led] = c;
    stripDirty = true;
}

colorF getColor(uint8_t led)
{
    return colorsF[led];
}

void setAll(colorF c)
{
    for (size_t i = 0; i < _KEYCOUNT; i++)
    {
        colors[i] = colorFToColor(c);
        colorsF[i] = c;
    }
    stripDirty = true;
}

void setErrorCode(uint8_t code)
{
    overlayError = true;
    errorCode = code;
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
                strip.SetPixelColor(i, RgbColor(0, 0, 255));
            }
        }
    }
    strip.Show();
    stripDirty = false;
}

}