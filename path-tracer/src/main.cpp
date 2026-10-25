// Monte Carlo path tracer: renders a cat mesh inside a room of colored walls.
//
//   path_tracer [--samples N] [--depth N] [--size N] [--model PATH] [--output PATH]
//               [--mirror-sphere] [--glass-sphere]

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "stb_image_write.h"

#include "camera.h"
#include "rng.h"
#include "scene.h"
#include "sphere.h"
#include "triangle_mesh.h"

// Set by CMake to the absolute path of assets/cat.obj so the binary runs from any directory.
#ifndef PATH_TRACER_DEFAULT_MODEL
#define PATH_TRACER_DEFAULT_MODEL "assets/cat.obj"
#endif

namespace {

struct Options {
    int samples_per_pixel = 64;
    int max_bounces = 5;
    int image_size = 512;
    std::string model_path = PATH_TRACER_DEFAULT_MODEL;
    std::string output_path = "render.png";
    bool mirror_sphere = false;
    bool glass_sphere = false;
};

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument("missing value after " + arg);
            return argv[++i];
        };
        if (arg == "--samples") options.samples_per_pixel = std::stoi(next());
        else if (arg == "--depth") options.max_bounces = std::stoi(next());
        else if (arg == "--size") options.image_size = std::stoi(next());
        else if (arg == "--model") options.model_path = next();
        else if (arg == "--output") options.output_path = next();
        else if (arg == "--mirror-sphere") options.mirror_sphere = true;
        else if (arg == "--glass-sphere") options.glass_sphere = true;
        else throw std::invalid_argument("unknown option: " + arg);
    }
    if (options.samples_per_pixel < 1 || options.max_bounces < 0 || options.image_size < 1) {
        throw std::invalid_argument("samples and size must be positive, depth non-negative");
    }
    return options;
}

// Six huge spheres approximate the walls of a room around the origin.
void add_room(Scene& scene) {
    const struct { Vec3 center; double radius; Vec3 albedo; } walls[] = {
        {{0, 1000, 0}, 940, {0.5, 0.0, 0.0}},    // ceiling, red
        {{0, 0, 1000}, 940, {0.5, 0.0, 0.5}},    // wall behind the camera, pink
        {{0, -1000, 0}, 990, {0.0, 0.0, 0.5}},   // floor, blue
        {{0, 0, -1000}, 940, {0.0, 0.5, 0.0}},   // back wall, green
        {{1000, 0, 0}, 940, {0.5, 0.5, 0.0}},    // right wall, yellow
        {{-1000, 0, 0}, 940, {0.0, 0.5, 0.5}},   // left wall, cyan
    };
    for (const auto& wall : walls) {
        scene.add(std::make_unique<Sphere>(wall.center, wall.radius, Material::diffuse(wall.albedo)));
    }
}

Scene build_scene(const Options& options) {
    Scene scene(PointLight{Vec3(-10, 20, 40), 1e7});
    add_room(scene);

    auto cat = std::make_unique<TriangleMesh>(Material::diffuse(Vec3(1.0, 1.0, 1.0)));
    cat->load_obj(options.model_path);
    cat->translate(Vec3(0, -13, -20));
    cat->scale(0.8);
    cat->build_bvh();
    std::cout << "Loaded " << cat->triangle_count() << " triangles from " << options.model_path << "\n";
    scene.add(std::move(cat));

    if (options.mirror_sphere) scene.add(std::make_unique<Sphere>(Vec3(25, 0, 0), 10, Material::mirror()));
    if (options.glass_sphere) scene.add(std::make_unique<Sphere>(Vec3(-25, 0, 0), 10, Material::glass(1.5)));
    return scene;
}

// Gamma-encodes linear radiance (in [0, 255]) to an 8-bit value.
unsigned char to_srgb_byte(double radiance) {
    const double linear = std::clamp(radiance / 255.0, 0.0, 1.0);
    return static_cast<unsigned char>(std::lround(255.0 * std::pow(linear, 1.0 / 2.2)));
}

std::vector<unsigned char> render(const Scene& scene, const Camera& camera, const Options& options) {
    const int width = camera.width();
    const int height = camera.height();
    constexpr double kPixelJitterStddev = 0.4;  // Gaussian anti-aliasing filter, in pixels
    std::vector<unsigned char> image(static_cast<std::size_t>(width) * height * 3);

#pragma omp parallel for schedule(dynamic, 1)
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const std::uint64_t pixel_index = static_cast<std::uint64_t>(row) * width + col;
            Rng rng(0x9E3779B97F4A7C15ULL * (pixel_index + 1));

            Vec3 sum;
            for (int s = 0; s < options.samples_per_pixel; ++s) {
                double dx, dy;
                sample_gaussian_pair(kPixelJitterStddev, rng, dx, dy);
                const Ray ray = camera.ray_through(col + dx, height - row - 1 + dy);
                sum += scene.radiance(ray, options.max_bounces, rng);
            }
            const Vec3 color = sum / options.samples_per_pixel;

            unsigned char* out = &image[pixel_index * 3];
            out[0] = to_srgb_byte(color.x);
            out[1] = to_srgb_byte(color.y);
            out[2] = to_srgb_byte(color.z);
        }
    }
    return image;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        const Scene scene = build_scene(options);
        const Camera camera(Vec3(0, 7, 55), 60.0, options.image_size, options.image_size);

        const auto start = std::chrono::steady_clock::now();
        const std::vector<unsigned char> image = render(scene, camera, options);
        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

        if (!stbi_write_png(options.output_path.c_str(), camera.width(), camera.height(), 3, image.data(), 0)) {
            throw std::runtime_error("failed to write " + options.output_path);
        }
        std::cout << "Rendered " << options.output_path << " in " << elapsed.count() << " s ("
                  << options.samples_per_pixel << " samples/pixel, " << options.max_bounces << " bounces)\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
}
