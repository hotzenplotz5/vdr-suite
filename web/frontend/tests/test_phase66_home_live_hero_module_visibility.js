'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');
const heroSource = fs.readFileSync(path.join(frontendRoot, 'home-live-hero.js'), 'utf8');

assert(heroSource.includes("return text(value.getSelectedModule());"));
assert(heroSource.includes("root.classList.remove('media-home-live-hero-active')"));
assert.strictEqual((heroSource.match(/new global\.MutationObserver/g) || []).length, 1);
assert(!heroSource.includes('setInterval('));

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
    appendChild(child) {
      this.children.push(child);
      child.parentNode = this;
      return child;
    },
    replaceChildren(...children) {
      this.children = [];
      children.forEach(child => this.appendChild(child));
    },
    setAttribute(name, value) {
      attributes[name] = String(value);
      if (name === 'tabindex') this.tabIndex = Number(value);
    },
    getAttribute(name) { return attributes[name] || null; },
    hasAttribute(name) { return Object.prototype.hasOwnProperty.call(attributes, name); },
    addEventListener(type, listener) {
      (listeners[type] ||= []).push(listener);
    },
    focus() {},
    scrollIntoView() {}
  };
}

const heroRoot = createNode('section');
heroRoot.dataset.homeZone = 'hero';

const document = {
  readyState: 'loading',
  head: null,
  createElement: createNode,
  querySelector(selector) {
    if (selector === '.media-home-hero[data-home-zone="hero"]') return heroRoot;
    return null;
  },
  getElementById() { return null; },
  addEventListener() {}
};

let selectedModule = 'overview';
let channelFetchCount = 0;
let epgFetchCount = 0;
const now = Math.floor(Date.now() / 1000);
const clientApi = {
  fetchClientChannels(options) {
    channelFetchCount += 1;
    assert.strictEqual(options.query.backend, 'backend-a');
    return Promise.resolve({
      channels: [
        {id: 'C1', name: 'Eins', number: 1, radio: false, enabled: true}
      ]
    });
  },
  fetchClientEpgCacheWindow(options) {
    epgFetchCount += 1;
    assert.strictEqual(options.query.backend, 'backend-a');
    assert.strictEqual(options.query.channelIds, 'C1');
    return Promise.resolve({
      events: [
        {channelId: 'C1', title: 'Heute Eins', startTime: now - 300, endTime: now + 900},
        {channelId: 'C1', title: 'Danach Eins', startTime: now + 900, endTime: now + 1800}
      ]
    });
  }
};

const window = {
  document,
  console,
  setTimeout,
  VdrSuitePlatform: {
    getClientApi() { return clientApi; },
    getSelectedBackendId() { return 'backend-a'; },
    getSelectedModule() { return selectedModule; }
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
  createChannelLogoElement() { return createNode('span'); }
});

vm.runInContext(heroSource, context, {filename: 'web/frontend/home-live-hero.js'});
assert.ok(window.VdrSuiteHomeLiveHero);

(async function () {
  const hero = window.VdrSuiteHomeLiveHero;

  await hero.refresh();
  assert.strictEqual(hero.snapshot().active, true);
  assert.strictEqual(heroRoot.classList.contains('media-home-live-hero-active'), true);
  assert.strictEqual(channelFetchCount, 1);
  assert.strictEqual(epgFetchCount, 1);
  assert.strictEqual(hero.snapshot().currentEventTitle, 'Heute Eins');

  const moduleNames = Array.from(new Set(
    Array.from(indexSource.matchAll(/data-module="([^"]+)"/g), match => match[1])
  )).filter(moduleName => moduleName !== 'overview');

  ['channels2', 'genres', 'recordings2', 'epg', 'channelsort', 'timers', 'searchtimers'].forEach(moduleName => {
    assert(moduleNames.includes(moduleName), 'production module navigation must include ' + moduleName);
  });

  ['series', 'settings', 'search'].forEach(moduleName => moduleNames.push(moduleName));

  for (const moduleName of moduleNames) {
    selectedModule = moduleName;
    await hero.__test.sync(false);
    assert.strictEqual(hero.snapshot().active, false, moduleName + ' must deactivate the Home hero');
    assert.strictEqual(
      heroRoot.classList.contains('media-home-live-hero-active'),
      false,
      moduleName + ' must not retain the Home hero presentation class'
    );
  }

  assert.strictEqual(channelFetchCount, 1, 'module changes must not add Home hero API reads');
  assert.strictEqual(epgFetchCount, 1, 'module changes must not add Home programme API reads');

  selectedModule = 'overview';
  await hero.__test.sync(false);
  assert.strictEqual(hero.snapshot().active, true);
  assert.strictEqual(heroRoot.classList.contains('media-home-live-hero-active'), true);
  assert.strictEqual(channelFetchCount, 1, 'returning Home reuses cached channel data without a parallel polling path');
  assert.strictEqual(epgFetchCount, 1, 'warm Home return reuses the fresh canonical EPG projection without another request');
  assert.strictEqual(hero.snapshot().currentEventTitle, 'Heute Eins');
  assert.strictEqual(document.querySelector('.media-home-hero[data-home-zone="hero"]'), heroRoot);

  console.log('phase66 Home Live-TV hero module visibility contract ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
