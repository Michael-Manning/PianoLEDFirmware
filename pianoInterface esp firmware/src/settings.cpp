#include <Arduino.h>
#include <EEPROM.h>

#include "lighting/color.h"
#include "settings.h"

namespace
{
    color colorSettingValues[settings::colorSettingCount];
    float floatSettingValues[settings::floatSettingCount];
}

namespace settings
{

void init()
{
    EEPROM.begin(sizeof(color) * colorSettingCount + sizeof(float) * floatSettingCount + 10);
}

// Set all settings to their default values
void restoreDefaults()
{
    saveColorSetting(settings::Colors::Ambiant, {1, 1, 1});
    saveColorSetting(settings::Colors::IndicateWhite, {255, 0, 0});
    saveColorSetting(settings::Colors::IndicateBlack, {0, 0, 255});
    saveColorSetting(settings::Colors::WaitingWhite, {255, 0, 0});
    saveColorSetting(settings::Colors::WaitingBlack, {255, 0, 0});
    saveColorSetting(settings::Colors::InFrameWhite, {0, 255, 0});
    saveColorSetting(settings::Colors::InFrameBlack, {0, 255, 0});
    saveFloatSetting(settings::Floats::IndicateFadeTime, 0.6f);
}

void loadSettings()
{
    for (size_t i = 0; i < colorSettingCount; i++)
    {
        EEPROM.get(i * sizeof(color), colorSettingValues[i]);
    }
    for (size_t i = 0; i < floatSettingCount; i++)
    {
        EEPROM.get(i * sizeof(color) + colorSettingCount * sizeof(color), floatSettingValues[i]);
    }    
}

void saveColorSetting(settings::Colors setting, color value)
{
    saveColorSetting(static_cast<unsigned int>(setting), value);
}

void saveColorSetting(unsigned int setting, color value)
{
    EEPROM.put(setting * sizeof(color), value);
    colorSettingValues[setting] = value;
    EEPROM.commit();
}

void saveFloatSetting(settings::Floats setting, float value)
{
    EEPROM.put(static_cast<unsigned int>(setting) * sizeof(float) + colorSettingCount * sizeof(color), value);
    floatSettingValues[static_cast<unsigned int>(setting)] = value;
    EEPROM.commit();
}

color getColorSetting(settings::Colors setting)
{
    return colorSettingValues[static_cast<unsigned int>(setting)];
}
color getColorSetting(unsigned int setting)
{
return colorSettingValues[setting];
}

bool getFloatSetting(settings::Floats setting)
{
    return floatSettingValues[static_cast<unsigned int>(setting)];
}

void dumpToSerial()
{
    char buff[128];
    Serial.println("Printing settings:");
    for (size_t i = 0; i < colorSettingCount; i++)
    {
        sprintf(buff, "color %d = R:%d G:%d B: %d", i, colorSettingValues[i].r, colorSettingValues[i].g, colorSettingValues[i].b);
        Serial.println(buff);
    }
    for (size_t i = 0; i < floatSettingCount; i++)
    {
        sprintf(buff, "float %d = %f", i, floatSettingValues[i]);
        Serial.println(buff);
    }
}

} // namespace settings