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
const timers = new Map();
const sources = [];
let timerId = 0;
let backend = 'A';
let moduleName = 'overview';
let canonicalBackendReady = false;
let recordingReads = 0;
let folderReads = 0;
const queuedRecordingResponses = [];
const doc = {
  hidden: false,
  head: new Element(),
  createElement() { return new Element(); },
  getElementById() { return null; },
  querySelector(selector) {
    if (selector === '[data-home-zone="additional-sections"]') return host;
    if (selector === '#backends .backend-card.selected, #backends [aria-selected="true"]') {
      return canonicalBackendReady ? {dataset: {backendId: backend}} : null;
    }
    return null;
  },
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
    if (queuedRecordingResponses.length) {
      return queuedRecordingResponses.shift()();
    }
    return Promise.resolve({recordings: [{
      recordingId: 'import-' + recordingReads, backendId: backend,
      backendNativeId: '/recordings/import-' + recordingReads,
      title: 'Imported movie ' + recordingReads
    }]});
  },
  fetchClientRecordingFolder(options) {
    folderReads++;
    assert.strictEqual(options.backendId, backend);
    return Promise.resolve({folders: [], recordings: []});
  },
  fetchClientGenres() { return Promise.resolve({genres: []}); },
  fetchClientGenreRecordings() { return Promise.resolve({recordings: []}); }
};
const window = {
  document: doc, console,
  setTimeout(fn) { const id = ++timerId; timers.set(id, fn); return id; },
  clearTimeout(id) { timers.delete(id); },
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
  // Production shell selects the canonical backend before publishing Home resume.
  // Recording Discovery must start immediately from that lifecycle event; no
  // viewport/IntersectionObserver gate is allowed.
  canonicalBackendReady = true;
  (listeners['vdr-suite:home-resume'] || []).forEach(fn => fn({
    detail: {backendId: backend}
  }));
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
  const flatten = node => [node, ...node.children.flatMap(flatten)];
  assert(flatten(host).some(node => node.dataset.recordingId === 'import-2'),
    'native import hint reaches the actual Home card projection');
  await window.VdrSuiteHomeRecordingDiscovery._test.refreshForHome();
  assert.strictEqual(recordingReads, 2, 'unchanged Home remains retained');

  let resolveStaleRead;
  queuedRecordingResponses.push(() => new Promise(resolve => {
    resolveStaleRead = resolve;
  }));
  queuedRecordingResponses.push(() => Promise.resolve({recordings: [{
    recordingId: 'manual-current',
    backendId: backend,
    backendNativeId: '/recordings/manual-current',
    title: 'Manual current',
    metadata: {
      presentation: {posterUrl: '/manual/current.webp'},
      artwork: {preferredUrl: '/manual/current.webp'}
    }
  }]}));
  first.emit(5);
  await tick();
  first.emit(6);
  await tick();
  assert.strictEqual(recordingReads, 4,
    'a newer recording invalidation must start without waiting for a slow older generation');
  assert(flatten(host).some(node => node.dataset.recordingId === 'manual-current'),
    'newer presentation generation must publish while an older request is still pending');
  assert(flatten(host).some(node => node.src === '/manual/current.webp'),
    'newer manual artwork must be visible before the stale request finishes');

  resolveStaleRead({recordings: [{
    recordingId: 'stale-old',
    backendId: backend,
    backendNativeId: '/recordings/stale-old',
    title: 'Stale old',
    metadata: {
      presentation: {posterUrl: '/stale/old.webp'},
      artwork: {preferredUrl: '/stale/old.webp'}
    }
  }]});
  await settle();
  await tick();
  assert.strictEqual(recordingReads, 4,
    'stale completion must not trigger another serialized catch-up refresh');
  assert(!flatten(host).some(node => node.dataset.recordingId === 'stale-old'),
    'older generation must not overwrite newer recording presentation');
  assert(flatten(host).some(node => node.src === '/manual/current.webp'),
    'older generation must not overwrite newer manual artwork');
  assert(!flatten(host).some(node => node.src === '/stale/old.webp'),
    'stale artwork must never replace the newer manual artwork');

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
