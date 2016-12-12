#pragma once

#ifdef DEBUG
#include <sstream>
#endif

#include <mapbox/geometry/wagyu/wagyu.hpp>

#include <experimental/optional>

namespace mapbox { namespace mrmvt {

using optional_geometry = std::experimental::optional<geometry::geometry<std::int64_t>>;

struct bbox {
    std::int64_t min_x;
    std::int64_t max_x;
    std::int64_t min_y;
    std::int64_t max_y;
    std::int64_t unbuffered_min_x;
    std::int64_t unbuffered_min_y;
};

inline bbox create_bbox(std::uint32_t x, std::uint32_t y, std::int64_t buffer) {
    std::int64_t min_x = static_cast<std::int64_t>(x * 4096);
    std::int64_t min_y = static_cast<std::int64_t>(y * 4096);
    return bbox { min_x - buffer, 
                  min_x + 4095 + buffer, 
                  min_y - buffer, 
                  min_y + 4095 + buffer,
                  min_x,
                  min_y }; 
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

    bbox b;

    optional_geometry operator() (geometry::point<std::int64_t> const& pt) const {
        if (pt.x >= b.min_x && 
            pt.x <= b.max_x &&
            pt.y >= b.min_y &&
            pt.y <= b.max_y) {
            geometry::point<std::int64_t> new_pt { pt.x - b.unbuffered_min_x, pt.y - b.unbuffered_min_y };
            return optional_geometry(geometry::geometry<std::int64_t>(new_pt));
        }
        return optional_geometry();
    }
    
    optional_geometry operator() (geometry::multi_point<std::int64_t> const& mp) const {
        geometry::multi_point<std::int64_t> new_mp;
        new_mp.reserve(mp.size());
        for (auto const& pt : mp) {
            if (pt.x >= b.min_x && 
                pt.x <= b.max_x &&
                pt.y >= b.min_y &&
                pt.y <= b.max_y) {
                geometry::point<std::int64_t> new_pt { pt.x - b.unbuffered_min_x, pt.y - b.unbuffered_min_y };
                new_mp.push_back(new_pt);
            }
        }
        if (new_mp.empty()) {
            return optional_geometry();
        }
        return optional_geometry(geometry::geometry<std::int64_t>(std::move(new_mp)));
    }

    optional_geometry operator() (geometry::line_string<std::int64_t> const& ls) {
        geometry::line_string<std::int64_t> new_ls;
        new_ls.reserve(ls.size());
        for (auto const& pt : ls) {
            if (pt.x >= b.min_x && 
                pt.x <= b.max_x &&
                pt.y >= b.min_y &&
                pt.y <= b.max_y) {
                geometry::point<std::int64_t> new_pt { pt.x - b.unbuffered_min_x, pt.y - b.unbuffered_min_y };
                new_ls.push_back(new_pt);
            }
        }
        if (new_ls.empty()) {
            return optional_geometry();
        }
        return optional_geometry(geometry::geometry<std::int64_t>(std::move(new_ls)));
    }

    optional_geometry operator() (geometry::multi_line_string<std::int64_t> const& mls) {
        geometry::multi_line_string<std::int64_t> new_mls;
        new_mls.reserve(mls.size());
        for (auto const& ls : mls) {
            geometry::line_string<std::int64_t> new_ls;
            new_ls.reserve(ls.size());
            for (auto const& pt : ls) {
                if (pt.x >= b.min_x && 
                    pt.x <= b.max_x &&
                    pt.y >= b.min_y &&
                    pt.y <= b.max_y) {
                    geometry::point<std::int64_t> new_pt { pt.x - b.unbuffered_min_x, pt.y - b.unbuffered_min_y };
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

    optional_geometry operator() (geometry::polygon<std::int64_t> const& poly) {
        if (poly.empty()) {
            return optional_geometry();
        }
        geometry::linear_ring<std::int64_t> clip_box;
        clip_box.reserve(4);
        clip_box.push_back(geometry::point<std::int64_t> { b.min_x, b.min_y } );
        clip_box.push_back(geometry::point<std::int64_t> { b.min_x, b.max_y } );
        clip_box.push_back(geometry::point<std::int64_t> { b.max_x, b.max_y } );
        clip_box.push_back(geometry::point<std::int64_t> { b.max_x, b.min_y } );
        clip_box.push_back(geometry::point<std::int64_t> { b.min_x, b.min_y } );
        
        mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
        clipper.add_ring(clip_box, mapbox::geometry::wagyu::polygon_type_clip);
        clipper.add_polygon(poly);

        mapbox::geometry::multi_polygon<std::int64_t> solution;
        clipper.execute(mapbox::geometry::wagyu::clip_type_intersection, solution,
                        mapbox::geometry::wagyu::fill_type_positive,
                        mapbox::geometry::wagyu::fill_type_even_odd);
        multi_polygon_offset(solution, b.unbuffered_min_x, b.unbuffered_min_y);
        if (solution.empty()) {
            return optional_geometry();
        } else if (solution.size() == 1) {
            return optional_geometry(geometry::geometry<std::int64_t>(std::move(solution[0])));
        } else {
            return optional_geometry(geometry::geometry<std::int64_t>(std::move(solution)));
        }
    }
    
    optional_geometry operator() (geometry::multi_polygon<std::int64_t> const& mp) {
        if (mp.empty()) {
            return optional_geometry();
        }
        geometry::linear_ring<std::int64_t> clip_box;
        clip_box.reserve(4);
        clip_box.push_back(geometry::point<std::int64_t> { b.min_x, b.min_y } );
        clip_box.push_back(geometry::point<std::int64_t> { b.min_x, b.max_y } );
        clip_box.push_back(geometry::point<std::int64_t> { b.max_x, b.max_y } );
        clip_box.push_back(geometry::point<std::int64_t> { b.max_x, b.min_y } );
        clip_box.push_back(geometry::point<std::int64_t> { b.min_x, b.min_y } );
        
        mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
        clipper.add_ring(clip_box, mapbox::geometry::wagyu::polygon_type_clip);
        
        for (auto const& poly : mp) {
            clipper.add_polygon(poly);
        }

        mapbox::geometry::multi_polygon<std::int64_t> solution;
        clipper.execute(mapbox::geometry::wagyu::clip_type_intersection, solution,
                        mapbox::geometry::wagyu::fill_type_positive,
                        mapbox::geometry::wagyu::fill_type_even_odd);
        multi_polygon_offset(solution, b.unbuffered_min_x, b.unbuffered_min_y);
        if (solution.empty()) {
            return optional_geometry();
        } else if (solution.size() == 1) {
            return optional_geometry(geometry::geometry<std::int64_t>(std::move(solution[0])));
        } else {
            return optional_geometry(geometry::geometry<std::int64_t>(std::move(solution)));
        }
    }

    optional_geometry operator() (geometry::geometry_collection<std::int64_t> const& gc) {
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
    return geometry::geometry<std::int64_t>::visit(g, clip_visitor { bbox });
}

}}
