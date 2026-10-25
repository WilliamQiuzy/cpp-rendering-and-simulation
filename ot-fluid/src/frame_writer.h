#include "Vector.h"

#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <sstream>

#include "Polygon.h"

#ifndef SAVESVG_H
#define SAVESVG_H

void save_svg(const std::vector<Polygon> &polygons, std::string filename, std::string fillcol = "none");

void save_svg_animated(const std::vector<Polygon> &polygons, std::string filename, int frameid, int nbframes);

void save_frame(const std::vector<Polygon> &cells, std::string filename, int frameid = 0);

#endif // SAVESVG_H
