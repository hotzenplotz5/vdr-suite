'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const lifecycleSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-owner-lifecycle.js'),
  'utf8'
);
const trackSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-track-controls.js'),
  'utf8'
);

function descendants(root) {
  const values = [];
  (function walk(item) {
    if (!item) return;
    values.push(item);
    (item.children || []).forEach(walk);
  }(root));
  return values;
}

function node(tagName) {
  const listeners = {};
  const value = {
    tagName: String(tagName || '').toUpperCase(),
    children: [],
    className: '',
    classList: {toggle(name, enabled) { this[name] = Boolean(enabled); }},
    style: {},
    textContent: '',
    hidden: false,
    disabled: false,
    value: '',
    selected: false,
    parentNode: null,
    currentTime: 0,
    appendChild(child) {
      if (child.parentNode && typeof child.parentNode.removeChild === 'function') {
        child.parentNode.removeChild(child);
      }
      child.parentNode = this;
      this.children.push(child);
      if (this.tagName === 'SELECT' && child.selected) this.value = child.value;
      return child;
    },
    replaceChildren() {
      this.children.forEach(child => { child.parentNode = null; });
      this.children = [];
      if (this.tagName === 'SELECT') this.value = '';
    },
    removeChild(child) {
      const index = this.children.indexOf(child);
      if (index >= 0) this.children.splice(index, 1);
      child.parentNode = null;
      return child;
    },
    querySelector(selector) {
      if (selector === 'video') {
        return descendants(this).find(item => item.tagName === 'VIDEO') || null;
      }
      if (selector === '.recordings2-recording-fallback-shell') {
        return descendants(this).find(item =>
          String(item.className || '').split(/\s+/).includes('recordings2-recording-fallback-shell')
        ) || null;
      }
      return null;
    },
    setAttribute(name, val) { this[name] = String(val); },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    dispatch(name) {
      (listeners[name] || []).forEach(callback => callback({target: this}));
    }
  };
  if (value.tagName === 'TRACK') value.track = {mode: 'disabled'};
  Object.defineProperty(value, 'firstChild', {
    get() { return this.children.length ? this.children[0] : null; }
  });
  return value;
}

function find(root, predicate) {
  return descendants(root).find(predicate);
}

function flush(count = 12) {
  let result = Promise.resolve();
  for (let index = 0; index < count; index += 1) result = result.then(() => Promise.resolve());
  return result;
}

function deferred() {
  let resolve;
  let reject;
  const promise = new Promise((res, rej) => {
    resolve = res;
    reject = rej;
  });
  return {promise, resolve, reject};
}

function subtitleTrackStatus(sessionId) {
  return {
    mediaSession: {
      id: sessionId,
      state: 'ready',
      presentationProfileId: 'hls-fmp4',
      tracks: {
        audio: {
          selectionSupported: false,
          selectionReason: null,
          selectedTrackId: null,
          defaultTrackId: null,
          availableTracks: []
        },
        subtitles: {
          selectionSupported: true,
          selectionReason: null,
          offSupported: true,
          offSelected: true,
          availableTracks: [
            {
              id: 'subtitle-1',
              language: 'deu',
              label: 'Deutsch',
              roles: [],
              default: false,
              selectable: true,
              deliveryFormat: 'webvtt'
            }
          ],
          selectedTrackId: null,
          defaultTrackId: null
        }
      }
    }
  };
}

(async function () {
  let timerCalls = 0;
  function setTimeoutFake() {
    timerCalls += 1;
    return timerCalls;
  }
  function clearTimeoutFake() {}

  let assigned = {};
  let activeSessionId = '';
  let hlsAbsolutePosition = 37;
  const subtitleRequests = [];
  const subtitleResponses = [];

  const fallbackElement = node('section');
  fallbackElement.className = 'recordings2-recording-fallback-shell';
  const video = node('video');
  video.currentTime = 7;
  fallbackElement.appendChild(video);
  const fallbackOwner = Object.freeze({
    position() { return hlsAbsolutePosition; },
    state() { return 'playing'; },
    selectAudioTrack() { return Promise.reject(new Error('audio not used')); },
    stop() { return Promise.resolve(true); }
  });
  fallbackElement.__vdrSuiteRecordingFallbackOwner = fallbackOwner;

  const document = {createElement: node};
  const urlState = {created: [], revoked: []};
  class BlobFake {
    constructor(parts, options) {
      this.parts = parts;
      this.type = options && options.type;
    }
  }

  const window = {
    document,
    console,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake,
    Blob: BlobFake,
    URL: {
      createObjectURL(blob) {
        const value = 'blob:subtitle-' + String(urlState.created.length + 1);
        urlState.created.push({value, blob});
        return value;
      },
      revokeObjectURL(value) { urlState.revoked.push(value); }
    },
    VdrSuiteBrowserSession: {csrfHeaders() { return {'X-CSRF-Token': 'csrf-subtitle'}; }},
    VdrSuiteClientApi: {
      requestJson(requestPath, options) {
        assert.strictEqual(requestPath, '/api/media/sessions');
        const body = JSON.parse(options.body);
        assert.strictEqual(body.operation, 'track-status');
        return Promise.resolve(subtitleTrackStatus(body.sessionId));
      }
    },
    fetch(requestPath, options) {
      assert.strictEqual(requestPath, '/api/media/sessions');
      const body = JSON.parse(options.body);
      assert.strictEqual(body.operation, 'select-subtitle-track');
      subtitleRequests.push(body);
      const pending = deferred();
      subtitleResponses.push(pending);
      return pending.promise;
    }
  };
  window.window = window;
  Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
    configurable: true,
    enumerable: true,
    get() { return assigned; },
    set(value) { assigned = value; }
  });

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
    Date,
    Set,
    Blob: BlobFake,
    URL: window.URL,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake
  });
  vm.runInContext(lifecycleSource, context, {filename: 'playback-owner-lifecycle.js'});

  const lifecycle = window.VdrSuitePlaybackOwnerLifecycle.create({
    state: 'idle',
    sessionId: null,
    transport: 'hls-compatibility'
  });

  const basePanel = Object.freeze({
    element: fallbackElement,
    start() { return Promise.resolve(activeSessionId); },
    sessionId() { return activeSessionId; },
    position() { return hlsAbsolutePosition; },
    state() { return activeSessionId ? 'fallback' : 'idle'; },
    snapshot() { return lifecycle.snapshot(); },
    subscribe(callback) { return lifecycle.subscribe(callback); },
    seekAbsolute() { return Promise.reject(new Error('HLS seek not used')); },
    stop() { return Promise.resolve(true); },
    destroy() {},
    relinquishForReplacement() { return Promise.resolve(activeSessionId); }
  });

  vm.runInContext(trackSource, context, {filename: 'recording-track-controls.js'});
  window.VdrSuiteRecordings2Playback = Object.freeze({createPanel() { return basePanel; }});
  const playback = window.VdrSuiteRecordings2Playback.createPanel({id: 'quiet-place-2'}, 'default');

  activeSessionId = 'hls-subtitle-session';
  lifecycle.publish({
    state: 'playing',
    sessionId: activeSessionId,
    transport: 'hls-compatibility',
    transition: 'session-replaced'
  });
  await flush();

  assert.strictEqual(timerCalls, 0, 'canonical lifecycle must not start the legacy session watcher');

  const subtitleSelect = find(
    playback.element,
    item => item.tagName === 'SELECT' && item['aria-label'] === 'Untertitel auswählen'
  );
  assert.ok(subtitleSelect, 'subtitle selector must be present on canonical HLS lifecycle');
  assert.strictEqual(subtitleSelect.disabled, false);

  subtitleSelect.value = 'subtitle-1';
  subtitleSelect.dispatch('change');
  await flush();

  assert.strictEqual(subtitleRequests.length, 1, 'user selection must request the first WebVTT binding');
  assert.strictEqual(subtitleRequests[0].streamBasePositionSeconds, 30);

  // Model a presentation-base drift while the first WebVTT response is in
  // flight. The retired timer watcher used to recover this condition. The
  // canonical lifecycle path must now recover immediately without polling.
  hlsAbsolutePosition = 39;
  subtitleResponses[0].resolve({
    ok: true,
    headers: {get(name) { return name === 'Content-Type' ? 'text/vtt; charset=utf-8' : ''; }},
    text() { return Promise.resolve('WEBVTT\n\n00:00:01.000 --> 00:00:03.000\nHallo\n'); }
  });
  await flush(20);

  assert.strictEqual(subtitleRequests.length, 2, 'stale same-session WebVTT response must trigger one bounded rebind');
  assert.strictEqual(subtitleRequests[1].streamBasePositionSeconds, 32, 'rebind must use the current presentation base');
  assert.strictEqual(timerCalls, 0, 'subtitle recovery must remain event-driven and timer-free');

  subtitleResponses[1].resolve({
    ok: true,
    headers: {get(name) { return name === 'Content-Type' ? 'text/vtt; charset=utf-8' : ''; }},
    text() { return Promise.resolve('WEBVTT\n\n00:00:01.000 --> 00:00:03.000\nHallo erneut\n'); }
  });
  await flush(20);

  const mountedTrack = video.children.find(item => item.tagName === 'TRACK');
  assert.ok(mountedTrack, 'successful retry must mount a browser text track on the active HLS video');
  assert.strictEqual(mountedTrack.parentNode, video);
  assert.strictEqual(mountedTrack.track.mode, 'showing');
  assert.strictEqual(mountedTrack.kind, 'subtitles');
  assert.strictEqual(mountedTrack.srclang, 'de');
  assert.strictEqual(subtitleSelect.value, 'subtitle-1');
  assert.strictEqual(urlState.created.length, 1, 'only the accepted WebVTT response should create a Blob URL');
  assert.strictEqual(timerCalls, 0, 'no legacy watcher may be reintroduced after mounting');

  playback.destroy();
  console.log('phase65d subtitle canonical lifecycle rebind ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
