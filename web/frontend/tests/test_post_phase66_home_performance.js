'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(
  path.join(frontendRoot, 'home-live-hero.js'),
  'utf8'
);
const recentMoviesBundle = fs.readFileSync(
  path.join(frontendRoot, 'home-recently-watched.js'),
  'utf8'
);
const genreControllerSource = fs.readFileSync(
  path.join(frontendRoot, '..', '..', 'api', 'rest', 'src', 'GenreBrowserController.cpp'),
  'utf8'
);

function between(startMarker, endMarker) {
  const start = source.indexOf(startMarker);
  const end = source.indexOf(endMarker, start + startMarker.length);
  assert(start >= 0, 'missing source marker: ' + startMarker);
  assert(end > start, 'missing source end marker: ' + endMarker);
  return source.slice(start, end);
}

assert(source.includes('eventsByChannel: new Map()'));
assert(source.includes('function rebuildEventIndex()'));
assert(source.includes('state.eventsByChannel = index;'));

const channelEventsSource = between(
  'function channelEvents(channel, events)',
  'function currentEventForChannel('
);
assert(channelEventsSource.includes('events === state.events'));
assert(channelEventsSource.includes('state.eventsByChannel.get(id) || []'));

const applyProgramsSource = between(
  'function applyPrograms(data, append)',
  'function loadProgrammePage('
);
assert.strictEqual(
  (applyProgramsSource.match(/rebuildEventIndex\(\)/g) || []).length,
  2,
  'both reset and append EPG updates must rebuild the channel index'
);

const programmeRailSource = between(
  'function renderProgrammeRail(kind, title, current)',
  'function renderProgrammeRails()'
);
const railMountIndex = programmeRailSource.indexOf('section.appendChild(rail);');
const scrollRestoreIndex = programmeRailSource.indexOf('rail.scrollLeft = previousScrollLeft;');
assert(railMountIndex >= 0, 'programme rail must be mounted into its section');
assert(scrollRestoreIndex >= 0, 'programme rail must restore its previous scroll position');
assert(
  scrollRestoreIndex > railMountIndex,
  'programme rail scroll position must be restored after DOM mount so real browsers do not clamp it back to zero'
);

const renderSource = between(
  'function render(options)',
  'function selectIndex('
);
assert(renderSource.includes("if (config.programmeRails !== false) renderProgrammeRails();"));

const selectSource = between(
  'function selectIndex(index)',
  'function selectOffset('
);
assert(selectSource.includes('render({programmeRails: false});'));
assert(!selectSource.includes('renderProgrammeRails'));

assert(source.includes('const PROGRAMME_WARM_REUSE_MS = 60000;'));
assert(source.includes('programmeLoadedAt: 0'));
const clearProgramsSource = between(
  'function clearPrograms()',
  'function channelEvents('
);
assert(clearProgramsSource.includes('state.programmeLoadedAt = 0;'));
const programmeLoadSource = between(
  'function loadProgrammePage(sequence, offset, reset)',
  'function loadPrograms('
);
assert(programmeLoadSource.includes('state.programmeLoadedAt = Date.now();'));
const loadSource = between(
  'function load(force)',
  'function sync(force)'
);
assert(loadSource.includes('const reuseWarmPrograms = state.events.length > 0'));
assert(loadSource.includes('Date.now() - state.programmeLoadedAt <= PROGRAMME_WARM_REUSE_MS'));
assert(loadSource.includes('render({programmeRails: !reuseWarmPrograms});'));
assert(loadSource.includes('if (reuseWarmPrograms) return Promise.resolve(null);'));

assert(source.includes('function clearPrograms()'));
assert(source.includes('state.eventsByChannel = new Map();'));

const recentMoviesMarker = '// Bounded Phase-66 Recording Discovery follow-up.';
const recentMoviesMarkerIndex = recentMoviesBundle.indexOf(recentMoviesMarker);
assert(recentMoviesMarkerIndex >= 0, 'recent movies projection marker must exist');
const recentMoviesSource = recentMoviesBundle.slice(recentMoviesMarkerIndex);
assert(recentMoviesSource.includes("text(provider(recording).contentKind) !== 'movie'"));
assert(recentMoviesSource.includes('provider(recording).releaseDate'));
assert(recentMoviesSource.includes('fetchClientRecordings({'));
assert(!recentMoviesSource.includes('fetchClientGenreRecordings({'));
assert(
  genreControllerSource.includes('json << "},\\\"provider\\\":{},\\\"native\\\":{},\\\"artwork\\\":{\\\"preferredUrl\\\":";'),
  'genre recording projection must be recognized as metadata-light while provider fields remain empty'
);

console.log('post-phase66 Home hero performance hotpath contract ok');
