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
const presentationRequests = [];
const videos = [];
let liveUpdateSource = null;
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
      createImageData(w, h) { return {width: w, height: h, data: new Uint8ClampedArray(w * h * 4)}; },
      clearRect(x = 0, y = 0, w = value.width || 0, h = value.height || 0) {
        if (!value.pixels || !(value.width > 0) || !(value.height > 0)) return;
        const left = Math.max(0, Math.trunc(x));
        const top = Math.max(0, Math.trunc(y));
        const right = Math.min(value.width, left + Math.max(0, Math.trunc(w)));
        const bottom = Math.min(value.height, top + Math.max(0, Math.trunc(h)));
        for (let row = top; row < bottom; ++row) {
          for (let column = left; column < right; ++column) {
            const offset = (row * value.width + column) * 4;
            value.pixels[offset] = 0;
            value.pixels[offset + 1] = 0;
            value.pixels[offset + 2] = 0;
            value.pixels[offset + 3] = 0;
          }
        }
      },
      putImageData(image, dx = 0, dy = 0) {
        if (!value.pixels ||
            value.pixels.length !== value.width * value.height * 4) {
          value.pixels = new Uint8ClampedArray(value.width * value.height * 4);
        }
        for (let row = 0; row < image.data.length / 4 / image.width; ++row) {
          for (let column = 0; column < image.width; ++column) {
            const source = (row * image.width + column) * 4;
            const target =
              ((dy + row) * value.width + (dx + column)) * 4;
            value.pixels[target] = image.data[source];
            value.pixels[target + 1] = image.data[source + 1];
            value.pixels[target + 2] = image.data[source + 2];
            value.pixels[target + 3] = image.data[source + 3];
          }
        }
      }
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
function qoiFullFrame() {
  // The pre-media application frame is opaque on purpose. Newer full resync
  // frames contain a transparent video hole plus one opaque control.
  const bytes = [113, 111, 105, 102, 0, 0, 0, 16, 0, 0, 0, 9, 4, 0];
  for (let i = 0; i < 144; ++i) {
    const alpha = frameRevision === 1 ? 255 : (i === 143 ? 255 : 0);
    bytes.push(255, 255, 255, 255, alpha);
  }
  bytes.push(0, 0, 0, 0, 0, 0, 0, 1);
  return new Uint8Array(bytes);
}
function qoiControlPatch() {
  // One opaque 1x1 RGBA pixel at the bottom-right control position.
  return new Uint8Array([
    113, 111, 105, 102,
    0, 0, 0, 1,
    0, 0, 0, 1,
    4, 0,
    255, 255, 255, 255, 255,
    0, 0, 0, 0, 0, 0, 0, 1
  ]);
}

const mount = node('div');
const document = {
  readyState: 'complete', hidden: false, visibilityState: 'visible',
  head: node('head'), createElement: node, addEventListener() {},
  getElementById(id) { return id === 'detail-data' ? mount : null; },
  querySelector() { return null; }, querySelectorAll() { return []; }
};
const client = {
  createClientLiveUpdateSource() {
    const listeners = Object.create(null);
    const source = {
      closed: false,
      onopen: null,
      addEventListener(type, handler) {
        (listeners[type] ||= []).push(handler);
      },
      close() { this.closed = true; },
      emit(type, value = {}) {
        const event = {data: JSON.stringify(value)};
        if (type === 'open' && typeof this.onopen === 'function') this.onopen(event);
        (listeners[type] || []).forEach(handler => handler(event));
      }
    };
    liveUpdateSource = source;
    return source;
  },
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
  fetchClientHbbtvPresentation({query}) {
    presentationRequests.push({query: {...query}});
    const knownRevision = Number(query && query.revision) || 0;
    if (!frameAvailable || knownRevision === frameRevision) {
      return Promise.resolve({status: 204, revision: frameRevision});
    }
    if (knownRevision > 0) {
      return Promise.resolve({
        status: 200,
        revision: frameRevision,
        width: 16,
        height: 9,
        frameKind: 'patch',
        baseRevision: knownRevision,
        patchX: 15,
        patchY: 8,
        patchWidth: 1,
        patchHeight: 1,
        bytes: qoiControlPatch()
      });
    }
    return Promise.resolve({
      status: 200,
      revision: frameRevision,
      width: 16,
      height: 9,
      frameKind: 'full',
      baseRevision: 0,
      patchX: 0,
      patchY: 0,
      patchWidth: 16,
      patchHeight: 9,
      bytes: qoiFullFrame()
    });
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
  let canvas = find('.vdr-suite-hbbtv-overlay');
  assert.strictEqual(canvas.style.height, '360px', 'application viewport must not inherit the 150px metadata-less video height');
  assert.strictEqual(canvas.style.width, '640px');

  assert.strictEqual(canvas.pixels[3], 255, 'pre-media application frame is deliberately opaque');
  mediaActive = true; await tick();
  assert(video.src.includes('hbbtv_media_test'), 'actual canonical owner attached the external stream');
  assert.strictEqual(video.videoWidth, 0, 'external attach reset intrinsic metadata again');
  assert.strictEqual(canvas.style.height, '360px');
  assert.strictEqual(canvas.pixels[3], 0, 'media attach clears stale application pixels over the video rectangle');
  const canvasZ = Number(liveSource.match(/\.vdr-suite-hbbtv-overlay\{[^}]*z-index:(\d+)/)[1]);
  assert(Number(video.style.getPropertyValue('z-index')) < canvasZ, 'broadband video must remain BELOW the HbbTV application plane');
  const readsBeforeInput = presentationRequests.length;
  remote('ok').click(); await flush();
  assert.deepStrictEqual({...requests.find(v => v.kind === 'input').payload},
    {backendId: 'default', sessionId: 'app_1', action: 'ok'});
  assert.strictEqual(presentationRequests.length, readsBeforeInput,
    'input acknowledgement is not a frame signal; no premature readback');
  assert(find('.vdr-suite-hbbtv-input-diagnostics').textContent.includes('Bild …'),
    'missing frame latency must remain pending rather than being shown as 0 ms');
  const firstEventSource = liveUpdateSource;
  firstEventSource.emit('update', {
    backendId: 'default', sequenceNumber: 1,
    changedDomains: ['hbbtvPresentation']
  });
  await flush();
  assert.strictEqual(canvas.pixels[3], 0, 'fresh provider patch keeps the video hole transparent');
  assert.strictEqual(canvas.pixels[143 * 4 + 3], 255, 'fresh provider patch paints an opaque control pixel above video');
  assert.strictEqual(canvas.dataset.hbbtvRevision, '2', 'patch advances the canvas revision without replacing the full surface');
  const readsAfterFirstEvent = presentationRequests.length;
  firstEventSource.emit('update', {
    backendId: 'default', sequenceNumber: 1,
    changedDomains: ['hbbtvPresentation']
  });
  await flush();
  assert.strictEqual(presentationRequests.length, readsAfterFirstEvent,
    'duplicate SSE sequence is ignored');
  assert(Number(video.style.getPropertyValue('z-index')) < canvasZ, 'native controls stay above the visible broadband video');
  assert.strictEqual(video.style.getPropertyValue('pointer-events'), 'none');
  for (const key of ['Enter', 'g', 'r', 'ArrowLeft']) {
    let prevented = false;
    canvas.dispatch('keydown', {key, preventDefault() { prevented = true; }});
    await flush(); assert(prevented);
  }
  assert.deepStrictEqual(requests.filter(v => v.kind === 'input').map(v => v.payload.action),
    ['ok', 'ok', 'green', 'red', 'left']);
  firstEventSource.emit('update', {
    backendId: 'default', sequenceNumber: 2,
    changedDomains: ['hbbtvPresentation']
  });
  await flush();
  assert.strictEqual(canvas.dataset.hbbtvRevision, '6',
    'one push readback coalesces rapid input-driven frame revisions');

  // A reconnect while a patch request is pending invalidates that request.
  // Its late success must not replace the revision-0 full-frame resync.
  const originalRead = client.fetchClientHbbtvPresentation;
  let finishOldRead;
  client.fetchClientHbbtvPresentation = function({query}) {
    presentationRequests.push({query: {...query}});
    return new Promise(resolve => {
      finishOldRead = () => resolve({
        status: 200, revision: 7, width: 16, height: 9,
        frameKind: 'patch', baseRevision: 6,
        patchX: 15, patchY: 8, patchWidth: 1, patchHeight: 1,
        bytes: qoiControlPatch()
      });
    });
  };
  firstEventSource.emit('update', {
    backendId: 'default', sequenceNumber: 3,
    changedDomains: ['hbbtvPresentation']
  });
  await flush();
  assert.strictEqual(typeof finishOldRead, 'function',
    'an incremental frame read must be in flight before SSE reconnect');
  assert.strictEqual(presentationRequests.at(-1).query.revision, '6');
  firstEventSource.emit('resync', {reason: 'reconnect'});
  client.fetchClientHbbtvPresentation = originalRead;
  frameRevision = 8;
  finishOldRead();
  await flush();
  assert.strictEqual(presentationRequests.at(-1).query.revision, '0',
    'SSE resync must force a complete read after discarding the stale patch');
  assert.strictEqual(canvas.dataset.hbbtvRevision, '8',
    'a late pre-reconnect patch must never overwrite the resynchronized canvas');
  assert.strictEqual(canvas.pixels[3], 0,
    'resynchronized full frame must preserve the broadband video hole');

  // A final container layout arrives independently of frame events and input.
  width = 960;
  const observer = observers.find(v => v.targets.includes(video));
  assert(observer, 'the canonical video/container layout must be observed');
  observer.fn();
  assert.strictEqual(canvas.style.width, '960px');
  assert.strictEqual(canvas.style.height, '540px');
  const mediaRequestsBeforeRefresh = requests.filter(v => v.kind === 'media').length;
  const frameReadsBeforeRefresh = presentationRequests.length;
  // Simulate an outer view refresh removing only the replaceable HbbTV canvas.
  // The canonical video/media owner must stay mounted while the overlay recovers.
  const previousCanvas = canvas;
  previousCanvas.parentNode.children =
    previousCanvas.parentNode.children.filter(value => value !== previousCanvas);
  previousCanvas.parentNode = null;
  live.__test.applyPrograms({}); live.__test.render();
  await flush();
  assert.strictEqual(find('video'), video);
  assert.strictEqual(video.parentNode, owner);
  assert.strictEqual(requests.filter(v => v.kind === 'media').length, mediaRequestsBeforeRefresh,
    'unrelated presentation refresh must not stop/restart Live');
  canvas = find('.vdr-suite-hbbtv-overlay');
  assert(canvas && canvas !== previousCanvas, 'missing HbbTV canvas is recreated without replacing the player');
  assert.strictEqual(canvas.dataset.hbbtvRevision, '8',
    'recreated canvas is synchronized from a complete current frame');
  assert.strictEqual(canvas.pixels[143 * 4 + 3], 255,
    'full readback restores the opaque application control');
  assert(presentationRequests.length > frameReadsBeforeRefresh,
    'recreated canvas performs one full frame readback');

  find('.vdr-suite-hbbtv-session-toggle').click(); await flush(); await tick();
  assert.strictEqual(firstEventSource.closed, true, 'confirmed session close releases the SSE rail');
  assert.strictEqual(video.style.getPropertyValue('aspect-ratio'), originalRatio, 'close restores the broadcast layout');
  assert.strictEqual(video.style.getPropertyValue('z-index'), '');
  assert(video.src.includes('live_session_test'), 'close restores broadcast through the same owner');
  frameRevision = 1; mediaActive = false; frameAvailable = false;
  find('.vdr-suite-hbbtv-session-toggle').click(); await flush(); await tick();
  assert.strictEqual(live.snapshot().hbbtvSessionId, 'app_2');
  assert.strictEqual(live.snapshot().hbbtvFrameRevision, 0,
    'revision-only response before a rendered frame must not advance the consumer');
  assert.strictEqual(video.style.getPropertyValue('aspect-ratio'), originalRatio, 'revision-only response cannot reuse old or default canvas geometry');
  const secondEventSource = liveUpdateSource;
  const readsBeforeLateFrame = presentationRequests.length;
  frameAvailable = true; await tick();
  assert.strictEqual(presentationRequests.length, readsBeforeLateFrame,
    'no presentation retry timer runs while waiting for the next provider event');
  secondEventSource.emit('resync', {reason: 'reconnect'});
  await flush();
  assert.strictEqual(Number(presentationRequests.at(-1).query.revision), 0,
    'SSE resync must request complete pixels after an invalid revision-only response');
  assert.strictEqual(live.snapshot().hbbtvFrameRevision, 1,
    'full-frame SSE recovery restores the painted revision');
  assert.strictEqual(find('video'), video);
  assert.strictEqual(canvas.style.height, '540px', 'SSE resync fetches a fresh full frame after reconnect');
  remote('ok').click(); await flush();
  assert.strictEqual(requests.filter(v => v.kind === 'input').at(-1).payload.sessionId, 'app_2');

  let finishInput;
  pendingInput = new Promise(resolve => { finishInput = resolve; });
  remote('ok').click(); await flush(); remote('green').click();
  const inputCount = requests.filter(v => v.kind === 'input').length;
  live.stop(); await flush();
  assert.strictEqual(secondEventSource.closed, true, 'view teardown releases its SSE rail');
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
