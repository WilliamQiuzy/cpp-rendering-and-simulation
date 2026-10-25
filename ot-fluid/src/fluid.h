#pragma once

#include <vector>

#include "optimal_transport.h"

struct FluidParameters {
    double fluid_fraction = 0.4;   // share of the unit square occupied by fluid
    double particle_mass = 200.0;
    double spring_epsilon = 0.004; // stiffness 1/eps^2 pulling particles to their cell centroids
    double time_step = 0.003;
    Vec2 gravity{0.0, -9.81};
};

// Gallouet-Merigot style incompressible fluid: at every step, optimal transport assigns
// each particle a cell of equal area, and a stiff spring pulls the particle toward the
// cell centroid, which keeps the fluid volume constant.
class FluidSimulation {
public:
    FluidSimulation(const std::vector<Vec2>& initial_positions, const FluidParameters& parameters);

    // Solves the transport problem for the current positions, then advances one time step.
    // Returns the cells used for this step (useful for drawing).
    std::vector<Polygon> step();

    // liblbfgs status of the most recent transport solve (0 when it converged).
    int last_solver_status() const { return last_solver_status_; }

private:
    void keep_inside_unit_square(Vec2& position, Vec2& velocity) const;

    FluidParameters parameters_;
    FluidTransportSolver solver_;
    std::vector<Vec2> positions_;
    std::vector<Vec2> velocities_;
    int last_solver_status_ = 0;
};
