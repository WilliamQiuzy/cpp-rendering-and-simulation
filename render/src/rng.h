#pragma once

#include <cmath>
#include <cstdint>
#include <random>

#include "vec3.h"

// Random number source owned by a single pixel, so parallel rendering is free of
// data races and every run produces the same image.
class Rng {
public:
    explicit Rng(std::uint64_t seed) : engine_(seed) {}

    double uniform() { return distribution_(engine_); }

private:
    std::mt19937_64 engine_;
    std::uniform_real_distribution<double> distribution_{0.0, 1.0};
};

// Cosine-weighted direction on the hemisphere around `normal`. Sampling with this
// density cancels the cosine term of the rendering equation for diffuse surfaces.
inline Vec3 sample_cosine_hemisphere(const Vec3& normal, Rng& rng) {
    const double r1 = rng.uniform();
    const double r2 = rng.uniform();
    const double x = std::cos(2.0 * M_PI * r1) * std::sqrt(1.0 - r2);
    const double y = std::sin(2.0 * M_PI * r1) * std::sqrt(1.0 - r2);
    const double z = std::sqrt(r2);

    // Tangent built from the two largest normal components to stay well conditioned.
    Vec3 tangent;
    const double ax = std::abs(normal.x), ay = std::abs(normal.y), az = std::abs(normal.z);
    if (ax < ay && ax <= az) {
        tangent = Vec3(0.0, normal.z, -normal.y);
    } else if (ay <= ax && ay <= az) {
        tangent = Vec3(normal.z, 0.0, -normal.x);
    } else {
        tangent = Vec3(normal.y, -normal.x, 0.0);
    }
    tangent = normalized(tangent);
    const Vec3 bitangent = cross(normal, tangent);

    return x * tangent + y * bitangent + z * normal;
}

// Two independent Gaussian samples (Box-Muller), used to jitter camera rays for
// anti-aliasing.
inline void sample_gaussian_pair(double stddev, Rng& rng, double& a, double& b) {
    const double r1 = rng.uniform();
    const double r2 = rng.uniform();
    const double radius = std::sqrt(-2.0 * std::log(r1)) * stddev;
    a = radius * std::cos(2.0 * M_PI * r2);
    b = radius * std::sin(2.0 * M_PI * r2);
}
