#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "aabb.h"
#include "shape.h"

// Triangle mesh loaded from a Wavefront OBJ file, accelerated with a bounding
// volume hierarchy (BVH). Shading uses the flat geometric normal of each face.
class TriangleMesh : public Shape {
public:
    explicit TriangleMesh(const Material& material) : Shape(material) {}

    // Reads vertex positions and faces; polygons are split into triangle fans.
    // Throws std::runtime_error if the file cannot be read.
    void load_obj(const std::string& path);

    // Transforms are applied in call order: translate(p) then scale(s) maps v to s * (v + p).
    void translate(const Vec3& offset);
    void scale(double factor);

    // Must be called after loading and transforming, before rendering.
    void build_bvh();

    std::optional<Hit> intersect(const Ray& ray) const override;

    std::size_t triangle_count() const { return triangles_.size(); }

private:
    using Triangle = std::array<int, 3>;  // indices into vertices_

    struct BvhNode {
        Aabb bounds;
        int begin = 0;  // range of triangles_ covered by this node
        int end = 0;
        std::unique_ptr<BvhNode> left;
        std::unique_ptr<BvhNode> right;
        bool is_leaf() const { return !left && !right; }
    };

    static constexpr int kMaxLeafTriangles = 8;

    Aabb bounds_of(int begin, int end) const;
    Vec3 centroid(const Triangle& triangle) const;
    void build_node(BvhNode& node, int begin, int end);
    bool intersect_triangle(const Ray& ray, const Triangle& triangle, double max_t, Hit& hit) const;

    std::vector<Vec3> vertices_;
    std::vector<Triangle> triangles_;
    std::unique_ptr<BvhNode> root_;
};
