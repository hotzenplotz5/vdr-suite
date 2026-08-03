'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-metadata-detail.js'),
  'utf8'
);
const ownerSource = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-detail-owner.js'),
  'utf8'
);

class MockElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.parentNode = null;
    this.className = '';
    this.dataset = {};
    this.attributes = {};
    this.style = {};
    this.textContent = '';
    this.hidden = false;
    this.disabled = false;
    this.listeners = {};
    this.id = '';
  }

  get firstChild() {
    return this.children[0] || null;
  }

  get classList() {
    return {
      add: (...names) => {
        const values = new Set(this.className.split(/\s+/).filter(Boolean));
        names.forEach(name => values.add(name));
        this.className = Array.from(values).join(' ');
      },
      contains: name => this.className.split(/\s+/).filter(Boolean).includes(name)
    };
  }

  appendChild(child) {
    this.children.push(child);
    child.parentNode = this;
    return child;
  }

  insertBefore(child, reference) {
    if (!reference) return this.appendChild(child);
    const index = this.children.indexOf(reference);
    assert.ok(index >= 0, 'insertBefore reference must be a direct child');
    this.children.splice(index, 0, child);
    child.parentNode = this;
    return child;
  }

  replaceChildren(...children) {
    this.children.forEach(child => { child.parentNode = null; });
    this.children = [];
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
  }

  addEventListener(name, listener) {
    if (!this.listeners[name]) this.listeners[name] = [];
    this.listeners[name].push(listener);
  }

  click() {
    (this.listeners.click || []).forEach(listener => listener({target: this, currentTarget: this}));
  }

  querySelector(selector) {
    return this.querySelectorAll(selector)[0] || null;
  }

  querySelectorAll(selector) {
    const results = [];
    const matches = element => {
      if (selector.startsWith('.')) {
        return element.classList.contains(selector.slice(1));
      }
      if (selector.startsWith('[') && selector.endsWith(']')) {
        const content = selector.slice(1, -1);
        const [rawName, rawValue] = content.split('=');
        const attribute = rawName.replace(/^data-/, '').replace(/-([a-z])/g, (_, ch) => ch.toUpperCase());
        if (rawValue === undefined) return Object.prototype.hasOwnProperty.call(element.dataset, attribute);
        return String(element.dataset[attribute]) === rawValue.replace(/^"|"$/g, '');
      }
      return element.tagName === selector.toUpperCase();
    };
    const visit = element => {
      element.children.forEach(child => {
        if (matches(child)) results.push(child);
        visit(child);
      });
    };
    visit(this);
    return results;
  }
}

class MockImage {
  constructor() {
    this.onload = null;
    this._src = '';
  }

  set src(value) {
    this._src = String(value || '');
    if (typeof this.onload === 'function') this.onload();
  }

  get src() {
    return this._src;
  }
}

const document = {
  head: new MockElement('head'),
  documentElement: new MockElement('html'),
  createElement(tagName) {
    return new MockElement(tagName);
  },
  getElementById(id) {
    return this.head.children.find(child => child.id === id) || null;
  }
};

let metadataRequest = null;
let metadataRequestCount = 0;
const scheduledTimeouts = [];
const resolvedPublicPaths = [];
let recordingSearch = null;
const metadata = {
  available: true,
  status: 'ready',
  mediaType: 'series',
  title: 'Tatort',
  originalTitle: 'Tatort',
  episodeName: 'Die Nacht der Kommissare',
  seasonNumber: 2023,
  episodeNumber: 22,
  runtimeMinutes: 90,
  releaseDate: '1970-11-29',
  firstAired: '2023-06-18',
  imdbId: 'tt0806910',
  genres: ['Drama', 'Crime'],
  networks: ['Das Erste'],
  productionCountries: [],
  providerHints: {hd: 1, language: 4},
  overview: 'Ausführliche Beschreibung.',
  preferredArtwork: {
    available: true,
    url: '/api/epg/cache/metadata/image?kind=preferred&index=0'
  },
  people: [
    {
      role: 'actor',
      name: 'Klaus J. Behrendt',
      characterName: 'Max Ballauf',
      image: {
        available: true,
        url: '/api/epg/cache/metadata/image?kind=person&index=0'
      }
    },
    {
      role: 'actor',
      name: 'Dietmar Bär',
      characterName: 'Freddy Schenk',
      image: {available: false, url: ''}
    }
  ],
  images: [
    {
      orientation: 'landscape',
      image: {
        available: true,
        url: '/api/epg/cache/metadata/image?kind=gallery&index=0'
      }
    },
    {
      orientation: 'portrait',
      image: {
        available: true,
        url: '/api/epg/cache/metadata/image?kind=gallery&index=1'
      }
    }
  ]
};

const window = {
  Image: MockImage,
  setTimeout(callback) {
    scheduledTimeouts.push(callback);
    return scheduledTimeouts.length;
  },
  VdrSuitePlatform: {
    getSelectedBackendId() {
      return 'default';
    }
  },
  VdrSuitePublicUrl: {
    resolvePath(pathName) {
      const value = String(pathName || '');
      resolvedPublicPaths.push(value);
      return '/vdr-suite' + value;
    }
  },
  VdrSuiteClientApi: {
    requestJson(pathName, options) {
      metadataRequest = {pathName, options};
      metadataRequestCount += 1;
      return Promise.resolve(metadataRequestCount < 3
        ? {available: false, status: 'pending'}
        : metadata);
    },
    fetchClientRecordingPersons(options) {
      recordingSearch = options;
      return Promise.resolve({
        matches: [
          {recording: {id: 'rec-1', title: 'Tatort Aufnahme', backendId: 'default'}}
        ]
      });
    }
  }
};

const context = vm.createContext({
  Array,
  Boolean,
  Date,
  Error,
  Map,
  Math,
  Number,
  Object,
  Promise,
  Set,
  String,
  URLSearchParams,
  console,
  document,
  encodeURIComponent,
  window
});

vm.runInContext(source, context, {filename: 'epg-metadata-detail.js'});

async function run() {
  assert.ok(window.VdrSuiteEpgMetadataDetail);
  assert.strictEqual(window.VdrSuiteEpgMetadataDetail.formatDate('2023-06-18'), '18.06.2023');
  assert.strictEqual(window.VdrSuiteEpgMetadataDetail.mediaTypeLabel('series'), 'Serie');
  assert.strictEqual(window.VdrSuiteEpgMetadataDetail.roleLabel('actor'), 'Schauspiel');
  assert.strictEqual(
    window.VdrSuiteEpgMetadataDetail.isPublicImageUrl('/var/cache/private.jpg'),
    false
  );

  const detail = new MockElement('article');
  detail.className = 'channels2-detail has-artwork';
  const existingArtwork = new MockElement('div');
  existingArtwork.className = 'channels2-artwork epg-detail-artwork';
  existingArtwork.style.backgroundImage =
    'url("/api/epg/cache/artwork?backend=default&channelId=C-1-1079-11110&eventId=37059")';
  const hero = new MockElement('div');
  hero.className = 'epg-detail-hero';
  const metaGrid = new MockElement('div');
  metaGrid.className = 'epg-detail-meta-grid';
  const description = new MockElement('div');
  description.className = 'epg-detail-description';
  const actions = new MockElement('div');
  actions.className = 'epg-detail-actions';
  detail.appendChild(existingArtwork);
  detail.appendChild(hero);
  detail.appendChild(metaGrid);
  detail.appendChild(description);
  detail.appendChild(actions);

  window.VdrSuiteEpgMetadataDetail.enhance(
    detail,
    {id: '18829', channelId: 'C-61441-10014-10355', title: 'Tatort'},
    {id: 'C-61441-10014-10355'}
  );

  for (let attempt = 0;
       attempt < 20 && detail.dataset.epgMetadataAvailable !== 'true';
       attempt += 1) {
    while (scheduledTimeouts.length) {
      scheduledTimeouts.shift()();
    }
    await Promise.resolve();
    await Promise.resolve();
  }

  assert.ok(metadataRequest);
  assert.strictEqual(metadataRequestCount, 3);
  assert.strictEqual(metadataRequest.pathName, '/api/epg/cache/metadata');
  assert.strictEqual(metadataRequest.options.query.backend, 'default');
  assert.strictEqual(metadataRequest.options.query.eventId, '18829');
  assert.strictEqual(detail.dataset.epgMetadataAvailable, 'true');
  assert.strictEqual(detail.querySelectorAll('.epg-metadata-tab').length, 4);
  assert.strictEqual(detail.querySelectorAll('.epg-person-card').length, 2);
  assert.strictEqual(detail.querySelectorAll('.epg-gallery-thumb').length, 2);
  assert.strictEqual(detail.querySelectorAll('.epg-detail-artwork').length, 1);
  assert.ok(detail.textContent === '' || typeof detail.textContent === 'string');

  const metadataArtwork = detail.querySelector('.epg-detail-artwork');
  assert.strictEqual(metadataArtwork, existingArtwork);
  assert.strictEqual(
    metadataArtwork.style.backgroundImage,
    'url("/vdr-suite/api/epg/cache/metadata/image?kind=preferred&index=0")'
  );
  assert.ok(detail.classList.contains('epg-has-artwork'));
  assert.strictEqual(
    resolvedPublicPaths[0],
    '/api/epg/cache/metadata/image?kind=preferred&index=0'
  );

  const castButtons = detail.querySelectorAll('.epg-person-card');
  castButtons[0].click();
  await Promise.resolve();
  await Promise.resolve();

  assert.ok(recordingSearch);
  assert.strictEqual(recordingSearch.backendId, 'default');
  assert.strictEqual(recordingSearch.query.name, 'Klaus J. Behrendt');

  vm.runInContext(`
    function createEpgEventDetailCard(event, channel) {
      const detail = document.createElement('article');
      const hero = document.createElement('div');
      hero.className = 'epg-detail-hero';
      detail.appendChild(hero);
      return detail;
    }
  `, context);
  vm.runInContext(ownerSource, context, {filename: 'epg-detail-owner.js'});

  const persistentPath =
    '/api/epg/cache/artwork?backend=default&channelId=C-1-1051-10301&eventId=10580';
  const ownerDetail = window.VdrSuiteEpgDetailOwner.createCard(
    {
      id: '10580',
      channelId: 'C-1-1051-10301',
      title: 'ZDF-Morgenmagazin',
      artwork: {available: true, url: persistentPath}
    },
    {id: 'C-1-1051-10301'}
  );
  const ownerArtwork = ownerDetail.querySelector('.epg-detail-artwork');
  assert.ok(ownerArtwork);
  assert.strictEqual(
    ownerArtwork.style.backgroundImage,
    'url("/vdr-suite' + persistentPath + '")'
  );
  assert.ok(resolvedPublicPaths.includes(persistentPath));

  assert.ok(source.includes("addTab('epg', 'EPG', false)"));
  assert.ok(source.includes("addTab('scraper', 'Scraper', true)"));
  assert.ok(source.includes("addTab('cast', 'Besetzung', true)"));
  assert.ok(source.includes("addTab('images', 'Bilder', true)"));
  assert.ok(source.includes("'/api/epg/cache/metadata'"));
  assert.ok(source.includes('resolvePublicImageUrl'));
  assert.ok(!source.includes("if (detail.querySelector('.epg-detail-artwork')) return;"));
  assert.ok(ownerSource.includes('resolvePublicUrl'));
  assert.ok(source.includes('fetchClientRecordingPersons'));
  assert.ok(!source.includes('/var/cache/vdr/plugins/tvscraper/'));

  console.log('test_epg_metadata_detail passed');
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
