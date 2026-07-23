'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'modules', 'genres.js'),
  'utf8'
);

class FakeElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.listeners = Object.create(null);
    this.className = '';
    this.id = '';
    this.type = '';
    this.disabled = false;
    this._textContent = '';
    this.classList = {
      add: name => this.setClass(name, true),
      remove: name => this.setClass(name, false),
      toggle: (name, enabled) => this.setClass(name, enabled),
      contains: name => this.className.split(/\s+/).includes(name)
    };
  }

  setClass(name, enabled) {
    const names = new Set(this.className.split(/\s+/).filter(Boolean));
    if (enabled) names.add(name);
    else names.delete(name);
    this.className = Array.from(names).join(' ');
  }

  set textContent(value) {
    this._textContent = String(value === undefined || value === null ? '' : value);
  }

  get textContent() {
    return this._textContent + this.children.map(child => child.textContent || '').join('');
  }

  append() {
    Array.from(arguments).forEach(child => this.appendChild(child));
  }

  appendChild(child) {
    if (child) this.children.push(child);
    return child;
  }

  replaceChildren() {
    this.children = [];
    this._textContent = '';
    this.append.apply(this, arguments);
  }

  addEventListener(type, handler) {
    if (!this.listeners[type]) this.listeners[type] = [];
    this.listeners[type].push(handler);
  }

  dispatch(type) {
    (this.listeners[type] || []).forEach(handler => handler({target: this}));
  }
}

function findButton(root, expected, contains) {
  if (root.tagName === 'BUTTON') {
    const matches = contains
      ? root.textContent.includes(expected)
      : root.textContent === expected;
    if (matches) return root;
  }
  for (const child of root.children) {
    const found = findButton(child, expected, contains);
    if (found) return found;
  }
  return null;
}

function flush() {
  return new Promise(resolve => setImmediate(resolve));
}

(async function main() {
  const mount = new FakeElement('main');
  const elementsById = Object.create(null);
  const head = new FakeElement('head');
  head.appendChild = function (child) {
    FakeElement.prototype.appendChild.call(this, child);
    if (child && child.id) elementsById[child.id] = child;
    return child;
  };

  const document = {
    head: head,
    createElement: tag => new FakeElement(tag),
    getElementById: id => elementsById[id] || null,
    querySelector: () => null,
    querySelectorAll: () => [],
    addEventListener: () => {}
  };

  let registeredModule = null;
  let channelRequests = 0;
  let epgItemRequests = 0;
  const requestOrder = [];
  const client = {
    fetchClientGenres: options => {
      requestOrder.push('genres:' + options.scope);
      if (options.scope === 'epg' && channelRequests > 0) {
        return new Promise(() => {});
      }
      return Promise.resolve(
        options.scope === 'epg'
          ? {
              backendId: 'default',
              scope: 'epg',
              totalItems: 1,
              genres: [{id: 'drama', label: 'Drama', count: 1, known: true}]
            }
          : {
              backendId: 'default',
              scope: 'recordings',
              totalItems: 2,
              genres: [{id: 'action', label: 'Action', count: 2, known: true}]
            }
      );
    },
    fetchClientChannels: () => {
      requestOrder.push('channels');
      channelRequests += 1;
      return new Promise(() => {});
    },
    fetchClientGenreRecordings: () => Promise.resolve({items: [], total: 0}),
    fetchClientGenreEpg: () => {
      requestOrder.push('epg-items');
      epgItemRequests += 1;
      return Promise.resolve({items: [], total: 0});
    }
  };

  const platform = {
    registerModule: (name, moduleApi) => {
      assert.strictEqual(name, 'genres');
      registeredModule = moduleApi;
    },
    hasModule: () => false,
    getClientApi: () => client,
    getMountTarget: name => name === 'genres' ? mount : null,
    getSelectedBackendId: () => 'default'
  };

  const context = {
    window: null,
    document: document,
    console: console,
    Date: Date,
    Error: Error,
    Intl: Intl,
    Math: Math,
    Number: Number,
    Object: Object,
    Promise: Promise,
    Set: Set,
    String: String,
    setTimeout: setTimeout,
    clearTimeout: clearTimeout,
    VdrSuitePlatform: platform
  };
  context.window = context;

  vm.createContext(context);
  vm.runInContext(source, context);

  assert(registeredModule, 'genres module was not registered');
  registeredModule.activate();
  await flush();
  await flush();

  const epgButton = findButton(mount, 'EPG', false);
  assert(epgButton, 'EPG scope button was not rendered');
  epgButton.dispatch('click');
  await flush();
  await flush();

  assert.strictEqual(channelRequests, 0,
    'EPG overview must not start supplementary channels before the genre response');
  assert(!mount.textContent.includes('Genres werden aus dem persistenten Index geladen'),
    'EPG overview must render before supplementary channel metadata');
  assert(mount.textContent.includes('1 Einträge'), 'EPG overview result was not rendered');
  assert(mount.textContent.includes('Drama'), 'EPG genre card was not rendered');

  const dramaButton = findButton(mount, 'Drama', true);
  assert(dramaButton, 'EPG genre card button was not rendered');
  dramaButton.dispatch('click');
  await flush();
  await flush();
  await new Promise(resolve => setTimeout(resolve, 10));

  assert.strictEqual(epgItemRequests, 1, 'EPG result page should load once');
  assert.strictEqual(channelRequests, 1,
    'supplementary channels should start only after EPG results render');
  assert(requestOrder.indexOf('genres:epg') < requestOrder.indexOf('channels'),
    'EPG genre request must be scheduled before supplementary channels');
  assert(requestOrder.indexOf('epg-items') < requestOrder.indexOf('channels'),
    'EPG result request must complete before supplementary channels start');
  assert(!mount.textContent.includes('Genres werden aus dem persistenten Index geladen'),
    'pending supplementary channels must not restore the loading state');

  console.log('genres runtime request ordering ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
