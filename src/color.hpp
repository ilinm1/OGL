#pragma once

#include <algorithm>

struct Color
{
    union
    {
        unsigned char Rgba[4];
        unsigned int Uint = 0;
    };

    Color() {}

    Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255)
    {
        Rgba[0] = r;
        Rgba[1] = g;
        Rgba[2] = b;
        Rgba[3] = a;
    }

    Color& operator+=(const Color& r)
    {
        Rgba[0] = std::min(static_cast<int>(Rgba[0]) + r.Rgba[0], 255); //avoiding overflows
        Rgba[1] = std::min(static_cast<int>(Rgba[1]) + r.Rgba[1], 255);
        Rgba[2] = std::min(static_cast<int>(Rgba[2]) + r.Rgba[2], 255);
        Rgba[3] = std::min(static_cast<int>(Rgba[3]) + r.Rgba[3], 255);
        return *this;
    }

    friend Color operator+(Color l, const Color& r)
    {
        l += r;
        return l;
    }

    Color& operator-=(const Color& r)
    {
        Rgba[0] -= r.Rgba[0];
        Rgba[1] -= r.Rgba[1];
        Rgba[2] -= r.Rgba[2];
        Rgba[3] -= r.Rgba[3];
        return *this;
    }

    friend Color operator-(Color l, const Color& r)
    {
        l -= r;
        return l;
    }

    Color& operator*=(const float& r)
    {
        Rgba[0] *= r;
        Rgba[1] *= r;
        Rgba[2] *= r;
        Rgba[3] *= r;
        return *this;
    }

    friend Color operator*(Color l, const float& r)
    {
        l *= r;
        return l;
    }

    Color& operator/=(const float& r)
    {
        Rgba[0] /= r;
        Rgba[1] /= r;
        Rgba[2] /= r;
        Rgba[3] /= r;
        return *this;
    }

    friend Color operator/(Color l, const float& r)
    {
        l /= r;
        return l;
    }

    friend bool operator==(const Color& l, const Color& r)
    {
        return l.Uint == r.Uint;
    }

    friend bool operator!=(const Color& l, const Color& r)
    {
        return !(l == r);
    }
};

#define COLOR_TRANSPARENT Color {}
#define COLOR_WHITE Color(255, 255, 255)
#define COLOR_BLACK Color(0, 0, 0)
