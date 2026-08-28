'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const recoverySource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-network-recovery.js'),
  'utf8'
);

function node(tagName) {
  const listeners = {};
  return {
    tagName: String(tagName || '').toUpperCase(),
    className: '',
    classList: {toggle() {}},
    textContent: '',
    currentTime: 0,
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    removeEventListener(name, callback) {
      if (!listeners[name]) return;
      listeners[name] = listeners[name].filter(entry => entry !== callback);
    },
    dispatch(name) {
      (listeners[name] || []).slice().forEach(callback => callback({target: this}));
    }
  };
}

function flush(rounds) {
  let promise = Promise.resolve();
  for (let index = 0; index < (rounds || 12); index += 1) {
    promise = promise.then(() => Promise.resolve());
  }
  return promise;
}

(async function () {
  const status = node('div');
  status.className = 'recordings2-playback-status';
  const video = node('video');
  const shell = {
    querySelector(selector) {
      if (selector === '.recordings2-playback-status') return status;
      if (selector === 'video') return video;
      return null;
    }
  };

  const listeners = {};
  const timers = new Map();
  let nextTimerId = 1;
  let reachable = true;
  let createCount = 0;
  let sessionId = '';
  let state = 'idle';
  let snapshot = {
    state: 'idle',
    transport: null,
    failure: null
  };
  const subscribers = [];
  const seekPositions = [];

  function setTimeoutFake(callback, delay) {
    const id = nextTimerId++;
    timers.set(id, {callback, delay: Number(delay) || 0});
    return id;
  }

  function clearTimeoutFake(id) {
    timers.delete(id);
  }

  function runReachabilityTimers() {
    const selected = Array.from(timers.entries())
      .filter(entry => entry[1].delay > 0 && entry[1].delay < 10000);
    selected.forEach(function (entry) {
      timers.delete(entry[0]);
      entry[1].callback();
    });
  }

  function emit(nextSnapshot) {
    snapshot = Object.assign({}, nextSnapshot);
    state = snapshot.state || state;
    subscribers.slice().forEach(callback => callback(snapshot));
  }

  const sourcePanel = {
    element: shell,
    start() {
      createCount += 1;
      sessionId = 'recording_session_' + createCount;
      state = 'playing';
      snapshot = {
        state: 'playing',
        transport: 'progressive-fmp4',
        activeSessionId: sessionId,
        failure: null
      };
      return Promise.resolve(sessionId);
    },
    play() {
      state = 'playing';
      snapshot = Object.assign({}, snapshot, {state: 'playing', failure: null});
      return Promise.resolve(true);
    },
    pause() {
      state = 'paused';
      snapshot = Object.assign({}, snapshot, {state: 'paused'});
      return true;
    },
    stop() {
      state = 'stopped';
      snapshot = Object.assign({}, snapshot, {state: 'stopped'});
      return Promise.resolve(true);
    },
    seekAbsolute(position) {
      seekPositions.push(position);
      state = 'paused';
      snapshot = Object.assign({}, snapshot, {state: 'paused'});
      return Promise.resolve(true);
    },
    position() {
      return video.currentTime;
    },
    snapshot() {
      return snapshot;
    },
    subscribe(callback) {
      subscribers.push(callback);
      return function () {
        const index = subscribers.indexOf(callback);
        if (index >= 0) subscribers.splice(index, 1);
      };
    },
    state() {
      return state;
    },
    sessionId() {
      return sessionId;
    },
    destroy() {}
  };

  const sourceFacade = Object.freeze({
    createPanel() {
      return sourcePanel;
    }
  });

  let playbackValue = sourceFacade;
  const window = {
    document: {},
    navigator: {onLine: true},
    Date,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake,
    fetch(pathname, options) {
      assert.ok(String(pathname).startsWith('/api/vdr/health?recoveryProbe='));
      assert.strictEqual(options.method, 'GET');
      assert.strictEqual(options.credentials, 'same-origin');
      assert.strictEqual(options.cache, 'no-store');
      return reachable
        ? Promise.resolve({ok: true, status: 200})
        : Promise.reject(new Error('network unreachable'));
    },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    removeEventListener(name, callback) {
      if (!listeners[name]) return;
      listeners[name] = listeners[name].filter(entry => entry !== callback);
    },
    VdrSuiteRecordingNetworkRecoveryGuard: Object.freeze({
      guardPlayback(value) { return value; },
      withoutCompatibilityFallback(recording, backendId, callback) {
        assert.strictEqual(recording.id, 'recording-42');
        assert.strictEqual(backendId, 'default');
        return Promise.resolve().then(callback);
      }
    })
  };
  window.window = window;

  Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
    configurable: true,
    enumerable: true,
    get() { return playbackValue; },
    set(value) { playbackValue = value; }
  });

  const context = vm.createContext({
    window,
    document: window.document,
    navigator: window.navigator,
    console,
    Object,
    String,
    Number,
    Boolean,
    Promise,
    RegExp,
    Error,
    Date,
    Math,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake
  });
  vm.runInContext(recoverySource, context, {filename: 'recording-network-recovery.js'});

  const panel = window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-42'},
    'default'
  );

  await panel.start();
  video.dispatch('playing');
  video.currentTime = 47;
  video.dispatch('timeupdate');
  await flush();

  // Real acceptance finding: the browser may continue to claim navigator.onLine
  // and may not emit an offline event even though the Suite origin is unreachable.
  reachable = false;
  assert.strictEqual(window.navigator.onLine, true);

  emit({
    state: 'stopped',
    transport: 'progressive-fmp4',
    activeSessionId: 'recording_session_1',
    failure: {
      category: 'transport',
      origin: 'platform-player',
      stage: 'media-playback',
      terminal: true,
      recoveryClass: 'none',
      reasonCode: 'client_media_network_error'
    }
  });
  await flush(20);

  assert.strictEqual(
    createCount,
    1,
    'classified failure must not create a session until same-origin reachability is proven'
  );
  assert.ok(
    status.textContent.includes('Verbindung unterbrochen'),
    'failed reachability probe must remain visibly recoverable'
  );
  assert.ok(
    Array.from(timers.values()).some(timer => timer.delay === 2000),
    'reachability must be rechecked without relying on a browser online event'
  );

  reachable = true;
  runReachabilityTimers();
  await flush(30);

  assert.strictEqual(
    createCount,
    2,
    'reachability return must request exactly one fresh owner-authorized session'
  );
  assert.deepStrictEqual(
    seekPositions,
    [47],
    'recovery must restore the last canonical absolute position'
  );
  assert.strictEqual(panel.sessionId(), 'recording_session_2');
  assert.strictEqual(panel.state(), 'playing');

  video.dispatch('playing');
  await flush(20);

  assert.ok(
    status.textContent.includes('Verbindung wiederhergestellt'),
    'real media must complete recovery visibly'
  );

  runReachabilityTimers();
  await flush();
  assert.strictEqual(createCount, 2, 'completed recovery must not create additional sessions');

  panel.destroy();
  console.log('phase65d Recording network reachability regression: PASS');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
