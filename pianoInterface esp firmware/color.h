#ifndef COLOR_H
#define COLOR_H

#include <Arduino.h>

struct color;
struct colorF;

struct colorF
{
    float r, g, b;
};

struct color
{
    byte r, g, b;

    constexpr operator colorF() const
    {
        return {(float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f};
    };
};







constexpr colorF colorToColorF(color c)
{
    return {(float)c.r / 255.0f, (float)c.g / 255.0f, (float)c.b / 255.0f};
};

constexpr color colorFToColor(colorF c)
{
    return {static_cast<byte>(c.r * 255), static_cast<byte>(c.g * 255), static_cast<byte>(c.b * 255)};
};


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

colorF colorMin(const colorF &a, const colorF &b); 
colorF colorMin(const colorF &a, float b); 
colorF colorMax(const colorF &a, const colorF &b); 
colorF colorMax(const colorF &a, float b); 

#endif