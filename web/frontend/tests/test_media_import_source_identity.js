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

console.log('MT_META71_SOURCE_IDENTITY=PASS');
