#pragma once

#include <algorithm>
#include <deque>
#include <exception>
#include <functional>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "domain.h"

namespace transport_catalogue {

class TransportCatalogue {
 public:

  void AddBus(std::string& name, std::vector<Stop*> route, bool is_roundtrip);
  const Bus* FindBus(std::string_view name) const;
  std::set<std::string_view> GetAllBusForStop(const std::string& name) const;
  std::vector<Bus*> GetAllBuses() const;

  void AddStop(std::string& name, double latitude, double longitude);
  void SetDistance(const Stop* first_stop, const Stop* second_stop, double distance);
  double GetDistance(const Stop* first_stop, const Stop* second_stop) const;
  const Stop* FindStop(const std::string& name) const;
  std::vector<Stop*> GetAllStops() const;

    const std::unordered_map<std::pair<const Stop *, const Stop *>, double, transport_catalogue::PairHasher> &
    GetStopToStopDistance() const;


private:

  std::deque<Bus> buses_;
  std::deque<Stop> stops_;
  std::unordered_map<std::string_view, const Bus*, transport_catalogue::NameHasher> bus_index;
  std::unordered_map<std::string_view, const Stop*, transport_catalogue::NameHasher> stop_index;
  std::unordered_map<Stop*, std::set<std::string_view>> stop_to_buses;
  std::unordered_map<std::pair<const Stop*, const Stop*>, double, transport_catalogue::PairHasher> stop_to_stop_distance;

    [[maybe_unused]] static double ComputeLenghtRoute(std::vector<Stop*>& route, const std::function<double(const Stop*, const Stop*)>& func);

};

} // namespace transport_catalogue
