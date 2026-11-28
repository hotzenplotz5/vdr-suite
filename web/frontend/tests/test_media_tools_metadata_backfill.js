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
  fs.readFileSync(path.join(root, 'recordings2-shared.js'), 'utf8'),
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

function canonicalId(number) {
  return 'rec_' + String(number).padStart(32, '0');
}
function clientFor(requests, resolver) {
  return {
    fetchClientRecordings({query}) {
      const match = [raw04, posterOnly].find(m => m.recording.path === query.path);
      assert.strictEqual(query.backend, 'default');
      assert(match, 'legacy identity lookup must use the actual recording path');
      return Promise.resolve({recordings: [Object.assign({}, match.recording, {
        publicRecordingId: canonicalId(match.episodeNumber)
      })], totalCount: 1});
    },
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
        const id = route.split('recordingId=')[1];
        assert.strictEqual(route, '/api/v1/backends/default/recordings/metadata/manual?recordingId=' + id);
        assert(/^rec_[0-9a-f]{32}$/.test(id));
        return Promise.resolve({found: false, recordingId:id, backendId:'default'});
      }
      if (action === 'assign') {
        assert.strictEqual(opts.method, 'POST');
        assert.strictEqual(route, '/api/v1/backends/default/recordings/metadata/assign');
        assert.strictEqual(body.recordingId, canonicalId(body.episodeNumber));
        assert.strictEqual(body.resourceKey, undefined);
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


  // Foundation (2021) S01E05: an old import has the correct episode identity
  // but raw title, missing poster and no description.
  const foundationMember = {
    seasonNumber: 1, episodeNumber: 5, posterUrl: '',
    episodeTitle: 'DE - Foundation (2021) - S01E05 - Erwachen',
    recording: {
      backendId: 'default', publicRecordingId: canonicalId(5),
      backendNativeId: '/video/Foundation/S01E05.rec',
      path: 'Serien/Foundation (2021)/S01E05',
      title: 'DE - Foundation (2021) - S01E05 - Erwachen'
    }
  };
  const existingFoundation = {
    found: true, providerId: 'tmdb', mediaType: 'episode',
    externalNamespace: 'tv-episode', externalId: '555',
    seasonNumber: 1, episodeNumber: 5,
    title: foundationMember.episodeTitle,
    overview: '', posterAvailable: false, revision: 3
  };
  async function repairFoundation(existing) {
    const calls = [];
    const client = {
      requestJson(route, options) {
        const action = route.split('/').pop().split('?')[0];
        const body = options && options.body ? JSON.parse(options.body) : {};
        calls.push({action, body});
        if (action === 'resolve') {
          assert.strictEqual(body.year, 2021,
            'original filename supplies the series premiere year');
          assert.strictEqual(body.title, 'Foundation');
          return Promise.resolve({
            status: 'matched', mediaType: 'episode',
            series: {
              providerId: 'tmdb', kind: 'series',
              externalId: '93740', title: 'Foundation',
              posterReference: '/foundation-poster.jpg'
            },
            episode: {
              providerId: 'tmdb', kind: 'episode',
              externalNamespace: 'tv-episode', externalId: '555',
              parentExternalId: '93740',
              seasonNumber: 1, episodeNumber: 5,
              title: 'Erwachen', overview: 'Beschreibung von Folge 5',
              releaseDate: '2021-10-22'
            }
          });
        }
        if (action === 'manual') {
          assert.strictEqual(route, '/api/v1/backends/default/recordings/metadata/manual?recordingId=' + canonicalId(5));
          return Promise.resolve(Object.assign({recordingId:canonicalId(5),backendId:'default'}, existing));
        }
        if (action === 'assign') {
          assert.strictEqual(route, '/api/v1/backends/default/recordings/metadata/assign');
          assert.strictEqual(body.recordingId, canonicalId(5));
          assert.strictEqual(body.resourceKey, undefined);
          return Promise.resolve({found: true, revision: 4});
        }
        throw new Error('Unexpected API route: ' + route);
      }
    };
    const result = await api.backfillSeriesEpisodeMetadata(
      {title: 'Foundation', episodes: [foundationMember]},
      'default', client);
    return {result, calls};
  }
  assert.strictEqual(api.missingSeriesEpisodeMetadata(foundationMember), true);
  const corrected = await repairFoundation(existingFoundation);
  assert.strictEqual(corrected.result.updated, 1);
  assert.deepStrictEqual(corrected.calls.map(call => call.action),
    ['resolve', 'manual', 'assign']);
  assert.strictEqual(corrected.calls[2].body.title, 'Erwachen');
  assert.strictEqual(corrected.calls[2].body.overview,
    'Beschreibung von Folge 5');
  assert.strictEqual(corrected.calls[2].body.expectedRevision, 3);
  assert.strictEqual(corrected.calls[2].body.posterReference,
    '/foundation-poster.jpg');
  const foreign = await repairFoundation(
    Object.assign({}, existingFoundation, {externalId: 'different-episode'}));
  assert.strictEqual(foreign.result.skipped, 1);
  assert.deepStrictEqual(foreign.calls.map(call => call.action),
    ['resolve', 'manual']);
  const localized = await repairFoundation(
    Object.assign({}, existingFoundation, {
      title: 'Deutscher Alternativtitel',
      overview: 'Vorhandener deutscher Beschreibungstext'
    }));
  assert.strictEqual(localized.result.updated, 1);
  assert.strictEqual(localized.calls[2].body.title, 'Deutscher Alternativtitel');
  assert.strictEqual(localized.calls[2].body.overview,
    'Vorhandener deutscher Beschreibungstext');
  const posterSelected = await repairFoundation(
    Object.assign({}, existingFoundation, {posterAvailable: true}));
  assert.strictEqual(posterSelected.result.skipped, 1,
    'existing selected artwork must not be lost');

  const publicId = 'rec_0123456789abcdef0123456789abcdef';
  const publicMember = Object.assign({}, raw04, {
    recording: {recordingId: publicId, backendId: 'default', recordedAt:'2026-11-28T10:00:00Z'}
  });
  async function repairPublic(existing) {
    const calls = [];
    const client = {
      requestJson(route, options) {
        const body = options.body ? JSON.parse(options.body) : {};
        calls.push({route, body});
        if (route === '/api/v1/backends/default/metadata/resolve')
          return Promise.resolve(resolver(body));
        if (route === '/api/v1/backends/default/recordings/metadata/manual?recordingId=' + publicId)
          return Promise.resolve(existing);
        if (route === '/api/v1/backends/default/recordings/metadata/assign') {
          assert.strictEqual(body.recordingId, publicId);
          assert.strictEqual(body.resourceKey, undefined);
          return Promise.resolve({found: true, revision: 1});
        }
        throw Error('Unexpected private metadata request: ' + route);
      }
    };
    const result = await api.backfillSeriesEpisodeMetadata({title, episodes:[publicMember]}, 'default', client);
    return {result, calls};
  }
  const publicRepair = await repairPublic({recordingId: publicId, backendId:'default',found:false});
  assert.strictEqual(publicRepair.result.updated, 1);
  assert.strictEqual(publicRepair.calls.length, 3);
  const wrongScope = await repairPublic({recordingId: publicId, backendId:'other',found:false});
  assert.strictEqual(wrongScope.result.skipped, 1);
  assert.strictEqual(wrongScope.calls.length, 2);
  const foreignPublic = await repairPublic({recordingId: publicId, backendId:'default',found:true,
    providerId:'tmdb', externalNamespace:'tv-episode',externalId:'foreign',mediaType:'episode',
    seasonNumber:2,episodeNumber:4,revision:3});
  assert.strictEqual(foreignPublic.result.skipped, 1);
  assert.strictEqual(foreignPublic.calls.length, 2);

  const missingRequests = [];
  const missingClient = clientFor(missingRequests, resolver);
  missingClient.fetchClientRecordings = () => Promise.resolve({recordings:[],totalCount:0});
  const missingIdentity = await api.backfillSeriesEpisodeMetadata({title, episodes:[raw04]}, 'default', missingClient);
  assert.strictEqual(missingIdentity.failed, 1);
  assert.strictEqual(missingRequests.length, 0, 'no metadata mutation without resolved identity');

  let untouched = 0;
  const empty = await api.backfillSeriesEpisodeMetadata({
    title, episodes: [complete, raw11]
  }, 'default', {requestJson() { ++untouched; throw Error('Must not call'); }});
  assert.strictEqual(empty.total, 0);
  assert.strictEqual(untouched, 0);
  console.log('MT.META8 Public-v1 metadata resolver backfill: PASS');
}()).catch(err => { console.error(err); process.exitCode = 1; });
