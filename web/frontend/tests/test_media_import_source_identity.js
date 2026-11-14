'use strict';

// MT.META7.1: execute the actual production source parser in isolation.
// No parallel implementation, provider calls or import mutations.
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const production = fs.readFileSync(path.join(__dirname, '..', 'modules',
  'media-import.js'), 'utf8');
const hook = production.replace(/\}\)\(window\);\s*$/, [
  "global.__meta71 = {parse: sourceMetadataIdentity,",
  "  batchTitle: batchSeriesTitleFromLabel,",
  "  folderTitle: onlineSeriesSearchTitle};",
  "})(window);"
].join('\n'));
assert.notStrictEqual(hook, production, 'test must use the real module');
const window = {};
window.window = window;
vm.runInNewContext(hook, {window}, {filename: 'media-import.js'});
const {parse, batchTitle, folderTitle} = window.__meta71;

function check(source, expected) {
  const got = parse(source);
  for (const key of Object.keys(expected)) {
    const actual = key === 'titleHints'
      ? Array.from(got.titleHints, value => value.title) : got[key];
    assert.deepStrictEqual(actual, expected[key],
      source.label + ' ' + key);
  }
  return got;
}

const rom = {folder: 'Serien/Rom',
  label: 'Rom (2005) (US) - S01E02 - Wie Titus Pullo die Republik stürzte.mkv'};
check(rom, {kind: 'series', fileTitle: 'Rom', folderTitle: 'Rom',
  titleHints: ['Rom'], year: 2005, season: 1, episode: 2,
  episodeTitle: 'Wie Titus Pullo die Republik stürzte'});
assert.strictEqual(Array.from(parse(rom).countries).join(','), 'US');
assert.strictEqual(batchTitle(rom), 'Rom');
assert.strictEqual(folderTitle(rom), 'Rom');

// A wrong filename query is not a separate provider-specific title case.
const qualified = {folder: 'Serien/Rom',
  label: 'DE - Rom (2005) (US) - S01E02 - Folge Zwei.php'};
check(qualified, {fileTitle: 'Rom', folderTitle: 'Rom',
  titleHints: ['Rom'], language: 'DE', year: 2005,
  season: 1, episode: 2, episodeTitle: 'Folge Zwei'});

// Generic external-feed language decorations must never become part of a
// TMDb lookup. Keep unrelated title text and numeric sequel identifiers.
check({folder: 'SciFi', label: '| DE | Mickey 17.mkv'}, {
  kind: 'movie', fileTitle: 'Mickey 17', language: 'DE',
  titleHints: ['Mickey 17', 'SciFi'], year: 0
});
check({folder: 'SciFi', label: '[EN] Silent Orbit Part 2.PHP'}, {
  kind: 'movie', fileTitle: 'Silent Orbit Part 2', language: 'EN',
  titleHints: ['Silent Orbit Part 2', 'SciFi']
});
check({folder: 'Filme', label: '(FR) La Planète Perdue (2022).mkv'}, {
  kind: 'movie', fileTitle: 'La Planète Perdue', language: 'FR',
  year: 2022, titleHints: ['La Planète Perdue']
});
check({folder: 'Serien/Another Show',
  label: '| EN | Another Show (2020) - S01E03 - Un épisode.ts'}, {
  kind: 'series', fileTitle: 'Another Show', language: 'EN',
  folderTitle: 'Another Show', titleHints: ['Another Show'],
  season: 1, episode: 3, year: 2020
});
check({folder: 'Filme', label: 'Der Untergang.mkv'}, {
  fileTitle: 'Der Untergang', language: '',
  titleHints: ['Der Untergang']
});

// Exact bytes from the real yaVDR checked-preview database: U+2503 heavy
// box-drawing strokes, NOT ASCII pipes. All paths use the same source parser.
const liveUnicode = {folder: 'SciFi', label: '┃DE┃ Mickey 17.mkv'};
check(liveUnicode, {kind: 'movie', fileTitle: 'Mickey 17',
  language: 'DE', year: 0, titleHints: ['Mickey 17', 'SciFi']});
check({folder: 'SciFi', label: '│ EN │ Deep Space (2025).php'}, {
  kind: 'movie', fileTitle: 'Deep Space', language: 'EN', year: 2025,
  titleHints: ['Deep Space', 'SciFi']
});
check({folder: 'Filme', label: '║FR║ La Planète Perdue.mkv'}, {
  fileTitle: 'La Planète Perdue', language: 'FR',
  titleHints: ['La Planète Perdue']
});
check({folder: 'Serien/Rom', label: '｜DE｜ Rom (2005) - S01E02 - Folge Zwei.mkv'}, {
  kind: 'series', fileTitle: 'Rom', language: 'DE', year: 2005,
  season: 1, episode: 2, episodeTitle: 'Folge Zwei',
  titleHints: ['Rom']
});
// Language-looking text in the middle of a title is NOT a file prefix;
// arbitrary delimited words or numbers must never be removed.
check({folder: 'Filme', label: 'Film ┃DE┃ und das Geheimnis.mkv'}, {
  fileTitle: 'Film ┃DE┃ und das Geheimnis', language: ''
});
check({folder: 'Filme', label: '┃XYZ┃ Der letzte Planet.mkv'}, {
  fileTitle: '┃XYZ┃ Der letzte Planet', language: ''
});

// A source-supplied episode title remains independent evidence for the
// single series resolver. The actual external-feed Unicode separator must
// not contaminate either the series identity or the episode hint.
check({folder: 'Oskar',
  label: '┃DE┃ Ocean Stories - S01E08 - Die Flaschenpost.mkv'}, {
  kind: 'series', fileTitle: 'Ocean Stories', language: 'DE',
  season: 1, episode: 8, episodeTitle: 'Die Flaschenpost',
  titleHints: ['Ocean Stories', 'Oskar']
});

// Real Media Tools feed labels use U+258E (▎) after the language code.
// The repeated dotted source basename is not an episode title.
check({folder: 'Doku',
  label: 'DE ▎ Lost Treasures of Rome - S01E06 - Lost.Treasures.Of.Rome.S01E06.mkv'}, {
  kind: 'series', fileTitle: 'Lost Treasures of Rome', language: 'DE',
  season: 1, episode: 6, episodeTitle: '',
  titleHints: ['Lost Treasures of Rome', 'Doku']
});
// Preserve genuine episode names and remove no legitimate title suffix.
check({folder: 'Serien',
  label: 'EN ▎ Sample Series - S01E06 - A New Discovery.mkv'}, {
  kind: 'series', fileTitle: 'Sample Series', language: 'EN',
  season: 1, episode: 6, episodeTitle: 'A New Discovery'
});

// Episode is named by coordinates; the adjacent show folder supplies title.
const folderBased = {folder: 'Serien/Andor/Staffel 1',
  label: 'S01E01 Pilot.mkv'};
check(folderBased, {kind: 'series', fileTitle: '', folderTitle: 'Andor',
  titleHints: ['Andor'], year: 0, season: 1, episode: 1,
  episodeTitle: 'Pilot'});

// A second distinct hint survives: the Core will validate identities.
const alternate = {folder: 'Serien/Star Wars Andor',
  label: 'Andor (2022) - S02E04 - Waren Sie schon einmal in Ghorman.mkv'};
check(alternate, {kind: 'series', fileTitle: 'Andor',
  folderTitle: 'Star Wars Andor',
  titleHints: ['Andor', 'Star Wars Andor'], year: 2022,
  season: 2, episode: 4});

// A generic import directory does not masquerade as a series title.
const generic = {folder: 'Eingang/Unsortiert',
  label: 'DE - Unbekannte Serie (2020) - Staffel 2 Folge 11 - Finale.ts'};
check(generic, {kind: 'series', fileTitle: 'Unbekannte Serie',
  folderTitle: '', titleHints: ['Unbekannte Serie'],
  year: 2020, season: 2, episode: 11, episodeTitle: 'Finale'});
const foundation = {folder: 'Serien/Foundation',
  label: 'Foundation (2021) - S01E05 - Erwachen.mkv'};
check(foundation, {titleHints: ['Foundation'],
  year: 2021, season: 1, episode: 5});

// Legacy PHP suffixes are video source filenames (never PHP execution).
const movie = {folder: 'Filme', label: 'Rebel Moon Deel 2 The Scargiver.PHP'};
check(movie, {kind: 'movie', fileTitle: 'Rebel Moon Deel 2 The Scargiver',
  titleHints: ['Rebel Moon Deel 2 The Scargiver'],
  year: 0, season: null, episode: null});
const regionalMovie = {folder: 'Filme',
  label: 'Original_Title (2019) (GB).php'};
check(regionalMovie, {kind: 'movie', fileTitle: 'Original Title',
  year: 2019, titleHints: ['Original Title']});
assert.strictEqual(Array.from(parse(regionalMovie).countries).join(','), 'GB');
assert(!production.includes('Rom (2005) (US)'),
  'no title-specific workaround belongs in the parser');

// MT.META7.3: exercise the production batch snapshot against a fake Core.
// A confirmed series directory may reuse its series ID, but each episode is
// checked by the existing authenticated episodes endpoint, never assumed.
const batchCalls = [];
const batchSeries = {kind: 'series', providerId: 'tmdb',
  externalNamespace: 'tv', externalId: '202359',
  title: 'Rom', originalTitle: 'Rome', overview: '',
  releaseDate: '2005-01-01', posterReference: '/rom-poster.jpg'};
function batchEpisode(number) {
  return {kind: 'episode', providerId: 'tmdb',
    externalNamespace: 'tv-episode', externalId: String(1100 + number),
    parentExternalId: '202359', title: 'Episode ' + number,
    originalTitle: '', overview: '', releaseDate: '2005-01-02',
    posterReference: '/rom-episode.jpg',
    seasonNumber: 1, episodeNumber: number};
}
const batchWindow = {
  VdrSuiteBrowserSession: {csrfHeaders() { return {}; }},
  VdrSuiteClientApi: {
    requestJson(route, options) {
      const data = JSON.parse(options.body);
      batchCalls.push({route, data});
      if (route.endsWith('/metadata/resolve')) {
        return Promise.resolve({status: 'matched', mediaType: 'episode',
          series: batchSeries, episode: batchEpisode(data.episodeNumber)});
      }
      assert(route.endsWith('/recordings/metadata/episodes'));
      assert.strictEqual(data.seriesExternalId, '202359');
      return Promise.resolve({providerId: 'tmdb', providerAvailable: true,
        truncated: false, candidates: [batchEpisode(2), batchEpisode(3)]});
    }
  }
};
const batchHook = production.replace(/\}\)\(window\);\s*$/, [
  "global.__meta73 = {",
  " activate(view) { current = view; owner = view.owner; },",
  " snapshot: batchMetadataSnapshot",
  "};",
  "})(window);"
].join('\n'));
assert.notStrictEqual(batchHook, production);
vm.runInNewContext(batchHook, {window: batchWindow},
  {filename: 'media-import.js'});
function batchSource(folder, number) {
  return {sourceId: 'batch_' + folder + '_' + number,
    folder, label: 'Rom (2005) - S01E' +
      String(number).padStart(2, '0') + ' - Episode ' + number + '.mkv'};
}
const batchPlan = {state: 'ready', targetFolder: 'Serien/Rom'};
(async function testBatchSeriesReuse() {
  const view = {owner: 7, backendId: 'default', onlineSeries: null};
  batchWindow.__meta73.activate(view);
  const cache = new Map();
  const first = await batchWindow.__meta73.snapshot(
    view, batchSource('Serien/Rom', 1), batchPlan, cache);
  const second = await batchWindow.__meta73.snapshot(
    view, batchSource('Serien/Rom', 2), batchPlan, cache);
  assert(first.startsWith('v2|episode|'));
  assert(second.startsWith('v2|episode|'));
  assert.deepStrictEqual(batchCalls.map(x => x.route.split('/').pop()),
    ['resolve', 'episodes'], 'same named group reuses confirmed series ID');
  // A distinct directory never inherits the first group's identity.
  await batchWindow.__meta73.snapshot(
    view, batchSource('Serien/Other', 3), batchPlan, cache);
  assert(batchCalls[2].route.endsWith('/metadata/resolve'));
  // Generic import/genre roots never share identity merely by title.
  const doku = batchSource('Doku', 1);
  await batchWindow.__meta73.snapshot(view, doku, batchPlan, cache);
  await batchWindow.__meta73.snapshot(view,
    batchSource('Doku', 2), batchPlan, cache);
  assert(batchCalls[3].route.endsWith('/metadata/resolve'));
  assert(batchCalls[4].route.endsWith('/metadata/resolve'));
  // Same-title productions without a source year cannot borrow a previous
  // series identity even when they share a named directory.
  function undated(number) {
    return {sourceId: 'undated_' + number, folder: 'Serien/Rom',
      label: 'Rom - S01E' + String(number).padStart(2, '0') +
        ' - Episode ' + number + '.mkv'};
  }
  const undatedCache = new Map();
  await batchWindow.__meta73.snapshot(view, undated(1),
    batchPlan, undatedCache);
  await batchWindow.__meta73.snapshot(view, undated(2),
    batchPlan, undatedCache);
  assert(batchCalls[5].route.endsWith('/metadata/resolve'));
  assert(batchCalls[6].route.endsWith('/metadata/resolve'));
  // A different descriptive title on a same-coordinate source must go
  // through the normal Core resolver, not inherit a cached series blindly.
  const differentTitle = batchSource('Serien/Rom', 3);
  differentTitle.label = 'Rom (2005) - S01E03 - A Different Story.mkv';
  await batchWindow.__meta73.snapshot(view, differentTitle,
    batchPlan, cache);
  assert(batchCalls[7].route.endsWith('/recordings/metadata/episodes'));
  assert(batchCalls[8].route.endsWith('/metadata/resolve'));
  console.log('MT_META73_BATCH_SERIES_REUSE=PASS');
})().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});

// Exercise the actual automatic single-source resolver wiring. A distinct
// folder hint is tried only after not_found, not after ambiguity or failure.
const liveRequests = [];
let replyMode = 'matched';
const liveSeries = {
  kind: 'series', providerId: 'tmdb', externalNamespace: 'tv',
  externalId: '1456', title: 'Some Show',
  originalTitle: 'Some Show', overview: '', releaseDate: '2022-01-01',
  posterReference: '/cover.jpg'
};
const liveEpisode = {
  kind: 'episode', providerId: 'tmdb', externalNamespace: 'tv-episode',
  externalId: '778', parentExternalId: '1456', title: 'Episode',
  seasonNumber: 2, episodeNumber: 4, overview: '',
  releaseDate: '2022-01-02', posterReference: '/episode.jpg'
};
const liveWindow = {
  VdrSuiteBrowserSession: {
    csrfHeaders() { return {'X-CSRF-Token': 'existing-session'}; }
  },
  VdrSuiteClientApi: {
    requestJson(route, options) {
      assert(route.endsWith('/metadata/resolve'));
      assert.strictEqual(options.method, 'POST');
      const body = JSON.parse(options.body);
      liveRequests.push(body);
      if (replyMode === 'ambiguous')
        return Promise.resolve({status: 'ambiguous'});
      if (replyMode === 'offline')
        return Promise.reject({status: 503});
      if (liveRequests.length === 1)
        return Promise.resolve({status: 'not_found'});
      return Promise.resolve({
        status: 'matched', mediaType: 'episode',
        series: liveSeries, episode: liveEpisode
      });
    }
  }
};
const autoHook = production.replace(/\}\)\(window\);\s*$/, [
  "global.__meta71Auto = {",
  " activate(view) { current = view; owner = view.owner;",
  "   updatePreview = function() {}; },",
  " auto: autoIdentifyEpisode",
  "};",
  "})(window);"
].join('\n'));
assert.notStrictEqual(autoHook, production);
vm.runInNewContext(autoHook, {window: liveWindow}, {filename: 'media-import.js'});

const liveSource = {
  sourceId: 'scan_meta71_test', folder: 'Serien/Alternate Show',
  label: 'Some Show (2022) - S02E04 - Episode.mkv'
};
function liveView() {
  return {owner: 4, backendId: 'default', previewGeneration: 7,
    select: {value: liveSource.sourceId}, sources: [liveSource.sourceId],
    onlineUserControlled: false, onlineAuto: false,
    onlineSelected: null, onlineItems: [], onlineAutoSelected: false};
}
async function settle() {
  for (let i = 0; i < 20; ++i) await Promise.resolve();
}
(async function () {
  let view = liveView();
  liveWindow.__meta71Auto.activate(view);
  liveWindow.__meta71Auto.auto(view, liveSource);
  await settle();
  assert.deepStrictEqual(liveRequests.map(x => x.title),
    ['Some Show', 'Alternate Show'],
    'only not_found tries the separate normalized folder hint');
  assert(liveRequests.every(function (payload) {
    return payload.episodeTitle === 'Episode' &&
      payload.seasonNumber === 2 && payload.episodeNumber === 4;
  }), 'series fallback preserves filename-derived episode title and coordinates');
  assert.strictEqual(view.onlineSelected, liveEpisode);

  liveRequests.length = 0;
  replyMode = 'ambiguous';
  view = liveView();
  liveWindow.__meta71Auto.activate(view);
  liveWindow.__meta71Auto.auto(view, liveSource);
  await settle();
  assert.strictEqual(liveRequests.length, 1);
  assert.strictEqual(view.onlineSelected, null);
  assert(view.onlineStatus.includes('Mehrdeutige'),
    'an ambiguous series must not be auto-assigned');

  liveRequests.length = 0;
  replyMode = 'offline';
  view = liveView();
  liveWindow.__meta71Auto.activate(view);
  liveWindow.__meta71Auto.auto(view, liveSource);
  await settle();
  assert.strictEqual(liveRequests.length, 1);
  assert.strictEqual(view.onlineSelected, null);
  assert(view.onlineStatus.includes('fehlgeschlagen'),
    'a provider error must not turn into a second hidden search');

  console.log('MT_META71_SOURCE_IDENTITY=PASS');
  console.log('MT_META71_AUTO_LOOKUP_FALLBACK=PASS');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
