#pragma once

#include <algorithm>
#include <optional>
#include <vector>

#include "domain.h"
#include "geo.h"
#include "transport_catalogue.h"

inline const double EPSILON = 1e-6;

bool IsZero(double value);

struct RouteRender {
  svg::Polyline route;
  svg::Text route_start_name;
  svg::Text route_start_under_name;
  svg::Text route_end_name;
  svg::Text route_end_under_name;
  bool is_line_trip;
};

struct StopRender {
  std::string name;
  svg::Circle circle;
  svg::Text render_name;
  svg::Text render_under_name;
};

class SphereProjector {
 public:
  template<typename PointInputIt>
  SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
                  double max_height, double padding)
      : padding_(padding) {

    if (points_begin == points_end) {
      return;
    }

    const auto [left_it, right_it] = std::minmax_element(points_begin, points_end,
                                                         [](auto lhs, auto rhs) {
                                                           return lhs.lng < rhs.lng;
                                                         });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    const auto [bottom_it, top_it] = std::minmax_element(points_begin, points_end,
                                                         [](auto lhs, auto rhs) {
                                                           return lhs.lat < rhs.lat;
                                                         });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
      width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
      height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
      zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
      zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
      zoom_coeff_ = *height_zoom;
    }
  }

  svg::Point operator()(geo::Coordinates coords) const {
    return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
      (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
  }

 private:
  double padding_;
  double min_lon_ = 0;
  double max_lat_ = 0;
  double zoom_coeff_ = 0;
};

class MapRenderer {
public:
  MapRenderer(const transport_catalogue::TransportCatalogue& tc, const RendererSettings& rs)
    : tc_(tc), rs_(rs) {
  }

  void Draw(std::ostream& out);

private:
  const transport_catalogue::TransportCatalogue& tc_;
  const RendererSettings& rs_;

  std::vector<StopRender> RenderStops(std::vector<transport_catalogue::Stop*> stops, SphereProjector& sp) const;
  std::vector<RouteRender> RenderRoutes( std::vector<transport_catalogue::Bus*> all_buses, SphereProjector& sp) const;
  svg::Text SetRouteName(geo::Coordinates coordinates, std::string& bus_name, svg::Color color, SphereProjector& sp) const;
  svg::Text SetRouteUnderName(geo::Coordinates coordinates, std::string& bus_name, SphereProjector& sp) const;
  void FillDocument(svg::Document& doc, std::vector<StopRender>& stops_renders, std::vector<RouteRender>& routes_renders);

};


