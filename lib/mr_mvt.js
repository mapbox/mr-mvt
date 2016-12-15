"use strict";

var binary = require('node-pre-gyp');
var exists = require('fs').existsSync || require('path').existsSync;
var path = require('path');
var binding_path = binary.find(path.resolve(path.join(__dirname,'../package.json')));

var MR_MVT = module.exports = require(binding_path);
MR_MVT.version = require('../package.json').version;
