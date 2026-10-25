#pragma once

#include <optional>

#include "ray.h"
#include "vec3.h"

enum class MaterialType { Diffuse, Mirror, Refractive };

struct Material {
    Vec3 albedo{1.0, 1.0, 1.0};
    MaterialType type = MaterialType::Diffuse;
    double refractive_index = 1.0;  // only used by Refractive materials

    static Material diffuse(const Vec3& albedo) { return {albedo, MaterialType::Diffuse, 1.0}; }
    static Material mirror() { return {Vec3(1.0, 1.0, 1.0), MaterialType::Mirror, 1.0}; }
    static Material glass(double ior) { return {Vec3(1.0, 1.0, 1.0), MaterialType::Refractive, ior}; }
};

class Shape;

// Closest intersection of a ray with the scene.
struct Hit {
    double t = 0.0;      // distance along the ray
    Vec3 point;
    Vec3 normal;         // unit geometric normal, pointing out of the shape
    const Shape* shape = nullptr;
};

class Shape {
public:
    explicit Shape(const Material& material) : material_(material) {}
    virtual ~Shape() = default;

    virtual std::optional<Hit> intersect(const Ray& ray) const = 0;

    const Material& material() const { return material_; }

private:
    Material material_;
};
