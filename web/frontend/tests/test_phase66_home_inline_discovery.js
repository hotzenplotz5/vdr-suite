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
assert(source.includes("const INLINE_METADATA_CONCURRENCY = 4"));
assert(source.includes("client.requestJson('/api/vdr/recordings/metadata'"));
assert(source.includes('recordingMetadataPosterUrl'));
assert(source.includes('createInlineRecordingCard(projected, backendId, original)'));
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
    if (value === 'img') return this.tagName === 'IMG';
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
const metadataCalls = [];
const openRecordingCalls = [];
const moduleSelections = [];
let activeMetadata = 0;
let maximumActiveMetadata = 0;

function recording(id, title) {
  return {
    recordingId: id,
    backendId: 'backend-a',
    backendNativeId: 'native-' + id,
    path: 'Action/' + title,
    title: 'Action/' + title,
    metadata: {
      presentation: {
        title: 'Action/' + title,
        posterUrl: '/weak/' + id + '.jpg'
      },
      artwork: {preferredUrl: '/weak/' + id + '.jpg'}
    }
  };
}

function richMetadata(nativeId) {
  const id = String(nativeId).replace(/^native-/, '');
  if (id === 'r2') return {available: false};
  return {
    available: true,
    title: 'Kanonisch ' + id.toUpperCase(),
    overview: 'Scraper-Zusammenfassung ' + id,
    images: [
      {url: '/metadata/' + id + '-landscape.jpg', width: 1600, height: 900},
      {url: '/metadata/' + id + '-poster.jpg', width: 1000, height: 1500}
    ],
    preferredArtwork: {url: '/metadata/' + id + '-landscape.jpg', width: 1600, height: 900}
  };
}

const genreRecordings = [
  recording('r1', 'Action Eins'),
  recording('r2', 'Action Zwei'),
  recording('r3', 'Action Drei'),
  recording('r4', 'Action Vier'),
  recording('r5', 'Action Fünf'),
  recording('r6', 'Action Sechs')
];

const client = {
  fetchClientGenreRecordings(options) {
    genreCalls.push(options);
    if (options.offset === 0) {
      return Promise.resolve({
        recordings: [genreRecordings[0]],
        total: genreRecordings.length,
        hasMore: true
      });
    }
    return Promise.resolve({
      recordings: genreRecordings.slice(1),
      total: genreRecordings.length,
      hasMore: false
    });
  },

  requestJson(pathValue, options) {
    assert.strictEqual(pathValue, '/api/vdr/recordings/metadata');
    metadataCalls.push(options);
    activeMetadata += 1;
    maximumActiveMetadata = Math.max(maximumActiveMetadata, activeMetadata);
    const nativeId = options.query.backendNativeId;
    return new Promise(resolve => {
      setTimeout(() => {
        activeMetadata -= 1;
        resolve(richMetadata(nativeId));
      }, 8);
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
  VdrSuiteFrontendHelpers: {
    recordingMetadataPosterUrl(value) {
      const images = value && Array.isArray(value.images) ? value.images : [];
      const portrait = images.find(image => Number(image.height) > Number(image.width));
      return portrait ? portrait.url : String(value && value.preferredArtwork && value.preferredArtwork.url || '');
    }
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

function wait(milliseconds) {
  return new Promise(resolve => setTimeout(resolve, milliseconds));
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
  let genreExpansion = genreSection.querySelector('.media-home-inline-expansion');
  assert(genreExpansion, 'genre contents must appear directly below the genre rail');
  assert.strictEqual(
    genreExpansion.querySelectorAll('.media-home-discovery-card.recording').length,
    0,
    'raw recording cards must not flash before their native metadata lookup settles'
  );

  await wait(40);
  genreExpansion = genreSection.querySelector('.media-home-inline-expansion');
  const genreCards = genreExpansion.querySelectorAll('.media-home-discovery-card.recording');
  assert.strictEqual(genreCards.length, genreRecordings.length);
  assert.strictEqual(metadataCalls.length, genreRecordings.length);
  assert.strictEqual(maximumActiveMetadata, 4, 'inline native metadata reads must stay bounded to four concurrent requests');
  assert.deepStrictEqual(
    metadataCalls.map(call => call.query.backendNativeId),
    genreRecordings.map(item => item.backendNativeId),
    'native metadata must be resolved in canonical genre order'
  );

  const firstRecording = genreCards[0];
  assert.strictEqual(firstRecording.dataset.recordingId, 'r1');
  assert.strictEqual(firstRecording.querySelector('strong').textContent, 'Kanonisch R1');
  assert.strictEqual(firstRecording.querySelector('img').src, '/metadata/r1-poster.jpg');

  const notFoundRecording = genreCards[1];
  assert.strictEqual(notFoundRecording.dataset.recordingId, 'r2');
  assert.strictEqual(
    notFoundRecording.querySelector('strong').textContent,
    'Action/Action Zwei',
    'real native metadata misses must retain the existing recording fallback'
  );
  assert.strictEqual(notFoundRecording.querySelector('img').src, '/weak/r2.jpg');

  clickElement(firstRecording);
  await settle();
  assert.deepStrictEqual(moduleSelections, ['recordings2'], 'only choosing an actual recording may enter the canonical recording owner');
  assert.strictEqual(openRecordingCalls.length, 1);
  assert.strictEqual(openRecordingCalls[0].recording, genreRecordings[0], 'presentation projection must never replace the canonical recording handoff');
  assert.strictEqual(openRecordingCalls[0].recording.path, 'Action/Action Eins');
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

  console.log('phase66 Home inline genre native metadata and folder discovery contract ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
