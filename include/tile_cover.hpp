#pragma once

#include <mapbox/geometry/geometry.hpp>

#include <cmath>
#include <limits>
#include <vector>

#ifdef DEBUG
#include <iostream>
#endif

namespace mapbox { namespace tile_cover {

#ifdef DEBUG
template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const geometry::linear_ring<T>& ring) {
    out << "[";
    bool first = true;
    for (auto const& pt: ring) {
        if (first) {
            out << "[";
            first = false;
        } else {
            out << ",[";
        }
        out << pt.x << "," << pt.y << "]";
    }
    out << "]";
    return out;
}

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const geometry::polygon<T>& poly) {
    out << "[";
    bool first = true;
    for (auto const& ring: poly) {
        if (first) {
            first = false;
        } else {
            out << ",";
        }
        out << ring;
    }
    out << "]";
    return out;
}

#endif

struct tile_coordinate {
	std::uint32_t x;
	std::uint32_t y;
    bool fill;
};

inline bool operator== (tile_coordinate const& a, tile_coordinate const& b) {
    return a.x == b.x && a.y == b.y;
}

inline bool operator< (tile_coordinate const& a, tile_coordinate const& b) {
    if (a.y == b.y) {
        if (a.x == b.x) {
            return a.fill < b.fill;
        } else {
            return a.x < b.x;
        }
    } else {
        return a.y < b.y;
    }
}

using tile_coordinates = std::vector<tile_coordinate>;

inline tile_coordinate point_to_tile(geometry::point<std::int64_t> const& pt, 
                                     std::uint32_t extent) {
    return tile_coordinate { static_cast<std::uint32_t>(pt.x / extent), static_cast<std::uint32_t>(pt.y / extent), false };
}

inline bool almost_zero(double val) {
    return std::fabs(val) < 1e-12;
}

inline void line_cover(tile_coordinates & tiles,
                       std::uint32_t extent, 
                       mapbox::geometry::line_string<std::int64_t> const& line) {
    auto itr = line.begin();
    double x0 = (static_cast<double>(itr->x) / static_cast<double>(extent));
    double x1 = 0.0;
    std::int64_t i_x0 = static_cast<std::int64_t>(x0);
    std::int64_t i_x1 = 0;
    double y0 = (static_cast<double>(itr->y) / static_cast<double>(extent));
    double y1 = 0.0;
    std::int64_t i_y0 = static_cast<std::int64_t>(y0);
    std::int64_t i_y1 = 0;
    for (++itr; itr != line.end(); ++itr) {
        x1 = x0;
        i_x1 = i_x0;
        y1 = y0;
        i_y1 = i_y0;
        x0 = (static_cast<double>(itr->x) / static_cast<double>(extent));
        i_x0 = static_cast<std::int64_t>(x0);
        y0 = (static_cast<double>(itr->y) / static_cast<double>(extent));
        i_y0 = static_cast<std::int64_t>(y0);
        double dx = x1 - x0;
        double dy = y1 - y0;
        std::int64_t i_dx = i_x1 - i_x0;
        std::int64_t i_dy = i_y1 - i_y0;
        if (i_dx == 0 && i_dy == 0) {
            auto const& back = tiles.back();
            if (!tiles.empty() || back.x != i_x0 || back.y != i_y0) {
                tiles.push_back(tile_coordinate { 
                        static_cast<std::uint32_t>(i_x0), 
                        static_cast<std::uint32_t>(i_y0),
                        false });
            }
            continue;
        }
        double sx = dx > 0.0 ? 1.0 : -1.0;
        double sy = dy > 0.0 ? 1.0 : -1.0;
        double t_max_x = almost_zero(dx) ? std::numeric_limits<double>::infinity() : std::fabs(((dx > 0.0 ? 1.0 : 0.0) + static_cast<double>(i_x0) - x0) / dx);
        double t_max_y = almost_zero(dy) ? std::numeric_limits<double>::infinity() : std::fabs(((dy > 0 ? 1 : 0) + static_cast<double>(i_y0) - y0) / dy);
        double tdx = std::fabs(sx / dx);
        double tdy = std::fabs(sy / dy);
        
        auto const& back = tiles.back();
        if (back.x != i_x0 || back.y != i_y0) {
            tiles.push_back(tile_coordinate { 
                    static_cast<std::uint32_t>(i_x0), 
                    static_cast<std::uint32_t>(i_y0),
                    false });
        }
        std::int64_t x = i_x0;
        std::int64_t y = i_y0;
        while (t_max_x < 1.0 || t_max_y < 1.0) {
            if (t_max_x < t_max_y) {
                t_max_x += tdx;
                x += sx;
            } else {
                t_max_y += tdy;
                y += sy;
            }
            tiles.push_back(tile_coordinate { 
                    static_cast<std::uint32_t>(x), 
                    static_cast<std::uint32_t>(y),
                    false });
        }
    }
}

inline void ring_cover(tile_coordinates & partial_ring,
                       tile_coordinates & all_tiles,
                       std::uint32_t extent, 
                       mapbox::geometry::linear_ring<std::int64_t> const& ring) {
    auto itr = ring.begin();
    double x1 = (static_cast<double>(itr->x) / static_cast<double>(extent));
    double x0 = 0.0;
    std::int64_t i_x1 = static_cast<std::int64_t>(x1);
    std::int64_t i_x0 = 0;
    double y1 = (static_cast<double>(itr->y) / static_cast<double>(extent));
    double y0 = 0.0;
    std::int64_t i_y1 = static_cast<std::int64_t>(y1);
    std::int64_t i_y0 = 0;
    for (++itr; itr != ring.end(); ++itr) {
        x0 = x1;
        i_x0 = i_x1;
        y0 = y1;
        i_y0 = i_y1;
        x1 = (static_cast<double>(itr->x) / static_cast<double>(extent));
        i_x1 = static_cast<std::int64_t>(x1);
        y1 = (static_cast<double>(itr->y) / static_cast<double>(extent));
        i_y1 = static_cast<std::int64_t>(y1);
        double dx = x1 - x0;
        double dy = y1 - y0;
        std::int64_t i_dx = i_x1 - i_x0;
        std::int64_t i_dy = i_y1 - i_y0;
        if (i_dx == 0 && i_dy == 0) {
            if (partial_ring.empty()) {
                partial_ring.push_back(tile_coordinate { 
                        static_cast<std::uint32_t>(i_x0),
                        static_cast<std::uint32_t>(i_y0),
                        false });
            }
            if (all_tiles.empty()) {
                all_tiles.push_back(tile_coordinate { 
                        static_cast<std::uint32_t>(i_x0), 
                        static_cast<std::uint32_t>(i_y0),
                        false });
            }
            continue;
        }
        double sx = dx > 0.0 ? 1.0 : -1.0;
        double sy = dy > 0.0 ? 1.0 : -1.0;
        double t_max_x = almost_zero(dx) ? std::numeric_limits<double>::infinity() : std::fabs(((dx > 0.0 ? 1.0 : 0.0) + static_cast<double>(i_x0) - x0) / dx);
        double t_max_y = almost_zero(dy) ? std::numeric_limits<double>::infinity() : std::fabs(((dy > 0.0 ? 1.0 : 0.0) + static_cast<double>(i_y0) - y0) / dy);
        double tdx = std::fabs(sx / dx);
        double tdy = std::fabs(sy / dy);
        
        if (partial_ring.empty() || partial_ring.back().y != i_y0) {
            partial_ring.push_back(tile_coordinate { 
                    static_cast<std::uint32_t>(i_x0),
                    static_cast<std::uint32_t>(i_y0),
                    false });
        }
        if (all_tiles.empty() || all_tiles.back().x != i_x0 || all_tiles.back().y != i_y0) {
            all_tiles.push_back(tile_coordinate { 
                    static_cast<std::uint32_t>(i_x0), 
                    static_cast<std::uint32_t>(i_y0),
                    false });
        }

        std::int64_t x = i_x0;
        std::int64_t y = i_y0;
        while (t_max_x < 1.0 || t_max_y < 1.0) {
            if (t_max_x < t_max_y) {
                t_max_x += tdx;
                x += static_cast<std::int64_t>(sx);
            } else {
                t_max_y += tdy;
                y += static_cast<std::int64_t>(sy);
            }
            if (partial_ring.empty() || partial_ring.back().y != y) {
                partial_ring.push_back(tile_coordinate { 
                        static_cast<std::uint32_t>(x), 
                        static_cast<std::uint32_t>(y),
                        false });
            }
            if (all_tiles.empty() || all_tiles.back().x != x || all_tiles.back().y != y) {
                all_tiles.push_back(tile_coordinate { 
                        static_cast<std::uint32_t>(x), 
                        static_cast<std::uint32_t>(y),
                        false });
            }
        }
    }
    if (partial_ring.size() > 1 && partial_ring.front().y == partial_ring.back().y) {
        partial_ring.pop_back();
    }
}

inline void polygon_cover(tile_coordinates & tiles,
                          std::uint32_t extent, 
                          mapbox::geometry::polygon<std::int64_t> const& polygon) {
    tile_coordinates intersections;
    for (auto const& ring : polygon) {
        tile_coordinates partial_ring;
        tile_coordinates all_tiles;
        ring_cover(partial_ring, all_tiles, extent, ring);
        if (partial_ring.size() >= 3) {
            auto itr_1 = partial_ring.end();
            --itr_1;
            auto itr_2 = partial_ring.begin();
            auto itr_3 = std::next(itr_2);
            while (itr_2 != partial_ring.end()) {
                if ((itr_2->y > itr_1->y || itr_2->y > itr_3->y) && // not local minimum
                    (itr_2->y < itr_1->y || itr_2->y < itr_3->y) && // not local maximum
                    itr_2->y != itr_3->y) {
                    intersections.push_back(*itr_2);
                }
                ++itr_1;
                ++itr_2;
                ++itr_3;
                if (itr_1 == partial_ring.end()) {
                    itr_1 = partial_ring.begin();
                }
                if (itr_3 == partial_ring.end()) {
                    itr_3 = partial_ring.begin();
                }
            }
        }
        std::sort(all_tiles.begin(), all_tiles.end());
        all_tiles.erase(std::unique(all_tiles.begin(), all_tiles.end()), all_tiles.end());
        tiles.insert(tiles.end(), all_tiles.begin(), all_tiles.end());
    }

    std::sort(intersections.begin(), intersections.end());
    for (auto itr = intersections.begin(); itr != intersections.end(); ++itr) {
        auto y = itr->y;
        auto x = itr->x;
        ++itr;
        for (; x < itr->x; x++) {
            tiles.push_back(tile_coordinate{ x, y, true });
        }
    }
}

struct tile_cover_visitor {
    std::int64_t extent;
    tile_coordinates & tiles;

    void operator() (geometry::point<std::int64_t> const& pt) {
        tiles.push_back(point_to_tile(pt, extent));
    }

    void operator() (geometry::multi_point<std::int64_t> const& mp) {
        for (auto const& pt : mp) {
            tiles.push_back(point_to_tile(pt, extent));
        }
    }
    
    void operator() (geometry::line_string<std::int64_t> const& l) {
        line_cover(tiles, extent, l);
    }

    void operator() (geometry::multi_line_string<std::int64_t> const& ml) {
        for (auto const& l : ml) {
            line_cover(tiles, extent, l);
        }
    }

    void operator() (geometry::polygon<std::int64_t> const& poly) {
        polygon_cover(tiles, extent, poly);
    }

    void operator() (geometry::multi_polygon<std::int64_t> const& mp) {
        for (auto const& p : mp) {
            polygon_cover(tiles, extent, p);
        }
    }

    void operator() (geometry::geometry_collection<std::int64_t> const& gc) {
        for (auto const& g : gc) {
            geometry::geometry<std::int64_t>::visit(g, (*this));
        }
    }
};

inline tile_coordinates get_tiles(geometry::geometry<std::int64_t> const& g,
                                  std::int64_t extent) {
    tile_coordinates tiles;
    geometry::geometry<std::int64_t>::visit(g, tile_cover_visitor { extent, tiles } );
    std::sort(tiles.begin(), tiles.end());
    tiles.erase(std::unique(tiles.begin(), tiles.end()), tiles.end());
    return tiles;
}

}}
