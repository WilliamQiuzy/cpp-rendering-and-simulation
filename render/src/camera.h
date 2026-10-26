#pragma once

#include <cmath>

#include "ray.h"

// Pinhole camera looking down -z.
class Camera {
public:
    Camera(const Vec3& position, double horizontal_fov_degrees, int width, int height)
        : position_(position),
          width_(width),
          height_(height),
          image_plane_z_(-width / (2.0 * std::tan(horizontal_fov_degrees * M_PI / 360.0))) {}

    // Ray through continuous pixel coordinates (x right, y up, origin at the
    // bottom-left corner of the image).
    Ray ray_through(double x, double y) const {
        const Vec3 direction(x + 0.5 - width_ / 2.0, y + 0.5 - height_ / 2.0, image_plane_z_);
        return {position_, normalized(direction)};
    }

    int width() const { return width_; }
    int height() const { return height_; }

private:
    Vec3 position_;
    int width_;
    int height_;
    double image_plane_z_;
};
