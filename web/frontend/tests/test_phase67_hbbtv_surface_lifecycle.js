'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

// Execute the production view AND canonical Live playback owner. Only browser
// primitives and Suite HTTP responses are fixtures; no exported key/start wrapper.
const liveSource = fs.readFileSync('web/frontend/live-tv-view.js', 'utf8');
const timers = new Map();
const observers = [];
const requests = [];
const videos = [];
let nextTimer = 0;
let width = 640;
let frameRevision = 1;
let frameAvailable = true;
let mediaActive = false;
let sessionNumber = 0;
let session;
let pendingInput = null;

function style() {
  const values = new Map();
  return {
    setProperty(name, value, priority = '') { values.set(name, [String(value), priority]); },
    getPropertyValue(name) { return (values.get(name) || [''])[0]; },
    getPropertyPriority(name) { return (values.get(name) || ['', ''])[1]; },
    removeProperty(name) { values.delete(name); }
  };
}
function node(tag) {
  const listeners = {};
  const attrs = {};
  const value = {
    tagName: tag.toUpperCase(), children: [], parentNode: null, dataset: {},
    style: style(), className: '', hidden: false, disabled: false,
    classList: {add() {}, remove() {}, toggle() {}},
    appendChild(child) {
      if (child.parentNode) child.parentNode.children = child.parentNode.children.filter(v => v !== child);
      child.parentNode = this; this.children.push(child); return child;
    },
    replaceChildren(...children) {
      this.children.forEach(child => { child.parentNode = null; });
      this.children = []; children.forEach(child => this.appendChild(child));
    },
    contains(child) { return child === this || this.children.some(v => v.contains(child)); },
    querySelectorAll(selector) {
      const found = [];
      for (const child of this.children) {
        if (matches(child, selector)) found.push(child);
        found.push(...child.querySelectorAll(selector));
      }
      return found;
    },
    querySelector(selector) { return this.querySelectorAll(selector)[0] || null; },
    setAttribute(name, v) { attrs[name] = String(v); },
    getAttribute(name) { return attrs[name] || null; },
    removeAttribute(name) { delete attrs[name]; if (name === 'src') this.src = ''; },
    addEventListener(type, fn) { (listeners[type] ||= []).push(fn); },
    dispatch(type, event = {}) {
      const e = Object.assign({target: this, preventDefault() {}, stopPropagation() {}}, event);
      (listeners[type] || []).forEach(fn => fn(e));
    },
    click() { if (!this.disabled) this.dispatch('click'); },
    focus() {}, scrollIntoView() {},
    play() { return Promise.resolve(); }, pause() {},
    load() { this.videoWidth = 0; this.videoHeight = 0; },
    getBoundingClientRect() {
      const ratio = this.style.getPropertyValue('aspect-ratio').split('/').map(Number);
      const height = ratio.length === 2 ? width * ratio[1] / ratio[0] : 150;
      return {left: 0, top: 0, width, height};
    },
    getContext() { return {
      createImageData(w, h) { return {data: new Uint8ClampedArray(w * h * 4)}; },
      clearRect() {}, putImageData(image) { value.pixels = image.data; }
    }; }
  };
  if (tag === 'video') videos.push(value);
  return value;
}
function matches(value, selector) {
  if (selector[0] === '.') return value.className.split(/\s+/).includes(selector.slice(1));
  if (selector === '[data-hbbtv-action]') return Boolean(value.dataset.hbbtvAction);
  return value.tagName.toLowerCase() === selector;
}
function qoiFrame() {
  // A transparent video hole plus an opaque control pixel after OK. The QOI
  // decoder and putImageData path are production code, including alpha.
  const bytes = [113, 111, 105, 102, 0, 0, 0, 16, 0, 0, 0, 9, 4, 0];
  for (let i = 0; i < 144; ++i) bytes.push(255, 255, 255, 255, frameRevision > 1 && i === 143 ? 255 : 0);
  bytes.push(0, 0, 0, 0, 0, 0, 0, 1);
  return new Uint8Array(bytes);
}
const mount = node('div');
const document = {
  readyState: 'complete', hidden: false, visibilityState: 'visible',
  head: node('head'), createElement: node, addEventListener() {},
  getElementById(id) { return id === 'detail-data' ? mount : null; },
  querySelector() { return null; }, querySelectorAll() { return []; }
};
const client = {
  requestJson(path, options) {
    const body = JSON.parse(options.body);
    requests.push({kind: 'media', body});
    return Promise.resolve({mediaSession: {
      id: body.sessionId || 'live_session_test', resourceKind: 'live-channel',
      state: body.operation === 'stop' ? 'ended' : 'ready',
      presentationProfileId: 'live-progressive-fmp4',
      mediaPath: '/api/media/sessions/live_session_test/live/stream.mp4'
    }});
  },
  fetchClientChannels() { return Promise.resolve({channels: [{id: 'zdf', name: 'ZDF HD', enabled: true}]}); },
  fetchClientHbbtvApplications() { return Promise.resolve({result: 'available', available: true,
    applications: [{controlCode: 1, ref: {applicationId: 1, descriptorRevision: 1}}]}); },
  fetchClientHbbtvSessionLaunch() {
    requests.push({kind: 'launch'});
    session = {sessionId: 'app_' + (++sessionNumber), backendId: 'default', channelId: 'zdf',
      applicationId: 1, descriptorRevision: 1, state: 'active',
      capabilities: {inputActions: ['ok', 'red', 'green', 'up', 'down', 'left', 'right']}};
    return Promise.resolve(session);
  },
  fetchClientHbbtvSessionStatus() { return Promise.resolve(session); },
  fetchClientHbbtvSessionInput({payload}) {
    requests.push({kind: 'input', payload});
    frameRevision++;
    return pendingInput || Promise.resolve(session);
  },
  fetchClientHbbtvSessionClose() { mediaActive = false; session = {...session, state: 'closed'}; return Promise.resolve(session); },
  fetchClientHbbtvPresentation() {
    if (!frameAvailable) return Promise.resolve({status: 204, revision: frameRevision});
    return Promise.resolve({status: 200, revision: frameRevision, width: 16, height: 9, bytes: qoiFrame()});
  },
  fetchClientHbbtvMedia() { return Promise.resolve({sessionId: session.sessionId, backendId: 'default',
    available: mediaActive, state: mediaActive ? 'streaming' : 'none', mediaRevision: 1, fullscreen: true}); },
  mutateClientHbbtvMedia({payload}) {
    requests.push({kind: 'hbbtv-media', payload});
    return Promise.resolve({mediaSession: {id: 'hbbtv_media_test', resourceKind: 'hbbtv-media',
      state: 'ready', mediaRevision: 1, fullscreen: true,
      mediaPath: '/api/media/sessions/hbbtv_media_test/live/stream.mp4'}});
  }
};
const window = {
  document, console, VdrSuiteClientApi: client,
  VdrSuiteBrowserSession: {restore: () => Promise.resolve({authenticated: true}), csrfHeaders: () => ({}), subscribe() {}},
  addEventListener() {}, removeEventListener() {},
  setTimeout(fn) { const id = ++nextTimer; timers.set(id, fn); return id; },
  clearTimeout(id) { timers.delete(id); },
  ResizeObserver: class {
    constructor(fn) { this.fn = fn; this.targets = []; observers.push(this); }
    observe(target) { this.targets.push(target); }
    disconnect() { this.targets = []; }
  }
};
const context = vm.createContext({window, document, console, Uint8Array, Uint8ClampedArray});
for (const path of ['api/session-frontend-sync.js', 'hbbtv-qoi.js', 'live-tv-view.js']) {
  vm.runInContext(fs.readFileSync('web/frontend/' + path, 'utf8'), context, {filename: path});
}
const live = window.VdrSuiteLiveTvView;
const find = selector => mount.querySelector(selector);
const remote = action => find('.vdr-suite-hbbtv-remote').children.find(v => v.dataset.hbbtvAction === action);
async function flush() { for (let i = 0; i < 100; ++i) await Promise.resolve(); }
async function tick() {
  const pending = [...timers]; timers.clear();
  pending.forEach(([, fn]) => fn()); await flush();
}

async function run() {
  live.open(); await flush();
  find('.vdr-suite-live-tv-channel').click(); await flush();
  const video = videos[0];
  const owner = video.parentNode;
  const originalRatio = video.style.getPropertyValue('aspect-ratio');
  assert.strictEqual(videos.length, 1);
  assert.strictEqual(video.videoWidth, 0, 'launch before the first decoded Live frame');
  find('.vdr-suite-hbbtv-session-toggle').click();
  find('.vdr-suite-hbbtv-session-toggle').click();
  await flush(); await tick();
  assert.strictEqual(requests.filter(v => v.kind === 'launch').length, 1, 'rapid launch is single-flight');
  const canvas = find('.vdr-suite-hbbtv-overlay');
  assert.strictEqual(canvas.style.height, '360px', 'application viewport must not inherit the 150px metadata-less video height');
  assert.strictEqual(canvas.style.width, '640px');

  mediaActive = true; await tick();
  assert(video.src.includes('hbbtv_media_test'), 'actual canonical owner attached the external stream');
  assert.strictEqual(video.videoWidth, 0, 'external attach reset intrinsic metadata again');
  assert.strictEqual(canvas.style.height, '360px');
  remote('ok').click(); await flush(); await tick();
  assert.deepStrictEqual({...requests.find(v => v.kind === 'input').payload},
    {backendId: 'default', sessionId: 'app_1', action: 'ok'});
  assert.strictEqual(canvas.pixels[3], 0, 'transparent provider video hole survives QOI decode');
  assert.strictEqual(canvas.pixels[143 * 4 + 3], 255, 'OK can produce a newer opaque application pixel in the synthetic frame');
  const canvasZ = Number(liveSource.match(/\.vdr-suite-hbbtv-overlay\{[^}]*z-index:(\d+)/)[1]);
  assert(Number(video.style.getPropertyValue('z-index')) > canvasZ, 'real external media must remain ABOVE the captured application plane');
  assert.strictEqual(video.style.getPropertyValue('pointer-events'), 'none');
  for (const key of ['Enter', 'g', 'r', 'ArrowLeft']) {
    let prevented = false;
    canvas.dispatch('keydown', {key, preventDefault() { prevented = true; }});
    await flush(); assert(prevented);
  }
  assert.deepStrictEqual(requests.filter(v => v.kind === 'input').map(v => v.payload.action),
    ['ok', 'ok', 'green', 'red', 'left']);

  // A final container layout arrives independently of HTTP polling and input.
  width = 960;
  const observer = observers.find(v => v.targets.includes(video));
  assert(observer, 'the canonical video/container layout must be observed');
  observer.fn();
  assert.strictEqual(canvas.style.width, '960px');
  assert.strictEqual(canvas.style.height, '540px');
  const beforeRefresh = requests.length;
  live.__test.applyPrograms({}); live.__test.render();
  assert.strictEqual(find('video'), video);
  assert.strictEqual(video.parentNode, owner);
  assert.strictEqual(requests.length, beforeRefresh, 'unrelated presentation refresh must not stop/restart Live');

  find('.vdr-suite-hbbtv-session-toggle').click(); await flush(); await tick();
  assert.strictEqual(video.style.getPropertyValue('aspect-ratio'), originalRatio, 'close restores the broadcast layout');
  assert.strictEqual(video.style.getPropertyValue('z-index'), '');
  assert(video.src.includes('live_session_test'), 'close restores broadcast through the same owner');
  frameRevision = 1; mediaActive = false; frameAvailable = false;
  find('.vdr-suite-hbbtv-session-toggle').click(); await flush(); await tick();
  assert.strictEqual(live.snapshot().hbbtvSessionId, 'app_2');
  assert.strictEqual(video.style.getPropertyValue('aspect-ratio'), originalRatio, 'revision-only response cannot reuse old or default canvas geometry');
  frameAvailable = true; await tick();
  assert.strictEqual(find('video'), video);
  assert.strictEqual(canvas.style.height, '540px', 'immediate relaunch derives fresh layout');
  remote('ok').click(); await flush();
  assert.strictEqual(requests.filter(v => v.kind === 'input').at(-1).payload.sessionId, 'app_2');

  let finishInput;
  pendingInput = new Promise(resolve => { finishInput = resolve; });
  remote('ok').click(); await flush(); remote('green').click();
  const inputCount = requests.filter(v => v.kind === 'input').length;
  live.stop(); await flush();
  finishInput(session); pendingInput = null; await flush();
  assert.strictEqual(requests.filter(v => v.kind === 'input').length, inputCount, 'queued stale input never reaches another session');
  assert.strictEqual(video.style.getPropertyValue('aspect-ratio'), originalRatio);
  assert.strictEqual(video.style.getPropertyValue('z-index'), '');
  assert(observers.every(v => v.targets.length === 0), 'teardown releases layout observers');
  const oldWidth = canvas.style.width;
  width = 320; observer.fn();
  assert.strictEqual(canvas.style.width, oldWidth, 'late observer callback cannot revive a detached surface');
  assert.strictEqual(videos.length, 1, 'HbbTV never constructs a second video owner');
  console.log('HbbTV production input, media visibility, early layout, final resize and teardown ok');
}
run().catch(error => { console.error(error); process.exitCode = 1; });
