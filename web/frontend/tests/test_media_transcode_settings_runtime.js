'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.resolve(
  __dirname,
  '..',
  'settings-media-transcode.js'
);
const source = fs.readFileSync(sourcePath, 'utf8');

new vm.Script(source, {filename: sourcePath});

assert.ok(source.includes("'/settings/media-transcode'"));
assert.ok(source.includes("['deployment'"));
assert.ok(source.includes("['auto'"));
assert.ok(source.includes("['software'"));
assert.ok(source.includes("['vaapi'"));
assert.ok(source.includes('clearManagedOverride = true'));
assert.ok(source.includes('payload.videoEncoderMode = selected'));
assert.ok(source.includes('VdrSuiteBrowserSession'));
assert.ok(source.includes('csrfHeaders()'));
assert.ok(source.includes('Running sessions keep their encoder'));
assert.ok(source.includes('no silent software fallback'));
assert.ok(source.includes('minimumRealtimeThreshold'));
assert.ok(source.includes('softwareCalibrated'));
assert.ok(source.includes('vaapi.suitable'));
assert.ok(!source.includes('/dev/dri/'));
assert.ok(!source.includes('ffmpegArguments'));
assert.ok(!source.includes('VDR_SUITE_MEDIA_VAAPI_DEVICE'));

const document = {
  readyState: 'loading',
  documentElement: {lang: 'de'},
  listeners: {},
  addEventListener(name, listener) {
    this.listeners[name] = listener;
  }
};
const window = {};
vm.runInNewContext(source, {
  Boolean,
  Date,
  Error,
  JSON,
  MutationObserver: function () {},
  Number,
  Object,
  Promise,
  String,
  document,
  fetch() {
    return Promise.reject(new Error('not used'));
  },
  window
}, {filename: sourcePath});

assert.ok(window.VdrSuiteMediaTranscodeSettings);
assert.strictEqual(typeof window.VdrSuiteMediaTranscodeSettings.start, 'function');
assert.strictEqual(typeof window.VdrSuiteMediaTranscodeSettings.mount, 'function');
assert.strictEqual(typeof document.listeners.DOMContentLoaded, 'function');

console.log('test_media_transcode_settings_runtime passed');
