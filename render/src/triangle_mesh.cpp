#include "triangle_mesh.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

// Converts a 1-based (or negative, relative) OBJ index to a 0-based index.
int resolve_index(int obj_index, std::size_t count) {
    return obj_index < 0 ? static_cast<int>(count) + obj_index : obj_index - 1;
}

// Extracts the position index from a face token such as "7", "7/2", "7//3" or "7/2/3".
int position_index(const std::string& token) {
    return std::stoi(token.substr(0, token.find('/')));
}

}  // namespace

void TriangleMesh::load_obj(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("cannot open OBJ file: " + path);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream in(line);
        std::string keyword;
        in >> keyword;

        if (keyword == "v") {
            Vec3 p;
            in >> p.x >> p.y >> p.z;
            vertices_.push_back(p);
        } else if (keyword == "f") {
            std::vector<int> corners;
            std::string token;
            while (in >> token) {
                corners.push_back(resolve_index(position_index(token), vertices_.size()));
            }
            for (std::size_t i = 1; i + 1 < corners.size(); ++i) {
                triangles_.push_back({corners[0], corners[i], corners[i + 1]});
            }
        }
        // Normals, texture coordinates and material groups are not used by the renderer.
    }
}

void TriangleMesh::add_triangle(const Vec3& a, const Vec3& b, const Vec3& c) {
    const int base = static_cast<int>(vertices_.size());
    vertices_.push_back(a);
    vertices_.push_back(b);
    vertices_.push_back(c);
    triangles_.push_back({base, base + 1, base + 2});
}

void TriangleMesh::translate(const Vec3& offset) {
    for (Vec3& v : vertices_) v = v + offset;
}

void TriangleMesh::scale(double factor) {
    for (Vec3& v : vertices_) v = v * factor;
}

Aabb TriangleMesh::bounds_of(int begin, int end) const {
    Aabb box;
    for (int i = begin; i < end; ++i) {
        for (int corner : triangles_[i]) box.expand(vertices_[corner]);
    }
    return box;
}

Vec3 TriangleMesh::centroid(const Triangle& triangle) const {
    return (vertices_[triangle[0]] + vertices_[triangle[1]] + vertices_[triangle[2]]) / 3.0;
}

void TriangleMesh::build_bvh() {
    root_ = std::make_unique<BvhNode>();
    build_node(*root_, 0, static_cast<int>(triangles_.size()));
}

// Median split along the longest axis of the node's bounding box.
void TriangleMesh::build_node(BvhNode& node, int begin, int end) {
    node.bounds = bounds_of(begin, end);
    node.begin = begin;
    node.end = end;
    if (end - begin <= kMaxLeafTriangles) return;

    const int axis = node.bounds.longest_axis();
    const int middle = begin + (end - begin) / 2;
    std::nth_element(triangles_.begin() + begin, triangles_.begin() + middle, triangles_.begin() + end,
                     [&](const Triangle& a, const Triangle& b) { return centroid(a)[axis] < centroid(b)[axis]; });

    node.left = std::make_unique<BvhNode>();
    node.right = std::make_unique<BvhNode>();
    build_node(*node.left, begin, middle);
    build_node(*node.right, middle, end);
}

// Moller-Trumbore ray/triangle test. Updates `hit` only for intersections closer than max_t.
bool TriangleMesh::intersect_triangle(const Ray& ray, const Triangle& triangle, double max_t, Hit& hit) const {
    constexpr double kEpsilon = 1e-8;
    const Vec3& v0 = vertices_[triangle[0]];
    const Vec3 edge1 = vertices_[triangle[1]] - v0;
    const Vec3 edge2 = vertices_[triangle[2]] - v0;

    const Vec3 p = cross(ray.direction, edge2);
    const double det = dot(edge1, p);
    if (std::abs(det) < kEpsilon) return false;  // ray parallel to the triangle
    const double inv_det = 1.0 / det;

    const Vec3 s = ray.origin - v0;
    const double u = dot(s, p) * inv_det;
    if (u < 0.0 || u > 1.0) return false;

    const Vec3 q = cross(s, edge1);
    const double v = dot(ray.direction, q) * inv_det;
    if (v < 0.0 || u + v > 1.0) return false;

    const double t = dot(edge2, q) * inv_det;
    if (t <= kEpsilon || t >= max_t) return false;

    hit.t = t;
    hit.point = ray.at(t);
    const Vec3 n = cross(edge1, edge2);
    const double length = n.length();
    hit.normal = (length > 1e-12) ? n / length : n;
    hit.shape = this;
    return true;
}

std::optional<Hit> TriangleMesh::intersect(const Ray& ray) const {
    if (!root_ || !root_->bounds.intersects(ray)) return std::nullopt;

    Hit closest;
    closest.t = std::numeric_limits<double>::max();
    bool found = false;

    std::vector<const BvhNode*> stack{root_.get()};
    while (!stack.empty()) {
        const BvhNode* node = stack.back();
        stack.pop_back();

        if (!node->is_leaf()) {
            if (node->left && node->left->bounds.intersects(ray)) stack.push_back(node->left.get());
            if (node->right && node->right->bounds.intersects(ray)) stack.push_back(node->right.get());
            continue;
        }
        for (int i = node->begin; i < node->end; ++i) {
            found |= intersect_triangle(ray, triangles_[i], closest.t, closest);
        }
    }

    if (!found) return std::nullopt;
    return closest;
}
