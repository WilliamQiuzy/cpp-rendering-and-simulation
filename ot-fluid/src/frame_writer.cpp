#include "frame_writer.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "stb_image_write.h"

namespace {

constexpr double kOutlineWidthPixels = 2.0;

void set_pixel(std::vector<unsigned char>& image, int size, int x, int y, unsigned char r, unsigned char g,
               unsigned char b) {
    const std::size_t offset = (static_cast<std::size_t>(size - y - 1) * size + x) * 3;  // flip: y up
    image[offset] = r;
    image[offset + 1] = g;
    image[offset + 2] = b;
}

// Convex point-in-polygon test (consistent edge orientation) that also returns the
// distance to the nearest edge, used to draw outlines.
bool inside_convex(const std::vector<Vec2>& vertices, const Vec2& p, double& edge_distance) {
    int previous_sign = 0;
    edge_distance = std::numeric_limits<double>::max();
    for (std::size_t j = 0; j < vertices.size(); ++j) {
        const Vec2& a = vertices[j];
        const Vec2& b = vertices[(j + 1) % vertices.size()];
        const Vec2 edge = b - a;
        const double side = cross(p - a, edge);
        const int sign = (side > 0) - (side < 0);
        if (sign != 0) {
            if (previous_sign != 0 && sign != previous_sign) return false;
            previous_sign = sign;
        }
        const double edge_length = std::sqrt(edge.length_squared());
        const double along = dot(p - a, edge);
        if (along >= 0.0 && along <= edge_length * edge_length) {
            edge_distance = std::min(edge_distance, std::abs(side) / edge_length);
        }
    }
    return true;
}

}  // namespace

bool write_frame_png(const std::vector<Polygon>& cells, const std::string& path, int size) {
    std::vector<unsigned char> image(static_cast<std::size_t>(size) * size * 3, 255);

    for (const Polygon& cell : cells) {
        if (cell.empty()) continue;

        // Work in pixel units, restricted to the cell's bounding box.
        std::vector<Vec2> pixels;
        pixels.reserve(cell.vertices.size());
        double min_x = size, min_y = size, max_x = 0.0, max_y = 0.0;
        for (const Vec2& v : cell.vertices) {
            pixels.push_back(size * v);
            min_x = std::min(min_x, size * v.x);
            min_y = std::min(min_y, size * v.y);
            max_x = std::max(max_x, size * v.x);
            max_y = std::max(max_y, size * v.y);
        }
        const int x0 = std::clamp(static_cast<int>(min_x), 0, size - 1);
        const int y0 = std::clamp(static_cast<int>(min_y), 0, size - 1);
        const int x1 = std::clamp(static_cast<int>(std::ceil(max_x)), 0, size - 1);
        const int y1 = std::clamp(static_cast<int>(std::ceil(max_y)), 0, size - 1);

        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                double edge_distance;
                if (!inside_convex(pixels, Vec2(x, y), edge_distance)) continue;
                if (edge_distance <= kOutlineWidthPixels) {
                    set_pixel(image, size, x, y, 0, 0, 0);
                } else {
                    set_pixel(image, size, x, y, 0, 0, 255);
                }
            }
        }
    }
    return stbi_write_png(path.c_str(), size, size, 3, image.data(), 0) != 0;
}
