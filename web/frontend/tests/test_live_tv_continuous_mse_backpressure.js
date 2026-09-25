'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const sourcePath = 'web/frontend/api/session-frontend-sync.js';
const source = fs.readFileSync(sourcePath, 'utf8');

function bytes() {
  const values = [];
  for (let index = 0; index < arguments.length; index += 1) {
    const value = arguments[index];
    if (value instanceof Uint8Array) values.push(...value);
    else values.push(value);
  }
  return Uint8Array.from(values);
}

function uint32(value) {
  return Uint8Array.from([
    (value >>> 24) & 0xff,
    (value >>> 16) & 0xff,
    (value >>> 8) & 0xff,
    value & 0xff
  ]);
}

function ascii(value) {
  return Uint8Array.from(String(value).split('').map(character => character.charCodeAt(0)));
}

function box(type, payload) {
  const body = payload instanceof Uint8Array ? payload : new Uint8Array(0);
  return bytes(uint32(body.byteLength + 8), ascii(type), body);
}

function concat() {
  let length = 0;
  const values = Array.from(arguments);
  values.forEach(value => { length += value.byteLength; });
  const result = new Uint8Array(length);
  let offset = 0;
  values.forEach(value => {
    result.set(value, offset);
    offset += value.byteLength;
  });
  return result;
}

function node(tagName) {
  const listeners = {};
  const value = {
    tagName: String(tagName || '').toUpperCase(),
    children: [],
    className: '',
    classList: {toggle() {}, add() {}, remove() {}},
    style: {},
    dataset: {},
    textContent: '',
    hidden: false,
    disabled: false,
    controls: false,
    autoplay: false,
    playsInline: false,
    preload: '',
    src: '',
    currentTime: 0,
    paused: true,
    value: '',
    type: '',
    min: '',
    max: '',
    step: '',
    error: null,
    parentNode: null,
    appendChild(child) {
      child.parentNode = this;
      this.children.push(child);
      return child;
    },
    setAttribute(name, val) { this[name] = String(val); },
    removeAttribute(name) { if (name === 'src') this.src = ''; },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    removeEventListener(name, callback) {
      if (!listeners[name]) return;
      listeners[name] = listeners[name].filter(value => value !== callback);
    },
    dispatch(name, event) {
      (listeners[name] || []).slice().forEach(callback => callback(event || {target: this}));
    },
    pause() {
      this.paused = true;
      this.dispatch('pause', {target: this});
    },
    load() { this.loaded = (this.loaded || 0) + 1; },
    play() {
      this.paused = false;
      this.played = (this.played || 0) + 1;
      this.dispatch('play', {target: this});
      return Promise.resolve();
    },
    replaceWith(replacement) { this.replacement = replacement; }
  };
  return value;
}

function flush(rounds) {
  let promise = Promise.resolve();
  for (let index = 0; index < (rounds || 24); index += 1) {
    promise = promise.then(() => Promise.resolve());
  }
  return promise;
}

(async function () {
  const init = box('moov', box('avcC', Uint8Array.from([1, 0x64, 0x00, 0x1f])));
  const fragment = concat(box('moof'), box('mdat'));
  const chunks = [
    concat(init, fragment),
    fragment,
    fragment,
    fragment,
    fragment
  ];

  let readCalls = 0;
  let cancelCalls = 0;
  let activeSourceBuffer = null;
  let activeVideo = null;

  const reader = {
    read() {
      readCalls += 1;
      const index = readCalls - 1;
      if (index < chunks.length) {
        return Promise.resolve({done: false, value: chunks[index]});
      }
      return new Promise(function () {});
    },
    cancel() {
      cancelCalls += 1;
      return Promise.resolve();
    }
  };

  class FakeSourceBuffer {
    constructor() {
      this.mode = 'segments';
      this.updating = false;
      this.appendCalls = 0;
      this.bufferedEnd = 0;
      this.listeners = {};
      const self = this;
      this.buffered = {
        get length() { return self.bufferedEnd > 0 ? 1 : 0; },
        start() { return 0; },
        end() { return self.bufferedEnd; }
      };
    }
    addEventListener(name, callback) {
      if (!this.listeners[name]) this.listeners[name] = [];
      this.listeners[name].push(callback);
    }
    removeEventListener(name, callback) {
      if (!this.listeners[name]) return;
      this.listeners[name] = this.listeners[name].filter(value => value !== callback);
    }
    dispatch(name) {
      (this.listeners[name] || []).slice().forEach(callback => callback());
    }
    appendBuffer() {
      this.appendCalls += 1;
      if (this.appendCalls > 1) this.bufferedEnd += 4;
      this.dispatch('updateend');
    }
    remove(start, end) {
      if (Number(end) > Number(start) && end >= this.bufferedEnd) {
        this.bufferedEnd = Number(start) || 0;
      }
      this.dispatch('updateend');
    }
  }

  class FakeMediaSource {
    constructor() {
      this.readyState = 'open';
    }
    static isTypeSupported() { return true; }
    addEventListener() {}
    removeEventListener() {}
    addSourceBuffer() {
      activeSourceBuffer = new FakeSourceBuffer();
      return activeSourceBuffer;
    }
    endOfStream() { this.ended = true; }
  }

  class FakeAbortController {
    constructor() { this.signal = {}; }
    abort() { this.aborted = true; }
  }

  const document = {
    readyState: 'complete',
    visibilityState: 'visible',
    createElement(tagName) {
      const value = node(tagName);
      if (value.tagName === 'VIDEO') activeVideo = value;
      return value;
    },
    addEventListener() {}
  };

  const requests = [];
  const window = {
    console,
    document,
    performance: {now() { return 1000; }},
    Date,
    setTimeout,
    clearTimeout,
    MediaSource: FakeMediaSource,
    AbortController: FakeAbortController,
    ReadableStream: function ReadableStream() {},
    URL: {
      createObjectURL() { return 'blob:live-mse-backpressure'; },
      revokeObjectURL() {}
    },
    VdrSuitePublicUrl: {
      resolvePath(path) { return path; }
    },
    VdrSuiteBrowserSession: {
      restore() { return Promise.resolve({authenticated: true}); },
      subscribe() {},
      csrfHeaders() { return {'X-CSRF-Token': 'csrf-live-backpressure'}; }
    },
    VdrSuiteRecordings2Playback: Object.freeze({
      createPanel() {
        return Object.freeze({
          element: node('section'),
          start() { return Promise.resolve('legacy-session'); },
          destroy() {},
          sessionId() { return 'legacy-session'; }
        });
      }
    }),
    VdrSuiteClientApi: {
      requestJson(path, options) {
        requests.push({path, options});
        const body = JSON.parse(options.body);
        if (body.operation === 'stop') {
          return Promise.resolve({mediaSession: {id: body.sessionId, state: 'ended'}});
        }
        assert.strictEqual(body.resourceKind, 'live-channel');
        return Promise.resolve({
          mediaSession: {
            id: 'live_backpressure_session',
            resourceKind: 'live-channel',
            state: 'ready',
            presentationProfileId: 'live-progressive-fmp4',
            mediaPath: '/api/media/sessions/live_backpressure_session/live/stream.mp4'
          }
        });
      }
    },
    fetch(path) {
      assert.strictEqual(
        String(path),
        '/api/media/sessions/live_backpressure_session/live/stream.mp4'
      );
      return Promise.resolve({
        ok: true,
        status: 200,
        body: {getReader() { return reader; }}
      });
    },
    addEventListener() {},
    removeEventListener() {}
  };
  window.window = window;

  const context = vm.createContext({
    window,
    document,
    console,
    Object,
    String,
    Number,
    Array,
    Boolean,
    Promise,
    RegExp,
    Error,
    JSON,
    Math,
    Uint8Array,
    Date,
    setTimeout,
    clearTimeout
  });
  vm.runInContext(source, context, {filename: sourcePath});

  const playback = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'C-1-1051-10301', name: 'Das Erste HD'},
    'default',
    {}
  );

  assert.strictEqual(await playback.start(), 'live_backpressure_session');
  await flush(48);

  assert.ok(activeSourceBuffer, 'Live-TV MSE must create one SourceBuffer');
  assert.ok(activeVideo, 'Live-TV must own one HTMLMediaElement');
  assert.strictEqual(
    activeSourceBuffer.bufferedEnd,
    12,
    'reproduction must reach the shared 12-second continuous-MSE high-water mark'
  );

  assert.ok(
    readCalls > 3,
    'Live-TV must keep draining its real-time Gateway stream after 12 seconds; ' +
      'otherwise browser forward-buffer backpressure propagates through FIFO/FFmpeg ' +
      'into the bounded SuiteBridge provider and can terminate as backpressure_overflow'
  );

  playback.destroy();
  await flush(8);
  assert.strictEqual(cancelCalls, 1);

  console.log('Live-TV continuous MSE must not deadlock its bounded provider at the Recording high-water mark');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
