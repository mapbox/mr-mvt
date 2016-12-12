#include "map_to_features.hpp"

int main(int argc, char* argv[]) {
    std::string layer_name("layer");
    if (argc > 1) {
        layer_name = std::string(argv[1]);
    }
    mapbox::geojson::geojson<double> json = mapbox::mrmvt::geojson_std_in<double>();
    mapbox::geometry::feature_collection<double> fc = mapbox::mrmvt::geojson_to_fc<double>(std::move(json));
    mapbox::mrmvt::to_std_out(fc, layer_name);
    return 0;
}
