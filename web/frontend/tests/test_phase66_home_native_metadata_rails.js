'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = [
  fs.readFileSync(path.join(frontendRoot, 'platform', 'helpers.js'), 'utf8'),
  fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8')
].join('\n');

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
    this.src = '';
    this.alt = '';
    this.loading = '';
    this.type = '';
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }

  appendChild(child) {
    if (!child) return child;
    if (child.parentNode) child.remove();
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
    if (child.parentNode) child.remove();
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
    if (!rail) return null;
    return findElement(this, (element) =>
      element.attributes['data-home-discovery-rail'] === rail[1]);
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

function findElements(root, predicate, found) {
  const result = found || [];
  if (!root) return result;
  for (const child of root.children || []) {
    if (predicate(child)) result.push(child);
    findElements(child, predicate, result);
  }
  return result;
}

function findRail(host, key) {
  return host.querySelector('[data-home-discovery-rail="' + key + '"]');
}

function findSeriesCards(root) {
  return findElements(root, (element) =>
    element.dataset && Boolean(element.dataset.seriesKey));
}

function findRecordingCard(root, recordingId) {
  return findElement(root, (element) =>
    element.dataset && element.dataset.recordingId === recordingId);
}

function findImage(root) {
  return findElement(root, (element) => element.tagName === 'IMG');
}

function hasText(root, expected) {
  return Boolean(findElement(root, (element) => element.textContent === expected));
}

function makeRecording(id, nativeId, pathValue, title, posterUrl) {
  return {
    recordingId: id,
    backendId: 'default',
    backendNativeId: nativeId,
    path: pathValue,
    title: title,
    metadata: {
      provider: {},
      presentation: {posterUrl: posterUrl || ''},
      artwork: {preferredUrl: posterUrl || ''}
    }
  };
}

function richMovie(title, nativeId) {
  return {
    available: true,
    mediaType: 'movie',
    provider: 'tvscraper',
    providerId: 9001,
    title: title,
    preferredArtwork: {
      available: true,
      url: '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=' +
        encodeURIComponent(nativeId) + '&kind=preferred&index=0'
    },
    images: [
      {
        orientation: 'landscape',
        image: {
          available: true,
          url: '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=' +
            encodeURIComponent(nativeId) + '&kind=image&index=0'
        }
      },
      {
        orientation: 'portrait',
        image: {
          available: true,
          url: '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=' +
            encodeURIComponent(nativeId) + '&kind=image&index=1'
        }
      }
    ]
  };
}

function createHarness() {
  const host = new FakeElement('div');
  let selectedModule = 'overview';
  let activeClient = null;

  const document = {
    readyState: 'loading',
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
  context.window.window = context.window;
  context.window.document = document;
  context.window.setTimeout = function (callback) {
    callback();
    return 1;
  };
  context.window.selectModule = function (moduleName) {
    selectedModule = moduleName;
    return true;
  };
  context.window.VdrSuiteHomeLivePreview = {cancel() {}};
  context.window.VdrSuiteRecordings2 = {openRecording() {}, openFolder() {}};
  context.window.VdrSuiteGenres = {openRecordingGenre() { return true; }};
  context.window.VdrSuitePlatform = {
    getSelectedBackendId() { return 'default'; },
    getSelectedModule() { return selectedModule; },
    getClientApi() { return activeClient; }
  };

  vm.createContext(context);
  vm.runInContext(source, context, {filename: 'phase66-home-native-metadata-rails.js'});

  return {
    publicApi: context.window.VdrSuiteHomeRecordingDiscovery,
    api: context.window.VdrSuiteHomeRecordingDiscovery._test,
    host,
    setClient(client) {
      activeClient = client;
    }
  };
}

async function proveSeriesDoesNotFlashWeakProjection(harness) {
  const recordings = [
    makeRecording(
      'sgu-1',
      'native-sgu-1',
      'Serien/Stargate#3A_Universe/2016-01-05.10.52.3-0.rec',
      'Stargate#3A_Universe/2016-01-05.10.52.3-0',
      '/weak/stargate-episode-1.jpg'
    ),
    makeRecording(
      'sgu-2',
      'native-sgu-2',
      'Serien/Stargate#3A_Universe/2016-01-06.10.52.3-0.rec',
      'Stargate#3A_Universe/2016-01-06.10.52.3-0',
      '/weak/stargate-episode-2.jpg'
    )
  ];
  const client = {
    fetchClientRecordings() {
      return Promise.resolve({recordings: []});
    },
    fetchClientGenres() {
      return Promise.resolve({
        genres: [{id: 'series', label: 'Serien', count: recordings.length}]
      });
    },
    fetchClientGenreRecordings() {
      return Promise.resolve({
        recordings,
        total: recordings.length,
        hasMore: false
      });
    },
    fetchClientRecordingFolder() {
      return Promise.resolve({
        folders: [],
        recordings: [],
        recordingCount: 0
      });
    },
    requestJson() {
      return new Promise(function () {});
    }
  };

  harness.setClient(client);
  let settled = false;
  harness.publicApi.refresh().then(function () {
    settled = true;
  });

  await new Promise((resolve) => setImmediate(resolve));
  assert.strictEqual(settled, false);
  const seriesRail = findRail(harness.host, 'series');
  assert(seriesRail);
  assert.strictEqual(findSeriesCards(seriesRail).length, 0);
  assert(hasText(seriesRail, 'Serien werden gruppiert …'));
  assert(!hasText(seriesRail, 'Stargate#3A_Universe'));
}

async function proveMetadataPublishesPerResponse(api) {
  const recordings = [];
  for (let index = 1; index <= 8; index += 1) {
    recordings.push(makeRecording(
      'progress-' + String(index),
      'native-progress-' + String(index),
      'Serien/Progress/2016-01-' + String(index).padStart(2, '0') + '.rec',
      'Progress raw ' + String(index),
      ''
    ));
  }

  const calls = [];
  const snapshots = [];
  const client = {
    requestJson(route, request) {
      const nativeId = request.query.backendNativeId;
      calls.push({route, request});
      const index = Number(nativeId.split('-').pop());
      if (index > 4) return new Promise(function () {});
      return Promise.resolve({
        available: true,
        mediaType: 'series',
        provider: 'tvscraper',
        providerId: 7000,
        title: 'Progress',
        episodeName: 'Folge ' + String(index),
        seasonNumber: 1,
        episodeNumber: index,
        preferredArtwork: {available: false},
        images: []
      });
    }
  };

  let settled = false;
  api.fetchSeriesRecordingMetadata(
    client,
    recordings,
    'default',
    0,
    function (rich) {
      snapshots.push(rich.size);
    }
  ).then(function () {
    settled = true;
  });

  await new Promise((resolve) => setImmediate(resolve));
  assert.strictEqual(settled, false);
  assert.strictEqual(calls.length, 8);
  assert(snapshots.length >= 1);
  assert.strictEqual(snapshots[snapshots.length - 1], 4);
  assert(calls.every((call) => call.route === '/api/vdr/recordings/metadata'));
  assert(calls.every((call) => call.request.cache === 'no-store'));
  assert(calls.every((call) => call.request.credentials === 'same-origin'));
}

async function proveRandomGenreUsesNativeMetadata(api, host) {
  const richRecording = makeRecording(
    'western-rich',
    'native-western-rich',
    'Western/Lone_Ranger',
    'Western/Lone Ranger',
    '/weak/lone-ranger-still.jpg'
  );
  const fallbackRecording = makeRecording(
    'western-fallback',
    'native-western-fallback',
    'Western/Ohne_Metadaten',
    'Western/Ohne Metadaten',
    '/weak/western-fallback.jpg'
  );
  const recordings = [richRecording, fallbackRecording];
  const metadataCalls = [];
  const canonical = richMovie('Lone Ranger', richRecording.backendNativeId);
  const portraitUrl = canonical.images[1].image.url;

  const client = {
    fetchClientGenreRecordings(request) {
      assert.strictEqual(request.backendId, 'default');
      assert.strictEqual(request.genreId, 'western');
      return Promise.resolve({
        recordings,
        total: recordings.length,
        hasMore: false
      });
    },
    requestJson(route, request) {
      metadataCalls.push({route, request});
      if (request.query.backendNativeId === richRecording.backendNativeId) {
        return Promise.resolve(canonical);
      }
      return Promise.resolve({available: false});
    }
  };

  assert.strictEqual(await api.loadRandomGenre(
    client,
    'default',
    0,
    {id: 'western', label: 'Western', count: recordings.length}
  ), true);
  await new Promise((resolve) => setImmediate(resolve));

  assert.strictEqual(metadataCalls.length, 2);
  assert(metadataCalls.every((call) => call.route === '/api/vdr/recordings/metadata'));
  assert(metadataCalls.every((call) => call.request.query.backend === 'default'));

  const randomGenre = findRail(host, 'random-genre');
  assert(randomGenre);
  const richCard = findRecordingCard(randomGenre, richRecording.recordingId);
  const fallbackCard = findRecordingCard(randomGenre, fallbackRecording.recordingId);
  assert(richCard);
  assert(fallbackCard);
  assert(hasText(richCard, 'Lone Ranger'));
  assert(!hasText(richCard, 'Western/Lone Ranger'));
  assert.strictEqual(findImage(richCard).src, portraitUrl);
  assert.notStrictEqual(findImage(richCard).src, '/weak/lone-ranger-still.jpg');
  assert(hasText(fallbackCard, 'Western/Ohne Metadaten'));
  assert.strictEqual(findImage(fallbackCard).src, '/weak/western-fallback.jpg');
}


async function proveMetadataStartsBeforePaginationCompletes(api) {
  const first = makeRecording('page-1', 'native-page-1', 'Serien/Overlap/one.rec', 'raw one', '');
  const second = makeRecording('page-2', 'native-page-2', 'Serien/Overlap/two.rec', 'raw two', '');
  let releaseSecondPage = null;
  let metadataCalls = 0;
  let settled = false;
  const client = {
    fetchClientGenreRecordings(request) {
      if (Number(request.offset || 0) === 0) {
        return Promise.resolve({recordings: [first], total: 2, hasMore: true});
      }
      return new Promise(function (resolve) {
        releaseSecondPage = function () {
          resolve({recordings: [second], total: 2, hasMore: false});
        };
      });
    },
    requestJson(route, request) {
      metadataCalls += 1;
      return Promise.resolve({
        available: true,
        mediaType: 'series',
        provider: 'tvscraper',
        providerId: 8000,
        title: 'Overlap',
        episodeName: request.query.backendNativeId,
        seasonNumber: 1,
        episodeNumber: metadataCalls,
        preferredArtwork: {available: false},
        images: []
      });
    }
  };

  const loading = api.fetchAllSeriesRecordings(client, 'default', 'series', 0, function () {})
    .then(function () { settled = true; });
  await new Promise((resolve) => setImmediate(resolve));
  assert.strictEqual(settled, false, 'second recording page must still be pending');
  assert.strictEqual(metadataCalls, 1, 'first page metadata must start before full pagination completes');
  assert.strictEqual(typeof releaseSecondPage, 'function');
  releaseSecondPage();
  await loading;
  assert.strictEqual(metadataCalls, 2);
}

async function proveMetadataDeduplicatesAcrossConsumers(api) {
  const recording = makeRecording('dedup', 'native-dedup', 'Serien/Dedup/one.rec', 'raw dedup', '');
  let calls = 0;
  let release = null;
  const client = {
    requestJson() {
      calls += 1;
      return new Promise(function (resolve) { release = resolve; });
    }
  };
  const first = api.fetchSeriesRecordingMetadata(client, [recording], 'default', 0);
  const second = api.fetchSeriesRecordingMetadata(client, [recording], 'default', 0);
  await new Promise((resolve) => setImmediate(resolve));
  assert.strictEqual(calls, 1, 'same backend/native id must have one in-flight request per generation');
  release({
    available: true,
    mediaType: 'series',
    provider: 'tvscraper',
    providerId: 8100,
    title: 'Dedup',
    episodeName: 'One',
    seasonNumber: 1,
    episodeNumber: 1,
    preferredArtwork: {available: false},
    images: []
  });
  const values = await Promise.all([first, second]);
  assert.strictEqual(values[0].has('native-dedup'), true);
  assert.strictEqual(values[1].has('native-dedup'), true);
}

(async function () {
  const seriesHarness = createHarness();
  assert(seriesHarness.publicApi);
  assert(seriesHarness.api);
  await proveSeriesDoesNotFlashWeakProjection(seriesHarness);

  const overlapHarness = createHarness();
  await proveMetadataStartsBeforePaginationCompletes(overlapHarness.api);

  const dedupHarness = createHarness();
  await proveMetadataDeduplicatesAcrossConsumers(dedupHarness.api);

  const progressHarness = createHarness();
  await proveMetadataPublishesPerResponse(progressHarness.api);

  const genreHarness = createHarness();
  await proveRandomGenreUsesNativeMetadata(genreHarness.api, genreHarness.host);

  console.log('phase66 Home native metadata rail regressions ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
