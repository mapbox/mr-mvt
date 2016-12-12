#pragma once

#include "tile_cover.hpp"
#include "clip.hpp"

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

namespace mapbox { namespace mrmvt {

inline void map_to_tile() {
    // don't skip the whitespace while reading
    std::cin >> std::noskipws;
    
    std::string feature_str;
    std::string layer_name;
    std::string zoom_level;
    while (std::getline(std::cin, zoom_level, ' ') && 
           std::getline(std::cin, layer_name, ' ') && 
           std::getline(std::cin, feature_str)) {
        auto feature = geojson::parse_feature<std::int64_t>(feature_str);
        auto tiles = tile_cover::get_tiles(feature.geometry, 4096);
        std::clog << "Tiles for zoom: " << zoom_level << std::endl;
        for (auto const& t : tiles) {
            std::clog << t.x << ", " << t.y << std::endl;
            auto og = clip(feature.geometry, t.x, t.y, 0);
            if (!og) {
                continue;
            }
            geometry::feature<std::int64_t> f { 
                std::move(*og), 
                feature.properties, 
                feature.id
            };
            std::cout << zoom_level << "," << t.x << "," << t.y << " " << layer_name << " " << mapbox::geojson::stringify<std::int64_t>(f) << std::endl;
        }
    }
}


}}
