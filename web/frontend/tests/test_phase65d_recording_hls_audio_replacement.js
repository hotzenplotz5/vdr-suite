'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-fallback-controls.js'),
  'utf8'
);

function element(tagName) {
  const listeners = {};
  const node = {
    tagName: String(tagName || '').toUpperCase(),
    children: [], style: {}, className: '', textContent: '',
    hidden: false, disabled: false, type: '', title: '', value: '',
    currentTime: 0, paused: true, controls: true, firstChild: null, parentNode: null,
    appendChild(child) {
      child.parentNode = this;
      this.children.push(child);
      this.firstChild = this.children[0] || null;
      return child;
    },
    replaceChildren() {
      this.children = [];
      Array.from(arguments).forEach(child => this.appendChild(child));
      this.firstChild = this.children[0] || null;
    },
    removeChild(child) {
      this.children = this.children.filter(value => value !== child);
      this.firstChild = this.children[0] || null;
    },
    setAttribute(name, value) { this[name] = String(value); },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    dispatch(name) { (listeners[name] || []).forEach(callback => callback({target: this})); },
    play() { this.paused = false; this.dispatch('play'); return Promise.resolve(); },
    pause() { this.paused = true; this.dispatch('pause'); },
    querySelector(selector) {
      const all = descendants(this);
      if (selector === 'video') return all.find(value => value.tagName === 'VIDEO') || null;
      if (selector === 'button.recordings2-primary') {
        return all.find(value => value.tagName === 'BUTTON' && value.className === 'recordings2-primary') || null;
      }
      return null;
    }
  };
  return node;
}

function descendants(root) {
  const result = [];
  (function walk(value) {
    if (!value) return;
    result.push(value);
    (value.children || []).forEach(walk);
  }(root));
  return result;
}

const requests = [];
let sequence = 0;
const document = {createElement: element};
const window = {
  document,
  console,
  Object,
  Number,
  String,
  Math,
  Promise,
  Error,
  Array,
  VdrSuiteBrowserSession: {
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-hls-audio'}; }
  },
  VdrSuiteClientApi: {
    requestJson(requestPath, options) {
      const body = JSON.parse(options.body);
      requests.push({path: requestPath, body});
      if (body.operation) throw new Error('unexpected operation ' + body.operation);
      sequence += 1;
      const id = 'hls-audio-' + sequence;
      return Promise.resolve({mediaSession: {
        id,
        state: 'ready',
        presentationProfileId: 'hls-fmp4',
        mediaPath: '/api/media/sessions/' + id + '/hls/master.m3u8',
        playback: {
          positionSeconds: Number(body.startPositionSeconds) || 0,
          durationSeconds: 5400,
          seek: {supported: false, preparing: false},
          resume: {supported: true, preparing: false}
        }
      }});
    }
  }
};
window.window = window;
window.setTimeout = setTimeout;
window.clearTimeout = clearTimeout;

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
  Number,
  String,
  Math,
  Promise,
  Error,
  Array,
  JSON,
  RegExp,
  setTimeout,
  clearTimeout
});
vm.runInContext(source, context, {filename: 'recording-fallback-controls.js'});

let creates = 0;
let destroys = 0;
window.VdrSuiteRecordings2Playback = {
  createPanel(recording, backendId, options) {
    creates += 1;
    const root = element('section');
    const primary = element('button');
    primary.className = 'recordings2-primary';
    root.appendChild(primary);
    const video = element('video');
    root.appendChild(video);
    let sessionId = '';
    return {
      element: root,
      start() {
        return Promise.resolve(options.createSession()).then(session => {
          sessionId = session.mediaSession.id;
          video.paused = false;
          video.dispatch('play');
          return sessionId;
        });
      },
      destroy() {
        destroys += 1;
        video.paused = true;
        sessionId = '';
      },
      sessionId() { return sessionId; }
    };
  }
};

(async function () {
  const playback = window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-cloverfield'},
    'default'
  );
  assert.ok(playback);
  assert.strictEqual(typeof playback.selectAudioTrack, 'function');
  assert.strictEqual(typeof playback.canSelectAudioTrack, 'function');

  assert.strictEqual(await playback.start(), 'hls-audio-1');
  assert.strictEqual(playback.canSelectAudioTrack(), true);
  const firstVideo = descendants(playback.element).find(value => value.tagName === 'VIDEO');
  firstVideo.currentTime = 125;
  firstVideo.dispatch('timeupdate');
  assert.strictEqual(playback.position(), 125);

  assert.strictEqual(await playback.selectAudioTrack('audio-2'), 'hls-audio-2');
  assert.strictEqual(playback.sessionId(), 'hls-audio-2');
  assert.strictEqual(playback.state(), 'playing');
  assert.strictEqual(destroys, 1, 'old HLS owner must be destroyed before replacement');
  assert.strictEqual(creates, 2, 'audio switch must create exactly one replacement owner');
  assert.strictEqual(requests.length, 2);
  assert.strictEqual(requests[0].body.audioTrackId, undefined);
  assert.strictEqual(requests[1].body.audioTrackId, 'audio-2');
  assert.strictEqual(requests[1].body.startPositionSeconds, 125);

  const secondVideo = descendants(playback.element).find(value => value.tagName === 'VIDEO');
  secondVideo.currentTime = 5;
  secondVideo.dispatch('timeupdate');
  assert.strictEqual(playback.position(), 130);
  assert.strictEqual(playback.pause(), true);
  assert.strictEqual(playback.state(), 'paused');

  assert.strictEqual(await playback.selectAudioTrack('audio-1'), 'hls-audio-3');
  assert.strictEqual(playback.sessionId(), 'hls-audio-3');
  assert.strictEqual(playback.state(), 'paused', 'paused audio switch must remain paused');
  assert.strictEqual(destroys, 2);
  assert.strictEqual(creates, 3);
  assert.strictEqual(requests[2].body.audioTrackId, 'audio-1');
  assert.strictEqual(requests[2].body.startPositionSeconds, 130);

  await assert.rejects(
    playback.selectAudioTrack('pid-123'),
    /Ungültige normalisierte Tonspur-ID/
  );
  assert.strictEqual(creates, 3, 'provider-native ID must not create a stream');

  const api = window.VdrSuiteRecordingFallbackControls.__test;
  assert.strictEqual(api.safeAudioTrackId('audio-2'), 'audio-2');
  assert.strictEqual(api.safeAudioTrackId('audio-0'), '');
  assert.strictEqual(api.safeAudioTrackId('pid-123'), '');

  console.log('phase65d HLS audio replacement preserves position, pause state and normalized IDs ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
