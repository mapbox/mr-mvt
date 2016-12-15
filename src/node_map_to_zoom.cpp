#include "node.hpp"
#include "node_map_to_zoom.hpp"
#include "tile_cover.hpp"
#include "map_to_zoom.hpp"

#include <exception>
#include <stdexcept>
#include <iostream>

MapToZoom::MapToZoom(char const* json) : 
    geom(mapbox::geojson::parse_geometry<double>(json)) {
}

void MapToZoom::Initialize(v8::Handle<v8::Object> target) {

    Nan::HandleScope scope;

    v8::Local<v8::FunctionTemplate> lcons = Nan::New<v8::FunctionTemplate>(MapToZoom::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    
    lcons->SetClassName(Nan::New("MapToZoom").ToLocalChecked());
    
    Nan::SetPrototypeMethod(lcons, "execute", execute);
    
    target->Set(Nan::New("MapToZoom").ToLocalChecked(),lcons->GetFunction());
    constructor.Reset(lcons);
}

Nan::Persistent<v8::FunctionTemplate> MapToZoom::constructor;

/**
 * Main class, called MapToZoom
 * @class MapToZoom
 * @param {Buffer} buffer
 * @example
 * var mrmvt = require('index.js');
 * var m2z = new mrmvt.MapToZoom(buffer);
 */
NAN_METHOD(MapToZoom::New) {
    if (!info.IsConstructCall()) {
        return Nan::ThrowTypeError("Cannot call constructor as function, you need to use 'new' keyword");
    }

    if (info.Length() < 1) {
        return Nan::ThrowTypeError("MapToZoom requires one parameter, a buffer");
    }
    if (!info[0]->IsObject()) {
        return Nan::ThrowTypeError("arg must be a buffer");
    }
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        return Nan::ThrowTypeError("arg must be a buffer");
    }
    if (node::Buffer::Length(obj) <= 0) {
        return Nan::ThrowTypeError("buffer is empty");
    }
    try {
        const char* json = node::Buffer::Data(obj);
        auto *const self = new MapToZoom(json);
        self->Wrap(info.This());
    } catch (const std::exception &ex) {
        std::string err_msg = "Error in processing buffer: ";
        err_msg += std::string(ex.what());
        return Nan::ThrowError(err_msg.c_str());
    }
    info.GetReturnValue().Set(info.This());
}

/**
 * Tile coordinates information as an array in the form of
 * [x, y, is_solid]
 *
 * @typedef {Array} tileCoordinates
 * 
 */

/**
 * The callback type 'mapToZoomCallback' and is the definition
 * of the callback required for using the MapToZoom objects's 
 * execute method
 *
 * @callback mapToZoomCallback
 * @param {Object} error, null if no error is thrown
 * @param {Object} [response]
 * @param {number} [response.zoom] - the zoom level of the data provided
 * @param {string} [response.data] - a string containing the geometry data for the zoom level
 * @param {tileCoordinates[]} [response.tiles] - array of tile coordinates
 */

/**
 * Request data in tile coordinates for a particular zoom level
 * additionally, will recieve an array of all the vector tiles associated
 * with that zoom level.
 *
 * @name execute
 * @memberof MapToZoom
 * @param {number} zoom
 * @param {Object} [options]
 * @param {number} [options.simplify_distance=4] - the distance for simplification, 0 disables simplification
 * @param {number} [options.extent=4096] - the size of the extent for tiles
 * @param {mapToZoomCallback} callback
 * @example
 * var m2z = new mrmvt.MapToZoom();
 * m2z.execute(0, {}, function(err, response) {
 *   if (err) throw err;
 *   // use response object here
 * });
 *
 */

struct MapToZoomBaton {
    uv_work_t request; // required
    Nan::Persistent<v8::Function> cb; // callback function type
    mapbox::geometry::geometry<double> const& geom;
    mapbox::tile_cover::tile_coordinates tiles;
    double simplify_distance;
    std::size_t zoom;
    std::size_t extent;
    std::string error_name;
    std::string result;

    MapToZoomBaton(mapbox::geometry::geometry<double> const& g,
                   double simplify_distance_,
                   std::size_t zoom_,
                   std::size_t extent_,
                   v8::Local<v8::Function> const& callback) : 
            request(),
            cb(callback),
            geom(g),
            tiles(),
            simplify_distance(simplify_distance_),
            zoom(zoom_),
            extent(extent_),
            error_name(),
            result() {
        request.data = this;
    }
};

NAN_METHOD(MapToZoom::execute) {
    // Default values
    std::size_t zoom = 0;
    std::size_t extent = 4096;
    double simplify_distance = 4.0;

    // check third argument, should be a 'callback' function.
    // This allows us to set the callback so we can use it to return errors
    // instead of throwing as well.
    if (!info[2]->IsFunction()) {
        Nan::ThrowTypeError("third arg 'callback' must be a function");
        return;
    }
    v8::Local<v8::Function> callback = info[2].As<v8::Function>();

    if (!info[0]->IsUint32()) {
        CallbackError("first arg 'zoom' must be an unsigned integer", callback);
        return;
    }
    zoom = static_cast<std::size_t>(info[0]->Uint32Value());

    // check second argument, should be an 'options' object
    if (!info[1]->IsObject()) 
    {
        CallbackError("second arg 'options' must be an object", callback);
        return;
    }
    v8::Local<v8::Object> options = info[1].As<v8::Object>();

    if (options->Has(Nan::New("extent").ToLocalChecked())) {
        v8::Local<v8::Value> extent_val = options->Get(Nan::New("extent").ToLocalChecked());
        if (!extent_val->IsUint32())
        {
            CallbackError("option 'extent' must be an unsigned integer", callback);
            return;
        }
        extent = static_cast<std::size_t>(extent_val->Uint32Value());
    }
    
    if (options->Has(Nan::New("simplify_distance").ToLocalChecked())) {
        v8::Local<v8::Value> simplify_distance_val = options->Get(Nan::New("simplify_distance").ToLocalChecked());
        if (!simplify_distance_val->IsNumber())
        {
            CallbackError("option 'simplify_distance' must be an number", callback);
            return;
        }
        simplify_distance = simplify_distance_val->NumberValue();
        if (simplify_distance < 0.0) {
            CallbackError("option 'simplify_distance' must be a positive number value", callback);
            return;
        }
    }

    // set up the baton to pass into our threadpool
    MapToZoom* me = Nan::ObjectWrap::Unwrap<MapToZoom>(info.Holder());

    MapToZoomBaton *baton = new MapToZoomBaton(me->geom, simplify_distance, zoom, extent, callback);

    /*
    `uv_queue_work` is the all-important way to pass info into the threadpool.
    It cannot take v8 objects, so we need to do some manipulation above to convert into cpp objects
    otherwise things get janky. It takes four arguments:

    1) which loop to use, node only has one so we pass in a pointer to the default
    2) the baton defined above, we use this to access information important for the method
    3) operations to be executed within the threadpool
    4) operations to be executed after #3 is complete to pass into the callback
    */
    uv_queue_work(uv_default_loop(), &baton->request, AsyncExecute, (uv_after_work_cb)AfterExecute);
    return;
}

void MapToZoom::AsyncExecute(uv_work_t* req)
{
    MapToZoomBaton *baton = static_cast<MapToZoomBaton *>(req->data);

    // The try/catch is critical here: if code was added that could throw an unhandled error INSIDE the threadpool, it would be disasterous
    try {
        mapbox::geometry::geometry<std::int64_t> g = mapbox::mrmvt::geom_to_zoom(baton->geom, 
                                                          baton->zoom,
                                                          baton->extent,
                                                          baton->simplify_distance);
        baton->result = mapbox::geojson::stringify<std::int64_t>(g);
        baton->tiles = mapbox::tile_cover::get_tiles(g, baton->extent);
    } catch (std::exception const& ex) {
        baton->error_name = ex.what();
    }

}

void MapToZoom::AfterExecute(uv_work_t* req)
{
    Nan::HandleScope scope;

    MapToZoomBaton *baton = static_cast<MapToZoomBaton *>(req->data);

    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(baton->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(baton->cb), 1, argv);
    } else {
        v8::Local<v8::Object> result = Nan::New<v8::Object>();
        v8::Local<v8::Array> tiles = Nan::New<v8::Array>(baton->tiles.size());
        std::size_t i = 0;
        for (auto const& t : baton->tiles) {
            v8::Local<v8::Array> row = Nan::New<v8::Array>(3);
            Nan::Set(row, 0, Nan::New(t.x));
            Nan::Set(row, 1, Nan::New(t.y));
            Nan::Set(row, 2, Nan::New(t.fill));
            Nan::Set(tiles, i++, row);
        }
        Nan::Set(result, Nan::New("tiles").ToLocalChecked(), tiles);
        Nan::Set(result, Nan::New("zoom").ToLocalChecked(), Nan::New(static_cast<std::uint32_t>(baton->zoom)));
        Nan::Set(result, Nan::New("data").ToLocalChecked(), Nan::New<v8::String>(baton->result.data()).ToLocalChecked());
        v8::Local<v8::Value> argv[2] = { Nan::Null(), result };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(baton->cb), 2, argv);
    }

    baton->cb.Reset();
    delete baton;
}

