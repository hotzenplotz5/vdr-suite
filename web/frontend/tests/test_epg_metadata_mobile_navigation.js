'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-metadata-detail.js'),
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
    this.scrollCalls = [];
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

  append(...children) {
    children.forEach(child => this.appendChild(child));
  }

  appendChild(child) {
    this.children.push(child);
    if (child && typeof child === 'object') child.parentNode = this;
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
    this.children.forEach(child => {
      if (child) child.parentNode = null;
    });
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
    (this.listeners.click || []).forEach(listener => listener({
      target: this,
      currentTarget: this
    }));
  }

  scrollIntoView(options) {
    this.scrollCalls.push(options);
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
      return element.tagName === selector.toUpperCase();
    };
    const visit = element => {
      element.children.forEach(child => {
        if (!(child instanceof MockElement)) return;
        if (matches(child)) results.push(child);
        visit(child);
      });
    };
    visit(this);
    return results;
  }
}

const document = {
  head: new MockElement('head'),
  createElement(tagName) {
    return new MockElement(tagName);
  },
  getElementById(id) {
    return this.head.children.find(child => child.id === id) || null;
  }
};

let recordingSearch = null;
const metadata = {
  available: true,
  mediaType: 'movie',
  title: 'Testfilm',
  people: [
    {
      role: 'actor',
      name: 'Test Person',
      characterName: 'Rolle',
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
    }
  ]
};
const recording = {
  id: 'rec-1',
  backendId: 'default',
  title: 'Technischer Titel',
  path: 'Filme/Testfilm',
  startTime: '1780000000',
  durationSeconds: 5400,
  sizeMb: 4096,
  metadata: {
    presentation: {
      title: 'Testfilm',
      subtitle: 'Untertitel',
      summary: 'Beschreibung der Aufnahme.',
      posterUrl: '/recording-artwork/default/0123456789abcdef0123456789abcdef'
    },
    provider: {},
    artwork: {}
  }
};

const window = {
  requestAnimationFrame(callback) {
    callback();
  },
  VdrSuitePlatform: {
    getSelectedBackendId() {
      return 'default';
    }
  },
  VdrSuiteClientApi: {
    requestJson() {
      return Promise.resolve(metadata);
    },
    fetchClientRecordingPersons(options) {
      recordingSearch = options;
      return Promise.resolve({matches: [{recording: recording}]});
    }
  }
};

vm.runInNewContext(source, {
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
  document,
  encodeURIComponent,
  window
}, {filename: 'epg-metadata-detail.js'});

async function flush() {
  await Promise.resolve();
  await Promise.resolve();
}

async function run() {
  const detail = new MockElement('article');
  const hero = new MockElement('div');
  hero.className = 'epg-detail-hero';
  const meta = new MockElement('div');
  meta.className = 'epg-detail-meta-grid';
  const description = new MockElement('div');
  description.className = 'epg-detail-description';
  const actions = new MockElement('div');
  actions.className = 'epg-detail-actions';
  detail.append(hero, meta, description, actions);

  window.VdrSuiteEpgMetadataDetail.enhance(
    detail,
    {id: '100', channelId: 'C-1', title: 'Testfilm'},
    {id: 'C-1'}
  );
  await flush();

  const style = document.head.children.find(
    child => child.id === 'vdr-suite-epg-metadata-detail-style'
  );
  assert.ok(style.textContent.includes(
    '.epg-person-recording-details[hidden]{display:none!important}'
  ));
  assert.ok(style.textContent.includes('white-space:nowrap'));

  const tabs = detail.querySelectorAll('.epg-metadata-tab');
  const panels = detail.querySelectorAll('.epg-metadata-panel');
  assert.strictEqual(tabs.length, 4);
  assert.strictEqual(panels.length, 3);

  tabs[2].click();
  assert.strictEqual(panels[0].hidden, true);
  assert.strictEqual(panels[1].hidden, false);
  assert.strictEqual(panels[2].hidden, true);
  assert.strictEqual(panels[1].scrollCalls.length, 1);

  const personEntry = panels[1].querySelector('.epg-person-entry');
  const personCard = personEntry.querySelector('.epg-person-card');
  const result = personEntry.querySelector('.epg-person-search-result');
  assert.strictEqual(personEntry.children[0], personCard);
  assert.strictEqual(personEntry.children[1], result);
  assert.strictEqual(result.hidden, true);

  personCard.click();
  await flush();
  assert.strictEqual(result.hidden, false);
  assert.ok(result.scrollCalls.length >= 1);
  assert.strictEqual(personCard.attributes['aria-expanded'], 'true');
  assert.ok(recordingSearch);
  assert.strictEqual(recordingSearch.query.name, 'Test Person');

  const recordingCard = result.querySelector('.epg-person-recording-card');
  const posterImage = result.querySelector('.epg-person-recording-poster').querySelector('img');
  const recordingDetails = result.querySelector('.epg-person-recording-details');
  assert.ok(recordingCard);
  assert.strictEqual(posterImage.src, recording.metadata.presentation.posterUrl);
  assert.strictEqual(recordingDetails.hidden, true);

  recordingCard.click();
  assert.strictEqual(recordingDetails.hidden, false);
  assert.strictEqual(recordingCard.attributes['aria-expanded'], 'true');

  tabs[3].click();
  assert.strictEqual(panels[1].hidden, true);
  assert.strictEqual(panels[2].hidden, false);
  assert.strictEqual(panels[2].scrollCalls.length, 1);

  console.log('test_epg_metadata_mobile_navigation passed');
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
