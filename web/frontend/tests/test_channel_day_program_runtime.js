'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.resolve(__dirname, '..', 'channel-day-program.js');
const source = fs.readFileSync(sourcePath, 'utf8');
const testHookMarker = 'global.VdrSuiteChannels2 = moduleApi;';
const instrumentedSource = source.replace(
  testHookMarker,
  'global.__VdrSuiteChannelDayProgramTest=Object.freeze({' +
    'eventArtwork,resolvePublicArtworkUrl,renderEventDetail,visibleEventsForDay,booleanValue,channelIsRadio,' +
    'channelIsEncrypted,channelIsEnabled,channelMatchesFilters,' +
    'filterChannels,state});' + testHookMarker
);

assert.notStrictEqual(instrumentedSource, source);

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
    this.parentNode = null;
    this.classList = {
      add: (...names) => {
        const values = new Set(this.className.split(/\s+/).filter(Boolean));
        names.forEach(name => values.add(name));
        this.className = Array.from(values).join(' ');
      },
      contains: name => this.className.split(/\s+/).filter(Boolean).includes(name),
      remove: (...names) => {
        const removed = new Set(names);
        this.className = this.className.split(/\s+/).filter(name => name && !removed.has(name)).join(' ');
      }
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

  removeChild(child) {
    const index = this.children.indexOf(child);
    if (index >= 0) this.children.splice(index, 1);
    child.parentNode = null;
    return child;
  }

  prepend(child) {
    this.children.unshift(child);
    if (child && typeof child === 'object') child.parentNode = this;
  }

  querySelector(selector) {
    if (selector.startsWith('.')) {
      const className = selector.slice(1);
      return this.find(element => element.classList.contains(className));
    }
    return null;
  }

  find(predicate) {
    for (const child of this.children) {
      if (!(child instanceof MockElement)) continue;
      if (predicate(child)) return child;
      const nested = child.find(predicate);
      if (nested) return nested;
    }
    return null;
  }

  replaceChildren(...children) {
    this.children.forEach(child => {
      if (child && typeof child === 'object') child.parentNode = null;
    });
    this.children = [];
    children.forEach(child => this.appendChild(child));
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }
}

let metadataEnhancements = [];
let deferredLoads = 0;
const resolvedPublicPaths = [];
const window = {
  VdrSuitePlatform: {
    getSelectedBackendId() { return 'living-room'; },
    getClientApi() { return null; },
    getMountTarget() { return null; }
  },
  VdrSuitePublicUrl: {
    resolvePath(pathName) {
      const value = String(pathName || '');
      resolvedPublicPaths.push(value);
      return '/vdr-suite' + value;
    }
  },
  VdrSuiteEpgMetadataDetail: {
    enhance(detail, event, channel) {
      metadataEnhancements.push({detail, event, channel});
      detail.dataset.epgMetadataDetail = 'true';
      return detail;
    }
  },
  VdrSuiteDeferredFrontendRuntimes: {
    loadEpgDetail() {
      deferredLoads += 1;
      return Promise.resolve();
    }
  },
  setTimeout,
  clearTimeout
};

const document = {
  head: new MockElement('head'),
  createElement(tagName) { return new MockElement(tagName); },
  getElementById() { return null; },
  querySelector() { return null; }
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

async function run() {
  const moduleApi = window.VdrSuiteChannels2;
  assert.ok(moduleApi);
  assert.strictEqual(typeof moduleApi.activate, 'function');
  assert.strictEqual(typeof moduleApi.renderList, 'function');
  assert.strictEqual(typeof moduleApi.deactivate, 'function');
  assert.strictEqual(typeof moduleApi.refresh, 'function');
  assert.doesNotThrow(() => moduleApi.activate());
  assert.doesNotThrow(() => moduleApi.refresh());
  assert.doesNotThrow(() => moduleApi.deactivate());

  const testApi = window.__VdrSuiteChannelDayProgramTest;
  assert.ok(testApi);

  const localEpoch = (year, month, day, hour, minute) => Math.floor(
    new Date(year, month - 1, day, hour, minute).getTime() / 1000
  );
  const selectedToday = new Date(2026, 6, 23);
  const now = localEpoch(2026, 7, 23, 8, 13);
  const todayEvents = [
    {
      id: 'future',
      startTime: localEpoch(2026, 7, 23, 9, 0),
      endTime: localEpoch(2026, 7, 23, 10, 0)
    },
    {
      id: 'past',
      startTime: localEpoch(2026, 7, 23, 7, 0),
      endTime: localEpoch(2026, 7, 23, 8, 0)
    },
    {
      id: 'current',
      startTime: localEpoch(2026, 7, 23, 8, 0),
      endTime: localEpoch(2026, 7, 23, 9, 0)
    }
  ];
  assert.deepStrictEqual(
    Array.from(testApi.visibleEventsForDay(todayEvents, selectedToday, now), item => item.id),
    ['current', 'future']
  );

  const gapEvents = [
    {
      id: 'ended-before-gap',
      startTime: localEpoch(2026, 7, 23, 7, 0),
      endTime: localEpoch(2026, 7, 23, 8, 0)
    },
    {
      id: 'next-after-gap',
      startTime: localEpoch(2026, 7, 23, 8, 30),
      endTime: localEpoch(2026, 7, 23, 9, 0)
    }
  ];
  assert.deepStrictEqual(
    Array.from(testApi.visibleEventsForDay(gapEvents, selectedToday, now), item => item.id),
    ['next-after-gap']
  );

  const midnightNow = localEpoch(2026, 7, 23, 0, 5);
  const midnightEvents = [
    {
      id: 'ended-yesterday',
      startTime: localEpoch(2026, 7, 22, 23, 0),
      endTime: localEpoch(2026, 7, 22, 23, 55)
    },
    {
      id: 'crosses-midnight',
      startTime: localEpoch(2026, 7, 22, 23, 50),
      endTime: localEpoch(2026, 7, 23, 0, 20)
    },
    {
      id: 'after-midnight',
      startTime: localEpoch(2026, 7, 23, 0, 20),
      endTime: localEpoch(2026, 7, 23, 1, 0)
    }
  ];
  assert.deepStrictEqual(
    Array.from(
      testApi.visibleEventsForDay(midnightEvents, selectedToday, midnightNow),
      item => item.id
    ),
    ['crosses-midnight', 'after-midnight']
  );

  const pastDay = new Date(2026, 6, 22);
  assert.deepStrictEqual(
    Array.from(testApi.visibleEventsForDay(todayEvents, pastDay, now), item => item.id),
    ['past', 'current', 'future']
  );
  const futureDay = new Date(2026, 6, 24);
  assert.deepStrictEqual(
    Array.from(testApi.visibleEventsForDay(todayEvents, futureDay, now), item => item.id),
    ['past', 'current', 'future']
  );

  const publicArtworkUrl = '/api/epg/cache/artwork?backend=default&channelId=C-1-1079-10351&eventId=13483';
  const resolvedArtworkUrl = '/vdr-suite' + publicArtworkUrl;
  assert.strictEqual(
    testApi.eventArtwork({artwork: {available: true, url: publicArtworkUrl}, imageUrl: '/legacy.jpg'}),
    resolvedArtworkUrl
  );
  assert.strictEqual(testApi.resolvePublicArtworkUrl('/fallback.jpg'), '/fallback.jpg');
  assert.strictEqual(testApi.eventArtwork({artwork: {available: false, url: '/unavailable.jpg'}, imageUrl: '/fallback.jpg'}), '/fallback.jpg');
  assert.strictEqual(testApi.eventArtwork({artwork: {available: true, url: '   '}, posterUrl: '/empty-fallback.jpg'}), '/empty-fallback.jpg');
  assert.ok(resolvedPublicPaths.includes(publicArtworkUrl));

  const event = {
    id: '13483',
    title: 'Testsendung',
    description: 'Beschreibung',
    channelId: 'C-1-1079-10351',
    startTime: 1760000000,
    endTime: 1760003600,
    artwork: {available: true, url: publicArtworkUrl}
  };
  const channel = {id: 'C-1-1079-10351', name: 'Testsender'};
  const detail = testApi.renderEventDetail(event, channel);
  assert.ok(detail.classList.contains('has-artwork'));
  assert.strictEqual(detail.querySelector('.channels2-artwork').style.backgroundImage, `url("${resolvedArtworkUrl}")`);
  assert.ok(detail.querySelector('.channels2-artwork').classList.contains('epg-detail-artwork'));
  assert.strictEqual(detail.querySelector('.channels2-artwork').parentNode, detail);
  assert.ok(detail.querySelector('.channels2-detail-copy').classList.contains('epg-detail-hero'));
  assert.strictEqual(detail.querySelector('.channels2-detail-copy').parentNode, detail);
  assert.ok(detail.querySelector('.channels2-description').classList.contains('epg-detail-description'));
  assert.ok(detail.querySelector('.channels2-actions').classList.contains('epg-detail-actions'));
  assert.strictEqual(detail.querySelector('.channels2-actions').parentNode, detail);
  assert.strictEqual(detail.querySelector('.channels2-feedback').parentNode, detail);
  assert.strictEqual(metadataEnhancements.length, 1);
  assert.strictEqual(metadataEnhancements[0].detail, detail);
  assert.strictEqual(metadataEnhancements[0].event, event);
  assert.strictEqual(metadataEnhancements[0].channel, channel);
  assert.strictEqual(deferredLoads, 0);

  const detailWithoutArtwork = testApi.renderEventDetail(Object.assign({}, event, {artwork: {available: false, url: publicArtworkUrl}}), channel);
  assert.ok(!detailWithoutArtwork.classList.contains('has-artwork'));
  assert.strictEqual(detailWithoutArtwork.querySelector('.channels2-artwork'), null);
  assert.strictEqual(metadataEnhancements.length, 2);

  let resolveDeferred;
  window.VdrSuiteEpgMetadataDetail = null;
  window.VdrSuiteDeferredFrontendRuntimes = {
    loadEpgDetail() {
      deferredLoads += 1;
      return new Promise(resolve => { resolveDeferred = resolve; });
    }
  };
  const deferredDetail = testApi.renderEventDetail(Object.assign({}, event, {id: '13484'}), channel);
  const parent = new MockElement('div');
  parent.appendChild(deferredDetail);
  let deferredEnhancement = null;
  window.VdrSuiteEpgMetadataDetail = {
    enhance(currentDetail, currentEvent, currentChannel) {
      deferredEnhancement = {currentDetail, currentEvent, currentChannel};
      currentDetail.dataset.epgMetadataDetail = 'true';
    }
  };
  resolveDeferred();
  await Promise.resolve();
  await Promise.resolve();
  assert.ok(deferredEnhancement);
  assert.strictEqual(deferredEnhancement.currentDetail, deferredDetail);
  assert.strictEqual(deferredEnhancement.currentEvent.id, '13484');
  assert.strictEqual(deferredLoads, 1);

  window.VdrSuiteEpgMetadataDetail = null;
  window.VdrSuiteDeferredFrontendRuntimes = {
    loadEpgDetail() {
      return Promise.reject(new Error('Bundle fehlt'));
    }
  };
  const failedDetail = testApi.renderEventDetail(Object.assign({}, event, {id: '13485'}), channel);
  parent.appendChild(failedDetail);
  await Promise.resolve();
  await Promise.resolve();
  const failedFeedback = failedDetail.querySelector('.channels2-feedback');
  assert.ok(failedFeedback.classList.contains('error'));
  assert.ok(failedFeedback.textContent.includes('Bundle fehlt'));

  assert.strictEqual(testApi.booleanValue('false', true), false);
  assert.strictEqual(testApi.booleanValue('true', false), true);
  assert.strictEqual(testApi.channelIsRadio({radio: true}), true);
  assert.strictEqual(testApi.channelIsRadio({radio: 'false'}), false);
  assert.strictEqual(testApi.channelIsEncrypted({encrypted: true}), true);
  assert.strictEqual(testApi.channelIsEncrypted({caids: ['1702']}), true);
  assert.strictEqual(testApi.channelIsEnabled({active: false}), false);

  const channels = [
    {id: 'tv-free', name: 'Das Erste HD', number: 1, group: 'Öffentlich', radio: false, encrypted: false, enabled: true},
    {id: 'tv-pay', name: 'Pay TV', number: 2, group: 'Pay', radio: false, encrypted: true, enabled: true},
    {id: 'radio', name: 'Radio Eins', number: 3, group: 'Radio', radio: true, encrypted: false, enabled: true},
    {id: 'disabled', name: 'Alt TV', number: 4, group: 'Weitere', radio: false, encrypted: false, enabled: false}
  ];

  testApi.state.channels = channels;
  testApi.state.encryptionAvailable = true;
  testApi.state.query = '';
  testApi.state.typeFilter = 'tv';
  testApi.state.accessFilter = 'free';
  testApi.state.statusFilter = 'enabled';
  testApi.filterChannels();
  assert.deepStrictEqual(Array.from(testApi.state.visible, item => item.id), ['tv-free']);

  testApi.state.typeFilter = 'radio';
  testApi.state.accessFilter = 'all';
  testApi.filterChannels();
  assert.deepStrictEqual(Array.from(testApi.state.visible, item => item.id), ['radio']);

  testApi.state.typeFilter = 'all';
  testApi.state.statusFilter = 'disabled';
  testApi.filterChannels();
  assert.deepStrictEqual(Array.from(testApi.state.visible, item => item.id), ['disabled']);

  testApi.state.statusFilter = 'all';
  testApi.state.query = 'pay';
  testApi.filterChannels();
  assert.deepStrictEqual(Array.from(testApi.state.visible, item => item.id), ['tv-pay']);

  assert.ok(!source.includes('fetch('));
  assert.ok(source.includes('fetchClientChannels'));
  assert.ok(source.includes('fetchClientEpgCacheWindow'));
  assert.ok(source.includes('fetchClientEpgChannelWindow'));
  assert.ok(source.includes('fetchClientTimerCreateAction'));
  assert.ok(source.includes('visibleEventsForDay'));
  assert.ok(source.includes('resolvePublicArtworkUrl'));
  assert.ok(source.includes("[['all', 'Alle'], ['tv', 'TV'], ['radio', 'Radio']]"));
  assert.ok(source.includes("[['all', 'Alle'], ['free', 'Frei'], ['encrypted', 'Verschlüsselt']]"));
  assert.ok(source.includes("[['all', 'Alle'], ['enabled', 'Aktiv'], ['disabled', 'Deaktiviert']]"));
  assert.ok(source.includes("intro.append(addText(document.createElement('h3'), 'Kanäle')"));
  assert.ok(source.includes('adoptCanonicalChannelNavigation'));
  assert.ok(source.includes('artwork.available === true'));
  assert.ok(source.includes('VdrSuiteEpgMetadataDetail'));
  assert.ok(source.includes('loadEpgDetail'));
  assert.ok(source.includes('epg-detail-actions'));
  assert.ok(source.includes('@media(max-width:720px)'));
  assert.ok(source.includes('.channels2-detail.has-artwork,.channels2-detail.epg-has-artwork{grid-template-columns:1fr}'));

  console.log('test_channel_day_program_runtime passed');
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
