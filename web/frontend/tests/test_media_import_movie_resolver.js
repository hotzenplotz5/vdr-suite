'use strict';

// MT.META7.2: execute the real Media Import automatic movie path without
// creating an import, a TMDb provider, a recording writer or a second matcher.
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const src = fs.readFileSync(path.join(__dirname, '..', 'modules',
  'media-import.js'), 'utf8');
assert(!src.includes('normalizedMovieMatchTitle('),
  'automatic movie identity belongs to the central Core resolver');
const hooked = src.replace(/\}\)\(window\);\s*$/, [
  'global.__movieResolverTest = {',
  '  activate(v) {current = v; owner = v.owner;',
  '    updatePreview = function() {};},',
  '  movie: autoIdentifyMovie',
  '};',
  '})(window);'
].join('\n'));
assert.notStrictEqual(hooked, src, 'execute actual product source');
const requests = [];
const pending = [];
let mode = 'matched';
const movie = {
  providerId: 'tmdb', kind: 'movie', externalNamespace: 'movie',
  externalId: '5002', title: 'Solar Voyage - Part Two: The Red Horizon',
  originalTitle: 'Solar Voyage - Part Two: The Red Horizon',
  overview: 'Provider movie description', releaseDate: '2024-04-11',
  posterReference: '/cover.jpg'
};
const win = {
  VdrSuiteBrowserSession: {
    csrfHeaders() {return {'X-CSRF-Token': 'existing-token'};}
  },
  VdrSuiteClientApi: {
    requestJson(route, options) {
      assert(route.endsWith('/metadata/resolve'),
        'movies must use exactly the existing central Public-v1 resolver');
      assert.strictEqual(options.method, 'POST');
      assert.strictEqual(options.credentials, 'same-origin');
      assert.strictEqual(options.headers['X-CSRF-Token'], 'existing-token');
      const payload = JSON.parse(options.body);
      requests.push(payload);
      if (mode === 'offline') return Promise.reject({status: 503});
      if (mode === 'late') return new Promise(resolve => pending.push(resolve));
      if (mode === 'matched') return Promise.resolve({
        status: 'matched', mediaType: 'movie', movie
      });
      return Promise.resolve({status: mode});
    }
  }
};
vm.runInNewContext(hooked, {window: win}, {filename: 'media-import.js'});

const source = {sourceId: 'scan_movie_012345', folder: 'Filme',
  label: 'Solar Voyage Deel 2 The Red Horizon.php'};
const plan = {status: 'ready', nfoYear: '2024',
  posterDataUri: 'data:image/jpeg;base64,/9j/2Q=='};
function view() {
  return {owner: 1, backendId: 'default', previewGeneration: 10,
    select: {value: source.sourceId}, sources: [source.sourceId],
    onlineKind: 'movie', onlineUserControlled: false, onlineAuto: false,
    onlineSelected: null, onlinePending: false, onlineAutoSelected: false,
    onlineItems: []};
}
async function flush() {
  for (let i = 0; i < 20; ++i) await Promise.resolve();
}

(async function run() {
  for (const answer of ['matched', 'ambiguous', 'not_found', 'offline']) {
    mode = answer;
    requests.length = 0;
    const current = view();
    win.__movieResolverTest.activate(current);
    win.__movieResolverTest.movie(current, source, plan);
    await flush();
    assert.strictEqual(requests.length, 1,
      'one automatic movie source -> one Core lookup');
    assert.deepStrictEqual(
      JSON.parse(JSON.stringify(requests[0])),
      {title: 'Solar Voyage Deel 2 The Red Horizon',
        kind: 'movie', year: 2024});
    assert.strictEqual(current.onlinePending, false);
    assert.strictEqual(Boolean(current.onlineSelected), answer === 'matched');
    assert.strictEqual(current.onlineAutoSelected, answer === 'matched');
    if (answer === 'ambiguous')
      assert(current.onlineStatus.includes('Mehrdeutiger'));
    if (answer === 'offline')
      assert(current.onlineStatus.includes('nicht erreichbar'));
  }

  // Late replies from an old source selection are read-only and ignored.
  mode = 'late';
  requests.length = 0;
  const stale = view();
  win.__movieResolverTest.activate(stale);
  win.__movieResolverTest.movie(stale, source, plan);
  await flush();
  assert.strictEqual(pending.length, 1);
  stale.select.value = 'scan_different_source';
  pending.shift()({status: 'matched', mediaType: 'movie', movie});
  await flush();
  assert.strictEqual(stale.onlineSelected, null);
  assert.strictEqual(stale.onlineAutoSelected, false);

  console.log('MT_META72_AUTO_MOVIE_RESOLVER=PASS');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
