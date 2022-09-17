#include <fstream>
#include <iostream>
#include <string_view>
#include "serialization.h"
#include "json.h"
#include "json_reader.h"

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

   const std::string_view mode(argv[1]);
    transport_catalogue::TransportCatalogue db;
    RendererSettings render_set;
    RoutingSettings rout_set;
    transport_router::TransportRouter trans_rout(db);

    json::Document doc = json::Load(std::cin);
    const auto serialization_settings = doc.GetRoot().AsDict().at("serialization_settings"s);

    if (mode == "make_base"sv) {
        JsonHandler json_handler(db, render_set, rout_set, trans_rout);
        json_handler.ParseInputJson(doc);

        proto_serialize::Serialize(serialization_settings.AsDict().at("file"s).AsString(), proto_serialize::CreateSerializeDb(json_handler));

    } else if (mode == "process_requests"sv) {
        auto db_ser = proto_serialize::LoadDB(serialization_settings.AsDict().at("file"s).AsString());
        if(db_ser.has_value()) {
            JsonHandler json_handler(proto_serialize::Convert(*db_ser, db, render_set, rout_set, trans_rout));
            json_handler.OutputJson(doc, std::cout);

        }
    } else {
        PrintUsage();
        return 1;
    }
}