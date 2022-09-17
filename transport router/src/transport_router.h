#pragma once

#include "transport_catalogue.h"
#include "graph.h"
#include "domain.h"
#include "router.h"
#include "transport_catalogue.pb.h"

namespace transport_router {

using Minutes = double;

struct BusRideInfo {
    bool operator==(const BusRideInfo &rhs) const {
        return bus_name == rhs.bus_name &&
               span_count == rhs.span_count &&
               time == rhs.time;
    }

    bool operator!=(const BusRideInfo &rhs) const {
        return !(rhs == *this);
    }

    std::string_view bus_name;
    int span_count = 0;
    Minutes time = 0;
};

struct WaitInfo {
    bool operator==(const WaitInfo &rhs) const {
        return stop_name == rhs.stop_name &&
               time == rhs.time;
    }

    bool operator!=(const WaitInfo &rhs) const {
        return !(rhs == *this);
    }


    std::string_view stop_name;
    Minutes time = 0;
};

struct VertexIds {
    bool operator==(const VertexIds &rhs) const {
        return in == rhs.in &&
               out == rhs.out;
    }

    bool operator!=(const VertexIds &rhs) const {
        return !(rhs == *this);
    }

    graph::VertexId in = 0;
    graph::VertexId out = 0;
};

using RouteItem = std::variant<std::monostate, WaitInfo, BusRideInfo>;

class TransportRouter {
public:
  explicit TransportRouter(const transport_catalogue::TransportCatalogue& catalogue)
    : catalogue_(catalogue) {
  }

  void InitRouter(const RoutingSettings& settings);
    void SetRouter(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                   const std::vector<transport_catalogue::Stop *> &all_stops,
                   const std::vector<transport_catalogue::Bus *> &all_buses);
  std::optional<std::pair<Minutes, std::vector<RouteItem>>> GetRouteInfo(std::string_view from, std::string_view to) const;

    const graph::DirectedWeightedGraph<Minutes> &GetGraph() const;

    const std::unique_ptr<graph::Router<Minutes>> &GetRouter() const;
    void SetSettings(const RoutingSettings &settings);
private:
    const transport_catalogue::TransportCatalogue& catalogue_;
    RoutingSettings settings_{};
    graph::DirectedWeightedGraph<Minutes> graph_;
    std::unique_ptr<graph::Router<Minutes>> router_;

  graph::VertexId vertexes_counter_ = 0;
  std::unordered_map<std::string_view, VertexIds> vertexes_;
    std::unordered_map<graph::EdgeId, WaitInfo> wait_edges_;
    std::unordered_map<graph::EdgeId, BusRideInfo> bus_edges_;
public:
    void setBusEdges(const std::unordered_map<graph::EdgeId, BusRideInfo> &busEdges);

public:

    void setVertexesCounter(unsigned long vertexesCounter);

    void setVertexes(const std::unordered_map<std::string_view, VertexIds> &vertexes);
    void setWaitEdges(const std::unordered_map<graph::EdgeId, WaitInfo> &waitEdges);
private:
public:

    graph::VertexId getVertexesCounter() const;

    const std::unordered_map<std::string_view, VertexIds> &getVertexes() const;

    const std::unordered_map<graph::EdgeId, WaitInfo> &getWaitEdges() const;

    const std::unordered_map<graph::EdgeId, BusRideInfo> &getBusEdges() const;
private:

  void CreateEdges();
  void SetWaitEdges();
  size_t CreateVertexes();
  void CreateWaitEdges();
  void CreateBusEdges();
  Minutes CalculateTripTime(const transport_catalogue::Stop* from, const transport_catalogue::Stop* to) const;

  template<typename It>
  void ConnectStations(It begin, It end, std::string_view bus_name);
};


template<typename It>
void TransportRouter::ConnectStations(It begin, It end, std::string_view bus_name) {
  for (auto from_it = begin; from_it != end; ++from_it) {
    for (auto to_it = std::next(from_it); to_it != end; ++to_it) {
      std::string_view departure_name = (*from_it)->name;
      graph::VertexId departure = vertexes_.at(departure_name).out;

      std::string_view arrival_name = (*to_it)->name;
      graph::VertexId arrival = vertexes_.at(arrival_name).in;

      Minutes weight = 0;
      int span_count = 0;

      for (auto it = from_it; it != to_it; ++it) {
        weight += CalculateTripTime(*it, *(it + 1));
        ++span_count;
      }

      auto bus_edge_id = graph_.AddEdge({departure, arrival, weight});

      bus_edges_[bus_edge_id] = {bus_name, span_count, weight};
    }
  }
}

} // transport_router
