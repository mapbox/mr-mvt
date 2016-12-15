#pragma once

#pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wunused-parameter"
// #pragma GCC diagnostic ignored "-Wshadow"
#include <nan.h>
#pragma GCC diagnostic pop

/*
 * This is an internal function used to return callback error messages instead of
 * throwing errors.
 * Usage: 
 * 
 * v8::Local<v8::Function> callback;
 * CallbackError("error message", callback);
 * return; // this is important to prevent duplicate callbacks from being fired!
 */
inline void CallbackError(std::string message, v8::Local<v8::Function> callback) {
    v8::Local<v8::Value> argv[1] = { Nan::Error(message.c_str()) };
    Nan::MakeCallback(Nan::GetCurrentContext()->Global(), callback, 1, argv);
}
