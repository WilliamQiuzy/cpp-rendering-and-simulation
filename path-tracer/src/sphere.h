#pragma once

#include <cmath>

#include "shape.h"

class Sphere : public Shape {
public:
    Sphere(const Vec3& center, double radius, const Material& material)
        : Shape(material), center_(center), radius_(radius) {}

    std::optional<Hit> intersect(const Ray& ray) const override {
        // Solve |O + tD - C|^2 = r^2 with |D| = 1, i.e. t^2 + 2bt + c = 0.
        const Vec3 oc = ray.origin - center_;
        const double b = dot(ray.direction, oc);
        const double c = oc.length_squared() - radius_ * radius_;
        const double discriminant = b * b - c;
        if (discriminant < 0.0) return std::nullopt;

        const double root = std::sqrt(discriminant);
        const double t_near = -b - root;
        const double t_far = -b + root;
        if (t_far < 0.0) return std::nullopt;  // sphere is behind the ray

        Hit hit;
        hit.t = (t_near >= 0.0) ? t_near : t_far;  // t_far when the ray starts inside
        hit.point = ray.at(hit.t);
        const Vec3 outward = hit.point - center_;
        const double length = outward.length();
        hit.normal = (length > 1e-12) ? outward / length : Vec3();
        hit.shape = this;
        return hit;
    }

private:
    Vec3 center_;
    double radius_;
};
