#include "transport_router.h"

using namespace transport_router;

void TransportRouter::InitRouter(const RoutingSettings& settings) {
  settings_ = settings;
  graph_ = graph::DirectedWeightedGraph<Minutes>(CreateVertexes());
  CreateEdges();
  router_ = std::make_unique<graph::Router<Minutes>>(&graph_);
}

std::optional<std::pair<Minutes, std::vector<RouteItem>>> TransportRouter::GetRouteInfo(std::string_view from, std::string_view to) const {
  graph::VertexId from_vertex = vertexes_.at(from).in;
  graph::VertexId to_vertex = vertexes_.at(to).in;

  std::optional<graph::Router<Minutes>::RouteInfo> route_info = router_->BuildRoute(from_vertex, to_vertex);

  if (!route_info.has_value()) {
    return {};
  }

  std::pair<Minutes, std::vector<RouteItem>> output;
  output.first = route_info->weight;

  for (const auto& edge_id : route_info->edges) {
    if (bus_edges_.count(edge_id))
      output.second.emplace_back(bus_edges_.at(edge_id));

    if (wait_edges_.count(edge_id))
      output.second.emplace_back(wait_edges_.at(edge_id));
  }

  return output;
}

void TransportRouter::CreateEdges() {
    CreateWaitEdges();
    CreateBusEdges();
}

size_t TransportRouter::CreateVertexes() {
  using namespace transport_catalogue;

  for (const Stop* stop : catalogue_.GetAllStops()) {
    vertexes_[stop->name].in = vertexes_counter_++;
    vertexes_[stop->name].out = vertexes_counter_++;
  }

  return vertexes_counter_;
}

void TransportRouter::CreateWaitEdges() {
  using namespace transport_catalogue;

  for (const Stop* stop : catalogue_.GetAllStops()) {
    wait_edges_.insert({graph_.AddEdge({vertexes_.at(stop->name).in, vertexes_.at(stop->name).out,static_cast<Minutes>(settings_.wait_time)}),
                       {stop->name,static_cast<Minutes>(settings_.wait_time)}});
  }
}

void TransportRouter::SetWaitEdges() {
    using namespace transport_catalogue;

    int i = 0;

    for (const Stop* stop : catalogue_.GetAllStops()) {
        wait_edges_.insert({i, {stop->name,static_cast<Minutes>(settings_.wait_time)}});

        ++i;
    }
}

void TransportRouter::CreateBusEdges() {
  using namespace transport_catalogue;

  for (const Bus* bus : catalogue_.GetAllBuses()) {
    ConnectStations(bus->route.begin(), bus->route.end(), bus->name);

    if (bus->last_stop != bus->route.front())
      ConnectStations(bus->route.rbegin(), bus->route.rend(), bus->name);
  }
}

Minutes TransportRouter::CalculateTripTime(const transport_catalogue::Stop* from, const transport_catalogue::Stop* to) const {
  return 60.0 * catalogue_.GetDistance(from, to) / (1000.0 * settings_.velocity);
}

//void TransportRouter::SetRouter(const transport_catalogue_serialize::TransportCatalogue &db_serialize) {
//    settings_.velocity = db_serialize.routing_settings().bus_velocity();
//    settings_.wait_time = db_serialize.routing_settings().bus_wait_time();
//
//
////    graph_ = db_serialize.graph();
//    CreateEdges();
////    router_ = std::make_unique<graph::Router<Minutes>>(graph_);
//}

const graph::DirectedWeightedGraph<Minutes> &TransportRouter::GetGraph() const {
    return graph_;
}

const std::unique_ptr<graph::Router<Minutes>> &TransportRouter::GetRouter() const {
    return router_;
}

void TransportRouter::SetRouter(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                const std::vector<transport_catalogue::Stop *> &all_stops,
                                const std::vector<transport_catalogue::Bus *> &all_buses) {

    router_ = std::make_unique<graph::Router<Minutes>>();

    for (const auto& edge_ser : db_serialize.transport_router().graph().edge()) {
        graph::Edge<double> edge = {edge_ser.from(), edge_ser.to(), edge_ser.weight()};
        graph_.GetMutableEdges().push_back(edge);
    }

    for (const auto& incidence_list_ser : db_serialize.transport_router().graph().incidence_list()) {

        std::vector<size_t> incidence_list;
        incidence_list.reserve(incidence_list_ser.edgeid_size());
        for (const auto& edgeid: incidence_list_ser.edgeid()) {
            incidence_list.emplace_back(edgeid);
        }

        graph_.GetMutableIncidenceLists().emplace_back(std::move(incidence_list));
    }

    router_->SetGraph(&graph_);

    for (const auto& foo_ser : db_serialize.transport_router().router().vector_route_internal_data()) {

        std::vector<std::optional<graph::Router<double>::RouteInternalData>> vector;
        vector.reserve(foo_ser.internal_data_size());

        for (const auto& bar_ser : foo_ser.internal_data()) {

            graph::Router<double>::RouteInternalData data;
            if (!bar_ser.empty()) {
                data.weight = bar_ser.value().weight();
                if (!bar_ser.value().prev_edge().empty()) {
                    data.prev_edge = bar_ser.value().prev_edge().value();
                }
                vector.emplace_back(data);
            } else {
                vector.push_back({});
            }
        }

        router_->GetMutableInternalData().emplace_back(vector);

    }

    setVertexesCounter(db_serialize.transport_router().vertexes_counter());

    for (const auto& vertex : db_serialize.transport_router().vertexes()) {
        vertexes_.insert({all_stops[vertex.stop_id()]->name, {vertex.in(), vertex.out()}});
    }

    for (const auto& wait_edge : db_serialize.transport_router().wait_edges()) {
        wait_edges_.insert({wait_edge.edgeid(), {all_stops[wait_edge.stop_id()]->name, wait_edge.time()}});
    }

    for (const auto& bus_edge : db_serialize.transport_router().bus_edges()) {
        bus_edges_.insert({bus_edge.edge_id(), {all_buses[bus_edge.bus_id()]->name, bus_edge.span_count(), bus_edge.time()}});
    }

}

void TransportRouter::SetSettings(const RoutingSettings &settings) {
    settings_ = settings;
}

graph::VertexId TransportRouter::getVertexesCounter() const {
    return vertexes_counter_;
}

const std::unordered_map<std::string_view, VertexIds> &TransportRouter::getVertexes() const {
    return vertexes_;
}

const std::unordered_map<graph::EdgeId, WaitInfo> &TransportRouter::getWaitEdges() const {
    return wait_edges_;
}

const std::unordered_map<graph::EdgeId, BusRideInfo> &TransportRouter::getBusEdges() const {
    return bus_edges_;
}

void TransportRouter::setVertexesCounter(unsigned long vertexesCounter) {
    vertexes_counter_ = vertexesCounter;
}

void TransportRouter::setVertexes(const std::unordered_map<std::string_view, VertexIds> &vertexes) {
    vertexes_ = vertexes;
}

void TransportRouter::setWaitEdges(const std::unordered_map<graph::EdgeId, WaitInfo> &waitEdges) {
    wait_edges_ = waitEdges;
}

void TransportRouter::setBusEdges(const std::unordered_map<graph::EdgeId, BusRideInfo> &busEdges) {
    bus_edges_ = busEdges;
}