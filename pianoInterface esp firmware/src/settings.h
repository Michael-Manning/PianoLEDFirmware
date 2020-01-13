#ifndef SETTINGS_H
#define SETTINGS_H

#include "lighting/color.h"

namespace settings
{

enum class Colors
{
    Ambiant = 0,
    IndicateWhite = 1,
    IndicateBlack = 2,
    WaitingWhite = 3,
    WaitingBlack = 4,
    InFrameWhite = 5,
    InFrameBlack = 6
};
constexpr unsigned int colorSettingCount = 7;

enum class Floats
{
    IndicateFadeTime = 0,
    VelocityThreshold = 1
};
constexpr unsigned int floatSettingCount = 1;

void init();

void restoreDefaults();

void loadSettings();

void commitSettings();

void saveColorSetting(settings::Colors setting, color value);
void saveColorSetting(unsigned int setting, color value);

void saveFloatSetting(settings::Floats setting, float value);

color getColorSetting(settings::Colors setting);
color getColorSetting(unsigned int setting);

bool getFloatSetting(settings::Floats setting);

void dumpToSerial();

} // namespace settings

#endif