#pragma once

#include "douglas_peucker.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas" // clang+gcc
#pragma GCC diagnostic ignored "-Wpragmas"         // gcc
#pragma GCC diagnostic ignored "-Wexpansion-to-defined"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#pragma GCC diagnostic pop

#include <mapbox/geojson.hpp>

#include <cmath>
#include <iostream>
#include <istream>

namespace mapbox {
namespace mrmvt {

static const double R2D = 180.0 / M_PI;
static const double MAX_LATITUDE = R2D * (2 * std::atan(std::exp(M_PI)) - (M_PI / 2.0));

struct to_tile_coord_visitor {
    double size;
    double simplify_distance;

    geometry::point<std::int64_t> convert(geometry::point<double> const& pt) {
        std::int64_t x = 0;
        std::int64_t y = 0;
        if (pt.x > 180.0) { 
            x = size - 1;
        } else if (pt.x < -180.0) { 
            x = 0;
        } else {
            x = std::round((pt.x + 180.0) * (size / 360.0));
        }
        if (pt.y > MAX_LATITUDE) {
            y = 0;
        } else if (pt.y < -MAX_LATITUDE) {
            y = size - 1;
        } else {
            y = size - std::round((std::log(std::tan((90.0 + pt.y) * (M_PI / 360.0))) + M_PI) * (size / (2.0 * M_PI)));
        }
        return geometry::point<std::int64_t>(x, y);
    }
    
    geometry::linear_ring<std::int64_t> convert(geometry::linear_ring<double> const& geom) {
        geometry::linear_ring<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& g : geom) {
            new_geom.push_back(convert(g));
        }
        if (new_geom.size() <= 4) {
            return new_geom;
        }
        geometry::linear_ring<std::int64_t> simplified;
        douglas_peucker<std::int64_t>(new_geom, std::back_inserter(simplified), simplify_distance);
        return simplified;
    }

    geometry::polygon<std::int64_t> convert(geometry::polygon<double> const& geom) {
        geometry::polygon<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& g : geom) {
            new_geom.push_back(convert(g));
        }
        return new_geom;
    }
    
    geometry::multi_polygon<std::int64_t> convert(geometry::multi_polygon<double> const& geom) {
        geometry::multi_polygon<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& g : geom) {
            new_geom.push_back(convert(g));
        }
        return new_geom;
    }

    geometry::line_string<std::int64_t> convert(geometry::line_string<double> const& geom) {
        geometry::line_string<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& g : geom) {
            new_geom.push_back(convert(g));
        }
        if (new_geom.size() <= 4) {
            return new_geom;
        }
        geometry::line_string<std::int64_t> simplified;
        douglas_peucker<std::int64_t>(new_geom, std::back_inserter(simplified), simplify_distance);
        return simplified;
    }

    geometry::multi_line_string<std::int64_t> convert(geometry::multi_line_string<double> const& geom) {
        geometry::multi_line_string<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& g : geom) {
            new_geom.push_back(convert(g));
        }
        return new_geom;
    }

    geometry::multi_point<std::int64_t> convert(geometry::multi_point<double> const& geom) {
        geometry::multi_point<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& g : geom) {
            new_geom.push_back(convert(g));
        }
        return new_geom;
    }

    geometry::geometry_collection<std::int64_t> convert(geometry::geometry_collection<double> const& geom) {
        geometry::geometry_collection<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& g : geom) {
            new_geom.push_back(geometry::geometry<double>::visit(g, (*this)));
        }
        return new_geom;
    }

    template <typename T>
    geometry::geometry<std::int64_t> operator() (T const& g) {
        return convert(g);
    }
};

inline geometry::geometry<std::int64_t> geom_to_zoom(geometry::geometry<double> g, 
                                                     std::size_t z,
                                                     std::size_t extent,
                                                     double simplify_distance) {
    double size = extent * std::pow(2, z);
    return geometry::geometry<double>::visit(g, to_tile_coord_visitor { size, simplify_distance });
}

inline void map_feature_to_zoom(std::string const& layer_name,
                                std::string const& feature_str,
                                std::size_t min_z,
                                std::size_t max_z,
                                std::size_t extent = 4096,
                                double simplify_distance = 4.0) {
    auto feature = geojson::parse_feature<double>(feature_str);
    for (auto z = min_z; z <= max_z; ++z) {
        geometry::feature<std::int64_t> f { 
            geom_to_zoom(feature.geometry, z, extent, simplify_distance), 
            feature.properties, 
            feature.id
        };
        std::cout << z << " " << layer_name << " " << mapbox::geojson::stringify<std::int64_t>(f) << std::endl;
    }
}

inline void map_to_zoom(std::size_t min_z, std::size_t max_z) {
    // don't skip the whitespace while reading
    std::cin >> std::noskipws;
    
    std::string feature_str;
    std::string layer_name;
    while (std::getline(std::cin, layer_name, ' ') && std::getline(std::cin, feature_str)) {
        map_feature_to_zoom(layer_name, feature_str, min_z, max_z);
    }
}

}}
