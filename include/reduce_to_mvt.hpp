#pragma once

#include "output_mbtiles.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas" // clang+gcc
#pragma GCC diagnostic ignored "-Wpragmas"         // gcc
#pragma GCC diagnostic ignored "-Wexpansion-to-defined"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#pragma GCC diagnostic pop

#include <mapbox/geometry.hpp>
#include <mapbox/geojson.hpp>
#include <mapbox/vector_tile/encode_layer.hpp>

#include <cmath>
#include <iostream>
#include <istream>
#include <sstream>
#include <stdexcept>

namespace mapbox {
namespace mrmvt {

struct value_type_visitor {
    template <typename T>
    json_field_type operator() (T const &) {
        return json_field_type_number;
    }

    json_field_type operator() (bool const&) { 
        return json_field_type_boolean;
    }

    json_field_type operator() (std::string const&) {
        return json_field_type_string;
    }
};

inline void add_to_fields(std::map<std::string, json_field_type> & fields,
                          geometry::property_map const& prop_map) {
    for (auto const& pm : prop_map) {
        auto f = fields.find(pm.first);
        if (f == fields.end()) {
            fields.emplace(pm.first, geometry::value::visit(pm.second, value_type_visitor()));
        }
    }
}

inline void add_to_layer_map(layer_map_type & layer_map, 
                             std::string const & layer_name,
                             int z,
                             geometry::feature<std::int64_t> const& f) {
    auto lm = layer_map.find(layer_name);
    if (layer_map.end() == lm) {
        layer_meta_data lmd = { z, z, std::map<std::string, json_field_type>() };
        add_to_fields(lmd.fields, f.properties);
        layer_map.emplace(layer_name, lmd);
    } else {
        add_to_fields(lm->second.fields, f.properties);
        if (lm->second.min_zoom > z) {
            lm->second.min_zoom = z;
        }
        if (lm->second.max_zoom < z) {
            lm->second.max_zoom = z;
        }
    }
}

inline void encode_tile_feature(layer_map_type & layer_map,
                                std::string const& layer_name,
                                int z,
                                std::string const& feature_str,
                                geometry::feature_collection<std::int64_t> & features) {
    auto feature = geojson::parse_feature<std::int64_t>(feature_str);
    add_to_layer_map(layer_map, layer_name, z, feature);
    features.push_back(feature);
}

inline void encode_tile_layer(std::string & buffer,
                              std::string const& layer_name,
                              geometry::feature_collection<std::int64_t> & features) {
    if (!features.empty()) {
        mapbox::vector_tile::encode_layer(buffer, layer_name, features);
    }
    features.clear();
}

inline void encode_vector_tile(sqlite_db const& db,
                               std::string & buffer,
                               int z,
                               int x, 
                               int y) {
    if (!buffer.empty()) {
        mbtiles_write_tile(db, z, x, y, buffer.c_str(), buffer.size());
    }
    buffer.clear();
}

inline void set_z_x_y(std::string const& zxy_str, int & z, int & x, int & y) {
    std::stringstream ss(zxy_str);
    if (!(ss >> z)) {
        throw std::runtime_error("Invalid z in z,x,y");
    }
    if (ss.peek() == ',') {
        ss.ignore();
    }
    if (!(ss >> x)) {
        std::cerr << zxy_str << std::endl;
        throw std::runtime_error("Invalid x in z,x,y");
    }
    if (ss.peek() == ',') {
        ss.ignore();
    }
    if (!(ss >> y)) {
        throw std::runtime_error("Invalid y in z,x,y");
    }
}

inline void find_min_max_zoom(layer_map_type const& layer_map,
                              int & min_zoom,
                              int & max_zoom) {
    for (auto const& lm : layer_map) {
        if (lm.second.min_zoom < min_zoom) {
            min_zoom = lm.second.min_zoom;
        }
        if (lm.second.max_zoom > max_zoom) {
            max_zoom = lm.second.max_zoom;
        }
    }
}

inline void reduce_to_mvt(std::string const& db_name) {
    // don't skip the whitespace while reading
    std::cin >> std::noskipws;
    layer_map_type layer_map;
    auto db = mbtiles_open(db_name);
    int z = 0;
    int x = 0;
    int y = 0;
    std::string feature_str;
    std::string layer_name;
    std::string current_layer_name;
    std::string zxy_str;
    std::string current_zxy;
    std::string buffer;
    geometry::feature_collection<std::int64_t> features;
    while (std::getline(std::cin, zxy_str, ' ') && 
           std::getline(std::cin, layer_name, ' ') && 
           std::getline(std::cin, feature_str)) {
        if (zxy_str != current_zxy) {
            encode_tile_layer(buffer, current_layer_name, features);
            current_layer_name = layer_name;
            encode_vector_tile(db, buffer, z, x, y);
            current_zxy = zxy_str;
            set_z_x_y(zxy_str, z, x, y);
        } else if (current_layer_name != layer_name) {
            encode_tile_layer(buffer, current_layer_name, features);
            current_layer_name = layer_name;
        }
        encode_tile_feature(layer_map, current_layer_name, z, feature_str, features);
    }
    encode_tile_layer(buffer, current_layer_name, features);
    encode_vector_tile(db, buffer, z, x, y);
    int min_zoom = std::numeric_limits<int>::max();
    int max_zoom = std::numeric_limits<int>::min();
    find_min_max_zoom(layer_map, min_zoom, max_zoom);
    mbtiles_write_metadata(db, db_name, min_zoom, max_zoom, layer_map);
    mbtiles_close(db);
}

}}
