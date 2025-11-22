#pragma once

#include <cmath>

struct Vec2
{
    float X = 0;
    float Y = 0;

    Vec2() {};

    Vec2(float xy)
    {
        X = Y = xy;
    }

    Vec2(float x, float y)
    {
        X = x; Y = y;
    }

    //returns vector with maximal component values of both input vectors
    static Vec2 Max(Vec2 a, Vec2 b)
    {
        return Vec2(std::max(a.X, b.X), std::max(a.Y, b.Y));
    }

    //returns vector with minimal component values of both input vectors
    static Vec2 Min(Vec2 a, Vec2 b)
    {
        return Vec2(std::min(a.X, b.X), std::min(a.Y, b.Y));
    }

    //in radians
    static Vec2 FromAngle(float angle)
    {
        return Vec2(cosf(angle), sinf(angle));
    }

    float Length()
    {
        return sqrtf(X * X + Y * Y);
    }

    float Dotp(const Vec2& v)
    {
        return X * v.X + Y * v.Y;
    }

    Vec2 Normalized()
    {
        float l = Length();
        return Vec2(X / l, Y / l);
    }

    Vec2 Abs()
    {
        return Vec2 { abs(X), abs(Y) };
    }

    Vec2& operator+=(const Vec2& r)
    {
        X += r.X;
        Y += r.Y;
        return *this;
    }

    friend Vec2 operator+(Vec2 l, const Vec2& r)
    {
        l += r;
        return l;
    }

    Vec2& operator-=(const Vec2& r)
    {
        X -= r.X;
        Y -= r.Y;
        return *this;
    }

    friend Vec2 operator-(Vec2 l, const Vec2& r)
    {
        l -= r;
        return l;
    }

    Vec2& operator*=(const float& r)
    {
        X *= r;
        Y *= r;
        return *this;
    }

    friend Vec2 operator*(Vec2 l, const float& r)
    {
        l *= r;
        return l;
    }

    Vec2& operator/=(const float& r)
    {
        X /= r;
        Y /= r;
        return *this;
    }

    friend Vec2 operator/(Vec2 l, const float& r)
    {
        l /= r;
        return l;
    }

    friend bool operator==(const Vec2& l, const Vec2& r)
    {
        return l.X == r.X && l.Y == r.Y;
    }

    friend bool operator!=(const Vec2& l, const Vec2& r)
    {
        return !(l == r);
    }
};
