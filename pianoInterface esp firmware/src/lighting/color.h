#ifndef COLOR_H
#define COLOR_H

#include <stdint.h>

struct color;
struct colorF;

// a 32 bit, floating point color
struct colorF
{
    float r, g, b;
};

// an 8 bit color
struct color
{
    uint8_t r, g, b;

    constexpr operator colorF() const
    {
        return {(float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f};
    };
};

// Converts an 8 bit color to a 32 bit color (this is also implicit)
constexpr colorF colorToColorF(color c)
{
    return {(float)c.r / 255.0f, (float)c.g / 255.0f, (float)c.b / 255.0f};
};

// Converts a 32 bit color to an 8 bit color
constexpr color colorFToColor(colorF c)
{
    return {static_cast<uint8_t>(c.r * 255), static_cast<uint8_t>(c.g * 255), static_cast<uint8_t>(c.b * 255)};
};

colorF colorMin(const colorF &a, const colorF &b); 
colorF colorMin(const colorF &a, float b); 
colorF colorMax(const colorF &a, const colorF &b); 
colorF colorMax(const colorF &a, float b); 

bool operator == (color &a, color &b);
bool operator != (color &a, color &b);

bool operator == (const colorF &a, const colorF &b);
bool operator != (const colorF &a, const colorF &b);

colorF operator + (const colorF &a, const colorF &b);
colorF operator - (const colorF &a, const colorF &b);
colorF operator * (const colorF &a, const colorF &b);
colorF operator / (const colorF &a, const colorF &b);
colorF operator += (colorF &a, const colorF &b);
colorF operator -= (colorF &a, const colorF &b);
colorF operator *= (colorF &a, const colorF &b);
colorF operator /= (colorF &a, const colorF &b);

colorF operator + (const colorF &a, float b);
colorF operator - (const colorF &a, float b);
colorF operator * (const colorF &a, float b);
colorF operator / (const colorF &a, float b);
colorF operator += (colorF &a, float b);
colorF operator -= (colorF &a, float b);
colorF operator *= (colorF &a, float b);
colorF operator /= (colorF &a, float b);

#endif