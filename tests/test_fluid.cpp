#include <cmath>
#include <random>
#include <vector>

#include "check.h"
#include "optimal_transport.h"
#include "polygon.h"
#include "power_diagram.h"

namespace {

Polygon unit_square(bool clockwise) {
    Polygon square;
    square.vertices = clockwise ? std::vector<Vec2>{{0, 0}, {0, 1}, {1, 1}, {1, 0}}
                                : std::vector<Vec2>{{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    return square;
}

std::vector<Vec2> random_sites(int count, unsigned seed) {
    std::mt19937 engine(seed);
    std::uniform_real_distribution<double> unit(0.05, 0.95);
    std::vector<Vec2> sites(count);
    for (Vec2& s : sites) s = Vec2(unit(engine), unit(engine));
    return sites;
}

}  // namespace

TEST(polygon_area_and_centroid_any_orientation) {
    for (bool clockwise : {true, false}) {
        const Polygon square = unit_square(clockwise);
        CHECK_NEAR(square.area(), 1.0, 1e-12);
        CHECK_NEAR(square.centroid().x, 0.5, 1e-12);
        CHECK_NEAR(square.centroid().y, 0.5, 1e-12);
    }
}

// Closed forms: over the unit square, the integral of |x - c|^2 is 1/6 about the center
// and 2/3 about a corner.
TEST(polygon_integrated_squared_distance) {
    const Polygon square = unit_square(true);
    CHECK_NEAR(square.integrated_squared_distance(Vec2(0.5, 0.5)), 1.0 / 6.0, 1e-12);
    CHECK_NEAR(square.integrated_squared_distance(Vec2(0.0, 0.0)), 2.0 / 3.0, 1e-12);
}

// With equal weights and an air phase that never wins, the cells tile the unit square.
TEST(voronoi_cells_partition_the_square) {
    const std::vector<Vec2> sites = random_sites(25, 3);
    const std::vector<Polygon> cells = compute_power_cells(sites, std::vector<double>(sites.size(), 0.0), -100.0);
    double total = 0.0;
    for (const Polygon& cell : cells) total += cell.area();
    CHECK_NEAR(total, 1.0, 1e-9);
}

// Two sites at x = 0.25 and 0.75 with weights 0.1 and 0: the power bisector moves to x = 0.6.
TEST(power_weights_shift_the_bisector) {
    const std::vector<Vec2> sites = {{0.25, 0.5}, {0.75, 0.5}};
    const std::vector<Polygon> cells = compute_power_cells(sites, {0.1, 0.0}, -100.0);
    CHECK_NEAR(cells[0].area(), 0.6, 1e-12);
    CHECK_NEAR(cells[1].area(), 0.4, 1e-12);
}

// A lone fluid particle owns the disk of radius sqrt(w - w_air) around it (a 200-gon here).
TEST(air_phase_clips_cells_to_disks) {
    const std::vector<Vec2> sites = {{0.5, 0.5}};
    const double radius = 0.2;
    const std::vector<Polygon> cells = compute_power_cells(sites, {radius * radius}, 0.0);
    CHECK_NEAR(cells[0].area(), M_PI * radius * radius, 1e-3);
    CHECK(compute_power_cells(sites, {-0.01}, 0.0)[0].empty());  // air wins everywhere
}

// The transport solver must give every particle the same share of the fluid volume.
TEST(transport_solver_equalizes_cell_areas) {
    const double fluid_fraction = 0.4;
    const std::vector<Vec2> particles = random_sites(12, 11);
    const FluidTransportSolver::Result result = FluidTransportSolver(fluid_fraction).solve(particles);
    CHECK(result.status == 0);
    double total = 0.0;
    for (const Polygon& cell : result.cells) {
        CHECK_NEAR(cell.area(), fluid_fraction / particles.size(), 1e-3);
        total += cell.area();
    }
    CHECK_NEAR(total, fluid_fraction, 1e-3);
}

int main() { return check::run_all(); }
