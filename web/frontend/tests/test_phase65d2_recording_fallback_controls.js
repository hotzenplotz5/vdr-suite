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
    children: [], style: {}, dataset: {}, className: '', textContent: '',
    hidden: false, disabled: false, type: '', title: '', value: '', min: '', max: '', step: '',
    currentTime: 0, duration: 999, paused: true, controls: true, firstChild: null, parentNode: null,
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
    click() { this.dispatch('click'); },
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

function flush() {
  return Promise.resolve().then(() => Promise.resolve()).then(() => Promise.resolve()).then(() => Promise.resolve());
}

const requests = [];
let createSequence = 0;
const document = {createElement: element};
const window = {
  document, console, Object, Number, String, Math, Promise, Error, Array,
  VdrSuiteBrowserSession: {csrfHeaders() { return {'X-CSRF-Token': 'csrf-test'}; }},
  VdrSuiteClientApi: {
    requestJson(requestPath, options) {
      const body = JSON.parse(options.body);
      requests.push({path: requestPath, body});
      if (body.operation === 'playback-status') {
        return Promise.resolve({mediaSession: {
          id: body.sessionId,
          state: 'ready',
          presentationProfileId: 'hls-fmp4',
          mediaPath: '/api/media/sessions/' + body.sessionId + '/hls/master.m3u8',
          playback: {
            positionSeconds: 0,
            durationSeconds: 7134,
            seek: {supported: false, preparing: false},
            resume: {supported: true, preparing: false}
          }
        }});
      }
      createSequence += 1;
      const id = 'hls-session-' + createSequence;
      const start = Number(body.startPositionSeconds) || 0;
      return Promise.resolve({mediaSession: {
        id,
        state: 'ready',
        presentationProfileId: 'hls-fmp4',
        mediaPath: '/api/media/sessions/' + id + '/hls/master.m3u8',
        playback: {
          positionSeconds: start,
          durationSeconds: createSequence === 1 ? null : 7134,
          seek: {supported: false, preparing: false},
          resume: createSequence === 1
            ? {supported: false, preparing: true}
            : {supported: true, preparing: false}
        }
      }});
    }
  }
};
let timerSequence = 0;
window.setTimeout = function (callback) { timerSequence += 1; Promise.resolve().then(callback); return timerSequence; };
window.clearTimeout = function () {};
window.window = window;

let currentPlayback = {};
Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
  configurable: true, enumerable: true,
  get() { return currentPlayback; },
  set(value) { currentPlayback = value; }
});

const context = vm.createContext({window, document, console, Object, Number, String, Math, Promise, Error, Array});
vm.runInContext(source, context, {filename: 'recording-fallback-controls.js'});

assert.strictEqual(window.__vdrSuiteRecordingFallbackControlsBound, true);
assert.strictEqual(window.VdrSuiteRecordingFallbackControls.__test.formatTime(2494), '00:41:34');

let creates = 0;
let starts = 0;
let destroys = 0;
window.VdrSuiteRecordings2Playback = {
  createPanel(recording, backendId, options) {
    creates += 1;
    assert.ok(options && typeof options.createSession === 'function');
    const panel = element('section');
    const startButton = element('button');
    startButton.className = 'recordings2-primary';
    panel.appendChild(startButton);
    const video = element('video');
    panel.appendChild(video);
    let sessionId = '';
    return {
      element: panel,
      start() {
        starts += 1;
        return Promise.resolve(options.createSession()).then(session => {
          sessionId = session.mediaSession.id;
          video.paused = false;
          video.dispatch('play');
          return sessionId;
        });
      },
      destroy() { destroys += 1; sessionId = ''; video.paused = true; },
      sessionId() { return sessionId; }
    };
  }
};

(async function () {
  const factory = window.VdrSuiteRecordings2Playback;
  assert.strictEqual(factory.__vdrSuiteFallbackControlsDecorated, true);
  const playback = factory.createPanel({id: 'recording-42'}, 'default');
  assert.ok(playback && playback.element);
  assert.strictEqual(typeof playback.resume, 'function');
  assert.strictEqual(typeof playback.canResume, 'function');

  const buttons = descendants(playback.element).filter(value => value.tagName === 'BUTTON');
  const stopButton = buttons.find(value => value.textContent === 'Stop');
  const back60Button = buttons.find(value => value.textContent === '−60');
  const timeline = descendants(playback.element).find(value => value.tagName === 'INPUT' && value.type === 'range');
  const positionLabel = descendants(playback.element).find(value => value.className === 'recordings2-playback-position');
  assert.ok(stopButton && back60Button && timeline && positionLabel);
  assert.strictEqual(back60Button.disabled, true);
  assert.strictEqual(timeline.disabled, true, 'HLS random seek must remain fail-closed');

  assert.strictEqual(await playback.start(), 'hls-session-1');
  await flush();
  assert.strictEqual(playback.canResume(), true, 'index status must activate HLS resume without enabling seek');
  assert.strictEqual(playback.duration(), 7134, 'duration must come from the Recording contract, not video.duration');

  const firstVideo = descendants(playback.element).find(value => value.tagName === 'VIDEO');
  firstVideo.currentTime = 2494;
  firstVideo.dispatch('timeupdate');
  assert.strictEqual(playback.position(), 2494);
  assert.strictEqual(positionLabel.textContent, '00:41:34 / 01:58:54');
  assert.strictEqual(firstVideo.controls, false);

  await playback.stop();
  assert.strictEqual(destroys, 1);
  assert.strictEqual(playback.state(), 'stopped');
  assert.strictEqual(playback.position(), 2494);
  assert.strictEqual(playback.canResume(), true);

  assert.strictEqual(await playback.resume(2494), 'hls-session-2');
  assert.strictEqual(creates, 2, 'resume must create a fresh HLS transport owner');
  assert.strictEqual(starts, 2);
  const createRequests = requests.filter(entry => !entry.body.operation);
  assert.strictEqual(createRequests.length, 2);
  assert.strictEqual(Object.prototype.hasOwnProperty.call(createRequests[0].body, 'startPositionSeconds'), false);
  assert.strictEqual(createRequests[1].body.startPositionSeconds, 2494);

  const secondVideo = descendants(playback.element).find(value => value.tagName === 'VIDEO');
  secondVideo.currentTime = 3;
  secondVideo.dispatch('timeupdate');
  assert.strictEqual(playback.position(), 2497, 'resumed HLS position must be absolute Recording time');
  assert.strictEqual(playback.duration(), 7134);

  await assert.rejects(playback.seekAbsolute(10), /Kompatibilitätspfad/);
  assert.strictEqual(timeline.disabled, true);
  console.log('phase65d2 fallback duration, index and HLS resume contract ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
