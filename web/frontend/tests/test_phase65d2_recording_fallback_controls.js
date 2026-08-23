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
    children: [],
    style: {},
    dataset: {},
    className: '',
    textContent: '',
    hidden: false,
    disabled: false,
    type: '',
    title: '',
    value: '',
    min: '',
    max: '',
    step: '',
    currentTime: 0,
    duration: NaN,
    paused: true,
    controls: true,
    firstChild: null,
    parentNode: null,
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
    dispatch(name) {
      (listeners[name] || []).forEach(callback => callback({target: this}));
    },
    click() { this.dispatch('click'); },
    play() {
      this.paused = false;
      this.dispatch('play');
      return Promise.resolve();
    },
    pause() {
      this.paused = true;
      this.dispatch('pause');
    },
    querySelector(selector) {
      const all = descendants(this);
      if (selector === 'video') {
        return all.find(value => value.tagName === 'VIDEO') || null;
      }
      if (selector === 'button.recordings2-primary') {
        return all.find(value =>
          value.tagName === 'BUTTON' && value.className === 'recordings2-primary'
        ) || null;
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

const document = {createElement: element};
const window = {document, console, Object, Number, String, Math, Promise};
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
  Array
});
vm.runInContext(source, context, {filename: 'recording-fallback-controls.js'});

assert.strictEqual(window.__vdrSuiteRecordingFallbackControlsBound, true);
assert.strictEqual(
  window.VdrSuiteRecordingFallbackControls.__test.formatTime(2494),
  '00:41:34'
);

let creates = 0;
let starts = 0;
let destroys = 0;
window.VdrSuiteRecordings2Playback = {
  createPanel() {
    creates += 1;
    const panel = element('section');
    const startButton = element('button');
    startButton.className = 'recordings2-primary';
    panel.appendChild(startButton);
    const video = element('video');
    video.duration = 7134;
    panel.appendChild(video);
    let sessionId = '';
    return {
      element: panel,
      start() {
        starts += 1;
        sessionId = 'legacy-session-' + starts;
        video.paused = false;
        video.dispatch('play');
        return Promise.resolve(sessionId);
      },
      destroy() {
        destroys += 1;
        sessionId = '';
        video.paused = true;
      },
      sessionId() { return sessionId; }
    };
  }
};

(async function () {
  const playbackFactory = window.VdrSuiteRecordings2Playback;
  assert.strictEqual(playbackFactory.__vdrSuiteFallbackControlsDecorated, true);

  const playback = playbackFactory.createPanel({id: 'recording-42'}, 'default');
  assert.ok(playback && playback.element);
  assert.strictEqual(typeof playback.play, 'function');
  assert.strictEqual(typeof playback.pause, 'function');
  assert.strictEqual(typeof playback.stop, 'function');
  assert.strictEqual(typeof playback.position, 'function');
  assert.strictEqual(typeof playback.duration, 'function');
  assert.strictEqual(typeof playback.state, 'function');
  assert.strictEqual(typeof playback.seekAbsolute, 'function');
  assert.strictEqual(typeof playback.seekRelative, 'function');

  const buttons = descendants(playback.element).filter(value => value.tagName === 'BUTTON');
  const stopButton = buttons.find(value => value.textContent === 'Stop');
  const back60Button = buttons.find(value => value.textContent === '−60');
  const playPauseButton = buttons.find(value => value.textContent === 'Play');
  const restartButton = buttons.find(value => value.textContent === '↺ Wiedergabe von vorn');
  const timeline = descendants(playback.element).find(value =>
    value.tagName === 'INPUT' && value.type === 'range'
  );
  const positionLabel = descendants(playback.element).find(value =>
    value.className === 'recordings2-playback-position'
  );

  assert.ok(stopButton && back60Button && playPauseButton && restartButton && timeline && positionLabel);
  assert.strictEqual(back60Button.disabled, true, 'fallback must not advertise unsupported time seek');
  assert.strictEqual(timeline.disabled, true, 'fallback timeline must stay disabled without a truthful seek contract');
  assert.strictEqual(restartButton.hidden, true);

  const firstSession = await playback.start();
  assert.strictEqual(firstSession, 'legacy-session-1');
  assert.strictEqual(playback.state(), 'playing');
  assert.strictEqual(stopButton.disabled, false, 'Stop must remain available after fallback activation');

  const firstVideo = descendants(playback.element).find(value => value.tagName === 'VIDEO');
  assert.ok(firstVideo);
  assert.strictEqual(firstVideo.controls, false, 'suite-owned controls replace native fallback controls');
  firstVideo.currentTime = 2494;
  firstVideo.dispatch('timeupdate');
  assert.strictEqual(playback.position(), 2494);
  assert.strictEqual(playback.duration(), 7134);
  assert.strictEqual(positionLabel.textContent, '00:41:34 / 01:58:54');

  assert.strictEqual(playback.pause(), true);
  assert.strictEqual(playback.state(), 'paused');
  await playback.play();
  assert.strictEqual(playback.state(), 'playing');

  await playback.stop();
  assert.strictEqual(destroys, 1);
  assert.strictEqual(playback.state(), 'stopped');
  assert.strictEqual(playback.position(), 2494);
  assert.strictEqual(restartButton.hidden, false, 'fallback Stop must offer an explicit from-start restart');

  const secondSession = await playback.start();
  assert.strictEqual(secondSession, 'legacy-session-2');
  assert.strictEqual(creates, 2, 'restart must create a fresh legacy transport owner');
  assert.strictEqual(starts, 2);
  assert.strictEqual(playback.state(), 'playing');

  await assert.rejects(
    playback.seekAbsolute(10),
    /Kompatibilitätspfad/,
    'fallback must fail closed for unsupported time seek'
  );

  console.log('phase65d2 recording fallback controls remain stable and truthful');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
