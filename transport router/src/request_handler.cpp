#include "request_handler.h"

transport_catalogue::TransportCatalogue& RequestHandler::GetTransportCatalogue() const {
  return db_;
}

RendererSettings& RequestHandler::GetRenderSettings() const {
  return render_settings_;
}

RoutingSettings& RequestHandler::GetRoutingSettings() const {
  return routing_settings_;
}

void RequestHandler::SetTransportRouter() {
  transport_router_.InitRouter(routing_settings_);
}

transport_router::TransportRouter& RequestHandler::GetTransportRouter() const {
  return transport_router_;
}



std::optional<transport_catalogue::BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
  std::optional<transport_catalogue::BusStat> bus_stat;

  try {
    bus_stat = db_.FindBus(bus_name)->bus_stat;
  } catch (std::invalid_argument&) {
    return bus_stat;
  }

  return bus_stat;
}

std::unordered_set<transport_catalogue::Bus*> RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
  using namespace std::literals;

  std::unordered_set<transport_catalogue::Bus*> buses;

  try {
    std::set<std::string_view> buses_name = db_.GetAllBusForStop(std::string(stop_name));

    for (const auto& bus : buses_name) {
      buses.insert(const_cast<transport_catalogue::Bus*>(db_.FindBus(bus)));
    }
  } catch (std::invalid_argument&) {
    throw std::invalid_argument("No stop"s);
  }

  return buses;
}
