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
  for (let index = 0; index < (rounds || 12); index += 1) {
    promise = promise.then(() => Promise.resolve());
  }
  return promise;
}

(async function () {
  assert.ok(
    source.includes('const CONTINUOUS_BUFFER_FORWARD_SECONDS = 12;'),
    'continuous fMP4 must reuse the accepted 12-second browser buffer target as its forward high-water mark'
  );
  assert.ok(
    source.includes('function waitForContinuousBufferRoom('),
    'continuous fMP4 must have explicit pull backpressure before reading more HTTP bytes'
  );

  const init = box('moov', box('avcC', Uint8Array.from([1, 0x64, 0x00, 0x1f])));
  const fragment = concat(box('moof'), box('mdat'));
  const chunks = [concat(init, fragment), fragment, fragment, fragment, fragment];

  let readCalls = 0;
  let cancelCalls = 0;
  let legacyStartCalls = 0;
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
      // First append is the init segment. Every later append represents one
      // complete four-second fMP4 media fragment.
      if (this.appendCalls > 1) this.bufferedEnd += 4;
      this.dispatch('updateend');
    }
    remove(start, end) {
      if (Number(end) > Number(start) && end >= this.bufferedEnd) this.bufferedEnd = Number(start) || 0;
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
      createObjectURL() { return 'blob:bounded-continuous-fmp4'; },
      revokeObjectURL() {}
    },
    VdrSuitePublicUrl: {resolvePath(path) { return path; }},
    VdrSuiteBrowserSession: {
      subscribe() {},
      csrfHeaders() { return {'X-CSRF-Token': 'csrf-test'}; }
    },
    VdrSuiteRecordings2Playback: Object.freeze({
      createPanel() {
        return Object.freeze({
          element: node('section'),
          start() { legacyStartCalls += 1; return Promise.resolve('legacy-session'); },
          destroy() {},
          sessionId() { return 'legacy-session'; }
        });
      }
    }),
    VdrSuiteClientApi: {
      requestJson(path, options) {
        const body = JSON.parse(options.body);
        if (body.operation === 'stop') {
          return Promise.resolve({mediaSession: {id: body.sessionId, state: 'ended'}});
        }
        if (body.operation === 'playback-status') {
          return Promise.resolve({mediaSession: {
            id: body.sessionId,
            state: 'ready',
            recordingId: 'recording-backpressure',
            presentationProfileId: 'progressive-fmp4',
            mediaPath: '/api/media/sessions/' + body.sessionId + '/recording/stream.mp4',
            playback: {
              positionSeconds: 0,
              durationSeconds: 3600,
              seek: {supported: true, preparing: false, window: {startSeconds: 0, endSeconds: 3600}}
            }
          }});
        }
        return Promise.resolve({mediaSession: {
          id: 'recording_backpressure_session',
          state: 'ready',
          recordingId: body.recordingId,
          presentationProfileId: 'progressive-fmp4',
          mediaPath: '/api/media/sessions/recording_backpressure_session/recording/stream.mp4',
          playback: {
            positionSeconds: 0,
            durationSeconds: 3600,
            seek: {supported: true, preparing: false, window: {startSeconds: 0, endSeconds: 3600}}
          }
        }});
      }
    },
    fetch(path) {
      assert.ok(String(path).endsWith('/recording/stream.mp4'));
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

  const playback = window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-backpressure', title: 'MSE Backpressure'},
    'default'
  );
  assert.strictEqual(await playback.start(), 'recording_backpressure_session');
  await flush(24);

  assert.ok(activeSourceBuffer, 'continuous fMP4 must create a SourceBuffer');
  assert.ok(activeVideo, 'production Recording owner must own one video');
  assert.strictEqual(activeSourceBuffer.bufferedEnd, 12, 'three four-second fragments must fill the 12-second forward window');
  assert.strictEqual(
    readCalls,
    3,
    'ReadableStream reader must stop pulling once SourceBuffer reaches the forward high-water mark'
  );
  assert.strictEqual(legacyStartCalls, 0, 'bounded progressive playback must not enter HLS fallback');

  activeVideo.dispatch('playing');
  activeVideo.currentTime = 4;
  activeVideo.dispatch('timeupdate');
  await flush(24);

  assert.strictEqual(
    readCalls,
    4,
    'advancing playback below the high-water mark must release exactly the next HTTP pull'
  );
  assert.strictEqual(activeSourceBuffer.bufferedEnd, 16);
  assert.strictEqual(
    activeSourceBuffer.bufferedEnd - activeVideo.currentTime,
    12,
    'reader must re-enter backpressure after restoring the bounded forward window'
  );

  playback.destroy();
  await flush(8);
  assert.strictEqual(cancelCalls, 1, 'destroy must cancel the owned streaming reader while backpressure is waiting');
  assert.strictEqual(legacyStartCalls, 0);

  console.log('continuous fMP4 MSE forward-buffer backpressure contract ok');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
