'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const lifecycleSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-owner-lifecycle.js'),
  'utf8'
);
const restartSeekSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-fallback-restart-seek.js'),
  'utf8'
);
const fallbackSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-fallback-controls.js'),
  'utf8'
);
const continuitySource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-timeline-continuity.js'),
  'utf8'
);

function descendants(root) {
  const result = [];
  (function walk(value) {
    if (!value) return;
    result.push(value);
    (value.children || []).forEach(walk);
  }(root));
  return result;
}

function element(tagName) {
  const listeners = {};
  const node = {
    tagName: String(tagName || '').toUpperCase(),
    children: [],
    style: {},
    dataset: {},
    className: '',
    hidden: false,
    disabled: false,
    type: '',
    title: '',
    value: '',
    min: '',
    max: '',
    step: '',
    currentTime: 0,
    paused: true,
    controls: true,
    firstChild: null,
    parentNode: null,
    textContent: '',
    classList: {toggle() {}},
    appendChild(child) {
      if (child.parentNode && typeof child.parentNode.removeChild === 'function') {
        child.parentNode.removeChild(child);
      }
      child.parentNode = this;
      this.children.push(child);
      this.firstChild = this.children[0] || null;
      return child;
    },
    replaceChildren() {
      this.children.forEach(child => { child.parentNode = null; });
      this.children = [];
      Array.from(arguments).forEach(child => this.appendChild(child));
      this.firstChild = this.children[0] || null;
    },
    removeChild(child) {
      const index = this.children.indexOf(child);
      if (index >= 0) this.children.splice(index, 1);
      child.parentNode = null;
      this.firstChild = this.children[0] || null;
      return child;
    },
    setAttribute(name, value) { this[name] = String(value); },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    dispatch(name, event) {
      (listeners[name] || []).slice().forEach(callback => callback(event || {target: this}));
    },
    click() { this.dispatch('click', {target: this}); },
    play() {
      this.paused = false;
      this.dispatch('play', {target: this});
      return Promise.resolve();
    },
    pause() {
      this.paused = true;
      this.dispatch('pause', {target: this});
    },
    querySelector(selector) {
      const all = descendants(this).slice(1);
      if (selector === 'video') return all.find(value => value.tagName === 'VIDEO') || null;
      if (selector === 'button.recordings2-primary') {
        return all.find(value =>
          value.tagName === 'BUTTON' &&
          String(value.className || '').split(/\s+/).includes('recordings2-primary')
        ) || null;
      }
      if (selector.charAt(0) === '.') {
        const className = selector.slice(1);
        return all.find(value =>
          String(value.className || '').split(/\s+/).includes(className)
        ) || null;
      }
      const aria = selector.match(/^(button|input)\[aria-label="([^"]+)"\]$/);
      if (aria) {
        return all.find(value =>
          value.tagName === aria[1].toUpperCase() && value['aria-label'] === aria[2]
        ) || null;
      }
      return null;
    }
  };
  return node;
}

function flush(count = 16) {
  let chain = Promise.resolve();
  for (let index = 0; index < count; index += 1) chain = chain.then(() => Promise.resolve());
  return chain;
}

const requests = [];
let createSequence = 0;
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
  Set,
  VdrSuiteBrowserSession: {
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-continuity-hls'}; }
  },
  VdrSuiteClientApi: {
    requestJson(requestPath, options) {
      const body = JSON.parse(options.body);
      requests.push({path: requestPath, body});
      if (body.operation === 'playback-status') {
        return Promise.resolve({mediaSession: {
          id: body.sessionId,
          state: 'ready',
          playback: {
            positionSeconds: 0,
            durationSeconds: 7134,
            seek: {supported: false, preparing: false},
            resume: {supported: true, preparing: false}
          }
        }});
      }
      createSequence += 1;
      const start = Number(body.startPositionSeconds) || 0;
      return Promise.resolve({mediaSession: {
        id: 'hls-session-' + createSequence,
        state: 'ready',
        presentationProfileId: 'hls-fmp4',
        mediaPath: '/api/media/sessions/hls-session-' + createSequence + '/hls/master.m3u8',
        playback: {
          positionSeconds: start,
          durationSeconds: 7134,
          seek: {supported: false, preparing: false},
          resume: {supported: true, preparing: false}
        }
      }});
    }
  },
  setTimeout() { return 1; },
  clearTimeout() {},
  addEventListener() {},
  removeEventListener() {}
};
window.window = window;

let currentPlayback = {};
Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
  configurable: true,
  enumerable: true,
  get() { return currentPlayback; },
  set(value) { currentPlayback = value; }
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
  Set
});
vm.runInContext(lifecycleSource, context, {filename: 'playback-owner-lifecycle.js'});
vm.runInContext(restartSeekSource, context, {filename: 'recording-fallback-restart-seek.js'});
vm.runInContext(fallbackSource, context, {filename: 'recording-fallback-controls.js'});
vm.runInContext(continuitySource, context, {filename: 'playback-timeline-continuity.js'});

window.VdrSuiteRecordings2Playback = {
  createPanel(recording, backendId, options) {
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
        return Promise.resolve(options.createSession()).then(session => {
          sessionId = session.mediaSession.id;
          video.paused = false;
          video.dispatch('play', {target: video});
          return sessionId;
        });
      },
      destroy() { sessionId = ''; video.paused = true; },
      sessionId() { return sessionId; }
    };
  }
};

(async function () {
  const playback = window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-42'},
    'default'
  );
  assert.strictEqual(playback.__vdrSuitePlaybackTimelineContinuityDecorated, true);
  assert.strictEqual(playback.snapshot().continuity.generation, 0,
    'pre-authorized HLS transport construction must not invent a presentation');

  assert.strictEqual(await playback.start(), 'hls-session-1');
  await flush();
  assert.strictEqual(playback.snapshot().continuity.generation, 1);
  assert.strictEqual(playback.presentationBasePosition(), 0);
  assert.strictEqual(playback.position(), 0);

  let video = playback.element.querySelector('video');
  video.currentTime = 47;
  video.dispatch('timeupdate', {target: video});
  assert.strictEqual(playback.position(), 47);
  assert.strictEqual(playback.presentationBasePosition(), 0);

  const timeline = playback.element.querySelector('input[aria-label="Wiedergabeposition"]');
  assert.ok(timeline && timeline.disabled === false);

  // User-style timeline commit delegates to the established restart-seek owner:
  // stop old HLS owner -> create new authorized session -> preserve absolute
  // target as presentation base. No client-local stop/create sequence is added.
  timeline.value = '1200';
  timeline.dispatch('input', {target: timeline});
  timeline.dispatch('change', {target: timeline});
  await flush(24);

  const creates = requests.filter(entry => !entry.body.operation);
  assert.strictEqual(creates.length, 2);
  assert.strictEqual(creates[1].body.startPositionSeconds, 1200);
  assert.strictEqual(playback.sessionId(), 'hls-session-2');
  assert.strictEqual(playback.position(), 1200);
  assert.strictEqual(playback.presentationBasePosition(), 1200);
  assert.strictEqual(playback.snapshot().presentationBasePositionSeconds, 1200);
  assert.strictEqual(playback.snapshot().continuity.generation, 2,
    'replacement-session restart must create exactly one new presentation generation');
  assert.strictEqual(playback.snapshot().continuity.state, 'stable');

  video = playback.element.querySelector('video');
  video.currentTime = 9;
  video.dispatch('timeupdate', {target: video});
  assert.strictEqual(playback.position(), 1209);
  assert.strictEqual(playback.presentationBasePosition(), 1200);
  assert.strictEqual(
    playback.position(),
    playback.presentationBasePosition() + Math.floor(video.currentTime),
    'HLS replacement must keep absolute Recording time as base + local transport time'
  );

  console.log('phase65d Slice 3 HLS replacement-session timeline continuity production composition ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
