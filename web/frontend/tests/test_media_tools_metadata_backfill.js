'use strict';
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const root = path.join(__dirname, '..');
const win = {document: null};
win.window = win;
win.VdrSuitePlatform = {
  getSelectedBackendId() { return 'default'; },
  getSelectedModule() { return 'overview'; },
  getClientApi() { return null; }
};
win.VdrSuiteBrowserSession = {
  csrfHeaders() { return {'X-CSRF-Token': 'existing-session-token'}; }
};
vm.createContext(win);
vm.runInContext([
  fs.readFileSync(path.join(root, 'platform/helpers.js'), 'utf8'),
  fs.readFileSync(path.join(root, 'home-recording-discovery.js'), 'utf8')
].join('\n'), win);
const api = win.VdrSuiteHomeRecordingDiscovery._test;
const series = {
  kind: 'series', providerId: 'tmdb', externalNamespace: 'tv',
  externalId: '4321', title: 'Star Wars: Andor',
  posterReference: '/series-original.jpg'
};
function episode(number, title) {
  return {
    kind: 'episode', providerId: 'tmdb', externalNamespace: 'tv-episode',
    parentExternalId: '4321', externalId: String(71000 + number),
    title, overview: 'TMDb-Folgenbeschreibung', seasonNumber: 2,
    episodeNumber: number, posterReference: '/episode-poster.jpg'
  };
}
function member(number, title, poster, locked) {
  const recording = {
    recordingId: 'andor-' + number, backendId: 'default',
    backendNativeId: '/srv/vdr/video/Serien/Andor/S02E' + number + '.rec',
    path: 'Serien/Andor/S02E' + number + ' ' + title,
    metadata: {
      provider: {episodeTitle: title, seasonNumber: 2, episodeNumber: Number(number)},
      presentation: {posterUrl: poster}
    }
  };
  if (locked) recording.seriesMetadata = {manualAssignment: {active: true}};
  return api.seriesMemberProjection(recording, null, 'default');
}
const raw04 = member('04',
  'DE - Star Wars Andor (2022) - S02E04 - Waren Sie schon einmal in Ghorman', '');
const raw11 = member('11',
  'DE - Star Wars Andor (2022) - S02E11 - Wer weiß noch davon', '', true);
const posterOnly = member('06', 'Die Freude ist ganz meinerseits', '');
const complete = member('01', 'Ein Jahr später', '/already-correct.jpg');
const title = 'Star Wars-Andor';

assert.strictEqual(api.missingSeriesEpisodeMetadata(raw04), true);
assert.strictEqual(api.missingSeriesEpisodeMetadata(raw11), false,
  'selected assignment cannot be overwritten');
assert.strictEqual(api.missingSeriesEpisodeMetadata(posterOnly), true);
assert.strictEqual(api.missingSeriesEpisodeMetadata(complete), false);
assert.strictEqual(api.seriesMetadataBackfillTargets({
  title, episodes: [raw04, raw11, posterOnly, complete]
}).length, 2);

function clientFor(requests, resolver) {
  return {
    requestJson(route, opts) {
      const action = route.split('/').pop().split('?')[0];
      const body = opts && opts.body ? JSON.parse(opts.body) : {};
      requests.push({route, action, opts, body});
      if (action === 'resolve') {
        assert.strictEqual(route, '/api/v1/backends/default/metadata/resolve');
        assert.strictEqual(opts.method, 'POST');
        assert.strictEqual(opts.headers['X-CSRF-Token'], 'existing-session-token');
        assert.strictEqual(body.title, 'Star Wars-Andor');
        assert.strictEqual(body.kind, 'series');
        assert.strictEqual(body.seasonNumber, 2);
        return Promise.resolve(resolver(body));
      }
      if (action === 'manual') {
        assert.strictEqual(opts.method, 'GET');
        return Promise.resolve({found: false});
      }
      if (action === 'assign') {
        assert.strictEqual(opts.method, 'POST');
        assert.strictEqual(body.providerId, 'tmdb');
        assert.strictEqual(body.seriesTitle, 'Star Wars: Andor');
        assert.strictEqual(body.mediaType, 'episode');
        assert.strictEqual(body.expectedRevision, 0);
        assert.strictEqual(body.posterReference, '/series-original.jpg');
        return Promise.resolve({found: true, revision: 1});
      }
      throw new Error('Unexpected legacy metadata search: ' + route);
    }
  };
}
(async function () {
  const requests = [];
  const resolver = b => ({
    status: 'matched', mediaType: 'episode', series,
    episode: b.episodeNumber === 4
      ? episode(4, 'Waren Sie schon einmal in Ghorman')
      : episode(6, 'Die Freude ist ganz meinerseits')
  });
  const reports = [];
  const result = await api.backfillSeriesEpisodeMetadata({
    title, episodes: [raw04, posterOnly, complete, raw11]
  }, 'default', clientFor(requests, resolver), message => reports.push(message));
  assert.strictEqual(result.total, 2);
  assert.strictEqual(result.updated, 2);
  assert.strictEqual(result.failed, 0);
  assert.strictEqual(result.skipped, 0);
  assert.deepStrictEqual(requests.map(x => x.action), [
    'resolve', 'manual', 'assign', 'resolve', 'manual', 'assign'
  ]);
  assert.deepStrictEqual(requests.filter(x => x.action === 'assign').map(x =>
    x.body.episodeNumber), [4, 6]);
  assert(reports.some(x => x.includes('2 ergänzt')));

  const unresolvedRequests = [];
  const unresolved = await api.backfillSeriesEpisodeMetadata({
    title, episodes: [raw04]
  }, 'default', clientFor(unresolvedRequests, () => ({
    status: 'ambiguous'
  })));
  assert.strictEqual(unresolved.updated, 0);
  assert.strictEqual(unresolved.skipped, 1);
  assert.deepStrictEqual(unresolvedRequests.map(x => x.action), ['resolve']);

  const wrongTranslationRequests = [];
  const wrongTranslation = await api.backfillSeriesEpisodeMetadata({
    title, episodes: [posterOnly]
  }, 'default', clientFor(wrongTranslationRequests, () => ({
    status: 'matched', mediaType: 'episode', series,
    episode: episode(6, 'Alternative regionale Übersetzung')
  })));
  assert.strictEqual(wrongTranslation.updated, 0);
  assert.strictEqual(wrongTranslation.skipped, 1);
  assert.deepStrictEqual(wrongTranslationRequests.map(x => x.action),
    ['resolve', 'manual'], 'correct regional title remains untouched');

  let untouched = 0;
  const empty = await api.backfillSeriesEpisodeMetadata({
    title, episodes: [complete, raw11]
  }, 'default', {requestJson() { ++untouched; throw Error('Must not call'); }});
  assert.strictEqual(empty.total, 0);
  assert.strictEqual(untouched, 0);
  console.log('MT.META8 Public-v1 metadata resolver backfill: PASS');
}()).catch(err => { console.error(err); process.exitCode = 1; });
