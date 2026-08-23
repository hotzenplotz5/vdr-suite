'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'recording-playback-restart-choice.js'),
  'utf8'
);

function element(tagName) {
  const listeners = {};
  return {
    tagName: String(tagName || '').toUpperCase(),
    children: [],
    style: {},
    className: '',
    textContent: '',
    hidden: false,
    disabled: false,
    parentNode: null,
    nextSibling: null,
    appendChild(child) {
      child.parentNode = this;
      const previous = this.children[this.children.length - 1];
      if (previous) previous.nextSibling = child;
      this.children.push(child);
      return child;
    },
    insertBefore(child, reference) {
      child.parentNode = this;
      const index = reference ? this.children.indexOf(reference) : -1;
      if (index < 0) this.children.push(child);
      else this.children.splice(index, 0, child);
      this.children.forEach(function (value, childIndex, values) {
        value.nextSibling = values[childIndex + 1] || null;
      });
      return child;
    },
    addEventListener(name, callback, capture) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push({callback: callback, capture: capture === true});
    },
    click() {
      const entries = listeners.click || [];
      entries.filter(value => value.capture).forEach(value => value.callback({target: this}));
      entries.filter(value => !value.capture).forEach(value => value.callback({target: this}));
    }
  };
}

function wait(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

const startButton = element('button');
startButton.className = 'recordings2-primary';
startButton.hidden = true;
const stopButton = element('button');
const timeline = element('input');
timeline.disabled = false;
const status = element('p');
const panel = element('section');
panel.appendChild(startButton);
panel.appendChild(stopButton);
panel.appendChild(timeline);
panel.appendChild(status);
panel.querySelector = function (selector) {
  if (selector === 'button.recordings2-primary') return startButton;
  if (selector === 'button[aria-label="Wiedergabe stoppen"]') return stopButton;
  if (selector === 'input[aria-label="Wiedergabeposition"]') return timeline;
  if (selector === '.recordings2-playback-status') return status;
  return null;
};

let state = 'playing';
let sessionId = 'session-1';
let position = 2537;
let startCalls = 0;
const seekCalls = [];

const playback = {
  element: panel,
  state() { return state; },
  position() { return position; },
  start() {
    startCalls += 1;
    state = 'playing';
    sessionId = 'session-' + (startCalls + 1);
    startButton.hidden = true;
    startButton.disabled = true;
    return Promise.resolve(sessionId);
  },
  seekAbsolute(target) {
    seekCalls.push(target);
    return Promise.resolve(true);
  },
  sessionId() { return sessionId; }
};

// Existing Recording controller stop handler: the UI helper must capture the
// logical position before this bubble-phase handler resets/replaces playback.
stopButton.addEventListener('click', function () {
  state = 'stopped';
  sessionId = '';
  startButton.hidden = false;
  startButton.disabled = false;
});

const document = {createElement: element};
const window = {document, setTimeout, clearTimeout, Promise, Object, Number, String, Math, console};
window.window = window;
const context = vm.createContext({window, document, setTimeout, clearTimeout, Promise, Object, Number, String, Math, console});
vm.runInContext(source, context, {filename: 'recording-playback-restart-choice.js'});

(async function () {
  const helper = window.VdrSuiteRecordingPlaybackRestartChoice;
  assert.ok(helper && typeof helper.install === 'function');
  assert.strictEqual(helper.__test.formatTime(2537), '00:42:17');

  const installed = helper.install(playback);
  assert.ok(installed);
  assert.strictEqual(helper.install(playback), installed, 'installation must be idempotent');

  stopButton.click();
  await wait(50);
  assert.strictEqual(installed.stopPosition(), 2537);
  assert.strictEqual(installed.canResume(), true);
  assert.strictEqual(installed.choices.hidden, false);
  assert.strictEqual(startButton.hidden, true, 'generic restart button must be replaced by the choice UI');
  assert.strictEqual(installed.resumeButton.hidden, false);
  assert.strictEqual(installed.resumeButton.textContent, '▶ Wiedergabe ab 00:42:17 fortsetzen');
  assert.strictEqual(installed.fromStartButton.textContent, '↺ Wiedergabe von vorn');
  assert.strictEqual(status.textContent, 'Wiedergabe gestoppt · Wiedergabe ab 00:42:17 fortsetzen?');

  installed.resumeButton.click();
  await wait(0);
  assert.strictEqual(startCalls, 1, 'resume must create exactly one fresh MediaSession');
  assert.deepStrictEqual(seekCalls, [2537], 'resume must seek the new MediaSession to the stopped position');
  assert.strictEqual(installed.choices.hidden, true);

  position = 3012;
  timeline.disabled = false;
  stopButton.click();
  await wait(50);
  assert.strictEqual(installed.resumeButton.textContent, '▶ Wiedergabe ab 00:50:12 fortsetzen');
  installed.fromStartButton.click();
  await wait(0);
  assert.strictEqual(startCalls, 2, 'from-start must create one fresh MediaSession');
  assert.deepStrictEqual(seekCalls, [2537], 'from-start must not issue a seek');

  position = 600;
  timeline.disabled = true;
  stopButton.click();
  await wait(50);
  assert.strictEqual(installed.canResume(), false);
  assert.strictEqual(installed.resumeButton.hidden, true, 'resume must not be advertised without a truthful seek contract');
  assert.strictEqual(installed.fromStartButton.disabled, false);
  assert.strictEqual(status.textContent, 'Wiedergabe gestoppt · Wiedergabe von vorn möglich.');

  console.log('phase65d2 recording stop resume choice ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
