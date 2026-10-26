#include "optimal_transport.h"

#include <vector>

#include "lbfgs.h"
#include "power_diagram.h"

namespace {

struct CallbackContext {
    const FluidTransportSolver* solver;
    const std::vector<Vec2>* particles;
};

lbfgsfloatval_t evaluate(void* instance, const lbfgsfloatval_t* x, lbfgsfloatval_t* gradient, int n,
                         lbfgsfloatval_t /*step*/) {
    const auto* context = static_cast<const CallbackContext*>(instance);
    return context->solver->negated_dual(*context->particles, x, gradient, n);
}

}  // namespace

double FluidTransportSolver::negated_dual(const std::vector<Vec2>& particles, const double* x, double* gradient,
                                          int n) const {
    const int particle_count = n - 1;
    const std::vector<double> weights(x, x + particle_count);
    const double air_weight = x[particle_count];
    const std::vector<Polygon> cells = compute_power_cells(particles, weights, air_weight);

    const double target_area = fluid_fraction_ / particle_count;  // lambda
    const double air_fraction = 1.0 - fluid_fraction_;

    double dual = 0.0;
    double fluid_area = 0.0;
    for (int i = 0; i < particle_count; ++i) {
        const Polygon& cell = cells[i];
        const double area = cell.area();
        fluid_area += area;
        gradient[i] = area - target_area;
        dual += cell.integrated_squared_distance(particles[i]) - weights[i] * area + target_area * weights[i];
    }

    const double air_area = 1.0 - fluid_area;
    gradient[particle_count] = air_area - air_fraction;
    dual += air_weight * (air_fraction - air_area);

    return -dual;
}

FluidTransportSolver::Result FluidTransportSolver::solve(const std::vector<Vec2>& particles) const {
    const int n = static_cast<int>(particles.size()) + 1;
    lbfgsfloatval_t* x = lbfgs_malloc(n);
    for (int i = 0; i + 1 < n; ++i) x[i] = 1.0 / particles.size();
    x[n - 1] = 0.0;  // air weight

    lbfgs_parameter_t parameters;
    lbfgs_parameter_init(&parameters);
    parameters.linesearch = LBFGS_LINESEARCH_BACKTRACKING_ARMIJO;
    parameters.max_iterations = 10000;

    CallbackContext context{this, &particles};
    lbfgsfloatval_t final_value = 0.0;
    Result result;
    result.status = lbfgs(n, x, &final_value, evaluate, nullptr, &context, &parameters);

    const std::vector<double> weights(x, x + n - 1);
    result.cells = compute_power_cells(particles, weights, x[n - 1]);
    lbfgs_free(x);
    return result;
}
