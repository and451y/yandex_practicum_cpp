#include "map_renderer.h"

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

std::vector<StopRender> MapRenderer::RenderStops(std::vector<transport_catalogue::Stop*> all_stops, SphereProjector& sp) const {
  using namespace svg; using namespace transport_catalogue;

  std::vector<StopRender> stops_renders;
  std::vector<transport_catalogue::Stop*> stops = std::move(all_stops);
  std::vector<Stop*> sorted_stops_with_bus;
  for (const auto& stop : stops) {
    if (!tc_.GetAllBusForStop(stop->name).empty())
      sorted_stops_with_bus.emplace_back(stop);
  }

  std::sort(sorted_stops_with_bus.begin(), sorted_stops_with_bus.end(),
            [](const Stop* lhs, const Stop* rhs){return lhs->name < rhs->name;}
  );

  for (const auto& stop : sorted_stops_with_bus) {
    StopRender stop_render;
    stop_render.name = stop->name;
    stop_render.circle = std::move(Circle().SetCenter(sp(stop->coordinates)).SetRadius(rs_.stop_radius).SetFillColor("white"));

    stop_render.render_name = std::move(
        Text().SetPosition(sp(stop->coordinates)).SetData(stop->name).SetOffset(Point( {rs_.stop_label_offset.lat, rs_.stop_label_offset.lng}))
              .SetFontSize(uint32_t(rs_.stop_label_font_size)).SetFontFamily("Verdana").SetFillColor("black")
    );

    stop_render.render_under_name = std::move(
        Text().SetPosition(sp(stop->coordinates)).SetData(stop->name).SetOffset(Point( {rs_.stop_label_offset.lat, rs_.stop_label_offset.lng}))
              .SetFontSize(uint32_t(rs_.stop_label_font_size)).SetFontFamily("Verdana").SetFillColor(rs_.underlayer_color)
              .SetStrokeColor(rs_.underlayer_color).SetStrokeWidth(rs_.underlayer_width).SetStrokeLineCap(StrokeLineCap::ROUND)
              .SetStrokeLineJoin(StrokeLineJoin::ROUND)
    );

    stops_renders.push_back(std::move(stop_render));
  }

  return stops_renders;
}

std::vector<RouteRender> MapRenderer::RenderRoutes(std::vector<transport_catalogue::Bus*> all_buses, SphereProjector& sp) const {
  using namespace svg; using namespace transport_catalogue;

  std::vector<RouteRender> routes_renders;
  int color_counter = 0;

  std::vector<transport_catalogue::Bus*> sorted_all_bus = std::move(all_buses);
  std::sort(sorted_all_bus.begin(), sorted_all_bus.end(), [](const Bus* b1, const Bus* b2) {return b1->name < b2->name;});

  for (const auto& bus : sorted_all_bus) {
    if (bus->route.empty())
      continue;

    RouteRender route_render;
    Polyline poly;

    for (const Stop* stop : bus->route) {
      poly.AddPoint(sp(stop->coordinates));
    }
    poly.SetStrokeWidth(rs_.line_width).SetStrokeColor(rs_.color_pallete[color_counter]).SetStrokeLineCap(StrokeLineCap::ROUND)
        .SetStrokeLineJoin(StrokeLineJoin::ROUND).SetFillColor("none");
    route_render.route = std::move(poly);

    route_render.route_start_name = std::move(SetRouteName(bus->route.front()->coordinates, bus->name, rs_.color_pallete[color_counter], sp));
    route_render.route_start_under_name = std::move(SetRouteUnderName(bus->route.front()->coordinates, bus->name, sp));

    if (*bus->route.begin() != bus->last_stop) {
      route_render.is_line_trip = true;
      route_render.route_end_name = std::move(SetRouteName(bus->route[bus->route.size()/2]->coordinates, bus->name, rs_.color_pallete[color_counter], sp));
      route_render.route_end_under_name = std::move(SetRouteUnderName(bus->route[bus->route.size() / 2]->coordinates, bus->name, sp));
    } else {
      route_render.is_line_trip = false;
    }

    routes_renders.push_back(std::move(route_render));
    color_counter = (color_counter + 1) % int(rs_.color_pallete.size());
  }

  return routes_renders;
}

svg::Text MapRenderer::SetRouteName(geo::Coordinates coordinates, std::string& bus_name, svg::Color color, SphereProjector& sp) const {
  using namespace svg;

   return Text().SetPosition(sp(coordinates)).SetData(bus_name).SetFillColor(color)
                .SetOffset(Point( {rs_.bus_label_offset.lat, rs_.bus_label_offset.lng}))
                .SetFontSize(rs_.bus_label_font_size).SetFontFamily("Verdana")
                .SetFontWeight("bold");
}

svg::Text MapRenderer::SetRouteUnderName(geo::Coordinates coordinates, std::string& bus_name, SphereProjector& sp) const {
  using namespace svg;

  return Text().SetPosition(sp(coordinates)).SetData(bus_name).SetFillColor(rs_.underlayer_color)
      .SetOffset(Point( {rs_.bus_label_offset.lat, rs_.bus_label_offset.lng})).SetFontSize(
      rs_.bus_label_font_size).SetFontFamily("Verdana").SetFontWeight("bold").SetStrokeColor(
      rs_.underlayer_color).SetStrokeWidth(rs_.underlayer_width).SetStrokeLineCap(
      StrokeLineCap::ROUND).SetStrokeLineJoin(StrokeLineJoin::ROUND);
}


void MapRenderer::Draw(std::ostream& out) {
  using namespace transport_catalogue;
  using namespace svg;

  Document doc;
  std::vector<geo::Coordinates> stops_coordinates;
  std::vector<StopRender> stops_renders;
  std::vector<RouteRender> routes_renders;

  for (const auto& stop : tc_.GetAllStops()) {
    if (tc_.GetAllBusForStop(stop->name).size() != 0) {
      stops_coordinates.emplace_back(stop->coordinates);
    }
  }

  SphereProjector sp(stops_coordinates.begin(), stops_coordinates.end(),
                      rs_.width, rs_.height, rs_.padding);

  stops_renders = std::move(RenderStops(tc_.GetAllStops(), sp));
  routes_renders = std::move(RenderRoutes(tc_.GetAllBuses(), sp));

  FillDocument(doc, stops_renders, routes_renders);

  doc.Render(out);
}

void MapRenderer::FillDocument(svg::Document& doc, std::vector<StopRender>& stops_renders, std::vector<RouteRender>& routes_renders) {
  for (const auto bus : routes_renders) {
    doc.Add(bus.route);
  }

  for (const auto bus : routes_renders) {
    doc.Add(bus.route_start_under_name);
    doc.Add(bus.route_start_name);

    if (bus.is_line_trip) {
      doc.Add(bus.route_end_under_name);
      doc.Add(bus.route_end_name);
    }
  }

  for (const auto stop : stops_renders) {
    doc.Add(stop.circle);
  }

  for (const auto stop : stops_renders) {
    doc.Add(stop.render_under_name);
    doc.Add(stop.render_name);
  }
}

