'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');
const logoSource = fs.readFileSync(path.join(frontendRoot, 'channel-logos.js'), 'utf8');
const heroSource = fs.readFileSync(path.join(frontendRoot, 'home-live-hero.js'), 'utf8');

assert(indexSource.includes('class="media-home-only media-home-hero"'));
assert(indexSource.includes('data-home-zone="hero"'));
assert(indexSource.includes('<script src="../frontend/channel-logos.js"></script>'));
assert(indexSource.includes('<script src="../frontend/home-live-hero.js"></script>'));
assert(logoSource.includes('createChannelLogoElement'));
assert(heroSource.includes('VdrSuiteHomeLiveHero'));
assert(heroSource.includes('fetchClientChannels'));
assert(heroSource.includes('fetchClientEpgCacheWindow'));
assert(heroSource.includes('VdrSuiteLiveTvView'));
assert(heroSource.includes("event.key === 'ArrowLeft'"));
assert(heroSource.includes("event.key === 'ArrowRight'"));
assert(heroSource.includes("root.addEventListener('touchstart'"));
assert(heroSource.includes("root.addEventListener('touchend'"));
assert(heroSource.includes('@media(max-width:46rem)'));
assert(!heroSource.includes('/api/media/sessions'));
assert(!heroSource.includes('createLivePanel('));
assert(!heroSource.includes('<video'));

function createClassList(initial) {
  const values = new Set(String(initial || '').split(/\s+/).filter(Boolean));
  return {
    add(...names) { names.forEach(name => values.add(name)); },
    remove(...names) { names.forEach(name => values.delete(name)); },
    contains(name) { return values.has(name); },
    toggle(name, enabled) {
      if (enabled === undefined) enabled = !values.has(name);
      if (enabled) values.add(name); else values.delete(name);
      return enabled;
    }
  };
}

function createNode(tagName) {
  const listeners = Object.create(null);
  const attributes = Object.create(null);
  return {
    tagName: String(tagName || '').toUpperCase(),
    children: [],
    dataset: {},
    style: {},
    className: '',
    classList: createClassList(),
    textContent: '',
    disabled: false,
    hidden: false,
    parentNode: null,
    appendChild(child) {
      child.parentNode = this;
      this.children.push(child);
      return child;
    },
    replaceChildren(...children) {
      this.children = [];
      children.forEach(child => this.appendChild(child));
    },
    setAttribute(name, value) {
      attributes[name] = String(value);
      if (name === 'class') {
        this.className = String(value);
        this.classList = createClassList(value);
      }
      if (name === 'tabindex') this.tabIndex = Number(value);
    },
    getAttribute(name) { return attributes[name] || null; },
    hasAttribute(name) { return Object.prototype.hasOwnProperty.call(attributes, name); },
    addEventListener(type, listener) {
      (listeners[type] ||= []).push(listener);
    },
    dispatch(type, event = {}) {
      (listeners[type] || []).forEach(listener => listener(Object.assign({
        target: this,
        currentTarget: this,
        preventDefault() { this.defaultPrevented = true; }
      }, event)));
    },
    click() { this.dispatch('click'); },
    focus() { this.focused = true; },
    scrollIntoView() { this.scrolled = true; },
    remove() { this.removed = true; }
  };
}

function flattenText(node) {
  return [node.textContent].concat((node.children || []).map(flattenText)).join(' ');
}

function findButton(node, label) {
  if (node.tagName === 'BUTTON' && node.textContent === label) return node;
  for (const child of node.children || []) {
    const found = findButton(child, label);
    if (found) return found;
  }
  return null;
}

function findByClass(node, className) {
  if (String(node.className || '').split(/\s+/).includes(className)) return node;
  for (const child of node.children || []) {
    const found = findByClass(child, className);
    if (found) return found;
  }
  return null;
}

const heroRoot = createNode('section');
heroRoot.dataset.homeZone = 'hero';
const detailData = createNode('section');
const moduleNav = createNode('nav');
const backends = createNode('section');
const overviewTab = createNode('button');
overviewTab.classList.add('module-tab', 'active');
overviewTab.dataset.module = 'overview';

const liveEntry = createNode('article');
liveEntry.dataset.brandModule = 'livetv';
let liveNavigationCount = 0;
liveEntry.addEventListener('click', () => { liveNavigationCount += 1; });

const epgEntry = createNode('article');
epgEntry.dataset.brandModule = 'epg';
let epgNavigationCount = 0;
epgEntry.addEventListener('click', () => { epgNavigationCount += 1; });

const head = createNode('head');
const documentListeners = Object.create(null);
const document = {
  readyState: 'loading',
  head,
  createElement: createNode,
  querySelector(selector) {
    if (selector === '.media-home-hero[data-home-zone="hero"]') return heroRoot;
    if (selector === '.module-tab.active[data-module="overview"]') return overviewTab;
    if (selector === '.module-tab.active') return overviewTab;
    if (selector === '[data-brand-module="livetv"]') return liveEntry;
    if (selector === '[data-brand-module="channels2"]') return null;
    if (selector === '[data-brand-module="epg"]') return epgEntry;
    return null;
  },
  getElementById(id) {
    if (id === 'detail-data') return detailData;
    if (id === 'module-nav') return moduleNav;
    if (id === 'backends') return backends;
    if (id === 'vdr-suite-home-live-hero-style') {
      return head.children.find(child => child.id === id) || null;
    }
    return null;
  },
  addEventListener(type, listener) {
    (documentListeners[type] ||= []).push(listener);
  }
};

let channelFetchCount = 0;
let epgFetchCount = 0;
let sessionRequestCount = 0;
let liveStartCount = 0;
const now = Math.floor(Date.now() / 1000);
const channelResponse = {
  channels: [
    {id: 'C20', name: 'Zwanzig', number: 20, radio: false, enabled: true},
    {id: 'R1', name: 'Radio', number: 1, radio: true, enabled: true},
    {id: 'C2', name: 'Zwei', number: 2, radio: false, enabled: true},
    {id: 'C1', name: 'Eins', number: 1, radio: false, enabled: true}
  ]
};
const events = [];
[
  ['C1', 'Heute Eins'],
  ['C2', 'Heute Zwei'],
  ['C20', 'Heute Zwanzig']
].forEach(([channelId, title]) => {
  events.push({channelId, title, startTime: now - 600, endTime: now + 1200});
  events.push({channelId, title: 'Danach ' + title, startTime: now + 1200, endTime: now + 3000});
});

const clientApi = {
  fetchClientChannels(options) {
    channelFetchCount += 1;
    assert.strictEqual(options.query.backend, 'backend-a');
    return Promise.resolve(channelResponse);
  },
  fetchClientEpgCacheWindow(options) {
    epgFetchCount += 1;
    assert.strictEqual(options.query.backend, 'backend-a');
    assert.strictEqual(options.query.channelIds, 'C1,C2,C20');
    return Promise.resolve({events});
  },
  requestJson(requestPath) {
    if (String(requestPath).includes('/api/media/sessions')) sessionRequestCount += 1;
    return Promise.reject(new Error('unexpected request'));
  }
};

const window = {
  document,
  console,
  setTimeout,
  VdrSuitePlatform: {
    getClientApi() { return clientApi; },
    getSelectedBackendId() { return 'backend-a'; },
    getSelectedModule() { return 'overview'; }
  },
  VdrSuitePublicUrl: {
    resolvePath(value) { return '/vdr-suite' + value; }
  },
  VdrSuiteLiveTvView: {
    startChannel(channel) {
      liveStartCount += 1;
      window.lastStartedChannelId = channel.id;
      return Promise.resolve('live-session-owned-by-existing-runtime');
    }
  }
};

const context = vm.createContext({
  window,
  document,
  console,
  setTimeout,
  Date,
  Math,
  Number,
  String,
  Boolean,
  Object,
  Array,
  Promise,
  Error,
  RegExp,
  encodeURIComponent,
  addText(element, value) {
    element.textContent = String(value);
    return element;
  }
});

vm.runInContext(logoSource, context, {filename: 'web/frontend/channel-logos.js'});
vm.runInContext(heroSource, context, {filename: 'web/frontend/home-live-hero.js'});
assert.ok(window.VdrSuiteHomeLiveHero);

(async function () {
  const hero = window.VdrSuiteHomeLiveHero;
  await hero.refresh();

  assert.strictEqual(channelFetchCount, 1);
  assert.strictEqual(epgFetchCount, 1);
  assert.strictEqual(liveStartCount, 0);
  assert.strictEqual(sessionRequestCount, 0);
  assert.strictEqual(hero.snapshot().channelCount, 3);
  assert.strictEqual(hero.snapshot().selectedChannelId, 'C1');
  assert.strictEqual(hero.snapshot().currentEventTitle, 'Heute Eins');
  assert.strictEqual(hero.snapshot().nextEventTitle, 'Danach Heute Eins');
  assert(flattenText(heroRoot).includes('Eins'));
  assert(findByClass(heroRoot, 'media-home-live-artwork'));

  const dataRequestBaseline = channelFetchCount + epgFetchCount;

  // Rapid browsing is presentation state only: no data reload, no session work.
  heroRoot.dispatch('keydown', {key: 'ArrowRight'});
  assert.strictEqual(hero.snapshot().selectedChannelId, 'C2');
  assert.strictEqual(hero.snapshot().currentEventTitle, 'Heute Zwei');
  assert.strictEqual(liveStartCount, 0);
  assert.strictEqual(sessionRequestCount, 0);
  assert.strictEqual(channelFetchCount + epgFetchCount, dataRequestBaseline);

  heroRoot.dispatch('touchstart', {touches: [{clientX: 210, clientY: 30}]});
  heroRoot.dispatch('touchend', {changedTouches: [{clientX: 105, clientY: 34}]});
  assert.strictEqual(hero.snapshot().selectedChannelId, 'C20');
  assert.strictEqual(hero.snapshot().currentEventTitle, 'Heute Zwanzig');
  assert.strictEqual(liveStartCount, 0);
  assert.strictEqual(sessionRequestCount, 0);
  assert.strictEqual(channelFetchCount + epgFetchCount, dataRequestBaseline);

  // Rendering is viewport-independent; CSS recomposes the same focused truth.
  const selectedBeforeRerender = hero.snapshot().selectedChannelId;
  hero.__test.render();
  assert.strictEqual(hero.snapshot().selectedChannelId, selectedBeforeRerender);
  assert(findButton(heroRoot, 'Live ansehen'));
  assert(findButton(heroRoot, 'EPG'));

  // Only explicit Watch Live delegates to the existing canonical Live-TV owner.
  findButton(heroRoot, 'Live ansehen').click();
  assert.strictEqual(liveNavigationCount, 1);
  assert.strictEqual(liveStartCount, 1);
  assert.strictEqual(window.lastStartedChannelId, 'C20');

  // EPG remains the existing owning navigation flow.
  findButton(heroRoot, 'EPG').click();
  assert.strictEqual(epgNavigationCount, 1);
  assert.strictEqual(sessionRequestCount, 0);

  console.log('phase66 live tv hero browse-only production contract ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
