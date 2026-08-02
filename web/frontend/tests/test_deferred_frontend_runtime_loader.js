'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

class MockScript {
  constructor() {
    this.id = '';
    this.src = '';
    this.async = true;
    this.dataset = {};
    this.listeners = {};
  }

  addEventListener(name, listener) {
    if (!this.listeners[name]) this.listeners[name] = [];
    this.listeners[name].push(listener);
  }

  dispatch(name) {
    (this.listeners[name] || []).forEach(listener => listener());
  }
}

async function verifyRuntimeApi() {
  const scripts = [];
  const document = {
    readyState: 'loading',
    listeners: {},
    head: {
      appendChild(script) {
        scripts.push(script);
      }
    },
    createElement(tagName) {
      assert.strictEqual(tagName, 'script');
      return new MockScript();
    },
    getElementById(id) {
      return scripts.find(script => script.id === id) || null;
    },
    addEventListener(name, listener) {
      this.listeners[name] = listener;
    }
  };
  const window = {
    fetch() {
      return Promise.reject(new Error('not used'));
    },
    location: {href: 'http://localhost/frontend/'}
  };

  vm.runInNewContext(source, {
    Boolean,
    Date,
    Error,
    Object,
    Promise,
    String,
    URL,
    URLSearchParams,
    console,
    document,
    window
  }, {filename: sourcePath});

  assert.ok(window.VdrSuiteDeferredFrontendRuntimes);
  assert.strictEqual(typeof window.VdrSuiteDeferredFrontendRuntimes.loadEpgDetail, 'function');
  assert.strictEqual(typeof window.VdrSuiteDeferredFrontendRuntimes.loadRecordings2, 'function');
  assert.strictEqual(scripts.length, 0);

  const first = window.VdrSuiteDeferredFrontendRuntimes.loadEpgDetail();
  const second = window.VdrSuiteDeferredFrontendRuntimes.loadEpgDetail();
  assert.strictEqual(scripts.length, 1);
  assert.strictEqual(scripts[0].id, 'vdr-suite-epg-searchtimer-actions-runtime');
  assert.strictEqual(scripts[0].src, '/frontend/epg-searchtimer-actions.js');
  assert.strictEqual(scripts[0].async, false);
  scripts[0].dispatch('load');
  await Promise.all([first, second]);
  assert.strictEqual(scripts[0].dataset.loaded, 'true');

  window.VdrSuiteEpgSearchTimerActions = {};
  window.VdrSuiteEpgMetadataDetail = {};
  window.VdrSuiteEpgMetadataDetailHook = {};
  window.VdrSuiteEpgDetailDesktopFocus = {};
  await window.VdrSuiteDeferredFrontendRuntimes.loadEpgDetail();
  assert.strictEqual(scripts.length, 1);

  delete window.VdrSuiteEpgSearchTimerActions;
  delete window.VdrSuiteEpgMetadataDetail;
  delete window.VdrSuiteEpgMetadataDetailHook;
  delete window.VdrSuiteEpgDetailDesktopFocus;
  scripts[0].dataset.loaded = '';
  scripts[0].dataset.failed = 'true';
  await assert.rejects(
    window.VdrSuiteDeferredFrontendRuntimes.loadEpgDetail(),
    /Frontend-Runtime konnte nicht geladen werden/
  );
}

const sourcePath = path.resolve(
  __dirname,
  '..',
  'platform',
  'deferred-runtime-loader.js'
);
const source = fs.readFileSync(sourcePath, 'utf8');

assert.ok(source.includes("'/frontend/epg-searchtimer-actions.js'"));
assert.ok(source.includes("'/frontend/recordings2-shared.js'"));
assert.ok(source.includes("'/frontend/recordings2-actions.js'"));
assert.ok(source.includes("'/frontend/recordings2.js'"));
assert.ok(!source.includes("'/frontend/recording-trash-ux.js'"));
assert.ok(source.includes('script.src = src;'));
assert.ok(!source.includes("script.src = src + '?runtime='"));
assert.ok(source.includes('window.VdrSuiteEpgSearchTimerActions'));
assert.ok(source.includes('window.VdrSuiteEpgMetadataDetail'));
assert.ok(source.includes('window.VdrSuiteEpgMetadataDetailHook'));
assert.ok(source.includes('window.VdrSuiteEpgDetailDesktopFocus'));
assert.ok(source.includes('window.VdrSuiteRecordings2'));
assert.ok(source.includes('window.VdrSuiteDeferredFrontendRuntimes'));
assert.ok(source.includes('function loadVdrSuiteEpgDetailRuntime()'));
assert.ok(source.includes('function loadVdrSuiteRecordings2Runtime()'));
assert.ok(source.includes('start: startVdrSuiteDeferredFrontendRuntimes'));
assert.ok(source.includes('loadEpgDetail: loadVdrSuiteEpgDetailRuntime'));
assert.ok(source.includes('loadRecordings2: loadVdrSuiteRecordings2Runtime'));
assert.ok(source.includes("document.addEventListener(\n      'DOMContentLoaded',"));
assert.ok(source.includes('startVdrSuiteDeferredFrontendRuntimes,\n      {once: true}'));
assert.ok(source.includes('} else {\n    startVdrSuiteDeferredFrontendRuntimes();'));
assert.ok(!source.includes("'/frontend/epg-metadata-detail.js'"));
assert.ok(!source.includes("'/frontend/epg-metadata-detail-hook.js'"));
assert.ok(!source.includes("'/frontend/epg-detail-desktop-focus.js'"));
assert.ok(!source.includes("'/frontend/channel-day-program.js'"));
assert.ok(!source.includes("'/frontend/channel-day-program-compat.js'"));

verifyRuntimeApi().then(() => {
  console.log('test_deferred_frontend_runtime_loader passed');
}).catch(error => {
  console.error(error);
  process.exitCode = 1;
});

require('./test_query_cache_refresh_security_runtime.js');
