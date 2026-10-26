#pragma once

#include <vector>

#include "polygon.h"

// Semi-discrete optimal transport between the uniform measure on the unit square and
// N fluid particles plus one "air" phase. Each particle must receive area
// fluid_fraction / N; the air receives the remaining 1 - fluid_fraction.
//
// The transport plan is a power diagram whose weights maximize the concave dual
//   g(W) = sum_i [ integral over cell_i of |x - p_i|^2 - w_i |cell_i| + lambda w_i ]
//          + w_air (air_fraction - air_area),
// with gradient dg/dw_i = lambda - |cell_i|. We maximize g with L-BFGS.
class FluidTransportSolver {
public:
    explicit FluidTransportSolver(double fluid_fraction) : fluid_fraction_(fluid_fraction) {}

    struct Result {
        std::vector<Polygon> cells;  // one per particle; the air is the complement
        int status = 0;              // liblbfgs return code, 0 on convergence
    };

    Result solve(const std::vector<Vec2>& particles) const;

    // Negated dual value and gradient for weights `x` (N particle weights, then the air
    // weight), in the form liblbfgs minimizes. Public so the callback can reach it.
    double negated_dual(const std::vector<Vec2>& particles, const double* x, double* gradient, int n) const;

private:
    double fluid_fraction_;
};
