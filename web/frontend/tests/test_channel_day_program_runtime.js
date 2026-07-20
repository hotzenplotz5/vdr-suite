'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.resolve(__dirname, '..', 'channel-day-program.js');
const source = fs.readFileSync(sourcePath, 'utf8');
const testHookMarker = 'global.VdrSuiteChannels2=moduleApi;';
const instrumentedSource = source.replace(
  testHookMarker,
  'global.__VdrSuiteChannelDayProgramTest=Object.freeze({eventArtwork,renderEventDetail});' +
    testHookMarker
);

assert.notStrictEqual(instrumentedSource, source);

let registeredName = null;
let registeredApi = null;

class MockElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.attributes = {};
    this.className = '';
    this.dataset = {};
    this.disabled = false;
    this.onclick = null;
    this.style = {};
    this.textContent = '';
    this.type = '';
    this.value = '';

    this.classList = {
      add: (...names) => {
        const values = new Set(this.className.split(/\s+/).filter(Boolean));
        names.forEach((name) => values.add(name));
        this.className = Array.from(values).join(' ');
      },
      contains: (name) => this.className.split(/\s+/).filter(Boolean).includes(name),
      remove: (...names) => {
        const removed = new Set(names);
        this.className = this.className
          .split(/\s+/)
          .filter((name) => name && !removed.has(name))
          .join(' ');
      }
    };
  }

  append(...children) {
    this.children.push(...children);
  }

  appendChild(child) {
    this.children.push(child);
    return child;
  }

  prepend(child) {
    this.children.unshift(child);
  }

  querySelector(selector) {
    if (selector.startsWith('.')) {
      const className = selector.slice(1);
      return this.find((element) => element.classList.contains(className));
    }

    return null;
  }

  find(predicate) {
    for (const child of this.children) {
      if (!(child instanceof MockElement)) {
        continue;
      }

      if (predicate(child)) {
        return child;
      }

      const nested = child.find(predicate);
      if (nested) {
        return nested;
      }
    }

    return null;
  }

  replaceChildren(...children) {
    this.children = children;
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }
}

const window = {
  VdrSuitePlatform: {
    getSelectedBackendId() {
      return 'living-room';
    },
    getClientApi() {
      return null;
    },
    getMountTarget() {
      return null;
    },
    hasModule() {
      return false;
    },
    registerModule(name, api) {
      registeredName = name;
      registeredApi = api;
    }
  },
  setTimeout,
  clearTimeout
};

const document = {
  head: new MockElement('head'),
  createElement(tagName) {
    return new MockElement(tagName);
  },
  getElementById() {
    return null;
  },
  querySelector() {
    return null;
  }
};

vm.runInNewContext(instrumentedSource, {
  Array,
  Boolean,
  Date,
  Event: function Event() {},
  JSON,
  Map,
  Math,
  Number,
  Object,
  Promise,
  Set,
  String,
  document,
  window
}, {filename: sourcePath});

assert.strictEqual(registeredName, 'channels2');
assert.ok(registeredApi);
assert.strictEqual(window.VdrSuiteChannels2, registeredApi);
assert.strictEqual(typeof registeredApi.activate, 'function');
assert.strictEqual(typeof registeredApi.deactivate, 'function');
assert.strictEqual(typeof registeredApi.refresh, 'function');

assert.doesNotThrow(() => registeredApi.activate());
assert.doesNotThrow(() => registeredApi.refresh());
assert.doesNotThrow(() => registeredApi.deactivate());

const testApi = window.__VdrSuiteChannelDayProgramTest;
assert.ok(testApi);
assert.strictEqual(typeof testApi.eventArtwork, 'function');
assert.strictEqual(typeof testApi.renderEventDetail, 'function');

const publicArtworkUrl =
  '/api/epg/cache/artwork?backend=default&channelId=C-1-1079-10351&eventId=13483';

assert.strictEqual(
  testApi.eventArtwork({
    artwork: {
      available: true,
      url: publicArtworkUrl
    },
    imageUrl: '/legacy-should-not-win.jpg'
  }),
  publicArtworkUrl
);

assert.strictEqual(
  testApi.eventArtwork({
    artwork: {
      available: false,
      url: '/unavailable.jpg'
    },
    imageUrl: '/legacy-fallback.jpg'
  }),
  '/legacy-fallback.jpg'
);

assert.strictEqual(
  testApi.eventArtwork({
    artwork: {
      available: true,
      url: '   '
    },
    posterUrl: '/legacy-empty-url-fallback.jpg'
  }),
  '/legacy-empty-url-fallback.jpg'
);

const event = {
  title: 'Testsendung',
  description: 'Beschreibung',
  channelId: 'C-1-1079-10351',
  startTime: 1760000000,
  endTime: 1760003600,
  artwork: {
    available: true,
    url: publicArtworkUrl
  }
};
const channel = {
  id: 'C-1-1079-10351',
  name: 'Testsender'
};

const detail = testApi.renderEventDetail(event, channel);
assert.ok(detail.classList.contains('has-artwork'));

const artworkElement = detail.querySelector('.channels2-artwork');
assert.ok(artworkElement);
assert.strictEqual(
  artworkElement.style.backgroundImage,
  `url("${publicArtworkUrl}")`
);

const detailWithoutArtwork = testApi.renderEventDetail(
  Object.assign({}, event, {artwork: {available: false, url: publicArtworkUrl}}),
  channel
);
assert.ok(!detailWithoutArtwork.classList.contains('has-artwork'));
assert.strictEqual(
  detailWithoutArtwork.querySelector('.channels2-artwork').style.backgroundImage,
  undefined
);

assert.ok(!source.includes('fetch('));
assert.ok(source.includes("registerModule('channels2'"));
assert.ok(source.includes('fetchClientChannels'));
assert.ok(source.includes('fetchClientEpgCacheWindow'));
assert.ok(source.includes('fetchClientEpgChannelWindow'));
assert.ok(source.includes('fetchClientTimerCreateAction'));
assert.ok(source.includes('artwork.available===true'));
assert.ok(source.includes('text(artwork.url)'));
assert.ok(
  source.includes(
    '.channels2-detail.has-artwork{grid-template-columns:minmax(10rem,16rem) minmax(0,1fr)}'
  )
);
assert.ok(source.includes('@media(max-width:720px)'));
assert.ok(
  source.includes(
    '.channels2-detail.has-artwork{grid-template-columns:1fr}'
  )
);
assert.ok(source.includes('Channels 2'));

console.log('test_channel_day_program_runtime passed');
