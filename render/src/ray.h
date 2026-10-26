#pragma once

#include "vec3.h"

struct Ray {
    Vec3 origin;
    Vec3 direction;  // unit length

    Vec3 at(double t) const { return origin + t * direction; }
};

// Offset used to start secondary rays slightly off a surface so they do not
// re-intersect the surface they leave ("shadow acne").
constexpr double kRayEpsilon = 1e-4;
