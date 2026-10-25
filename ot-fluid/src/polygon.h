#pragma once

#include <vector>

#include "vec2.h"

// Convex polygon with vertices in order (either orientation).
struct Polygon {
    std::vector<Vec2> vertices;

    bool empty() const { return vertices.size() < 3; }

    double area() const;

    // Integral of |x - point|^2 over the polygon, the transport cost of moving the
    // polygon's mass onto `point`.
    double integrated_squared_distance(const Vec2& point) const;

    Vec2 centroid() const;
};
