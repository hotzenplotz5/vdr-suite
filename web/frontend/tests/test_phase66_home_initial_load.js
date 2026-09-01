'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');
const appSource = fs.readFileSync(path.join(frontendRoot, 'app.js'), 'utf8');
const discoverySource = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8');

const deferredIndex = indexSource.indexOf('../frontend/platform/deferred-runtime-loader.js');
const clientIndex = indexSource.indexOf('../frontend/api/client-api.js');
const appIndex = indexSource.indexOf('../frontend/app.js');
assert(deferredIndex >= 0 && clientIndex > deferredIndex && appIndex > clientIndex);

const markStart = appSource.indexOf('function markSelected(backendId)');
const markEnd = appSource.indexOf('\nfunction loadBackendDetails(', markStart);
const loadStart = markEnd + 1;
const loadEnd = appSource.indexOf('\nfunction renderBackend(', loadStart);
assert(markStart >= 0 && markEnd > markStart);
assert(loadStart > markEnd && loadEnd > loadStart);
const markSelectedSource = appSource.slice(markStart, markEnd);
const loadBackendDetailsSource = appSource.slice(loadStart, loadEnd);
assert(loadBackendDetailsSource.includes('markSelected(backendId);'));
assert(loadBackendDetailsSource.indexOf("selectModule('overview');") < loadBackendDetailsSource.indexOf('markSelected(backendId);'));

function classList(initial) {
  const values = new Set(String(initial || '').split(/\s+/).filter(Boolean));
  return {
    contains(name) { return values.has(name); },
    add(name) { values.add(name); },
    remove(name) { values.delete(name); },
    toggle(name, enabled) {
      if (enabled === undefined) enabled = !values.has(name);
      if (enabled) values.add(name); else values.delete(name);
      return enabled;
    }
  };
}

class FakeElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.attributes = Object.create(null);
    this.dataset = Object.create(null);
    this.className = '';
    this.classList = classList();
    this.parentNode = null;
    this.textContent = '';
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
    Array.from(arguments).forEach(child => this.appendChild(child));
  }

  replaceChildren() {
    this.children.forEach(child => { child.parentNode = null; });
    this.children = [];
    Array.from(arguments).forEach(child => this.appendChild(child));
  }

  addEventListener() {}

  remove() {
    if (!this.parentNode) return;
    const index = this.parentNode.children.indexOf(this);
    if (index >= 0) this.parentNode.children.splice(index, 1);
    this.parentNode = null;
  }

  querySelector(selector) {
    const rail = String(selector || '').match(/^\[data-home-discovery-rail="([^"]+)"\]$/);
    if (rail) {
      return findElement(this, element => element.attributes['data-home-discovery-rail'] === rail[1]);
    }
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

function flush() {
  return new Promise(resolve => setImmediate(resolve));
}

async function main() {
  const host = new FakeElement('section');
  const backendHost = new FakeElement('section');
  const backendCard = new FakeElement('article');
  backendCard.dataset.backendId = 'backend-a';
  backendCard.classList = classList('backend-card');
  backendHost.appendChild(backendCard);

  const intersectionObservers = [];
  const backendObservers = [];
  const calls = {
    recordings: [],
    genres: [],
    genreRecordings: [],
    folders: []
  };

  const client = {
    fetchClientRecordings(request) {
      calls.recordings.push(request);
      return Promise.resolve({recordings: []});
    },
    fetchClientGenres(request) {
      calls.genres.push(request);
      return Promise.resolve({genres: []});
    },
    fetchClientGenreRecordings(request) {
      calls.genreRecordings.push(request);
      return Promise.resolve({items: [], total: 0, hasMore: false});
    },
    fetchClientRecordingFolder(request) {
      calls.folders.push(request);
      return Promise.resolve({folders: []});
    }
  };

  const documentListeners = Object.create(null);
  const document = {
    readyState: 'complete',
    head: {appendChild() {}},
    createElement(tagName) { return new FakeElement(tagName); },
    getElementById(id) {
      if (id === 'backends') return backendHost;
      return null;
    },
    querySelector(selector) {
      if (selector === '[data-home-zone="additional-sections"]') return host;
      if (selector === '#backends .backend-card.selected, #backends [aria-selected="true"]') {
        return backendCard.classList.contains('selected') ? backendCard : null;
      }
      return null;
    },
    querySelectorAll(selector) {
      if (selector === '.backend-card') return [backendCard];
      return [];
    },
    addEventListener(type, listener) {
      (documentListeners[type] ||= []).push(listener);
    }
  };

  const context = {
    window: {},
    console,
    setImmediate
  };
  context.window.window = context.window;
  context.window.document = document;
  context.window.console = console;
  context.window.setTimeout = function(callback) {
    callback();
    return 1;
  };
  context.window.VdrSuiteClientApi = client;
  context.window.IntersectionObserver = function(callback, options) {
    this.callback = callback;
    this.options = options;
    this.observe = target => { this.target = target; };
    this.disconnect = () => { this.disconnected = true; };
    intersectionObservers.push(this);
  };
  context.window.MutationObserver = function(callback) {
    this.callback = callback;
    this.observe = (target, options) => {
      this.target = target;
      this.options = options;
    };
    this.disconnect = () => { this.disconnected = true; };
    backendObservers.push(this);
  };

  vm.createContext(context);
  vm.runInContext(
    `var selectedBackendId = '';\n` +
    `var selectedModule = 'overview';\n` +
    `window.VdrSuitePlatform = {\n` +
    `  getSelectedBackendId: function () { return selectedBackendId; },\n` +
    `  getSelectedModule: function () { return selectedModule; },\n` +
    `  getClientApi: function () { return window.VdrSuiteClientApi; }\n` +
    `};\n` +
    markSelectedSource,
    context
  );
  vm.runInContext(discoverySource, context);

  assert.strictEqual(intersectionObservers.length, 1);
  assert.strictEqual(backendObservers.length, 1);
  assert.strictEqual(backendObservers[0].target, backendHost);
  assert.strictEqual(backendObservers[0].options.childList, true);
  assert.strictEqual(backendObservers[0].options.subtree, true);
  assert.strictEqual(backendObservers[0].options.attributes, true);
  assert(backendObservers[0].options.attributeFilter.includes('class'));

  intersectionObservers[0].callback([{isIntersecting: true}]);
  await flush();
  await flush();

  assert.deepStrictEqual(calls.recordings, []);
  assert.deepStrictEqual(calls.genres, []);
  assert.deepStrictEqual(calls.folders, []);

  vm.runInContext("markSelected('backend-a');", context);
  assert(backendCard.classList.contains('selected'));
  backendObservers[0].callback([{type: 'attributes', target: backendCard}]);

  assert.strictEqual(intersectionObservers.length, 2);
  intersectionObservers[1].callback([{isIntersecting: true}]);
  await flush();
  await flush();

  assert.strictEqual(calls.recordings.length, 1);
  assert.strictEqual(calls.genres.length, 1);
  assert.strictEqual(calls.folders.length, 1);
  assert.strictEqual(calls.recordings[0].query.backend, 'backend-a');
  assert.strictEqual(calls.genres[0].backendId, 'backend-a');
  assert.strictEqual(calls.folders[0].backendId, 'backend-a');
  assert(!JSON.stringify(calls).includes('"default"'));

  console.log('phase66 home initial backend lifecycle ok');
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
