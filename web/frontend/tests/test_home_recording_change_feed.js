'use strict';
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

class Element {
  constructor() { this.children = []; this.attributes = {}; this.dataset = {}; this.style = {}; }
  appendChild(node) { node.parentNode = this; this.children.push(node); return node; }
  append(...nodes) { nodes.forEach(node => this.appendChild(node)); }
  replaceChildren(...nodes) { this.children = []; this.append(...nodes); }
  insertBefore(node, before) {
    node.parentNode = this;
    const index = this.children.indexOf(before);
    if (index < 0) this.children.push(node); else this.children.splice(index, 0, node);
  }
  setAttribute(key, value) { this.attributes[key] = value; }
  addEventListener() {}
  remove() { if (this.parentNode) this.parentNode.children = this.parentNode.children.filter(n => n !== this); }
  querySelector(selector) {
    const match = selector.match(/data-home-discovery-rail="([^"]+)"/);
    if (!match) return null;
    return this.children.find(node => node.attributes['data-home-discovery-rail'] === match[1]) || null;
  }
  querySelectorAll() { return []; }
}
const host = new Element();
const listeners = {};
const observers = [];
const timers = new Map();
const sources = [];
let timerId = 0;
let backend = 'A';
let moduleName = 'overview';
let recordingReads = 0;
let folderReads = 0;
let pendingRead = null;
const doc = {
  hidden: false,
  head: new Element(),
  createElement() { return new Element(); },
  getElementById() { return null; },
  querySelector(selector) { return selector === '[data-home-zone="additional-sections"]' ? host : null; },
  addEventListener(type, listener) { (listeners[type] ||= []).push(listener); }
};
const client = {
  createClientLiveUpdateSource() {
    const source = {
      handlers: {}, closed: false,
      addEventListener(type, listener) { this.handlers[type] = listener; },
      close() { this.closed = true; },
      emit(sequenceNumber, backendId = backend, changedDomains = ['recordings']) {
        this.handlers.update({data: JSON.stringify({sequenceNumber, backendId, changedDomains})});
      }
    };
    sources.push(source);
    return source;
  },
  fetchClientRecordings(options) {
    recordingReads++;
    assert.strictEqual(options.cache, 'no-store');
    assert.strictEqual(options.query.backend, backend);
    if (pendingRead) return pendingRead;
    return Promise.resolve({recordings: []});
  },
  fetchClientRecordingFolder(options) {
    folderReads++;
    assert.strictEqual(options.query.backend, backend);
    return Promise.resolve({folders: [], recordings: []});
  },
  fetchClientGenres() { return Promise.resolve({genres: []}); },
  fetchClientGenreRecordings() { return Promise.resolve({recordings: []}); }
};
const window = {
  document: doc, console,
  setTimeout(fn) { const id = ++timerId; timers.set(id, fn); return id; },
  clearTimeout(id) { timers.delete(id); },
  IntersectionObserver: class {
    constructor(callback) { this.callback = callback; observers.push(this); }
    observe() {}
    disconnect() {}
  },
  VdrSuitePlatform: {
    getClientApi() { return client; },
    getSelectedModule() { return moduleName; },
    getSelectedBackendId() { return backend; }
  }
};
window.window = window;
vm.runInNewContext(fs.readFileSync(path.join(__dirname, '..', 'home-recording-discovery.js'), 'utf8'),
  {window, console}, {filename: 'home-recording-discovery.js'});

const settle = () => new Promise(resolve => setImmediate(resolve));
async function tick() {
  const ready = [...timers.values()]; timers.clear();
  ready.forEach(fn => fn());
  for (let i = 0; i < 4; i++) await settle();
}
function visibility(hidden) { doc.hidden = hidden; (listeners.visibilitychange || []).forEach(fn => fn()); }
function navigate(next) {
  moduleName = next;
  const target = {
    dataset: {module: next},
    closest(selector) {
      if (selector === '.module-tab[data-module], [data-brand-module]') return this;
      return next === 'overview' && selector.includes('overview') ? this : null;
    }
  };
  (listeners.click || []).forEach(fn => fn({target}));
}

(async () => {
  // Real install -> lazy visibility -> canonical Home refresh, no test-only subscription call.
  observers[0].callback([{isIntersecting: true}]);
  await tick();
  assert.strictEqual(sources.length, 1, 'Home must subscribe through the production lifecycle');
  assert.strictEqual(recordingReads, 1);
  const first = sources[0];
  first.onopen();
  first.emit(1, 'B');
  first.emit(2, 'A', ['recordingMarks']);
  first.handlers.update({data: 'bad-json'});
  await tick();
  assert.strictEqual(recordingReads, 1);
  first.emit(3);
  first.emit(4);
  first.emit(4);
  await tick();
  assert.strictEqual(recordingReads, 2, 'same-backend recording burst invalidates retained Home once');
  assert.strictEqual(folderReads, 2);
  assert(host.children.length > 0, 'canonical Home renders the refreshed projection');
  await window.VdrSuiteHomeRecordingDiscovery._test.refreshForHome();
  assert.strictEqual(recordingReads, 2, 'unchanged Home remains retained');

  let resolveRead;
  pendingRead = new Promise(resolve => { resolveRead = resolve; });
  first.emit(5);
  await tick();
  first.emit(6);
  await tick();
  assert.strictEqual(recordingReads, 3, 'one in-flight refresh only');
  pendingRead = null;
  resolveRead({recordings: []});
  await settle();
  await tick();
  assert.strictEqual(recordingReads, 4, 'a hint during a read survives');

  visibility(true);
  assert(first.closed);
  first.emit(7);
  await tick();
  assert.strictEqual(recordingReads, 4, 'closed source cannot refresh hidden Home');
  visibility(false);
  await tick();
  assert.strictEqual(sources.length, 2);
  assert.strictEqual(recordingReads, 5, 'visibility recovery catches up without manual refresh');
  const second = sources[1];
  second.onopen();
  second.emit(20);
  await tick();
  const beforeReconnect = recordingReads;
  second.onerror();
  await tick();
  assert.strictEqual(recordingReads, beforeReconnect, 'normal finite-feed close does not reload Home');
  second.onopen();
  second.emit(1);
  second.onerror();
  await tick();
  assert.strictEqual(recordingReads, beforeReconnect + 1, 'daemon sequence reset catches up');

  navigate('recordings2');
  assert(second.closed);
  await tick();
  const beforeExit = recordingReads;
  second.emit(99);
  await tick();
  assert.strictEqual(recordingReads, beforeExit);
  backend = 'B';
  navigate('overview');
  await tick();
  observers[observers.length - 1].callback([{isIntersecting: true}]);
  await tick();
  const third = sources[sources.length - 1];
  assert(!third.closed);
  const beforeBackend = recordingReads;
  third.onopen();
  third.emit(100, 'A');
  await tick();
  assert.strictEqual(recordingReads, beforeBackend, 'old backend does not invalidate current Home');
  third.emit(101, 'B');
  await tick();
  assert.strictEqual(recordingReads, beforeBackend + 1);
  console.log('Home recording feed production lifecycle: PASS');
})().catch(error => { console.error(error); process.exitCode = 1; });
