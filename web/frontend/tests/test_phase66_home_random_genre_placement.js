'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8');

class FakeElement {
  constructor() {
    this.children = [];
    this.attributes = {};
    this.parentNode = null;
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }

  appendChild(child) {
    if (child.parentNode) child.remove();
    child.parentNode = this;
    this.children.push(child);
    return child;
  }

  insertBefore(child, reference) {
    if (child.parentNode) child.remove();
    child.parentNode = this;
    if (!reference) {
      this.children.push(child);
      return child;
    }
    const index = this.children.indexOf(reference);
    if (index < 0) this.children.push(child);
    else this.children.splice(index, 0, child);
    return child;
  }

  remove() {
    if (!this.parentNode) return;
    const index = this.parentNode.children.indexOf(this);
    if (index >= 0) this.parentNode.children.splice(index, 1);
    this.parentNode = null;
  }

  get nextElementSibling() {
    if (!this.parentNode) return null;
    const index = this.parentNode.children.indexOf(this);
    return index >= 0 ? this.parentNode.children[index + 1] || null : null;
  }

  querySelector(selector) {
    const match = String(selector || '').match(/^\[data-home-discovery-rail="([^"]+)"\]$/);
    if (!match) return null;
    return this.children.find((child) => child.attributes['data-home-discovery-rail'] === match[1]) || null;
  }
}

(async function () {
  const host = new FakeElement();
  const document = {
    readyState: 'loading',
    head: null,
    querySelector(selector) {
      return selector === '[data-home-zone="additional-sections"]' ? host : null;
    },
    addEventListener() {},
    getElementById() { return null; },
    createElement() { return new FakeElement(); }
  };

  const context = {window: {}, console};
  context.window.window = context.window;
  context.window.document = document;
  context.window.setTimeout = function () { return 1; };
  context.window.VdrSuitePlatform = {
    getSelectedBackendId() { return 'default'; },
    getSelectedModule() { return 'overview'; },
    getClientApi() { return null; }
  };

  vm.createContext(context);
  vm.runInContext(source, context, {filename: 'home-recording-discovery.js'});
  const api = context.window.VdrSuiteHomeRecordingDiscovery._test;
  assert(api);

  const newly = new FakeElement();
  newly.setAttribute('data-home-discovery-rail', 'newly-recorded');
  const genres = new FakeElement();
  genres.setAttribute('data-home-discovery-rail', 'genres');
  const folders = new FakeElement();
  folders.setAttribute('data-home-discovery-rail', 'folders');
  const random = new FakeElement();
  random.setAttribute('data-home-discovery-rail', 'random-genre');
  host.appendChild(newly);
  host.appendChild(genres);
  host.appendChild(folders);
  host.appendChild(random);

  assert.strictEqual(api.positionRandomGenreRail(), true);
  assert.deepStrictEqual(
    host.children.map((entry) => entry.attributes['data-home-discovery-rail']),
    ['newly-recorded', 'random-genre', 'genres', 'folders']
  );

  const recordings = Array.from({length: 205}, (_, index) => ({
    recordingId: 'drama-' + String(index + 1),
    backendId: 'default',
    title: 'Drama ' + String(index + 1)
  }));
  const calls = [];
  const client = {
    fetchClientGenreRecordings(request) {
      calls.push(request);
      const offset = Number(request.offset || 0);
      const limit = Number(request.limit || 0);
      const items = recordings.slice(offset, offset + limit);
      return Promise.resolve({
        recordings: items,
        total: recordings.length,
        hasMore: offset + items.length < recordings.length
      });
    }
  };

  const loaded = await api.fetchAllSeriesRecordings(client, 'default', 'drama', 0);
  assert.strictEqual(loaded.length, recordings.length);
  assert.strictEqual(loaded[0].recordingId, 'drama-1');
  assert.strictEqual(loaded[204].recordingId, 'drama-205');
  assert.deepStrictEqual(calls.map((call) => call.offset), [0, 100, 200]);
  assert(calls.every((call) => call.limit === 100));
  assert(calls.every((call) => call.genreId === 'drama'));

  assert(source.includes("fetchAllSeriesRecordings(client, backendId, id, generation)"));
  assert(!source.includes("fetchAllSeriesRecordings(client, backendId, id, generation).then(function (recordings) {\n        recordings = recordings.slice"));

  console.log('phase66 random genre placement and full membership regression ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
