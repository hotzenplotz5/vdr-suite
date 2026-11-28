'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery-bootstrap.js'), 'utf8');

assert(source.includes("const CATEGORY_SELECTOR = '.media-home-discovery-card.genre, .media-home-discovery-card.folder'"));
assert(source.includes('fetchPublicGenreRecordings({'));
assert(!source.includes('fetchClientGenreRecordings('));
assert(source.includes('publicId ? paging.client.fetchPublicRecordingFolder : paging.client.fetchClientRecordingFolder'));
assert(source.includes('query: publicId ? {folderId: publicId'));
assert(source.includes('fetchClientRecordingFolder({'));
assert(source.includes("const INLINE_METADATA_CONCURRENCY = 4"));
assert(source.includes("client.requestJson('/api/v1/recordings/'"));
assert(!source.includes("client.requestJson('/api/vdr/recordings/metadata'"));
assert(source.includes('recordingMetadataPosterUrl'));
assert(source.includes('createInlineRecordingCard(projected, backendId, original)'));
assert(source.includes('recording && recording.nativeMetadata'));
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
    this.style = {};
    this.getBoundingClientRect = () => ({height: Number.parseFloat(this.style.minHeight) || 320});
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
    if (value === '[data-home-inline-kind="folder"]') return this.attributes['data-home-inline-kind'] === 'folder';
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

function recording(id, title, nativeMetadata) {
  const value = {
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
  if (nativeMetadata) value.nativeMetadata = nativeMetadata;
  return value;
}

function nativeImageUrl(id, kind, index) {
  return '/api/vdr/recordings/metadata/image' +
    '?backend=backend-a' +
    '&backendNativeId=native-' + id +
    '&kind=' + kind +
    '&index=' + String(index);
}

function richMetadata(nativeId) {
  const id = String(nativeId).replace(/^native-/, '');
  if (id === 'r2') return {available: false};
  return {
    available: true,
    title: 'Kanonisch ' + id.toUpperCase(),
    overview: 'Scraper-Zusammenfassung ' + id,
    images: [
      {
        orientation: 'landscape',
        image: {
          available: true,
          url: nativeImageUrl(id, 'gallery', 0),
          width: 1600,
          height: 900
        }
      },
      {
        orientation: 'portrait',
        image: {
          available: true,
          url: nativeImageUrl(id, 'gallery', 1),
          width: 1000,
          height: 1500
        }
      }
    ],
    preferredArtwork: {
      available: true,
      url: nativeImageUrl(id, 'preferred', 0),
      width: 1600,
      height: 900
    }
  };
}

function genreRecordingId(number) {
  return 'rec_' + String(number).padStart(32, '0');
}
function genreArtworkId(number, variant) {
  return String(number).padStart(31, '0') + variant;
}
function genreArtworkUrl(number, variant) {
  return '/api/v1/recordings/' + genreRecordingId(number) + '/artwork/' +
    genreArtworkId(number, variant) + '?backendId=backend-a';
}
const genreRecordings = ['Eins', 'Zwei', 'Drei', 'Vier', 'Fünf', 'Sechs'].map((label, index) => ({
  recordingId:genreRecordingId(index + 1), backendId:'backend-a',
  recordedAt:'2026-11-28T10:00:00Z', title:'Action ' + label,
  preferredArtworkId:genreArtworkId(index + 1, 0)
}));
function publicGenreDetail(recording) {
  const number = genreRecordings.indexOf(recording) + 1;
  return Object.assign({}, recording, {metadata:number === 2 ? {
    state:'absent', source:'none', contentKind:'unknown'
  } : {
    state:'available', source:'restfulapi-scraper-bridge', contentKind:'movie',
    providerTitle:'Kanonisch R' + number,
    overview:'Scraper-Zusammenfassung r' + number,
    images:[
      {orientation:'landscape',image:{available:true,url:genreArtworkUrl(number,1)}},
      {orientation:'portrait',image:{available:true,url:genreArtworkUrl(number,2)}}
    ],
    preferredArtwork:{available:true,url:genreArtworkUrl(number,0)}
  }});
}

const client = {
  fetchClientGenreRecordings() { throw new Error('Private genre read'); },
  fetchPublicGenreRecordings(options) {
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
    assert.strictEqual(options.query.backendId, 'backend-a');
    assert.strictEqual(options.query.backendNativeId, undefined);
    const recording = genreRecordings.find(item => pathValue === '/api/v1/recordings/' + item.recordingId);
    assert(recording, 'metadata must use the exact public member identity');
    metadataCalls.push({pathValue, options});
    activeMetadata += 1;
    maximumActiveMetadata = Math.max(maximumActiveMetadata, activeMetadata);
    return new Promise(resolve => {
      setTimeout(() => {
        activeMetadata -= 1;
        resolve(publicGenreDetail(recording));
      }, 8);
    });
  },

  fetchClientRecordingFolder(options) {
    folderCalls.push(options);
    const currentPath = options.query.path;
    const combined = options.query.homeCards === 1;
    if (currentPath === 'Action') {
      return Promise.resolve({
        recordingFolder: true,
        ...(combined ? {homeCardPage:true,offset:0,returnedCards:2,hasMore:false,cacheReady:true} : {}),
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
            singleRecording: recording(
              'f1',
              'Ordner Film',
              richMetadata('native-f1')
            )
          },
          {
            path: 'Action/Marvel',
            name: 'Marvel_Collection',
            recordingCount: 2,
            singleRecordingLeaf: false,
            ...(combined ? {representativePosterUrl:nativeImageUrl('f2','gallery',1)} : {})
          }
        ],
        recordings: []
      });
    }
    return Promise.resolve({
      recordingFolder: true,
      ...(combined ? {homeCardPage:true,offset:0,returnedCards:1,hasMore:false,cacheReady:true} : {}),
      path: currentPath,
      totalCount: 1,
      folderCount: 0,
      recordingCount: 1,
      returnedCount: 1,
      folders: [],
      recordings: [
        recording(
          'f2',
          'Marvel Film',
          richMetadata('native-f2')
        )
      ]
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
// Exercise the production representative traversal, using the fixture's
// canonical folder and recording projections.
window.VdrSuiteRecordings2Shared = {
  recordingList: value => value && value.recordings || [],
  folderList: value => value && value.folders || [],
  normalizePath: value => String(value || ''),
  first: (value, keys, fallback) => keys.map(key => value && value[key]).find(Boolean) || fallback,
  decodeDisplayText: value => String(value || '').replace(/_/g, ' ')
};
vm.runInContext(fs.readFileSync(path.join(frontendRoot, 'recordings2-folder-artwork.js'), 'utf8'),
  context, {filename: 'web/frontend/recordings2-folder-artwork.js'});
// Exercise the production helper rather than a stub: this includes the
// automatically expanded folder's exact Home image URL transformation.
vm.runInContext(fs.readFileSync(path.join(frontendRoot, 'platform', 'helpers.js'), 'utf8'),
  context, {filename: 'web/frontend/platform/helpers.js'});
vm.runInContext(source, context, {filename: 'web/frontend/home-recording-discovery-bootstrap.js'});

function bootstrapScroll(rail) {
  window.VdrSuiteHomeRecordingDiscoveryBootstrap.__test.handleRailScroll({target:rail});
}

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
    'raw recording cards must not flash before their public metadata lookup settles'
  );

  await wait(40);
  genreExpansion = genreSection.querySelector('.media-home-inline-expansion');
  const genreCards = genreExpansion.querySelectorAll('.media-home-discovery-card.recording');
  assert.strictEqual(genreCards.length, genreRecordings.length);
  assert.strictEqual(metadataCalls.length, genreRecordings.length);
  assert.strictEqual(maximumActiveMetadata, 4, 'inline public metadata reads must stay bounded to four concurrent requests');
  assert.deepStrictEqual(
    metadataCalls.map(call => call.pathValue),
    genreRecordings.map(item => '/api/v1/recordings/' + item.recordingId),
    'public metadata must be resolved in canonical genre order'
  );

  assert.strictEqual(genreCards[5].querySelector('strong').textContent, 'Kanonisch R6');
  assert.strictEqual(genreCards[5].querySelector('img').src, genreArtworkUrl(6, 2) + '&variant=home');
  const firstRecording = genreCards[0];
  assert.strictEqual(firstRecording.dataset.recordingId, genreRecordingId(1));
  assert.strictEqual(firstRecording.querySelector('strong').textContent, 'Kanonisch R1');
  assert.strictEqual(firstRecording.querySelector('img').loading, 'lazy');
  assert.strictEqual(firstRecording.querySelector('img').decoding, 'async');
  assert.strictEqual(
    firstRecording.querySelector('img').src,
    genreArtworkUrl(1, 2) + '&variant=home'
  );

  const notFoundRecording = genreCards[1];
  assert.strictEqual(notFoundRecording.dataset.recordingId, genreRecordingId(2));
  assert.strictEqual(
    notFoundRecording.querySelector('strong').textContent,
    'Action Zwei',
    'absent public metadata must retain the recording title'
  );
  assert.strictEqual(notFoundRecording.querySelector('img').src, genreArtworkUrl(2, 0) + '&variant=home');

  clickElement(firstRecording);
  await settle();
  assert.deepStrictEqual(moduleSelections, ['recordings2'], 'only choosing an actual recording may enter the canonical recording owner');
  assert.strictEqual(openRecordingCalls.length, 1);
  assert.strictEqual(openRecordingCalls[0].recording, genreRecordings[0], 'presentation projection must never replace the canonical recording handoff');
  assert.strictEqual(openRecordingCalls[0].recording.path, undefined);
  assert.strictEqual(openRecordingCalls[0].recording.backendNativeId, undefined);
  assert.strictEqual(openRecordingCalls[0].options.backLabel, '← Zurück zu Home');

  clickElement(genreCard);
  await settle();
  assert.strictEqual(genreSection.querySelector('.media-home-inline-expansion'), null, 'second category click must collapse inline contents');
  assert.strictEqual(genreCard.attributes['aria-expanded'], 'false');

  const metadataReadsBeforeFolder = metadataCalls.length;
  const folderCard = makeCard('folder', 'Action', 'Action');
  const folderSection = makeSection('folders', folderCard);
  const prioritizedRails = [];
  window.VdrSuiteHomeRecordingDiscovery = {prioritizeInlineRailArtwork: rail => prioritizedRails.push(rail)};
  const folderClick = clickElement(folderCard);
  assert.strictEqual(folderClick.defaultPrevented, true);
  await settle();
  await settle();

  let folderExpansion = folderSection.querySelector('.media-home-inline-expansion');
  assert(folderExpansion, 'folder contents must appear directly below the folder rail');
  assert.strictEqual(prioritizedRails.at(-1), folderExpansion.querySelector('.media-home-discovery-rail'));
  bootstrapScroll(folderExpansion.querySelector('.media-home-discovery-rail'));
  assert.strictEqual(prioritizedRails.length, 2, 'horizontal scrolling must reprioritize the actual inline rail');
  const ordinaryMovieRail = new FakeElement('div','media-home-discovery-rail recent-movies');
  bootstrapScroll(ordinaryMovieRail);
  assert.strictEqual(prioritizedRails.at(-1), ordinaryMovieRail,
    'horizontal scrolling in ordinary Home movie rails must also prewarm their adjacent posters');
  assert.strictEqual(folderCalls.filter(call => call.query.limit === 12).length, 1, 'manual folder demand uses one bounded combined page');
  assert.strictEqual(folderExpansion.querySelectorAll('.media-home-inline-folder').length, 1, 'non-leaf subfolders must stay inline');
  assert.strictEqual(folderExpansion.querySelectorAll('.media-home-discovery-card.recording').length, 1, 'embedded single-recording leaves must project as recordings');
  const leafRecording =
    folderExpansion.querySelector('.media-home-discovery-card.recording');
  assert.strictEqual(leafRecording.dataset.recordingId, 'f1');
  assert.strictEqual(
    leafRecording.querySelector('strong').textContent,
    'Kanonisch F1',
    'folder recording must use embedded canonical metadata title'
  );
  assert.strictEqual(
    leafRecording.querySelector('img').src,
    nativeImageUrl('f1', 'gallery', 1) + '&variant=home',
    'TVScraper portrait must beat the weak recording frame'
  );
  assert.strictEqual(
    metadataCalls.length,
    metadataReadsBeforeFolder,
    'folder cards must not add per-recording metadata HTTP reads'
  );
  assert.strictEqual(folderCalls[0].query.path, 'Action');

  const nestedFolder = folderExpansion.querySelector('.media-home-inline-folder');
  assert.strictEqual(nestedFolder.querySelector('strong').textContent, 'Marvel Collection');
  assert.strictEqual(nestedFolder.dataset.inlineFolderPath, 'Action/Marvel');
  assert.strictEqual(nestedFolder.querySelector('img').src, nativeImageUrl('f2', 'gallery', 1) + '&variant=home',
    'visible subfolder must show the existing canonical representative cover');
  assert.deepStrictEqual(folderCalls.filter(call => call.query.limit === 1).map(call => call.query.path), [],
    'server representative must not require a per-folder HTTP read');
  clickElement(nestedFolder);
  assert.strictEqual(folderSection.querySelector('.media-home-inline-expansion').style.minHeight, '320px',
    'switch must reserve previous folder height while loading');
  await settle();
  await settle();

  folderExpansion = folderSection.querySelector('.media-home-inline-expansion');
  assert.strictEqual(folderCalls.filter(call => call.query.limit === 12)[1].query.path, 'Action/Marvel');
  assert(folderExpansion.querySelector('.media-home-inline-back'), 'nested folder projection must expose inline back navigation');
  assert.strictEqual(folderExpansion.style.minHeight, '', 'successful folder render releases reservation');
  assert.strictEqual(folderExpansion.querySelectorAll('.media-home-discovery-card.recording').length, 1);
  const nestedRecording =
    folderExpansion.querySelector('.media-home-discovery-card.recording');
  assert.strictEqual(
    nestedRecording.querySelector('strong').textContent,
    'Kanonisch F2'
  );
  assert.strictEqual(
    nestedRecording.querySelector('img').src,
    nativeImageUrl('f2', 'gallery', 1) + '&variant=home'
  );
  assert.strictEqual(
    metadataCalls.length,
    metadataReadsBeforeFolder,
    'nested folder cards must remain metadata-HTTP-free'
  );
  assert.strictEqual(moduleSelections.length, 1, 'folder browsing itself must never leave Home');

  clickElement(folderExpansion.querySelector('.media-home-inline-back'));
  await settle();
  await settle();
  assert.strictEqual(folderCalls.filter(call => call.query.limit === 12)[2].query.path, 'Action', 'inline back navigation must return to the selected root folder');

  // Automatic expansion must request a genuine combined card page: hold
  // the 24-card lookahead and verify the initial twelve already exist.
  const bootstrap = window.VdrSuiteHomeRecordingDiscoveryBootstrap;
  const randomCard = makeCard('folder', 'Doku', 'Doku');
  const randomSection = makeSection('folders', randomCard);
  const totalCards = Array.from({length: 40}, (_, i) => recording('d' + i, 'Doku ' + i));
  const timers = [];
  window.setTimeout = callback => { timers.push(callback); return timers.length; };
  window.VdrSuiteBrowserSession = {isAuthenticated: () => true};
  window.VdrSuitePlatform.getSelectedModule = () => 'overview';
  let fenceEpoch = 1;
  window.VdrSuiteHomeCache = {captureRailFence() {
    const epoch = fenceEpoch; return () => epoch === fenceEpoch;
  }};
  let releaseLookahead;
  const originalFolderFetch = client.fetchClientRecordingFolder;
  client.fetchClientRecordingFolder = function (options) {
    if (options.query.path !== 'Doku') return originalFolderFetch(options);
    folderCalls.push(options);
    const {offset, limit, homeCards} = options.query;
    assert.strictEqual(homeCards, 1);
    const items = totalCards.slice(offset, offset + limit);
    // Include embedded leaves in the combined count; the renderer must
    // project those as recordings, retaining native metadata and identity.
    const response = {homeCardPage: true, offset, returnedCards: items.length,
      hasMore: offset + items.length < totalCards.length,
      folders: items.map(item => ({path:'Doku/' + item.recordingId, name:item.title,
        singleRecordingLeaf:true, singleRecording:item})), recordings: []};
    if (limit === 24) return new Promise(resolve => { releaseLookahead = () => resolve(response); });
    return Promise.resolve(response);
  };
  const beforeRandom = folderCalls.length;
  assert.strictEqual(await bootstrap.openRandomFolderInline(randomCard), true);
  const randomExpansion = randomSection.querySelector('.media-home-inline-expansion');
  const randomRail = randomExpansion.querySelector('.media-home-discovery-rail');
  assert.strictEqual(randomRail.children.length, 12);
  assert.strictEqual(folderCalls.length, beforeRandom + 1);
  assert.strictEqual(folderCalls.at(-1).query.limit, 12);
  assert.strictEqual(timers.length, 1);
  timers.shift()();
  await settle();
  assert.strictEqual(folderCalls.at(-1).query.limit, 24);
  assert.strictEqual(folderCalls.at(-1).query.offset, 12);
  assert.strictEqual(randomRail.children.length, 12);
  const more = randomExpansion.children.find(node => node.textContent === 'Weitere Aufnahmen');
  assert(more);
  clickElement(more);
  await settle();
  assert.strictEqual(folderCalls.length, beforeRandom + 2, 'demand joins pending lookahead');
  releaseLookahead();
  await settle();
  assert.strictEqual(randomRail.children.length, 36);
  assert.strictEqual(randomExpansion.querySelector('.media-home-discovery-rail'), randomRail);
  assert.strictEqual(new Set(randomRail.children.map(node => node.dataset.recordingId)).size, 36);
  timers.shift()();
  await settle();
  ++fenceEpoch;
  releaseLookahead();
  await settle();
  clickElement(more);
  await settle();
  assert.strictEqual(randomRail.children.length, 40);
  assert.strictEqual(folderCalls.at(-1).query.limit, 12, 'invalidated speculative page requires demand read');
  assert.strictEqual(more.parentNode, null);
  // Do not replace a manually/automatically opened folder with a later
  // scheduled auto-selection, and reject responses after collapse.
  assert.strictEqual(await bootstrap.openRandomFolderInline(randomCard), false);
  clickElement(randomCard);
  assert.strictEqual(randomSection.querySelector('.media-home-inline-expansion'), null);
  const initialAgain = bootstrap.openRandomFolderInline(randomCard);
  await initialAgain;
  const staleExpansion = randomSection.querySelector('.media-home-inline-expansion');
  const staleRail = staleExpansion.querySelector('.media-home-discovery-rail');
  timers.shift()();
  await settle();
  clickElement(randomCard);
  releaseLookahead();
  await settle();
  assert.strictEqual(randomSection.querySelector('.media-home-inline-expansion'), null);
  assert.strictEqual(staleRail.children.length, 12);
  // Real click enters the same owner with a first-page preview while
  // the authoritative response is held. Preview cards cannot open recordings.
  const warmCard = makeCard('folder', 'Warm', 'Warm');
  const warmSection = makeSection('folders', warmCard);
  let releaseWarm, paintWarm, currentWarm, removeWarm, savedWarm, acceptedWarm;
  const warmResponse = {homeCardPage:true, path:'Warm', cacheReady:true,
    offset:0, returnedCards:1, hasMore:false, folders:[],
    recordings:[recording('warm-new','Fresh recording')]};
  client.fetchClientRecordingFolder = options => {
    assert.strictEqual(options.query.homeCards,1);
    assert.strictEqual(options.query.limit,12);
    return new Promise(resolve => { releaseWarm = () => resolve(warmResponse); });
  };
  window.VdrSuiteHomeCache = {beginFolder(path,backend,current,paint) {
    assert.strictEqual(path,'Warm'); assert.strictEqual(backend,'backend-a');
    paintWarm=paint; currentWarm=current;
    return {accept(payload){acceptedWarm=payload;},
      finish(cards){savedWarm=cards;if(removeWarm)removeWarm();},
      cancel(){if(removeWarm)removeWarm();}};
  }};
  clickElement(warmCard);
  await settle();
  assert(currentWarm());
  removeWarm=paintWarm([{id:'r:old',title:'Cached recording',subtitle:'',posterUrl:''}]);
  assert.strictEqual(typeof removeWarm,'function');
  const warmExpansion=warmSection.querySelector('.media-home-inline-expansion');
  const warmRails=warmExpansion.querySelectorAll('.media-home-discovery-rail');
  assert.strictEqual(warmRails.length,2);
  assert.strictEqual(warmRails[0].hidden,true);
  assert.strictEqual(warmRails[1].children[0].tagName,'DIV');
  assert.strictEqual(warmRails[1].children[0].querySelector('strong').textContent,'Cached recording');
  releaseWarm(); await settle(); await settle();
  assert.strictEqual(acceptedWarm,warmResponse);
  assert.strictEqual(savedWarm[0].id,'r:warm-new');
  assert.strictEqual(warmExpansion.querySelectorAll('.media-home-discovery-rail').length,1);
  assert.strictEqual(warmRails[0].hidden,false);
  assert.strictEqual(warmRails[0].children[0].dataset.recordingId,'warm-new');
  clickElement(warmCard);
  assert.strictEqual(currentWarm(),false);
  client.fetchClientRecordingFolder = originalFolderFetch;
  const seriesCard=makeCard('folder','Serien','Serien');
  seriesCard.__vdrSuiteFolderEntry={path:'Serien',recordingCount:464};
  const seriesSection=makeSection('folders',seriesCard);
  let mounted=0, stopped=0;
  window.VdrSuiteHomeRecordingDiscovery={mountFolderSeries(expansion,path,backend,config) {
    assert.strictEqual(path,'Serien'); assert.strictEqual(backend,'backend-a');
    assert.strictEqual(config.count(),464); assert(config.current());
    expansion.__stopFolderSeries=()=>{stopped++;}; mounted++; return true;
  }};
  const beforeSeries=folderCalls.length;
  clickElement(seriesCard); await settle();
  assert.strictEqual(mounted,1);
  assert.strictEqual(folderCalls.length,beforeSeries,'complete series model needs no duplicate folder HTTP read');
  clickElement(seriesCard);
  assert.strictEqual(stopped,1);
  assert.strictEqual(seriesSection.querySelector('.media-home-inline-expansion'),null);
  // Public Home folders use opaque identities, not reconstructed native paths.
  delete window.VdrSuiteHomeRecordingDiscovery;
  delete window.VdrSuiteHomeCache;
  const publicRoot='fld1_'+'a'.repeat(32), publicChild='fld1_'+'b'.repeat(32);
  const publicRecording='rec_'+'c'.repeat(32), artwork='d'.repeat(32);
  const publicCalls=[];
  const publicCard=makeCard('folder',publicRoot,'Public root');
  publicCard.__vdrSuiteFolderEntry={folderId:publicRoot,name:'Public root',recordingCount:1};
  const publicSection=makeSection('folders',publicCard);
  client.fetchClientRecordingFolder=()=>{throw new Error('Unexpected private folder read');};
  client.fetchPublicRecordingFolder=options=>{
    publicCalls.push(options);
    assert(!Object.hasOwn(options.query,'path'));
    const root=options.query.folderId===publicRoot;
    assert(root || options.query.folderId===publicChild);
    return Promise.resolve({publicFolder:true,recordingFolder:true,offset:0,
      returnedCards:1,nextOffset:1,hasMore:false,
      folders:root?[{folderId:publicChild,name:'Nested child',recordingCount:1,
        representativeRecording:{recordingId:publicRecording,backendId:'backend-a',preferredArtworkId:artwork}}]:[],
      recordings:root?[]:[{recordingId:publicRecording,backendId:'backend-a',title:'Public recording',preferredArtworkId:artwork}]});
  };
  clickElement(publicCard); await settle(); await settle();
  let publicExpansion=publicSection.querySelector('.media-home-inline-expansion');
  const publicNested=publicExpansion.querySelector('.media-home-inline-folder');
  assert(publicNested);
  assert.strictEqual(publicNested.dataset.inlineFolderPath,publicChild);
  assert.strictEqual(publicNested.querySelector('strong').textContent,'Nested child');
  clickElement(publicNested); await settle(); await settle();
  publicExpansion=publicSection.querySelector('.media-home-inline-expansion');
  assert.strictEqual(publicExpansion.querySelector('.media-home-discovery-card.recording').dataset.recordingId,publicRecording);
  clickElement(publicExpansion.querySelector('.media-home-inline-back')); await settle(); await settle();
  assert.deepStrictEqual(publicCalls.map(call=>call.query.folderId),[publicRoot,publicChild,publicRoot]);
  console.log('phase66 Home inline genre public metadata, paginated folder and preview contract ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
