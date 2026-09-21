'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const source = fs.readFileSync('web/frontend/hbbtv-qoi.js', 'utf8');
const window = {};
vm.runInNewContext(source, {
  window,
  Uint8Array,
  Uint8ClampedArray,
  ArrayBuffer,
  Number,
  Error,
  Object
}, {filename: 'hbbtv-qoi.js'});

assert.ok(window.VdrSuiteQoi);
assert.strictEqual(typeof window.VdrSuiteQoi.decode, 'function');

const bytes = new Uint8Array([
  0x71, 0x6f, 0x69, 0x66,
  0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x01,
  0x04, 0x00,
  0xff, 0xff, 0x00, 0x00, 0xff,
  0xff, 0x00, 0xff, 0x00, 0x80,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
]);

const decoded = window.VdrSuiteQoi.decode(bytes);
assert.strictEqual(decoded.width, 2);
assert.strictEqual(decoded.height, 1);
assert.strictEqual(decoded.channels, 4);
assert.deepStrictEqual(
  Array.from(decoded.pixels),
  [255, 0, 0, 255, 0, 255, 0, 128]
);

const transparent = new Uint8Array([
  0x71, 0x6f, 0x69, 0x66,
  0x00, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x01,
  0x04, 0x00,
  0xff, 0x10, 0x20, 0x30, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
]);
assert.strictEqual(
  window.VdrSuiteQoi.decode(transparent).pixels[3],
  0
);

assert.throws(
  function() {
    window.VdrSuiteQoi.decode(new Uint8Array([1, 2, 3]));
  },
  /hbbtv_qoi_truncated/
);

console.log('Phase 67 HbbTV QOI decoder ok');
