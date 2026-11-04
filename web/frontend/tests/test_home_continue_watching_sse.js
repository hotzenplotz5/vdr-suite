'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(path.join(__dirname, '..', 'home-continue-watching.js'), 'utf8');

class Element {
  constructor(tag) {
    this.tagName = String(tag || '').toUpperCase();
    this.children = [];
    this.parentNode = null;
    this.dataset = {};
    this.attributes = {};
    this.className = '';
    this.textContent = '';
    this.scrollLeft = 0;
    this.listeners = {};
    this.classList = {
      add: name => { this.className += ' ' + name; }
    };
  }
  appendChild(node) {
    if (node.parentNode) node.remove();
    node.parentNode = this;
    this.children.push(node);
    return node;
  }
  append(...nodes) { nodes.forEach(node => this.appendChild(node)); }
  insertBefore(node, before) {
    if (node.parentNode) node.remove();
    node.parentNode = this;
    const index = this.children.indexOf(before);
    if (index < 0) this.children.push(node);
    else this.children.splice(index, 0, node);
    return node;
  }
  remove() {
    if (!this.parentNode) return;
    this.parentNode.children = this.parentNode.children.filter(child => child !== this);
    this.parentNode = null;
  }
  replaceChildren(...nodes) {
    this.children.forEach(node => { node.parentNode = null; });
    this.children = [];
    this.append(...nodes);
  }
  setAttribute(key, value) { this.attributes[key] = String(value); }
  addEventListener(event, callback) { (this.listeners[event] ||= []).push(callback); }
  click() { (this.listeners.click || []).forEach(callback => callback()); }
  querySelector(selector) {
    if (selector === '[data-home-continue-watching]') {
      return this.children.find(node => Object.hasOwn(node.attributes, 'data-home-continue-watching')) || null;
    }
    return null;
  }
}

const listeners = {};
const host = new Element('div');
const head = new Element('head');
let backend = 'A';
let moduleName = 'overview';
let authenticated = true;
let items = [];
let reads = 0;
const queuedResponses = [];
const sources = [];
const timers = [];
const selectedBackend = () => ({dataset: {backendId: backend}});
const document = {
  readyState: 'loading',
  hidden: false,
  head,
  createElement: tag => new Element(tag),
  getElementById: () => null,
  querySelector(selector) {
    if (selector === '[data-home-zone="additional-sections"]') return host;
    if (selector === '#backends .backend-card.selected, #backends [aria-selected="true"]') {
      return selectedBackend();
    }
    return null;
  },
  addEventListener(event, callback) { (listeners[event] ||= []).push(callback); }
};
function item(id, position, poster = '/poster-' + id + '.webp') {
  return {
    backendId: backend,
    recordingId: id,
    title: 'Film ' + id,
    posterUrl: poster,
    resumePositionSeconds: position,
    durationKnown: true,
    durationSeconds: 600,
    recording: {id, recordingId: id, backendId: backend, title: 'Film ' + id}
  };
}
items = [item('one', 80), item('two', 140)];
const client = {
  createClientLiveUpdateSource() {
    const source = {
      closed: false, handlers: {},
      addEventListener(name, handler) { this.handlers[name] = handler; },
      close() { this.closed = true; },
      emit(sequenceNumber, changedDomains = ['continueWatching'], backendId = backend) {
        this.handlers.update({data: JSON.stringify({sequenceNumber, changedDomains, backendId})});
      }
    };
    sources.push(source);
    return source;
  }
};
const window = {
  window: null, document,
  VdrSuiteBrowserSession: {
    isAuthenticated() { return authenticated; },
    csrfHeaders() { return {'X-VDR-Suite-CSRF': 'session-proof'}; }
  },
  VdrSuitePlatform: {
    getSelectedModule() { return moduleName; },
    getClientApi() { return client; }
  },
  setTimeout(callback) { timers.push(callback); return timers.length; },
  clearTimeout() {},
  fetch(url, options) {
    assert.strictEqual(url, '/api/media/continue-watching');
    assert.strictEqual(options.credentials, 'same-origin');
    assert.strictEqual(options.headers['X-VDR-Suite-CSRF'], 'session-proof');
    const body = JSON.parse(options.body);
    assert.strictEqual(body.operation, 'list');
    assert.strictEqual(body.backendId, backend);
    reads++;
    if (queuedResponses.length) return queuedResponses.shift()();
    return Promise.resolve({ok: true, json: () => Promise.resolve({items: items.slice()})});
  }
};
window.window = window;
const context = vm.createContext({window, console});
vm.runInContext(source, context, {filename: 'home-continue-watching.js'});
const api = window.VdrSuiteHomeContinueWatching;
const settle = async () => {
  for (let i = 0; i < 12; i++) await new Promise(resolve => setImmediate(resolve));
};
const tick = async () => {
  timers.splice(0).forEach(callback => callback());
  await settle();
};
function section() { return host.querySelector('[data-home-continue-watching]'); }
function rail() { const s = section(); return s && s.children.find(node => node.className === 'media-home-continue-rail'); }
function card(id) { return rail() && rail().children.find(node => node.dataset.recordingId === id); }
function dispatch(event, arg) { (listeners[event] || []).forEach(handler => handler(arg || {})); }
function tab(module) {
  moduleName = module;
  dispatch('click', {target: {
    closest(selector) {
      if (selector === '.module-tab[data-module], [data-brand-module]') {
        return {dataset: {module}};
      }
      return null;
    }
  }});
}

(async function () {
  dispatch('DOMContentLoaded');
  await tick();
  assert.strictEqual(reads, 1, 'initial Home performs one canonical read');
  assert.strictEqual(sources.length, 1, 'uses the established SSE client factory');
  const first = sources[0];
  const oldOne = card('one');
  const oldTwo = card('two');
  const artwork = oldOne.children[0];
  const poster = artwork.children[0];
  rail().scrollLeft = 215;

  first.onopen();
  first.emit(1, ['timers']);
  first.emit(2, ['continueWatching'], 'B');
  first.emit(3, ['recordingMarks']);
  await tick();
  assert.strictEqual(reads, 1, 'unrelated domains and backends do not reload progress');

  items = [item('one', 200), item('two', 140)];
  first.emit(4);
  first.emit(5);
  first.emit(5);
  await tick();
  assert.strictEqual(reads, 2, 'SSE burst coalesces to one readback');
  assert.strictEqual(card('one'), oldOne, 'same card survives progress change');
  assert.strictEqual(card('one').children[0], artwork, 'same artwork node survives');
  assert.strictEqual(artwork.children[0], poster, 'same image survives');
  assert.strictEqual(card('two'), oldTwo, 'unaffected item identity survives');
  assert.strictEqual(rail().scrollLeft, 215, 'horizontal scroll position stays');
  assert.strictEqual(oldOne.__vdrSuiteContinueView.meter.value, 33);
  assert(oldOne.__vdrSuiteContinueView.progressText.textContent.includes('3:20'));

  const renamed = item('one', 200);
  renamed.title = 'Film Eins – umbenannt';
  items = [renamed, item('two', 140)];
  first.emit(6);
  await tick();
  assert.strictEqual(card('one'), oldOne, 'title correction retains the Recording card');
  assert.strictEqual(card('one').children[0], artwork,
    'title-only correction retains the existing artwork container');
  assert.strictEqual(artwork.children[0], poster,
    'title-only correction does not request the poster again');
  assert.strictEqual(poster.alt, 'Poster zu Film Eins – umbenannt');

  items = [item('one', 200)];
  first.emit(7);
  await tick();
  assert(!card('two'), 'completed/cleared recording leaves the rail');
  assert.strictEqual(card('one'), oldOne);

  queuedResponses.push(() => Promise.resolve({ok: false}));
  first.emit(8);
  await tick();
  assert.strictEqual(card('one'), oldOne, 'failed read retains last good projection');

  let releaseOld;
  queuedResponses.push(() => new Promise(resolve => { releaseOld = resolve; }));
  first.emit(9);
  await tick();
  items = [item('one', 360)];
  first.emit(10);
  await tick();
  assert.strictEqual(card('one').__vdrSuiteContinueItem.resumePositionSeconds, 360);
  releaseOld({ok: true, json: () => Promise.resolve({items: [item('one', 12)]})});
  await tick();
  assert.strictEqual(card('one').__vdrSuiteContinueItem.resumePositionSeconds, 360,
    'stale response cannot overwrite newer committed progress');

  document.hidden = true;
  dispatch('visibilitychange');
  assert(first.closed, 'hidden Home closes SSE connection');
  first.emit(11);
  await tick();
  assert.strictEqual(card('one').__vdrSuiteContinueItem.resumePositionSeconds, 360);
  document.hidden = false;
  dispatch('visibilitychange');
  await tick();
  assert.strictEqual(sources.length, 2, 'visibility return reconnects SSE');
  const second = sources[1];
  second.onopen();
  items = [item('one', 400)];
  second.emit(20);
  await tick();
  assert.strictEqual(card('one').__vdrSuiteContinueItem.resumePositionSeconds, 400);

  // Reconnect to a restarted daemon with a lower event sequence.
  second.onerror();
  await tick();
  const beforeReset = reads;
  second.onopen();
  second.emit(1);
  second.onerror();
  await tick();
  assert.strictEqual(reads, beforeReset + 1,
    'daemon sequence reset triggers one readback');

  tab('recordings2');
  assert(second.closed, 'leaving Home closes SSE connection');
  const beforeLeave = reads;
  second.emit(30);
  await tick();
  assert.strictEqual(reads, beforeLeave);
  tab('overview');
  dispatch('vdr-suite:home-resume');
  await tick();
  assert.strictEqual(sources.length, 3);

  // A changed backend is read from the authenticated owner, never from an event.
  dispatch('click', {target: {closest(selector) {
    return selector === '#backends' ? {} : null;
  }}});
  backend = 'B';
  items = [item('other', 50)];
  dispatch('vdr-suite:home-resume');
  await tick();
  assert(card('other'));
  assert(!card('one'));
  assert.strictEqual(sources.length, 4);
  const fourth = sources[3];
  const beforeWrongBackend = reads;
  fourth.emit(31, ['continueWatching'], 'A');
  await tick();
  assert.strictEqual(reads, beforeWrongBackend);

  authenticated = false;
  document.hidden = true;
  dispatch('visibilitychange');
  assert(fourth.closed);
  const beforeLogout = reads;
  fourth.emit(32);
  await tick();
  assert.strictEqual(reads, beforeLogout, 'logged-out actor cannot read progress');

  console.log('Home Continue Watching SSE readback and DOM retention: PASS');
}()).catch(error => { console.error(error); process.exitCode = 1; });
