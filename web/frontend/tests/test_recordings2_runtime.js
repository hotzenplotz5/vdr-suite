'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const modules = new Map();
const document = {
  querySelector() { return null; },
  querySelectorAll() { return []; },
  getElementById() { return null; },
  addEventListener() {},
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
  VdrSuitePlatform: {
    registerModule(name, api) { modules.set(name, api); },
    hasModule(name) { return modules.has(name); },
    getSelectedBackendId() { return 'default'; },
    getMountTarget() { return null; },
    getClientApi() { return null; }
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

assert.ok(window.VdrSuiteRecordings2Shared);
assert.ok(window.VdrSuiteRecordings2FolderArtwork);
assert.ok(window.VdrSuiteRecordings2Actions);
assert.ok(window.VdrSuiteRecordings2BrowserView);
assert.ok(window.VdrSuiteRecordings2);
assert.strictEqual(modules.get('recordings2'), window.VdrSuiteRecordings2);
assert.strictEqual(window.VdrSuiteRecordingBrowser, undefined);

const test = window.VdrSuiteRecordings2.__test;
assert.strictEqual(test.normalizePath('/Serien//Tatort/'), 'Serien/Tatort');
assert.strictEqual(test.decodeDisplayText('Der#20Film_2026'), 'Der Film 2026');
assert.strictEqual(test.formatDuration(3660), '1 h 1 min');
assert.strictEqual(test.formatSize(1536), '1.5 GB');

const recording = {
  title: 'Technischer_Titel',
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

assert.strictEqual(test.recordingNativeTitle(recording), 'Technischer Titel');
assert.strictEqual(test.recordingMetadataTitle(recording), 'Die Serie');
assert.strictEqual(test.recordingTitle(recording), 'Technischer Titel');
assert.strictEqual(test.recordingTitle(Object.assign({}, recording, {
  title: 'Drama/Neuer_Name'
})), 'Neuer Name');
assert.strictEqual(test.recordingTitle({
  title: '',
  metadata: recording.metadata
}), 'Die Serie');
assert.strictEqual(test.recordingSubtitle(recording), 'S02E04 · Der Fall');
assert.strictEqual(test.recordingSummary(recording), 'Darstellungstext');
assert.strictEqual(test.recordingPosterUrl(recording), '/api/recordings/artwork?id=poster');

assert.throws(() => test.applyFolderData({recordingFolder: false}, false), /gültigen Aufnahmeordner/);
test.applyFolderData({
  recordingFolder: true,
  path: 'Serien/Tatort',
  parentPath: 'Serien',
  folders: [],
  recordings: [recording],
  recordingCount: 1,
  returnedCount: 1
}, false);

console.log('recordings2 modular runtime ok');