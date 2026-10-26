#pragma once

#include <cmath>

// Three-component vector used for points, directions and RGB radiance.
struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    constexpr Vec3() = default;
    constexpr Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    double operator[](int axis) const { return axis == 0 ? x : (axis == 1 ? y : z); }
    double& operator[](int axis) { return axis == 0 ? x : (axis == 1 ? y : z); }

    double length_squared() const { return x * x + y * y + z * z; }
    double length() const { return std::sqrt(length_squared()); }
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(const Vec3& a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(double s, const Vec3& a) { return {s * a.x, s * a.y, s * a.z}; }
inline Vec3 operator*(const Vec3& a, double s) { return s * a; }
inline Vec3 operator/(const Vec3& a, double s) { return {a.x / s, a.y / s, a.z / s}; }

// Component-wise product, used to tint radiance by an albedo.
inline Vec3 operator*(const Vec3& a, const Vec3& b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }

inline Vec3& operator+=(Vec3& a, const Vec3& b) {
    a = a + b;
    return a;
}

inline double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline Vec3 normalized(const Vec3& v) { return v / v.length(); }

// Mirror reflection of an incoming direction about a surface normal.
inline Vec3 reflect(const Vec3& direction, const Vec3& normal) {
    return direction - 2.0 * dot(direction, normal) * normal;
}
