'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'session-frontend-sync.js'),
  'utf8'
);

function node(tagName) {
  const listeners = {};
  return {
    tagName: String(tagName || '').toUpperCase(),
    children: [],
    className: '',
    classList: {toggle() {}, add() {}, remove() {}},
    style: {},
    textContent: '',
    title: '',
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
    placeholder: '',
    inputMode: '',
    error: null,
    parentNode: null,
    appendChild(child) { child.parentNode = this; this.children.push(child); return child; },
    setAttribute(name, value) { this[name] = String(value); },
    removeAttribute(name) { if (name === 'src') this.src = ''; },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    removeEventListener() {},
    dispatch(name, event) {
      (listeners[name] || []).forEach(callback => callback(event || {target: this}));
    },
    click() { this.dispatch('click', {target: this}); },
    replaceWith(replacement) { this.replacement = replacement; },
    pause() { this.paused = true; this.dispatch('pause', {target: this}); },
    load() { this.loaded = (this.loaded || 0) + 1; },
    play() {
      this.paused = false;
      this.played = (this.played || 0) + 1;
      this.dispatch('play', {target: this});
      return Promise.resolve();
    }
  };
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

function find(root, predicate) {
  return descendants(root).find(predicate);
}

function flush() {
  return Promise.resolve().then(() => Promise.resolve()).then(() => Promise.resolve());
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

const requests = [];
const stopResponses = [];
const videos = [];
let sessionSequence = 0;

function recordingSession(id, backendId, recordingId) {
  return {
    mediaSession: {
      id,
      state: 'ready',
      backendId,
      recordingId,
      presentationProfileId: 'progressive-fmp4',
      growing: false,
      mediaPath: '/api/media/sessions/' + id + '/recording/stream.mp4',
      playback: {
        positionSeconds: 0,
        durationSeconds: 3600,
        seek: {
          supported: true,
          preparing: false,
          window: {startSeconds: 0, endSeconds: 3600}
        }
      }
    }
  };
}

const document = {
  readyState: 'complete',
  visibilityState: 'visible',
  createElement(tagName) {
    const value = node(tagName);
    if (value.tagName === 'VIDEO') videos.push(value);
    return value;
  },
  addEventListener() {}
};

const window = {
  console,
  document,
  performance: {now() { return 1000; }},
  Date,
  VdrSuitePublicUrl: {
    resolvePath(value) { return '/vdr-suite' + value; }
  },
  VdrSuiteBrowserSession: {
    subscribe() {},
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-test'}; }
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
    requestJson(requestPath, options) {
      const body = JSON.parse(options.body);
      requests.push({path: requestPath, body});
      if (body.operation === 'stop') {
        const response = deferred();
        stopResponses.push({body, response});
        return response.promise;
      }
      sessionSequence += 1;
      const id = 'recording_session_' + sessionSequence;
      return Promise.resolve(recordingSession(id, body.backendId, body.recordingId));
    }
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
  Date
});
vm.runInContext(source, context, {filename: 'session-frontend-sync.js'});

(async function () {
  const playback = window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-restart'},
    'living-room'
  );
  const startButton = find(
    playback.element,
    value => value.tagName === 'BUTTON' && value.className === 'recordings2-primary'
  );
  const controls = find(
    playback.element,
    value => value.className === 'recordings2-playback-controls'
  );
  const video = videos[0];
  assert.ok(startButton && controls && video);

  const firstId = await playback.start();
  assert.strictEqual(firstId, 'recording_session_1');
  assert.strictEqual(playback.sessionId(), firstId);
  assert.strictEqual(playback.state(), 'playing');
  assert.strictEqual(startButton.hidden, true);
  assert.strictEqual(controls.hidden, false);

  const firstMediaPath = video.src;
  const firstLoadCount = video.loaded || 0;
  const firstStopPromise = playback.stop();
  await flush();

  assert.strictEqual(stopResponses.length, 1, 'Stop must issue exactly one server request');
  assert.strictEqual(video.paused, true, 'Stop must pause local playback immediately');
  assert.strictEqual(
    video.src,
    firstMediaPath,
    'Progressive transport must remain connected until the server confirms cleanup'
  );
  assert.strictEqual(
    video.loaded || 0,
    firstLoadCount,
    'Stop must not reset the media element before the server response'
  );

  stopResponses[0].response.resolve({mediaSession: {id: firstId, state: 'ended'}});
  const firstStop = await firstStopPromise;
  assert.strictEqual(firstStop, true);
  assert.strictEqual(playback.state(), 'stopped');
  assert.strictEqual(playback.sessionId(), '', 'stopped session must no longer be panel owner');
  assert.strictEqual(startButton.hidden, false);
  assert.strictEqual(startButton.disabled, false);
  assert.strictEqual(startButton.textContent, '▶ Wiedergabe erneut starten');
  assert.strictEqual(controls.hidden, true);
  assert.strictEqual(video.hidden, true);
  assert.strictEqual(video.src, '', 'confirmed server stop must release the local media transport');
  assert.ok(
    (video.loaded || 0) > firstLoadCount,
    'confirmed server stop must reset the media element exactly after cleanup confirmation'
  );

  let createRequests = requests.filter(value => !value.body.operation);
  let stopRequests = requests.filter(value => value.body.operation === 'stop');
  assert.strictEqual(createRequests.length, 1);
  assert.strictEqual(stopRequests.length, 1);
  assert.strictEqual(stopRequests[0].body.sessionId, firstId);

  startButton.click();
  await flush();
  createRequests = requests.filter(value => !value.body.operation);
  assert.strictEqual(createRequests.length, 2, 'restart must create exactly one new MediaSession');
  assert.strictEqual(playback.sessionId(), 'recording_session_2');
  assert.strictEqual(playback.state(), 'playing');
  assert.strictEqual(startButton.hidden, true);
  assert.strictEqual(controls.hidden, false);
  assert.strictEqual(video.hidden, false);

  const secondStopPromise = playback.stop();
  await flush();
  assert.strictEqual(stopResponses.length, 2);
  stopResponses[1].response.resolve({mediaSession: {id: 'recording_session_2', state: 'ended'}});
  const secondStop = await secondStopPromise;
  assert.strictEqual(secondStop, true);
  stopRequests = requests.filter(value => value.body.operation === 'stop');
  assert.strictEqual(stopRequests.length, 2);
  assert.strictEqual(stopRequests[1].body.sessionId, 'recording_session_2');

  console.log('phase65d2 recording stop restart and cleanup ordering ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
