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
    tagName: String(tagName || '').toUpperCase(), children: [], style: {}, className: '',
    textContent: '', hidden: false, disabled: false, parentNode: null, nextSibling: null,
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
      if (index < 0) this.children.push(child); else this.children.splice(index, 0, child);
      this.children.forEach((value, childIndex, values) => { value.nextSibling = values[childIndex + 1] || null; });
      return child;
    },
    addEventListener(name, callback, options) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push({callback, capture: options === true || Boolean(options && options.capture)});
    },
    click() {
      const event = {
        preventDefault() {}, immediateStopped: false,
        stopImmediatePropagation() { this.immediateStopped = true; }
      };
      const entries = listeners.click || [];
      for (const entry of entries.filter(value => value.capture)) {
        entry.callback(event); if (event.immediateStopped) return;
      }
      for (const entry of entries.filter(value => !value.capture)) {
        entry.callback(event); if (event.immediateStopped) return;
      }
    }
  };
}

const startButton = element('button');
startButton.className = 'recordings2-primary';
startButton.hidden = true;
const stopButton = element('button');
const timeline = element('input');
timeline.disabled = true;
const status = element('p');
const panel = element('section');
panel.appendChild(startButton);
panel.appendChild(stopButton);
panel.appendChild(timeline);
panel.appendChild(status);
panel.querySelector = selector => {
  if (selector === 'button.recordings2-primary') return startButton;
  if (selector === 'button[aria-label="Wiedergabe stoppen"]') return stopButton;
  if (selector === 'input[aria-label="Wiedergabeposition"]') return timeline;
  if (selector === '.recordings2-playback-status') return status;
  return null;
};

let state = 'playing';
let stopCalls = 0;
let startCalls = 0;
const resumeCalls = [];
const playback = {
  element: panel,
  state() { return state; },
  position() { return 2494; },
  canResume() { return true; },
  stop() { stopCalls += 1; state = 'stopped'; return Promise.resolve(true); },
  start() { startCalls += 1; state = 'playing'; return Promise.resolve('from-start-session'); },
  resume(position) { resumeCalls.push(position); state = 'playing'; return Promise.resolve('resumed-hls-session'); },
  seekAbsolute() { throw new Error('HLS seek must not be used'); }
};

const document = {createElement: element};
const window = {document, Promise, Object, Number, String, Math, console};
window.window = window;
const context = vm.createContext({window, document, Promise, Object, Number, String, Math, console});
vm.runInContext(source, context, {filename: 'recording-playback-restart-choice.js'});

(async function () {
  const installed = window.VdrSuiteRecordingPlaybackRestartChoice.install(playback);
  assert.ok(installed);
  stopButton.click();
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(stopCalls, 1);
  assert.strictEqual(installed.canResume(), true, 'owner resume capability must work with disabled HLS timeline');
  assert.strictEqual(installed.resumeButton.hidden, false);
  assert.strictEqual(installed.resumeButton.textContent, '▶ Wiedergabe ab 00:41:34 fortsetzen');

  installed.resumeButton.click();
  await Promise.resolve();
  await Promise.resolve();
  assert.deepStrictEqual(resumeCalls, [2494], 'HLS resume must call the owner resume operation directly');
  assert.strictEqual(startCalls, 0, 'HLS resume must not start at zero and then pretend to seek');

  state = 'stopped';
  installed.fromStartButton.click();
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(startCalls, 1, 'from-start still uses the ordinary fresh-session start');
  assert.deepStrictEqual(resumeCalls, [2494]);
  console.log('phase65d2 restart choice supports direct HLS resume owner');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
