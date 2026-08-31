'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(
  path.join(frontendRoot, 'home-recording-discovery.js'),
  'utf8'
);

class FakeElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.attributes = {};
    this.dataset = {};
    this.listeners = {};
    this.parentNode = null;
    this.className = '';
    this.textContent = '';
    this.src = '';
    this.alt = '';
    this.loading = '';
    this.type = '';
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }

  appendChild(child) {
    if (!child) return child;
    child.parentNode = this;
    this.children.push(child);
    return child;
  }

  append() {
    Array.from(arguments).forEach((child) => this.appendChild(child));
  }

  replaceChildren() {
    this.children.forEach((child) => { child.parentNode = null; });
    this.children = [];
    Array.from(arguments).forEach((child) => this.appendChild(child));
  }

  addEventListener(type, handler) {
    if (!this.listeners[type]) this.listeners[type] = [];
    this.listeners[type].push(handler);
  }

  remove() {
    if (!this.parentNode) return;
    const index = this.parentNode.children.indexOf(this);
    if (index >= 0) this.parentNode.children.splice(index, 1);
    this.parentNode = null;
  }

  querySelector(selector) {
    const match = String(selector || '').match(
      /^\[data-home-discovery-rail="([^"]+)"\]$/
    );
    if (!match) return null;
    return findElement(this, (element) =>
      element.attributes['data-home-discovery-rail'] === match[1]);
  }

  closest() {
    return null;
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

function findRail(host, key) {
  return host.querySelector('[data-home-discovery-rail="' + key + '"]');
}

function findSeriesCard(root, seriesKey) {
  if (!root) return null;
  return findElement(root, (element) =>
    element.dataset && element.dataset.seriesKey === seriesKey);
}

function findSeriesCards(root) {
  if (!root) return [];
  return findElements(root, (element) =>
    element.dataset && Boolean(element.dataset.seriesKey));
}

function findSeasonButtons(root) {
  if (!root) return [];
  return findElements(root, (element) =>
    element.dataset && element.dataset.seasonNumber !== undefined);
}

function findBackButton(root) {
  if (!root) return null;
  return findElement(root, (element) =>
    String(element.className).includes('media-home-series-back'));
}

function makeEpisode(seriesTitle, seasonNumber, episodeNumber, idPrefix) {
  const season = String(seasonNumber).padStart(2, '0');
  const episode = String(episodeNumber).padStart(2, '0');
  const id = idPrefix + '-s' + season + 'e' + episode;
  return {
    recordingId: id,
    backendId: 'default',
    backendNativeId: 'native-' + id,
    path: 'Serien/' + seriesTitle + '/S' + season + 'E' + episode + ' Folge ' + episode,
    title: 'Serien/' + seriesTitle + '/S' + season + 'E' + episode + ' Folge ' + episode,
    metadata: {
      provider: {},
      presentation: {posterUrl: ''},
      artwork: {preferredUrl: ''}
    }
  };
}

function buildSeriesSet() {
  const items = [];
  for (let season = 1; season <= 10; season += 1) {
    for (let episode = 1; episode <= 15; episode += 1) {
      if (season === 9 && episode > 10) continue;
      items.push(makeEpisode('The Walking Dead', season, episode, 'twd'));
    }
  }
  assert.strictEqual(items.length, 145);
  for (let seriesIndex = 1; seriesIndex <= 13; seriesIndex += 1) {
    const title = 'Serie ' + String(seriesIndex).padStart(2, '0');
    for (let episode = 1; episode <= 9; episode += 1) {
      items.push(makeEpisode(title, 1, episode, 'series-' + seriesIndex));
    }
  }
  assert.strictEqual(items.length, 262);
  return items;
}

function createHarness(seriesItems) {
  const host = new FakeElement('div');
  const calls = [];
  const pendingPages = new Map();

  function pagePayload(request) {
    const serverLimit = Math.min(Number(request.limit || 0) || 48, 100);
    const offset = Number(request.offset || 0);
    const items = seriesItems.slice(offset, offset + serverLimit);
    return {
      backendId: 'default',
      genreId: 'series',
      total: seriesItems.length,
      limit: serverLimit,
      offset,
      hasMore: offset + items.length < seriesItems.length,
      items
    };
  }

  const client = {
    fetchClientRecordings() {
      return Promise.resolve({recordings: []});
    },
    fetchClientGenres() {
      return Promise.resolve({
        genres: [{id: 'series', label: 'Serien', count: seriesItems.length}]
      });
    },
    fetchClientGenreRecordings(request) {
      calls.push(request);
      if (Number(request.offset || 0) === 100) {
        return new Promise((resolve) => {
          pendingPages.set(100, () => resolve(pagePayload(request)));
        });
      }
      return Promise.resolve(pagePayload(request));
    },
    fetchClientRecordingFolder() {
      return Promise.resolve({folders: []});
    }
  };

  const document = {
    readyState: 'loading',
    head: null,
    querySelector(selector) {
      return selector === '[data-home-zone="additional-sections"]' ? host : null;
    },
    createElement(tagName) {
      return new FakeElement(tagName);
    },
    addEventListener() {},
    getElementById() { return null; }
  };

  const context = {window: {}, console};
  context.window.window = context.window;
  context.window.document = document;
  context.window.setTimeout = function () { return 1; };
  context.window.VdrSuiteHomeLivePreview = {cancel() {}};
  context.window.VdrSuitePlatform = {
    getSelectedBackendId() { return 'default'; },
    getSelectedModule() { return 'overview'; },
    getClientApi() { return client; }
  };

  vm.createContext(context);
  vm.runInContext(source, context);

  return {
    api: context.window.VdrSuiteHomeRecordingDiscovery,
    host,
    calls,
    releasePage(offset) {
      const release = pendingPages.get(offset);
      if (!release) return false;
      pendingPages.delete(offset);
      release();
      return true;
    }
  };
}

async function flushMicrotasks(turns) {
  for (let index = 0; index < (turns || 8); index += 1) {
    await Promise.resolve();
  }
}

(async function () {
  const harness = createHarness(buildSeriesSet());
  let settled = false;
  const refresh = harness.api.refresh().then((value) => {
    settled = true;
    return value;
  });

  await flushMicrotasks();

  assert.strictEqual(settled, false);
  assert.deepStrictEqual(
    harness.calls.map((call) => Number(call.offset || 0)),
    [0, 100]
  );

  const partialRail = findRail(harness.host, 'series');
  assert(partialRail);
  assert.strictEqual(findSeriesCards(partialRail).length, 1);
  const partialWalkingDead = findSeriesCard(
    partialRail,
    'folder:serien/the walking dead'
  );
  assert(partialWalkingDead);
  assert(findElement(partialWalkingDead, (element) =>
    String(element.textContent).includes('100 Folgen')));

  partialWalkingDead.listeners.click[0]();
  assert.strictEqual(findSeasonButtons(findRail(harness.host, 'series')).length, 7);

  assert.strictEqual(harness.releasePage(100), true);
  assert.strictEqual(await refresh, true);

  assert.deepStrictEqual(
    harness.calls.map((call) => Number(call.offset || 0)),
    [0, 100, 200]
  );
  assert.strictEqual(
    findSeasonButtons(findRail(harness.host, 'series')).length,
    10
  );
  assert.strictEqual(
    findSeriesCards(findRail(harness.host, 'series')).length,
    0
  );

  const back = findBackButton(findRail(harness.host, 'series'));
  assert(back);
  back.listeners.click[0]();
  assert.strictEqual(
    findSeriesCards(findRail(harness.host, 'series')).length,
    14
  );

  console.log('phase66 series projection renders progressively while pagination continues');
}()).catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
