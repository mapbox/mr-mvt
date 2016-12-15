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

test('MapToZoom - execute - point zoom 0', function(t) {
    var m2z = new mrmvt.MapToZoom(point_buffer);
    m2z.execute(0, {}, function(err, output) {
        t.error(err);
        t.equal(output.data, '{"type":"Point","coordinates":[2744,1613]}');
        t.equal(output.zoom, 0);
        t.equal(output.tiles.length, 1);
        t.equal(output.tiles[0][0], 0);
        t.equal(output.tiles[0][1], 0);
        t.equal(output.tiles[0][2], false);
        t.end();
    });
});

test('MapToZoom - execute - polygon zoom 10', function(t) {
    var m2z = new mrmvt.MapToZoom(polygon_buffer);
    m2z.execute(10, {}, function(err, output) {
        t.error(err);
        t.equal(output.zoom, 10);
        t.equal(output.tiles.length, 705);
        t.end();
    });
});
