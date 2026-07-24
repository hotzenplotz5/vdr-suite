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
    this._textContent = String(
      value === undefined || value === null ? '' : value
    );
  }

  get textContent() {
    return this._textContent +
      this.children.map(child => child.textContent || '').join('');
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
  const epgQueries = [];
  const client = {
    fetchClientGenres: options => Promise.resolve(
      options.scope === 'epg'
        ? {
            backendId: 'default',
            scope: 'epg',
            totalItems: 5,
            categories: [
              {
                id: 'movie',
                label: 'Film',
                count: 2,
                children: [
                  {id: 'thriller', label: 'Thriller', count: 1},
                  {id: 'drama', label: 'Drama', count: 1}
                ]
              },
              {id: 'series', label: 'Serie', count: 1, children: []},
              {
                id: 'documentary',
                label: 'Dokumentation',
                count: 1,
                children: []
              },
              {id: 'sports', label: 'Sport', count: 1, children: []}
            ]
          }
        : {
            backendId: 'default',
            scope: 'recordings',
            totalItems: 1,
            genres: [
              {id: 'action', label: 'Action', count: 1, known: true}
            ]
          }
    ),
    fetchClientChannels: () => {
      channelRequests += 1;
      return new Promise(() => {});
    },
    fetchClientGenreRecordings: () => Promise.resolve({
      items: [],
      total: 0
    }),
    fetchClientGenreEpg: options => {
      epgItemRequests += 1;
      epgQueries.push({
        contentClass: options.contentClass,
        genreId: options.genreId
      });
      const title = options.contentClass === 'series'
        ? 'Lifestyle-Serie'
        : 'Testfilm';
      return Promise.resolve({
        items: [{
          eventId: '100',
          channelId: 'C-1',
          channelName: 'Das Erste HD',
          title: title,
          startTime: 1784839299,
          artwork: {available: false, url: ''}
        }],
        total: 1,
        hasMore: false
      });
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

  assert(mount.textContent.includes('5 Einträge'));
  assert(mount.textContent.includes('4 Hauptkategorien'));
  for (const label of ['Film', 'Serie', 'Dokumentation', 'Sport']) {
    assert(mount.textContent.includes(label), label + ' was not rendered');
  }
  for (const hidden of ['Nachrichten', 'Talkshow', 'Reality']) {
    assert(!mount.textContent.includes(hidden), hidden + ' must stay hidden');
  }

  for (let index = 0; index < 12; index += 1) {
    const filmButton = findButton(mount, 'Film', true);
    assert(filmButton, 'Film category disappeared');
    filmButton.dispatch('click');

    assert(mount.textContent.includes('Filmgenres'));
    assert(mount.textContent.includes('Alle Filme'));
    assert(mount.textContent.includes('Thriller'));
    assert(!mount.textContent.includes('Nachrichten'));

    const thrillerButton = findButton(mount, 'Thriller', true);
    assert(thrillerButton, 'Thriller child disappeared');
    thrillerButton.dispatch('click');
    await flush();
    await flush();

    assert(mount.textContent.includes('Das Erste HD'));
    assert.strictEqual(
      epgQueries[epgQueries.length - 1].contentClass,
      'movie'
    );
    assert.strictEqual(
      epgQueries[epgQueries.length - 1].genreId,
      'thriller'
    );

    const filmBack = findButton(mount, '← Filmgenres', false);
    assert(filmBack, 'Film result back button was not rendered');
    filmBack.dispatch('click');

    const mainBack = findButton(mount, '← EPG-Hauptkategorien', false);
    assert(mainBack, 'Film hierarchy back button was not rendered');
    mainBack.dispatch('click');
  }

  const seriesButton = findButton(mount, 'Serie', true);
  assert(seriesButton, 'Series category was not rendered');
  seriesButton.dispatch('click');
  await flush();
  await flush();

  assert.strictEqual(epgQueries[epgQueries.length - 1].contentClass, 'series');
  assert.strictEqual(epgQueries[epgQueries.length - 1].genreId, '');
  assert(mount.textContent.includes('Lifestyle-Serie'));

  assert.strictEqual(epgItemRequests, 13);
  assert.strictEqual(
    channelRequests, 0,
    'Genre browser must never issue a supplementary VDR channel request'
  );

  console.log(
    'genres runtime database-only EPG Genre navigation hierarchy ok'
  );
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
