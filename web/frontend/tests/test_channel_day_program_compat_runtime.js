'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.resolve(__dirname, '..', 'channel-day-program-compat.js');
const source = fs.readFileSync(sourcePath, 'utf8');

function classList(initial) {
  const values = new Set(initial || []);
  return {
    add(value) { values.add(value); },
    remove(value) { values.delete(value); },
    contains(value) { return values.has(value); }
  };
}

function makeNode(tagName, classes) {
  const listeners = Object.create(null);
  const node = {
    tagName: String(tagName || '').toUpperCase(),
    nodeType: 1,
    children: [],
    parentNode: null,
    firstChild: null,
    className: '',
    classList: classList(classes),
    dataset: {},
    style: {},
    hidden: false,
    disabled: false,
    paused: false,
    textContent: '',
    type: '',
    title: '',
    id: '',
    attributes: {},
    appendChild(child) {
      if (!child) return child;
      if (child.parentNode && typeof child.parentNode.removeChild === 'function') {
        child.parentNode.removeChild(child);
      }
      this.children.push(child);
      child.parentNode = this;
      this.firstChild = this.children[0] || null;
      return child;
    },
    removeChild(child) {
      const index = this.children.indexOf(child);
      if (index >= 0) this.children.splice(index, 1);
      if (child) child.parentNode = null;
      this.firstChild = this.children[0] || null;
      return child;
    },
    replaceChildren() {
      this.children.forEach(child => { child.parentNode = null; });
      this.children = [];
      this.firstChild = null;
      Array.from(arguments).forEach(child => this.appendChild(child));
    },
    setAttribute(name, value) {
      const normalized = String(value);
      this.attributes[name] = normalized;
      if (name === 'id') this.id = normalized;
      if (name === 'data-brand-module') this.dataset.brandModule = normalized;
      if (name === 'data-i18n') this.dataset.i18n = normalized;
      if (name === 'tabindex') this.tabIndex = Number(normalized);
    },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    dispatch(name, event) {
      const value = event || {target: this};
      if (!value.target) value.target = this;
      (listeners[name] || []).slice().forEach(callback => callback(value));
    },
    click() {
      this.dispatch('click', {
        target: this,
        preventDefault() {},
        stopImmediatePropagation() {}
      });
    },
    contains(candidate) {
      if (candidate === this) return true;
      return this.children.some(child => child && typeof child.contains === 'function'
        ? child.contains(candidate)
        : child === candidate);
    },
    querySelector(selector) {
      if (selector === 'video') {
        if (this.tagName === 'VIDEO') return this;
        for (const child of this.children) {
          if (child && typeof child.querySelector === 'function') {
            const found = child.querySelector(selector);
            if (found) return found;
          }
        }
      }
      return null;
    },
    querySelectorAll() { return []; },
    closest(selector) {
      let current = this;
      while (current) {
        if (selector === '.brand-feature' && current.classList && current.classList.contains('brand-feature')) {
          return current;
        }
        current = current.parentNode;
      }
      return null;
    }
  };
  return node;
}

// Preserve the pre-existing compatibility API contract.
const legacyShell = {classList: classList(['channel-browser-module'])};
const legacyRoot = {
  querySelectorAll(selector) {
    assert.strictEqual(selector, '.channel-browser-module');
    return [legacyShell];
  }
};
const basicWindow = {setTimeout, clearTimeout};
const basicDocument = {
  readyState: 'loading',
  addEventListener() {},
  getElementById() { return null; },
  querySelector() { return null; }
};
vm.runInNewContext(source, {
  Array,
  Boolean,
  Date,
  Error,
  JSON,
  Map,
  Math,
  MutationObserver: function MutationObserver() {},
  Number,
  Object,
  Promise,
  RegExp,
  Set,
  String,
  WeakMap,
  document: basicDocument,
  window: basicWindow,
  setTimeout,
  clearTimeout
}, {filename: sourcePath});

const compat = basicWindow.VdrSuiteChannelDayProgramCompat;
assert.ok(compat);
assert.strictEqual(compat.markChannelBrowserShells(legacyRoot), 1);
assert.strictEqual(legacyShell.classList.contains('channel-browser-shell'), true);
assert.strictEqual(compat.normalizedActionLabel('Mehr …'), 'mehr ...');
assert.strictEqual(
  compat.normalizedActionLabel('  Serie   automatisch aufnehmen  '),
  'serie automatisch aufnehmen'
);

let selectedBackendId = 'living-room';
let selectedModule = 'overview';
let channelModulePlayback = null;
let openLiveViewCalls = 0;
let recordingCalls = 0;
let destroyCalls = 0;
let relinquishCalls = 0;
let fullscreenCalls = 0;
let pipCalls = 0;
let nextSession = 1;
let authSubscriber = null;
const globalListeners = Object.create(null);
const createCalls = [];

const brandEntry = makeNode('article', ['brand-feature']);
const brandCopy = makeNode('div');
const liveLabel = makeNode('div');
liveLabel.setAttribute('data-i18n', 'shell.liveTv');
liveLabel.textContent = 'Live TV';
brandCopy.appendChild(liveLabel);
brandEntry.appendChild(brandCopy);

const moduleTab = makeNode('button');
moduleTab.dataset.module = 'channels2';
moduleTab.click = function() {
  openLiveViewCalls += 1;
  selectedModule = 'channels2';
};

const detailMount = makeNode('section');
const restoreButton = makeNode('button');
restoreButton.textContent = '▶ Live-TV starten';
detailMount.querySelector = function(selector) {
  return selector === '.channels2-live button' ? restoreButton : null;
};

function makeActualLive(channel, backendId, options) {
  const video = makeNode('video');
  video.paused = false;
  video.play = function() { this.paused = false; return Promise.resolve(); };
  video.pause = function() { this.paused = true; };
  video.requestPictureInPicture = function() { pipCalls += 1; return Promise.resolve(); };
  video.requestFullscreen = function() { fullscreenCalls += 1; return Promise.resolve(); };

  const panel = makeNode('section');
  panel.appendChild(video);
  const sessionId = 'live-session-' + String(nextSession++);
  const actual = {
    element: panel,
    startCount: 0,
    start() {
      this.startCount += 1;
      return Promise.resolve(sessionId);
    },
    destroy() {
      destroyCalls += 1;
    },
    relinquishForReplacement() {
      relinquishCalls += 1;
      return Promise.resolve(sessionId);
    },
    sessionId() { return sessionId; }
  };
  createCalls.push({channel, backendId, options, actual, video});
  return actual;
}

const rawPlaybackApi = {
  createPanel() {
    recordingCalls += 1;
    return {kind: 'recording'};
  },
  createLivePanel(channel, backendId, options) {
    return makeActualLive(channel, backendId, options || {});
  }
};

const originalChannels2 = {
  activate() { return true; },
  deactivate() {
    if (channelModulePlayback) {
      channelModulePlayback.destroy();
      channelModulePlayback = null;
    }
    return true;
  }
};

const documentListeners = Object.create(null);
const richDocument = {
  readyState: 'loading',
  pictureInPictureEnabled: true,
  pictureInPictureElement: null,
  documentElement: makeNode('html'),
  head: makeNode('head'),
  body: makeNode('body'),
  createElement: makeNode,
  getElementById() { return null; },
  querySelector(selector) {
    if (selector === '[data-i18n="shell.liveTv"]') return liveLabel;
    if (selector === '[data-module="channels2"]') return moduleTab;
    return null;
  },
  addEventListener(name, callback) {
    if (!documentListeners[name]) documentListeners[name] = [];
    documentListeners[name].push(callback);
  }
};

const richWindow = {
  setTimeout,
  clearTimeout,
  document: richDocument,
  VdrSuiteChannels2: originalChannels2,
  VdrSuitePlatform: {
    getSelectedBackendId() { return selectedBackendId; },
    getSelectedModule() { return selectedModule; },
    getMountTarget() { return detailMount; }
  },
  VdrSuiteBrowserSession: {
    subscribe(callback) {
      authSubscriber = callback;
      callback({authenticated: true});
      return function() {};
    }
  },
  addEventListener(name, callback) { globalListeners[name] = callback; },
  removeEventListener() {}
};

// Mirror session-frontend-sync.js: the direct Live facade is exposed through a
// configurable getter/setter so later Recording playback runtime assignments are
// wrapped instead of replacing the accepted direct Live entrypoint.
let playbackFacadeValue = rawPlaybackApi;
Object.defineProperty(richWindow, 'VdrSuiteRecordings2Playback', {
  configurable: true,
  enumerable: true,
  get() { return playbackFacadeValue; },
  set(value) { playbackFacadeValue = value; }
});

restoreButton.click = function() {
  const resumed = richWindow.VdrSuiteRecordings2Playback.createLivePanel(
    createCalls[0].channel,
    selectedBackendId,
    {}
  );
  channelModulePlayback = resumed;
  resumed.start();
};

vm.runInNewContext(source, {
  Array,
  Boolean,
  Date,
  Error,
  JSON,
  Map,
  Math,
  MutationObserver: function MutationObserver() {},
  Number,
  Object,
  Promise,
  RegExp,
  Set,
  String,
  WeakMap,
  document: richDocument,
  window: richWindow,
  setTimeout,
  clearTimeout
}, {filename: sourcePath});

const playbackShell = richWindow.VdrSuitePlaybackShell;
assert.ok(playbackShell);
assert.strictEqual(typeof playbackShell.snapshot, 'function');
assert.strictEqual(typeof richWindow.VdrSuiteRecordings2Playback.createLivePanel, 'function');
assert.strictEqual(richWindow.VdrSuiteRecordings2Playback.createPanel().kind, 'recording');
assert.strictEqual(recordingCalls, 1, 'Recording playback must stay delegated');

// Upper Live-TV entry becomes keyboard/click actionable but only opens Channels2.
assert.strictEqual(brandEntry.dataset.brandModule, 'channels2');
assert.strictEqual(brandEntry.attributes.role, 'button');
assert.strictEqual(brandEntry.attributes.tabindex, '0');
brandEntry.click();
assert.strictEqual(openLiveViewCalls, 1);
assert.strictEqual(selectedModule, 'channels2');
assert.strictEqual(createCalls.length, 0, 'opening Live TV must not auto-start a channel');

const channelA = {id: 'A', name: 'Sender A', enabled: true};
const channelB = {id: 'B', name: 'Sender B', enabled: true};
const channelC = {id: 'C', name: 'Sender C', enabled: true};
const channelD = {id: 'D', name: 'Sender D', enabled: true};

(async function() {
  const playbackA = richWindow.VdrSuiteRecordings2Playback.createLivePanel(
    channelA,
    'living-room',
    {}
  );
  const originalElement = playbackA.element;
  channelModulePlayback = playbackA;
  assert.strictEqual(await playbackA.start(), 'live-session-1');
  assert.strictEqual(createCalls.length, 1);
  assert.strictEqual(createCalls[0].actual.startCount, 1);

  // Leaving Channels2 invokes its normal deactivate(), but the shell intercepts
  // only the playback destroy boundary and moves the exact same element to mini.
  selectedModule = 'epg';
  richWindow.VdrSuiteChannels2.deactivate();
  assert.strictEqual(destroyCalls, 0, 'module navigation must not STOP live playback');
  assert.strictEqual(channelModulePlayback, null, 'Channels2 releases private ownership');
  assert.strictEqual(playbackShell.snapshot().active, true);
  assert.strictEqual(playbackShell.snapshot().miniVisible, true);
  assert.strictEqual(playbackA.element, originalElement);

  // Returning to Channels2 restores the same proxy/session through the existing
  // channel start path: no second adapter, session or start call is created.
  selectedModule = 'channels2';
  richWindow.VdrSuiteChannels2.activate();
  await Promise.resolve();
  assert.strictEqual(channelModulePlayback, playbackA);
  assert.strictEqual(createCalls.length, 1);
  assert.strictEqual(createCalls[0].actual.startCount, 1);
  assert.strictEqual(playbackShell.snapshot().miniVisible, false);
  assert.strictEqual(await channelModulePlayback.start(), 'live-session-1');

  // Existing replacement semantics stay exact: relinquish first, then pass the
  // yielded session id to the next create without an intermediate browser STOP.
  const yieldedA = await playbackA.relinquishForReplacement();
  assert.strictEqual(yieldedA, 'live-session-1');
  assert.strictEqual(relinquishCalls, 1);
  assert.strictEqual(destroyCalls, 0);

  const playbackB = richWindow.VdrSuiteRecordings2Playback.createLivePanel(
    channelB,
    'living-room',
    {replacesSessionId: yieldedA}
  );
  assert.strictEqual(await playbackB.start(), 'live-session-2');
  assert.strictEqual(createCalls.length, 2);
  assert.strictEqual(createCalls[1].options.replacesSessionId, 'live-session-1');

  assert.throws(
    () => richWindow.VdrSuiteRecordings2Playback.createLivePanel(channelC, 'living-room', {}),
    /Replacement-Handoff/,
    'parallel Live owners without the accepted handoff must fail closed'
  );
  assert.strictEqual(createCalls.length, 2);

  // Native PiP is feature-detected only. Its absence does not affect normal live.
  assert.strictEqual(playbackShell.snapshot().pipAvailable, true);
  richDocument.pictureInPictureEnabled = false;
  assert.strictEqual(playbackShell.__test.pipSupported(createCalls[1].video), false);
  assert.strictEqual(playbackShell.snapshot().active, true);
  richDocument.pictureInPictureEnabled = true;

  // A backend boundary is a real STOP boundary.
  selectedBackendId = 'bedroom';
  assert.strictEqual(playbackShell.__test.checkBackendBoundary(), true);
  assert.strictEqual(destroyCalls, 1);
  assert.strictEqual(playbackShell.snapshot().active, false);
  assert.strictEqual(playbackShell.snapshot().lastStopReason, 'backend_changed');

  // Browser-session loss is also a real STOP boundary.
  selectedBackendId = 'living-room';
  const playbackC = richWindow.VdrSuiteRecordings2Playback.createLivePanel(channelC, 'living-room', {});
  assert.strictEqual(await playbackC.start(), 'live-session-3');
  assert.strictEqual(typeof authSubscriber, 'function');
  authSubscriber({authenticated: false});
  assert.strictEqual(destroyCalls, 2);
  assert.strictEqual(playbackShell.snapshot().active, false);
  assert.strictEqual(playbackShell.snapshot().lastStopReason, 'browser_session_lost');

  // Explicit shell stop remains deterministic.
  authSubscriber({authenticated: true});
  const playbackD = richWindow.VdrSuiteRecordings2Playback.createLivePanel(channelD, 'living-room', {});
  assert.strictEqual(await playbackD.start(), 'live-session-4');
  assert.strictEqual(playbackShell.stop(), true);
  assert.strictEqual(destroyCalls, 3);
  assert.strictEqual(playbackShell.snapshot().active, false);
  assert.strictEqual(playbackShell.snapshot().lastStopReason, 'explicit_shell_stop');

  // Fatal player events end ownership rather than leaving a stale mini-player.
  const playbackE = richWindow.VdrSuiteRecordings2Playback.createLivePanel(channelA, 'living-room', {});
  assert.strictEqual(await playbackE.start(), 'live-session-5');
  createCalls[4].video.dispatch('error', {target: createCalls[4].video});
  assert.strictEqual(destroyCalls, 4);
  assert.strictEqual(playbackShell.snapshot().lastStopReason, 'playback_error');

  // The shell does not replace the accepted underlying pagehide owner.
  assert.strictEqual(globalListeners.pagehide, undefined);
  assert.strictEqual(typeof globalListeners.pageshow, 'function');

  // A later Recording runtime assignment must remain behind the shell facade.
  const laterPlaybackApi = {
    createPanel() {
      recordingCalls += 1;
      return {kind: 'recording-late'};
    },
    createLivePanel(channel, backendId, options) {
      return makeActualLive(channel, backendId, options || {});
    }
  };
  richWindow.VdrSuiteRecordings2Playback = laterPlaybackApi;
  assert.strictEqual(richWindow.VdrSuiteRecordings2Playback.createPanel().kind, 'recording-late');
  assert.strictEqual(recordingCalls, 2);
  assert.strictEqual(typeof richWindow.VdrSuiteRecordings2Playback.createLivePanel, 'function');

  assert.ok(source.includes('global.VdrSuitePlaybackShell = api'));
  assert.ok(source.includes('replacesSessionId'));
  assert.ok(source.includes('requestPictureInPicture'));
  assert.ok(!source.includes('navigator.userAgent'));
  assert.ok(!source.includes('fetch('));

  console.log('persistent Live playback shell lifecycle ok');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
