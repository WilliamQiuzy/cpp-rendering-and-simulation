#pragma once

#include <string>
#include <vector>

#include "polygon.h"

// Rasterizes fluid cells (blue, with black outlines) on a white square image and
// writes it as a PNG. Returns false if the file could not be written.
bool write_frame_png(const std::vector<Polygon>& cells, const std::string& path, int size = 1000);
