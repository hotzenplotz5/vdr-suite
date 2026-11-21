'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const source = fs.readFileSync(
  'web/frontend/home-recording-discovery.js',
  'utf8'
);

assert(
  source.includes('/recordings/series-hierarchy'),
  'Home must own one explicit hierarchy mutation route'
);

assert(
  source.includes('seriesHierarchyOverride'),
  'embedded hierarchy override must be consumed'
);

assert(
  source.includes('Pilot / Miniserie'),
  'Pilot/Miniserie preset must remain available'
);

assert(
  source.includes('TV-Filme / Specials'),
  'TV movie / special preset must remain available'
);

assert(
  source.includes('Automatische Zuordnung'),
  'hierarchy override must remain reversible'
);

const seriesRailSource = source.slice(
  source.indexOf('function renderSeriesRail('),
  source.indexOf('function renderSeriesDetail(')
);

const seriesDetailSource = source.slice(
  source.indexOf('function renderSeriesDetail('),
  source.indexOf('function applySeriesProjection(')
);

assert(
  !seriesRailSource.includes('seriesHierarchyCanEdit(member)'),
  'renderSeriesRail must never reference episode hierarchy member state'
);

assert(
  !seriesRailSource.includes('member.seriesHierarchy'),
  'Series overview cards must remain independent from per-episode hierarchy state'
);

assert.strictEqual(
  (seriesDetailSource.match(/seriesHierarchyCanEdit\(member\)/g) || []).length,
  1,
  'renderSeriesDetail must own exactly one episode hierarchy edit control'
);

assert(
  !source.includes(
    "requestJson('/api/backends/' + backendId + '/recordings/series-hierarchy?"
  ),
  'Home must not introduce per-recording hierarchy GET fanout'
);

const mutationCalls = [];

const windowObject = {
  document: null,
  setTimeout,
  clearTimeout,
  Date,
  Math,
  Promise,
  console,
  VdrSuiteFrontendHelpers: {
    recordingMetadataPosterUrl() {
      return '';
    }
  },
  VdrSuiteBrowserSession: {
    csrfHeaders() {
      return {'X-VDR-Suite-CSRF': 'test-token'};
    }
  }
};

windowObject.window = windowObject;

const context = {
  window: windowObject,
  console,
  setTimeout,
  clearTimeout,
  Date,
  Math,
  Promise
};

vm.createContext(context);
vm.runInContext(
  source,
  context,
  {filename: 'home-recording-discovery.js'}
);

const api =
  context.window.VdrSuiteHomeRecordingDiscovery;

assert(api);
assert(api._test);

function recording(id, title, override) {
  const value = {
    id,
    recordingId: id,
    backendId: 'default',
    backendNativeId: id + '-native',
    resourceKey: 'cache/' + id,
    title,
    path:
      '/srv/vdr/video/Serien/Battlestar_Galactica/' +
      title +
      '/2026-01-01.00.00.1-0.rec',
    metadata: {
      provider: {}
    }
  };

  if (override) {
    value.seriesHierarchyOverride = override;
  }

  return value;
}

function rich(episodeName, seasonNumber, episodeNumber) {
  return {
    available: true,
    status: 'ready',
    provider: 'tvscraper',
    mediaType: 'series',
    providerId: 1972,
    title: 'Battlestar Galactica',
    episodeName,
    seasonNumber,
    episodeNumber
  };
}

const pilot = recording(
  'pilot-double',
  'Pilot',
  {
    available: true,
    groupType: 'special',
    seasonNumber: 0,
    groupLabel: 'Pilot / Miniserie',
    sortOrder: -100,
    episodeStart: 1,
    episodeEnd: 2,
    revision: 1
  }
);

const seasonOne = recording(
  'season-one',
  '33 Minuten'
);

const razor = recording(
  'razor',
  'Auf Messers Schneide',
  {
    available: true,
    groupType: 'special',
    seasonNumber: 0,
    groupLabel: 'TV-Filme / Specials',
    sortOrder: 1000,
    episodeStart: 0,
    episodeEnd: 0,
    revision: 1
  }
);

const unknown = recording(
  'unknown',
  'Noch unbekannt'
);

const members = [
  api._test.seriesMemberProjection(
    pilot,
    rich('Pilot', 0, 0),
    'default'
  ),
  api._test.seriesMemberProjection(
    seasonOne,
    rich('33 Minuten', 1, 1),
    'default'
  ),
  api._test.seriesMemberProjection(
    razor,
    rich('Auf Messers Schneide', 0, 0),
    'default'
  ),
  api._test.seriesMemberProjection(
    unknown,
    rich('Noch unbekannt', 0, 0),
    'default'
  )
];

const projection =
  api._test.buildSeriesProjection(members);

assert.strictEqual(projection.length, 1);

const series = projection[0];

assert.deepStrictEqual(
  Array.from(
    series.seasons,
    (season) => season.label
  ),
  [
    'Pilot / Miniserie',
    'Staffel 1',
    'TV-Filme / Specials',
    'Staffel unbekannt'
  ],
  'specials must bracket normal seasons according to explicit sort order while unknown remains last'
);

const pilotSeason =
  series.seasons[0];

assert.strictEqual(
  pilotSeason.groupType,
  'special'
);

assert.strictEqual(
  pilotSeason.episodes.length,
  1
);

assert.strictEqual(
  pilotSeason.episodes[0].nativeSeasonNumber,
  0,
  'native hierarchy must remain preserved underneath presentation override'
);

assert.strictEqual(
  pilotSeason.episodes[0].seriesHierarchyOverrideAvailable,
  true
);

assert.strictEqual(
  api._test.seriesEpisodeNumberLabel(
    pilotSeason.episodes[0]
  ),
  'Folge 1–2',
  'double episode range must be presented without splitting the VDR recording'
);

const regularSeason =
  series.seasons[1];

assert.strictEqual(
  regularSeason.number,
  1
);

assert.strictEqual(
  regularSeason.episodes[0].seasonNumber,
  1
);

assert.strictEqual(
  regularSeason.episodes[0].episodeNumber,
  1
);

assert.strictEqual(
  regularSeason.episodes[0].seriesHierarchyOverrideAvailable,
  false
);

pilot.seriesHierarchyOverride = {
  available: false
};

const resetProjection =
  api._test.buildSeriesProjection(
    [pilotSeason.episodes[0]]
  );

assert.strictEqual(
  resetProjection[0].seasons[0].label,
  'Staffel unbekannt',
  'clearing the override must restore the untouched native hierarchy'
);

const client = {
  requestJson(route, options) {
    mutationCalls.push({route, options});
    return Promise.resolve({
      available: true,
      backendId: 'default',
      recordingKey: 'cache/pilot-double',
      groupType: 'special',
      seasonNumber: 0,
      groupLabel: 'Pilot / Miniserie',
      sortOrder: -100,
      episodeStart: 1,
      episodeEnd: 2,
      revision: 2
    });
  }
};

(async function () {
  const response =
    await api._test.requestSeriesHierarchyOverride(
      client,
      'default',
      pilot,
      {
        operation: 'set',
        groupType: 'special',
        seasonNumber: 0,
        groupLabel: 'Pilot / Miniserie',
        sortOrder: -100,
        episodeStart: 1,
        episodeEnd: 2
      }
    );

  assert.strictEqual(response.available, true);
  assert.strictEqual(mutationCalls.length, 1);

  assert.strictEqual(
    mutationCalls[0].route,
    '/api/backends/default/recordings/series-hierarchy'
  );

  assert.strictEqual(
    mutationCalls[0].options.method,
    'POST'
  );

  assert.strictEqual(
    mutationCalls[0].options.credentials,
    'same-origin'
  );

  assert.strictEqual(
    mutationCalls[0].options.cache,
    'no-store'
  );

  assert.strictEqual(
    mutationCalls[0].options.headers['Content-Type'],
    'application/json'
  );

  assert.strictEqual(
    mutationCalls[0].options.headers['X-VDR-Suite-CSRF'],
    'test-token'
  );

  const body =
    JSON.parse(mutationCalls[0].options.body);

  assert.strictEqual(
    body.resourceKey,
    'cache/pilot-double'
  );

  assert.strictEqual(
    body.operation,
    'set'
  );

  assert.strictEqual(
    body.groupType,
    'special'
  );

  assert.strictEqual(
    body.groupLabel,
    'Pilot / Miniserie'
  );

  assert.strictEqual(
    body.episodeStart,
    1
  );

  assert.strictEqual(
    body.episodeEnd,
    2
  );

  assert.strictEqual(
    mutationCalls.length,
    1,
    'one operator mutation must produce exactly one hierarchy HTTP request'
  );

  let failedClosed = false;

  try {
    await api._test.requestSeriesHierarchyOverride(
      client,
      'default',
      {
        recordingId: 'missing-key'
      },
      {
        operation: 'set',
        groupType: 'season',
        seasonNumber: 1
      }
    );
  } catch (_) {
    failedClosed = true;
  }

  assert.strictEqual(
    failedClosed,
    true,
    'hierarchy mutation without canonical resourceKey must fail closed'
  );

  assert.strictEqual(
    mutationCalls.length,
    1,
    'failed-closed mutation must not send an HTTP request'
  );

  console.log(
    'Home Series hierarchy override, specials, double episodes and mutation boundary ok'
  );
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
