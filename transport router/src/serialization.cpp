#include "serialization.h"

void proto_serialize::Serialize(const std::filesystem::path& path,
               const transport_catalogue_serialize::TransportCatalogue& db) {
    std::ofstream out_file(path, std::ios::binary);
    db.SerializeToOstream(&out_file);
}

transport_catalogue_serialize::TransportCatalogue proto_serialize::CreateSerializeDb(JsonHandler &json_handler) {
    transport_catalogue_serialize::TransportCatalogue db_serialize;

    size_t stop_index = 0;
    const auto all_stop = json_handler.GetTransportCatalogue().GetAllStops();
    const auto all_bus = json_handler.GetTransportCatalogue().GetAllBuses();

    SerilizeStop(db_serialize, stop_index, all_stop);
    SerializeStopsDistance(json_handler.GetTransportCatalogue(), db_serialize, all_stop);
    SerializeBus(json_handler.GetTransportCatalogue(), db_serialize, all_stop);
    SerializeRoutingSettings(json_handler, db_serialize);
    SerializeRenderSettings(json_handler, db_serialize);
    SerializeTransportRouter(json_handler, db_serialize, all_stop, all_bus);

    return db_serialize;
}

void proto_serialize::SerializeTransportRouter(const JsonHandler &json_handler,
                                               transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                               const std::vector<transport_catalogue::Stop *> &all_stop, const std::vector<transport_catalogue::Bus *> &all_bus) {
    db_serialize.mutable_transport_router()->set_vertexes_counter(json_handler.GetTransportRouter().getVertexesCounter());

    db_serialize.mutable_transport_router()->mutable_vertexes()->Reserve(json_handler.GetTransportRouter().getVertexes().size());

    for (const auto& [key, value] : json_handler.GetTransportRouter().getVertexes()) {
        auto ptr = db_serialize.mutable_transport_router()->mutable_vertexes()->Add();

        ptr->set_stop_id(GetStopId(all_stop, key));
        ptr->set_in(value.in);
        ptr->set_out(value.out);
    }

    db_serialize.mutable_transport_router()->mutable_bus_edges()->Reserve(json_handler.GetTransportRouter().getBusEdges().size());


    for (const auto& [key, value] : json_handler.GetTransportRouter().getBusEdges()) {
        auto ptr = db_serialize.mutable_transport_router()->mutable_bus_edges()->Add();

        ptr->set_edge_id(key);
        ptr->set_bus_id(GetBusId(all_bus, value.bus_name));
        ptr->set_span_count(value.span_count);
        ptr->set_time(value.time);
    }

    db_serialize.mutable_transport_router()->mutable_wait_edges()->Reserve(json_handler.GetTransportRouter().getWaitEdges().size());

    for (const auto& [key, value] : json_handler.GetTransportRouter().getWaitEdges()) {
        auto ptr = db_serialize.mutable_transport_router()->mutable_wait_edges()->Add();

        ptr->set_edgeid(key);
        ptr->set_stop_id(GetStopId(all_stop, value.stop_name));
        ptr->set_time(value.time);
    }

    for (const auto& edge : json_handler.GetTransportRouter().GetGraph().GetEdges()) {
        auto ptr = db_serialize.mutable_transport_router()->mutable_graph()->add_edge();
        ptr->set_from(edge.from);
        ptr->set_to(edge.to);
        ptr->set_weight(edge.weight);
    }

    for (const auto& vector_incidence_list : json_handler.GetTransportRouter().GetGraph().GetIncidenceLists()) {
        auto ptr = db_serialize.mutable_transport_router()->mutable_graph()->add_incidence_list();

        for (const auto& incidence_list : vector_incidence_list) {
            ptr->add_edgeid(incidence_list);
        }
    }


    for (const auto& vector_internal_data : json_handler.GetTransportRouter().GetRouter()->GetRoutesInternalData()) {

        auto vector_internal_data_ptr = db_serialize.mutable_transport_router()->mutable_router()->add_vector_route_internal_data();

        for (const auto& internal_data : vector_internal_data) {
            auto ptr = vector_internal_data_ptr->add_internal_data();

            if (internal_data.has_value()) {
                ptr->mutable_value()->set_weight(internal_data->weight);

                if (internal_data->prev_edge.has_value()) {
                    ptr->mutable_value()->mutable_prev_edge()->set_value(internal_data->prev_edge.value());
                } else {
                    ptr->mutable_value()->mutable_prev_edge()->set_empty(true);
                }
            } else {
                ptr->set_empty(true);
            }
        }
    }
}

void proto_serialize::SerializeRenderSettings(const JsonHandler &json_handler,
                                              transport_catalogue_serialize::TransportCatalogue &db_serialize) {
    db_serialize.mutable_render_settings()->set_width(json_handler.GetRenderSettings().width);
    db_serialize.mutable_render_settings()->set_height(json_handler.GetRenderSettings().height);
    db_serialize.mutable_render_settings()->set_padding(json_handler.GetRenderSettings().padding);
    db_serialize.mutable_render_settings()->set_stop_radius(json_handler.GetRenderSettings().stop_radius);
    db_serialize.mutable_render_settings()->set_line_width(json_handler.GetRenderSettings().line_width);
    db_serialize.mutable_render_settings()->mutable_bus_label()->set_font_size(
            json_handler.GetRenderSettings().bus_label_font_size);
    db_serialize.mutable_render_settings()->mutable_bus_label()->mutable_offset()->set_lat(
            json_handler.GetRenderSettings().bus_label_offset.lat);
    db_serialize.mutable_render_settings()->mutable_bus_label()->mutable_offset()->set_lng(
            json_handler.GetRenderSettings().bus_label_offset.lng);
    db_serialize.mutable_render_settings()->mutable_stop_label()->set_font_size(
            json_handler.GetRenderSettings().stop_label_font_size);
    db_serialize.mutable_render_settings()->mutable_stop_label()->mutable_offset()->set_lat(
            json_handler.GetRenderSettings().stop_label_offset.lat);
    db_serialize.mutable_render_settings()->mutable_stop_label()->mutable_offset()->set_lng(
            json_handler.GetRenderSettings().stop_label_offset.lng);
    db_serialize.mutable_render_settings()->set_underlayer_width(json_handler.GetRenderSettings().underlayer_width);

    SerializeUnderLayerColor(json_handler, db_serialize);

    SerializePallete(json_handler, db_serialize);
}

void proto_serialize::SerializePallete(const JsonHandler &json_handler,
                                       transport_catalogue_serialize::TransportCatalogue &db_serialize) {
    for (const auto &color: json_handler.GetRenderSettings().color_pallete) {
        auto color_ser = db_serialize.mutable_render_settings()->add_color_palette();

        if (std::holds_alternative<std::string>(color)) {
            color_ser->set_color_stirng(std::get<std::string>(color));
        } else if (std::holds_alternative<svg::Rgba>(color)) {
            color_ser->mutable_color_rgba()->set_r(std::get<svg::Rgba>(color).red);
            color_ser->mutable_color_rgba()->set_g(std::get<svg::Rgba>(color).green);
            color_ser->mutable_color_rgba()->set_b(std::get<svg::Rgba>(color).blue);
            color_ser->mutable_color_rgba()->set_oppacity(std::get<svg::Rgba>(color).opacity);
        } else {
            color_ser->mutable_color_rgb()->set_r(std::get<svg::Rgb>(color).red);
            color_ser->mutable_color_rgb()->set_g(std::get<svg::Rgb>(color).green);
            color_ser->mutable_color_rgb()->set_b(std::get<svg::Rgb>(color).blue);
        }
    }
}

void proto_serialize::SerializeUnderLayerColor(const JsonHandler &json_handler,
                                               transport_catalogue_serialize::TransportCatalogue &db_serialize) {
    if (std::holds_alternative<std::string>(json_handler.GetRenderSettings().underlayer_color)) {
        db_serialize.mutable_render_settings()->mutable_underlayer_color()->set_color_stirng(
                std::get<std::string>(json_handler.GetRenderSettings().underlayer_color));
    } else if (std::holds_alternative<svg::Rgba>(json_handler.GetRenderSettings().underlayer_color)) {
        db_serialize.mutable_render_settings()->mutable_underlayer_color()->mutable_color_rgba()->set_r(
                std::get<svg::Rgba>(json_handler.GetRenderSettings().underlayer_color).red);
        db_serialize.mutable_render_settings()->mutable_underlayer_color()->mutable_color_rgba()->set_g(
                std::get<svg::Rgba>(json_handler.GetRenderSettings().underlayer_color).green);
        db_serialize.mutable_render_settings()->mutable_underlayer_color()->mutable_color_rgba()->set_b(
                std::get<svg::Rgba>(json_handler.GetRenderSettings().underlayer_color).blue);
        db_serialize.mutable_render_settings()->mutable_underlayer_color()->mutable_color_rgba()->set_oppacity(
                std::get<svg::Rgba>(json_handler.GetRenderSettings().underlayer_color).opacity);
    } else {
        db_serialize.mutable_render_settings()->mutable_underlayer_color()->mutable_color_rgb()->set_r(
                std::get<svg::Rgb>(json_handler.GetRenderSettings().underlayer_color).red);
        db_serialize.mutable_render_settings()->mutable_underlayer_color()->mutable_color_rgb()->set_g(
                std::get<svg::Rgb>(json_handler.GetRenderSettings().underlayer_color).green);
        db_serialize.mutable_render_settings()->mutable_underlayer_color()->mutable_color_rgb()->set_b(
                std::get<svg::Rgb>(json_handler.GetRenderSettings().underlayer_color).blue);
    }
}

void proto_serialize::SerializeRoutingSettings(const JsonHandler &json_handler,
                                               transport_catalogue_serialize::TransportCatalogue &db_serialize) {
    db_serialize.mutable_routing_settings()->set_bus_velocity(json_handler.GetRoutingSettings().velocity);
    db_serialize.mutable_routing_settings()->set_bus_wait_time(json_handler.GetRoutingSettings().wait_time);
}


void proto_serialize::SerializeBus(const transport_catalogue::TransportCatalogue &db,
                                   transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                   const std::vector<transport_catalogue::Stop *> &all_stop) {
    for (const auto bus : db.GetAllBuses()) {
        transport_catalogue_serialize::Bus* bus_ptr = db_serialize.add_bus();

        bus_ptr->set_name(bus->name);

        bool is_round_trip = (bus->last_stop == bus->route.front());
        bus_ptr->set_is_round_trip(is_round_trip);

        for (const transport_catalogue::Stop* stop : bus->raw_route) {
            bus_ptr->add_route(GetStopId(all_stop, stop));
        }
    }
}

void proto_serialize::SerializeStopsDistance(const transport_catalogue::TransportCatalogue &db,
                                             transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                             const std::vector<transport_catalogue::Stop *> &all_stop) {
    for (const auto& [stops, distance] : db.GetStopToStopDistance()) {
        transport_catalogue_serialize::StopToStopDistance* stop_to_stop_ptr = db_serialize.add_distance();
        stop_to_stop_ptr->set_from(GetStopId(all_stop, stops.first));
        stop_to_stop_ptr->set_to(GetStopId(all_stop, stops.second));
        stop_to_stop_ptr->set_distance(distance);
    }
}

void proto_serialize::SerilizeStop(transport_catalogue_serialize::TransportCatalogue &db_serialize, long stop_index,
                                   const std::vector<transport_catalogue::Stop *> &all_stop) {
    for (const auto& stop : all_stop) {
        transport_catalogue_serialize::Stop* ptr = db_serialize.add_stop();

        ptr->set_name(stop->name);
        ptr->mutable_coordinates()->set_lng(stop->coordinates.lng);
        ptr->mutable_coordinates()->set_lat(stop->coordinates.lat);
        ptr->set_stop_id(stop_index);

        ++stop_index;
    }
}

size_t proto_serialize::GetStopId(const std::vector<transport_catalogue::Stop*> &all_stop, const transport_catalogue::Stop *stop) {
    size_t stop_id = std::distance(all_stop.begin(),
                        std::find_if(all_stop.begin(), all_stop.end(),
                                             [stop](const transport_catalogue::Stop* lhs){
                                    return lhs->name == stop->name;})
    );
    return stop_id;
}

size_t proto_serialize::GetStopId(const std::vector<transport_catalogue::Stop*> &all_stop, const std::string_view stop_name) {
    size_t stop_id = std::distance(all_stop.begin(),
                                 std::find_if(all_stop.begin(), all_stop.end(),
                                              [&stop_name](const transport_catalogue::Stop* lhs){
                                                  return lhs->name == stop_name;})
    );
    return stop_id;
}

size_t proto_serialize::GetBusId(const std::vector<transport_catalogue::Bus*> &all_buses, const std::string_view bus_name) {
    size_t bus_id = std::distance(all_buses.begin(),
                                 std::find_if(all_buses.begin(), all_buses.end(),
                                              [&bus_name](const transport_catalogue::Bus* lhs){
                                                  return lhs->name == bus_name;})
    );
    return bus_id;
}

std::optional<transport_catalogue_serialize::TransportCatalogue> proto_serialize::LoadDB(const std::filesystem::path& path) {
    std::ifstream in_file(path, std::ios::binary);
    transport_catalogue_serialize::TransportCatalogue db_deserialize;

    if (!db_deserialize.ParseFromIstream(&in_file)) {
        return std::nullopt;
    }

    // тут нужен move, поскольку возвращается другой тип
    return {std::move(db_deserialize)};
}

// return json_handler
JsonHandler
proto_serialize::Convert(const transport_catalogue_serialize::TransportCatalogue &db_serialize, transport_catalogue::TransportCatalogue& db,
                         RendererSettings& render_set, RoutingSettings& rout_set, transport_router::TransportRouter& trans_rout) {
    std::vector<std::string_view> all_stop_raw;

    all_stop_raw.reserve(db_serialize.stop().size());

    for (const auto& stop : db_serialize.stop()) {
        all_stop_raw.push_back(stop.name());
    }

    DeserealizeStops(db_serialize, db, all_stop_raw);

    DeserealizeStopToStopDistance(db_serialize, db, all_stop_raw);

    DeserlizeBus(db_serialize, db, all_stop_raw);

    const auto all_stop = db.GetAllStops();
    const auto all_bus = db.GetAllBuses();

    DeserealizeRoutSettings(db_serialize, rout_set);

    DeserealizeRenderSettings(db_serialize, render_set);

    trans_rout.SetSettings(rout_set);
    trans_rout.SetRouter(db_serialize, all_stop, all_bus);



    return {db, render_set, rout_set, trans_rout};
}

void proto_serialize::DeserealizeStops(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                       transport_catalogue::TransportCatalogue &db,
                                       std::vector<std::string_view> &all_stops) {
    for (const auto& stop_ser : db_serialize.stop()) {
        db.AddStop(const_cast<std::string &>(stop_ser.name()), stop_ser.coordinates().lat(), stop_ser.coordinates().lng());
    }
}

void
proto_serialize::DeserealizeStopToStopDistance(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                               transport_catalogue::TransportCatalogue &db,
                                               const std::vector<std::string_view> &all_stops) {
    for (const auto& stops_distance : db_serialize.distance()) {
        auto* first_stop = const_cast<transport_catalogue::Stop*>(db.FindStop(std::string(all_stops[stops_distance.from()])));
        auto* second_stop = const_cast<transport_catalogue::Stop*>(db.FindStop(std::string(all_stops[stops_distance.to()])));

        db.SetDistance(first_stop, second_stop, stops_distance.distance());
    }
}

void proto_serialize::DeserlizeBus(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                   transport_catalogue::TransportCatalogue &db,
                                   const std::vector<std::string_view> &all_stops) {
    for (const auto& bus : db_serialize.bus()) {
        std::vector<transport_catalogue::Stop*> route;
        route.reserve(bus.route().size());


        for (const auto& stop_id : bus.route()) {
            route.push_back(const_cast<transport_catalogue::Stop*>(db.FindStop(std::string(all_stops[stop_id]))));
        }

        db.AddBus(const_cast<std::string &>(bus.name()), route, bus.is_round_trip());
    }
}

void proto_serialize::DeserealizeRenderSettings(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                                RendererSettings &render_set) {
    render_set.width = db_serialize.render_settings().width();
    render_set.height = db_serialize.render_settings().height();
    render_set.padding = db_serialize.render_settings().padding();
    render_set.stop_radius = db_serialize.render_settings().stop_radius();
    render_set.line_width = db_serialize.render_settings().line_width();
    render_set.bus_label_font_size = db_serialize.render_settings().bus_label().font_size();
    render_set.bus_label_offset = {db_serialize.render_settings().bus_label().offset().lat(), db_serialize.render_settings().bus_label().offset().lng()};
    render_set.stop_label_font_size = db_serialize.render_settings().stop_label().font_size();
    render_set.stop_label_offset = {db_serialize.render_settings().stop_label().offset().lat(), db_serialize.render_settings().stop_label().offset().lng()};
    render_set.underlayer_width = db_serialize.render_settings().underlayer_width();

    svg::Color color_under;

    if(db_serialize.render_settings().underlayer_color().has_color_rgb()) {
        color_under = {svg::Rgb(db_serialize.render_settings().underlayer_color().color_rgb().r(),
                                  db_serialize.render_settings().underlayer_color().color_rgb().g(),
                                  db_serialize.render_settings().underlayer_color().color_rgb().b())};
    } else if(db_serialize.render_settings().underlayer_color().has_color_rgba()) {
        color_under = {svg::Rgba(db_serialize.render_settings().underlayer_color().color_rgba().r(),
                          db_serialize.render_settings().underlayer_color().color_rgba().g(),
                          db_serialize.render_settings().underlayer_color().color_rgba().b(),
                           db_serialize.render_settings().underlayer_color().color_rgba().oppacity())};
    } else {
        color_under = {db_serialize.render_settings().underlayer_color().color_stirng()};
    }

    render_set.underlayer_color = color_under;

    for (const auto& color_ser : db_serialize.render_settings().color_palette()) {
        svg::Color color;

        if(color_ser.has_color_rgb()) {
            color = {svg::Rgb(color_ser.color_rgb().r(),
                              color_ser.color_rgb().g(),
                              color_ser.color_rgb().b())};
        } else if(color_ser.has_color_rgba()) {
            color = {svg::Rgba(color_ser.color_rgba().r(),
                               color_ser.color_rgba().g(),
                               color_ser.color_rgba().b(),
                               color_ser.color_rgba().oppacity())};
        } else {
            color = {color_ser.color_stirng()};
        }

        render_set.color_pallete.push_back(color);
    }
}

void proto_serialize::DeserealizeRoutSettings(const transport_catalogue_serialize::TransportCatalogue &db_serialize,
                                              RoutingSettings &rout_set) {
    rout_set.wait_time = db_serialize.routing_settings().bus_wait_time();
    rout_set.velocity = db_serialize.routing_settings().bus_velocity();
}
