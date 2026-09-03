'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'home-recording-discovery.js'),
  'utf8'
);

class FakeElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.attributes = {};
    this.dataset = {};
    this.listeners = {};
    this.parentNode = null;
    this.className = '';
    this.textContent = '';
    this.type = '';
    this.src = '';
    this.alt = '';
    this.loading = '';
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }

  appendChild(child) {
    if (!child) return child;
    if (child.parentNode && typeof child.remove === 'function') child.remove();
    child.parentNode = this;
    this.children.push(child);
    return child;
  }

  append() {
    Array.from(arguments).forEach((child) => this.appendChild(child));
  }

  replaceChildren() {
    this.children.forEach((child) => { child.parentNode = null; });
    this.children = [];
    Array.from(arguments).forEach((child) => this.appendChild(child));
  }

  addEventListener(type, handler) {
    if (!this.listeners[type]) this.listeners[type] = [];
    this.listeners[type].push(handler);
  }

  remove() {
    if (!this.parentNode) return;
    const index = this.parentNode.children.indexOf(this);
    if (index >= 0) this.parentNode.children.splice(index, 1);
    this.parentNode = null;
  }

  insertBefore(child, reference) {
    if (!child) return child;
    if (child.parentNode && typeof child.remove === 'function') child.remove();
    child.parentNode = this;
    if (!reference) {
      this.children.push(child);
      return child;
    }
    const index = this.children.indexOf(reference);
    if (index < 0) this.children.push(child);
    else this.children.splice(index, 0, child);
    return child;
  }

  get nextElementSibling() {
    if (!this.parentNode) return null;
    const index = this.parentNode.children.indexOf(this);
    return index >= 0 ? this.parentNode.children[index + 1] || null : null;
  }

  querySelector(selector) {
    const match = String(selector || '').match(
      /^\[data-home-discovery-rail="([^"]+)"\]$/
    );
    if (!match) return null;
    return findElement(this, (element) =>
      element.attributes['data-home-discovery-rail'] === match[1]);
  }

  querySelectorAll() {
    return [];
  }

  closest() {
    return null;
  }
}

function findElement(root, predicate) {
  if (!root) return null;
  for (const child of root.children || []) {
    if (predicate(child)) return child;
    const nested = findElement(child, predicate);
    if (nested) return nested;
  }
  return null;
}

function findRail(host, key) {
  return host.querySelector('[data-home-discovery-rail="' + key + '"]');
}

function makeSeriesRecording(backendId) {
  return {
    recordingId: backendId + '-series-1',
    backendId,
    backendNativeId: backendId + '-native-series-1',
    path: 'Serien/Testserie/S01E01 Pilot',
    title: 'Serien/Testserie/S01E01 Pilot',
    metadata: {
      provider: {},
      presentation: {posterUrl: ''},
      artwork: {preferredUrl: ''}
    }
  };
}

function createHarness(initialMetadataMode) {
  const host = new FakeElement('div');
  const documentListeners = {};
  const observers = [];
  const genreCalls = [];
  const genreListCalls = [];
  const metadataResolvers = [];
  let selectedModule = 'overview';
  let backendId = 'default';
  let metadataMode = initialMetadataMode || 'available-false';

  class FakeIntersectionObserver {
    constructor(callback) {
      this.callback = callback;
      this.disconnected = false;
      this.target = null;
      observers.push(this);
    }

    observe(target) {
      this.target = target;
    }

    disconnect() {
      this.disconnected = true;
    }

    fire() {
      this.callback([{isIntersecting: true, target: this.target}]);
    }
  }

  const client = {
    fetchClientRecordings() {
      return Promise.resolve({recordings: []});
    },
    fetchClientGenres(request) {
      genreListCalls.push(request);
      return Promise.resolve({
        genres: [
          {id: 'other', label: 'Andere', count: 1},
          {id: 'series', label: 'Serien', count: 1}
        ]
      });
    },
    fetchClientGenreRecordings(request) {
      genreCalls.push({
        backendId: request.backendId,
        genreId: request.genreId,
        offset: Number(request.offset || 0)
      });
      if (request.genreId !== 'series') {
        return Promise.resolve({recordings: [], total: 0, hasMore: false});
      }
      const recording = makeSeriesRecording(request.backendId);
      return Promise.resolve({
        recordings: [recording],
        total: 1,
        hasMore: false
      });
    },
    fetchClientRecordingFolder() {
      return Promise.resolve({folders: [], recordings: [], recordingCount: 0});
    },
    requestJson(route, request) {
      assert.strictEqual(route, '/api/vdr/recordings/metadata');
      assert.strictEqual(request.query.backend, backendId);
      if (metadataMode === 'reject') {
        return Promise.reject(new Error('metadata unavailable'));
      }
      if (metadataMode === 'deferred') {
        return new Promise((resolve) => {
          metadataResolvers.push(resolve);
        });
      }
      return Promise.resolve({available: false});
    }
  };

  const document = {
    readyState: 'complete',
    head: null,
    querySelector(selector) {
      return selector === '[data-home-zone="additional-sections"]' ? host : null;
    },
    createElement(tagName) {
      return new FakeElement(tagName);
    },
    addEventListener(type, handler) {
      if (!documentListeners[type]) documentListeners[type] = [];
      documentListeners[type].push(handler);
    },
    getElementById() { return null; }
  };

  const context = {window: {}, console};
  context.Math = Object.create(Math);
  context.Math.random = function () { return 0; };
  context.window.window = context.window;
  context.window.document = document;
  context.window.IntersectionObserver = FakeIntersectionObserver;
  context.window.setTimeout = function (callback) {
    callback();
    return 1;
  };
  context.window.selectModule = function (moduleName) {
    selectedModule = moduleName;
    return true;
  };
  context.window.VdrSuiteHomeLivePreview = {cancel() {}};
  context.window.VdrSuiteHomeRecordingDiscoveryBootstrap = {
    installMouseDrag() { return true; }
  };
  context.window.VdrSuitePlatform = {
    getSelectedBackendId() { return backendId; },
    getSelectedModule() { return selectedModule; },
    getClientApi() { return client; }
  };

  vm.createContext(context);
  vm.runInContext(source, context, {
    filename: 'post-phase66-recording-discovery-performance.js'
  });

  const publicApi = context.window.VdrSuiteHomeRecordingDiscovery;
  assert(publicApi && publicApi._test);

  function fireModuleClick(value) {
    selectedModule = value;
    const target = {
      dataset: {module: value},
      closest(selector) {
        const candidate = String(selector || '');
        if (candidate === '.module-tab[data-module], [data-brand-module]') return this;
        if (candidate.includes('overview') && value === 'overview') return this;
        return null;
      }
    };
    (documentListeners.click || []).forEach((handler) => handler({target}));
  }

  return {
    publicApi,
    api: publicApi._test,
    host,
    observers,
    genreCalls,
    genreListCalls,
    metadataResolvers,
    setModule(value) { selectedModule = value; },
    setBackend(value) { backendId = value; },
    setMetadataMode(value) { metadataMode = value; },
    fireModuleClick,
    fireHomeClick() {
      fireModuleClick('overview');
    },
    fireLatestObserver() {
      assert(observers.length > 0);
      observers[observers.length - 1].fire();
    },
    seriesCalls(requestBackendId) {
      return genreCalls.filter((call) =>
        call.genreId === 'series' &&
        (!requestBackendId || call.backendId === requestBackendId));
    }
  };
}

async function flush(turns) {
  for (let index = 0; index < (turns || 4); index += 1) {
    await Promise.resolve();
  }
  await new Promise((resolve) => setImmediate(resolve));
}

async function proveInFlightCoalescing() {
  const harness = createHarness('deferred');
  const first = harness.api.refreshForHome();
  const second = harness.api.refreshForHome();
  assert.strictEqual(first, second);

  await flush(2);
  assert.strictEqual(harness.seriesCalls('default').length, 1);
  assert.strictEqual(harness.metadataResolvers.length, 1);
  harness.metadataResolvers.shift()({available: false});
  assert.strictEqual(await first, true);
  assert.strictEqual(await second, true);
  assert.strictEqual(harness.api.seriesWarm('default'), true);
}

async function proveWarmProductionReturnAndForcedRefresh() {
  const harness = createHarness('available-false');
  assert.strictEqual(await harness.api.refreshForHome(), true);
  assert.strictEqual(harness.seriesCalls('default').length, 1);
  assert.strictEqual(harness.api.seriesWarm('default'), true);

  const initialSeriesSection = findRail(harness.host, 'series');
  assert(initialSeriesSection);

  harness.fireModuleClick('recordings2');
  harness.fireModuleClick('overview');
  harness.fireLatestObserver();
  await flush();

  assert.strictEqual(harness.seriesCalls('default').length, 1);
  assert.strictEqual(findRail(harness.host, 'series'), initialSeriesSection);

  assert.strictEqual(await harness.publicApi.refresh(), true);
  assert.strictEqual(harness.seriesCalls('default').length, 2);
  assert.strictEqual(harness.api.seriesWarm('default'), true);

  harness.setBackend('secondary');
  harness.fireHomeClick();
  harness.fireLatestObserver();
  await flush();
  assert.strictEqual(harness.seriesCalls('secondary').length, 1);
  assert.strictEqual(harness.api.seriesWarm('secondary'), true);
}

async function proveInterruptedMetadataNeverWarms() {
  const harness = createHarness('deferred');
  const first = harness.api.refreshForHome();
  await flush(2);
  assert.strictEqual(harness.metadataResolvers.length, 1);

  harness.fireModuleClick('recordings2');
  harness.metadataResolvers.shift()({available: false});
  await Promise.resolve();
  harness.fireModuleClick('overview');
  assert.strictEqual(await first, true);

  assert.strictEqual(
    harness.api.seriesWarm('default'),
    false,
    'metadata resolved during a production-style Home exit must not become a warm Series projection'
  );

  harness.setMetadataMode('available-false');
  assert.strictEqual(await harness.api.refreshForHome(), true);
  assert.strictEqual(harness.seriesCalls('default').length, 2);
}

async function proveMetadataErrorNeverWarms() {
  const harness = createHarness('reject');
  assert.strictEqual(await harness.api.refreshForHome(), true);
  assert.strictEqual(harness.api.seriesWarm('default'), false);
  assert.strictEqual(harness.seriesCalls('default').length, 1);

  harness.setMetadataMode('available-false');
  assert.strictEqual(await harness.api.refreshForHome(), true);
  assert.strictEqual(harness.seriesCalls('default').length, 2);
  assert.strictEqual(harness.api.seriesWarm('default'), true);
}

(async function () {
  await proveInFlightCoalescing();
  await proveWarmProductionReturnAndForcedRefresh();
  await proveInterruptedMetadataNeverWarms();
  await proveMetadataErrorNeverWarms();
  console.log('post-Phase-66 Series discovery reuse lifecycle contract passed');
}()).catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
