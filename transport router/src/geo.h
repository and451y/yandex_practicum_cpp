#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

#include "svg.h"

const int kEarthRadius = 6371000;

namespace geo {

struct Coordinates {
    double lat; // Широта
    double lng; // Долгота
};

double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace geo
