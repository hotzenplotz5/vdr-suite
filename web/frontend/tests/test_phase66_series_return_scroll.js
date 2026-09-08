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
    this.scrollLeft = 0;
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

  insertBefore(child, before) {
    if (child.parentNode) child.remove();
    child.parentNode = this;
    const index = before ? this.children.indexOf(before) : -1;
    if (index < 0) this.children.push(child);
    else this.children.splice(index, 0, child);
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

const otherSeries = Object.assign({}, series, {key: 'other', title: 'Other'});
api._test.renderSeriesRail([series, otherSeries], 'default');
const initialSection = host.querySelector('[data-home-discovery-rail="series"]');
const initialRail = findElement(initialSection, element => element.className === 'media-home-discovery-rail series');
const initialCard = initialRail.children[0];
initialRail.scrollLeft = 280;
api._test.renderSeriesRail([Object.assign({}, series), otherSeries], 'default');
assert.strictEqual(initialRail.parentNode, initialSection, 'metadata retry retains the actual scrolling element');
assert.strictEqual(initialRail.children[0], initialCard, 'unchanged cards and poster nodes survive retries');
assert.strictEqual(initialRail.scrollLeft, 280);
const updatedOther = Object.assign({}, otherSeries, {posterUrl: '/new-poster.jpg'});
api._test.renderSeriesRail([series, updatedOther], 'default');
assert.strictEqual(initialRail.children[0], initialCard, 'one enriched poster does not recreate other cards');
assert.strictEqual(initialRail.children.length, 2);
assert.strictEqual(initialRail.scrollLeft, 280, 'metadata enrichment retains horizontal position');
api._test.renderSeriesRail([series], 'default');
assert.strictEqual(initialRail.children.length, 1, 'withdrawn series disappears');
assert.strictEqual(initialRail.children[0], initialCard);
assert.strictEqual(api._test.renderSeriesDetail(series, season, 'default'), true);
api._test.renderSeriesRail([series], 'default');
assert.strictEqual(initialRail.parentNode, initialSection, 'return from details remounts the original Series list');
assert.strictEqual(initialRail.scrollLeft, 280, 'return from details restores the list position after mounting');
assert.strictEqual(initialRail.children[0], initialCard);
api._test.renderSeriesDetail(series, season, 'default');
const seriesSection = host.querySelector('[data-home-discovery-rail="series"]');
assert(seriesSection);
assert.strictEqual(seriesSection.scrollIntoViewCalls.length, 0);

const episodeCard = findElement(seriesSection, (element) =>
  element.dataset && element.dataset.recordingId === recording.recordingId);
assert(episodeCard);
const seasonRail = findElement(seriesSection, element => element.className === 'media-home-series-season-rail');
const episodeRail = episodeCard.parentNode;
seasonRail.scrollLeft = 140;
episodeRail.scrollLeft = 280;
api._test.renderSeriesDetail(series, season, 'default');
assert.strictEqual(episodeCard.parentNode, episodeRail, 'unchanged detail keeps episode card mounted');
assert.strictEqual(episodeRail.parentNode, seriesSection, 'unchanged detail keeps scrolling rail mounted');
const enrichedSeries = Object.assign({}, series, {posterUrl: '/series-cover.jpg'});
api._test.renderSeriesDetail(enrichedSeries, season, 'default');
assert.strictEqual(episodeRail.parentNode, seriesSection, 'summary enrichment must not rebuild episode rail');
assert.strictEqual(seasonRail.parentNode, seriesSection, 'summary enrichment must not rebuild season rail');
assert.strictEqual(episodeRail.scrollLeft, 280);
assert.strictEqual(seasonRail.scrollLeft, 140);
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
