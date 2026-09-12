'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const bridgeSource = fs.readFileSync(
  path.resolve(__dirname, '../web/frontend/browser-performance-bridge.js'),
  'utf8'
);

function createHarness() {
  const sent = [];
  const timers = [];
  let observer = null;
  let loaded = false;
  let now = 0;
  const frame = {
    attributes: { 'data-vdr-suite-diagnostics': 'cold-timeout-token' },
    hasAttribute(name) { return name in this.attributes; },
    getAttribute(name) { return this.attributes[name]; }
  };
  const parent = {
    postMessage(message, origin) { sent.push({ message, origin }); }
  };
  const owner = {
    snapshot() {
      return {
        loadingChannels: !loaded,
        loadingPrograms: !loaded,
        programmeLoadedChannelCount: loaded ? 24 : 0,
        programmeHasMore: true
      };
    }
  };
  const child = {
    frameElement: frame,
    parent,
    VdrSuiteArtworkProbe: { start() {}, stop() { return { metrics: {} }; } },
    VdrSuiteHomeLiveHero: owner,
    document: {
      documentElement: {},
      querySelectorAll(selector) {
        if (!loaded) return [];
        if (selector.includes('data-home-live-guide="now"')) return new Array(23).fill({});
        if (selector.includes('data-home-live-guide="next"')) return new Array(23).fill({});
        return [];
      }
    },
    performance: {
      now() { return now; },
      getEntriesByType() { return []; }
    },
    location: {
      origin: 'https://example.test',
      href: 'https://example.test/vdr-suite/frontend/browser-performance-home.html#vdrSuiteEpgCold=1'
    },
    URL,
    MutationObserver: class {
      constructor(callback) { this.callback = callback; observer = this; }
      observe() {}
      disconnect() {}
    },
    requestAnimationFrame() {
      // Intentionally stall paint callbacks to model throttled mobile iframes.
    },
    setTimeout(fn, ms) {
      timers.push({ fn, ms });
      return timers.length;
    },
    clearTimeout() {},
    addEventListener() {}
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

  return {
    sent,
    timers,
    getObserver() { return observer; },
    setLoaded(value) { loaded = Boolean(value); },
    setNow(value) { now = Number(value); }
  };
}

async function flush() {
  await new Promise(resolve => setImmediate(resolve));
}

(async function main() {
  const timeout = createHarness();
  assert.equal(timeout.timers.length, 1);
  assert.equal(timeout.timers[0].ms, 30000);
  timeout.setNow(30000);
  timeout.timers[0].fn();
  await flush();

  const timeoutMessage = timeout.sent.find(item => item.message.type === 'epg-result');
  assert.ok(timeoutMessage, 'cold EPG timeout must complete without paint callbacks');
  assert.equal(timeoutMessage.message.value.label, 'home-epg-cold-startup');
  assert.equal(timeoutMessage.message.value.timedOut, true);
  assert.equal(timeoutMessage.message.value.milestones.paintReadyMs, null);

  const success = createHarness();
  assert.equal(success.timers.length, 1);
  assert.equal(success.timers[0].ms, 30000);
  success.setNow(700);
  success.setLoaded(true);
  success.getObserver().callback([{}]);

  assert.equal(success.timers.length, 2, 'successful readiness must arm a bounded paint fallback');
  assert.equal(success.timers[1].ms, 250);
  assert.equal(success.sent.some(item => item.message.type === 'epg-result'), false);

  success.setNow(950);
  success.timers[1].fn();
  await flush();

  const successMessage = success.sent.find(item => item.message.type === 'epg-result');
  assert.ok(successMessage, 'ready cold EPG measurement must complete when animation frames stall');
  assert.equal(successMessage.message.value.label, 'home-epg-cold-startup');
  assert.equal(successMessage.message.value.mode, 'cold-startup');
  assert.equal(successMessage.message.value.timedOut, false);
  assert.equal(successMessage.message.value.rails.nowCards, 23);
  assert.equal(successMessage.message.value.rails.nextCards, 23);
  assert.equal(successMessage.message.value.milestones.paintReadyMs, 950);

  console.log('browser Home EPG timeout and stalled-paint fallback complete deterministically');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
