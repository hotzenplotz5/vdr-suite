'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.resolve(__dirname, '..', 'channel-day-program.js');
const source = fs.readFileSync(sourcePath, 'utf8');

let registeredName = null;
let registeredApi = null;

const window = {
  VdrSuitePlatform: {
    getSelectedBackendId() {
      return 'living-room';
    },
    getClientApi() {
      return null;
    },
    getMountTarget() {
      return null;
    },
    hasModule() {
      return false;
    },
    registerModule(name, api) {
      registeredName = name;
      registeredApi = api;
    }
  },
  setTimeout,
  clearTimeout
};

const document = {
  head: {
    appendChild() {}
  },
  createElement() {
    return {
      append() {},
      appendChild() {},
      classList: {
        add() {},
        remove() {}
      },
      dataset: {},
      setAttribute() {},
      replaceChildren() {},
      textContent: '',
      value: ''
    };
  },
  getElementById() {
    return null;
  },
  querySelector() {
    return null;
  }
};

vm.runInNewContext(source, {
  Array,
  Boolean,
  Date,
  Event: function Event() {},
  JSON,
  Map,
  Math,
  Number,
  Object,
  Promise,
  Set,
  String,
  document,
  window
}, {filename: sourcePath});

assert.strictEqual(registeredName, 'channels2');
assert.ok(registeredApi);
assert.strictEqual(window.VdrSuiteChannels2, registeredApi);
assert.strictEqual(typeof registeredApi.activate, 'function');
assert.strictEqual(typeof registeredApi.deactivate, 'function');
assert.strictEqual(typeof registeredApi.refresh, 'function');

assert.doesNotThrow(() => registeredApi.activate());
assert.doesNotThrow(() => registeredApi.refresh());
assert.doesNotThrow(() => registeredApi.deactivate());

assert.ok(!source.includes('fetch('));
assert.ok(source.includes("registerModule('channels2'"));
assert.ok(source.includes('fetchClientChannels'));
assert.ok(source.includes('fetchClientEpgCacheWindow'));
assert.ok(source.includes('fetchClientEpgChannelWindow'));
assert.ok(source.includes('fetchClientTimerCreateAction'));
assert.ok(source.includes('Channels 2'));

console.log('test_channel_day_program_runtime passed');
