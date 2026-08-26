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
    replaceWith(next) {
      if (!this.parentNode) return;
      const parent = this.parentNode;
      const index = parent.children.indexOf(this);
      if (index < 0) return;
      if (next.parentNode && typeof next.parentNode.removeChild === 'function') {
        next.parentNode.removeChild(next);
      }
      parent.children[index] = next;
      next.parentNode = parent;
      this.parentNode = null;
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
  Object.defineProperty(value, 'firstChild', {
    get() { return this.children.length ? this.children[0] : null; }
  });
  return value;
}

function find(root, predicate) {
  return descendants(root).find(predicate);
}

function tracks(selectedTrackId) {
  return {
    audio: {
      selectionSupported: true,
      selectionReason: null,
      selectedTrackId,
      defaultTrackId: null,
      availableTracks: [
        {id: 'audio-1', language: 'ger', codec: 'aac', channels: 2, layout: 'stereo', roles: [], default: false},
        {id: 'audio-2', language: 'eng', codec: 'aac', channels: 2, layout: 'stereo', roles: [], default: false}
      ]
    },
    subtitles: {
      selectionSupported: false,
      selectionReason: 'no_subtitle_tracks',
      offSupported: true,
      offSelected: true,
      availableTracks: [],
      selectedTrackId: null,
      defaultTrackId: null
    }
  };
}

function flush(count = 10) {
  let promise = Promise.resolve();
  for (let index = 0; index < count; index += 1) promise = promise.then(() => Promise.resolve());
  return promise;
}

(async function () {
  let timerCalls = 0;
  function setTimeoutFake() {
    timerCalls += 1;
    return timerCalls;
  }
  function clearTimeoutFake() {}

  const requests = [];
  let activeSessionId = '';
  let activeProfileId = 'progressive-fmp4';
  let activeTrackId = 'audio-1';
  let fallbackSelections = 0;
  let lifecycle = null;

  const document = {createElement: node};
  let assigned = {};
  const window = {
    document,
    console,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake,
    VdrSuiteBrowserSession: {csrfHeaders() { return {'X-CSRF-Token': 'csrf-lifecycle'}; }},
    VdrSuiteClientApi: {
      requestJson(requestPath, options) {
        assert.strictEqual(requestPath, '/api/media/sessions');
        const body = JSON.parse(options.body);
        requests.push(body);
        assert.strictEqual(body.operation, 'track-status');
        return Promise.resolve({mediaSession: {
          id: body.sessionId,
          state: 'ready',
          presentationProfileId: activeProfileId,
          tracks: tracks(activeTrackId)
        }});
      }
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
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake
  });
  vm.runInContext(lifecycleSource, context, {filename: 'playback-owner-lifecycle.js'});
  assert.ok(window.VdrSuitePlaybackOwnerLifecycle, 'lifecycle publisher must be installed');

  lifecycle = window.VdrSuitePlaybackOwnerLifecycle.create({
    state: 'idle',
    sessionId: null,
    transport: 'none'
  });
  const observed = [];
  const unsubscribeProbe = lifecycle.subscribe(snapshot => observed.push(snapshot));
  assert.strictEqual(observed.length, 1, 'subscribe must immediately publish the current snapshot');
  assert.strictEqual(observed[0].state, 'idle');
  assert.strictEqual(observed[0].lifecycleVersion, 1);
  assert.strictEqual(observed[0].lifecycleRevision, 0);

  const fastElement = node('section');
  fastElement.className = 'recordings2-fast-player';
  const fastVideo = node('video');
  fastVideo.currentTime = 0;
  fastElement.appendChild(fastVideo);
  const internalStartButton = node('button');
  internalStartButton.className = 'recordings2-primary';
  fastElement.appendChild(internalStartButton);

  const fallbackElement = node('section');
  fallbackElement.className = 'recordings2-recording-fallback-shell';
  const fallbackVideo = node('video');
  fallbackVideo.currentTime = 0;
  fallbackElement.appendChild(fallbackVideo);

  const fallbackOwner = Object.freeze({
    selectAudioTrack(trackId) {
      fallbackSelections += 1;
      activeTrackId = trackId;
      activeSessionId = 'hls-session-' + String(fallbackSelections + 1);
      lifecycle.publish({
        state: 'playing',
        sessionId: activeSessionId,
        transport: 'hls-compatibility',
        transition: 'session-replaced'
      });
      return Promise.resolve(activeSessionId);
    },
    state() { return 'playing'; },
    position() { return 42; },
    stop() { return Promise.resolve(true); }
  });
  fallbackElement.__vdrSuiteRecordingFallbackOwner = fallbackOwner;

  internalStartButton.addEventListener('click', function () {
    activeSessionId = 'progressive-session-1';
    activeProfileId = 'progressive-fmp4';
    lifecycle.publish({
      state: 'starting',
      sessionId: activeSessionId,
      transport: 'progressive-fmp4',
      transition: 'session-started'
    });
  });

  const basePanel = Object.freeze({
    element: fastElement,
    start() { internalStartButton.dispatch('click'); return Promise.resolve(activeSessionId); },
    sessionId() { return activeSessionId; },
    position() { return 42; },
    state() { return activeProfileId === 'hls-fmp4' ? 'fallback' : (activeSessionId ? 'playing' : 'idle'); },
    snapshot() { return lifecycle.snapshot(); },
    subscribe(callback) { return lifecycle.subscribe(callback); },
    seekAbsolute() { return Promise.resolve(true); },
    stop() {
      activeSessionId = '';
      lifecycle.publish({state: 'stopped', sessionId: null, transport: 'none', transition: 'stopped'});
      return Promise.resolve(true);
    },
    destroy() {
      lifecycle.publish({state: 'destroyed', sessionId: null, transport: 'none', transition: 'destroyed'});
    },
    relinquishForReplacement() { return Promise.resolve(activeSessionId); }
  });

  vm.runInContext(trackSource, context, {filename: 'recording-track-controls.js'});
  window.VdrSuiteRecordings2Playback = Object.freeze({createPanel() { return basePanel; }});
  const playback = window.VdrSuiteRecordings2Playback.createPanel({id: 'cloverfield'}, 'default');
  const ownerShell = playback.element;

  assert.strictEqual(ownerShell.className, 'recordings2-track-owner-shell');
  assert.strictEqual(requests.length, 0, 'idle lifecycle snapshot must not issue track-status traffic');
  assert.strictEqual(timerCalls, 0, 'canonical lifecycle owner must not start the legacy session-watch timer');

  // Production-style action: the visible Start button enters the owner-internal
  // closure. Track controls must learn the session only from owner publication,
  // not from an intercepted/decorated start() call.
  internalStartButton.dispatch('click');
  await flush();
  assert.ok(
    requests.some(body => body.sessionId === 'progressive-session-1'),
    'internal Start must publish the first canonical session and trigger track-status'
  );
  assert.strictEqual(timerCalls, 0, 'first session discovery must remain event-driven');

  let select = find(ownerShell, item => item.tagName === 'SELECT');
  assert.ok(select, 'track selector must remain in the persistent owner shell');
  assert.strictEqual(select.disabled, false);

  // Model the production progressive -> HLS transport replacement on the same
  // persistent outer owner. The temporary no-session transport event clears
  // session-bound presentation; the replacement session then drives a new
  // track-status request through the same subscription.
  lifecycle.publish({
    state: 'replacing',
    sessionId: null,
    transport: 'hls-compatibility',
    transition: 'transport-replaced'
  });
  activeProfileId = 'hls-fmp4';
  activeSessionId = 'hls-session-1';
  fastElement.replaceWith(fallbackElement);
  lifecycle.publish({
    state: 'playing',
    sessionId: activeSessionId,
    transport: 'hls-compatibility',
    transition: 'session-replaced'
  });
  await flush();

  assert.strictEqual(ownerShell.children[0], fallbackElement, 'replacement transport must stay inside the persistent owner shell');
  assert.ok(
    requests.some(body => body.sessionId === 'hls-session-1'),
    'replacement MediaSession must trigger a fresh track-status request from lifecycle publication'
  );
  assert.strictEqual(timerCalls, 0, 'replacement session discovery must remain event-driven');

  select = find(ownerShell, item => item.tagName === 'SELECT');
  select.value = 'audio-2';
  select.dispatch('change');
  await flush(16);
  assert.strictEqual(fallbackSelections, 1, 'HLS selection must still delegate to the established HLS owner');
  assert.ok(
    requests.some(body => body.sessionId === 'hls-session-2'),
    'HLS replacement session must be verified after canonical publication'
  );

  const beforeStop = requests.length;
  await playback.stop();
  await flush();
  assert.strictEqual(requests.length, beforeStop, 'stopped owner must not create lifecycle polling traffic');
  assert.strictEqual(timerCalls, 0, 'canonical lifecycle must stay timer-free through stop');

  unsubscribeProbe();
  playback.destroy();
  assert.strictEqual(lifecycle.snapshot().state, 'destroyed');
  assert.ok(observed.some(snapshot => snapshot.transition === 'session-started'));
  assert.ok(observed.some(snapshot => snapshot.transition === 'session-replaced'));
  assert.ok(observed.some(snapshot => snapshot.transition === 'stopped'));
  assert.ok(observed.every(snapshot => snapshot.generation === undefined), 'Slice 2 must not invent presentation generation');

  console.log('phase65d canonical playback owner lifecycle publication ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
