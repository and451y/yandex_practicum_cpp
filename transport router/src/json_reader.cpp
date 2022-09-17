#include "json_reader.h"

void JsonHandler::ParseInputJson(const json::Document& doc) {
  using namespace std::literals;
  using namespace transport_catalogue;

  json::Node base_requests = doc.GetRoot().AsDict().at("base_requests"s);

  ParseStop(base_requests, GetTransportCatalogue());

  ParseStopDistance(base_requests, GetTransportCatalogue());

  ParseBus(base_requests, GetTransportCatalogue());

  FillRenderSettings(doc.GetRoot(), GetRenderSettings());

  ParseRoutingSettings(doc.GetRoot(), GetRoutingSettings());

  SetTransportRouter();

}

void JsonHandler::ParseRoutingSettings(const json::Node& json_base_req, RoutingSettings& routing_settings) {
  using namespace std::literals;

  json::Dict settings = json_base_req.AsDict().at("routing_settings"s).AsDict();
  routing_settings.velocity = settings.at("bus_velocity"s).AsDouble();
  routing_settings.wait_time = settings.at("bus_wait_time"s).AsInt();
}

void JsonHandler::FillRenderSettings(const json::Node& json_base_req, RendererSettings& ren_set) {
  using namespace std::literals;
  using namespace geo;


  json::Dict render_settings = json_base_req.AsDict().at("render_settings"s).AsDict();

  ren_set.bus_label_font_size = render_settings.at("bus_label_font_size"s).AsInt();
  ren_set.bus_label_offset = Coordinates({render_settings.at("bus_label_offset"s).AsArray()[0].AsDouble(),
                                          render_settings.at("bus_label_offset"s).AsArray()[1].AsDouble()});

  ParseColorPallete(render_settings.at("color_palette"s), ren_set);
  ParseUnderlayerColor(render_settings.at("underlayer_color"s), ren_set);

  ren_set.height = render_settings.at("height"s).AsDouble();
  ren_set.line_width = render_settings.at("line_width"s).AsDouble();
  ren_set.padding = render_settings.at("padding"s).AsDouble();
  ren_set.stop_label_font_size = render_settings.at("stop_label_font_size"s).AsInt();
  ren_set.stop_label_offset = Coordinates({render_settings.at("stop_label_offset"s).AsArray()[0].AsDouble(),
                                           render_settings.at("stop_label_offset"s).AsArray()[1].AsDouble()});

  ren_set.stop_radius = render_settings.at("stop_radius"s).AsDouble();
  ren_set.underlayer_width = render_settings.at("underlayer_width"s).AsDouble();
  ren_set.width = render_settings.at("width"s).AsDouble();
}

void JsonHandler::ParseColorPallete(const json::Node& node, RendererSettings& ren_set) {
  for (auto& color : node.AsArray()) {
    if (color.IsString()) {
      ren_set.color_pallete.emplace_back(color.AsString());
    } else if (json::Array rgb = color.AsArray(); rgb.size() == 3) {
      ren_set.color_pallete.emplace_back(svg::Rgb(rgb[0].AsDouble(), rgb[1].AsDouble(), rgb[2].AsDouble()));
    } else if (json::Array rgba = color.AsArray(); rgba.size() == 4) {
      ren_set.color_pallete.emplace_back(svg::Rgba(rgba[0].AsDouble(), rgba[1].AsDouble(), rgba[2].AsDouble(), rgba[3].AsDouble()));
    }
  }
}

void JsonHandler::ParseUnderlayerColor(const json::Node& node, RendererSettings& ren_set) {
  if (node.IsString()) {
    ren_set.underlayer_color = node.AsString();
  } else if (json::Array rgb = node.AsArray(); rgb.size() == 3) {
    ren_set.underlayer_color = svg::Rgb(rgb[0].AsDouble(), rgb[1].AsDouble(), rgb[2].AsDouble());
  } else if (json::Array rgba = node.AsArray(); rgb.size() == 4) {
    ren_set.underlayer_color = svg::Rgba(rgba[0].AsDouble(), rgba[1].AsDouble(), rgba[2].AsDouble(),
                                         rgba[3].AsDouble());
  }
}

void JsonHandler::OutputJson(const json::Document& doc, std::ostream& output) const {
  using namespace std::literals; using namespace transport_catalogue; using namespace json;

  const Dict& all_requests = doc.GetRoot().AsDict();
  json::Builder stats;
  stats.StartArray();

  for (const Node& req : all_requests.at("stat_requests"s).AsArray()) {
    json::Builder stat;
    stat.StartDict().Key("request_id"s).Value(req.AsDict().at("id"s));
    ProcessRequest(req, stat);
    stats.Value(stat.EndDict().Build());
  }

  Print(Document {stats.EndArray().Build()}, output);
}

void JsonHandler::ProcessRequest(const json::Node& request, json::Builder& stat) const {
  using namespace std::literals; using namespace transport_catalogue; using namespace json;
  std::string req_type = request.AsDict().at("type"s).AsString();

  if (req_type == "Stop"s) {
    ProcessStopReq(request, stat);
  }

  if (req_type == "Bus"s) {
    ProcessBusReq(request, stat);
  }

  if (req_type == "Map"s) {
    ProcessMapReq(stat);
  }

  if (req_type == "Route"s) {
    ProcessRouteReq(request, stat);
  }
}

void JsonHandler::ProcessStopReq(const json::Node& request, json::Builder& stat) const {
  using namespace std::literals; using namespace transport_catalogue;using namespace json;

  try {
    std::unordered_set<Bus*> buses = GetBusesByStop(request.AsDict().at("name"s).AsString());

    Array sort_by_name_buses = [&buses] {
      Array result;
      for (const auto& bus : buses) {
        result.push_back(Node(bus->name));
      }
      std::sort(result.begin(), result.end(), [](const Node& lhs, const Node& rhs) {return lhs.AsString() < rhs.AsString();});
      return result;
    }();

    stat.Key("buses"s).Value(Node(sort_by_name_buses));
  } catch (std::invalid_argument&) {
    stat.Key("error_message"s).Value(Node("not found"s));
  }

}

void JsonHandler::ProcessBusReq(const json::Node& request, json::Builder& stat) const {
  using namespace std::literals; using namespace transport_catalogue;using namespace json;

  std::string bus_name = request.AsDict().at("name"s).AsString();
  std::optional<BusStat> bus_stat(GetBusStat(bus_name));

  if (bus_stat) {
    stat.Key("curvature"s).Value(Node(bus_stat.value().curvature))
        .Key("route_length"s).Value(Node(bus_stat.value().real_distance))
        .Key("stop_count"s).Value(Node(bus_stat.value().stop_count))
        .Key("unique_stop_count"s).Value(Node(bus_stat.value().uniq_stops));
  } else {
    stat.Key("error_message"s).Value(Node("not found"s));
  }
}

void JsonHandler::ProcessRouteReq(const json::Node& request, json::Builder& stat) const {
  using namespace std::literals; using namespace transport_catalogue;using namespace json; using namespace transport_router;

  std::string from_stop = request.AsDict().at("from"s).AsString();
  std::string to_stop = request.AsDict().at("to"s).AsString();

    std::optional<std::pair<Minutes, std::vector<RouteItem>>> route_stat(GetTransportRouter().GetRouteInfo(from_stop, to_stop));

  if (!route_stat) {
    stat.Key("error_message"s).Value(Node("not found"s));
    return;
  }

  json::Builder builder_node;

  stat.Key("total_time"s).Value(Node(route_stat->first))
      .Key("items"s).StartArray();

  for (const auto& route_item : route_stat->second) {
      if (const auto wait_info_ptr = std::get_if<WaitInfo>(&route_item)) {
          stat.StartDict()
              .Key("type"s).Value(Node("Wait"s))
              .Key("stop_name"s).Value(Node(std::string(wait_info_ptr->stop_name)))
              .Key("time").Value(Node(wait_info_ptr->time)).EndDict();
      }

      if (const auto ride_info_prt = std::get_if<BusRideInfo>(&route_item)) {
          stat.StartDict()
              .Key("type"s).Value(Node("Bus"s))
              .Key("bus"s).Value(Node(std::string(ride_info_prt->bus_name)))
              .Key("span_count"s).Value(Node(ride_info_prt->span_count))
              .Key("time"s).Value(Node(ride_info_prt->time)).EndDict();
      }
  }

  stat.EndArray();
}

void JsonHandler::ProcessMapReq(json::Builder& stat) const {
  using namespace std::literals; using namespace transport_catalogue;using namespace json;

  std::string render_map;
  std::stringstream render_out(render_map);
  TransportCatalogue& tc = GetTransportCatalogue();
  const RendererSettings& rs = GetRenderSettings();
  MapRenderer renderer(tc, rs);
  renderer.Draw(render_out);

  stat.Key("map"s).Value(Node(render_out.str()));
}

void JsonHandler::ParseStop(const json::Node& json_base_req, transport_catalogue::TransportCatalogue& db) {
  using namespace std::literals;

  for (const auto& req : json_base_req.AsArray()) {
    if (req.AsDict().at("type"s).AsString() != "Stop"s)
      continue;

    std::string name = req.AsDict().at("name"s).AsString();
    db.AddStop(name, req.AsDict().at("latitude"s).AsDouble(), req.AsDict().at("longitude"s).AsDouble());
  }
}

void JsonHandler::ParseStopDistance(const json::Node& json_base_req, transport_catalogue::TransportCatalogue& db) {
  using namespace std::literals;

  for (const auto& req : json_base_req.AsArray()) {
    if (req.AsDict().at("type"s).AsString() != "Stop"s)
      continue;

    for (const auto& dist : req.AsDict().at("road_distances"s).AsDict()) {
      transport_catalogue::Stop* first_stop = const_cast<transport_catalogue::Stop*>(db.FindStop(req.AsDict().at("name"s).AsString()));
      transport_catalogue::Stop* second_stop = const_cast<transport_catalogue::Stop*>(db.FindStop(dist.first));

      db.SetDistance(first_stop, second_stop, dist.second.AsDouble());
    }
  }
}

void JsonHandler::ParseBus(const json::Node& json_base_req, transport_catalogue::TransportCatalogue& db) {
  using namespace std::literals;

  for (const auto& req : json_base_req.AsArray()) {
    if (req.AsDict().at("type"s).AsString() != "Bus"s)
      continue;

    std::vector<transport_catalogue::Stop*> route;

    for (const auto& stop : req.AsDict().at("stops"s).AsArray()) {
      transport_catalogue::Stop* finded_stop = const_cast<transport_catalogue::Stop*>(db.FindStop(stop.AsString()));
      route.push_back(finded_stop);
    }

    std::string name = req.AsDict().at("name"s).AsString();
    db.AddBus(name, route, req.AsDict().at("is_roundtrip"s).AsBool());
  }
}
