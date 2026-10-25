#pragma once

#include <cmath>

// Two-dimensional vector; the simulation domain is the unit square [0, 1]^2.
struct Vec2 {
    double x = 0.0;
    double y = 0.0;

    constexpr Vec2() = default;
    constexpr Vec2(double x_, double y_) : x(x_), y(y_) {}

    double length_squared() const { return x * x + y * y; }
};

inline Vec2 operator+(const Vec2& a, const Vec2& b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(const Vec2& a, const Vec2& b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator*(double s, const Vec2& a) { return {s * a.x, s * a.y}; }
inline Vec2 operator*(const Vec2& a, double s) { return s * a; }
inline Vec2 operator/(const Vec2& a, double s) { return {a.x / s, a.y / s}; }

inline Vec2& operator+=(Vec2& a, const Vec2& b) {
    a = a + b;
    return a;
}

inline double dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }

// z-component of the 3D cross product; twice the signed area of the triangle (0, a, b).
inline double cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }
