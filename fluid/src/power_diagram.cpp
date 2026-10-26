#include "power_diagram.h"

#include <cmath>

namespace {

constexpr int kDiskVertexCount = 200;  // polygonal approximation of the air boundary

// Sutherland-Hodgman clipping of `polygon` against the half-plane where `inside(x)`
// holds. `intersection(a, b)` returns the crossing point on segment [a, b].
template <typename InsideFn, typename IntersectFn>
Polygon clip(const Polygon& polygon, InsideFn inside, IntersectFn intersection) {
    Polygon result;
    const std::size_t n = polygon.vertices.size();
    for (std::size_t k = 0; k < n; ++k) {
        const Vec2& a = polygon.vertices[(k + n - 1) % n];
        const Vec2& b = polygon.vertices[k];
        const bool a_inside = inside(a);
        const bool b_inside = inside(b);
        if (b_inside) {
            if (!a_inside) result.vertices.push_back(intersection(a, b));
            result.vertices.push_back(b);
        } else if (a_inside) {
            result.vertices.push_back(intersection(a, b));
        }
    }
    return result;
}

// Keeps the side of the power bisector of (p_i, w_i) and (p_j, w_j) that belongs to i.
Polygon clip_by_power_bisector(const Polygon& cell, const Vec2& pi, double wi, const Vec2& pj, double wj) {
    const Vec2 axis = pj - pi;
    // Point of the bisector on segment [p_i, p_j]: the midpoint shifted by the weight difference.
    const Vec2 m = (pi + pj) / 2.0 + (wi - wj) / (2.0 * axis.length_squared()) * axis;
    auto inside = [&](const Vec2& x) {
        return (x - pi).length_squared() - wi <= (x - pj).length_squared() - wj;
    };
    auto intersection = [&](const Vec2& a, const Vec2& b) {
        const double t = dot(m - a, axis) / dot(b - a, axis);
        return a + t * (b - a);
    };
    return clip(cell, inside, intersection);
}

// Intersects a convex cell with a convex, clockwise-ordered polygon (the disk).
Polygon clip_by_convex_polygon(const Polygon& cell, const Polygon& clipper) {
    Polygon result = cell;
    const std::size_t n = clipper.vertices.size();
    for (std::size_t i = 0; i < n && !result.vertices.empty(); ++i) {
        const Vec2& u = clipper.vertices[i];
        const Vec2& v = clipper.vertices[(i + 1) % n];
        const Vec2 normal(v.y - u.y, u.x - v.x);  // edge rotated clockwise: points into a clockwise polygon
        auto inside = [&](const Vec2& x) { return dot(x - u, normal) >= 0.0; };
        auto intersection = [&](const Vec2& a, const Vec2& b) {
            const double t = dot(u - a, normal) / dot(b - a, normal);
            return a + t * (b - a);
        };
        result = clip(result, inside, intersection);
    }
    return result;
}

Polygon clockwise_disk(const Vec2& center, double radius) {
    Polygon disk;
    disk.vertices.reserve(kDiskVertexCount);
    for (int k = 0; k < kDiskVertexCount; ++k) {
        const double angle = 2.0 * M_PI * k / kDiskVertexCount;
        disk.vertices.emplace_back(center.x + radius * std::cos(angle), center.y - radius * std::sin(angle));
    }
    return disk;
}

Polygon compute_cell(int i, const std::vector<Vec2>& sites, const std::vector<double>& weights, double air_weight) {
    Polygon cell;
    cell.vertices = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};  // unit square, clockwise

    for (int j = 0; j < static_cast<int>(sites.size()) && !cell.vertices.empty(); ++j) {
        if (j != i) cell = clip_by_power_bisector(cell, sites[i], weights[i], sites[j], weights[j]);
    }

    const double radius_squared = weights[i] - air_weight;
    if (radius_squared <= 0.0) return Polygon();  // the air phase wins everywhere
    return clip_by_convex_polygon(cell, clockwise_disk(sites[i], std::sqrt(radius_squared)));
}

}  // namespace

std::vector<Polygon> compute_power_cells(const std::vector<Vec2>& sites, const std::vector<double>& weights,
                                         double air_weight) {
    std::vector<Polygon> cells(sites.size());
    // Cells are independent, so they are computed in parallel.
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < static_cast<int>(sites.size()); ++i) {
        cells[i] = compute_cell(i, sites, weights, air_weight);
    }
    return cells;
}
