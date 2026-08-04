'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.join(
  __dirname,
  '..',
  'recordings2-metadata-assignment.js'
);
const source = fs.readFileSync(sourcePath, 'utf8');

const windowObject = {
  VdrSuiteRecordings2Shared: {
    recordingTitle: recording => String(recording && recording.title || '')
  }
};
const context = vm.createContext({
  window: windowObject,
  WeakSet: WeakSet,
  encodeURIComponent: encodeURIComponent,
  Number: Number,
  Object: Object,
  String: String,
  Array: Array,
  Promise: Promise,
  console: console
});
vm.runInContext(source, context, {filename: sourcePath});

const runtime = windowObject.VdrSuiteRecordings2MetadataAssignment;
assert(runtime);
assert.strictEqual(typeof runtime.mount, 'function');
assert(runtime.__test);

assert.strictEqual(
  runtime.__test.backendPath('living room', 'search'),
  '/api/backends/living%20room/recordings/metadata/search'
);
assert.strictEqual(
  runtime.__test.candidateLabel({kind: 'movie'}),
  'Film'
);
assert.strictEqual(
  runtime.__test.candidateLabel({kind: 'series'}),
  'Serie'
);
assert.strictEqual(
  runtime.__test.candidateLabel({
    kind: 'episode',
    seasonNumber: 2,
    episodeNumber: 4
  }),
  'S2 E4'
);

const payload = runtime.__test.assignmentPayload(
  {backendNativeId: '/video/Sherlock/episode.rec'},
  {
    kind: 'episode',
    providerId: 'tmdb',
    externalNamespace: 'tv-episode',
    externalId: '123',
    title: 'Ein Fall von Pink',
    originalTitle: 'A Study in Pink',
    overview: 'Erste Folge',
    releaseDate: '2010-07-25',
    posterReference: '/episode.jpg',
    seasonNumber: 1,
    episodeNumber: 1
  },
  3,
  'episode'
);
assert.deepStrictEqual(JSON.parse(JSON.stringify(payload)), {
  resourceKey: '/video/Sherlock/episode.rec',
  providerId: 'tmdb',
  externalNamespace: 'tv-episode',
  externalId: '123',
  mediaType: 'episode',
  title: 'Ein Fall von Pink',
  originalTitle: 'A Study in Pink',
  overview: 'Erste Folge',
  releaseDate: '2010-07-25',
  posterReference: '/episode.jpg',
  seasonNumber: 1,
  episodeNumber: 1,
  expectedRevision: 3
});

function hasPostOperation(operation) {
  return new RegExp("post\\(\\s*backendId\\s*,\\s*'" + operation + "'").test(source);
}

assert(source.includes('VdrSuiteBrowserSession'));
assert(hasPostOperation('search'));
assert(hasPostOperation('seasons'));
assert(hasPostOperation('episodes'));
assert(hasPostOperation('assign'));
assert(hasPostOperation('withdraw'));
assert(!source.includes('api.themoviedb.org'));
assert(!source.includes('image.tmdb.org'));
assert(!source.includes('VDR_SUITE_TMDB_READ_ACCESS_TOKEN'));

console.log('recordings2 metadata assignment tests passed');
