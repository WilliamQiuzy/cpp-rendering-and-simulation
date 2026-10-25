#pragma once

#include <algorithm>
#include <limits>

#include "ray.h"

// Axis-aligned bounding box.
struct Aabb {
    Vec3 min{std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
             std::numeric_limits<double>::max()};
    Vec3 max{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(),
             std::numeric_limits<double>::lowest()};

    void expand(const Vec3& p) {
        for (int axis = 0; axis < 3; ++axis) {
            min[axis] = std::min(min[axis], p[axis]);
            max[axis] = std::max(max[axis], p[axis]);
        }
    }

    int longest_axis() const {
        const Vec3 extent = max - min;
        int axis = 0;
        if (extent.y > extent[axis]) axis = 1;
        if (extent.z > extent[axis]) axis = 2;
        return axis;
    }

    // Slab test: does the ray enter the box at a non-negative distance?
    bool intersects(const Ray& ray) const {
        double t_enter = std::numeric_limits<double>::lowest();
        double t_exit = std::numeric_limits<double>::max();
        for (int axis = 0; axis < 3; ++axis) {
            const double inv = 1.0 / ray.direction[axis];
            const double t0 = (min[axis] - ray.origin[axis]) * inv;
            const double t1 = (max[axis] - ray.origin[axis]) * inv;
            t_enter = std::max(t_enter, std::min(t0, t1));
            t_exit = std::min(t_exit, std::max(t0, t1));
        }
        return t_enter <= t_exit && t_exit >= 0.0;
    }
};
