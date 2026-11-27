'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const homeSource = fs.readFileSync(path.join(frontendRoot, 'home-recently-watched.js'), 'utf8');
const bootstrapSource = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery-bootstrap.js'), 'utf8');
const discoverySource = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8');
const marker = '// Bounded Phase-66 Recording Discovery follow-up.';
const markerIndex = homeSource.indexOf(marker);
assert(markerIndex >= 0);
const source = homeSource.slice(markerIndex);

assert(source.includes("const TITLE = 'Filme der letzten 5 Jahre';"));
assert(source.includes('const WARM_RETURN_MS = 60000;'));
assert(source.includes("text(provider(recording).contentKind) !== 'movie'"));
assert(source.includes('provider(recording).releaseDate'));
assert(source.includes('year >= nowYear - 4'));
assert(source.includes('year <= nowYear'));
assert(source.includes('new Date().getFullYear()'));
assert(source.includes('fetchClientRecordings'));
assert(!source.includes('fetchClientGenreRecordings'));
assert(source.includes('owner.canonicalRecordings'));
assert(source.includes('owner.recordingPosterUrl'));
assert(source.includes('owner.openRecording'));
assert(source.includes('function refreshForHome()'));
assert(source.includes('if (state.loadingPromise && state.loadingBackendId === backendId)'));
assert(source.indexOf('if (state.loadingPromise && state.loadingBackendId === backendId)') <
  source.indexOf('const generation = ++state.generation;'));
assert(source.includes('if (warmForBackend(backendId)) return Promise.resolve(true);'));
assert(source.includes("target.querySelector('[data-home-discovery-rail=\"series\"]')"));
assert(source.includes("rail.className = 'media-home-discovery-rail recent-movies'"));
assert(source.includes("const HOME_RAIL_NEAR_END_EVENT = 'vdr-suite-home-rail-near-end';"));
assert(source.includes('loadMoreMovies'));
assert(source.includes('rail.appendChild(createMovieCard(recording, state.backendId));'));
assert(!source.includes('/api/vdr/recordings/metadata'));
assert(!source.includes('requestJson('));
assert(!source.includes('MediaSession'));
assert(!source.toLowerCase().includes('recommend'));
assert(discoverySource.includes('VdrSuiteRecordings2.openRecording'));
assert(bootstrapSource.includes("const RAIL_SELECTOR = '.media-home-discovery-rail, .media-home-series-season-rail, .media-home-live-guide-rail';"));
assert(bootstrapSource.includes('scrollbar-width:none'));

const renderStart = source.indexOf('function render(recordings, backendId, visibleLimit)');
const renderEnd = source.indexOf('\n  function renderState(', renderStart);
assert(renderStart >= 0 && renderEnd > renderStart);
const renderSource = source.slice(renderStart, renderEnd);
assert(
  renderSource.indexOf('target.appendChild(rail);') < renderSource.indexOf('rail.scrollLeft = previousScrollLeft;'),
  'recent movies scroll position must be restored after initial mounting'
);
assert(!renderSource.includes('target.replaceChildren();'),
  'same-backend refresh must not unmount the existing rail');

class FakeElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.attributes = {};
    this.dataset = {};
    this.listeners = {};
    this.parentNode = null;
    this.nextElementSibling = null;
    this.className = '';
    this.textContent = '';
    this.src = '';
    this.alt = '';
    this.loading = '';
    this.decoding = '';
    this.fetchPriority = '';
    this.type = '';
    this.scrollLeft = 0;
  }

  syncSiblings() {
    this.children.forEach((child, index) => {
      child.nextElementSibling = this.children[index + 1] || null;
    });
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }

  appendChild(child) {
    if (child.parentNode) child.parentNode.removeChild(child);
    child.parentNode = this;
    this.children.push(child);
    this.syncSiblings();
    return child;
  }

  append() {
    Array.from(arguments).forEach((child) => this.appendChild(child));
  }

  removeChild(child) {
    const index = this.children.indexOf(child);
    if (index >= 0) this.children.splice(index, 1);
    child.parentNode = null;
    this.syncSiblings();
    return child;
  }

  insertBefore(child, before) {
    if (child.parentNode) child.parentNode.removeChild(child);
    const index = this.children.indexOf(before);
    if (index < 0) return this.appendChild(child);
    child.parentNode = this;
    this.children.splice(index, 0, child);
    this.syncSiblings();
    return child;
  }

  replaceChildren() {
    this.children.forEach((child) => { child.parentNode = null; });
    this.children = [];
    Array.from(arguments).forEach((child) => this.appendChild(child));
    this.syncSiblings();
  }

  addEventListener(type, handler) {
    if (!this.listeners[type]) this.listeners[type] = [];
    this.listeners[type].push(handler);
  }

  remove() {
    if (this.parentNode) this.parentNode.removeChild(this);
  }

  querySelector(selector) {
    if (selector === '.media-home-discovery-rail.recent-movies') {
      return findElement(this, (element) =>
        String(element.className || '').split(/\s+/).includes('media-home-discovery-rail') &&
        String(element.className || '').split(/\s+/).includes('recent-movies'));
    }
    const match = String(selector || '').match(/^\[data-home-discovery-rail="([^"]+)"\]$/);
    if (!match) return null;
    return findElement(this, (element) =>
      element.attributes['data-home-discovery-rail'] === match[1]);
  }
}

function findElement(root, predicate) {
  for (const child of root.children || []) {
    if (predicate(child)) return child;
    const nested = findElement(child, predicate);
    if (nested) return nested;
  }
  return null;
}

function findElements(root, predicate, found) {
  const result = found || [];
  for (const child of root.children || []) {
    if (predicate(child)) result.push(child);
    findElements(child, predicate, result);
  }
  return result;
}

function movie(id, releaseDate, startTime, title) {
  return {
    recordingId: id,
    backendId: 'default',
    startTime: startTime || '',
    title: title || id,
    metadata: {
      provider: {
        contentKind: 'movie',
        releaseDate,
        title: title || id
      },
      presentation: {
        title: title || id,
        posterUrl: '/poster/' + id + '.jpg'
      }
    }
  };
}

function nonMovie(id, contentKind, releaseDate) {
  return {
    recordingId: id,
    backendId: 'default',
    metadata: {provider: {contentKind, releaseDate}}
  };
}

const currentYear = new Date().getFullYear();
const host = new FakeElement('div');
const calls = [];
const opened = [];
const currentYearMovies = Array.from({length: 12}, (_, index) => {
  const day = String(index + 1).padStart(2, '0');
  return movie('current-' + day, String(currentYear) + '-05-' + day, '2026-05-' + day + 'T20:00:00', 'Aktuell ' + day);
});
const allRecordings = [
  movie('newest', String(currentYear) + '-06-01', '2026-07-01T20:00:00', 'Neu'),
  ...currentYearMovies.slice().reverse(),
  movie('boundary', String(currentYear - 4), '2025-04-01T20:00:00', 'Grenze')
];
let failNext = false;
const client = {
  fetchClientRecordings(request) {
    calls.push(request);
    const offset = Number(request.query.offset || 0);
    if (failNext) { failNext = false; return Promise.reject(new Error('unavailable')); }
    const recordings = allRecordings.slice(offset, offset + request.query.limit);
    return Promise.resolve({
      recordings,
      totalCount: allRecordings.length,
      limit: request.query.limit,
      offset
    });
  }
};

const document = {
  readyState: 'loading',
  querySelector(selector) {
    return selector === '[data-home-zone="additional-sections"]' ? host : null;
  },
  createElement(tagName) {
    return new FakeElement(tagName);
  },
  addEventListener() {}
};

const window = {
  document,
  setTimeout() { return 1; },
  VdrSuitePlatform: {
    getSelectedBackendId() { return 'default'; },
    getSelectedModule() { return 'overview'; },
    getClientApi() { return client; }
  },
  VdrSuiteHomeRecordingDiscovery: {
    _test: {
      canonicalRecordings(payload, backendId) {
        return (payload.recordings || []).filter((recording) =>
          recording.recordingId && recording.backendId === backendId);
      },
      recordingPosterUrl(recording) {
        return recording.metadata.presentation.posterUrl;
      },
      openRecording(recording, backendId) {
        opened.push({recording, backendId});
        return Promise.resolve(true);
      }
    }
  },
  VdrSuitePublicUrl: {resolvePath(value) { return value; }},
  VdrSuiteBrowserSession: {isAuthenticated() { return true; }}
};
window.window = window;

const context = vm.createContext({
  window,
  document,
  console,
  Promise,
  Date,
  Object,
  Number,
  Math,
  String,
  Boolean,
  Array,
  Set
});
vm.runInContext(source, context, {filename: 'home-recently-watched.js#recent-movies'});
const api = window.VdrSuiteHomeRecentMovies;
assert(api && api._test);

assert.strictEqual(api._test.releaseYear(movie('valid-date', String(currentYear) + '-01-02')), currentYear);
assert.strictEqual(api._test.releaseYear(movie('valid-year', String(currentYear))), currentYear);
assert.strictEqual(api._test.releaseYear(movie('bad-date', String(currentYear) + '-02-30')), 0);
assert.strictEqual(api._test.recentMovie(movie('inside', String(currentYear - 4) + '-01-01'), currentYear), true);
assert.strictEqual(api._test.recentMovie(movie('too-old', String(currentYear - 5) + '-12-31'), currentYear), false);
assert.strictEqual(api._test.recentMovie(movie('too-new', String(currentYear + 1) + '-01-01'), currentYear), false);
assert.strictEqual(api._test.recentMovie(nonMovie('episode', 'series-episode', String(currentYear)), currentYear), false);
assert.strictEqual(api._test.recentMovie(nonMovie('unclassified', 'unknown', String(currentYear)), currentYear), false);
assert.strictEqual(api._test.recentMovie(movie('missing-year', ''), currentYear), false);

(async function () {
  const initialLoad = api.refresh();
  const duplicateDirectLoad = api.refresh({retainVisible: true});
  const coalescedLoad = api._test.refreshForHome();
  assert.strictEqual(
    calls.length,
    1,
    'direct Home-resume and observer triggers must coalesce with an in-flight same-backend scan'
  );
  await Promise.all([initialLoad, duplicateDirectLoad, coalescedLoad]);

  assert.deepStrictEqual(calls.map((call) => call.query.offset), [0]);
  assert(calls.every((call) => call.query.limit === 12));
  assert(calls.every((call) => call.query.sort === 'movieReleaseDate' && call.query.order === 'desc'));
  assert(calls.every((call) => call.query.backend === 'default'));
  assert(calls.every((call) => call.query.movieReleaseYearFrom === new Date().getFullYear() - 4));
  assert(calls.every((call) => call.query.movieReleaseYearTo === new Date().getFullYear()));
  assert(calls.every((call) => call.cache === 'no-store'));
  assert(calls.every((call) => call.credentials === 'same-origin'));
  assert.strictEqual(api._test.warmForBackend('default'), true);

  const callsAfterInitialLoad = calls.length;
  assert.strictEqual(await api._test.refreshForHome(), true);
  assert.strictEqual(calls.length, callsAfterInitialLoad, 'warm Home return must not restart the full recording scan');

  const movieSection = host.querySelector('[data-home-discovery-rail="recent-movies"]');
  assert(movieSection);
  const seriesSection = new FakeElement('section');
  seriesSection.setAttribute('data-home-discovery-rail', 'series');
  host.appendChild(seriesSection);
  assert.strictEqual(api._test.positionBeforeSeries(), true);
  assert.strictEqual(movieSection.nextElementSibling, seriesSection);

  const heading = findElement(movieSection, (element) => element.tagName === 'H3');
  assert(heading);
  assert.strictEqual(heading.textContent, 'Filme der letzten 5 Jahre');

  let cards = findElements(movieSection, (element) =>
    element.dataset && Boolean(element.dataset.recordingId));
  assert.strictEqual(cards.length, 12);
  assert(cards.every((card) => Number(card.dataset.movieYear) === currentYear));
  assert.strictEqual(findElement(movieSection, (element) => element.dataset.recordingId === 'boundary'), null);

  const initialRail = findElement(movieSection, (element) =>
    String(element.className || '').split(/\s+/).includes('recent-movies'));
  assert(initialRail);
  initialRail.scrollLeft = 777;
  failNext = true;
  assert.strictEqual(await api._test.loadMoreMovies(), false);
  assert.strictEqual(initialRail.scrollLeft, 777);
  const moreButton = findElement(movieSection, element => element.tagName === 'BUTTON' && element.textContent === 'Weitere Filme');
  assert(moreButton, 'paging remains available when the rail cannot scroll');
  const more = moreButton.listeners.click[0]();
  assert.strictEqual(api._test.loadMoreMovies(), false, 'coalesce scrolling while a page is pending');
  assert.strictEqual(await more, true);
  assert.deepStrictEqual(calls.map(call => call.query.offset), [0, 12, 12]);
  assert.strictEqual(api._test.handleRailNearEnd({detail: {rail: initialRail}}), false);
  assert.strictEqual(moreButton.parentNode, null);
  cards = findElements(movieSection, (element) =>
    element.dataset && Boolean(element.dataset.recordingId));
  assert.strictEqual(cards.length, 14);
  assert.strictEqual(cards[cards.length - 1].dataset.recordingId, 'boundary');
  assert.strictEqual(Number(cards[cards.length - 1].dataset.movieYear), currentYear - 4);
  assert.strictEqual(new Set(cards.map((card) => card.dataset.recordingId)).size, cards.length);
  const expandedRail = findElement(movieSection, (element) =>
    String(element.className || '').split(/\s+/).includes('recent-movies'));
  assert.strictEqual(
    expandedRail,
    initialRail,
    'near-end expansion must preserve the existing rail DOM instance'
  );
  assert.strictEqual(expandedRail.scrollLeft, 777);
  assert.strictEqual(findElement(movieSection, (element) => element.dataset.recordingId === 'series'), null);
  assert.strictEqual(findElement(movieSection, (element) => element.dataset.recordingId === 'unknown'), null);
  assert.strictEqual(findElement(movieSection, (element) => element.dataset.recordingId === 'outside'), null);
  assert.strictEqual(findElement(movieSection, (element) => element.dataset.recordingId === 'no-year'), null);

  const image = findElement(movieSection, (element) => element.tagName === 'IMG');
  assert(image);
  assert.strictEqual(image.loading, 'lazy');
  assert.strictEqual(image.decoding, 'async');
  assert.strictEqual(image.fetchPriority, 'low');

  cards[0].listeners.click[0]();
  await Promise.resolve();
  assert.strictEqual(opened.length, 1);
  assert.strictEqual(opened[0].recording.recordingId, 'newest');
  assert.strictEqual(opened[0].backendId, 'default');

  // Same-backend refresh retains the existing rail, card and IMG identity.
  const retainedCard = cards[0];
  const otherCard = cards[1];
  const retainedImage = findElement(retainedCard, element => element.tagName === 'IMG');
  const revisedTitle = movie('newest', String(currentYear) + '-06-01',
    '2026-07-01T20:00:00', 'Neu umbenannt');
  allRecordings[0] = revisedTitle;
  await api.refresh();
  assert.strictEqual(findElement(movieSection, element =>
    String(element.className || '').split(' ').includes('recent-movies')), initialRail);
  assert.strictEqual(findElement(movieSection, element =>
    element.dataset.recordingId === 'newest'), retainedCard);
  assert.strictEqual(findElement(retainedCard, element => element.tagName === 'IMG'), retainedImage,
    'same cover URL must retain its original IMG node');
  assert.strictEqual(findElement(movieSection, element =>
    element.dataset.recordingId === 'current-12'), otherCard,
    'unmodified neighbors also keep their DOM identity');
  assert.strictEqual(findElement(retainedCard, element => element.tagName === 'STRONG').textContent,
    'Neu umbenannt');
  retainedCard.listeners.click[0]();
  await Promise.resolve();
  assert.strictEqual(opened.at(-1).recording, revisedTitle,
    'retained card opens the latest authoritative Recording object');
  const revisedArtwork = movie('newest', String(currentYear) + '-06-01',
    '2026-07-01T20:00:00', 'Neu umbenannt');
  revisedArtwork.metadata.presentation.posterUrl = '/poster/newest-revised.jpg';
  allRecordings[0] = revisedArtwork;
  await api.refresh();
  const refreshedImage = findElement(retainedCard, element => element.tagName === 'IMG');
  assert.notStrictEqual(refreshedImage, retainedImage, 'changed URL replaces only the affected artwork');
  assert.strictEqual(refreshedImage.src, '/poster/newest-revised.jpg');
  assert.strictEqual(findElement(movieSection, element =>
    element.dataset.recordingId === 'current-12'), otherCard,
    'unrelated movie cards are never re-rendered');

  // Composed production bootstrap and movie owner: only one speculative
  // page, no premature poster DOM, demand consumes the same response.
  const timers = [];
  window.setTimeout = fn => { timers.push(fn); return timers.length; };
  let fenceVersion = 1;
  window.VdrSuiteHomeCache = {
    begin() { return null; },
    captureRailFence() { const v = fenceVersion; return () => v === fenceVersion; }
  };
  vm.runInContext(bootstrapSource, context);
  const beforePrefetch = calls.length;
  await api.refresh();
  assert.strictEqual(calls.length, beforePrefetch + 1);
  assert.strictEqual(calls.at(-1).query.limit, 12);
  await timers.shift()();
  for (let i = 0; i < 12; ++i) await Promise.resolve();
  assert.strictEqual(calls.length, beforePrefetch + 2);
  assert.strictEqual(calls.at(-1).query.limit, 24);
  assert.strictEqual(calls.at(-1).query.offset, 12);
  assert.strictEqual(findElements(movieSection, el => Boolean(el.dataset && el.dataset.recordingId)).length, 12);
  assert.strictEqual(await api._test.loadMoreMovies(), true);
  assert.strictEqual(calls.length, beforePrefetch + 2, 'scroll consumes buffered response without another HTTP request');
  assert.strictEqual(findElements(movieSection, el => Boolean(el.dataset && el.dataset.recordingId)).length, 14);
  await api.refresh();
  await timers.shift()();
  for (let i = 0; i < 12; ++i) await Promise.resolve();
  ++fenceVersion;
  const beforeInvalidated = calls.length;
  assert.strictEqual(await api._test.loadMoreMovies(), true);
  assert.strictEqual(calls.length, beforeInvalidated + 1, 'invalidated prefetch requires an authorized demand read');
  // A demand arriving while lookahead is still in flight joins that read.
  await api.refresh();
  const originalFetch = client.fetchClientRecordings;
  let releasePrefetch;
  client.fetchClientRecordings = function (request) {
    if (request.query.limit !== 24) return originalFetch(request);
    calls.push(request);
    return new Promise(resolve => {
      releasePrefetch = () => resolve({recordings: allRecordings.slice(12), totalCount: allRecordings.length});
    });
  };
  await timers.shift()();
  for (let i = 0; i < 8; ++i) await Promise.resolve();
  assert.strictEqual(typeof releasePrefetch, 'function');
  const beforeJoin = calls.length;
  const joined = api._test.loadMoreMovies();
  assert.strictEqual(calls.length, beforeJoin);
  releasePrefetch();
  assert.strictEqual(await joined, true);
  assert.strictEqual(calls.length, beforeJoin);
  client.fetchClientRecordings = originalFetch;
  for (let i = 0; i < 12; ++i) await Promise.resolve();
  // Data Saver must never stop ordinary paging; it only suppresses lookahead.
  window.navigator = {onLine: true, connection: {saveData: true}};
  const beforeSaver = calls.length;
  await api.refresh();
  assert.strictEqual(calls.length, beforeSaver + 1);
  assert.strictEqual(timers.length, 0);
  assert.strictEqual(await api._test.loadMoreMovies(), true);
  assert.strictEqual(calls.length, beforeSaver + 2);
  assert.strictEqual(timers.length, 0);
  // Same film rail and DOM owner, now composed with the actual public JSON
  // client. Compare ordered public cards against the native baseline.
  const publicRequests = [];
  const publicRows = allRecordings.map((recording, index) => ({
    kind: 'recording', recordingId: 'rec_' + String(index + 1).padStart(32, '0'),
    backendId: 'default', title: recording.metadata.presentation.title,
    recordedAt: '2026-11-27T10:00:00Z',
    releaseDate: recording.metadata.provider.releaseDate,
    preferredArtworkId: 'a'.repeat(32)
  }));
  const wrapper = fs.readFileSync(path.join(frontendRoot, 'api/client-api.js'), 'utf8');
  vm.runInNewContext(wrapper, {window, URLSearchParams, fetch(url) {
    const parsed = new URL(url, 'https://test.invalid');
    publicRequests.push(parsed);
    assert.strictEqual(parsed.pathname, '/api/v1/recordings');
    assert.strictEqual(parsed.searchParams.get('view'), 'movies');
    assert.strictEqual(parsed.searchParams.get('backendId'), 'default');
    const offset = Number(parsed.searchParams.get('offset'));
    const limit = Number(parsed.searchParams.get('limit'));
    const items = publicRows.slice(offset, offset + limit);
    return Promise.resolve({ok:true, text:()=>Promise.resolve(JSON.stringify({
      items, page:{limit,offset,totalCount:publicRows.length,
        hasMore:offset+items.length<publicRows.length}, meta:{partial:false}
    }))});
  }});
  window.VdrSuitePlatform.getClientApi = () => window.VdrSuiteClientApi;
  window.VdrSuiteHomeRecordingDiscovery._test.recordingPosterUrl = recording =>
    '/api/v1/recordings/' + recording.recordingId + '/artwork/' +
      recording.preferredArtworkId + '?backendId=' + recording.backendId;
  vm.runInNewContext(fs.readFileSync(path.join(frontendRoot, 'platform/helpers.js'), 'utf8'), {window});
  assert.strictEqual(await api.refresh(), true);
  assert.strictEqual(publicRequests.length, 1);
  const publicCards = findElements(movieSection, el => Boolean(el.dataset && el.dataset.recordingId));
  assert.deepStrictEqual(publicCards.map(card => card.dataset.recordingId),
    publicRows.slice(0,12).map(row => row.recordingId));
  assert.strictEqual(publicCards[0].dataset.movieYear, String(currentYear));
  assert.strictEqual(findElement(publicCards[0], el => el.tagName === 'IMG').src,
    '/api/v1/recordings/' + publicRows[0].recordingId + '/artwork/' +
      'a'.repeat(32) + '?backendId=default&variant=home');
  publicCards[0].listeners.click[0]();
  await Promise.resolve();
  assert.strictEqual(opened.at(-1).recording.recordingId, publicRows[0].recordingId);
  assert.strictEqual(await api._test.loadMoreMovies(), true);
  assert.strictEqual(publicRequests.length, 2);
  assert.strictEqual(publicRequests[1].searchParams.get('offset'), '12');
  assert.deepStrictEqual(findElements(movieSection, el => Boolean(el.dataset && el.dataset.recordingId))
    .map(card => card.dataset.recordingId), publicRows.map(row => row.recordingId));
  console.log('phase66 recent movies rail contract ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
