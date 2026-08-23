'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

let renderDetailCount = 0;
let enhanceCount = 0;
const detailRoot = {dataset: {}};
const mountTarget = {
  querySelector(selector) {
    assert.strictEqual(selector, '.recordings2-detail');
    return detailRoot;
  },
  classList: {
    remove() {}
  }
};

const modules = new Map();
const shared = {
  PAGE_SIZE: 100,
  recordingPathTitle() { return ''; },
  normalizePath(value) { return String(value || '').replace(/^\/+|\/+$/g, ''); },
  selectedBackendId() { return 'default'; },
  mountTarget() { return mountTarget; },
  clientApi() { return null; },
  folderList() { return []; },
  recordingList() { return []; },
  number(value, fallback) {
    const result = Number(value);
    return Number.isFinite(result) ? result : Number(fallback || 0);
  },
  first(value, keys, fallback) {
    const source = value && typeof value === 'object' ? value : {};
    for (const key of keys || []) {
      if (source[key] !== undefined && source[key] !== null) return source[key];
    }
    return fallback;
  },
  platform() {
    return {
      registerModule(name, api) { modules.set(name, api); },
      hasModule(name) { return modules.has(name); }
    };
  },
  decodeDisplayText(value) { return String(value || ''); },
  recordingNativeTitle() { return ''; },
  recordingMetadataTitle() { return ''; },
  recordingTitle() { return ''; },
  recordingSubtitle() { return ''; },
  recordingSummary() { return ''; },
  recordingPosterUrl() { return ''; },
  formatDuration() { return ''; },
  formatSize() { return ''; }
};

const browserView = {
  create() {
    return {
      renderLoading() {},
      renderError() {},
      renderFolder() {},
      renderDetail() { renderDetailCount += 1; },
      destroy() {}
    };
  }
};

const document = {
  querySelector() { return null; },
  querySelectorAll() { return []; },
  getElementById() { return null; },
  addEventListener() {},
  createElement() {
    return {
      dataset: {},
      classList: {toggle() {}, contains() { return false; }},
      setAttribute() {},
      addEventListener() {},
      appendChild() {},
      append() {}
    };
  }
};

const window = {
  VdrSuiteRecordings2Shared: shared,
  VdrSuiteRecordings2FolderArtwork: null,
  VdrSuiteRecordings2BrowserView: browserView,
  VdrSuiteRecordings2Playback: Object.freeze({
    createPanel() {}
  }),
  VdrSuiteRecordings2MetadataDetail: Object.freeze({
    enhance(root, recording, backendId) {
      enhanceCount += 1;
      assert.strictEqual(root, detailRoot);
      assert.strictEqual(recording.id, 'recording-42');
      assert.strictEqual(backendId, 'default');
      root.dataset.recordings2MetadataDetail = 'true';
    }
  }),
  setTimeout(callback) { callback(); }
};
window.window = window;

const context = vm.createContext({
  window,
  document,
  console,
  Date,
  Object,
  String,
  Number,
  Array,
  Promise,
  Math
});

vm.runInContext(
  fs.readFileSync('web/frontend/recordings2.js', 'utf8'),
  context,
  {filename: 'recordings2.js'}
);

const runtime = window.VdrSuiteRecordings2;
assert.ok(runtime);
runtime.openRecording({id: 'recording-42', backendId: 'default'});
assert.strictEqual(renderDetailCount, 1);

runtime.refreshDetailAddon();
assert.strictEqual(
  renderDetailCount,
  1,
  'initial lazy metadata addon attachment must not re-render the recording detail'
);
assert.strictEqual(enhanceCount, 1);
assert.strictEqual(detailRoot.dataset.recordings2MetadataDetail, 'true');

runtime.refreshDetailAddon();
assert.strictEqual(
  renderDetailCount,
  2,
  'later explicit metadata refresh must keep the established full-render behavior'
);
assert.strictEqual(enhanceCount, 1);

console.log('recordings2 lazy detail addon preserves active playback owner');
