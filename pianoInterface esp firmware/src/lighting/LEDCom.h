#ifndef LEDCOM_H
#define LEDCOM_H

#include <stdint.h>

#include "color.h"

namespace LEDCom
{

void stripInit();

void setColor(uint8_t led, uint8_t r, uint8_t g, uint8_t b);

void setColor(uint8_t led, colorF c);

colorF getColor(uint8_t led);

void setAll(colorF c);

void setErrorCode(uint8_t code);

void updateLEDS();

}

#endif