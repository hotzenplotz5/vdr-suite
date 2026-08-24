'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-fallback-controls.js'),
  'utf8'
);

function descendants(root) {
  const result = [];
  (function walk(node) {
    if (!node) return;
    result.push(node);
    (node.children || []).forEach(walk);
  }(root));
  return result;
}

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
    currentTime: 0,
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
}

const document = {createElement: element};
let installedOwner = null;
let installCalls = 0;
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
  VdrSuiteRecordingPlaybackRestartChoice: {
    install(playback) {
      installCalls += 1;
      installedOwner = playback;
      return {};
    }
  }
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
  Array
});
vm.runInContext(source, context, {filename: 'recording-fallback-controls.js'});

window.VdrSuiteRecordings2Playback = {
  createPanel() {
    const panel = element('section');
    const startButton = element('button');
    startButton.className = 'recordings2-primary';
    panel.appendChild(startButton);
    panel.appendChild(element('video'));
    return {
      element: panel,
      start() { return Promise.resolve('legacy-session'); },
      destroy() {},
      sessionId() { return 'legacy-session'; }
    };
  }
};

const factory = window.VdrSuiteRecordings2Playback;
const playback = factory.createPanel({id: 'recording-42'}, 'default');

assert.ok(playback && playback.element, 'decorated fallback owner must be created');
assert.strictEqual(
  playback.element.className,
  'recordings2-recording-fallback-shell',
  'restart choice must bind to the replacement fallback shell, not the detached fast owner'
);
assert.strictEqual(installCalls, 1, 'fallback owner must bind the shared restart-choice UI exactly once');
assert.strictEqual(installedOwner, playback, 'shared restart-choice UI must own the actual fallback playback object');
assert.strictEqual(typeof playback.resume, 'function');
assert.strictEqual(typeof playback.canResume, 'function');

console.log('phase65d2 fallback replacement owner rebinds restart choice');
