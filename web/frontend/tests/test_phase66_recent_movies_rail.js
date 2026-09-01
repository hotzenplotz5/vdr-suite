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
assert(source.includes("text(provider(recording).contentKind) !== 'movie'"));
assert(source.includes('provider(recording).releaseDate'));
assert(source.includes('year >= nowYear - 4'));
assert(source.includes('year <= nowYear'));
assert(source.includes('new Date().getFullYear()'));
assert(source.includes('fetchClientRecordings'));
assert(source.includes('owner.canonicalRecordings'));
assert(source.includes('owner.recordingPosterUrl'));
assert(source.includes('owner.openRecording'));
assert(source.includes("target.querySelector('[data-home-discovery-rail=\"series\"]')"));
assert(source.includes("rail.className = 'media-home-discovery-rail recent-movies'"));
assert(!source.includes('/api/vdr/recordings/metadata'));
assert(!source.includes('requestJson('));
assert(!source.includes('MediaSession'));
assert(!source.toLowerCase().includes('recommend'));
assert(discoverySource.includes('VdrSuiteRecordings2.openRecording'));
assert(bootstrapSource.includes("const RAIL_SELECTOR = '.media-home-discovery-rail, .media-home-series-season-rail, .media-home-live-guide-rail';"));
assert(bootstrapSource.includes('scrollbar-width:none'));

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
const pageOne = [
  movie('outside', String(currentYear - 5) + '-12-31', '2026-08-01T20:00:00', 'Zu alt'),
  nonMovie('series', 'series-episode', String(currentYear) + '-02-02'),
  movie('newest', String(currentYear) + '-06-01', '2026-07-01T20:00:00', 'Neu'),
  nonMovie('unknown', 'unknown', String(currentYear) + '-01-01'),
  movie('invalid-date', String(currentYear) + '-02-30', '2026-06-01T20:00:00', 'Ungültig')
];
const pageTwo = [
  movie('boundary', String(currentYear - 4), '2025-04-01T20:00:00', 'Grenze'),
  movie('future', String(currentYear + 1) + '-01-01', '2026-05-01T20:00:00', 'Zukunft'),
  movie('no-year', '', '2026-04-01T20:00:00', 'Ohne Jahr')
];
const allRecordings = pageOne.concat(pageTwo);

const client = {
  fetchClientRecordings(request) {
    calls.push(request);
    const offset = Number(request.query.offset || 0);
    const recordings = offset === 0 ? pageOne : (offset === pageOne.length ? pageTwo : []);
    return Promise.resolve({
      recordings,
      totalCount: allRecordings.length,
      limit: recordings.length,
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
  VdrSuitePublicUrl: {resolvePath(value) { return value; }}
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
  assert.strictEqual(await api.refresh(), true);
  assert.deepStrictEqual(calls.map((call) => call.query.offset), [0, pageOne.length]);
  assert(calls.every((call) => call.query.limit === 100));
  assert(calls.every((call) => call.query.backend === 'default'));
  assert(calls.every((call) => call.cache === 'no-store'));
  assert(calls.every((call) => call.credentials === 'same-origin'));

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

  const cards = findElements(movieSection, (element) =>
    element.dataset && Boolean(element.dataset.recordingId));
  assert.deepStrictEqual(cards.map((card) => card.dataset.recordingId), ['newest', 'boundary']);
  assert.deepStrictEqual(cards.map((card) => Number(card.dataset.movieYear)), [currentYear, currentYear - 4]);
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

  console.log('phase66 recent movies rail contract ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
