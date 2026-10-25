#include "fluid.h"

#include <utility>

FluidSimulation::FluidSimulation(const std::vector<Vec2>& initial_positions, const FluidParameters& parameters)
    : parameters_(parameters),
      solver_(parameters.fluid_fraction),
      positions_(initial_positions),
      velocities_(initial_positions.size()) {}

std::vector<Polygon> FluidSimulation::step() {
    FluidTransportSolver::Result transport = solver_.solve(positions_);
    // If L-BFGS did not converge, the cells are unreliable and only gravity acts this step.
    last_solver_status_ = transport.status;
    const bool use_cells = transport.status == 0;

    const double stiffness = 1.0 / (parameters_.spring_epsilon * parameters_.spring_epsilon);
    for (std::size_t i = 0; i < positions_.size(); ++i) {
        Vec2 force = parameters_.particle_mass * parameters_.gravity;
        if (use_cells) force += stiffness * (transport.cells[i].centroid() - positions_[i]);

        // Semi-implicit Euler.
        velocities_[i] += parameters_.time_step * force / parameters_.particle_mass;
        positions_[i] += parameters_.time_step * velocities_[i];
        keep_inside_unit_square(positions_[i], velocities_[i]);
    }
    return std::move(transport.cells);
}

// Reflects a particle that left the domain back inside and stops its normal velocity.
void FluidSimulation::keep_inside_unit_square(Vec2& position, Vec2& velocity) const {
    if (position.x < 0.0) {
        position.x = -position.x;
        velocity.x = 0.0;
    }
    if (position.y < 0.0) {
        position.y = -position.y;
        velocity.y = 0.0;
    }
    if (position.x >= 1.0) {
        position.x = 2.0 - position.x;
        velocity.x = 0.0;
    }
    if (position.y >= 1.0) {
        position.y = 2.0 - position.y;
        velocity.y = 0.0;
    }
}
