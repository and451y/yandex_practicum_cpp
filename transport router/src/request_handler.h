#pragma once

#include <exception>
#include <iomanip>
#include <optional>
#include <string_view>
#include <unordered_set>

#include "domain.h"
#include "transport_catalogue.h"
#include "transport_router.h"

class RequestHandler {
 public:
  RequestHandler(transport_catalogue::TransportCatalogue& db, RendererSettings& rs, RoutingSettings& routing_settings, transport_router::TransportRouter& transport_router)
      : db_(db),
        render_settings_(rs),
        routing_settings_(routing_settings),
        transport_router_(transport_router) {
  }

  transport_catalogue::TransportCatalogue& GetTransportCatalogue() const;

  RendererSettings& GetRenderSettings() const;

  RoutingSettings& GetRoutingSettings() const;

  void SetTransportRouter();

  transport_router::TransportRouter& GetTransportRouter() const;

  std::optional<transport_catalogue::BusStat> GetBusStat(const std::string_view& bus_name) const;

  std::unordered_set<transport_catalogue::Bus*> GetBusesByStop(const std::string_view& stop_name) const;

  virtual ~RequestHandler() = default;

 private:
  transport_catalogue::TransportCatalogue& db_;
  RendererSettings& render_settings_;
  RoutingSettings& routing_settings_;
  transport_router::TransportRouter& transport_router_;
};
