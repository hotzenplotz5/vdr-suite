'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-searchtimer-actions.js'),
  'utf8'
);

const appSource = fs.readFileSync(path.resolve(__dirname, '..', 'app.js'), 'utf8');
function functionSlice(name, nextName) {
  const start = appSource.indexOf('function ' + name + '(');
  const end = appSource.indexOf('function ' + nextName + '(', start + 1);
  assert(start >= 0 && end > start, 'production function ' + name);
  return appSource.slice(start, end);
}
const pageSource = functionSlice('epgChannelPageOffset', 'loadEpgTimeline');
const detailSource = functionSlice('epgDetailChannelName', 'formatEpgTimerDay');

class MockStyle {
  constructor() {
    this.values = {};
    this.backgroundImage = '';
  }

  setProperty(name, value) {
    this.values[name] = String(value);
  }
}

class MockElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.parentNode = null;
    this.className = '';
    this.dataset = {};
    this.attributes = {};
    this.style = new MockStyle();
    this.textContent = '';
    this.disabled = false;
    this.hidden = false;
    this.value = '';
    this.listeners = {};
    this.id = '';
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
    if (child && typeof child === 'object') child.parentNode = this;
    return child;
  }

  replaceChildren(...children) {
    this.children = [];
    children.forEach(child => this.appendChild(child));
  }

  insertBefore(child, reference) {
    const index = this.children.indexOf(reference);
    if (index < 0) return this.appendChild(child);
    this.children.splice(index, 0, child);
    child.parentNode = this;
    return child;
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }

  addEventListener(name, listener) {
    if (!this.listeners[name]) this.listeners[name] = [];
    this.listeners[name].push(listener);
  }

  dispatch(name) {
    (this.listeners[name] || []).forEach(listener => listener({currentTarget: this, target: this}));
  }

  querySelector(selector) {
    return this.querySelectorAll(selector)[0] || null;
  }

  querySelectorAll(selector) {
    const results = [];
    const matches = element => {
      if (!(element instanceof MockElement)) return false;
      if (selector.startsWith('.')) return element.classList.contains(selector.slice(1));
      if (selector === 'button') return element.tagName === 'BUTTON';
      if (selector.startsWith('[') && selector.endsWith(']')) {
        const content = selector.slice(1, -1);
        const [rawName, rawValue] = content.split('=');
        const name = rawName
          .replace(/^data-/, '')
          .replace(/-([a-z])/g, (_, character) => character.toUpperCase());
        if (rawValue === undefined) {
          return Object.prototype.hasOwnProperty.call(element.dataset, name);
        }
        return String(element.dataset[name]) === rawValue.replace(/^"|"$/g, '');
      }
      return false;
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

  closest() {
    return null;
  }
}

const document = {
  documentElement: new MockElement('html'),
  head: new MockElement('head'),
  listeners: {},
  createElement(tagName) {
    return new MockElement(tagName);
  },
  getElementById(id) {
    return this.head.children.find(child => child.id === id) || null;
  },
  querySelector() {
    return null;
  },
  addEventListener(name, listener) {
    this.listeners[name] = listener;
  }
};

const window = {
  VdrSuitePlatform: {
    getSelectedBackendId() {
      return 'default';
    }
  },
  VdrSuiteClientApi: null,
  setTimeout(callback) {
    callback();
    return 1;
  },
  alert() {}
};

const context = vm.createContext({
  Array,
  Boolean,
  Date,
  Error,
  Event: function Event() {},
  Map,
  Math,
  Number,
  Object,
  Promise,
  Set,
  String,
  console,
  document,
  window
});

vm.runInContext(`
  let epgChannelOffset = 0;
  let selectedEpgDetail = null;
  const EPG_VISIBLE_CHANNEL_LIMIT = 15;
  const detailDataElement = document.createElement('section');
  let loadCount = 0;
  let lastRenderedChannels = [];

  function firstValue(object, keys, fallback) {
    for (const key of keys) if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') return object[key];
    return fallback;
  }
  function listFromResponse(data, key) {
    return Array.isArray(data) ? data :
      data && Array.isArray(data[key]) ? data[key] :
      data && Array.isArray(data.items) ? data.items : [];
  }
  function frontendChannelId(channel) { return String(firstValue(channel, ['id','channelId','nativeId'], '')).trim(); }
  function frontendEventChannelId(event) { return String(firstValue(event, ['channelId','channel','channel_id'], '')).trim(); }
  function addText(element, text) { element.textContent = String(text); return element; }
  let epgLoadedBackendId = '';
  let currentChannels = null;
  const catalogRequests = [];
  function selectedEpgBackendId() { return 'default'; }
  function frontendPlatformClientApi() {
    return {fetchClientChannels(options) {
      catalogRequests.push([options.query.backend, options.compactCatalog]);
      return Promise.resolve({channels:[{backendId:options.query.backend, channelId:'C-1', channelNumber:1, name:'Das Erste HD'}]});
    }};
  }
  function createChannelLogoElement(title, channelId) {
    const logo = document.createElement('div');
    logo.dataset.title = title;
    logo.dataset.channelId = channelId;
    return logo;
  }

  ${pageSource}
  ${detailSource}

  function createEpgEventDetailCard() {
    const detail = document.createElement('article');
    detail.className = 'epg-event-detail';

    const hero = document.createElement('div');
    hero.className = 'epg-detail-hero';
    detail.appendChild(hero);

    const actions = document.createElement('div');
    const search = document.createElement('button');
    search.className = 'epg-detail-action';
    search.textContent = 'Suchtimer';
    search.disabled = true;
    actions.appendChild(search);
    detail.appendChild(actions);
    return detail;
  }

  function renderEpgTimeView(channelData) {
    // The production renderer and fetch both call the production page helper.
    epgChannelOffset = epgChannelPageOffset(channelData.channels.length, epgChannelOffset, EPG_VISIBLE_CHANNEL_LIMIT);
    lastRenderedChannels = channelData.channels.slice(epgChannelOffset, epgChannelOffset + EPG_VISIBLE_CHANNEL_LIMIT);
    detailDataElement.children = [];

    const intro = document.createElement('article');
    intro.className = 'epg-timeline-intro';

    const description = document.createElement('p');
    description.textContent = 'Zeige Kanäle.';
    intro.appendChild(description);

    const modes = document.createElement('div');
    modes.className = 'epg-view-toggle';
    intro.appendChild(modes);

    const pager = document.createElement('div');
    pager.className = 'epg-pager';
    const previous = document.createElement('button');
    const next = document.createElement('button');
    previous.disabled = epgChannelOffset <= 0;
    next.disabled = epgChannelOffset + EPG_VISIBLE_CHANNEL_LIMIT >= channelData.channels.length;
    previous.addEventListener('click', () => {
      epgChannelOffset = epgChannelPageOffset(channelData.channels.length, epgChannelOffset - EPG_VISIBLE_CHANNEL_LIMIT, EPG_VISIBLE_CHANNEL_LIMIT);
      loadEpgTimeline();
    });
    next.addEventListener('click', () => {
      epgChannelOffset = epgChannelPageOffset(channelData.channels.length, epgChannelOffset + EPG_VISIBLE_CHANNEL_LIMIT, EPG_VISIBLE_CHANNEL_LIMIT);
      loadEpgTimeline();
    });
    pager.appendChild(previous);
    pager.appendChild(next);
    intro.appendChild(pager);
    detailDataElement.appendChild(intro);
  }

  function loadEpgTimeline() {
    loadCount += 1;
  }
`, context);

vm.runInContext(source, context, {filename: 'epg-searchtimer-actions.js'});

assert.ok(window.VdrSuiteEpgSearchTimerActions);
assert.strictEqual(window.VdrSuiteEpgSearchTimerActions.timelineEnhancementsReady(), true);
assert.strictEqual(document.documentElement.dataset.epgTimelineEnhancements, 'true');
assert.strictEqual(document.documentElement.dataset.epgDetailArtwork, 'true');
assert.strictEqual(document.documentElement.dataset.epgVerticalQuarterScale, undefined);
assert.ok(document.getElementById('vdr-suite-epg-timeline-enhancements'));

const channels = [
  {id: 'a1', group: 'Öffentlich'},
  {id: 'a2', group: 'Öffentlich'},
  {id: 'b1', group: 'Private'},
  {id: 'c1', group: ''}
];

const visible = vm.runInContext(
  `visibleEpgChannelsFromData(${JSON.stringify({channels})})`,
  context
);
assert.deepStrictEqual(Array.from(visible, channel => channel.id), ['a1', 'a2', 'b1', 'c1']);

vm.runInContext(
  `renderEpgTimeView(${JSON.stringify({channels})}, {events: []})`,
  context
);
const renderedIds = vm.runInContext('lastRenderedChannels.map(channel => channel.id)', context);
assert.deepStrictEqual(Array.from(renderedIds), ['a1', 'a2', 'b1', 'c1']);

const detailData = vm.runInContext('detailDataElement', context);
const groupControl = detailData.querySelector('.epg-group-control');
assert.ok(groupControl);
const select = groupControl.querySelector('.epg-group-select');
assert.ok(select);
assert.strictEqual(select.value, '__all__');
assert.deepStrictEqual(
  select.children.map(option => option.textContent),
  ['Alle Kanäle (4)', 'Öffentlich (2)', 'Private (1)', 'Weitere Sender (1)']
);
assert.strictEqual(detailData.querySelector('.epg-pager').hidden, true);

select.value = 'Private';
select.dispatch('change');
assert.strictEqual(vm.runInContext('epgChannelOffset', context), 0);
assert.strictEqual(vm.runInContext('loadCount', context), 1);

// 32 channels: every page is reachable exactly once; the final page must
// not be clamped back to 17 (which previously overlapped earlier channels).
context.largeCatalog = {channels:Array.from({length:32}, (_, index) => ({
  id:'sender-' + String(index + 1), group:'Öffentlich'
}))};
select.value = '__all__';
select.dispatch('change');
function renderPage() {
  return vm.runInContext(`
    const batch = visibleEpgChannelsFromData(largeCatalog);
    renderEpgTimeView(largeCatalog, {events:[]});
    ({fetch: batch.map(channel=>channel.id), render:lastRenderedChannels.map(channel=>channel.id), offset:epgChannelOffset})
  `.replace('const batch', 'var batch'), context);
}
let page = renderPage();
assert.deepStrictEqual(Array.from(page.fetch), Array.from(page.render));
assert.strictEqual(page.offset, 0);
assert.strictEqual(page.fetch.length, 15);
let pager = detailData.querySelector('.epg-pager');
assert.strictEqual(pager.hidden, false);
assert.strictEqual(pager.children[0].disabled, true);
assert.strictEqual(pager.children[1].disabled, false);
pager.children[1].dispatch('click');
page = renderPage();
assert.strictEqual(page.offset, 15);
assert.strictEqual(page.fetch[0], 'sender-16');
assert.deepStrictEqual(Array.from(page.fetch), Array.from(page.render));
detailData.querySelector('.epg-pager').children[1].dispatch('click');
page = renderPage();
assert.strictEqual(page.offset,30);
assert.deepStrictEqual(Array.from(page.fetch), ['sender-31','sender-32']);
assert.deepStrictEqual(Array.from(page.fetch), Array.from(page.render));
pager = detailData.querySelector('.epg-pager');
assert.strictEqual(pager.children[1].disabled,true);
pager.children[0].dispatch('click');
assert.strictEqual(renderPage().offset,15);
assert.strictEqual(vm.runInContext('epgChannelPageOffset(16, 15, 15)',context),15);
assert.strictEqual(vm.runInContext('epgChannelPageOffset(0, 30, 15)',context),0);
assert.strictEqual(vm.runInContext('epgChannelPageOffset(32, -15, 15)',context),0);
assert(appSource.includes('epgChannelOffset = epgChannelPageOffset(channels.length, epgChannelOffset, limit);'));

// A search/genre event may contain only native ID. Resolve actual sender
// names without using the ID as visible title, and retain ID for logo/actions.
assert.strictEqual(vm.runInContext("epgDetailChannelName({channelId:'C-1'},{id:'C-1',name:'C-1'},{channels:[{id:'C-1',name:'Das Erste HD'}]})",context),'Das Erste HD');
assert.strictEqual(vm.runInContext("epgDetailChannelName({channelId:'C-1',channelName:'ZDF HD'},{id:'C-1'},null)",context),'ZDF HD');
assert.strictEqual(vm.runInContext("epgDetailChannelName({channelId:'C-1'},{id:'other',name:'Wrong'},null)",context),'');
const detailMeta = vm.runInContext(`
  const meta = document.createElement('div');
  appendEpgDetailChannelMeta(meta, {channelId:'C-1',backendId:'mld'}, {id:'C-1',name:'C-1'});
  meta
`,context);
const channelBrand = detailMeta.querySelector('.epg-detail-channel-brand');
assert.strictEqual(channelBrand.children[0].textContent,'Sendername nicht verfügbar');
assert.strictEqual(JSON.stringify(vm.runInContext('catalogRequests',context)), '[ ["mld",true] ]'.replace(/ /g,''));
assert.ok(/function loadChannelSorter\(\)[\s\S]*?compactCatalog: true/.test(appSource));

const publicArtworkUrl = '/api/epg/cache/artwork?backend=default&channelId=C-1&eventId=17';
const detail = vm.runInContext(
  `createEpgEventDetailCard({title:'Film',channelId:'C-1',artwork:{available:true,url:'${publicArtworkUrl}'}},{id:'C-1',group:'Öffentlich'})`,
  context
);
assert.ok(detail.classList.contains('epg-has-artwork'));
const artwork = detail.querySelector('.epg-detail-artwork');
assert.ok(artwork);
assert.strictEqual(artwork.style.backgroundImage, `url("${publicArtworkUrl}")`);
assert.strictEqual(detail.children[0], artwork);
assert.strictEqual(detail.children[1].className, 'epg-detail-hero');
const searchTimerButton = detail.querySelectorAll('.epg-detail-action')[0];
assert.strictEqual(searchTimerButton.disabled, false);
assert.strictEqual(searchTimerButton.textContent, 'Suchtimer erstellen');

const unavailableDetail = vm.runInContext(
  `createEpgEventDetailCard({title:'Ohne Bild',channelId:'C-1',artwork:{available:false,url:'${publicArtworkUrl}'}},{id:'C-1',group:'Öffentlich'})`,
  context
);
assert.ok(!unavailableDetail.classList.contains('epg-has-artwork'));
assert.strictEqual(unavailableDetail.querySelector('.epg-detail-artwork'), null);

assert.ok(source.includes('decorateTimelineDetailArtwork'));
assert.ok(source.includes('artwork.available === true'));
assert.ok(source.includes("button.textContent = 'Suchtimer erstellen'"));
assert.ok(!source.includes('createEpgEventCard = function'));
assert.ok(!source.includes('createEpgProgramEventButton = function'));
assert.ok(!source.includes('appendEpgVerticalTimelineTicks = function'));
assert.ok(!source.includes('TIMELINE_QUARTER_SECONDS'));
assert.ok(!source.includes('MutationObserver'));

// Exercise the real production EPG renderer, not only the lightweight
// filtered-channel test double above. Both controls must drive one offset.
const actualRenderSource = functionSlice('renderEpgTimeView', 'renderEpgTimelineModePlaceholder');
const actualRenderContext = vm.createContext({document, Date, Math, Map});
vm.runInContext(`
  const detailDataElement = document.createElement('section');
  const EPG_VISIBLE_CHANNEL_LIMIT = 15;
  const epgWarmCacheStatus = '';
  let epgChannelOffset = 0;
  let selectedEpgDetail = {id:'previous-selection'};
  let epgProgramView = 'live';
  let epgTimeAxisMode = 'horizontal';
  let epgTimelineMode = 'time';
  let epgTimeWindowPageOffset = 0;
  let currentChannels = null;
  let currentEvents = null;
  let loadCount = 0;
  function epgIsMobileViewport() { return false; }
  function epgTimeAxisViewIsSelected() { return false; }
  function isMobileEpgLayout() { return false; }
  function listFromResponse(data, key) { return data[key] || []; }
  function addText(element, value) { element.textContent = String(value); return element; }
  function epgChannelPageOffset(size, offset, pageSize) {
    return Math.max(0, Math.min(Math.floor(Math.max(0, offset) / pageSize) * pageSize,
      Math.max(0, Math.ceil(size / pageSize) - 1) * pageSize));
  }
  function epgTimelineDisplayBounds(now) { return {start:now, end:now + 3600}; }
  function epgTimelineCurrentWindowEvents() { return []; }
  function buildEpgVisibleEventIndex() { return new Map(); }
  function formatEpgClockFromEpoch() { return '12:00'; }
  function loadEpgTimeline() { loadCount += 1; }
  function renderEpgWorkbench(list) { detailDataElement.appendChild(list); }
  function createEpgProgramViewGrid() {
    const grid = document.createElement('div'); grid.className = 'epg-program-grid'; return grid;
  }
  function createEpgVerticalTimeGrid() {
    const grid = document.createElement('div'); grid.className = 'epg-vertical-time-grid'; return grid;
  }
  function epgChannelTitle() { return 'Sender'; }
  function firstValue(object, keys, fallback) {
    for (const key of keys) if (object[key] !== undefined) return object[key];
    return fallback;
  }
  function epgIndexedEventsForChannel() { return []; }
  function appendEpgTimelineTicks() {}
  function appendEpgNowLine() {}
  function enableEpgDragPan() {}
`, actualRenderContext);
vm.runInContext(actualRenderSource, actualRenderContext);
actualRenderContext.catalog = {channels:Array.from({length:32}, (_, index) => ({
  id:'sender-' + String(index + 1)
}))};
const actualRender = () => vm.runInContext('renderEpgTimeView(catalog, {events:[]})', actualRenderContext);
const actualPagers = () => vm.runInContext('detailDataElement.querySelectorAll(".epg-pager")', actualRenderContext);

for (const view of ['live', 'vertical', 'horizontal']) {
  vm.runInContext(`epgProgramView='${view}'; epgTimeAxisMode='horizontal'; epgChannelOffset=0;`, actualRenderContext);
  actualRender();
  const rows = actualPagers();
  assert.strictEqual(rows.length, 2, 'top and bottom navigation in ' + view);
  assert.strictEqual(rows[1].classList.contains('epg-pager-bottom'), true);
  assert.deepStrictEqual(rows.map(row => row.children.map(button => button.disabled)), [[true,false],[true,false]]);
}
vm.runInContext("epgProgramView='live'; epgChannelOffset=0; selectedEpgDetail={id:'old'};", actualRenderContext);
actualRender();
actualPagers()[1].children[1].dispatch('click');
assert.strictEqual(vm.runInContext('epgChannelOffset', actualRenderContext), 15);
assert.strictEqual(vm.runInContext('selectedEpgDetail', actualRenderContext), null);
assert.strictEqual(vm.runInContext('loadCount', actualRenderContext), 1);
actualRender();
assert.deepStrictEqual(actualPagers().map(row => row.children.map(button => button.disabled)), [[false,false],[false,false]]);
actualPagers()[1].children[1].dispatch('click');
actualRender();
assert.strictEqual(vm.runInContext('epgChannelOffset', actualRenderContext), 30);
assert.deepStrictEqual(actualPagers().map(row => row.children.map(button => button.disabled)), [[false,true],[false,true]]);
actualPagers()[1].children[0].dispatch('click');
assert.strictEqual(vm.runInContext('epgChannelOffset', actualRenderContext), 15);
assert.strictEqual(vm.runInContext('loadCount', actualRenderContext), 3);
actualRenderContext.catalog = {channels:Array.from({length:15}, (_, i) => ({id:'s' + i}))};
vm.runInContext('epgChannelOffset=0;', actualRenderContext);
actualRender();
assert.strictEqual(actualPagers().length, 1, 'single-page catalogue has no redundant bottom pager');
assert.strictEqual(actualPagers()[0].hidden, true);

Promise.resolve().then(() => {
  assert.strictEqual(channelBrand.children[0].dataset.channelId,'C-1');
  assert.strictEqual(channelBrand.children[0].dataset.title,'Das Erste HD');
  assert.strictEqual(channelBrand.children[1].textContent,'Das Erste HD');
  console.log('test_epg_timeline_enhancements passed');
}).catch(error => { console.error(error); process.exitCode = 1; });
