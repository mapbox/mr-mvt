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
    std::int64_t buffer = 6;
    geometry::polygon<std::int64_t> fill_geometry;
    geometry::linear_ring<std::int64_t> fill_ring;
    fill_ring.push_back({-buffer, 4095+buffer});
    fill_ring.push_back({-buffer, -buffer});
    fill_ring.push_back({4095+buffer, -buffer});
    fill_ring.push_back({4095+buffer, 4095+buffer});
    fill_ring.push_back({-buffer, 4095+buffer});
    fill_geometry.push_back(fill_ring);
    
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
        for (auto const& t : tiles) {
            if (t.fill) {
                geometry::feature<std::int64_t> f { 
                    fill_geometry, 
                    feature.properties, 
                    feature.id
                };
                std::cout << zoom_level << "," << t.x << "," << t.y << " " << layer_name << " " << mapbox::geojson::stringify<std::int64_t>(f) << std::endl;
            } else {
                auto og = clip(feature.geometry, t.x, t.y, buffer);
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
}


}}
