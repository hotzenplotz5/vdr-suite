'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery-bootstrap.js'), 'utf8');

assert(source.includes("const CATEGORY_SELECTOR = '.media-home-discovery-card.genre, .media-home-discovery-card.folder'"));
assert(source.includes('fetchClientGenreRecordings({'));
assert(source.includes('fetchClientRecordingFolder({'));
assert(source.includes("'media-home-discovery-rail media-home-inline-rail'"));
assert(source.includes("card.setAttribute('aria-expanded'"));
assert(source.includes("global.VdrSuiteRecordings2.openRecording(recording"));
assert(source.includes('entry.singleRecordingLeaf !== true'));
assert(source.includes('folderHasMoreRecordings(payload, nextOffset, raw.length)'));
assert(source.includes('const recordingCount = Number(payload && payload.recordingCount)'));
assert(!source.includes("selectModule('genres')"));
assert(!source.includes("selectModule('recordings2');\n      global.VdrSuiteRecordings2.openFolder"));
assert(!source.includes('/api/media/sessions'));
assert(!source.includes('MediaSession'));
assert(!source.includes('navigator.mediaSession'));

class FakeElement {
  constructor(tagName, classes) {
    this.tagName = String(tagName || '').toUpperCase();
    this._classes = new Set();
    this.className = classes || '';
    this.dataset = {};
    this.attributes = {};
    this.children = [];
    this.parentNode = null;
    this.textContent = '';
    this.id = '';
    this.scrollLeft = 0;
    this.listeners = Object.create(null);
    this.classList = {
      contains: name => this._classes.has(name),
      add: (...names) => names.forEach(name => this._classes.add(name)),
      remove: (...names) => names.forEach(name => this._classes.delete(name)),
      toggle: (name, force) => {
        const next = force === undefined ? !this._classes.has(name) : Boolean(force);
        if (next) this._classes.add(name); else this._classes.delete(name);
        return next;
      }
    };
  }

  get className() { return Array.from(this._classes).join(' '); }
  set className(value) {
    this._classes = new Set(String(value || '').split(/\s+/).filter(Boolean));
  }

  appendChild(child) {
    child.parentNode = this;
    this.children.push(child);
    return child;
  }

  append(...children) {
    children.forEach(child => this.appendChild(child));
  }

  remove() {
    if (!this.parentNode) return;
    const index = this.parentNode.children.indexOf(this);
    if (index >= 0) this.parentNode.children.splice(index, 1);
    this.parentNode = null;
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
    if (name === 'id') this.id = String(value);
  }

  removeAttribute(name) {
    delete this.attributes[name];
  }

  addEventListener(type, listener) {
    (this.listeners[type] ||= []).push(listener);
  }

  emit(type, event) {
    (this.listeners[type] || []).forEach(listener => listener(event || {target: this}));
  }

  setPointerCapture() {}
  releasePointerCapture() {}

  matches(selector) {
    const value = String(selector || '').trim();
    if (!value) return false;
    if (value === 'strong') return this.tagName === 'STRONG';
    if (value === '.media-home-discovery-rail') return this.classList.contains('media-home-discovery-rail');
    if (value === '.media-home-series-season-rail') return this.classList.contains('media-home-series-season-rail');
    if (value === '.media-home-inline-expansion') return this.classList.contains('media-home-inline-expansion');
    if (value === '.media-home-inline-back') return this.classList.contains('media-home-inline-back');
    if (value === '.media-home-discovery-card.recording') {
      return this.classList.contains('media-home-discovery-card') && this.classList.contains('recording');
    }
    if (value === '.media-home-discovery-card.genre') {
      return this.classList.contains('media-home-discovery-card') && this.classList.contains('genre');
    }
    if (value === '.media-home-discovery-card.folder') {
      return this.classList.contains('media-home-discovery-card') && this.classList.contains('folder');
    }
    if (value === '.media-home-inline-folder') return this.classList.contains('media-home-inline-folder');
    if (value === '.media-home-hero.media-home-live-hero-active[data-home-zone="hero"]') {
      return this.classList.contains('media-home-hero') &&
        this.classList.contains('media-home-live-hero-active') &&
        this.attributes['data-home-zone'] === 'hero';
    }
    const railMatch = value.match(/^\[data-home-discovery-rail="([^"]+)"\]$/);
    if (railMatch) return this.attributes['data-home-discovery-rail'] === railMatch[1];
    return false;
  }

  closest(selector) {
    const selectors = String(selector || '').split(',').map(item => item.trim());
    let current = this;
    while (current) {
      if (selectors.some(item => current.matches(item))) return current;
      current = current.parentNode;
    }
    return null;
  }

  querySelectorAll(selector) {
    const selectors = String(selector || '').split(',').map(item => item.trim());
    const result = [];
    const visit = node => {
      (node.children || []).forEach(child => {
        if (selectors.some(item => child.matches(item))) result.push(child);
        visit(child);
      });
    };
    visit(this);
    return result;
  }

  querySelector(selector) {
    return this.querySelectorAll(selector)[0] || null;
  }
}

function makeCard(kind, key, label) {
  const card = new FakeElement('button', 'media-home-discovery-card ' + kind);
  if (kind === 'genre') card.dataset.genreId = key;
  else card.dataset.folderPath = key;
  const copy = card.appendChild(new FakeElement('span', 'media-home-discovery-copy'));
  const strong = copy.appendChild(new FakeElement('strong'));
  strong.textContent = label;
  return card;
}

function makeSection(kind, card) {
  const section = new FakeElement('section', 'media-home-discovery');
  section.setAttribute('data-home-discovery-rail', kind);
  const rail = section.appendChild(new FakeElement('div', 'media-home-discovery-rail ' + kind));
  rail.appendChild(card);
  return section;
}

const head = new FakeElement('head');
const documentListeners = Object.create(null);
const document = {
  head,
  createElement(tagName) { return new FakeElement(tagName); },
  getElementById(id) {
    return head.children.find(child => child.id === id) || null;
  },
  addEventListener(type, listener, options) {
    (documentListeners[type] ||= []).push({listener, options});
  }
};

function browserEvent(target, values) {
  return Object.assign({
    target,
    pointerType: 'mouse',
    button: 0,
    pointerId: 1,
    clientX: 0,
    clientY: 0,
    defaultPrevented: false,
    propagationStopped: false,
    immediatePropagationStopped: false,
    preventDefault() { this.defaultPrevented = true; },
    stopPropagation() { this.propagationStopped = true; },
    stopImmediatePropagation() {
      this.immediatePropagationStopped = true;
      this.propagationStopped = true;
    }
  }, values || {});
}

function dispatchDocument(type, target, values) {
  const current = browserEvent(target, values);
  (documentListeners[type] || []).forEach(entry => entry.listener(current));
  return current;
}

function clickElement(target) {
  const event = dispatchDocument('click', target);
  if (!event.immediatePropagationStopped) target.emit('click', event);
  return event;
}

const genreCalls = [];
const folderCalls = [];
const openRecordingCalls = [];
const moduleSelections = [];

function recording(id, title) {
  return {
    recordingId: id,
    backendId: 'backend-a',
    title,
    metadata: {presentation: {title}}
  };
}

const client = {
  fetchClientGenreRecordings(options) {
    genreCalls.push(options);
    if (options.offset === 0) {
      return Promise.resolve({
        recordings: [recording('r1', 'Action Eins')],
        total: 2,
        hasMore: true
      });
    }
    return Promise.resolve({
      recordings: [recording('r2', 'Action Zwei')],
      total: 2,
      hasMore: false
    });
  },

  fetchClientRecordingFolder(options) {
    folderCalls.push(options);
    const currentPath = options.query.path;
    if (currentPath === 'Action') {
      return Promise.resolve({
        recordingFolder: true,
        path: currentPath,
        totalCount: 2,
        folderCount: 2,
        recordingCount: 0,
        returnedCount: 0,
        folders: [
          {
            path: 'Action/Leaf Film',
            name: 'Leaf Film',
            recordingCount: 1,
            singleRecordingLeaf: true,
            singleRecording: recording('f1', 'Ordner Film')
          },
          {
            path: 'Action/Marvel',
            name: 'Marvel',
            recordingCount: 2,
            singleRecordingLeaf: false
          }
        ],
        recordings: []
      });
    }
    return Promise.resolve({
      recordingFolder: true,
      path: currentPath,
      totalCount: 1,
      folderCount: 0,
      recordingCount: 1,
      returnedCount: 1,
      folders: [],
      recordings: [recording('f2', 'Marvel Film')]
    });
  }
};

const window = {
  document,
  console,
  VdrSuitePlatform: {
    getSelectedBackendId() { return 'backend-a'; },
    getClientApi() { return client; }
  },
  VdrSuiteRecordings2: {
    openRecording(recordingValue, options) {
      openRecordingCalls.push({recording: recordingValue, options});
    }
  },
  selectModule(name) { moduleSelections.push(name); },
  VdrSuiteHomeLivePreview: {cancel() {}},
  loadVdrSuiteDeferredRuntime() {
    window.VdrSuiteHomeRecordingDiscovery = {install() {}};
    return Promise.resolve(true);
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
  setTimeout
});
vm.runInContext(source, context, {filename: 'web/frontend/home-recording-discovery-bootstrap.js'});

function settle() {
  return new Promise(resolve => setTimeout(resolve, 0));
}

(async function () {
  const genreCard = makeCard('genre', 'action', 'Action');
  const genreSection = makeSection('genres', genreCard);

  const genreClick = clickElement(genreCard);
  assert.strictEqual(genreClick.defaultPrevented, true);
  assert.strictEqual(genreClick.immediatePropagationStopped, true);
  await settle();
  await settle();

  assert.deepStrictEqual(genreCalls.map(call => call.offset), [0, 1], 'genre inline expansion must paginate to all recordings');
  assert.strictEqual(moduleSelections.length, 0, 'category expansion must stay on Home');
  assert.strictEqual(genreCard.attributes['aria-expanded'], 'true');
  const genreExpansion = genreSection.querySelector('.media-home-inline-expansion');
  assert(genreExpansion, 'genre contents must appear directly below the genre rail');
  assert.strictEqual(genreExpansion.querySelectorAll('.media-home-discovery-card.recording').length, 2);

  const firstRecording = genreExpansion.querySelector('.media-home-discovery-card.recording');
  clickElement(firstRecording);
  await settle();
  assert.deepStrictEqual(moduleSelections, ['recordings2'], 'only choosing an actual recording may enter the canonical recording owner');
  assert.strictEqual(openRecordingCalls.length, 1);
  assert.strictEqual(openRecordingCalls[0].recording.recordingId, 'r1');
  assert.strictEqual(openRecordingCalls[0].options.backLabel, '← Zurück zu Home');

  clickElement(genreCard);
  await settle();
  assert.strictEqual(genreSection.querySelector('.media-home-inline-expansion'), null, 'second category click must collapse inline contents');
  assert.strictEqual(genreCard.attributes['aria-expanded'], 'false');

  const folderCard = makeCard('folder', 'Action', 'Action');
  const folderSection = makeSection('folders', folderCard);
  const folderClick = clickElement(folderCard);
  assert.strictEqual(folderClick.defaultPrevented, true);
  await settle();
  await settle();

  let folderExpansion = folderSection.querySelector('.media-home-inline-expansion');
  assert(folderExpansion, 'folder contents must appear directly below the folder rail');
  assert.strictEqual(folderCalls.length, 1, 'folder totalCount must not trigger bogus recording pagination when recordingCount is zero');
  assert.strictEqual(folderExpansion.querySelectorAll('.media-home-inline-folder').length, 1, 'non-leaf subfolders must stay inline');
  assert.strictEqual(folderExpansion.querySelectorAll('.media-home-discovery-card.recording').length, 1, 'embedded single-recording leaves must project as recordings');
  assert.strictEqual(folderExpansion.querySelector('.media-home-discovery-card.recording').dataset.recordingId, 'f1');
  assert.strictEqual(folderCalls[0].query.path, 'Action');

  const nestedFolder = folderExpansion.querySelector('.media-home-inline-folder');
  clickElement(nestedFolder);
  await settle();
  await settle();

  folderExpansion = folderSection.querySelector('.media-home-inline-expansion');
  assert.strictEqual(folderCalls[1].query.path, 'Action/Marvel');
  assert(folderExpansion.querySelector('.media-home-inline-back'), 'nested folder projection must expose inline back navigation');
  assert.strictEqual(folderExpansion.querySelectorAll('.media-home-discovery-card.recording').length, 1);
  assert.strictEqual(moduleSelections.length, 1, 'folder browsing itself must never leave Home');

  clickElement(folderExpansion.querySelector('.media-home-inline-back'));
  await settle();
  await settle();
  assert.strictEqual(folderCalls[2].query.path, 'Action', 'inline back navigation must return to the selected root folder');

  console.log('phase66 Home inline genre/folder discovery contract ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
