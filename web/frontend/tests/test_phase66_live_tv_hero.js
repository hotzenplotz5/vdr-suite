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
assert(heroSource.includes("root.setAttribute('tabindex', '0')"));
assert(heroSource.includes('focusHeroRoot(root)'));
assert(heroSource.includes("root.addEventListener('touchstart'"));
assert(heroSource.includes("root.addEventListener('touchend'"));
assert(heroSource.includes('@media(max-width:46rem)'));
assert(heroSource.includes('const PROGRAMME_RAIL_LIMIT = 24;'));
assert(heroSource.includes("renderProgrammeRail('now', 'Was läuft jetzt', true);"));
assert(heroSource.includes("renderProgrammeRail('next', 'Was läuft danach', false);"));
assert(heroSource.includes('.media-home-live-guide-now{order:10}.media-home-live-guide-next{order:20}.media-home-continue-watching{order:30}'));
assert(heroSource.includes('.media-home-live-guide-rail{display:grid;grid-auto-flow:column;grid-auto-columns:minmax(11rem,15rem)'));
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
    focused: false,
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
    querySelector(selector) {
      const dataMatch = String(selector || '').match(/^\[data-home-live-guide="([^"]+)"\]$/);
      if (dataMatch && this.dataset.homeLiveGuide === dataMatch[1]) return this;
      for (const child of this.children || []) {
        if (child && typeof child.querySelector === 'function') {
          const found = child.querySelector(selector);
          if (found) return found;
        }
      }
      return null;
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
    remove() {
      if (this.parentNode) {
        const index = this.parentNode.children.indexOf(this);
        if (index >= 0) this.parentNode.children.splice(index, 1);
      }
      this.parentNode = null;
      this.removed = true;
    }
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
const additionalSections = createNode('section');
additionalSections.dataset.homeZone = 'additional-sections';
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
    if (selector === '[data-home-zone="additional-sections"]') return additionalSections;
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
for (let number = 21; number <= 48; number += 1) {
  channelResponse.channels.push({id: 'C' + String(number), name: 'Kanal ' + String(number), number, radio: false, enabled: true});
}
const events = [];
[
  ['C1', 'Heute Eins'],
  ['C2', 'Heute Zwei'],
  ['C20', 'Heute Zwanzig']
].forEach(([channelId, title]) => {
  events.push({channelId, title, startTime: now - 600, endTime: now + 1200});
  events.push({channelId, title: 'Danach ' + title, startTime: now + 1200, endTime: now + 3000});
});
for (let number = 21; number <= 48; number += 1) {
  const channelId = 'C' + String(number);
  events.push({channelId, title: 'Heute ' + String(number), startTime: now - 600, endTime: now + 1200});
  events.push({channelId, title: 'Danach ' + String(number), startTime: now + 1200, endTime: now + 3000});
}
const epgChannelRequests = [];

const clientApi = {
  fetchClientChannels(options) {
    channelFetchCount += 1;
    assert.strictEqual(options.query.backend, 'backend-a');
    return Promise.resolve(channelResponse);
  },
  fetchClientEpgCacheWindow(options) {
    epgFetchCount += 1;
    assert.strictEqual(options.query.backend, 'backend-a');
    const requested = String(options.query.channelIds || '').split(',').filter(Boolean);
    epgChannelRequests.push(requested);
    return Promise.resolve({events: events.filter(event => requested.includes(event.channelId))});
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
  assert.strictEqual(epgChannelRequests.length, 1);
  assert.strictEqual(epgChannelRequests[0].length, 24);
  assert.strictEqual(liveStartCount, 0);
  assert.strictEqual(sessionRequestCount, 0);
  assert.strictEqual(hero.snapshot().channelCount, 31);
  assert.strictEqual(hero.snapshot().programmeLoadedChannelCount, 24);
  assert.strictEqual(hero.snapshot().programmeHasMore, true);
  assert.strictEqual(hero.snapshot().selectedChannelId, 'C1');
  assert.strictEqual(hero.snapshot().currentEventTitle, 'Heute Eins');
  assert.strictEqual(hero.snapshot().nextEventTitle, 'Danach Heute Eins');
  assert(flattenText(heroRoot).includes('Eins'));
  assert(findByClass(heroRoot, 'media-home-live-artwork'));
  assert.strictEqual(heroRoot.tabIndex, 0);

  const nowSection = additionalSections.querySelector('[data-home-live-guide="now"]');
  const nextSection = additionalSections.querySelector('[data-home-live-guide="next"]');
  assert.ok(nowSection, 'Home must render Was läuft jetzt from the Hero-owned EPG projection');
  assert.ok(nextSection, 'Home must render Was läuft danach from the same EPG projection');
  assert.strictEqual(additionalSections.children[0], nowSection);
  assert.strictEqual(additionalSections.children[1], nextSection);
  assert(flattenText(nowSection).includes('Was läuft jetzt'));
  assert(flattenText(nowSection).includes('Heute Eins'));
  assert(flattenText(nowSection).includes('Heute Zwei'));
  assert(flattenText(nowSection).includes('Heute Zwanzig'));
  assert(flattenText(nextSection).includes('Was läuft danach'));
  assert(flattenText(nextSection).includes('Danach Heute Eins'));
  assert(flattenText(nextSection).includes('Danach Heute Zwei'));
  assert(flattenText(nextSection).includes('Danach Heute Zwanzig'));
  assert.strictEqual(findByClass(nowSection, 'media-home-live-guide-rail').children.length, 24);
  assert.strictEqual(findByClass(nextSection, 'media-home-live-guide-rail').children.length, 24);
  assert(findByClass(nowSection, 'media-home-live-guide-artwork'));

  assert.strictEqual(await hero.__test.loadNextProgrammePage(), true);
  assert.strictEqual(epgFetchCount, 2);
  assert.strictEqual(epgChannelRequests[1].length, 7);
  assert.strictEqual(hero.snapshot().programmeLoadedChannelCount, 31);
  assert.strictEqual(hero.snapshot().programmeHasMore, false);
  assert.strictEqual(findByClass(nowSection, 'media-home-live-guide-rail').children.length, 31);
  assert.strictEqual(findByClass(nextSection, 'media-home-live-guide-rail').children.length, 31);
  assert.strictEqual(new Set(findByClass(nowSection, 'media-home-live-guide-rail').children.map(card => card.dataset.channelId)).size, 31);

  const dataRequestBaseline = channelFetchCount + epgFetchCount;

  // Pointer activation gives the persistent Hero root keyboard ownership.
  heroRoot.focused = false;
  heroRoot.dispatch('click', {target: findByClass(heroRoot, 'media-home-live-focus')});
  assert.strictEqual(heroRoot.focused, true);

  // Rapid browsing is presentation state only. Re-rendering must return focus to
  // the persistent Hero root so repeated real-browser Arrow events keep working.
  heroRoot.focused = false;
  heroRoot.dispatch('keydown', {key: 'ArrowRight'});
  assert.strictEqual(hero.snapshot().selectedChannelId, 'C2');
  assert.strictEqual(hero.snapshot().currentEventTitle, 'Heute Zwei');
  assert.strictEqual(heroRoot.focused, true);
  assert.strictEqual(liveStartCount, 0);
  assert.strictEqual(sessionRequestCount, 0);
  assert.strictEqual(channelFetchCount + epgFetchCount, dataRequestBaseline);

  heroRoot.focused = false;
  heroRoot.dispatch('keydown', {key: 'ArrowRight'});
  assert.strictEqual(hero.snapshot().selectedChannelId, 'C20');
  assert.strictEqual(hero.snapshot().currentEventTitle, 'Heute Zwanzig');
  assert.strictEqual(heroRoot.focused, true);
  assert.strictEqual(liveStartCount, 0);
  assert.strictEqual(sessionRequestCount, 0);
  assert.strictEqual(channelFetchCount + epgFetchCount, dataRequestBaseline);

  heroRoot.dispatch('touchstart', {touches: [{clientX: 210, clientY: 30}]});
  heroRoot.dispatch('touchend', {changedTouches: [{clientX: 105, clientY: 34}]});
  assert.strictEqual(hero.snapshot().selectedChannelId, 'C21');
  assert.strictEqual(hero.snapshot().currentEventTitle, 'Heute 21');
  assert.strictEqual(liveStartCount, 0);
  assert.strictEqual(sessionRequestCount, 0);
  assert.strictEqual(channelFetchCount + epgFetchCount, dataRequestBaseline);

  // Rendering is viewport-independent; CSS recomposes the same focused truth.
  const selectedBeforeRerender = hero.snapshot().selectedChannelId;
  hero.__test.render();
  assert.strictEqual(hero.snapshot().selectedChannelId, selectedBeforeRerender);
  assert(findButton(heroRoot, 'Live ansehen'));
  assert(findButton(heroRoot, 'EPG'));
  assert.strictEqual(channelFetchCount + epgFetchCount, dataRequestBaseline, 'programme rails must not add API reads');

  // Only explicit Watch Live delegates to the existing canonical Live-TV owner.
  findButton(heroRoot, 'Live ansehen').click();
  assert.strictEqual(liveNavigationCount, 1);
  assert.strictEqual(liveStartCount, 1);
  assert.strictEqual(window.lastStartedChannelId, 'C21');

  // EPG remains the existing owning navigation flow.
  findButton(heroRoot, 'EPG').click();
  assert.strictEqual(epgNavigationCount, 1);
  assert.strictEqual(sessionRequestCount, 0);

  console.log('phase66 live tv hero browse-only production contract ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});