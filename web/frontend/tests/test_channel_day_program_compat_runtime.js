'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.resolve(__dirname, '..', 'channel-day-program-compat.js');
const source = fs.readFileSync(sourcePath, 'utf8');

function classList(initial) {
  const values = new Set(initial || []);
  return {
    add(value) {
      values.add(value);
    },
    contains(value) {
      return values.has(value);
    }
  };
}

const shell = {
  classList: classList(['channel-browser-module'])
};

const root = {
  querySelectorAll(selector) {
    assert.strictEqual(selector, '.channel-browser-module');
    return [shell];
  }
};

const window = {};
const document = {
  readyState: 'loading',
  addEventListener() {},
  getElementById() {
    return null;
  }
};

vm.runInNewContext(source, {
  Array,
  Boolean,
  MutationObserver: function MutationObserver() {},
  Object,
  Set,
  String,
  document,
  window
}, {filename: sourcePath});

const api = window.VdrSuiteChannelDayProgramCompat;
assert.ok(api);
assert.strictEqual(api.markChannelBrowserShells(root), 1);
assert.strictEqual(shell.classList.contains('channel-browser-shell'), true);
assert.ok(source.includes('.channel-browser-shell[hidden]{display:none!important;}'));
assert.ok(source.includes("event.target.closest(SOURCE_SELECTOR)"));
assert.ok(!source.includes('fetch('));

console.log('test_channel_day_program_compat_runtime passed');
