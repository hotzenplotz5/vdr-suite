'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

function source(name) {
  return fs.readFileSync(path.join(__dirname, '..', 'api', name), 'utf8');
}

const guardSource = source('recording-network-recovery-guard.js');
const lifecycleSource = source('playback-owner-lifecycle.js');
const failureSource = source('playback-failure-classification.js');
const syncSource = source('session-frontend-sync.js');
const timelineSource = source('playback-timeline-continuity.js');
const recoverySource = source('recording-network-recovery.js');

assert.ok(
  syncSource.includes('RECORDING_POST_START_WAITING_LIVENESS_MS'),
  'Recording owner must bound post-start waiting liveness'
);
assert.ok(
  syncSource.includes('X-VDR-Suite-Playback-Liveness-Probe'),
  'stall liveness must use an observational same-origin probe'
);
assert.ok(
  recoverySource.includes('panel.start({autoPlay: false})'),
  'recovery must create the fresh owner session without start-time autoplay'
);
const recoverySequenceStart = recoverySource.indexOf('function recoverySequence()');
const recoverySequenceEnd = recoverySource.indexOf('\n    function finishRecoverySuccess()', recoverySequenceStart);
assert.ok(recoverySequenceStart >= 0 && recoverySequenceEnd > recoverySequenceStart);
const recoverySequenceSource = recoverySource.slice(recoverySequenceStart, recoverySequenceEnd);
assert.ok(
  !recoverySequenceSource.includes('panel.pause()'),
  'recovery must not interrupt its own start-time play promise with pause()'
);

function descendants(root) {
  const values = [];
  (function walk(item) {
    if (!item) return;
    values.push(item);
    (item.children || []).forEach(walk);
  }(root));
  return values;
}

function hasClass(item, className) {
  return String(item && item.className || '').split(/\s+/).includes(className);
}

function matches(item, selector) {
  if (!item) return false;
  if (selector === 'video') return item.tagName === 'VIDEO';
  if (selector.startsWith('.')) return hasClass(item, selector.slice(1));
  return false;
}

function node(tagName) {
  const listeners = {};
  const value = {
    tagName: String(tagName || '').toUpperCase(),
    children: [],
    className: '',
    classList: {toggle() {}, add() {}, remove() {}},
    style: {},
    dataset: {},
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
    played: 0,
    pausedCalls: 0,
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
    setAttribute(name, val) { this[name] = String(val); },
    removeAttribute(name) { if (name === 'src') this.src = ''; },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    removeEventListener(name, callback) {
      if (!listeners[name]) return;
      listeners[name] = listeners[name].filter(entry => entry !== callback);
    },
    dispatch(name, event) {
      (listeners[name] || []).slice().forEach(callback => callback(event || {target: this}));
    },
    click() { this.dispatch('click', {target: this}); },
    replaceWith(replacement) { this.replacement = replacement; },
    querySelector(selector) {
      return descendants(this).find(item => item !== this && matches(item, selector)) || null;
    },
    pause() {
      this.pausedCalls += 1;
      this.paused = true;
      this.dispatch('pause', {target: this});
    },
    load() { this.loaded = (this.loaded || 0) + 1; },
    play() {
      this.paused = false;
      this.played += 1;
      this.dispatch('play', {target: this});
      return Promise.resolve();
    }
  };
  return value;
}

function flush(rounds) {
  let promise = Promise.resolve();
  for (let index = 0; index < (rounds || 20); index += 1) {
    promise = promise.then(() => Promise.resolve());
  }
  return promise;
}

function createRuntime() {
  const requests = [];
  const videos = [];
  const timers = new Map();
  const globalListeners = {};
  const navigator = {onLine: true};
  let nextTimerId = 1;
  let originReachable = true;
  let sessionSequence = 0;
  let createAttempts = 0;
  let legacyStarts = 0;
  let resolveRecoverySeek = null;

  function setTimeoutFake(callback, delay) {
    const id = nextTimerId++;
    timers.set(id, {callback, delay: Number(delay) || 0});
    return id;
  }

  function clearTimeoutFake(id) {
    timers.delete(id);
  }

  function runOneTimer(delay) {
    const entry = Array.from(timers.entries()).find(item => item[1].delay === delay);
    assert.ok(entry, 'expected timer with delay ' + delay + ' ms');
    timers.delete(entry[0]);
    entry[1].callback();
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

  function playback(position) {
    return {
      positionSeconds: position || 0,
      durationSeconds: 5530,
      seek: {
        supported: true,
        preparing: false,
        window: {startSeconds: 0, endSeconds: 5530}
      },
      resume: {supported: true, preparing: false}
    };
  }

  function recordingSession(id, backendId, recordingId, position) {
    return {
      mediaSession: {
        id,
        state: 'ready',
        backendId,
        recordingId,
        presentationProfileId: 'progressive-fmp4',
        growing: false,
        mediaPath: '/api/media/sessions/' + id + '/recording/stream.mp4',
        playback: playback(position)
      }
    };
  }

  const legacyFacade = Object.freeze({
    createPanel() {
      const element = node('section');
      return Object.freeze({
        element,
        start() { legacyStarts += 1; return Promise.resolve('legacy-session'); },
        stop() { return Promise.resolve(true); },
        destroy() {},
        sessionId() { return 'legacy-session'; }
      });
    }
  });

  const window = {
    console,
    document,
    navigator,
    performance: {now() { return 1000; }},
    Date,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake,
    VdrSuitePublicUrl: {resolvePath(value) { return '/vdr-suite' + value; }},
    VdrSuiteBrowserSession: {
      subscribe() {},
      csrfHeaders() { return {'X-CSRF-Token': 'csrf-test'}; }
    },
    VdrSuiteRecordings2Playback: legacyFacade,
    VdrSuiteClientApi: {
      requestJson(pathname, requestOptions) {
        const body = JSON.parse(requestOptions.body);
        requests.push({path: pathname, options: requestOptions, body});
        if (body.operation === 'stop') {
          return originReachable
            ? Promise.resolve({mediaSession: {id: body.sessionId, state: 'ended'}})
            : Promise.reject(new Error('network unreachable'));
        }
        if (body.operation === 'seek') {
          if (body.sessionId === 'recording_session_2') {
            return new Promise(resolve => {
              resolveRecoverySeek = function () {
                resolve(recordingSession(
                  body.sessionId,
                  body.backendId,
                  'recording-42',
                  body.positionSeconds
                ));
              };
            });
          }
          return Promise.resolve(recordingSession(
            body.sessionId,
            body.backendId,
            'recording-42',
            body.positionSeconds
          ));
        }
        if (body.operation === 'playback-status') {
          return Promise.resolve(recordingSession(body.sessionId, body.backendId, 'recording-42', 0));
        }
        createAttempts += 1;
        sessionSequence += 1;
        return Promise.resolve(recordingSession(
          'recording_session_' + sessionSequence,
          body.backendId,
          body.recordingId,
          0
        ));
      }
    },
    fetch(pathname, options) {
      const pathValue = String(pathname);
      if (pathValue.includes('playbackLivenessProbe=')) {
        assert.strictEqual(options.method, 'GET');
        assert.strictEqual(options.credentials, 'same-origin');
        assert.strictEqual(options.cache, 'no-store');
        assert.strictEqual(options.headers['X-VDR-Suite-Playback-Liveness-Probe'], '1');
      }
      else if (pathValue.includes('recoveryProbe=')) {
        assert.strictEqual(options.method, 'GET');
        assert.strictEqual(options.credentials, 'same-origin');
        assert.strictEqual(options.cache, 'no-store');
        assert.strictEqual(options.headers['X-VDR-Suite-Recovery-Probe'], '1');
      }
      else {
        throw new Error('unexpected fetch path: ' + pathValue);
      }
      return originReachable
        ? Promise.resolve({ok: true, status: 200})
        : Promise.reject(new Error('origin unreachable'));
    },
    addEventListener(name, callback) {
      if (!globalListeners[name]) globalListeners[name] = [];
      globalListeners[name].push(callback);
    },
    removeEventListener(name, callback) {
      if (!globalListeners[name]) return;
      globalListeners[name] = globalListeners[name].filter(entry => entry !== callback);
    }
  };
  window.window = window;

  const context = vm.createContext({
    window,
    document,
    navigator,
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
    Date,
    Set,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake
  });

  vm.runInContext(guardSource, context, {filename: 'recording-network-recovery-guard.js'});
  vm.runInContext(lifecycleSource, context, {filename: 'playback-owner-lifecycle.js'});
  vm.runInContext(failureSource, context, {filename: 'playback-failure-classification.js'});
  vm.runInContext(syncSource, context, {filename: 'session-frontend-sync.js'});
  vm.runInContext(timelineSource, context, {filename: 'playback-timeline-continuity.js'});
  vm.runInContext(recoverySource, context, {filename: 'recording-network-recovery.js'});

  return {
    window,
    navigator,
    requests,
    videos,
    runOneTimer,
    setOriginReachable(value) { originReachable = Boolean(value); },
    resolveRecoverySeek() {
      assert.strictEqual(typeof resolveRecoverySeek, 'function', 'recovery seek must be pending');
      const resolve = resolveRecoverySeek;
      resolveRecoverySeek = null;
      resolve();
    },
    createAttempts() { return createAttempts; },
    legacyStarts() { return legacyStarts; }
  };
}

(async function () {
  const runtime = createRuntime();
  const panel = runtime.window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-42', title: 'Stall Recovery'},
    'living-room'
  );

  const firstSessionId = await panel.start();
  assert.strictEqual(firstSessionId, 'recording_session_1');
  const video = runtime.videos[0];
  video.dispatch('playing');
  video.currentTime = 37;
  video.dispatch('timeupdate');
  await flush();
  assert.strictEqual(video.played, 1, 'ordinary initial start must autoplay exactly once');

  // A long waiting period alone is not a network failure. If the Suite origin
  // remains reachable, the owner keeps the established presentation alive.
  video.dispatch('waiting');
  runtime.runOneTimer(8000);
  await flush();
  assert.strictEqual(panel.state(), 'playing', 'reachable-origin buffering must not trigger recovery');
  assert.strictEqual(runtime.createAttempts(), 1);
  assert.strictEqual(runtime.legacyStarts(), 0);

  // Reproduce the real Android/Edge finding: navigator.onLine stays true, no
  // offline event arrives, no fetch reader/media error fires, and playback is
  // stuck in waiting with no timeline progress while the Suite origin is gone.
  runtime.setOriginReachable(false);
  assert.strictEqual(runtime.navigator.onLine, true);
  video.dispatch('waiting');
  runtime.runOneTimer(8000);
  await flush(30);

  assert.strictEqual(panel.state(), 'stopped', 'proven post-start network stall must stop the failed owner transport');
  assert.strictEqual(panel.snapshot().failure.reasonCode, 'client_transport_failed');
  assert.strictEqual(runtime.createAttempts(), 1, 'unreachable origin must not create a replacement MediaSession');
  assert.strictEqual(runtime.legacyStarts(), 0, 'post-start stall recovery must not activate HLS');
  const status = panel.element.querySelector('.recordings2-playback-status');
  assert.ok(status.textContent.includes('Verbindung unterbrochen'));

  runtime.setOriginReachable(true);
  runtime.runOneTimer(2000);
  await flush(30);

  assert.strictEqual(runtime.createAttempts(), 2, 'reachability return must create exactly one fresh owner session');
  const seeks = runtime.requests.filter(entry => entry.body.operation === 'seek');
  assert.strictEqual(seeks.length, 1);
  assert.strictEqual(seeks[0].body.sessionId, 'recording_session_2');
  assert.strictEqual(seeks[0].body.positionSeconds, 37);
  assert.strictEqual(video.played, 1, 'recovery session start must remain paused until the authoritative seek completes');

  runtime.resolveRecoverySeek();
  await flush(30);
  assert.strictEqual(video.played, 2, 'recovery must issue exactly one play after the seek');

  video.dispatch('playing');
  await flush(20);
  assert.strictEqual(panel.sessionId(), 'recording_session_2');
  assert.strictEqual(panel.state(), 'playing');
  assert.ok(status.textContent.includes('Verbindung wiederhergestellt'));
  assert.strictEqual(runtime.legacyStarts(), 0);

  panel.destroy();
  console.log('phase65d Recording post-start stall recovery regression: PASS');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
