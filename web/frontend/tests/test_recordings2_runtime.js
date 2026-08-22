'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const modules = new Map();
const deferredRuntimeLoads = [];
const documentListeners = Object.create(null);
let pipMini = null;
const document = {
  querySelector() { return null; },
  querySelectorAll() { return []; },
  getElementById(id) {
    if (id === 'vdr-suite-live-mini-player') return pipMini;
    return null;
  },
  addEventListener(name, callback) {
    if (!documentListeners[name]) documentListeners[name] = [];
    documentListeners[name].push(callback);
  },
  head: { appendChild() {} },
  createElement() {
    return {
      className: '',
      dataset: {},
      style: {},
      hidden: false,
      children: [],
      classList: { add() {}, remove() {}, contains() { return false; }, toggle() {} },
      setAttribute() {},
      appendChild(child) { this.children.push(child); return child; },
      append() { this.children.push(...arguments); },
      replaceChildren() { this.children = Array.from(arguments); },
      addEventListener() {},
      remove() {},
      querySelector() { return null; },
      insertBefore(child) { this.children.push(child); return child; }
    };
  }
};

const window = {
  setTimeout(callback) { callback(); },
  VdrSuitePublicUrl: {
    resolvePath(path) {
      const value = String(path || '');
      if (value === '/vdr-suite' || value.startsWith('/vdr-suite/')) return value;
      return '/vdr-suite' + value;
    }
  },
  VdrSuitePlatform: {
    registerModule(name, api) { modules.set(name, api); },
    hasModule(name) { return modules.has(name); },
    getSelectedBackendId() { return 'default'; },
    getMountTarget() { return null; },
    getClientApi() { return null; }
  },
  VdrSuiteRecordings2Playback: Object.freeze({
    createLivePanel() {}
  }),
  loadVdrSuiteDeferredRuntime(key, path, ready) {
    deferredRuntimeLoads.push({
      key,
      path,
      readyBefore: ready()
    });

    if (path === '/frontend/api/session-frontend-sync.js') {
      window.VdrSuiteRecordingFastPlayback = Object.freeze({});
      window.VdrSuiteLivePlayback = Object.freeze({createLivePanel() {}});
    } else if (path === '/frontend/recordings2-playback.js') {
      window.VdrSuiteRecordings2Playback = Object.freeze({
        createLivePanel() {},
        createPanel() {}
      });
    } else {
      throw new Error('Unexpected deferred runtime path: ' + path);
    }

    assert.strictEqual(ready(), true);
    return Promise.resolve();
  }
};

const context = vm.createContext({
  window,
  document,
  console,
  Date,
  Map,
  Object,
  String,
  Number,
  Math,
  Promise,
  Set,
  Array,
  parseInt
});

[
  'web/frontend/recordings2-shared.js',
  'web/frontend/recordings2-folder-artwork.js',
  'web/frontend/recordings2-actions.js',
  'web/frontend/recordings2-browser-view.js',
  'web/frontend/recordings2.js'
].forEach(path => {
  vm.runInContext(fs.readFileSync(path, 'utf8'), context, {filename: path});
});

async function run() {
  assert.ok(window.VdrSuiteRecordings2Shared);
  assert.ok(window.VdrSuiteRecordings2FolderArtwork);
  assert.ok(window.VdrSuiteRecordings2Actions);
  assert.ok(window.VdrSuiteRecordings2BrowserView);
  assert.ok(window.VdrSuiteRecordings2);
  assert.strictEqual(modules.get('recordings2'), window.VdrSuiteRecordings2);
  assert.strictEqual(window.VdrSuiteRecordingBrowser, undefined);

  const test = window.VdrSuiteRecordings2.__test;
  await test.ensurePlaybackRuntime();

  // Native PiP owns the visible video while active. The persistent Live mini
  // shell must disappear completely, then return only if the same video still
  // belongs to that mini shell when PiP is closed.
  const pipVideo = {};
  let pipVideoStillInMini = true;
  pipMini = {
    hidden: false,
    dataset: {},
    contains(candidate) {
      return pipVideoStillInMini && candidate === pipVideo;
    }
  };
  assert.strictEqual((documentListeners.enterpictureinpicture || []).length, 1);
  assert.strictEqual((documentListeners.leavepictureinpicture || []).length, 1);
  documentListeners.enterpictureinpicture[0]({target: pipVideo});
  assert.strictEqual(pipMini.hidden, true);
  assert.strictEqual(pipMini.dataset.vdrSuitePipSuppressed, 'true');
  documentListeners.leavepictureinpicture[0]({target: pipVideo});
  assert.strictEqual(pipMini.hidden, false);
  assert.strictEqual(pipMini.dataset.vdrSuitePipSuppressed, undefined);

  documentListeners.enterpictureinpicture[0]({target: pipVideo});
  assert.strictEqual(pipMini.hidden, true);
  pipVideoStillInMini = false;
  documentListeners.leavepictureinpicture[0]({target: pipVideo});
  assert.strictEqual(
    pipMini.hidden,
    true,
    'leaving PiP must not resurrect a mini shell after the video was reparented'
  );
  assert.strictEqual(pipMini.dataset.vdrSuitePipSuppressed, undefined);
  pipMini = null;

  assert.deepStrictEqual(deferredRuntimeLoads, [
    {
      key: 'vdr-suite-session-frontend-sync-runtime',
      path: '/frontend/api/session-frontend-sync.js',
      readyBefore: false
    },
    {
      key: 'vdr-suite-recordings2-playback-runtime',
      path: '/frontend/recordings2-playback.js',
      readyBefore: false
    }
  ]);
  assert.strictEqual(typeof test.ensurePlaybackRuntime, 'function');
  assert.strictEqual(typeof window.VdrSuiteRecordings2Playback.createLivePanel, 'function');
  assert.strictEqual(typeof window.VdrSuiteRecordings2Playback.createPanel, 'function');

  const httpPaths = fs.readFileSync('core/http/src/TestHttpServerPaths.inc', 'utf8');
  assert.ok(httpPaths.includes(
    '{"/frontend/api/session-frontend-sync.js", "api/session-frontend-sync.js",'
  ));

  const shared = window.VdrSuiteRecordings2Shared;
  assert.strictEqual(test.normalizePath('/Serien//Tatort/'), 'Serien/Tatort');
  assert.strictEqual(test.decodeDisplayText('Der#20Film_2026'), 'Der Film 2026');
  assert.strictEqual(test.formatDuration(3660), '1 h 1 min');
  assert.strictEqual(test.formatSize(1536), '1.5 GB');

  const recording = {
    title: 'Technischer_Titel',
    path: '/Serien/Tigeren_Club_gggg/2026-07-19.05.55.1-0.rec',
    backendNativeId: '/srv/vdr/video/Serien/Tigeren_Club_gggg/2026-07-19.05.55.1-0.rec',
    metadata: {
      provider: {
        seriesTitle: 'Die Serie',
        episodeTitle: 'Der Fall',
        overview: 'Provider-Zusammenfassung'
      },
      presentation: {
        title: 'Die Serie',
        subtitle: 'S02E04 · Der Fall',
        summary: 'Darstellungstext',
        posterUrl: '/api/recordings/artwork?id=poster',
        placeholderVariant: 4
      }
    }
  };

  assert.strictEqual(test.recordingPathTitle(recording), 'Tigeren Club gggg');
  assert.strictEqual(test.recordingNativeTitle(recording), 'Technischer Titel');
  assert.strictEqual(test.recordingMetadataTitle(recording), 'Die Serie');
  assert.strictEqual(test.recordingTitle(recording), 'Tigeren Club gggg');
  assert.strictEqual(test.recordingTitle({
    title: 'Drama/Neuer_Name',
    path: ''
  }), 'Neuer Name');
  assert.strictEqual(test.recordingTitle({
    title: '',
    path: '',
    metadata: recording.metadata
  }), 'Die Serie');
  assert.strictEqual(test.recordingSubtitle(recording), 'S02E04 · Der Fall');
  assert.strictEqual(test.recordingSummary(recording), 'Darstellungstext');
  assert.strictEqual(test.recordingPosterUrl(recording), '/api/recordings/artwork?id=poster');

  const poster = shared.createPoster(recording);
  assert.strictEqual(poster.children.length, 1);
  assert.strictEqual(
    poster.children[0].src,
    '/vdr-suite/api/recordings/artwork?id=poster'
  );

  const manualRecording = {
    title: 'Technischer_Titel',
    path: '/Action/Alter_Titel/2026-05-07.07.21.1-0.rec',
    backendNativeId: '/srv/vdr/video/Action/Alter_Titel/2026-05-07.07.21.1-0.rec',
    metadata: {
      provider: {
        source: 'manual',
        title: 'Face/Off – Im Körper des Feindes',
        overview: 'Manuell ausgewählte Beschreibung'
      },
      presentation: {
        title: 'Face/Off – Im Körper des Feindes',
        summary: 'Manuell ausgewählte Beschreibung',
        posterUrl: '/api/vdr/recordings/metadata/image?backend=default',
        placeholderVariant: 2
      },
      manualAssignment: {
        active: true,
        relationshipLocked: true,
        revision: 1
      }
    }
  };
  assert.strictEqual(
    test.recordingTitle(manualRecording),
    'Face/Off – Im Körper des Feindes'
  );
  assert.strictEqual(
    shared.createPoster(manualRecording).children[0].src,
    '/vdr-suite/api/vdr/recordings/metadata/image?backend=default'
  );

  const normalized = test.normalizeRecording(recording);
  assert.strictEqual(normalized.title, 'Tigeren Club gggg');
  assert.strictEqual(recording.title, 'Technischer_Titel');

  assert.throws(() => test.applyFolderData({recordingFolder: false}, false), /gültigen Aufnahmeordner/);
  test.applyFolderData({
    recordingFolder: true,
    path: 'Serien',
    parentPath: '',
    folders: [],
    recordings: [recording],
    recordingCount: 1,
    returnedCount: 1
  }, false);

  console.log('recordings2 modular runtime ok');
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});