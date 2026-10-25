#include "scene.h"

#include <algorithm>
#include <cmath>
#include <utility>

std::optional<Hit> Scene::intersect(const Ray& ray) const {
    std::optional<Hit> closest;
    for (const auto& shape : shapes_) {
        std::optional<Hit> hit = shape->intersect(ray);
        if (hit && (!closest || hit->t < closest->t)) closest = hit;
    }
    return closest;
}

Vec3 Scene::radiance(const Ray& ray, int bounces, Rng& rng) const {
    if (bounces < 0) return Vec3();

    const std::optional<Hit> hit = intersect(ray);
    if (!hit) return Vec3();

    switch (hit->shape->material().type) {
        case MaterialType::Mirror:
            return shade_mirror(ray, *hit, bounces, rng);
        case MaterialType::Refractive:
            return shade_refractive(ray, *hit, bounces, rng);
        case MaterialType::Diffuse:
        default:
            return shade_diffuse(*hit, bounces, rng);
    }
}

// Lambertian surface lit by the point light, with a visibility (shadow) test.
Vec3 Scene::direct_light(const Hit& hit) const {
    Vec3 to_light = light_.position - hit.point;
    const double distance = to_light.length();
    to_light = to_light / distance;

    const Ray shadow_ray{hit.point + kRayEpsilon * hit.normal, to_light};
    const std::optional<Hit> blocker = intersect(shadow_ray);
    if (blocker && blocker->t < distance) return Vec3();

    const Vec3& albedo = hit.shape->material().albedo;
    const double irradiance = light_.intensity / (4.0 * M_PI * distance * distance);
    return irradiance * (albedo / M_PI) * std::max(0.0, dot(hit.normal, to_light));
}

// Direct light plus one cosine-weighted indirect sample (the cosine and the
// sampling density cancel, leaving only the albedo).
Vec3 Scene::shade_diffuse(const Hit& hit, int bounces, Rng& rng) const {
    const Ray bounce{hit.point + kRayEpsilon * hit.normal, sample_cosine_hemisphere(hit.normal, rng)};
    const Vec3 indirect = radiance(bounce, bounces - 1, rng) * hit.shape->material().albedo;
    return direct_light(hit) + indirect;
}

Vec3 Scene::shade_mirror(const Ray& ray, const Hit& hit, int bounces, Rng& rng) const {
    const Ray reflected{hit.point + kRayEpsilon * hit.normal, reflect(ray.direction, hit.normal)};
    return radiance(reflected, bounces - 1, rng);
}

// Dielectric interface: choose reflection or refraction with Schlick's Fresnel
// probability, and fall back to reflection under total internal reflection.
Vec3 Scene::shade_refractive(const Ray& ray, const Hit& hit, int bounces, Rng& rng) const {
    Vec3 normal = hit.normal;
    double n_from = 1.0;
    double n_to = hit.shape->material().refractive_index;
    if (dot(ray.direction, normal) > 0.0) {  // leaving the object
        std::swap(n_from, n_to);
        normal = -normal;
    }

    const double ratio = n_from / n_to;
    const double cos_incident = -dot(ray.direction, normal);
    const double r0 = std::pow((n_from - n_to) / (n_from + n_to), 2);
    const double reflectance = r0 + (1.0 - r0) * std::pow(1.0 - std::abs(cos_incident), 5);
    const Ray reflected{hit.point + kRayEpsilon * normal, reflect(ray.direction, normal)};

    if (rng.uniform() < reflectance) return radiance(reflected, bounces - 1, rng);

    const double cos2_transmitted = 1.0 - ratio * ratio * (1.0 - cos_incident * cos_incident);
    if (cos2_transmitted < 0.0) return radiance(reflected, bounces - 1, rng);  // total internal reflection

    const Vec3 transmitted =
        -std::sqrt(cos2_transmitted) * normal + ratio * (ray.direction + cos_incident * normal);
    const Ray refracted{hit.point - kRayEpsilon * normal, transmitted};
    return radiance(refracted, bounces - 1, rng);
}
