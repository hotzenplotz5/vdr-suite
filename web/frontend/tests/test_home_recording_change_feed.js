'use strict';
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

class Element {
  constructor() { this.children = []; this.attributes = {}; this.dataset = {}; this.style = {}; }
  appendChild(node) { node.parentNode = this; this.children.push(node); return node; }
  append(...nodes) { nodes.forEach(node => this.appendChild(node)); }
  replaceChildren(...nodes) { this.children = []; this.append(...nodes); }
  insertBefore(node, before) {
    node.parentNode = this;
    const index = this.children.indexOf(before);
    if (index < 0) this.children.push(node); else this.children.splice(index, 0, node);
  }
  setAttribute(key, value) { this.attributes[key] = value; }
  addEventListener() {}
  remove() { if (this.parentNode) this.parentNode.children = this.parentNode.children.filter(n => n !== this); }
  querySelector(selector) {
    const match = selector.match(/data-home-discovery-rail="([^"]+)"/);
    if (!match) return null;
    return this.children.find(node => node.attributes['data-home-discovery-rail'] === match[1]) || null;
  }
  querySelectorAll() { return []; }
}
const host = new Element();
const listeners = {};
const timers = new Map();
const sources = [];
let timerId = 0;
let backend = 'A';
let moduleName = 'overview';
let canonicalBackendReady = false;
let recordingReads = 0;
let folderReads = 0;
let genreReads = 0;
let seriesReads = 0;
const queuedRecordingResponses = [];
const queuedSeriesResponses = [];
const seriesRecordings = [{
  recordingId: 'andor-1', backendNativeId: '/recordings/andor-1',
  title: 'Serien/Andor/S01E01 Pilot'
}, {
  recordingId: 'foundation-1', backendNativeId: '/recordings/foundation-1',
  title: 'Serien/Foundation/S01E01 Start',
  metadata: {presentation: {posterUrl: '/foundation-poster.webp'}}
}];
const doc = {
  hidden: false,
  head: new Element(),
  createElement() { return new Element(); },
  getElementById() { return null; },
  querySelector(selector) {
    if (selector === '[data-home-zone="additional-sections"]') return host;
    if (selector === '#backends .backend-card.selected, #backends [aria-selected="true"]') {
      return canonicalBackendReady ? {dataset: {backendId: backend}} : null;
    }
    return null;
  },
  addEventListener(type, listener) { (listeners[type] ||= []).push(listener); }
};
const client = {
  createClientLiveUpdateSource() {
    const source = {
      handlers: {}, closed: false,
      addEventListener(type, listener) { this.handlers[type] = listener; },
      close() { this.closed = true; },
      emit(sequenceNumber, backendId = backend, changedDomains = ['recordings'],
           recordingNativeIds = []) {
        this.handlers.update({data: JSON.stringify({
          sequenceNumber, backendId, changedDomains, recordingNativeIds
        })});
      }
    };
    sources.push(source);
    return source;
  },
  fetchClientRecordings(options) {
    recordingReads++;
    assert.strictEqual(options.cache, 'no-store');
    assert.strictEqual(options.query.backend, backend);
    if (queuedRecordingResponses.length) {
      return queuedRecordingResponses.shift()();
    }
    return Promise.resolve({recordings: [{
      recordingId: 'import-' + recordingReads, backendId: backend,
      backendNativeId: '/recordings/import-' + recordingReads,
      title: 'Imported movie ' + recordingReads
    }, {
      recordingId: 'unrelated', backendId: backend,
      backendNativeId: '/recordings/unrelated',
      title: 'Unrelated movie',
      metadata: {presentation: {posterUrl: '/unrelated.jpg'}}
    }]});
  },
  fetchClientRecordingFolder(options) {
    folderReads++;
    assert.strictEqual(options.backendId, backend);
    return Promise.resolve({folders: [], recordings: []});
  },
  fetchClientGenres() { genreReads++; return Promise.resolve({genres: []}); },
  fetchClientGenreRecordings(options) {
    seriesReads++;
    assert.strictEqual(options.backendId, backend);
    assert.strictEqual(options.genreId, 'series');
    assert.strictEqual(options.cache, 'no-store');
    if (queuedSeriesResponses.length) return queuedSeriesResponses.shift()();
    return Promise.resolve({recordings: seriesRecordings.map(recording => ({
      ...recording, backendId: backend
    })), total: seriesRecordings.length});
  }
};
const window = {
  document: doc, console,
  setTimeout(fn) { const id = ++timerId; timers.set(id, fn); return id; },
  clearTimeout(id) { timers.delete(id); },
  VdrSuitePlatform: {
    getClientApi() { return client; },
    getSelectedModule() { return moduleName; },
    getSelectedBackendId() { return backend; }
  }
};
window.window = window;
const productionContext = vm.createContext({window, console});
vm.runInContext(
  fs.readFileSync(path.join(__dirname, '..', 'platform', 'helpers.js'), 'utf8'),
  productionContext,
  {filename: 'platform/helpers.js'}
);
vm.runInContext(
  fs.readFileSync(path.join(__dirname, '..', 'home-recording-discovery.js'), 'utf8'),
  productionContext,
  {filename: 'home-recording-discovery.js'}
);

const settle = () => new Promise(resolve => setImmediate(resolve));
async function tick() {
  const ready = [...timers.values()]; timers.clear();
  ready.forEach(fn => fn());
  for (let i = 0; i < 4; i++) await settle();
}
function visibility(hidden) { doc.hidden = hidden; (listeners.visibilitychange || []).forEach(fn => fn()); }
function navigate(next) {
  moduleName = next;
  const target = {
    dataset: {module: next},
    closest(selector) {
      if (selector === '.module-tab[data-module], [data-brand-module]') return this;
      return next === 'overview' && selector.includes('overview') ? this : null;
    }
  };
  (listeners.click || []).forEach(fn => fn({target}));
}

(async () => {
  // Production shell selects the canonical backend before publishing Home resume.
  // Recording Discovery must start immediately from that lifecycle event; no
  // viewport/IntersectionObserver gate is allowed.
  canonicalBackendReady = true;
  (listeners['vdr-suite:home-resume'] || []).forEach(fn => fn({
    detail: {backendId: backend}
  }));
  await tick();
  assert.strictEqual(sources.length, 1, 'Home must subscribe through the production lifecycle');
  assert.strictEqual(recordingReads, 1);
  const first = sources[0];
  first.onopen();
  first.emit(1, 'B');
  first.emit(2, 'A', ['recordingMarks']);
  first.handlers.update({data: 'bad-json'});
  await tick();
  assert.strictEqual(recordingReads, 1);
  first.emit(3);
  first.emit(4);
  first.emit(4);
  await tick();
  assert.strictEqual(recordingReads, 2, 'same-backend recording burst invalidates retained Home once');
  assert.strictEqual(folderReads, 1,
    'recording SSE must not reload the unrelated Home folders rail');
  const flatten = node => [node, ...node.children.flatMap(flatten)];
  assert(flatten(host).some(node => node.dataset.recordingId === 'import-2'),
    'native import hint reaches the actual Home card projection');
  await window.VdrSuiteHomeRecordingDiscovery._test.refreshForHome();
  assert.strictEqual(recordingReads, 2, 'unchanged Home remains retained');

  let resolveStaleRead;
  queuedRecordingResponses.push(() => new Promise(resolve => {
    resolveStaleRead = resolve;
  }));
  queuedRecordingResponses.push(() => Promise.resolve({recordings: [{
    recordingId: 'manual-current',
    backendId: backend,
    backendNativeId: '/recordings/manual-current',
    title: 'Manual current',
    metadata: {
      presentation: {posterUrl: '/manual/current.webp'},
      artwork: {preferredUrl: '/manual/current.webp'}
    }
  }]}));
  first.emit(5);
  await tick();
  first.emit(6);
  await tick();
  assert.strictEqual(recordingReads, 4,
    'a newer recording invalidation must start without waiting for a slow older generation');
  assert(flatten(host).some(node => node.dataset.recordingId === 'manual-current'),
    'newer presentation generation must publish while an older request is still pending');
  assert(flatten(host).some(node => node.src === '/manual/current.webp'),
    'newer manual artwork must be visible before the stale request finishes');

  resolveStaleRead({recordings: [{
    recordingId: 'stale-old',
    backendId: backend,
    backendNativeId: '/recordings/stale-old',
    title: 'Stale old',
    metadata: {
      presentation: {posterUrl: '/stale/old.webp'},
      artwork: {preferredUrl: '/stale/old.webp'}
    }
  }]});
  await settle();
  await tick();
  assert.strictEqual(recordingReads, 4,
    'stale completion must not trigger another serialized catch-up refresh');
  assert(!flatten(host).some(node => node.dataset.recordingId === 'stale-old'),
    'older generation must not overwrite newer recording presentation');
  assert(flatten(host).some(node => node.src === '/manual/current.webp'),
    'older generation must not overwrite newer manual artwork');
  assert(!flatten(host).some(node => node.src === '/stale/old.webp'),
    'stale artwork must never replace the newer manual artwork');

  visibility(true);
  assert(first.closed);
  first.emit(7);
  await tick();
  assert.strictEqual(recordingReads, 4, 'closed source cannot refresh hidden Home');
  visibility(false);
  await tick();
  assert.strictEqual(sources.length, 2);
  assert.strictEqual(recordingReads, 5, 'visibility recovery catches up without manual refresh');
  const second = sources[1];
  second.onopen();
  second.emit(20);
  await tick();
  const beforeReconnect = recordingReads;
  second.onerror();
  await tick();
  assert.strictEqual(recordingReads, beforeReconnect, 'normal finite-feed close does not reload Home');
  second.onopen();
  second.emit(1);
  second.onerror();
  await tick();
  assert.strictEqual(recordingReads, beforeReconnect + 1, 'daemon sequence reset catches up');

  navigate('recordings2');
  assert(second.closed);
  await tick();
  const beforeExit = recordingReads;
  second.emit(99);
  await tick();
  assert.strictEqual(recordingReads, beforeExit);
  backend = 'B';
  navigate('overview');
  (listeners['vdr-suite:home-resume'] || []).forEach(fn => fn({
    detail: {backendId: backend}
  }));
  await tick();
  await tick();
  const third = sources[sources.length - 1];
  assert(!third.closed);
  const beforeBackend = recordingReads;
  third.onopen();
  third.emit(100, 'A');
  await tick();
  assert.strictEqual(recordingReads, beforeBackend, 'old backend does not invalidate current Home');
  third.emit(101, 'B');
  await tick();
  assert.strictEqual(recordingReads, beforeBackend + 1);

  const selectedNativeId = '/recordings/import-' + recordingReads;
  const unrelatedBefore = flatten(host).find(node => node.dataset.recordingId === 'unrelated');
  assert(unrelatedBefore);
  const unrelatedPosterBefore = flatten(unrelatedBefore).find(node => node.src === '/unrelated.jpg');
  assert(unrelatedPosterBefore);
  const readsBeforeScoped = recordingReads;
  const foldersBeforeScoped = folderReads;
  const targetedRequests = [];
  client.requestJson = (route, options) => {
    targetedRequests.push({route, options});
    return Promise.resolve({
      available: true, title: 'Corrected imported movie', mediaType: 'movie',
      preferredArtwork: {available: true, url: '/corrected/new-cover.webp'},
      images: []
    });
  };
  third.emit(102, 'B', ['recordings'], [selectedNativeId]);
  await tick();
  assert.strictEqual(recordingReads, readsBeforeScoped,
    'recording-scoped SSE must not refetch the complete recording collection');
  assert.strictEqual(folderReads, foldersBeforeScoped,
    'recording-scoped SSE must not reload Home folders');
  const scopedMetadataRequests = targetedRequests.filter(entry =>
    entry.route === '/api/vdr/recordings/metadata' &&
    entry.options.query.backendNativeId === selectedNativeId);
  assert.strictEqual(scopedMetadataRequests.length, 1,
    'scoped SSE reads its exact native Recording metadata once');
  const unrelatedAfter = flatten(host).find(node => node.dataset.recordingId === 'unrelated');
  assert.strictEqual(unrelatedAfter, unrelatedBefore,
    'scoped SSE must preserve the unrelated Recording card object');
  assert.strictEqual(
    flatten(unrelatedAfter).find(node => node.src === '/unrelated.jpg'),
    unrelatedPosterBefore,
    'scoped SSE must not touch or reload another Recording image');
  assert(flatten(host).some(node => node.src === '/corrected/new-cover.webp'),
    'recording-scoped SSE updates the existing tile artwork directly');

  // Series revalidation shares the existing feed but never remounts Home.
  const seriesSection = host.querySelector('[data-home-discovery-rail="series"]');
  assert(seriesSection, 'initial Home includes canonical Series');
  const seriesRail = seriesSection.children.find(node =>
    node.className === 'media-home-discovery-rail series');
  assert(seriesRail);
  const andor = () => seriesRail.children.find(card =>
    card.__vdrSuiteSeries && card.__vdrSuiteSeries.title === 'Andor');
  const foundation = () => seriesRail.children.find(card =>
    card.__vdrSuiteSeries && card.__vdrSuiteSeries.title === 'Foundation');
  const andorCard = andor();
  const foundationCard = foundation();
  assert(andorCard && foundationCard);
  const foundationArtwork = foundationCard.children[0];
  seriesRail.scrollLeft = 245;
  const beforeSeriesUpdate = seriesReads;
  const beforeOtherHome = [recordingReads, genreReads, folderReads];
  seriesRecordings.push({
    recordingId: 'andor-2', backendNativeId: '/recordings/andor-2',
    title: 'Serien/Andor/S01E02 Zweite Folge'
  });
  third.emit(103, 'B', ['recordings']);
  await tick();
  assert.strictEqual(seriesReads, beforeSeriesUpdate + 1,
    'inventory SSE revalidates canonical Series once');
  assert.strictEqual(andor(), andorCard,
    'new episode updates existing Series button rather than remounting it');
  assert.strictEqual(andor().__vdrSuiteSeries.episodes.length, 2);
  assert.strictEqual(foundation(), foundationCard,
    'unrelated Series button is unchanged');
  assert.strictEqual(foundationCard.children[0], foundationArtwork,
    'unrelated poster node remains untouched');
  assert.strictEqual(seriesRail.scrollLeft, 245,
    'Series scroll position remains stable');
  assert.strictEqual(genreReads, beforeOtherHome[1], 'SSE does not reload Genres');
  assert.strictEqual(folderReads, beforeOtherHome[2], 'SSE does not reload Folders');
  assert.strictEqual(recordingReads, beforeOtherHome[0] + 1,
    'inventory SSE still reconciles Newly Recorded once');

  const andorSeries = andor().__vdrSuiteSeries;
  const season = andorSeries.seasons.find(entry => entry.number === 1);
  assert(season);
  assert.strictEqual(window.VdrSuiteHomeRecordingDiscovery._test.renderSeriesDetail(
    andorSeries, season, 'B'), true);
  const detail = seriesSection.__vdrSuiteSeriesDetail;
  const episodeRail = detail.episodeRail;
  const seasonRail = detail.seasonRail;
  episodeRail.scrollLeft = 93;
  seasonRail.scrollLeft = 48;
  const firstEpisodeCard = flatten(episodeRail).find(node => node.dataset.recordingId === 'andor-1');
  assert(firstEpisodeCard);

  seriesRecordings.push({
    recordingId: 'andor-3', backendNativeId: '/recordings/andor-3',
    title: 'Serien/Andor/S01E03 Dritte Folge'
  });
  third.emit(104, 'B', ['recordings']);
  await tick();
  assert.strictEqual(detail.episodeRail, episodeRail, 'open episode rail is not remounted');
  assert.strictEqual(detail.seasonRail, seasonRail, 'open season rail is not remounted');
  assert.strictEqual(detail.selectedSeason.number, 1);
  assert.strictEqual(episodeRail.scrollLeft, 93);
  assert.strictEqual(seasonRail.scrollLeft, 48);
  assert.strictEqual(
    flatten(episodeRail).find(node => node.dataset.recordingId === 'andor-1'),
    firstEpisodeCard, 'unchanged existing episode card retains identity');
  assert(flatten(episodeRail).some(node => node.dataset.recordingId === 'andor-3'),
    'new episode appears in the already open season');

  // A failed read retains the last complete Series projection.
  queuedSeriesResponses.push(() => Promise.reject(new Error('503')));
  third.emit(105, 'B', ['recordings']);
  await tick();
  assert(flatten(episodeRail).some(node => node.dataset.recordingId === 'andor-3'),
    'failed SSE read must not clear open Series detail');

  // A slow older read may not undo a newer confirmed Series snapshot.
  let releaseOldSeries;
  queuedSeriesResponses.push(() => new Promise(resolve => { releaseOldSeries = resolve; }));
  seriesRecordings.push({
    recordingId: 'andor-4', backendNativeId: '/recordings/andor-4',
    title: 'Serien/Andor/S01E04 Vierte Folge'
  });
  third.emit(106, 'B', ['recordings']);
  await tick();
  third.emit(107, 'B', ['recordings']);
  await tick();
  assert(flatten(episodeRail).some(node => node.dataset.recordingId === 'andor-4'),
    'newer Series generation must render without waiting for older read');
  releaseOldSeries({recordings: [{
    recordingId: 'obsolete', backendId: 'B', backendNativeId: '/recordings/obsolete',
    title: 'Serien/Andor/S01E99 Alt'
  }], total: 1});
  await tick();
  assert(!flatten(episodeRail).some(node => node.dataset.recordingId === 'obsolete'),
    'stale Series generation must not overwrite new episodes');


  // A native-id metadata change updates only the affected Series presentation.
  let changedSeriesMetadataReads = 0;
  client.requestJson = (route, options) => {
    if (route === '/api/vdr/recordings/metadata' &&
        options.query.backendNativeId === '/recordings/andor-1') {
      changedSeriesMetadataReads++;
      return Promise.resolve({
        available: true, provider: 'manual', providerId: 12345,
        mediaType: 'episode', title: 'Andor', episodeName: 'Korrigierter Episodentitel',
        seasonNumber: 1, episodeNumber: 1,
        preferredArtwork: {available: true, url: '/andor-new-cover.webp'},
        images: []
      });
    }
    return Promise.resolve({available: false});
  };
  const foundationCardBeforeMetadata = foundationCard;
  third.emit(108, 'B', ['recordings'], ['/recordings/andor-1']);
  await tick();
  assert.strictEqual(changedSeriesMetadataReads, 1,
    'Series SSE reads only changed native metadata');
  assert(flatten(episodeRail).some(node =>
    typeof node.textContent === 'string' &&
    node.textContent.includes('Korrigierter Episodentitel')),
    'updated native episode title appears in the retained open season');
  assert.strictEqual(foundation(), foundationCardBeforeMetadata,
    'metadata correction does not replace unrelated Series cards');

  console.log('Home recording and Series SSE lifecycle: PASS');
})().catch(error => { console.error(error); process.exitCode = 1; });
