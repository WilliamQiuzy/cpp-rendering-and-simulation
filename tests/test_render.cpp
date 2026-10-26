#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "check.h"
#include "rng.h"
#include "scene.h"
#include "sphere.h"
#include "triangle_mesh.h"

namespace {

const Material kGrey = Material::diffuse(Vec3(0.5, 0.5, 0.5));

std::string write_temp_obj(const std::string& contents) {
    const std::string path = std::string(PHOTONFLOW_TEST_TMP) + "/test_mesh.obj";
    std::ofstream(path) << contents;
    return path;
}

}  // namespace

TEST(sphere_hit_from_outside) {
    const Sphere sphere(Vec3(0, 0, 0), 1.0, kGrey);
    const auto hit = sphere.intersect(Ray{Vec3(0, 0, 5), Vec3(0, 0, -1)});
    CHECK(hit.has_value());
    CHECK_NEAR(hit->t, 4.0, 1e-12);
    CHECK_NEAR(hit->point.z, 1.0, 1e-12);
    CHECK_NEAR(hit->normal.z, 1.0, 1e-12);
    CHECK(hit->shape == &sphere);
}

TEST(sphere_miss_and_behind) {
    const Sphere sphere(Vec3(0, 0, 0), 1.0, kGrey);
    CHECK(!sphere.intersect(Ray{Vec3(0, 2, 5), Vec3(0, 0, -1)}).has_value());  // passes above
    CHECK(!sphere.intersect(Ray{Vec3(0, 0, 5), Vec3(0, 0, 1)}).has_value());   // sphere behind the ray
}

TEST(sphere_hit_from_inside_returns_exit_point) {
    const Sphere sphere(Vec3(0, 0, 0), 2.0, kGrey);
    const auto hit = sphere.intersect(Ray{Vec3(0, 0, 0), Vec3(1, 0, 0)});
    CHECK(hit.has_value());
    CHECK_NEAR(hit->t, 2.0, 1e-12);
}

TEST(obj_polygon_is_triangulated_as_fan) {
    TriangleMesh mesh(kGrey);
    mesh.load_obj(write_temp_obj("v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2//1 3//1 4//1\n"));
    mesh.build_bvh();
    CHECK(mesh.triangle_count() == 2);
    const auto hit = mesh.intersect(Ray{Vec3(0.25, 0.75, 1), Vec3(0, 0, -1)});  // inside the second triangle
    CHECK(hit.has_value());
    CHECK_NEAR(hit->t, 1.0, 1e-12);
    CHECK_NEAR(std::abs(hit->normal.z), 1.0, 1e-12);
    CHECK(!mesh.intersect(Ray{Vec3(2, 2, 1), Vec3(0, 0, -1)}).has_value());
}

TEST(obj_negative_indices_are_relative) {
    TriangleMesh mesh(kGrey);
    mesh.load_obj(write_temp_obj("v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n"));
    mesh.build_bvh();
    CHECK(mesh.triangle_count() == 1);
    CHECK(mesh.intersect(Ray{Vec3(0.2, 0.2, 1), Vec3(0, 0, -1)}).has_value());
}

// The BVH must return exactly the closest hit found by testing every triangle.
TEST(bvh_matches_brute_force) {
    std::mt19937 engine(7);
    std::uniform_real_distribution<double> unit(-1.0, 1.0);
    auto random_point = [&]() { return Vec3(unit(engine), unit(engine), unit(engine)); };

    TriangleMesh mesh(kGrey);
    std::vector<std::unique_ptr<TriangleMesh>> singles;
    std::vector<Vec3> centroids;
    for (int i = 0; i < 500; ++i) {
        const Vec3 center = random_point();
        const Vec3 a = center + 0.1 * random_point(), b = center + 0.1 * random_point(), c = center + 0.1 * random_point();
        centroids.push_back((a + b + c) / 3.0);
        mesh.add_triangle(a, b, c);
        singles.push_back(std::make_unique<TriangleMesh>(kGrey));
        singles.back()->add_triangle(a, b, c);
        singles.back()->build_bvh();
    }
    mesh.build_bvh();

    // Half of the rays aim at a random triangle's centroid (hits), half go in random directions (mostly misses).
    std::uniform_int_distribution<int> pick(0, static_cast<int>(centroids.size()) - 1);
    int hits = 0;
    for (int r = 0; r < 300; ++r) {
        const Vec3 origin = 3.0 * random_point();
        const Vec3 direction = (r % 2 == 0) ? normalized(centroids[pick(engine)] - origin) : normalized(random_point());
        const Ray ray{origin, direction};
        double best = std::numeric_limits<double>::max();
        for (const auto& single : singles) {
            if (auto hit = single->intersect(ray)) best = std::min(best, hit->t);
        }
        const auto hit = mesh.intersect(ray);
        CHECK(hit.has_value() == (best < std::numeric_limits<double>::max()));
        if (hit) {
            ++hits;
            CHECK_NEAR(hit->t, best, 1e-12);
        }
    }
    CHECK(hits > 100);  // the comparison is only meaningful if many rays hit
}

// For cosine-weighted sampling, E[cos(theta)] = 2/3.
TEST(cosine_hemisphere_sampling_statistics) {
    Rng rng(123);
    const Vec3 normal = normalized(Vec3(0.3, 1.0, -0.2));
    const int n = 20000;
    double sum_cos = 0.0;
    bool all_unit = true, all_above = true;
    for (int i = 0; i < n; ++i) {
        const Vec3 d = sample_cosine_hemisphere(normal, rng);
        all_unit = all_unit && std::abs(d.length() - 1.0) < 1e-9;
        all_above = all_above && dot(d, normal) >= 0.0;
        sum_cos += dot(d, normal);
    }
    CHECK(all_unit);
    CHECK(all_above);
    CHECK_NEAR(sum_cos / n, 2.0 / 3.0, 0.01);
}

TEST(scene_returns_closest_hit) {
    Scene scene(PointLight{Vec3(0, 10, 0), 1.0});
    scene.add(std::make_unique<Sphere>(Vec3(0, 0, -10), 1.0, kGrey));
    scene.add(std::make_unique<Sphere>(Vec3(0, 0, -5), 1.0, kGrey));
    const auto hit = scene.intersect(Ray{Vec3(0, 0, 0), Vec3(0, 0, -1)});
    CHECK(hit.has_value());
    CHECK_NEAR(hit->t, 4.0, 1e-12);
    Rng rng(1);
    CHECK(scene.radiance(Ray{Vec3(0, 0, 0), Vec3(0, 0, 1)}, 3, rng).length() == 0.0);  // escapes to the background
}

TEST(mirror_reflection) {
    const Vec3 r = reflect(normalized(Vec3(1, -1, 0)), Vec3(0, 1, 0));
    CHECK_NEAR(r.x, std::sqrt(0.5), 1e-12);
    CHECK_NEAR(r.y, std::sqrt(0.5), 1e-12);
    CHECK_NEAR(r.z, 0.0, 1e-12);
}

int main() { return check::run_all(); }
