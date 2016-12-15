#pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-parameter"
// #pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

#include "node_map_to_zoom.hpp"

NAN_MODULE_INIT(Init) {
    MapToZoom::Initialize(target);
}

/*
 * This creates the module, started up with NAN_MODULE_INIT.
 * The naming/casing of the first argument is reflected in lib/map_to_zoom.js
 */
NODE_MODULE(mr_mvt, Init);
