#pragma once

#include <tuple>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <deque>
#include <cmath>
#include <cassert>
#include <functional>

#include "common.hh"

struct Point
{
    double x = 0;
    double y = 0;

    Point() = default;

    Point(double xi, double yi)
        : x(xi), y(yi)
    {
    }

    inline void operator+=(const Point &rhs)
    {
        x += rhs.x;
        y += rhs.y;
    }

    inline void operator-=(const Point &rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
    }

    inline void operator*=(double val)
    {
        x *= val;
        y *= val;
    }

    void rotate(double d, Point center)
    {
        const double PI = 3.1415;
        double r = d * PI * 2 / 360;
        Point tmp = *this;
        tmp -= center;
        double xi = tmp.x * cos(r) - tmp.y * sin(r);
        double yi = tmp.x * sin(r) + tmp.y * cos(r);
        tmp.x = xi;
        tmp.y = yi;
        tmp += center;
        *this = tmp;
    }

    void rotate_and_scale(double d, double scale, Point center)
    {
        const double PI = 3.1415;
        double r = d * PI * 2 / 360;
        Point tmp = *this;
        tmp -= center;
        double xi = tmp.x * cos(r) - tmp.y * sin(r);
        double yi = tmp.x * sin(r) + tmp.y * cos(r);
        tmp.x = xi * scale;
        tmp.y = yi * scale;
        tmp += center;
        *this = tmp;
    }

    void rotate(double d)
    {
        const double PI = 3.1415;
        double r = d * PI * 2 / 360;
        double xi = x * cos(r) - y * sin(r);
        double yi = x * sin(r) + y * cos(r);
        x = xi;
        y = yi;
    }

    double cross(const Point &rhs) const
    {
        return x * rhs.y - y * rhs.x;
    }

    double dot(const Point &rhs) const
    {
        return x * rhs.x + y * rhs.y;
    }

    double distance(const Point &rhs) const
    {
        return sqrtf(powf(rhs.x - x, 2) + powf(rhs.y - y, 2));
    }

    int distance_squared(const Point &rhs)
    {
        int a = rhs.x - x;
        int b = rhs.y - y;
        return a * a + b * b;
    }

    int manhattan_distance(const Point &rhs)
    {
        return abs(rhs.x - x) + abs(rhs.y - y);
    }

    double magnitude()
    {
        return sqrt(x * x + y * y);
    }

    void normalize()
    {
        Point a(x, y);
        double m = magnitude();
        assert(!std::isnan(m));

        if (m > 0.0)
        {
            a *= 1.0 / m;
            assert(!std::isnan(a.x) && !std::isnan(a.y));
        }

        x = a.x;
        y = a.y;
    }
};

inline Point operator+(const Point &lhs, const Point &rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

inline Point operator-(const Point &lhs, const Point &rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

inline Point operator*(const Point &lhs, double val)
{
    return {lhs.x * val, lhs.y * val};
}

inline bool operator==(const Point &lhs, const Point &rhs)
{
    return (int)lhs.x == (int)rhs.x && (int)lhs.y == (int)rhs.y;
}

inline bool operator!=(const Point &lhs, const Point &rhs)
{
    return !(lhs == rhs);
}

inline bool operator<(const Point &lhs, const Point &rhs)
{
    int64_t l = (int64_t)lhs.x + ((int64_t)lhs.y << 32);
    int64_t r = (int64_t)rhs.x + ((int64_t)rhs.y << 32);
    return l < r;
}

using Line = std::pair<Point, Point>;
