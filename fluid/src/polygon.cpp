#include "polygon.h"

#include <cmath>

namespace {

// Shoelace formula; positive for counter-clockwise vertex order.
double signed_area(const std::vector<Vec2>& v) {
    double twice_area = 0.0;
    for (std::size_t i = 0; i < v.size(); ++i) {
        twice_area += cross(v[i], v[(i + 1) % v.size()]);
    }
    return 0.5 * twice_area;
}

}  // namespace

double Polygon::area() const {
    if (vertices.size() < 2) return 0.0;
    return std::abs(signed_area(vertices));
}

// Fan triangulation from vertex 0. For a triangle T with corners c_k,
// integral over T of |x - p|^2 = |T| / 6 * sum_{k <= l} (c_k - p) . (c_l - p).
double Polygon::integrated_squared_distance(const Vec2& point) const {
    if (empty()) return 0.0;

    double total = 0.0;
    for (std::size_t i = 1; i + 1 < vertices.size(); ++i) {
        const Vec2 corners[3] = {vertices[0], vertices[i], vertices[i + 1]};
        double sum = 0.0;
        for (int k = 0; k < 3; ++k) {
            for (int l = k; l < 3; ++l) {
                sum += dot(corners[k] - point, corners[l] - point);
            }
        }
        const double triangle_area = 0.5 * std::abs(cross(corners[1] - corners[0], corners[2] - corners[0]));
        total += sum / 6.0 * triangle_area;
    }
    return total;
}

Vec2 Polygon::centroid() const {
    if (vertices.empty()) return Vec2();
    const double area = signed_area(vertices);
    if (area == 0.0) return vertices[0];  // degenerate cell

    Vec2 weighted_sum;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const Vec2& a = vertices[i];
        const Vec2& b = vertices[(i + 1) % vertices.size()];
        weighted_sum += (a + b) * cross(a, b);
    }
    return weighted_sum / (6.0 * area);
}
