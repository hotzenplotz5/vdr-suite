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
const genreSourcePath = path.join(__dirname, '..', 'recordings2-metadata-genre.js');
const genreSource = fs.readFileSync(genreSourcePath, 'utf8');

const windowObject = {};
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
const sharedSourcePath = path.join(__dirname, '..', 'recordings2-shared.js');
vm.runInContext(fs.readFileSync(sharedSourcePath, 'utf8'), context, {filename: sharedSourcePath});
vm.runInContext(source, context, {filename: sourcePath});
vm.runInContext(genreSource, context, {filename: genreSourcePath});

const runtime = windowObject.VdrSuiteRecordings2MetadataAssignment;
const genreRuntime = windowObject.VdrSuiteRecordings2MetadataGenre;
assert(runtime);
assert.strictEqual(typeof runtime.mount, 'function');
assert(runtime.__test);
assert(genreRuntime);
assert.strictEqual(typeof genreRuntime.mount, 'function');
assert(genreRuntime.__test);

assert.strictEqual(
  runtime.__test.publicBackendPath('living room', 'search'),
  '/api/v1/backends/living%20room/recordings/metadata/search'
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

assert.deepStrictEqual(
  JSON.parse(JSON.stringify(genreRuntime.__test.genrePayload(
    {backendNativeId: '/video/Sherlock/episode.rec'},
    'crime'
  ))),
  {
    genreId: 'crime'
  }
);

function hasPostOperation(operation) {
  return new RegExp("post\\(\\s*backendId\\s*,\\s*'" + operation + "'").test(source);
}

assert(source.includes('VdrSuiteBrowserSession'));
assert(hasPostOperation('search'));
assert(hasPostOperation('seasons'));
assert(hasPostOperation('episodes'));
assert(source.includes("'assign',\n        recording,"));
assert(source.includes("postRecording(backendId, 'withdraw', recording"));
assert(genreSource.includes('assignment().postRecording'));
assert(!source.includes("'/api/backends/'"));
assert(!genreSource.includes('resourceKey'));
assert(genreSource.includes('function get(backendId, recording)'));
assert(genreSource.includes('Genre hinzufügen'));
assert(source.includes('VdrSuiteRecordings2MetadataGenre'));
assert(!source.includes('api.themoviedb.org'));
assert(!source.includes('image.tmdb.org'));
assert(!source.includes('VDR_SUITE_TMDB_READ_ACCESS_TOKEN'));
assert(!genreSource.includes('api.themoviedb.org'));
assert(!genreSource.includes('image.tmdb.org'));
assert(!genreSource.includes('VDR_SUITE_TMDB_READ_ACCESS_TOKEN'));

const recId = 'rec_' + 'a'.repeat(32);
const publicPayload = runtime.__test.assignmentPayload({
  publicRecordingId:recId, backendNativeId:'/native/never-submit.rec'
}, {kind:'movie', providerId:'tmdb', externalNamespace:'movie', externalId:'13',
  title:'Forrest Gump'}, 1, 'movie');
assert.strictEqual(publicPayload.recordingId, recId);
assert(!('resourceKey' in publicPayload));
async function verifyRequests() {
const calls = [];
windowObject.VdrSuiteBrowserSession = {csrfHeaders:() => ({'X-CSRF-Token':'existing-session'})};
const oldCard = {backendId:'living-room', backendNativeId:'/video/Sherlock/episode.rec',
  path:'/video/Sherlock/episode.rec', recordingId:'old-native-id'};
const lookups = [];
windowObject.VdrSuiteClientApi = {
  fetchClientRecordings(options) {
    lookups.push(options);
    return Promise.resolve({totalCount:1, recordings:[
      Object.assign({}, oldCard, {publicRecordingId:recId})
    ]});
  },
  requestJson(route, options) {
    calls.push({route, options});
    return Promise.resolve({revision:2});
  }
};
await runtime.__test.postRecording('living-room','assign',oldCard,publicPayload);
await runtime.__test.postRecording('living-room','withdraw',{recordingId:recId}, {expectedRevision:2});
await runtime.__test.post('living-room','search',{query:'Forrest Gump',kind:'movie',limit:12});
assert(calls.every(c => c.route.startsWith('/api/v1/backends/living-room/recordings/metadata/')));
assert(calls.every(c => c.options.headers['X-CSRF-Token'] === 'existing-session'));
assert.strictEqual(JSON.parse(calls[0].options.body).recordingId, recId);
assert.strictEqual(JSON.parse(calls[1].options.body).expectedRevision, 2);
assert(calls.every(c => !c.options.body.includes('never-submit.rec')));

console.log('recordings2 metadata assignment tests passed');

// Genre changes share the existing assignment transport and never submit
// the private resource key when a public identity is already known.
const publicGenre = genreRuntime.__test.genrePayload({
  publicRecordingId:recId, backendNativeId:'/native/never-submit.rec'
}, 'crime');
assert.deepStrictEqual(JSON.parse(JSON.stringify(publicGenre)), {recordingId:recId, genreId:'crime'});
await genreRuntime.__test.post('living-room', oldCard, 'crime');
await genreRuntime.__test.post('living-room', {recordingId:recId}, '');
await genreRuntime.__test.get('living-room', oldCard);
assert.strictEqual(calls[calls.length - 3].route, '/api/v1/backends/living-room/recordings/metadata/genre');
assert.strictEqual(JSON.parse(calls[calls.length - 3].options.body).recordingId, recId);
assert.strictEqual(JSON.parse(calls[calls.length - 2].options.body).genreId, '');
assert.strictEqual(calls[calls.length - 1].route, '/api/v1/backends/living-room/recordings/metadata/genre?recordingId=' + recId);
assert.strictEqual(calls[calls.length - 1].options.method, 'GET');

assert.strictEqual(lookups.length, 3);
assert(lookups.every(call => call.query.backend === 'living-room'));
assert(calls.every(call => !String(call.options.body || '').includes('resourceKey')));
assert(calls.every(call => !String(call.options.body || '').includes('/video/')));
const before = calls.length;
await assert.rejects(runtime.__test.postRecording('other','withdraw',oldCard,{expectedRevision:2}));
assert.strictEqual(calls.length,before);
console.log('recordings2 metadata assignment and genre public identity requests passed');
}

verifyRequests().catch(error => {console.error(error); process.exitCode = 1;});
