#include <Arduino.h>
#include "color.h"

#define _MIN(a, b) a < b ? a : b
#define _MAX(a, b) a > b ? a : b

bool operator==(color &a, color &b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}
bool operator!=(color &a, color &b)
{
    return !(a == b);
}

#define _colorFCiel(c)     \
    c.r = _MIN(c.r, 1.0f); \
    c.g = _MIN(c.g, 1.0f); \
    c.b = _MIN(c.b, 1.0f);
#define _colorFFloor(c)    \
    c.r = _MAX(c.r, 0.0f); \
    c.g = _MAX(c.g, 0.0f); \
    c.b = _MAX(c.b, 0.0f);

bool operator==(const colorF &a, const colorF &b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}
bool operator!=(const colorF &a, const colorF &b)
{
    return !(a == b);
}

// colorF with ColorF
colorF operator+(const colorF &a, const colorF &b)
{
    colorF c = {a.r + b.r, a.g + b.g, a.b + b.b};
    _colorFCiel(c);
    return c;
}
colorF operator-(const colorF &a, const colorF &b)
{
    colorF c = {a.r - b.r, a.g - b.g, a.b - b.b};
    _colorFFloor(c);
    return c;
}
colorF operator*(const colorF &a, const colorF &b)
{
    colorF c = {a.r * b.r, a.g * b.g, a.b * b.b};
    _colorFCiel(c);
    return c;
}
colorF operator/(const colorF &a, const colorF &b)
{
    return {a.r / b.r, a.g / b.g, a.b / b.b};
}
colorF operator+=(colorF &a, const colorF &b)
{
    a = a + b;
    return a;
}
colorF operator-=(colorF &a, const colorF &b)
{
    a = a - b;
    return a;
}
colorF operator*=(colorF &a, const colorF &b)
{
    a = a * b;
    return a;
}
colorF operator/=(colorF &a, const colorF &b)
{
    a = a / b;
    return a;
}

// colorF and float
colorF operator+(const colorF &a, float b)
{
    colorF c = {a.r + b, a.g + b, a.b + b};
    _colorFCiel(c);
    return c;
}
colorF operator-(const colorF &a, float b)
{
    colorF c = {a.r - b, a.g - b, a.b - b};
    _colorFFloor(c);
    return c;
}
colorF operator*(const colorF &a, float b)
{
    colorF c = {a.r * b, a.g * b, a.b * b};
    _colorFCiel(c);
    return c;
}
colorF operator/(const colorF &a, float b)
{
    return {a.r / b, a.g / b, a.b / b};
}
colorF operator+=(colorF &a, float b)
{
    a = a + b;
    return a;
}
colorF operator-=(colorF &a, float b)
{
    a = a - b;
    return a;
}
colorF operator*=(colorF &a, float b)
{
    a = a * b;
    return a;
}
colorF operator/=(colorF &a, float b)
{
    a = a / b;
    return a;
}

colorF colorMin(const colorF &a, const colorF &b)
{
    colorF c;
    c.r = _MIN(a.r, b.r);
    c.g = _MIN(a.g, b.g);
    c.b = _MIN(a.b, b.b);
    return c;
}
colorF colorMin(const colorF &a, float b)
{
    colorF c;
    c.r = _MIN(a.r, b);
    c.g = _MIN(a.g, b);
    c.b = _MIN(a.b, b);
    return c;
}
colorF colorMax(const colorF &a, const colorF &b)
{
    colorF c;
    c.r = _MAX(a.r, b.r);
    c.g = _MAX(a.g, b.g);
    c.b = _MAX(a.b, b.b);
    return c;
}
colorF colorMax(const colorF &a, float b)
{
    colorF c;
    c.r = _MAX(a.r, b);
    c.g = _MAX(a.g, b);
    c.b = _MAX(a.b, b);
    return c;
}