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
  let seriesMode = 'normal';

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
      if (seriesMode === 'reject') return Promise.reject(new Error('Series transport unavailable'));
      if (seriesMode === 'empty') return Promise.resolve({recordings: [], total: 0, hasMore: false});
      const recording = makeSeriesRecording(request.backendId);
      const recordings = metadataMode === 'deferred-tail' ? [recording, Object.assign({}, recording, {
        recordingId: recording.recordingId + '-tail',
        backendNativeId: recording.backendNativeId + '-tail',
        path: 'Serien/Testserie/S01E02 Second',
        title: 'Serien/Testserie/S01E02 Second'
      })] : [recording];
      return Promise.resolve({
        recordings,
        total: recordings.length,
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
      if (metadataMode === 'deferred' ||
          (metadataMode === 'deferred-tail' && request.query.backendNativeId.endsWith('-tail'))) {
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
    setSeriesMode(value) { seriesMode = value; },
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

async function proveCompletionCoalescing() {
  const harness = createHarness('deferred-tail');
  assert.strictEqual(await harness.api.refreshForHome(), true);
  await flush();
  assert.strictEqual(harness.metadataResolvers.length, 1, 'tail metadata is still pending after first visible refresh');
  assert.strictEqual(harness.api.seriesWarm('default'), false);
  const continued = harness.api.refreshForHome();
  await flush();
  assert.strictEqual(harness.seriesCalls('default').length, 1, 'same-generation Home scheduling coalesces with unfinished metadata completion');
  assert.strictEqual(harness.metadataResolvers.length, 1, 'coalescing does not duplicate the pending native read');
  harness.metadataResolvers.shift()({available: false});
  assert.strictEqual(await continued, true);
  assert.strictEqual(harness.api.seriesWarm('default'), true);
}

async function proveCompletionCoalescingFences() {
  for (const transition of ['explicit-refresh', 'home-exit', 'backend-change']) {
    const harness = createHarness('deferred-tail');
    await harness.api.refreshForHome();
    await flush();
    assert.strictEqual(harness.metadataResolvers.length, 1);
    harness.setMetadataMode('available-false');
    if (transition === 'home-exit') {
      harness.fireModuleClick('recordings2');
      harness.setModule('overview');
    }
    if (transition === 'backend-change') harness.setBackend('secondary');
    await (transition === 'explicit-refresh' ? harness.publicApi.refresh() : harness.api.refreshForHome());
    await flush();
    assert.strictEqual(harness.seriesCalls().length, 2, transition + ' must start a fresh generation');
    const expectedBackend = transition === 'backend-change' ? 'secondary' : 'default';
    assert.strictEqual(harness.api.seriesWarm(expectedBackend), true);
    const section = findRail(harness.host, 'series');
    const rail = findElement(section, element => element.className === 'media-home-discovery-rail series');
    const card = rail.children[0];
    harness.metadataResolvers.shift()({available: false});
    await flush();
    assert.strictEqual(rail.children[0], card, 'stale completion must not replace current UI');
    assert.strictEqual(card.dataset.backendId, expectedBackend);
    assert.strictEqual(harness.api.seriesWarm(expectedBackend), true);
  }
}

async function proveRevalidationRetainsSelectionAndHandlesFailure() {
  const harness = createHarness('available-false');
  await harness.api.refreshForHome();
  await flush();
  const section = findRail(harness.host, 'series');
  const list = findElement(section, element => element.className === 'media-home-discovery-rail series');
  list.children[0].listeners.click[0]();
  const seasons = findElement(section, element => element.className === 'media-home-series-season-rail');
  seasons.children[0].listeners.click[0]();
  const episodes = findElement(section, element => element.className === 'media-home-discovery-rail series-episodes');
  episodes.scrollLeft = 280;
  harness.setMetadataMode('deferred');
  const pending = harness.publicApi.refresh();
  await flush();
  assert.strictEqual(episodes.parentNode, section, 'pending explicit revalidation keeps selected detail mounted');
  harness.metadataResolvers.shift()({available:false});
  await pending;
  await flush();
  assert.strictEqual(episodes.parentNode, section);
  assert.strictEqual(episodes.scrollLeft, 280);
  assert(seasons.children[0].className.includes(' selected'));
  harness.setSeriesMode('reject');
  await harness.publicApi.refresh();
  assert.strictEqual(episodes.parentNode, section, 'failed scan keeps valid detail visible');
  assert.strictEqual(harness.api.seriesWarm('default'), false, 'failure never certifies retained UI warm');
  harness.setSeriesMode('empty');
  await harness.publicApi.refresh();
  assert.strictEqual(findRail(harness.host, 'series'), null, 'authoritative empty scan removes stale Series UI');
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

  const seriesRail = findElement(initialSeriesSection, element => element.className === 'media-home-discovery-rail series');
  const seriesCard = seriesRail.children[0];
  seriesRail.scrollLeft = 280;
  harness.setMetadataMode('deferred');
  const revalidation = harness.publicApi.refresh();
  await flush();
  assert.strictEqual(seriesRail.parentNode, initialSeriesSection, 'revalidation preserves valid Series UI while metadata is pending');
  assert.strictEqual(seriesRail.children[0], seriesCard);
  assert.strictEqual(seriesRail.scrollLeft, 280);
  harness.metadataResolvers.shift()({available: false});
  assert.strictEqual(await revalidation, true);
  await flush();
  assert.strictEqual(seriesRail.parentNode, initialSeriesSection, 'unchanged completed revalidation preserves rail identity');
  assert.strictEqual(seriesRail.children[0], seriesCard);
  harness.setMetadataMode('available-false');
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
  await proveCompletionCoalescing();
  await proveCompletionCoalescingFences();
  await proveRevalidationRetainsSelectionAndHandlesFailure();
  await proveWarmProductionReturnAndForcedRefresh();
  await proveInterruptedMetadataNeverWarms();
  await proveMetadataErrorNeverWarms();
  console.log('post-Phase-66 Series discovery reuse lifecycle contract passed');
}()).catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
