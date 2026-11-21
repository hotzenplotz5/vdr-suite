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
      if (selector !== '.recordings2-recording-fallback-shell') return null;
      return descendants(this).find(item =>
        String(item.className || '').split(/\s+/).includes('recordings2-recording-fallback-shell')
      ) || null;
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

function flush(count = 10) {
  let promise = Promise.resolve();
  for (let index = 0; index < count; index += 1) {
    promise = promise.then(() => Promise.resolve());
  }
  return promise;
}

function tracks(selectedTrackId) {
  return {
    audio: {
      selectionSupported: true,
      selectionReason: null,
      selectedTrackId,
      defaultTrackId: null,
      availableTracks: [
        {
          id: 'audio-1', language: 'ger', label: '', codec: 'aac',
          channels: 2, layout: 'stereo', roles: [], default: false,
          selected: selectedTrackId === 'audio-1'
        },
        {
          id: 'audio-2', language: 'eng', label: '', codec: 'aac',
          channels: 2, layout: 'stereo', roles: [], default: false,
          selected: selectedTrackId === 'audio-2'
        }
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

const requests = [];
const selectionCalls = [];
let activeSessionId = 'hls-track-1';
let activeTrackId = 'audio-1';
let playbackState = 'playing';
const document = {createElement: node};
const root = node('section');
root.className = 'recordings2-recording-fallback-shell';

const hlsOwner = Object.freeze({
  element: root,
  start() { return Promise.resolve(activeSessionId); },
  sessionId() { return activeSessionId; },
  position() { return 125; },
  state() { return playbackState; },
  seekAbsolute() { throw new Error('HLS selection must not use progressive seek'); },
  selectAudioTrack(trackId) {
    selectionCalls.push({trackId, state: playbackState});
    activeTrackId = trackId;
    activeSessionId = 'hls-track-' + String(selectionCalls.length + 1);
    return Promise.resolve(activeSessionId);
  },
  pause() { playbackState = 'paused'; return true; },
  play() { playbackState = 'playing'; return Promise.resolve(true); },
  stop() { playbackState = 'stopped'; return Promise.resolve(true); },
  destroy() { playbackState = 'destroyed'; }
});
root.__vdrSuiteRecordingFallbackOwner = hlsOwner;

const window = {
  document,
  console,
  setTimeout,
  clearTimeout,
  VdrSuiteBrowserSession: {
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-hls-track'}; }
  },
  VdrSuiteClientApi: {
    requestJson(requestPath, options) {
      const body = JSON.parse(options.body);
      requests.push({path: requestPath, body});
      if (body.operation !== 'track-status') {
        throw new Error('HLS selector must not use same-session operation ' + body.operation);
      }
      return Promise.resolve({mediaSession: {
        id: body.sessionId,
        state: 'ready',
        presentationProfileId: 'hls-fmp4',
        tracks: tracks(activeTrackId)
      }});
    }
  }
};
window.window = window;

let assigned = {};
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
  setTimeout,
  clearTimeout
});
vm.runInContext(source, context, {filename: 'recording-track-controls.js'});

window.VdrSuiteRecordings2Playback = Object.freeze({
  createPanel() { return hlsOwner; }
});

(async function () {
  const playback = window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'cloverfield'},
    'default'
  );
  const ownerRoot = playback.element;
  assert.strictEqual(ownerRoot.className, 'recordings2-track-owner-shell');
  assert.strictEqual(await playback.start(), 'hls-track-1');
  await flush();

  const select = find(
    ownerRoot,
    item => item.tagName === 'SELECT' && item['aria-label'] === 'Tonspur auswählen'
  );
  assert.ok(select, 'HLS Recording must expose the normalized selector when restart-ready');
  const row = find(ownerRoot, item => item.className === 'recordings2-audio-track-control');
  assert.strictEqual(row.hidden, false);
  assert.strictEqual(select.disabled, false);
  assert.strictEqual(select.children.length, 2);
  assert.ok(select.children[0].textContent.includes('Deutsch'));
  assert.ok(select.children[1].textContent.includes('Englisch'));
  assert.ok(!select.children[0].textContent.includes('audio-1'));

  select.value = 'audio-2';
  select.dispatch('change');
  await flush(16);

  assert.deepStrictEqual(selectionCalls[0], {trackId: 'audio-2', state: 'playing'});
  assert.strictEqual(activeSessionId, 'hls-track-2');
  assert.strictEqual(activeTrackId, 'audio-2');
  assert.strictEqual(select.value, 'audio-2');
  assert.strictEqual(playbackState, 'playing');
  assert.strictEqual(
    requests.filter(entry => entry.body.operation === 'select-audio-track').length,
    0,
    'HLS track selection must use fresh-session D.2 replacement, not progressive same-session restart'
  );

  playbackState = 'paused';
  select.value = 'audio-1';
  select.dispatch('change');
  await flush(16);
  assert.deepStrictEqual(selectionCalls[1], {trackId: 'audio-1', state: 'paused'});
  assert.strictEqual(activeSessionId, 'hls-track-3');
  assert.strictEqual(activeTrackId, 'audio-1');
  assert.strictEqual(playbackState, 'paused');

  const api = window.VdrSuiteRecordingTrackControls.__test;
  assert.strictEqual(api.languageLabel('ger'), 'Deutsch');
  assert.strictEqual(api.languageLabel('deu'), 'Deutsch');
  assert.strictEqual(api.languageLabel('eng'), 'Englisch');
  assert.strictEqual(api.languageLabel('fra'), 'FRA');

  playback.destroy();
  console.log('phase65d HLS normalized track selector uses published D.2 fallback owner and friendly language labels ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
