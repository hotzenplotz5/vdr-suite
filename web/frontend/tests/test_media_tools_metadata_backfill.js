'use strict';
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontend = path.join(__dirname, '..');
const window = {document: null};
window.window = window;
window.VdrSuitePlatform = {
  getSelectedBackendId() { return 'default'; },
  getSelectedModule() { return 'overview'; },
  getClientApi() { return null; }
};
window.VdrSuiteBrowserSession = {
  csrfHeaders() { return {'X-CSRF-Token': 'existing-session-token'}; }
};
vm.createContext(window);
vm.runInContext([
  fs.readFileSync(path.join(frontend, 'platform/helpers.js'), 'utf8'),
  fs.readFileSync(path.join(frontend, 'home-recording-discovery.js'), 'utf8')
].join('\n'), window);
const api = window.VdrSuiteHomeRecordingDiscovery._test;

function recording(id, name, poster, embeddedManual) {
  const rec = {
    recordingId: 'andor-' + id, backendId: 'default',
    backendNativeId: '/srv/vdr/video/Serien/Andor/S02E' + id + '.rec',
    path: 'Serien/Andor/S02E' + id + ' ' + name,
    metadata: {provider: {
      episodeTitle: name, seasonNumber: 2, episodeNumber: Number(id)
    }, presentation: {posterUrl: poster}}
  };
  if (embeddedManual) rec.seriesMetadata = {
    manualAssignment: {active: true}
  };
  return api.seriesMemberProjection(rec, null, 'default');
}

const raw = recording('04', 'DE - Star Wars Andor (2022) - S02E04 - Waren Sie schon einmal in Ghorman', '');
const raw11 = recording('11', 'DE - Star Wars Andor (2022) - S02E11 - Wer weiß noch davon', '', true);
const posterMissing = recording('06', 'Die Freude ist ganz meinerseits', '');
const complete = recording('01', 'Ein Jahr später', '/existing/episode-art.jpg');
const series = {
  key: 'folder:serien/andor', title: 'Andor', path: 'Serien/Andor',
  episodes: [raw, posterMissing, complete, raw11]
};
assert.strictEqual(api.missingSeriesEpisodeMetadata(raw), true);
assert.strictEqual(api.missingSeriesEpisodeMetadata(posterMissing), true);
assert.strictEqual(api.missingSeriesEpisodeMetadata(complete), false);
assert.strictEqual(api.missingSeriesEpisodeMetadata(raw11), false);
assert.strictEqual(api.seriesMetadataBackfillTargets(series).length, 2);

const seriesCandidate = {
  kind: 'series', providerId: 'tmdb', externalNamespace: 'tv',
  externalId: '4321', title: 'Andor', posterReference: '/series-original.jpg'
};
function episodeCandidate(number, title) {
  return {
    kind: 'episode', providerId: 'tmdb', externalNamespace: 'tv-episode',
    parentExternalId: '4321', externalId: String(71000 + number),
    title, overview: 'Originale TMDb-Folgenbeschreibung',
    seasonNumber: 2, episodeNumber: number,
    posterReference: '/episode-original.jpg'
  };
}
const assigned = [];
const calls = [];
const client = {
  requestJson(route, options) {
    const action = route.split('/').pop().split('?')[0];
    const body = options && options.body ? JSON.parse(options.body) : {};
    calls.push({route, action, body, options});
    if (action === 'search') {
      assert.strictEqual(body.query, 'Andor');
      return Promise.resolve({
        providerId: 'tmdb', providerAvailable: true,
        candidates: [seriesCandidate]
      });
    }
    if (action === 'episodes') {
      assert.strictEqual(body.seriesExternalId, '4321');
      assert.strictEqual(body.seasonNumber, 2);
      return Promise.resolve({
        providerId: 'tmdb', providerAvailable: true,
        candidates: [episodeCandidate(4, 'Waren Sie schon einmal in Ghorman'),
          episodeCandidate(6, 'Die Freude ist ganz meinerseits')]
      });
    }
    if (action === 'manual') {
      assert.strictEqual(options.method, 'GET');
      return Promise.resolve({found: false});
    }
    if (action === 'assign') {
      assert.strictEqual(body.mediaType, 'episode');
      assert.strictEqual(body.providerId, 'tmdb');
      assert.strictEqual(body.seriesTitle, 'Andor');
      assert.strictEqual(body.expectedRevision, 0);
      assert.strictEqual(body.posterReference, '/series-original.jpg');
      assert.strictEqual(options.headers['X-CSRF-Token'], 'existing-session-token');
      assigned.push(body);
      return Promise.resolve({found: true, revision: 1});
    }
    throw new Error('Unexpected metadata API: ' + route);
  }
};

(async function () {
  const statuses = [];
  const done = await api.backfillSeriesEpisodeMetadata(series, 'default', client,
    function (message) { statuses.push(message); });
  assert.strictEqual(done.total, 2);
  assert.strictEqual(done.updated, 2);
  assert.strictEqual(done.skipped, 0);
  assert.strictEqual(done.failed, 0);
  assert.strictEqual(assigned.length, 2);
  assert.strictEqual(assigned[0].episodeNumber, 4);
  assert.strictEqual(assigned[0].title, 'Waren Sie schon einmal in Ghorman');
  assert.strictEqual(assigned[1].episodeNumber, 6);
  assert.strictEqual(assigned[1].title, 'Die Freude ist ganz meinerseits');
  assert.strictEqual(calls.filter(x => x.action === 'episodes').length, 1,
    'TMDb season fetched once for all missing episodes');
  assert.strictEqual(calls.filter(x => x.action === 'manual').length, 2);
  assert.strictEqual(calls.filter(x => x.action === 'assign').length, 2);
  assert(calls.every(x => x.route.includes('/recordings/metadata/')),
    'never calls media-import or video endpoints');
  assert(statuses.some(s => s.includes('2 ergänzt')));

  // MT.META8: Local franchise-prefixed group name must resolve the same
  // exact TMDb identity without fuzzy matching or a second video import.
  const aliasQueries = [];
  const aliasAssignments = [];
  const aliasClient = {
    requestJson(route, options) {
      const action = route.split('/').pop().split('?')[0];
      const body = options && options.body ? JSON.parse(options.body) : {};
      if (action === 'search') {
        aliasQueries.push(body.query);
        return Promise.resolve({
          providerId: 'tmdb', providerAvailable: true,
          candidates: body.query === 'Andor' ? [seriesCandidate] : []
        });
      }
      if (action === 'episodes') return Promise.resolve({
        providerId: 'tmdb', providerAvailable: true,
        candidates: [episodeCandidate(4, 'Waren Sie schon einmal in Ghorman')]
      });
      if (action === 'manual') return Promise.resolve({found: false});
      if (action === 'assign') {
        aliasAssignments.push(body);
        return Promise.resolve({found: true, revision: 1});
      }
      throw new Error('Unexpected alias metadata call ' + action);
    }
  };
  const aliasResult = await api.backfillSeriesEpisodeMetadata({
    title: 'Star Wars-Andor', episodes: [raw]
  }, 'default', aliasClient);
  assert.strictEqual(aliasResult.updated, 1);
  assert.deepStrictEqual(aliasQueries, ['Star Wars-Andor', 'Andor']);
  assert.strictEqual(aliasAssignments.length, 1);
  assert.strictEqual(aliasAssignments[0].seriesTitle, 'Andor',
    'persistent episode metadata uses the canonical TMDb series name');
  assert.strictEqual(aliasAssignments[0].episodeNumber, 4);

  // TMDb may return the *full* Star Wars branding for the short-query
  // fallback. Media Tools accepts either normalized title across both
  // requests; Home must behave identically.
  const brandedQueries = [];
  const brandedAssignments = [];
  const brandedSeriesCandidate = Object.assign({}, seriesCandidate, {
    title: 'Star Wars: Andor', originalTitle: 'Star Wars: Andor'
  });
  const brandedClient = {
    requestJson(route, options) {
      const action = route.split('/').pop().split('?')[0];
      const body = options && options.body ? JSON.parse(options.body) : {};
      if (action === 'search') {
        brandedQueries.push(body.query);
        return Promise.resolve({
          providerId: 'tmdb', providerAvailable: true,
          candidates: body.query === 'Andor' ? [brandedSeriesCandidate] : []
        });
      }
      if (action === 'episodes') return Promise.resolve({
        providerId: 'tmdb', providerAvailable: true,
        candidates: [episodeCandidate(4, 'Waren Sie schon einmal in Ghorman')]
      });
      if (action === 'manual') return Promise.resolve({found: false});
      if (action === 'assign') {
        brandedAssignments.push(body);
        return Promise.resolve({found: true, revision: 1});
      }
      throw new Error('Unexpected branded series metadata route ' + action);
    }
  };
  const brandedResult = await api.backfillSeriesEpisodeMetadata({
    title: 'Star Wars-Andor', episodes: [raw]
  }, 'default', brandedClient);
  assert.deepStrictEqual(brandedQueries, ['Star Wars-Andor', 'Andor']);
  assert.strictEqual(brandedResult.updated, 1,
    'TMDb full brand name must match the local franchise name after fallback');
  assert.strictEqual(brandedAssignments.length, 1);
  assert.strictEqual(brandedAssignments[0].seriesTitle, 'Star Wars: Andor');
  assert.strictEqual(brandedAssignments[0].episodeNumber, 4);

  const ambiguousAliasClient = {
    requestJson(route, options) {
      if (route.endsWith('/search')) return Promise.resolve({
        providerId: 'tmdb', providerAvailable: true,
        candidates: JSON.parse(options.body).query === 'Andor'
          ? [seriesCandidate, Object.assign({}, seriesCandidate,
            {externalId: '9999'})] : []
      });
      throw new Error('Ambiguous alias must never dispatch any metadata write');
    }
  };
  await assert.rejects(
    api.backfillSeriesEpisodeMetadata({
      title: 'Star Wars-Andor', episodes: [raw]
    }, 'default', ambiguousAliasClient),
    /Mehrere passende TMDb-Serien/
  );

  const noWriteCalls = [];
  const lockedClient = {
    requestJson(route, options) {
      const action = route.split('/').pop().split('?')[0];
      noWriteCalls.push(action);
      if (action === 'search') return Promise.resolve({
        providerId: 'tmdb', providerAvailable: true,
        candidates: [seriesCandidate]
      });
      if (action === 'episodes') return Promise.resolve({
        providerId: 'tmdb', providerAvailable: true,
        candidates: [episodeCandidate(4, 'Waren Sie schon einmal in Ghorman')]
      });
      if (action === 'manual') return Promise.resolve({found: true, revision: 3});
      throw new Error('Existing manual assignment must not be overwritten');
    }
  };
  const locked = await api.backfillSeriesEpisodeMetadata({
    title: 'Andor', episodes: [raw]
  }, 'default', lockedClient);
  assert.strictEqual(locked.updated, 0);
  assert.strictEqual(locked.skipped, 1);
  assert(!noWriteCalls.includes('assign'));

  const mismatchedPosterOnlyCalls = [];
  const mismatchedClient = {
    requestJson(route, options) {
      const action = route.split('/').pop().split('?')[0];
      mismatchedPosterOnlyCalls.push(action);
      if (action === 'search') return Promise.resolve({
        providerId: 'tmdb', providerAvailable: true, candidates: [seriesCandidate]
      });
      if (action === 'episodes') return Promise.resolve({
        providerId: 'tmdb', providerAvailable: true,
        candidates: [episodeCandidate(6, 'Alternative regionale Übersetzung')]
      });
      if (action === 'manual') return Promise.resolve({found: false});
      throw new Error('Correct episode title must not be replaced for artwork-only update');
    }
  };
  const mismatched = await api.backfillSeriesEpisodeMetadata({
    title: 'Andor', episodes: [posterMissing]
  }, 'default', mismatchedClient);
  assert.strictEqual(mismatched.updated, 0);
  assert.strictEqual(mismatched.skipped, 1);
  assert(!mismatchedPosterOnlyCalls.includes('assign'),
    'missing artwork must not overwrite a correct regional episode title');

  const ambiguousClient = {
    requestJson(route) {
      if (route.endsWith('/search')) return Promise.resolve({
        providerId: 'tmdb', providerAvailable: true,
        candidates: [seriesCandidate, Object.assign({}, seriesCandidate, {
          externalId: '8765'
        })]
      });
      throw new Error('Ambiguous candidates must not trigger another API operation');
    }
  };
  await assert.rejects(
    api.backfillSeriesEpisodeMetadata({title: 'Andor', episodes: [raw]},
      'default', ambiguousClient),
    /Keine eindeutige TMDb-Serie/
  );
  console.log('MT.META8 exact existing episode metadata backfill: PASS');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
