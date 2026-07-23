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

function findByText(root, expected) {
  if (root.tagName === 'BUTTON' && root.textContent === expected) return root;
  for (const child of root.children) {
    const found = findByText(child, expected);
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
  const client = {
    fetchClientGenres: options => Promise.resolve(
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
    ),
    fetchClientChannels: () => {
      channelRequests += 1;
      return new Promise(() => {});
    },
    fetchClientGenreRecordings: () => Promise.resolve({items: [], total: 0}),
    fetchClientGenreEpg: () => Promise.resolve({items: [], total: 0})
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

  const epgButton = findByText(mount, 'EPG');
  assert(epgButton, 'EPG scope button was not rendered');
  epgButton.dispatch('click');
  await flush();
  await flush();

  assert.strictEqual(channelRequests, 1, 'EPG channel metadata should load asynchronously once');
  assert(!mount.textContent.includes('Genres werden aus dem persistenten Index geladen'),
    'EPG overview must not wait for an unresolved channel request');
  assert(mount.textContent.includes('1 Einträge'), 'EPG overview result was not rendered');
  assert(mount.textContent.includes('Drama'), 'EPG genre card was not rendered');

  console.log('genres runtime independence ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
