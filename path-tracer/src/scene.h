#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "rng.h"
#include "shape.h"

struct PointLight {
    Vec3 position;
    double intensity = 0.0;  // radiant intensity, spread uniformly over the sphere
};

class Scene {
public:
    explicit Scene(const PointLight& light) : light_(light) {}

    void add(std::unique_ptr<Shape> shape) { shapes_.push_back(std::move(shape)); }

    std::optional<Hit> intersect(const Ray& ray) const;

    // Monte Carlo estimate of the radiance arriving along `ray`, following at most
    // `bounces` further reflections or refractions.
    Vec3 radiance(const Ray& ray, int bounces, Rng& rng) const;

private:
    Vec3 shade_diffuse(const Hit& hit, int bounces, Rng& rng) const;
    Vec3 shade_mirror(const Ray& ray, const Hit& hit, int bounces, Rng& rng) const;
    Vec3 shade_refractive(const Ray& ray, const Hit& hit, int bounces, Rng& rng) const;
    Vec3 direct_light(const Hit& hit) const;

    PointLight light_;
    std::vector<std::unique_ptr<Shape>> shapes_;
};
