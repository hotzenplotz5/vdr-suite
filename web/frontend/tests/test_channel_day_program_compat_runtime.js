'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.resolve(__dirname, '..', 'channel-day-program-compat.js');
const source = fs.readFileSync(sourcePath, 'utf8');
const indexSource = fs.readFileSync(path.resolve(__dirname, '..', 'index.html'), 'utf8');

function classList(initial) {
  const values = new Set(initial || []);
  return {
    add(value) { values.add(value); },
    remove(value) { values.delete(value); },
    contains(value) { return values.has(value); }
  };
}

function makeNode(tagName) {
  const listeners = Object.create(null);
  const node = {
    tagName: String(tagName || '').toUpperCase(),
    nodeType: 1,
    children: [],
    parentNode: null,
    firstChild: null,
    className: '',
    classList: classList(),
    dataset: {},
    style: {},
    hidden: false,
    disabled: false,
    paused: false,
    textContent: '',
    type: '',
    id: '',
    onclick: null,
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
      for (let index = 0; index < arguments.length; index += 1) {
        this.appendChild(arguments[index]);
      }
    },
    setAttribute(name, value) {
      this[name] = String(value);
    },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    dispatch(name, event) {
      (listeners[name] || []).slice().forEach(callback => callback(event || {target: this}));
    },
    click() {
      this.dispatch('click', {target: this});
      if (typeof this.onclick === 'function') this.onclick({target: this});
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
    closest() { return null; }
  };
  return node;
}

const shell = {
  classList: classList(['channel-browser-module'])
};
const root = {
  querySelectorAll(selector) {
    assert.strictEqual(selector, '.channel-browser-module');
    return [shell];
  }
};

const basicWindow = {
  setTimeout
};
const basicDocument = {
  readyState: 'loading',
  addEventListener() {},
  getElementById() { return null; }
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
  setTimeout
}, {filename: sourcePath});

const compat = basicWindow.VdrSuiteChannelDayProgramCompat;
assert.ok(compat);
assert.strictEqual(compat.markChannelBrowserShells(root), 1);
assert.strictEqual(shell.classList.contains('channel-browser-shell'), true);
assert.strictEqual(compat.normalizedActionLabel('Mehr …'), 'mehr ...');
assert.strictEqual(
  compat.normalizedActionLabel('  Serie   automatisch aufnehmen  '),
  'serie automatisch aufnehmen'
);
assert.ok(source.includes('.channel-browser-shell[hidden]{display:none!important;}'));
assert.ok(source.includes('.channel-day-program-view.channel-day-detail-mode'));
assert.ok(source.includes("script.src = '/frontend/channel-day-program.js?late='"));
assert.ok(source.includes("button.textContent = 'Serientimer'"));
assert.ok(source.includes("event.target.closest === 'function'"));
assert.ok(!source.includes('fetch('));

assert.match(
  indexSource,
  /<article class="brand-feature" data-brand-module="channels2" tabindex="0" role="button" aria-label="Live TV" data-i18n-aria-label="shell.liveTv">/
);
const channelRuntimePosition = indexSource.indexOf(
  '<script src="../frontend/channel-day-program.js"></script>'
);
const compatPosition = indexSource.indexOf(
  '<script src="../frontend/channel-day-program-compat.js"></script>'
);
const appPosition = indexSource.indexOf('<script src="../frontend/app.js"></script>');
assert.ok(channelRuntimePosition >= 0);
assert.ok(compatPosition > channelRuntimePosition);
assert.ok(appPosition > compatPosition);

let selectedBackendId = 'living-room';
let selectedModule = 'channels2';
let channelModulePlayback = null;
const createCalls = [];
let recordingCalls = 0;
let destroyCalls = 0;
let relinquishCalls = 0;
let nextSession = 1;
let authSubscriber = null;

function makeActualLive(channel, backendId, options) {
  const video = makeNode('video');
  video.paused = false;
  video.play = function() { this.paused = false; return Promise.resolve(); };
  video.pause = function() { this.paused = true; };
  video.requestPictureInPicture = function() { return Promise.resolve(); };
  video.requestFullscreen = function() { return Promise.resolve(); };

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
    sessionId() {
      return sessionId;
    }
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
    return makeActualLive(channel, backendId, options);
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
  querySelector() { return null; },
  addEventListener(name, callback) {
    if (!documentListeners[name]) documentListeners[name] = [];
    documentListeners[name].push(callback);
  }
};

const originalChannels2 = {
  activate() {
    return true;
  },
  deactivate() {
    if (channelModulePlayback) {
      channelModulePlayback.destroy();
      channelModulePlayback = null;
    }
    return true;
  }
};

const richWindow = {
  setTimeout,
  VdrSuiteRecordings2Playback: rawPlaybackApi,
  VdrSuiteChannels2: originalChannels2,
  VdrSuitePlatform: {
    getSelectedBackendId() { return selectedBackendId; },
    getSelectedModule() { return selectedModule; },
    getModule(name) {
      return name === 'channels2' ? originalChannels2 : null;
    }
  },
  VdrSuiteBrowserSession: {
    subscribe(callback) {
      authSubscriber = callback;
    }
  }
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
  setTimeout
}, {filename: sourcePath});

const playbackShell = richWindow.VdrSuitePlaybackShell;
assert.ok(playbackShell);
assert.strictEqual(typeof playbackShell.snapshot, 'function');
assert.strictEqual(typeof richWindow.VdrSuiteRecordings2Playback.createLivePanel, 'function');
assert.strictEqual(richWindow.VdrSuiteRecordings2Playback.createPanel().kind, 'recording');
assert.strictEqual(recordingCalls, 1, 'Recording playback facade must remain delegated');

const channelA = {id: 'A', name: 'Sender A', number: 1};
const channelB = {id: 'B', name: 'Sender B', number: 2};
const channelC = {id: 'C', name: 'Sender C', number: 3};

(async function() {
  const playbackA = richWindow.VdrSuiteRecordings2Playback.createLivePanel(
    channelA,
    'living-room',
    {}
  );
  const hostA = playbackA.element;
  const sessionA = await playbackA.start();
  assert.strictEqual(sessionA, 'live-session-1');
  assert.strictEqual(createCalls.length, 1);
  assert.strictEqual(createCalls[0].actual.startCount, 1);

  // The shell facade preserves Channels2 state deactivation while changing
  // only its playback ownership boundary: deactivate() detaches, not STOP.
  channelModulePlayback = playbackA;
  selectedModule = 'epg';
  richWindow.VdrSuiteChannels2.deactivate();
  assert.strictEqual(destroyCalls, 0);
  assert.strictEqual(playbackShell.snapshot().active, true);
  assert.strictEqual(playbackShell.snapshot().miniVisible, true);
  assert.notStrictEqual(richWindow.VdrSuiteChannels2, originalChannels2);
  assert.strictEqual(
    richWindow.VdrSuitePlatform.getModule('channels2'),
    richWindow.VdrSuiteChannels2
  );

  // Returning to the same live channel reuses the exact proxy/host/session.
  selectedModule = 'channels2';
  const resumedA = richWindow.VdrSuiteRecordings2Playback.createLivePanel(
    channelA,
    'living-room',
    {}
  );
  assert.strictEqual(resumedA, playbackA);
  assert.strictEqual(resumedA.element, hostA);
  const largeMount = makeNode('section');
  largeMount.appendChild(resumedA.element);
  assert.strictEqual(await resumedA.start(), 'live-session-1');
  assert.strictEqual(createCalls.length, 1);
  assert.strictEqual(createCalls[0].actual.startCount, 1);
  assert.strictEqual(playbackShell.snapshot().miniVisible, false);

  // The already-accepted relinquish + replacesSessionId contract remains exact.
  const yieldedA = await resumedA.relinquishForReplacement();
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

  // The adapter fails closed if any caller tries to bypass the accepted
  // replacement handoff and create a second live stream in parallel.
  assert.throws(
    function() {
      richWindow.VdrSuiteRecordings2Playback.createLivePanel(
        channelC,
        'living-room',
        {}
      );
    },
    /Replacement-Handoff/
  );
  assert.strictEqual(createCalls.length, 2);

  const yieldedB = await playbackB.relinquishForReplacement();
  assert.strictEqual(yieldedB, 'live-session-2');
  const playbackC = richWindow.VdrSuiteRecordings2Playback.createLivePanel(
    channelC,
    'living-room',
    {replacesSessionId: yieldedB}
  );
  assert.strictEqual(await playbackC.start(), 'live-session-3');
  assert.strictEqual(relinquishCalls, 2);
  assert.strictEqual(createCalls.length, 3);
  assert.strictEqual(createCalls[2].options.replacesSessionId, 'live-session-2');

  // PiP is native/feature-detected only; its absence does not affect playback.
  assert.strictEqual(playbackShell.snapshot().pipAvailable, true);
  richDocument.pictureInPictureEnabled = false;
  assert.strictEqual(playbackShell.__test.pipSupported(createCalls[2].video), false);
  assert.strictEqual(playbackShell.snapshot().active, true);
  richDocument.pictureInPictureEnabled = true;

  // Backend change is a real stop boundary.
  selectedBackendId = 'bedroom';
  assert.strictEqual(playbackShell.__test.checkBackendBoundary(), true);
  assert.strictEqual(destroyCalls, 1);
  assert.strictEqual(playbackShell.snapshot().active, false);
  assert.strictEqual(playbackShell.snapshot().lastStopReason, 'backend_changed');

  // Browser-session loss is also a real stop boundary.
  selectedBackendId = 'living-room';
  const playbackAgain = richWindow.VdrSuiteRecordings2Playback.createLivePanel(
    channelA,
    'living-room',
    {}
  );
  await playbackAgain.start();
  playbackShell.__test.bindBrowserSession();
  assert.strictEqual(typeof authSubscriber, 'function');
  authSubscriber({authenticated: true});
  authSubscriber({authenticated: false});
  assert.strictEqual(destroyCalls, 2);
  assert.strictEqual(playbackShell.snapshot().active, false);
  assert.strictEqual(playbackShell.snapshot().lastStopReason, 'browser_session_lost');

  // Explicit shell stop remains deterministic.
  const finalPlayback = richWindow.VdrSuiteRecordings2Playback.createLivePanel(
    channelA,
    'living-room',
    {}
  );
  await finalPlayback.start();
  playbackShell.stop();
  assert.strictEqual(destroyCalls, 3);
  assert.strictEqual(playbackShell.snapshot().active, false);
  assert.strictEqual(playbackShell.snapshot().lastStopReason, 'explicit_shell_stop');

  console.log('test_channel_day_program_compat_runtime passed');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
