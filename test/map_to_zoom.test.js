var test = require('tape');
var mrmvt = require('../lib/index.js');
var fs = require('fs');

var point_buffer = fs.readFileSync('./test/fixtures/point.geojson');
var polygon_buffer = fs.readFileSync('./test/fixtures/polygon.geojson');

test('MapToZoom - Initialization', function(t) {
    t.doesNotThrow(function() {
        new mrmvt.MapToZoom(point_buffer);
    });
    t.doesNotThrow(function() {
        new mrmvt.MapToZoom(polygon_buffer);
    });
    t.end();
});

test('MapToZoom - Initialization with Errors', function(t) {
    // empty buffer
    t.throws(function() {
        new mrmvt.MapToZoom(new Buffer(0));
    }, new RegExp('buffer is empty'));
    // invalid buffer
    t.throws(function() {
        new mrmvt.MapToZoom(new Buffer(1));
    }, new RegExp('Error in processing buffer'));
    // empty object
    t.throws(function() {
        new mrmvt.MapToZoom({});
    }, new RegExp('arg must be a buffer'));
    // invalid type - string
    t.throws(function() {
        new mrmvt.MapToZoom('');
    }, new RegExp('arg must be a buffer'));
    // invalid type - number
    t.throws(function() {
        new mrmvt.MapToZoom(1);
    }, new RegExp('arg must be a buffer'));
    // no parameters
    t.throws(function() {
        new mrmvt.MapToZoom();
    }, new RegExp('MapToZoom requires one parameter, a buffer'));
    t.end();
});
/*
test('MapToZoom - Execute', function(t) {
    var m2z = new mrmvt.MapToZoom(point_buffer);
    m2z.execute(0, {}, function(err, output) {
        t.error(err);
        t.end();
    });
});
*/
