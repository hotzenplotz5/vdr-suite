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
  'function loadProgrammeArtworkPage('
);
assert.strictEqual(
  (applyProgramsSource.match(/rebuildEventIndex\(\)/g) || []).length,
  3,
  'reset, append and retained-page EPG updates must rebuild the channel index'
);
assert(applyProgramsSource.includes('const previousSignature = programmeSignature(state.events);'));
assert.strictEqual(
  (applyProgramsSource.match(/programmeSignature\(state\.events\) !== previousSignature/g) || []).length,
  3,
  'all programme application modes must report whether the visible projection actually changed'
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
  'function loadProgrammePage(sequence, offset, reset, options)',
  'function loadPrograms(sequence, options)'
);
assert(programmeLoadSource.includes('state.programmeLoadedAt = Date.now();'));
assert(programmeLoadSource.includes('const programmesChanged ='));
assert(programmeLoadSource.includes('config.renderUnchanged !== false'));
assert(programmeLoadSource.includes('const preserveProjection ='));
assert(programmeLoadSource.includes('heroHasProgrammeProjection()'));
assert(programmeLoadSource.includes('(config.retainVisible === true || !reset)'));
assert(programmeLoadSource.includes('if (preserveProjection) refreshHeroNotice();'));
assert(
  !programmeLoadSource.includes("if (!reset || config.retainVisible !== true) render();"),
  'retained/incremental programme failures must not force a full Hero render'
);
const loadSource = between(
  'function load(force, options)',
  'function sync(force, options)'
);
assert(loadSource.includes('const reuseWarmPrograms = state.events.length > 0'));
assert(loadSource.includes('Date.now() - state.programmeLoadedAt <= PROGRAMME_WARM_REUSE_MS'));
assert(loadSource.includes('render({programmeRails: !reuseWarmPrograms});'));
assert(loadSource.includes('if (reuseWarmPrograms) return Promise.resolve(null);'));
assert(loadSource.includes('const revalidatePrograms = config.revalidatePrograms === true;'));
assert(loadSource.includes('renderUnchanged: !revalidatePrograms'));
const syncSource = between(
  'function sync(force, options)',
  'function scheduleSync(force, options)'
);
assert(syncSource.includes('config.revalidatePrograms === true'));
assert(source.includes('scheduleSync(false, {'));
assert(source.includes('retainVisible: true'));
assert(source.includes('revalidatePrograms: true'));

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
