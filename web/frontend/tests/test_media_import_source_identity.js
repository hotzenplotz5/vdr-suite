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
