#pragma once

#include <iostream>
#ifdef DEBUG
#include <sstream>
#endif

#include <mapbox/geometry/box.hpp>
#include <mapbox/geometry/wagyu/quick_clip.hpp>
#include <mapbox/geometry/wagyu/wagyu.hpp>

#include <experimental/optional>

namespace mapbox { namespace mrmvt {

using optional_geometry = std::experimental::optional<geometry::geometry<std::int64_t>>;
using optional_linear_ring = std::experimental::optional<geometry::linear_ring<std::int64_t>>;

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

inline geometry::box<std::int64_t> create_bbox(std::int64_t x, std::int64_t y, std::int64_t buffer) {
    std::int64_t min_x = static_cast<std::int64_t>(x * 4096);
    std::int64_t min_y = static_cast<std::int64_t>(y * 4096);
    geometry::point<std::int64_t> min(min_x - buffer,  min_y - buffer);
    geometry::point<std::int64_t> max(min_x + 4096 + buffer,  min_y + 4096 + buffer);
    return geometry::box<std::int64_t>(min, max);
}

inline void multi_polygon_offset(geometry::multi_polygon<std::int64_t> & mp,
                                 std::int64_t offset_x,
                                 std::int64_t offset_y) {
    for (auto & poly : mp) {
        for (auto & ring : poly) {
            for (auto & pt : ring) {
                pt.x -= offset_x;
                pt.y -= offset_y;
            }
        }
    }
}

struct clip_visitor {

    geometry::box<std::int64_t> b;
    std::int64_t offset_x;
    std::int64_t offset_y;

    optional_geometry operator() (geometry::point<std::int64_t> const& pt) const {
        if (pt.x >= b.min.x && 
            pt.x <= b.max.x &&
            pt.y >= b.min.y &&
            pt.y <= b.max.y) {
            geometry::point<std::int64_t> new_pt { pt.x - offset_x, pt.y - offset_y };
            return optional_geometry(geometry::geometry<std::int64_t>(new_pt));
        }
        return optional_geometry();
    }
    
    optional_geometry operator() (geometry::multi_point<std::int64_t> const& mp) const {
        geometry::multi_point<std::int64_t> new_mp;
        new_mp.reserve(mp.size());
        for (auto const& pt : mp) {
            if (pt.x >= b.min.x && 
                pt.x <= b.max.x &&
                pt.y >= b.min.y &&
                pt.y <= b.max.y) {
                geometry::point<std::int64_t> new_pt { pt.x - offset_x, pt.y - offset_y };
                new_mp.push_back(new_pt);
            }
        }
        if (new_mp.empty()) {
            return optional_geometry();
        }
        return optional_geometry(geometry::geometry<std::int64_t>(std::move(new_mp)));
    }

    optional_geometry operator() (geometry::line_string<std::int64_t> const& ls) const {
        geometry::line_string<std::int64_t> new_ls;
        new_ls.reserve(ls.size());
        for (auto const& pt : ls) {
            if (pt.x >= b.min.x && 
                pt.x <= b.max.x &&
                pt.y >= b.min.y &&
                pt.y <= b.max.y) {
                geometry::point<std::int64_t> new_pt { pt.x - offset_x, pt.y - offset_y };
                new_ls.push_back(new_pt);
            }
        }
        if (new_ls.empty()) {
            return optional_geometry();
        }
        return optional_geometry(geometry::geometry<std::int64_t>(std::move(new_ls)));
    }

    optional_geometry operator() (geometry::multi_line_string<std::int64_t> const& mls) const {
        geometry::multi_line_string<std::int64_t> new_mls;
        new_mls.reserve(mls.size());
        for (auto const& ls : mls) {
            geometry::line_string<std::int64_t> new_ls;
            new_ls.reserve(ls.size());
            for (auto const& pt : ls) {
                if (pt.x >= b.min.x && 
                    pt.x <= b.max.x &&
                    pt.y >= b.min.y &&
                    pt.y <= b.max.y) {
                    geometry::point<std::int64_t> new_pt { pt.x - offset_x, pt.y - offset_y };
                    new_ls.push_back(new_pt);
                }
            }
            if (!new_ls.empty()) {
                new_mls.push_back(new_ls);
            }
        }
        if (new_mls.empty()) {
            return optional_geometry();
        }
        return optional_geometry(geometry::geometry<std::int64_t>(std::move(new_mls)));
    }

    optional_geometry operator() (geometry::polygon<std::int64_t> const& poly) const {
        if (poly.empty()) {
            return optional_geometry();
        }
        auto solution = geometry::wagyu::clip<std::int64_t>(poly,
                                              b,
                                              geometry::wagyu::fill_type_positive);
        multi_polygon_offset(solution, offset_x, offset_y);
        if (solution.empty()) {
            return optional_geometry();
        } else if (solution.size() == 1) {
            return optional_geometry(geometry::geometry<std::int64_t>(std::move(solution[0])));
        } else {
            return optional_geometry(geometry::geometry<std::int64_t>(std::move(solution)));
        }
    }
    
    optional_geometry operator() (geometry::multi_polygon<std::int64_t> const& mp) const {
        if (mp.empty()) {
            return optional_geometry();
        }
        auto solution = geometry::wagyu::clip<std::int64_t>(mp,
                                                 b,
                                                 geometry::wagyu::fill_type_positive);
        multi_polygon_offset(solution, offset_x, offset_y);
        if (solution.empty()) {
            return optional_geometry();
        } else if (solution.size() == 1) {
            return optional_geometry(geometry::geometry<std::int64_t>(std::move(solution[0])));
        } else {
            return optional_geometry(geometry::geometry<std::int64_t>(std::move(solution)));
        }
    }

    optional_geometry operator() (geometry::geometry_collection<std::int64_t> const& gc) const {
        geometry::geometry_collection<std::int64_t> new_gc;
        new_gc.reserve(gc.size());
        for (auto const& g : gc) {
            auto og = geometry::geometry<std::int64_t>::visit(g, (*this));
            if (og) {
                new_gc.push_back(std::move(*og));
            }
        }
        if (new_gc.empty()) {
            return optional_geometry();
        }
        return optional_geometry(geometry::geometry<std::int64_t>(std::move(new_gc)));
    }
};

inline optional_geometry clip(geometry::geometry<std::int64_t> const& g, std::uint32_t x, std::uint32_t y, std::int64_t buffer) {
    auto bbox = create_bbox(x, y, buffer);
    std::int64_t offset_x = bbox.min.x + buffer;
    std::int64_t offset_y = bbox.min.y + buffer;
    clip_visitor visitor { bbox, offset_x, offset_y };
    return geometry::geometry<std::int64_t>::visit(g, visitor);
}

}}
