'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8');
const bootstrapSource = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery-bootstrap.js'), 'utf8');

assert(source.includes('function selectRandomFolder(entries, generation, randomValue)'));
assert(source.includes('folderEntryCount(entry) > 0'));
assert(source.includes('bootstrap.installMouseDrag();'));
assert(source.includes('selectedCard.click();'));
assert(source.includes('scheduleRandomFolderInline(visibleFolders, backendId, generation, Math.random())'));
assert(bootstrapSource.includes("doc.addEventListener('click', handleClickCapture, true)"));
assert(bootstrapSource.includes('handleInlineCategoryClick(event)'));
assert(bootstrapSource.includes('openFolderInline(card)'));

class FakeElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.attributes = {};
    this.dataset = {};
    this.listeners = Object.create(null);
    this.parentNode = null;
    this.className = '';
    this.textContent = '';
    this.type = '';
    this.clickCount = 0;
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
    (this.listeners[type] ||= []).push(handler);
  }

  remove() {
    if (!this.parentNode) return;
    const index = this.parentNode.children.indexOf(this);
    if (index >= 0) this.parentNode.children.splice(index, 1);
    this.parentNode = null;
  }

  click() {
    this.clickCount += 1;
  }

  querySelector(selector) {
    const rail = String(selector || '').match(/^\[data-home-discovery-rail="([^"]+)"\]$/);
    if (!rail) return null;
    return findElement(this, (element) => element.attributes['data-home-discovery-rail'] === rail[1]);
  }

  querySelectorAll(selector) {
    if (selector !== '.media-home-discovery-card.folder') return [];
    return findElements(this, (element) => {
      const classes = String(element.className || '').split(/\s+/);
      return classes.includes('media-home-discovery-card') && classes.includes('folder');
    });
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

const host = new FakeElement('div');
const scheduled = [];
let bootstrapInstalls = 0;

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

const window = {
  document,
  setTimeout(callback) {
    scheduled.push(callback);
    return scheduled.length;
  },
  VdrSuitePlatform: {
    getSelectedBackendId() { return 'default'; },
    getSelectedModule() { return 'overview'; },
    getClientApi() { return null; }
  },
  VdrSuiteHomeRecordingDiscoveryBootstrap: {
    installMouseDrag() {
      bootstrapInstalls += 1;
      return true;
    }
  }
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
  Set,
  Map
});
vm.runInContext(source, context, {filename: 'web/frontend/home-recording-discovery.js'});

const api = window.VdrSuiteHomeRecordingDiscovery._test;
assert(api);

const empty = {name: 'Leer', path: 'Leer', recordingCount: 0};
const action = {name: 'Action', path: 'Action', recordingCount: 48};
const fantasy = {name: 'Fantasy', path: 'Fantasy', totalCount: 47};
const folders = [empty, action, fantasy];

assert.strictEqual(api.folderEntryCount(empty), 0);
assert.strictEqual(api.folderEntryCount(action), 48);
assert.strictEqual(api.folderEntryCount(fantasy), 47);
assert.strictEqual(api.selectRandomFolder(folders, 10, 0.99).path, 'Fantasy');
assert.strictEqual(api.selectRandomFolder(folders, 10, 0).path, 'Fantasy', 'selection must stay stable within one generation');
assert.strictEqual(api.selectRandomFolder(folders, 11, 0).path, 'Action', 'a later generation may choose another folder');
assert.strictEqual(api.selectRandomFolder([empty], 12, 0.5), null, 'empty folders must not be auto-open candidates');

const rootRecording = {recordingId: 'root-1', backendId: 'default', title: 'Root'};
assert.strictEqual(api.renderFolderRail(folders, [rootRecording], 'default'), true);
const folderSection = host.querySelector('[data-home-discovery-rail="folders"]');
assert(folderSection);

const rootTile = findElement(folderSection, (element) => element.dataset.rootRecordingGroup === 'true');
assert(rootTile, 'Hauptverzeichnis tile must remain present');
const cards = folderSection.querySelectorAll('.media-home-discovery-card.folder');
assert.strictEqual(cards.length, 3);

assert.strictEqual(api.scheduleRandomFolderInline(folders, 'default', 0, 0.99), true);
assert.strictEqual(scheduled.length, 1, 'auto-open must be deferred until the folder rail is fully composed');
scheduled.shift()();

const actionCard = cards.find((card) => card.dataset.folderPath === 'Action');
const fantasyCard = cards.find((card) => card.dataset.folderPath === 'Fantasy');
const emptyCard = cards.find((card) => card.dataset.folderPath === 'Leer');
assert(actionCard && fantasyCard && emptyCard);
assert.strictEqual(bootstrapInstalls, 1, 'canonical bootstrap capture owner must be ensured before the synthetic click');
assert.strictEqual(fantasyCard.clickCount, 1, 'selected non-empty folder must receive the canonical click');
assert.strictEqual(actionCard.clickCount, 0);
assert.strictEqual(emptyCard.clickCount, 0);
assert.strictEqual(rootTile.clickCount, 0, 'Hauptverzeichnis must never be selected as the random folder expansion');

console.log('phase66 Home random recording-folder auto-open contract ok');
