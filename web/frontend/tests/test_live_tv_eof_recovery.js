'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const source = fs.readFileSync('web/frontend/api/session-frontend-sync.js', 'utf8');
const pendingTimers = new Map();
const pendingReads = [];
const requests = [];
const readers = [];
const listeners = {};
let timerSequence = 0;
let sessionSequence = 0;
let failCreates = 0;
let fakeNow = 1000000;
class TestDate extends Date {
  static now() {return fakeNow;}
}

function bytes() {
  const result = [];
  Array.from(arguments).forEach(function (part) {
    if (part instanceof Uint8Array) result.push(...part);
    else result.push(part);
  });
  return Uint8Array.from(result);
}
function box(type, body) {
  return bytes(0, 0, 0, body.length + 8,
    ...type.split('').map(letter => letter.charCodeAt(0)), body);
}
const init = box('moov', box('avcC', bytes(1, 0x64, 0, 0x1f)));
function node(tag) {
  const events = {};
  return {
    tagName: tag.toUpperCase(), children: [], style: {}, hidden: false,
    paused: false, currentTime: 0, classList: {toggle() {}},
    appendChild(child) {this.children.push(child); return child;},
    setAttribute() {}, removeAttribute(key) {if (key === 'src') this.src = '';},
    addEventListener(key, fn) {(events[key] ||= []).push(fn);},
    removeEventListener(key, fn) {events[key] = (events[key] || []).filter(item => item !== fn);},
    dispatch(key) {(events[key] || []).slice().forEach(fn => fn({target: this}));},
    pause() {this.paused = true;}, load() {},
    play() {this.paused = false; return Promise.resolve();}
  };
}
class Buffer {
  constructor() {
    this.updating = false;
    this.events = {};
    this.buffered = {length: 0};
  }
  addEventListener(key, cb) {(this.events[key] ||= []).push(cb);}
  removeEventListener(key, cb) {this.events[key] = (this.events[key] || []).filter(fn => fn !== cb);}
  appendBuffer() {(this.events.updateend || []).slice().forEach(fn => fn());}
  remove() {(this.events.updateend || []).slice().forEach(fn => fn());}
}
class MediaSource {
  constructor() {this.readyState = 'open';}
  static isTypeSupported() {return true;}
  addEventListener() {}
  removeEventListener() {}
  addSourceBuffer() {return new Buffer();}
  endOfStream() {this.ended = true;}
}
class AbortController {
  constructor() {this.signal = {};}
  abort() {this.aborted = true;}
}
const document = {
  readyState: 'complete', visibilityState: 'visible',
  createElement: node,
  addEventListener(key, cb) {(listeners[key] ||= []).push(cb);},
  removeEventListener(key, cb) {listeners[key] = (listeners[key] || []).filter(fn => fn !== cb);}
};
const window = {
  document, console, Date: TestDate, MediaSource, AbortController,
  ReadableStream: function ReadableStream() {},
  performance: {now() {return 1000;}},
  setTimeout(fn, delay) {const id = ++timerSequence; pendingTimers.set(id, {fn, delay}); return id;},
  clearTimeout(id) {pendingTimers.delete(id);},
  URL: {createObjectURL() {return 'blob:live-recovery';}, revokeObjectURL() {}},
  VdrSuitePublicUrl: {resolvePath(path) {return path;}},
  VdrSuiteBrowserSession: {
    restore() {return Promise.resolve({authenticated: true});},
    subscribe() {}, csrfHeaders() {return {'X-CSRF-Token': 'test-csrf'};}
  },
  VdrSuiteClientApi: {
    requestJson(path, options) {
      const body = JSON.parse(options.body);
      requests.push(body);
      if (body.operation === 'stop') return Promise.resolve({mediaSession: {state: 'ended'}});
      if (failCreates > 0) {failCreates -= 1; return Promise.reject(new Error('live_worker_exited'));}
      const id = 'live-' + (++sessionSequence);
      return Promise.resolve({mediaSession: {
        id, state: 'ready', mediaPath: '/api/media/sessions/' + id + '/live/stream.mp4'
      }});
    }
  },
  fetch(path) {
    assert.ok(/^\/api\/media\/sessions\/live-\d+\/live\/stream\.mp4$/.test(path));
    const reader = {
      delivered: false, cancelled: false,
      read() {
        if (!this.delivered) {
          this.delivered = true;
          return Promise.resolve({done: false, value: init});
        }
        return new Promise(resolve => pendingReads.push({reader: this, resolve}));
      },
      cancel() {this.cancelled = true; return Promise.resolve();}
    };
    readers.push(reader);
    return Promise.resolve({ok: true, status: 200, body: {getReader() {return reader;}}});
  },
  addEventListener(key, cb) {(listeners[key] ||= []).push(cb);},
  removeEventListener(key, cb) {listeners[key] = (listeners[key] || []).filter(fn => fn !== cb);}
};
window.window = window;
vm.runInContext(source, vm.createContext({
  window, document, console, Object, String, Number, Array, Boolean, Promise,
  RegExp, Error, JSON, Math, Uint8Array, Date: TestDate, setTimeout: window.setTimeout,
  clearTimeout: window.clearTimeout
}), {filename: 'session-frontend-sync.js'});

async function flush() {
  for (let i = 0; i < 70; ++i) await Promise.resolve();
}
function eof(reader) {
  const pending = pendingReads.find(entry => entry.reader === reader);
  assert.ok(pending, 'stream must have a pending browser reader');
  pendingReads.splice(pendingReads.indexOf(pending), 1);
  pending.resolve({done: true});
}
function runTimer(delay) {
  const found = Array.from(pendingTimers.entries()).find(([id, item]) => item.delay === delay);
  assert.ok(found, 'expected recovery timer ' + delay);
  pendingTimers.delete(found[0]);
  found[1].fn();
}
function dispatch(name, event) {(listeners[name] || []).slice().forEach(fn => fn(event || {}));}

(async function () {
  const playback = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'C-1-1051-10301', name: 'Live Test'}, 'default', {}
  );
  assert.strictEqual(await playback.start(), 'live-1');
  await flush();
  assert.strictEqual(readers.length, 1);
  const video = playback.element.children[2];
  video.dispatch('playing');
  assert.strictEqual(playback.element.children[1].textContent, 'Live-TV läuft.');

  eof(readers[0]);
  await flush();
  assert.ok(playback.element.children[1].textContent.includes('Wiederverbindung'));
  assert.ok(!playback.element.children[1].textContent.includes('läuft'));
  dispatch('pagehide', {persisted: true});
  assert.strictEqual(requests.filter(request => request.operation === 'stop').length, 0,
    'BFCache pagehide must not destroy an active session');

  runTimer(1000);
  await flush();
  assert.strictEqual(playback.sessionId(), 'live-2');
  assert.strictEqual(requests[1].replacesSessionId, 'live-1');
  assert.strictEqual(requests.filter(request => request.operation === 'stop').length, 0,
    'server replaces old worker; browser must not STOP it after replacement');
  assert.ok(readers[0].cancelled, 'old stream reader must be cancelled');

  // A short burst of decoded frames is not enough to reset the budget:
  // repeated one-second crashes must not cause an infinite restart storm.
  video.currentTime = 1;
  video.dispatch('timeupdate');
  eof(readers[1]);
  await flush();
  assert.ok(Array.from(pendingTimers.values()).some(timer => timer.delay === 2000));
  playback.destroy();
  runPendingTimers();
  await flush();
  assert.strictEqual(sessionSequence, 2, 'manual stop must fence delayed recovery');
  assert.strictEqual(requests.filter(request => request.operation === 'stop' &&
    request.sessionId === 'live-2').length, 1);

  const failed = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'C-1-1051-10301'}, 'default', {}
  );
  assert.strictEqual(await failed.start(), 'live-3');
  await flush();
  failCreates = 3;
  eof(readers[2]);
  await flush();
  for (const delay of [1000, 2000, 4000]) {
    runTimer(delay);
    await flush();
  }
  assert.strictEqual(requests.filter(request => !request.operation &&
    request.replacesSessionId === 'live-3').length, 3);
  assert.ok(failed.element.children[1].textContent.includes('Live-TV beendet'));
  assert.strictEqual(pendingTimers.size, 0, 'retries must be bounded');
  failed.destroy();

  const hidden = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'C-1-1051-10301'}, 'default', {}
  );
  assert.strictEqual(await hidden.start(), 'live-4');
  await flush();
  const before = sessionSequence;
  document.visibilityState = 'hidden';
  fakeNow += 30000;
  runTimer(5000);
  assert.strictEqual(sessionSequence, before,
    'background suspension must not be mistaken for a permanent STOP');
  document.visibilityState = 'visible';
  dispatch('visibilitychange');
  fakeNow += 6000;
  runTimer(5000);
  await flush();
  assert.ok(hidden.element.children[1].textContent.includes('Wiederverbindung'),
    'a resumed tab with no frame progress must eventually attempt recovery');
  hidden.destroy();
  console.log('Live-TV EOF recovery, BFCache, resumed-tab stall, ownership and retry budget: PASS');
})().catch(error => {console.error(error); process.exitCode = 1;});

function runPendingTimers() {
  for (const [id, item] of Array.from(pendingTimers.entries())) {
    pendingTimers.delete(id);
    if (item.delay <= 4000) item.fn();
  }
}
