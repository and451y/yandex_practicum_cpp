#pragma once

#include <algorithm>
#include <iostream>

#include "domain.h"
#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "svg.h"
#include "transport_catalogue.h"
#include "json_builder.h"
#include "transport_router.h"

class JsonHandler final : public RequestHandler {
  using RequestHandler::RequestHandler;

 public:

  void ParseInputJson(const json::Document& doc);

  void OutputJson(const json::Document& doc, std::ostream& output) const;

 private:

  void ParseBaseReq(const json::Node& all_req);

  void ParseStop(const json::Node& json_base_req, transport_catalogue::TransportCatalogue& db);

  void ParseStopDistance(const json::Node& json_base_req, transport_catalogue::TransportCatalogue& db);

  void ParseBus(const json::Node& json_base_req, transport_catalogue::TransportCatalogue& db);

  void ParseRoutingSettings(const json::Node& json_base_req, RoutingSettings& routing_settings);

  void FillRenderSettings(const json::Node& json_base_req, RendererSettings& ren_set);

  void ParseColorPallete(const json::Node& node, RendererSettings& ren_set);

  void ParseUnderlayerColor(const json::Node& node, RendererSettings& ren_set);

  void ProcessRequest(const json::Node& request, json::Builder& stat) const;
  void ProcessStopReq(const json::Node& request, json::Builder& stat) const;
  void ProcessBusReq(const json::Node& request, json::Builder& stat) const;
  void ProcessRouteReq(const json::Node& request, json::Builder& stat) const;
  void ProcessMapReq(json::Builder& stat) const;

};

