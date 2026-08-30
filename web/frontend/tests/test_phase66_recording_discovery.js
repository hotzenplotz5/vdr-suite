'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8');

const transitions = [];
const openedRecordings = [];
const openedFolders = [];
const openedGenres = [];
const previewReasons = [];
const scheduled = [];

const context = {
  window: {},
  console
};
context.window.window = context.window;
context.window.document = null;
context.window.setTimeout = function (callback) {
  scheduled.push(callback);
  return scheduled.length;
};
context.window.selectModule = function (moduleName) {
  transitions.push(moduleName);
  return true;
};
context.window.VdrSuiteHomeLivePreview = {
  cancel(reason) {
    previewReasons.push(reason);
  }
};
context.window.VdrSuiteRecordings2 = {
  openRecording(recording, options) {
    openedRecordings.push({recording, options});
  },
  openFolder(folderPath) {
    openedFolders.push(folderPath);
  }
};
context.window.VdrSuiteGenres = {
  openRecordingGenre(entry, options) {
    openedGenres.push({entry, options});
    return true;
  }
};
context.window.VdrSuitePlatform = {
  getSelectedBackendId() { return 'default'; },
  getSelectedModule() { return 'overview'; },
  getClientApi() { return null; }
};

vm.createContext(context);
vm.runInContext(source, context);

const api = context.window.VdrSuiteHomeRecordingDiscovery;
assert(api && api._test);

const sameBackend = {
  recordingId: 'recording-1',
  backendId: 'default',
  title: 'Film Eins'
};
const inheritedBackend = {
  id: 'recording-2',
  title: 'Film Zwei'
};
const foreignBackend = {
  recordingId: 'recording-3',
  backendId: 'remote-b',
  title: 'Fremde Aufnahme'
};
const missingIdentity = {
  backendId: 'default',
  title: 'Ohne ID'
};

const canonical = api._test.canonicalRecordings({
  recordings: [sameBackend, inheritedBackend, foreignBackend, missingIdentity]
}, 'default');
assert.strictEqual(canonical.length, 2);
assert.strictEqual(canonical[0], sameBackend);
assert.strictEqual(canonical[1], inheritedBackend);

const genres = api._test.canonicalGenres({genres: [
  {id: 'movie', count: 3},
  {id: 'series', count: 1},
  {id: 'empty', count: 0},
  {id: '', count: 4}
]});
assert.strictEqual(genres.length, 2);
assert.strictEqual(genres[0].id, 'movie');
assert.strictEqual(genres[1].id, 'series');

const folders = api._test.canonicalFolders({folders: [
  {name: 'Filme', path: 'Filme'},
  {name: 'Serien', path: 'Serien'},
  {name: '', path: ''}
]});
assert.strictEqual(folders.length, 2);
assert.strictEqual(folders[0].path, 'Filme');
assert.strictEqual(folders[1].path, 'Serien');

const seriesRecording = {
  recordingId: 'series-1',
  backendId: 'default',
  metadata: {
    provider: {
      contentKind: 'series-episode',
      seriesId: 'series:42',
      seriesTitle: 'Beispielserie',
      episodeTitle: 'Pilot'
    },
    presentation: {
      posterUrl: '/api/vdr/recordings/metadata/image?kind=preferred'
    }
  }
};
assert.strictEqual(api._test.isSeriesRecording(seriesRecording), true);
assert.strictEqual(api._test.isSeriesRecording({
  metadata: {provider: {contentKind: 'movie', seriesTitle: 'Falsche Serie'}}
}), false);
assert.strictEqual(api._test.recordingPosterUrl(seriesRecording), '/api/vdr/recordings/metadata/image?kind=preferred');
assert.strictEqual(api._test.recordingPosterUrl({
  metadata: {artwork: {preferredUrl: '/cached/poster.jpg'}}
}), '/cached/poster.jpg');

(async function () {
  assert.strictEqual(await api._test.openRecording(seriesRecording, 'default'), true);
  assert.strictEqual(openedRecordings.length, 1);
  assert.strictEqual(openedRecordings[0].recording, seriesRecording);
  assert.strictEqual(openedRecordings[0].options.backendId, 'default');
  assert.strictEqual(openedRecordings[0].options.backLabel, '← Zurück zu Home');
  assert.strictEqual(typeof openedRecordings[0].options.onClose, 'function');
  assert.strictEqual(transitions[0], 'recordings2');

  assert.strictEqual(await api._test.openFolder({name: 'Filme', path: 'Filme'}, 'default'), true);
  assert.strictEqual(openedFolders.length, 1);
  assert.strictEqual(openedFolders[0], 'Filme');
  assert.strictEqual(transitions[1], 'recordings2');

  const genre = {id: 'movie', label: 'Film', count: 3};
  assert.strictEqual(await api._test.openGenre(genre, 'default'), true);
  assert.strictEqual(openedGenres.length, 1);
  assert.strictEqual(openedGenres[0].entry, genre);
  assert.strictEqual(openedGenres[0].options.backendId, 'default');
  assert.strictEqual(transitions[2], 'genres');

  assert.strictEqual(previewReasons.length, 3);

  openedRecordings[0].options.onClose();
  assert.strictEqual(transitions[3], 'overview');
  assert.strictEqual(scheduled.length, 1);

  console.log('phase66 recording discovery ownership coverage ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
