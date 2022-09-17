#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "geo.h"

namespace transport_catalogue {

struct Stop {
  std::string name;
  geo::Coordinates coordinates;
};

struct BusStat {
  int uniq_stops;
  int stop_count;
  double real_distance;
  double curvature;
};


struct Bus {
  std::string name;
  std::vector<Stop*> raw_route;
  std::vector<Stop*> route;
  Stop* last_stop;

  BusStat bus_stat;
};


class NameHasher {
 public:
     size_t operator()(const std::string_view stop) const noexcept {
         return sv_hasher_(stop);
     }

 private:
     std::hash<std::string_view> sv_hasher_;
};

class PairHasher {
 public:
     size_t operator()(const std::pair<const Stop*, const Stop*> stops) const noexcept {
         return stops_hasher_(stops.first) + stops_hasher_(stops.second);
     }

 private:
     std::hash<const void*> stops_hasher_;
};

} // transport_catalogue

struct RendererSettings {
    double width, height, padding, line_width, stop_radius, underlayer_width;
    int bus_label_font_size, stop_label_font_size;
    geo::Coordinates bus_label_offset, stop_label_offset;
    svg::Color underlayer_color;
    std::vector<svg::Color> color_pallete;
};

template <typename Container, typename Element>
int UniqCounter(Container& container) {
  std::vector<Element> temp(container.begin(), container.end());
  std::sort(temp.begin(), temp.end());
  auto last = std::unique(temp.begin(), temp.end());
  // v now holds {1 2 3 4 5 x x}, where 'x' is indeterminate
  temp.erase(last, temp.end());

  return int(temp.size());
}

struct RoutingSettings {
    int wait_time;
    double velocity;
};
