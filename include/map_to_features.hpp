#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas" // clang+gcc
#pragma GCC diagnostic ignored "-Wpragmas"         // gcc
#pragma GCC diagnostic ignored "-Wexpansion-to-defined"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#pragma GCC diagnostic pop

#include <mapbox/geojson.hpp>

#include <iostream>
#include <istream>

namespace mapbox {
namespace mrmvt {

template <typename T>
mapbox::geojson::geojson<T> geojson_std_in() {
    // don't skip the whitespace while reading
    std::cin >> std::noskipws;

    // use stream iterators to copy the stream to a string
    std::istream_iterator<char> it(std::cin);
    std::istream_iterator<char> end;
    std::string buffer(it, end);
    return mapbox::geojson::parse<T>(buffer);
}

template <typename T>
struct to_feature_collection {

    mapbox::geometry::feature_collection<T> operator()(mapbox::geometry::geometry<T> g) {
        mapbox::geometry::feature_collection<T> fc;
        fc.push_back(mapbox::geometry::feature<T>{ g });
        return fc;
    }
    
    mapbox::geometry::feature_collection<T> operator()(mapbox::geometry::feature<T> f) {
        mapbox::geometry::feature_collection<T> fc;
        fc.push_back(f);
        return fc;
    }
    
    mapbox::geometry::feature_collection<T> operator()(mapbox::geometry::feature_collection<T> fc) {
        return fc;
    }
};

template <typename T>
mapbox::geometry::feature_collection<T> geojson_to_fc(mapbox::geojson::geojson<T> json) {
    return mapbox::geojson::geojson<T>::visit(json, to_feature_collection<T>());
}

template <typename T>
void to_std_out(mapbox::geometry::feature<T> const& f, std::string const& layer_name) {
    std::cout << layer_name << " " << mapbox::geojson::stringify<T>(f) << std::endl;
}

template <typename T>
void to_std_out(mapbox::geometry::feature_collection<T> const& fc, std::string const& layer_name) {
    for (auto const& f : fc) {
        to_std_out<T>(f, layer_name);
    }
}

}}
