'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const bridgeSource = fs.readFileSync(
  path.resolve(__dirname, '../web/frontend/browser-performance-bridge.js'),
  'utf8'
);

const sent = [];
const listeners = {};
const timers = [];
const frame = {
  attributes: { 'data-vdr-suite-diagnostics': 'cold-timeout-token' },
  hasAttribute(name) { return name in this.attributes; },
  getAttribute(name) { return this.attributes[name]; }
};
const parent = {
  postMessage(message, origin) { sent.push({ message, origin }); }
};
const child = {
  frameElement: frame,
  parent,
  VdrSuiteArtworkProbe: { start() {}, stop() { return { metrics: {} }; } },
  document: {
    documentElement: {},
    querySelectorAll() { return []; }
  },
  performance: {
    now() { return 30000; },
    getEntriesByType() { return []; }
  },
  location: {
    origin: 'https://example.test',
    href: 'https://example.test/vdr-suite/frontend/browser-performance-home.html#vdrSuiteEpgCold=1'
  },
  URL,
  MutationObserver: class {
    observe() {}
    disconnect() {}
  },
  requestAnimationFrame() {
    // Intentionally never invokes the callback. A timed-out cold diagnostic
    // must still complete instead of waiting forever for paint callbacks.
  },
  setTimeout(fn, ms) {
    timers.push({ fn, ms });
    return timers.length;
  },
  clearTimeout() {},
  addEventListener(type, fn) { listeners[type] = fn; }
};

vm.runInNewContext(bridgeSource, {
  window: child,
  Promise,
  URL,
  Number,
  Math,
  String,
  Boolean,
  Array,
  Object,
  RegExp,
  Error
});

assert.equal(timers.length, 1);
assert.equal(timers[0].ms, 30000);
assert.equal(sent.some(item => item.message.type === 'epg-result'), false);

timers[0].fn();

setImmediate(() => {
  const message = sent.find(item => item.message.type === 'epg-result');
  assert.ok(message, 'cold EPG timeout must complete even when requestAnimationFrame stalls');
  assert.equal(message.message.value.label, 'home-epg-cold-startup');
  assert.equal(message.message.value.mode, 'cold-startup');
  assert.equal(message.message.value.timedOut, true);
  assert.equal(message.message.value.milestones.paintReadyMs, null);
  console.log('browser Home EPG timeout completes without paint callbacks');
});
