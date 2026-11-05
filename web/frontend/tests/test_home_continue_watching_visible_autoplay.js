'use strict';

const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontend = path.join(__dirname, '..');
const filenames = [
  'recordings2-hero-detail.js',
  'recordings2-hero-prewarm.js',
  'recordings2-hero-visibility.js',
  'api/continue-watching-sync.js'
];
const sources = Object.fromEntries(filenames.map(name =>
  [name, fs.readFileSync(path.join(frontend, name), 'utf8')]));

const scheduled = [];
const events = [];
let existingPlayback = {createPanel() {}};
const document = {
  head: {appendChild() {}},
  getElementById() { return null; },
  createElement(tag) {
    return {
      tagName: String(tag).toUpperCase(),
      className: '', hidden: false, dataset: {},
      addEventListener() {},
      setAttribute() {}
    };
  }
};
const root = {
  dataset: {},
  isConnected: true,
  children: [],
  classList: {add(value) { events.push('class:' + value); }},
  appendChild(value) { this.children.push(value); return value; },
  querySelector(selector) {
    if (selector === '.recordings2-hero-mode-back') {
      return this.children.find(child => child.className === 'recordings2-hero-mode-back') || null;
    }
    return null;
  },
  querySelectorAll() { return []; }
};
const window = {
  document, console, fetch: async () => ({ok: true}),
  VdrSuiteBrowserSession: {csrfHeaders() {return {};}},
  VdrSuiteRecordings2Shared: {
    text(value) {return value == null ? '' : String(value);},
    first(value, keys, fallback) {
      for (const key of keys) if (value && value[key] != null) return value[key];
      return fallback;
    },
    node(tag, cls, value) {
      const node = document.createElement(tag);
      node.className = cls;
      node.textContent = value;
      return node;
    }
  },
  setTimeout(callback, delay) {
    scheduled.push({callback, delay});
    return scheduled.length;
  },
  clearTimeout() {}
};
window.window = window;
Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
  configurable: true,
  get() {return existingPlayback;},
  set(value) {existingPlayback = value;}
});

const context = vm.createContext({
  window, document, console, fetch: window.fetch, Promise, Object,
  Array, String, Number, Boolean, Math, Date, Error, WeakMap
});
function load(name) {
  vm.runInContext(sources[name], context, {filename: name});
}

// Production concatenation: prewarm wraps Hero first, visibility wraps last.
load('recordings2-hero-detail.js');
load('recordings2-hero-prewarm.js');
load('recordings2-hero-visibility.js');
load('api/continue-watching-sync.js');

const hero = window.VdrSuiteRecordings2HeroDetail;
assert.equal(typeof hero.enterPlaybackMode, 'function');
assert.equal(typeof hero.__test.resumePosition, 'function',
  'Visibility/prewarm wrappers must retain the original resume helper');
assert.equal(hero.__test.resumePosition({resumePositionSeconds: 238}), 238);
assert.equal(typeof hero.__test.showMode, 'function');

const cw = window.VdrSuiteContinueWatchingSync.__test;
const recording = {id: 'medicus2', backendId: 'default'};
const calls = [];
const owner = {
  element: {
    closest(selector) {
      assert.equal(selector, '.recordings2-detail');
      return root;
    },
    querySelectorAll() {return [];}
  },
  snapshot() {return {state: 'idle', sessionId: null};},
  subscribe(callback) {
    callback({state: 'idle', sessionId: null});
    return () => {};
  },
  start() {
    calls.push('start:' + root.dataset.recordings2HeroMode);
    return Promise.resolve('fast-media-session');
  },
  canResume() {return true;},
  seekAbsolute(position) {
    calls.push('seek:' + position);
    return Promise.resolve(true);
  }
};

async function flush() {
  for (let i = 0; i < 20; i += 1) await Promise.resolve();
}

(async () => {
  cw.rememberOpen(recording, {
    continueWatching: true,
    autoStartPlayback: true,
    playbackStartPositionSeconds: 238
  });
  cw.decorateOwner(owner, recording, 'default');
  assert.deepEqual(calls, [], 'Creating the owner must not start hidden playback');
  const initial = scheduled.find(job => job.delay === 0);
  assert(initial, 'Auto-Resume must schedule an attempt');
  initial.callback();
  assert.equal(root.dataset.recordings2HeroMode, 'playback');
  assert.equal(root.children.some(node => node.className === 'recordings2-hero-mode-back'), true,
    'The visible player must have a return-to-Details control');
  await flush();
  assert.deepEqual(calls, ['start:playback', 'seek:238'],
    'Fast playback may start only after the player is visible and must resume at 238');

  // A detached owner may never play audio behind the Details screen.
  const orphanCalls = [];
  const orphan = Object.assign({}, owner, {
    element: {closest() {return null;}, querySelectorAll() {return [];}},
    start() {orphanCalls.push('start'); return Promise.resolve('bad-session');}
  });
  cw.rememberOpen({id: 'orphan', backendId: 'default'}, {
    continueWatching: true, autoStartPlayback: true, playbackStartPositionSeconds: 42
  });
  cw.decorateOwner(orphan, {id: 'orphan'}, 'default');
  const extra = scheduled.filter(job => job.delay === 0);
  extra[extra.length - 1].callback();
  await flush();
  assert.deepEqual(orphanCalls, [], 'No mounted player means no automatic playback');

  console.log('Home Continue Watching visible-player-first autoplay and resume PASS');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
