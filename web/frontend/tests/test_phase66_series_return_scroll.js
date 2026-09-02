'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'home-recording-discovery.js'),
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
    this.type = '';
    this.scrollIntoViewCalls = [];
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
    const match = String(selector || '').match(/^\[data-home-discovery-rail="([^"]+)"\]$/);
    if (!match) return null;
    return findElement(this, (element) =>
      element.attributes['data-home-discovery-rail'] === match[1]);
  }

  scrollIntoView(options) {
    this.scrollIntoViewCalls.push(options || {});
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

const host = new FakeElement('div');
const openedRecordings = [];
const selectedModules = [];
let selectedModule = 'overview';

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
context.window.selectModule = function (moduleName) {
  selectedModule = moduleName;
  selectedModules.push(moduleName);
  return true;
};
context.window.VdrSuiteHomeLivePreview = {cancel() {}};
context.window.VdrSuiteRecordings2 = {
  openRecording(recording, options) {
    openedRecordings.push({recording, options});
  },
  openFolder() {}
};
context.window.VdrSuitePlatform = {
  getSelectedBackendId() { return 'default'; },
  getSelectedModule() { return selectedModule; },
  getClientApi() { return null; }
};

vm.createContext(context);
vm.runInContext(source, context);

const api = context.window.VdrSuiteHomeRecordingDiscovery;
assert(api && api._test);

const recording = {
  recordingId: 'test-series-s02e01',
  backendId: 'default',
  title: 'Serien/Testserie/S02E01 Pilot',
  metadata: {provider: {}, presentation: {}, artwork: {}}
};
const member = {
  recording,
  backendId: 'default',
  episodeNumber: 1,
  episodeTitle: 'Pilot',
  posterUrl: ''
};
const season = {
  number: 2,
  label: 'Staffel 2',
  episodes: [member]
};
const series = {
  key: 'folder:serien/testserie',
  title: 'Testserie',
  posterUrl: '',
  episodes: [member],
  seasons: [season]
};

assert.strictEqual(api._test.renderSeriesDetail(series, season, 'default'), true);
const seriesSection = host.querySelector('[data-home-discovery-rail="series"]');
assert(seriesSection);
assert.strictEqual(seriesSection.scrollIntoViewCalls.length, 0);

const episodeCard = findElement(seriesSection, (element) =>
  element.dataset && element.dataset.recordingId === recording.recordingId);
assert(episodeCard);
assert(episodeCard.listeners.click && episodeCard.listeners.click.length === 1);
episodeCard.listeners.click[0]();

Promise.resolve().then(function () {
  return Promise.resolve();
}).then(function () {
  assert.strictEqual(openedRecordings.length, 1);
  assert.strictEqual(openedRecordings[0].recording, recording);
  assert.strictEqual(openedRecordings[0].options.backLabel, '← Zurück zur Staffel');
  assert.strictEqual(typeof openedRecordings[0].options.onClose, 'function');
  assert.strictEqual(selectedModules[0], 'recordings2');

  openedRecordings[0].options.onClose();

  assert.strictEqual(selectedModule, 'overview');
  assert.strictEqual(selectedModules[selectedModules.length - 1], 'overview');
  const returnedSection = host.querySelector('[data-home-discovery-rail="series"]');
  assert.strictEqual(returnedSection, seriesSection);
  assert(findElement(returnedSection, (element) =>
    element.dataset && element.dataset.recordingId === recording.recordingId));
  assert.strictEqual(returnedSection.scrollIntoViewCalls.length, 1);
  assert.strictEqual(returnedSection.scrollIntoViewCalls[0].block, 'start');
  assert.strictEqual(returnedSection.scrollIntoViewCalls[0].behavior, 'auto');

  console.log('phase66 series season return reveals canonical Home series section');
}).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
