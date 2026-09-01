'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8');
const bootstrap = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery-bootstrap.js'), 'utf8');

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
    this.type = '';
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }

  appendChild(child) {
    if (!child) return child;
    if (child.parentNode) child.remove();
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

  insertBefore(child, reference) {
    if (!child) return child;
    if (child.parentNode) child.remove();
    child.parentNode = this;
    if (!reference) {
      this.children.push(child);
      return child;
    }
    const index = this.children.indexOf(reference);
    if (index < 0) this.children.push(child);
    else this.children.splice(index, 0, child);
    return child;
  }

  get nextElementSibling() {
    if (!this.parentNode) return null;
    const index = this.parentNode.children.indexOf(this);
    return index >= 0 ? this.parentNode.children[index + 1] || null : null;
  }

  querySelector(selector) {
    const rail = String(selector || '').match(/^\[data-home-discovery-rail="([^"]+)"\]$/);
    if (rail) {
      return findElement(this, (element) =>
        element.attributes['data-home-discovery-rail'] === rail[1]);
    }
    return null;
  }

  closest() { return null; }
}

function findElement(root, predicate) {
  for (const child of root.children || []) {
    if (predicate(child)) return child;
    const nested = findElement(child, predicate);
    if (nested) return nested;
  }
  return null;
}

function findElements(root, predicate, result) {
  const found = result || [];
  for (const child of root.children || []) {
    if (predicate(child)) found.push(child);
    findElements(child, predicate, found);
  }
  return found;
}

function textNodes(root, value) {
  return findElements(root, (element) => element.textContent === value);
}

function recordingIds(root) {
  return findElements(root, (element) =>
    element.dataset && element.dataset.recordingId).map((element) => element.dataset.recordingId);
}

function createHarness() {
  const host = new FakeElement('div');
  const scheduled = [];
  const genreCalls = [];
  const folderCalls = [];
  const metadataCalls = [];
  let selectedModule = 'overview';

  const document = {
    readyState: 'loading',
    head: null,
    querySelector(selector) {
      return selector === '[data-home-zone="additional-sections"]' ? host : null;
    },
    createElement(tagName) { return new FakeElement(tagName); },
    addEventListener() {},
    getElementById() { return null; }
  };

  const context = {window: {}, console};
  context.window.window = context.window;
  context.window.document = document;
  context.window.setTimeout = function (callback) {
    scheduled.push(callback);
    return scheduled.length;
  };
  context.window.selectModule = function (moduleName) {
    selectedModule = moduleName;
    return true;
  };
  context.window.VdrSuiteHomeLivePreview = {cancel() {}};
  context.window.VdrSuiteRecordings2 = {openRecording() {}, openFolder() {}};
  context.window.VdrSuiteGenres = {openRecordingGenre() { return true; }};
  context.window.VdrSuitePlatform = {
    getSelectedBackendId() { return 'default'; },
    getSelectedModule() { return selectedModule; },
    getClientApi() { return null; }
  };

  vm.createContext(context);
  vm.runInContext(source, context, {filename: 'home-recording-discovery.js'});

  return {
    api: context.window.VdrSuiteHomeRecordingDiscovery,
    host,
    scheduled,
    genreCalls,
    folderCalls,
    metadataCalls,
    folderClient(pages) {
      return {
        fetchClientRecordingFolder(request) {
          folderCalls.push(request);
          const offset = Number(request.query.offset || 0);
          return Promise.resolve(pages[offset]);
        }
      };
    },
    genreClient(recordings) {
      return {
        fetchClientGenreRecordings(request) {
          genreCalls.push(request);
          const offset = Number(request.offset || 0);
          const limit = Number(request.limit || 100);
          const items = recordings.slice(offset, offset + limit);
          return Promise.resolve({
            recordings: items,
            total: recordings.length,
            hasMore: offset + items.length < recordings.length
          });
        },
        requestJson() {
          metadataCalls.push(Array.from(arguments));
          return Promise.resolve({available: false});
        }
      };
    }
  };
}

(async function () {
  const harness = createHarness();
  const api = harness.api._test;
  assert(api);

  const directA = {recordingId: 'root-a', backendId: 'default', title: 'Root A'};
  const directB = {recordingId: 'root-b', backendId: 'default', title: 'Root B'};
  const directC = {recordingId: 'root-c', backendId: 'default', title: 'Root C'};
  const leaf = {recordingId: 'root-leaf', backendId: 'default', title: 'Root Leaf'};
  const realFilmFolder = {name: 'Filme', path: 'Filme', recordingCount: 7, singleRecordingLeaf: false};
  const realSeriesFolder = {name: 'Serien', path: 'Serien', recordingCount: 5, singleRecordingLeaf: false};
  const leafFolder = {
    name: 'Root Leaf',
    path: 'Root Leaf',
    recordingCount: 1,
    singleRecordingLeaf: true,
    singleRecording: leaf
  };

  const firstProjection = api.projectRootFolderPayload({
    folders: [realFilmFolder, leafFolder, realSeriesFolder],
    recordings: [directA, directB]
  }, 'default');
  assert.deepStrictEqual(Array.from(firstProjection.folders, (entry) => entry.path), ['Filme', 'Serien']);
  assert.deepStrictEqual(Array.from(firstProjection.rootRecordings, (recording) => recording.recordingId), [
    'root-a', 'root-b', 'root-leaf'
  ]);

  const folderPages = {
    0: {
      folders: [realFilmFolder, leafFolder, realSeriesFolder],
      recordings: [directA, directB],
      recordingCount: 3
    },
    2: {
      folders: [realFilmFolder, leafFolder, realSeriesFolder],
      recordings: [directC],
      recordingCount: 3
    }
  };
  const folderClient = harness.folderClient(folderPages);
  assert.strictEqual(await api.loadFolders(folderClient, 'default', 0), true);
  assert.deepStrictEqual(harness.folderCalls.map((call) => call.query.offset), [0, 2]);
  assert(harness.folderCalls.every((call) => call.query.path === ''));
  assert(harness.folderCalls.every((call) => call.query.limit === 100));

  let folderSection = harness.host.querySelector('[data-home-discovery-rail="folders"]');
  assert(folderSection);
  const rootTiles = findElements(folderSection, (element) =>
    element.dataset && element.dataset.rootRecordingGroup === 'true');
  assert.strictEqual(rootTiles.length, 1);
  assert.strictEqual(textNodes(rootTiles[0], 'Hauptverzeichnis').length, 1);
  assert.strictEqual(textNodes(rootTiles[0], '4 Aufnahmen').length, 1);
  assert.strictEqual(findElements(folderSection, (element) =>
    element.dataset && element.dataset.folderPath).length, 2);
  assert.strictEqual(findElement(folderSection, (element) =>
    element.dataset && element.dataset.folderPath === 'Root Leaf'), null);

  rootTiles[0].listeners.click[0]();
  folderSection = harness.host.querySelector('[data-home-discovery-rail="folders"]');
  assert.strictEqual(textNodes(folderSection, 'Hauptverzeichnis').length, 1);
  assert.deepStrictEqual(recordingIds(folderSection), ['root-a', 'root-b', 'root-leaf', 'root-c']);
  assert.strictEqual(findElements(folderSection, (element) =>
    element.dataset && element.dataset.folderPath).length, 0);
  const back = textNodes(folderSection, '← Aufnahmeordner')[0];
  assert(back);
  back.listeners.click[0]();
  folderSection = harness.host.querySelector('[data-home-discovery-rail="folders"]');
  assert.strictEqual(findElements(folderSection, (element) =>
    element.dataset && element.dataset.rootRecordingGroup === 'true').length, 1);
  assert.strictEqual(findElements(folderSection, (element) =>
    element.dataset && element.dataset.folderPath).length, 2);

  api.renderFolderRail([realFilmFolder, realSeriesFolder], [], 'default');
  folderSection = harness.host.querySelector('[data-home-discovery-rail="folders"]');
  assert.strictEqual(findElements(folderSection, (element) =>
    element.dataset && element.dataset.rootRecordingGroup === 'true').length, 0);
  assert.strictEqual(findElements(folderSection, (element) =>
    element.dataset && element.dataset.folderPath).length, 2);

  const genres = [
    {id: 'empty', label: 'Leer', count: 0},
    {id: 'drama', label: 'Drama', count: 2},
    {id: 'action', label: 'Action', count: 4},
    {id: 'fantasy', label: 'Fantasy', count: 1}
  ];
  const firstRandom = api.selectRandomGenre(genres, 10, 0.95);
  assert.strictEqual(firstRandom.id, 'fantasy');
  assert.strictEqual(api.selectRandomGenre(genres, 10, 0).id, 'fantasy');
  assert.strictEqual(api.selectRandomGenre(genres, 11, 0).id, 'drama');
  assert.strictEqual(api.selectRandomGenre([{id: 'empty', count: 0}], 12, 0.5), null);

  const dramaRecordings = [
    {recordingId: 'drama-1', backendId: 'default', title: 'Drama Eins'},
    {recordingId: 'drama-2', backendId: 'default', title: 'Drama Zwei'}
  ];
  const genreClient = harness.genreClient(dramaRecordings);
  assert.strictEqual(await api.loadRandomGenre(
    genreClient,
    'default',
    0,
    {id: 'drama', label: 'Drama', count: 2}
  ), true);
  assert.strictEqual(harness.genreCalls.length, 0);
  const scheduled = harness.scheduled.pop();
  assert.strictEqual(typeof scheduled, 'function');
  scheduled();
  await new Promise((resolve) => setImmediate(resolve));
  assert.strictEqual(harness.genreCalls.length, 1);
  assert.strictEqual(harness.genreCalls[0].genreId, 'drama');
  assert.strictEqual(harness.metadataCalls.length, 0);
  const randomSection = harness.host.querySelector('[data-home-discovery-rail="random-genre"]');
  assert(randomSection);
  assert.strictEqual(textNodes(randomSection, 'Drama').length, 1);
  assert.deepStrictEqual(recordingIds(randomSection), ['drama-1', 'drama-2']);
  const randomRail = findElement(randomSection, (element) =>
    String(element.className).split(/\s+/).includes('media-home-discovery-rail'));
  assert(randomRail);

  assert(!source.includes('MutationObserver'));
  assert(source.includes("global.VdrSuiteRecordings2.openRecording(recording"));
  assert(source.includes("image.loading = 'lazy'"));
  assert(bootstrap.includes("const RAIL_SELECTOR = '.media-home-discovery-rail, .media-home-series-season-rail, .media-home-live-guide-rail'"));
  assert(bootstrap.includes('scrollbar-width:none'));
  assert(bootstrap.includes('::-webkit-scrollbar'));

  console.log('phase66 Home root directory and random genre regression ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
