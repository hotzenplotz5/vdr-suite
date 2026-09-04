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
    const rail = String(selector || '').match(/^\[data-home-discovery-rail="([^"]+)"\]$/);
    if (rail) {
      return findElement(this, (element) =>
        element.attributes['data-home-discovery-rail'] === rail[1]);
    }
    if (selector === 'img') {
      return findElement(this, (element) => element.tagName === 'IMG');
    }
    return null;
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

function seriesRecording(backendId) {
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

function createHarness(firstMetadata) {
  const host = new FakeElement('div');
  const timers = [];
  let timerId = 0;
  let selectedModule = 'overview';
  let backendId = 'default';
  let metadataResponse = firstMetadata;
  let metadataCalls = 0;
  let recordingCalls = 0;
  let genreListCalls = 0;
  let seriesCalls = 0;
  let folderCalls = 0;

  const client = {
    fetchClientRecordings() {
      recordingCalls += 1;
      return Promise.resolve({recordings: []});
    },
    fetchClientGenres() {
      genreListCalls += 1;
      return Promise.resolve({
        genres: [
          {id: 'other', label: 'Andere', count: 1},
          {id: 'series', label: 'Serien', count: 1}
        ]
      });
    },
    fetchClientGenreRecordings(request) {
      if (request.genreId !== 'series') {
        return Promise.resolve({recordings: [], total: 0, hasMore: false});
      }
      seriesCalls += 1;
      return Promise.resolve({
        recordings: [seriesRecording(request.backendId)],
        total: 1,
        hasMore: false
      });
    },
    fetchClientRecordingFolder() {
      folderCalls += 1;
      return Promise.resolve({folders: [], recordings: [], recordingCount: 0});
    },
    requestJson(route, request) {
      assert.strictEqual(route, '/api/vdr/recordings/metadata');
      assert.strictEqual(request.query.backend, backendId);
      metadataCalls += 1;
      return Promise.resolve(metadataResponse);
    }
  };

  class FakeIntersectionObserver {
    observe() {}
    disconnect() {}
  }

  const document = {
    readyState: 'complete',
    head: null,
    querySelector(selector) {
      return selector === '[data-home-zone="additional-sections"]' ? host : null;
    },
    createElement(tagName) {
      return new FakeElement(tagName);
    },
    addEventListener() {},
    getElementById() { return null; }
  };

  const context = {window: {}, console};
  context.Math = Object.create(Math);
  context.Math.random = function () { return 0; };
  context.window.window = context.window;
  context.window.document = document;
  context.window.IntersectionObserver = FakeIntersectionObserver;
  context.window.setTimeout = function (callback, delay) {
    const entry = {id: ++timerId, callback, delay: Number(delay || 0), cancelled: false};
    timers.push(entry);
    return entry.id;
  };
  context.window.clearTimeout = function (id) {
    const entry = timers.find((candidate) => candidate.id === id);
    if (entry) entry.cancelled = true;
  };
  context.window.selectModule = function (moduleName) {
    selectedModule = moduleName;
    return true;
  };
  context.window.VdrSuiteFrontendHelpers = {
    recordingMetadataPosterUrl(value) {
      return value && value.preferredArtwork && value.preferredArtwork.available === true
        ? String(value.preferredArtwork.url || '')
        : '';
    }
  };
  context.window.VdrSuiteHomeLivePreview = {cancel() {}};
  context.window.VdrSuiteHomeRecordingDiscoveryBootstrap = {
    installMouseDrag() { return true; }
  };
  context.window.VdrSuiteRecordings2 = {
    openRecording() {},
    openFolder() {}
  };
  context.window.VdrSuitePlatform = {
    getSelectedBackendId() { return backendId; },
    getSelectedModule() { return selectedModule; },
    getClientApi() { return client; }
  };

  vm.createContext(context);
  vm.runInContext(source, context, {
    filename: 'post-phase66-series-metadata-artwork-completion.js'
  });

  const publicApi = context.window.VdrSuiteHomeRecordingDiscovery;
  assert(publicApi && publicApi._test);

  return {
    api: publicApi._test,
    host,
    setMetadata(value) { metadataResponse = value; },
    setModule(value) { selectedModule = value; },
    setBackend(value) { backendId = value; },
    metadataCalls() { return metadataCalls; },
    recordingCalls() { return recordingCalls; },
    genreListCalls() { return genreListCalls; },
    seriesCalls() { return seriesCalls; },
    folderCalls() { return folderCalls; },
    pendingRetryTimers() {
      return timers.filter((entry) => !entry.cancelled && entry.delay >= 60000);
    },
    runNextRetryTimer() {
      const entry = timers.find((candidate) =>
        !candidate.cancelled && candidate.delay >= 60000);
      assert(entry, 'expected one bounded Series metadata retry timer');
      entry.cancelled = true;
      entry.callback();
    }
  };
}

async function flush(turns) {
  for (let index = 0; index < (turns || 6); index += 1) {
    await Promise.resolve();
  }
  await new Promise((resolve) => setImmediate(resolve));
}

function seriesImage(harness) {
  const section = harness.host.querySelector('[data-home-discovery-rail="series"]');
  assert(section, 'Series rail must remain visible');
  return section.querySelector('img');
}

async function proveUnsettledMetadataReprojectsWithoutDiscoveryReload() {
  const harness = createHarness({
    available: false,
    status: 'not-found',
    settled: false
  });

  assert.strictEqual(await harness.api.refreshForHome(), true);
  await flush();
  assert.strictEqual(harness.metadataCalls(), 1);
  assert.strictEqual(harness.seriesCalls(), 1);
  assert.strictEqual(harness.api.seriesWarm('default'), false);
  assert.strictEqual(seriesImage(harness), null);
  assert.strictEqual(harness.pendingRetryTimers().length, 1);

  const baseline = {
    recordings: harness.recordingCalls(),
    genres: harness.genreListCalls(),
    series: harness.seriesCalls(),
    folders: harness.folderCalls()
  };

  harness.setMetadata({
    available: true,
    status: 'ready',
    provider: 'tvscraper',
    mediaType: 'series',
    providerId: 42,
    title: 'Testserie',
    episodeName: 'Pilot',
    seasonNumber: 1,
    episodeNumber: 1,
    preferredArtwork: {
      available: true,
      url: '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=default-native-series-1&kind=preferred&index=0'
    },
    images: []
  });
  harness.runNextRetryTimer();
  await flush();

  assert.strictEqual(harness.metadataCalls(), 2);
  assert.deepStrictEqual(
    {
      recordings: harness.recordingCalls(),
      genres: harness.genreListCalls(),
      series: harness.seriesCalls(),
      folders: harness.folderCalls()
    },
    baseline,
    'metadata completion must not restart Recording Discovery owners'
  );
  const image = seriesImage(harness);
  assert(image, 'completed TVScraper metadata must reproject Series artwork');
  assert(image.src.includes('/api/vdr/recordings/metadata/image?'));
  assert.strictEqual(harness.api.seriesWarm('default'), true);
  assert.strictEqual(harness.pendingRetryTimers().length, 0);
}

async function proveAuthoritativeNegativeDoesNotPoll() {
  const harness = createHarness({
    available: false,
    status: 'not-found',
    settled: true
  });

  assert.strictEqual(await harness.api.refreshForHome(), true);
  await flush();
  assert.strictEqual(harness.metadataCalls(), 1);
  assert.strictEqual(harness.api.seriesWarm('default'), true);
  assert.strictEqual(harness.pendingRetryTimers().length, 0);
}

async function proveHomeExitDoesNotOrphanUnsettledRetry() {
  const harness = createHarness({
    available: false,
    status: 'not-found',
    settled: false
  });

  assert.strictEqual(await harness.api.refreshForHome(), true);
  await flush();
  assert.strictEqual(harness.metadataCalls(), 1);
  assert.strictEqual(harness.pendingRetryTimers().length, 1);

  assert.strictEqual(
    await harness.api.openRecording(seriesRecording('default'), 'default'),
    true
  );
  assert.strictEqual(harness.pendingRetryTimers().length, 1);
  harness.runNextRetryTimer();
  await flush();
  assert.strictEqual(
    harness.metadataCalls(),
    1,
    'inactive Home must not issue Series metadata reads'
  );
  assert.strictEqual(
    harness.pendingRetryTimers().length,
    1,
    'Home exit must preserve one bounded retry opportunity for a later return'
  );

  harness.setModule('overview');
  harness.setMetadata({
    available: true,
    status: 'ready',
    provider: 'tvscraper',
    mediaType: 'series',
    providerId: 42,
    title: 'Testserie',
    episodeName: 'Pilot',
    seasonNumber: 1,
    episodeNumber: 1,
    preferredArtwork: {
      available: true,
      url: '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=default-native-series-1&kind=preferred&index=0'
    },
    images: []
  });
  harness.runNextRetryTimer();
  await flush();

  assert.strictEqual(harness.metadataCalls(), 2);
  assert(seriesImage(harness));
  assert.strictEqual(harness.api.seriesWarm('default'), true);
  assert.strictEqual(harness.pendingRetryTimers().length, 0);
}

async function proveBackendFenceStopsPendingRetry() {
  const harness = createHarness({
    available: false,
    status: 'not-found',
    settled: false
  });

  assert.strictEqual(await harness.api.refreshForHome(), true);
  await flush();
  assert.strictEqual(harness.pendingRetryTimers().length, 1);
  harness.setBackend('secondary');
  harness.runNextRetryTimer();
  await flush();
  assert.strictEqual(harness.metadataCalls(), 1);
  assert.strictEqual(harness.pendingRetryTimers().length, 0);
}

(async function () {
  await proveUnsettledMetadataReprojectsWithoutDiscoveryReload();
  await proveAuthoritativeNegativeDoesNotPoll();
  await proveHomeExitDoesNotOrphanUnsettledRetry();
  await proveBackendFenceStopsPendingRetry();
  console.log('post-Phase-66 Series metadata/artwork completion contract passed');
}()).catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
