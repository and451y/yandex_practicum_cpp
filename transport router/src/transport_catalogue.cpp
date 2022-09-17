#include "transport_catalogue.h"

using namespace std::literals;

using namespace transport_catalogue;

void TransportCatalogue::AddBus(std::string& name, std::vector<Stop*> route, bool is_roundtrip) {
    Bus bus;
    bus.name = std::move(name);
    bus.raw_route = std::move(route);
    bus.route = bus.raw_route;
    bus.last_stop = bus.route.back();

    if (!is_roundtrip) {
        bus.route.insert(bus.route.end(), std::next(bus.route.rbegin()), bus.route.rend());
    }

    double real_distance = 0;
    double ideal_distance = 0;

    for (auto it = bus.route.begin(); it != bus.route.end(); ++it) {
        if (std::next(it) == bus.route.end())
            break;

        auto from = *it;
        auto to = *std::next(it);

        real_distance += GetDistance(from, to);
        ideal_distance += geo::ComputeDistance(from->coordinates, to->coordinates);
    }

    bus.bus_stat.real_distance = real_distance;
    bus.bus_stat.curvature = real_distance / ideal_distance;
    bus.bus_stat.uniq_stops = UniqCounter<std::vector<Stop*>, Stop*>(bus.route);
    bus.bus_stat.stop_count = int(bus.route.size());

    buses_.push_back(std::move(bus));
    bus_index.emplace(buses_.back().name, &buses_.back());

    for (const auto& stop_ptr : buses_.back().route) {
        stop_to_buses.at(stop_ptr).insert(buses_.back().name);
    }
}

void TransportCatalogue::AddStop(std::string& name, double latitude, double longtitude) {
  Stop stop;
  stop.name = name;
  stop.coordinates = {latitude, longtitude};

  stops_.push_back(std::move(stop));

  stop_index.emplace(stops_.back().name, &stops_.back());

  stop_to_buses[&stops_.back()];
}

void TransportCatalogue::SetDistance(const Stop* first_stop, const Stop* second_stop,
                                     double distance) {
  std::pair<const Stop*, const Stop*> stops = {first_stop, second_stop};
  stop_to_stop_distance.emplace(stops, distance);
}


double TransportCatalogue::GetDistance(const Stop* first_stop, const Stop* second_stop) const {
  std::pair<const Stop*, const Stop*> stops = {first_stop, second_stop};
  auto result = stop_to_stop_distance.find(stops);

  if (result == stop_to_stop_distance.end()) {
    std::pair<const Stop*, const Stop*> reverse_stops = {second_stop, first_stop};
    result = stop_to_stop_distance.find(reverse_stops);
  }

  if (result == stop_to_stop_distance.end()) {
    throw std::invalid_argument("Stop to stop not found"s);
  }

  return result->second;
}

const Stop* TransportCatalogue::FindStop(const std::string& name) const {
  auto result = stop_index.find(name);
  if (result == stop_index.end())
    throw std::invalid_argument("Stop not found"s);

  return result->second;
}

const Bus* TransportCatalogue::FindBus(const std::string_view name) const {
  auto result = bus_index.find(name);
  if (result == bus_index.end())
    throw std::invalid_argument("Bus not found"s);

  return result->second;
}

std::set<std::string_view> TransportCatalogue::GetAllBusForStop(const std::string& name) const {
  try {
    auto result = stop_to_buses.find(const_cast<Stop*>(FindStop(name)));
    return result->second;
  } catch (std::invalid_argument&) {
    throw std::invalid_argument("No stop"s);
  }

}

std::vector<Bus*> TransportCatalogue::GetAllBuses() const {

  std::vector<Bus*> buses;

  for (const auto& bus : buses_) {
    Bus* bus_ptr = const_cast<Bus*>(&bus);
    buses.push_back(bus_ptr);
  }

  return buses;

}
std::vector<Stop*> TransportCatalogue::GetAllStops() const {

  std::vector<Stop*> stops;

  for (const auto& stop : stops_) {
    Stop* stop_ptr = const_cast<Stop*>(&stop);
    stops.push_back(stop_ptr);
  }

  return stops;

}


[[maybe_unused]] double TransportCatalogue::ComputeLenghtRoute(std::vector<Stop*>& route, const std::function<double(const Stop*, const Stop*)>& func) {
  double result = 0;
  for (auto it = route.begin(); it != route.end(); ++it) {
    if (std::next(it) == route.end())
      break;

    auto from = *it;
    auto to = *std::next(it);

    result += func(from, to);
  }

  return result;
}

const std::unordered_map<std::pair<const Stop *, const Stop *>, double, transport_catalogue::PairHasher> &
TransportCatalogue::GetStopToStopDistance() const {
    return stop_to_stop_distance;
}
