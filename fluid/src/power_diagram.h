#pragma once

#include <vector>

#include "polygon.h"

// Power diagram of weighted sites restricted to the unit square. The cell of site i is
//   { x : |x - p_i|^2 - w_i <= |x - p_j|^2 - w_j for all j }.
//
// For the free-surface fluid, each cell is further intersected with the disk
// |x - p_i|^2 - w_i <= -w_air, i.e. the region where the site beats the "air" phase,
// whose radius is sqrt(w_i - w_air).
std::vector<Polygon> compute_power_cells(const std::vector<Vec2>& sites, const std::vector<double>& weights,
                                         double air_weight);
