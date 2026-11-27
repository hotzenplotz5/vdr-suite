'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(path.join(__dirname, '..', 'home-continue-watching.js'), 'utf8');
const clientApiSource = fs.readFileSync(path.join(__dirname, '..', 'api', 'client-api.js'), 'utf8');

function runtime(overrides) {
  const window = {};
  const document = {
    readyState: 'loading',
    addEventListener() {},
    querySelector() { return null; }
  };
  const context = {
    window,
    document,
    fetch: async () => ({ok: true, json: async () => ({items: []})}),
    console,
    setTimeout,
    clearTimeout,
    Promise,
    Object,
    String,
    Number,
    Array,
    Boolean,
    JSON
  };
  Object.assign(window, overrides || {});
  window.window = window;
  window.document = document;
  window.fetch = context.fetch;
  vm.createContext(context);
  vm.runInContext(source, context, {filename: 'home-continue-watching.js'});
  return {window, api: window.VdrSuiteHomeContinueWatching};
}

function clientApiRuntime(fetchHandler) {
  const requests = [];
  const window = {};
  const context = {
    window,
    fetch: async (url, options) => {
      requests.push({url, options});
      return fetchHandler(url, options);
    },
    console,
    Promise,
    Object,
    String,
    Number,
    Array,
    Boolean,
    JSON,
    URLSearchParams
  };
  window.window = window;
  vm.createContext(context);
  vm.runInContext(clientApiSource, context, {filename: 'client-api.js'});
  return {api: window.VdrSuiteClientApi, requests};
}

function jsonResponse(status, payload) {
  return {
    ok: status >= 200 && status < 300,
    status,
    text: async () => JSON.stringify(payload)
  };
}

const backendNativeId = '/srv/vdr/video/Deferred/2026-08-30.07.00.00-0.rec';
const item = {
  backendId: 'default',
  recordingId: 'rec-deferred',
  backendNativeId,
  title: 'Deferred Recording',
  resumePositionSeconds: 93,
  progressRevision: 2,
  durationKnown: true,
  durationSeconds: 600,
  recording: {
    id: 'rec-deferred',
    recordingId: 'rec-deferred',
    backendId: 'default',
    backendNativeId,
    title: 'Deferred Recording',
    path: 'Filme/Deferred Recording',
    recordingPath: 'Filme/Deferred Recording',
    startTime: '2026-08-30T07:00:00Z',
    durationSeconds: 600,
    sizeMb: 1536,
    metadata: {
      native: {
        eventTitle: 'Deferred Recording',
        shortText: 'Native subtitle',
        description: 'Native description'
      },
      provider: {
        available: true,
        source: 'restfulapi-scraper-bridge',
        contentKind: 'movie',
        title: 'Deferred Recording',
        genreText: 'Drama',
        releaseDate: '1993-04-07',
        rating: 7.2
      },
      presentation: {
        title: 'Deferred Recording',
        subtitle: 'Native subtitle',
        summary: 'Native description',
        posterUrl: '/recording-artwork/default/deferred'
      }
    }
  }
};

(async function () {
  let previewCancels = 0;
  let loads = 0;
  const playbackLoads = [];
  const opened = [];
  const restartClears = [];
  const events = [];
  const first = runtime({
    VdrSuiteHomeLivePreview: {
      cancel(reason) {
        previewCancels += 1;
        assert.strictEqual(reason, 'Continue Watching geöffnet');
      }
    }
  });
  first.window.VdrSuiteDeferredFrontendRuntimes = {
    loadRecordings2() {
      loads += 1;
      first.window.VdrSuiteRecordings2 = {
        openRecording(recording, options) {
          opened.push({recording, options});
          events.push('open:' + options.playbackStartPositionSeconds);
        }
      };
      return Promise.resolve();
    }
  };
  first.window.loadVdrSuiteDeferredRuntime = function (id, src) {
    playbackLoads.push({id, src});
    events.push('load:' + src);
    if (src === '/frontend/api/session-frontend-sync.js') {
      first.window.VdrSuiteRecordingFastPlayback = {};
      first.window.VdrSuiteLivePlayback = {};
      return Promise.resolve();
    }
    if (src === '/frontend/recordings2-playback.js') {
      first.window.VdrSuiteRecordings2Playback = {createPanel() {}};
      first.window.VdrSuiteContinueWatchingSync = {
        clear(backendId, recordingId, expectedRevision) {
          restartClears.push({backendId, recordingId, expectedRevision});
          events.push('clear:' + backendId + ':' + recordingId);
          return Promise.resolve(true);
        }
      };
      return Promise.resolve();
    }
    return Promise.reject(new Error('unexpected deferred runtime ' + src));
  };

  assert.strictEqual(await first.api._test.openItem(item, true), true);
  assert.strictEqual(loads, 1, 'fresh Home must load the canonical deferred Recordings runtime');
  assert.deepStrictEqual(playbackLoads.map(entry => entry.src), [
    '/frontend/api/session-frontend-sync.js',
    '/frontend/recordings2-playback.js'
  ], 'fresh Home must await the canonical Recording playback runtime before opening Continue Watching');
  assert.strictEqual(previewCancels, 1, 'active/pending preview intent must be released before Recording playback');
  assert.strictEqual(opened.length, 1);
  assert.strictEqual(opened[0].recording.id, 'rec-deferred');
  assert.strictEqual(opened[0].recording.backendId, 'default');
  assert.strictEqual(
    opened[0].recording.backendNativeId,
    item.backendNativeId,
    'Continue Watching must preserve the stable backend identity required by canonical Recording metadata'
  );
  assert.strictEqual(opened[0].recording.path, item.recording.path);
  assert.strictEqual(opened[0].recording.startTime, item.recording.startTime);
  assert.strictEqual(opened[0].recording.durationSeconds, item.recording.durationSeconds);
  assert.strictEqual(opened[0].recording.sizeMb, item.recording.sizeMb);
  assert.strictEqual(opened[0].recording.metadata.provider.genreText, 'Drama');
  assert.strictEqual(opened[0].recording.metadata.provider.releaseDate, '1993-04-07');
  assert.strictEqual(opened[0].recording.metadata.provider.rating, 7.2);
  assert.strictEqual(opened[0].recording.metadata.provider.source, 'restfulapi-scraper-bridge');
  assert.strictEqual(opened[0].options.autoStartPlayback, true);
  assert.strictEqual(opened[0].options.playbackStartPositionSeconds, 93);
  assert.strictEqual(opened[0].options.continueWatching, true);
  assert.strictEqual(restartClears.length, 0, 'Continue must preserve the saved resume state');
  assert.deepStrictEqual(events.slice(0, 3), [
    'load:/frontend/api/session-frontend-sync.js',
    'load:/frontend/recordings2-playback.js',
    'open:93'
  ], 'canonical playback composition must be ready before the Recording open intent');

  assert.strictEqual(await first.api._test.openItem(item, false), true);
  assert.strictEqual(loads, 1, 'already loaded Recordings runtime must not be loaded twice');
  assert.strictEqual(playbackLoads.length, 2, 'ready playback runtime must not be loaded twice');
  assert.strictEqual(opened.length, 2);
  assert.strictEqual(opened[1].recording.backendNativeId, item.backendNativeId);
  assert.strictEqual(opened[1].recording.path, item.recording.path);
  assert.strictEqual(opened[1].recording.metadata.provider.genreText, 'Drama');
  assert.strictEqual(opened[1].options.playbackStartPositionSeconds, 0);
  assert.deepStrictEqual(restartClears, [{
    backendId: 'default', recordingId: 'rec-deferred', expectedRevision: 2
  }]);
  assert.deepStrictEqual(events.slice(-2), [
    'clear:default:rec-deferred',
    'open:0'
  ], 'From beginning must clear the old resume truth before opening canonical playback at zero');

  const unavailable = runtime({
    VdrSuiteHomeLivePreview: {cancel() {}}
  });
  assert.strictEqual(await unavailable.api._test.openItem(item, true), false);

  let testOnlyCancels = 0;
  const testOnlyPreview = runtime({
    VdrSuiteHomeLivePreview: {
      __test: {
        cancelPreview() { testOnlyCancels += 1; }
      }
    },
    VdrSuiteRecordings2: {
      openRecording() {}
    },
    VdrSuiteRecordings2Playback: {
      createPanel() {}
    },
    VdrSuiteContinueWatchingSync: {
      clear() { return Promise.resolve(true); }
    }
  });
  assert.strictEqual(await testOnlyPreview.api._test.openItem(item, true), true);
  assert.strictEqual(testOnlyCancels, 0, 'production Continue Watching must not call preview __test hooks');

  const composed = clientApiRuntime(async url => {
    if (url === '/api/backends/default/snapshot') {
      return jsonResponse(200, {
        snapshotAvailable: true,
        channelCount: 12,
        eventCount: 24,
        timerCount: 3,
        recordingCount: 0
      });
    }
    if (url === '/api/vdr/recordings/cache/status?backend=default') {
      return jsonResponse(200, {
        backendId: 'default',
        state: 'ready',
        cacheReady: true,
        totalCount: 39
      });
    }
    return jsonResponse(404, {error: 'unexpected request ' + url});
  });

  const composedSnapshot = await composed.api.fetchClientBackendSnapshot('default', {
    cache: 'no-store'
  });
  assert.strictEqual(
    composedSnapshot.recordingCount,
    39,
    'Media Home snapshot adapter must project the authoritative recording-cache total'
  );
  assert.deepStrictEqual(
    composed.requests.map(request => request.url),
    [
      '/api/backends/default/snapshot',
      '/api/vdr/recordings/cache/status?backend=default'
    ],
    'Home snapshot adapter must read the matching backend cache truth without another VDR live read'
  );

  const fallback = clientApiRuntime(async url => {
    if (url === '/api/backends/default/snapshot') {
      return jsonResponse(200, {
        snapshotAvailable: true,
        recordingCount: 0
      });
    }
    if (url === '/api/vdr/recordings/cache/status?backend=default') {
      return jsonResponse(503, {error: 'recording cache unavailable'});
    }
    return jsonResponse(404, {error: 'unexpected request ' + url});
  });

  const fallbackSnapshot = await fallback.api.fetchClientBackendSnapshot('default', {
    cache: 'no-store'
  });
  assert.strictEqual(
    fallbackSnapshot.recordingCount,
    0,
    'recording-cache status failure must not make the existing backend snapshot unreadable'
  );

  // Exercise the actual App snapshot owner with the actual Client API wrapper.
  {
    const appSource = fs.readFileSync(path.join(__dirname, '..', 'app.js'), 'utf8');
    const begin = appSource.indexOf('function markHomeStartup(');
    const end = appSource.indexOf('function renderBackend(', begin);
    assert(begin >= 0 && end > begin);
    const statusReplies = [], snapshots = [], renders = [], frames = [], events = [];
    const galleryEvents = [];
    const marks = new Map();
    const metric = {firstElementChild: {textContent: ''}};
    const slow = clientApiRuntime(async url => {
      if (url.includes('/snapshot')) return new Promise(resolve => snapshots.push(resolve));
      if (url.includes('/recordings/cache/status')) return new Promise((resolve, reject) => statusReplies.push({resolve, reject}));
      throw Error('unexpected request');
    });
    const ctx = {
      window: {performance: {getEntriesByName(name) {return marks.has(name) ? [marks.get(name)] : [];},
        mark(name) {marks.set(name, {startTime: marks.size + 1});}},
        requestAnimationFrame(fn) {frames.push(fn);}},
      document: {visibilityState:'visible', getElementById(id) {return id === 'home-recording-count' ? metric : null;}},
      browserSessionFrontendActive: true,
      backendDetailSequence: 0, selectedBackend: null, selectedBackendId: '',
      selectedModule: 'overview', currentSnapshot: null,
      resetMediaImportVisibility() {}, markSelected(id) {ctx.selectedBackendId = id;},
      resetImageGalleryVisibility() {galleryEvents.push('reset');},
      refreshImageGalleryVisibility(id) {galleryEvents.push(id);},
      selectModule(name) {ctx.selectedModule = name; events.push(['home', snapshots.length]);}, refreshMediaImportVisibility() {},
      refreshDetailButton: {disabled: false}, detailMetaElement: {},
      detailDataElement: {replaceChildren() {}, getClientRects() {return [{}];}},
      frontendPlatformClientApi() {return slow.api;},
      renderSnapshotMetrics(data) {renders.push(data);},
      valueOrZero(value) {return Number(value) || 0;}, reportConnectionUnavailable() {}
    };
    vm.createContext(ctx);
    vm.runInContext(appSource.slice(begin, end), ctx);
    const flush = () => new Promise(resolve => setImmediate(resolve));
    const first = ctx.loadBackendDetails({backendId:'default'});
    assert.deepStrictEqual(galleryEvents, ['reset','default']);
    assert.equal(statusReplies.length, 1);
    assert.deepStrictEqual(events[0], ['home', 1], 'snapshot starts before Home-resume discovery');
    assert.equal(ctx.refreshDetailButton.disabled, false);
    assert.ok(marks.has('vdr-suite:startup:home-navigation-ready'));
    assert.ok(!marks.has('vdr-suite:startup:snapshot-ready'));
    snapshots.shift()(jsonResponse(200, {recordingCount:0, snapshotAvailable:true}));
    await flush();
    assert.equal(renders.length, 1);
    assert.equal(renders[0].recordingCountPending, true);
    assert.equal(ctx.refreshDetailButton.disabled, false);
    assert.ok(marks.has('vdr-suite:startup:home-controls-ready'));
    assert.ok(!marks.has('vdr-suite:startup:recording-count-settled'));
    frames.shift()(); frames.shift()();
    assert.ok(marks.has('vdr-suite:startup:home-frame'));
    statusReplies.shift().resolve(jsonResponse(200, {totalCount:39}));
    await first;
    assert.equal(ctx.currentSnapshot.recordingCount, 39);
    assert.equal(metric.firstElementChild.textContent, '39');
    assert.equal(renders.length, 1);
    const second = ctx.loadBackendDetails({backendId:'other'});
    snapshots.shift()(jsonResponse(200, {recordingCount:7}));
    await flush();
    ctx.selectedModule = 'epg';
    statusReplies.shift().reject(Error('cache unavailable'));
    await second;
    assert.equal(ctx.currentSnapshot.recordingCount, 7);
    assert.equal(renders.length, 2);
    assert.equal(metric.firstElementChild.textContent, '39');
    const old = ctx.loadBackendDetails({backendId:'old'});
    const newest = ctx.loadBackendDetails({backendId:'new'});
    snapshots.shift()(jsonResponse(200, {recordingCount:123}));
    statusReplies.shift().resolve(jsonResponse(200, {totalCount:456}));
    await old;
    assert.equal(renders.length, 2);
    snapshots.shift()(jsonResponse(200, {recordingCount:9}));
    await flush();
    assert.equal(renders.length, 3);
    ctx.browserSessionFrontendActive = false;
    ctx.currentSnapshot = null;
    statusReplies.shift().resolve(jsonResponse(200, {totalCount:999}));
    await newest;
    assert.equal(ctx.currentSnapshot, null);
    ctx.browserSessionFrontendActive = true;
    const fail = ctx.loadBackendDetails({backendId:'failed'});
    ctx.selectedModule = 'recordings';
    snapshots.shift()(jsonResponse(503, {error:'snapshot unavailable'}));
    statusReplies.shift().resolve(jsonResponse(200, {totalCount:5}));
    await fail;
    assert.equal(ctx.selectedModule, 'recordings');
    assert.equal(ctx.refreshDetailButton.disabled, false);
    assert.equal(renders.length, 3);
    const late = ctx.loadBackendDetails({backendId:'late'});
    ctx.selectedModule = 'channels';
    snapshots.shift()(jsonResponse(200, {recordingCount:8}));
    statusReplies.shift().resolve(jsonResponse(200, {totalCount:19}));
    await late;
    assert.equal(ctx.currentSnapshot.recordingCount, 19);
    assert.equal(renders.length, 3, 'late snapshot must not rebuild another module');

    // Exercise the real module selection owner independently of the network.
    const selectStart = appSource.indexOf('function selectModule(');
    const selectEnd = appSource.indexOf('function markSelected(', selectStart);
    const shortcutStart = appSource.indexOf('function renderMediaToolsHomeShortcut()');
    const shortcutEnd = appSource.indexOf('function resetMediaImportVisibility(', shortcutStart);
    assert(shortcutStart >= 0 && shortcutEnd > shortcutStart);
    const modules = [];
    Object.assign(ctx, {
      document:{querySelectorAll(){return [];}, querySelector(){return null;}},
      placeMediaImportTab(){}, placeImageGalleryTab(){}, clearImageGalleryHome(){}, closeHomeHevcStream(){},
      renderSelectedModule(data){modules.push([ctx.selectedModule, data]);},
      shouldPublishHomeResume(){return false;}
    });
    // Include the real adjacent Home shortcut owner; this navigation fixture
    // has no mounted Home zone, so its normal early return is exercised.
    vm.runInContext(appSource.slice(shortcutStart, shortcutEnd) + '\n' +
      appSource.slice(selectStart, selectEnd),ctx);
    ctx.currentSnapshot = null;
    ctx.selectModule('recordings');
    assert.deepStrictEqual(modules[0], ['recordings', null]);
    ctx.browserSessionFrontendActive = false;
    ctx.selectedBackend = null;
    ctx.selectModule('timers');
    assert.equal(modules.length, 1, 'no new pre-auth module read');
  }
  console.log('phase66 continue watching Home runtime composition ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
