'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

function first(object, keys, fallback) {
  for (const key of keys) {
    if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
      return object[key];
    }
  }
  return fallback;
}

function element() {
  return {
    id: '',
    className: '',
    dataset: {},
    style: {},
    children: [],
    classList: {
      values: new Set(),
      add(value) { this.values.add(value); },
      contains(value) { return this.values.has(value); }
    },
    setAttribute() {},
    appendChild(child) { this.children.push(child); return child; },
    append() { this.children.push(...arguments); },
    addEventListener() {}
  };
}

const document = {
  head: {appendChild() {}},
  getElementById() { return null; },
  createElement() { return element(); }
};

const shared = {
  first,
  decodeDisplayText(value) { return String(value || '').replace(/_/g, ' '); },
  recordingTitle(recording) { return String(first(recording, ['title'], 'Aufnahme')); },
  selectedBackendId() { return 'default'; },
  clientApi() { return null; },
  PAGE_SIZE: 50,
  folderList(data) { return data && Array.isArray(data.folders) ? data.folders : []; },
  recordingList(data) { return data && Array.isArray(data.recordings) ? data.recordings : []; },
  node() { return element(); },
  createButton() { return element(); }
};

const window = {
  VdrSuiteRecordings2Shared: shared,
  setTimeout() {},
  confirm() { return true; }
};

const context = vm.createContext({
  window,
  document,
  console,
  Object,
  String,
  Number,
  Array,
  Promise,
  Set
});

[
  'web/frontend/recordings2-folder-artwork.js',
  'web/frontend/recordings2-actions.js'
].forEach(path => {
  vm.runInContext(fs.readFileSync(path, 'utf8'), context, {filename: path});
});

assert.ok(window.VdrSuiteRecordings2FolderArtwork);
assert.ok(window.VdrSuiteRecordings2Actions);

const genre = window.VdrSuiteRecordings2FolderArtwork;
assert.strictEqual(genre.normalizeName('Science-Fiction'), 'sciencefiction');
assert.strictEqual(genre.forFolderName('Action').slug, 'action');
assert.strictEqual(genre.forFolderName('Fantasy').sprite, '100% 0%');
assert.strictEqual(genre.forFolderName('Unsortiert'), null);
const actionArtwork = genre.create({name: 'Action'});
assert.strictEqual(actionArtwork.dataset.genre, 'action');
assert.ok(actionArtwork.style.backgroundImage.includes('recording-genre-action.svg'));

const test = window.VdrSuiteRecordings2Actions.__test;
assert.strictEqual(test.normalizeFolderPath(' Filme\\Archiv '), 'Filme/Archiv');
assert.strictEqual(test.targetFolderPath('/'), '');
assert.strictEqual(test.localTitle({title: 'Drama/Tatort'}), 'Tatort');

const recording = {
  recordingId: 'default:4711',
  title: 'Drama/Tatort',
  path: '/srv/vdr/video/Drama/Tatort/2026-01-01.20.15.1-0.rec',
  backendNativeId: '/srv/vdr/video/Drama/Tatort/2026-01-01.20.15.1-0.rec',
  startTime: 1767294900,
  durationSeconds: 5400
};
const expected = test.identity(recording);
assert.strictEqual(test.candidateMatches(Object.assign({}, recording), expected), true);
assert.strictEqual(test.candidateMatches(Object.assign({}, recording, {
  backendNativeId: '/srv/vdr/video/Andere/2026-01-01.20.15.1-0.rec'
}), expected), true);

const payload = test.actionPayload(recording, 'default', 'RENAME', {
  dryRun: false,
  newName: 'Tatort neu'
});
assert.strictEqual(payload.action, 'RENAME');
assert.strictEqual(payload.backendId, 'default');
assert.strictEqual(payload.dryRun, false);
assert.strictEqual(payload.newName, 'Tatort neu');
assert.strictEqual(payload.backendNativeId, recording.backendNativeId);

assert.strictEqual(test.isDryRunReady({
  success: false,
  message: 'dry-run backend execution skipped',
  warnings: ['dry-run only'],
  errors: []
}), true);
assert.strictEqual(test.isDryRunReady({success: true}), false);

console.log('recordings2 actions and genre runtime ok');
