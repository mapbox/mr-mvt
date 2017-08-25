#pragma once

#include <mapbox/geometry.hpp>

#pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-parameter"
// #pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

/**
 * MapToZoom class
 * This is in a header file so we can access it across other .cpp files
 * if necessary
 */
class MapToZoom: public Nan::ObjectWrap {
    public:
        MapToZoom(const char* json, std::size_t size);
        
        // initializer
        static void Initialize(v8::Handle<v8::Object> target);
        static Nan::Persistent<v8::FunctionTemplate> constructor;

        // methods required for the constructor
        static NAN_METHOD(New);

        // shout, custom async method
        static NAN_METHOD(execute);
        static void AsyncExecute(uv_work_t* req);
        static void AfterExecute(uv_work_t* req);

    private:
        // member variable
        // specific to each instance of the class
        mapbox::geometry::geometry<double> geom; 

};
