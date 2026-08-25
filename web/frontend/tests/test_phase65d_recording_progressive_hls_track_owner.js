'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-track-controls.js'),
  'utf8'
);

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

function descendants(root) {
  const values = [];
  (function walk(item) {
    if (!item) return;
    values.push(item);
    (item.children || []).forEach(walk);
  }(root));
  return values;
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

function flush(count = 8) {
  let promise = Promise.resolve();
  for (let index = 0; index < count; index += 1) promise = promise.then(() => Promise.resolve());
  return promise;
}

(async function () {
  const timers = [];
  function setTimeoutFake(callback) {
    timers.push(callback);
    return timers.length;
  }
  function clearTimeoutFake() {}

  let activeSessionId = 'progressive-session-1';
  let activeProfileId = 'progressive-fmp4';
  let activeTrackId = 'audio-1';
  let fallbackSelections = 0;
  const requests = [];

  const fastElement = node('section');
  fastElement.className = 'recordings2-fast-player';
  const fallbackElement = node('section');
  fallbackElement.className = 'recordings2-recording-fallback-shell';

  const fallbackOwner = Object.freeze({
    selectAudioTrack(trackId) {
      fallbackSelections += 1;
      activeTrackId = trackId;
      activeSessionId = 'hls-session-' + String(fallbackSelections + 1);
      return Promise.resolve(activeSessionId);
    },
    state() { return 'playing'; },
    stop() { return Promise.resolve(true); }
  });
  fallbackElement.__vdrSuiteRecordingFallbackOwner = fallbackOwner;

  const basePanel = Object.freeze({
    element: fastElement,
    start() { return Promise.resolve(activeSessionId); },
    sessionId() { return activeSessionId; },
    position() { return 42; },
    state() { return 'playing'; },
    seekAbsolute() { return Promise.resolve(true); },
    stop() { return Promise.resolve(true); },
    destroy() {},
    relinquishForReplacement() { return Promise.resolve(activeSessionId); }
  });

  let assigned = {};
  const document = {createElement: node};
  const window = {
    document,
    console,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake,
    VdrSuiteBrowserSession: {csrfHeaders() { return {'X-CSRF-Token': 'csrf-owner'}; }},
    VdrSuiteClientApi: {
      requestJson(requestPath, options) {
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
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake
  });
  vm.runInContext(source, context, {filename: 'recording-track-controls.js'});

  window.VdrSuiteRecordings2Playback = Object.freeze({createPanel() { return basePanel; }});
  const playback = window.VdrSuiteRecordings2Playback.createPanel({id: 'cloverfield'}, 'default');
  const ownerShell = playback.element;
  assert.strictEqual(ownerShell.className, 'recordings2-track-owner-shell');
  assert.strictEqual(ownerShell.children[0], fastElement);

  await playback.start();
  await flush();
  let select = find(ownerShell, item => item.tagName === 'SELECT');
  assert.ok(select, 'selector must be mounted in stable owner shell');
  assert.strictEqual(select.disabled, false);

  activeProfileId = 'hls-fmp4';
  activeSessionId = 'hls-session-1';
  fastElement.replaceWith(fallbackElement);
  assert.strictEqual(ownerShell.children[0], fallbackElement, 'HLS transport must replace only the inner fast node');
  assert.ok(find(ownerShell, item => item.className === 'recordings2-track-controls'), 'track controls must survive replacement');

  assert.ok(timers.length > 0, 'session watcher must be scheduled');
  const sessionWatch = timers.shift();
  sessionWatch();
  await flush();

  select = find(ownerShell, item => item.tagName === 'SELECT');
  assert.strictEqual(select.disabled, false, 'HLS selector must remain enabled after owner transition');
  assert.strictEqual(select.children.length, 2);
  assert.ok(select.children[0].textContent.includes('Deutsch'));
  assert.ok(select.children[1].textContent.includes('Englisch'));
  assert.ok(requests.some(body => body.sessionId === 'hls-session-1'), 'new HLS session must receive a fresh track-status request');

  select.value = 'audio-2';
  select.dispatch('change');
  await flush(16);
  assert.strictEqual(fallbackSelections, 1, 'selection must delegate to the existing HLS D.2 owner');
  assert.strictEqual(activeTrackId, 'audio-2');
  assert.strictEqual(select.value, 'audio-2');
  assert.ok(requests.some(body => body.sessionId === 'hls-session-2'), 'replacement session must be verified');

  console.log('phase65d progressive-to-HLS track owner survives DOM replacement and delegates selection');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
