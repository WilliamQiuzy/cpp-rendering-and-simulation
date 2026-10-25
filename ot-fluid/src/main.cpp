// Free-surface fluid simulated with semi-discrete optimal transport.
//
//   ot_fluid [--particles N] [--frames N] [--seed N] [--output-dir DIR]
//
// Writes one PNG per frame (DIR/frame_0000.png, ...).

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "fluid.h"
#include "frame_writer.h"

namespace {

struct Options {
    int particle_count = 50;
    int frame_count = 400;
    unsigned seed = 44;
    std::string output_dir = "frames";
};

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument("missing value after " + arg);
            return argv[++i];
        };
        if (arg == "--particles") options.particle_count = std::stoi(next());
        else if (arg == "--frames") options.frame_count = std::stoi(next());
        else if (arg == "--seed") options.seed = static_cast<unsigned>(std::stoul(next()));
        else if (arg == "--output-dir") options.output_dir = next();
        else throw std::invalid_argument("unknown option: " + arg);
    }
    if (options.particle_count < 1 || options.frame_count < 1) {
        throw std::invalid_argument("particles and frames must be positive");
    }
    return options;
}

std::vector<Vec2> random_positions(int count, unsigned seed) {
    std::default_random_engine engine(seed);
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    std::vector<Vec2> positions(count);
    for (Vec2& p : positions) {
        p.x = uniform(engine);
        p.y = uniform(engine);
    }
    return positions;
}

std::string frame_path(const std::string& dir, int frame) {
    char name[32];
    std::snprintf(name, sizeof(name), "frame_%04d.png", frame);
    return (std::filesystem::path(dir) / name).string();
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        std::filesystem::create_directories(options.output_dir);

        FluidSimulation simulation(random_positions(options.particle_count, options.seed), FluidParameters{});

        const auto start = std::chrono::steady_clock::now();
        int unconverged_frames = 0;
        for (int frame = 0; frame < options.frame_count; ++frame) {
            const std::vector<Polygon> cells = simulation.step();
            if (simulation.last_solver_status() != 0) ++unconverged_frames;
            const std::string path = frame_path(options.output_dir, frame);
            if (!write_frame_png(cells, path)) throw std::runtime_error("failed to write " + path);
            std::cout << "\rframe " << frame + 1 << "/" << options.frame_count << std::flush;
        }
        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
        std::cout << "\nSimulated " << options.frame_count << " frames in " << elapsed.count() << " s ("
                  << unconverged_frames << " frames where L-BFGS did not converge)\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
}
