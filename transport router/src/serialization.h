#include <fstream>
#include <optional>
#include <algorithm>
#include <iterator>
#include <filesystem>


#include "transport_catalogue.h"
#include "transport_catalogue.pb.h"
#include "json_reader.h"

namespace proto_serialize {
    void Serialize(const std::filesystem::path& path,
                   const transport_catalogue_serialize::TransportCatalogue& db);
    transport_catalogue_serialize::TransportCatalogue CreateSerializeDb(JsonHandler &json_handler);
    std::optional<transport_catalogue_serialize::TransportCatalogue> LoadDB(const std::filesystem::path& path);
    size_t GetStopId(const std::vector<transport_catalogue::Stop*> &all_stop, const transport_catalogue::Stop *stop);
    size_t GetStopId(const std::vector<transport_catalogue::Stop*> &all_stop, const std::string_view stop_name);
    size_t GetBusId(const std::vector<transport_catalogue::Bus*> &all_buses, const std::string_view bus_name);

        JsonHandler Convert(const transport_catalogue_serialize::TransportCatalogue &db_serialize, transport_catalogue::TransportCatalogue& db,
                            RendererSettings& render_set, RoutingSettings& rout_set, transport_router::TransportRouter& trans_rout);

    void SerilizeStop(transport_catalogue_serialize::TransportCatalogue &db_serialize, long stop_index,
                      const std::vector<transport_catalogue::Stop *> &all_stop);

    void SerializeStopsDistance(const transport_catalogue::TransportCatalogue &db,
                                transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                const std::vector<transport_catalogue::Stop *> &all_stop);

    void SerializeBus(const transport_catalogue::TransportCatalogue &db,
                      transport_catalogue_serialize::TransportCatalogue &db_serialize,
                      const std::vector<transport_catalogue::Stop *> &all_stop);

    void DeserealizeRoutSettings(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                 RoutingSettings &rout_set);

    void DeserealizeRenderSettings(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                   RendererSettings &render_set);

    void DeserlizeBus(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                      transport_catalogue::TransportCatalogue &db, const std::vector<std::string_view> &all_stops);

    void DeserealizeStopToStopDistance(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                       transport_catalogue::TransportCatalogue &db,
                                       const std::vector<std::string_view> &all_stops);

    void DeserealizeStops(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                          transport_catalogue::TransportCatalogue &db, std::vector<std::string_view> &all_stops);

    void SerializeRoutingSettings(const JsonHandler &json_handler,
                                  transport_catalogue_serialize::TransportCatalogue &db_serialize);

    void SerializeRenderSettings(const JsonHandler &json_handler,
                                 transport_catalogue_serialize::TransportCatalogue &db_serialize);

    void SerializeUnderLayerColor(const JsonHandler &json_handler,
                                  transport_catalogue_serialize::TransportCatalogue &db_serialize);

    void
    SerializePallete(const JsonHandler &json_handler, transport_catalogue_serialize::TransportCatalogue &db_serialize);

    void SerializeTransportRouter(const JsonHandler &json_handler,
                                  transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                  const std::vector<transport_catalogue::Stop *> &all_stop, const std::vector<transport_catalogue::Bus *> &all_bus);
} // namespace proto_serialize




