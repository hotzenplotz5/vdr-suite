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

assert.ok(recoverySource.includes('panel.subscribe(ownerChanged)'), 'recovery must observe canonical owner lifecycle');
assert.ok(recoverySource.includes('panel.seekAbsolute(interruptedPosition)'), 'recovery must restore canonical absolute position through owner seek');
assert.ok(recoverySource.includes('panel.start()'), 'recovery must delegate fresh authorization to the existing owner');
assert.ok(recoverySource.includes('withoutCompatibilityFallback'), 'recovery must suppress hidden HLS fallback during the recovery attempt');
assert.ok(!recoverySource.includes('VdrSuiteClientApi'), 'recovery policy must not create a parallel client API/session path');
assert.ok(!recoverySource.includes('/api/media/sessions'), 'recovery policy must not issue MediaSession requests directly');
assert.ok(!recoverySource.includes('createRecordingSession'), 'recovery policy must not duplicate the Recording session owner');
assert.ok(!recoverySource.includes('activateFallback'), 'recovery policy must not own compatibility fallback');

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
  if (selector === 'input[aria-label="Wiedergabeposition"]') {
    return item.tagName === 'INPUT' && item['aria-label'] === 'Wiedergabeposition';
  }
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
    querySelector(selector) { return descendants(this).find(item => item !== this && matches(item, selector)) || null; },
    pause() { this.paused = true; this.dispatch('pause', {target: this}); },
    load() { this.loaded = (this.loaded || 0) + 1; },
    play() {
      this.paused = false;
      this.played = (this.played || 0) + 1;
      this.dispatch('play', {target: this});
      return Promise.resolve();
    }
  };
  return value;
}

function flush(rounds) {
  let promise = Promise.resolve();
  for (let index = 0; index < (rounds || 10); index += 1) {
    promise = promise.then(() => Promise.resolve());
  }
  return promise;
}

function createRuntime(options) {
  const settings = Object.assign({lateLegacyAssignment: false, failRecoveryCreate: false}, options || {});
  const requests = [];
  const videos = [];
  const globalListeners = {};
  const navigator = {onLine: true};
  let sessionSequence = 0;
  let createAttempts = 0;
  let legacyStarts = 0;

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
      element.className = 'legacy-hls-panel';
      return Object.freeze({
        element,
        start() { legacyStarts += 1; return Promise.resolve('legacy-session'); },
        stop() { return Promise.resolve(true); },
        destroy() {},
        state() { return 'playing'; },
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
    setTimeout,
    clearTimeout,
    VdrSuitePublicUrl: {resolvePath(value) { return '/vdr-suite' + value; }},
    VdrSuiteBrowserSession: {
      subscribe() {},
      csrfHeaders() { return {'X-CSRF-Token': 'csrf-test'}; }
    },
    VdrSuiteClientApi: {
      requestJson(pathname, requestOptions) {
        const body = JSON.parse(requestOptions.body);
        requests.push({path: pathname, options: requestOptions, body});
        if (body.operation === 'stop') {
          if (!navigator.onLine) return Promise.reject(new Error('network error'));
          return Promise.resolve({mediaSession: {id: body.sessionId, state: 'ended'}});
        }
        if (body.operation === 'seek') {
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
        if (settings.failRecoveryCreate && createAttempts === 2) {
          return Promise.reject(new Error('media worker unavailable'));
        }
        sessionSequence += 1;
        return Promise.resolve(recordingSession(
          'recording_session_' + sessionSequence,
          body.backendId,
          body.recordingId,
          0
        ));
      }
    },
    fetch() { return Promise.resolve({ok: true}); },
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
  if (!settings.lateLegacyAssignment) window.VdrSuiteRecordings2Playback = legacyFacade;

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
    setTimeout,
    clearTimeout
  });

  vm.runInContext(guardSource, context, {filename: 'recording-network-recovery-guard.js'});
  vm.runInContext(lifecycleSource, context, {filename: 'playback-owner-lifecycle.js'});
  vm.runInContext(failureSource, context, {filename: 'playback-failure-classification.js'});
  vm.runInContext(syncSource, context, {filename: 'session-frontend-sync.js'});
  vm.runInContext(timelineSource, context, {filename: 'playback-timeline-continuity.js'});
  vm.runInContext(recoverySource, context, {filename: 'recording-network-recovery.js'});

  if (settings.lateLegacyAssignment) window.VdrSuiteRecordings2Playback = legacyFacade;

  function dispatchGlobal(name) {
    (globalListeners[name] || []).slice().forEach(callback => callback({type: name}));
  }

  return {
    window,
    navigator,
    requests,
    videos,
    dispatchGlobal,
    createAttempts() { return createAttempts; },
    legacyStarts() { return legacyStarts; }
  };
}

async function successfulRecovery(lateLegacyAssignment) {
  const runtime = createRuntime({lateLegacyAssignment});
  const panel = runtime.window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-42', title: 'Network Recovery'},
    'living-room'
  );
  const firstSessionId = await panel.start();
  const video = runtime.videos[0];
  video.dispatch('playing');
  await flush();
  const initialGeneration = panel.snapshot().continuity.generation;

  video.currentTime = 37;
  video.dispatch('timeupdate');
  runtime.navigator.onLine = false;
  runtime.dispatchGlobal('offline');
  video.error = {code: 2, message: 'network error'};
  video.dispatch('error');
  await flush();

  const status = panel.element.querySelector('.recordings2-playback-status');
  assert.strictEqual(panel.state(), 'stopped', 'canonical owner must first stop the failed transport');
  assert.ok(status.textContent.includes('Verbindung unterbrochen'), 'offline failure must become a recoverable interruption in the owner UI');
  assert.strictEqual(runtime.createAttempts(), 1, 'no replacement MediaSession may be created while offline');
  assert.strictEqual(runtime.legacyStarts(), 0, 'post-start network interruption must not switch to HLS');

  runtime.navigator.onLine = true;
  runtime.dispatchGlobal('online');
  await flush(20);

  assert.strictEqual(runtime.createAttempts(), 2, 'online transition must request exactly one fresh authorized MediaSession');
  const createRequests = runtime.requests.filter(entry => !entry.body.operation);
  assert.strictEqual(createRequests.length, 2);
  const secondSessionId = 'recording_session_2';
  const seeks = runtime.requests.filter(entry => entry.body.operation === 'seek');
  assert.strictEqual(seeks.length, 1, 'recovery must reposition through the existing authoritative seek operation');
  assert.strictEqual(seeks[0].body.sessionId, secondSessionId);
  assert.strictEqual(seeks[0].body.positionSeconds, 37, 'recovery must restore the last canonical absolute Recording position');
  assert.notStrictEqual(firstSessionId, secondSessionId);
  assert.strictEqual(runtime.legacyStarts(), 0);

  video.dispatch('playing');
  await flush();
  assert.strictEqual(panel.sessionId(), secondSessionId);
  assert.strictEqual(panel.state(), 'playing');
  assert.ok(status.textContent.includes('Verbindung wiederhergestellt'), 'real media must complete recovery visibly');
  assert.ok(panel.snapshot().continuity.generation > initialGeneration, 'replacement/reposition must advance playback presentation continuity');

  runtime.dispatchGlobal('online');
  await flush();
  assert.strictEqual(runtime.createAttempts(), 2, 'duplicate online events in the same network epoch must not create parallel sessions');
  panel.destroy();
}

async function decoderRemainsTerminal() {
  const runtime = createRuntime();
  const panel = runtime.window.VdrSuiteRecordings2Playback.createPanel({id: 'recording-42'}, 'living-room');
  await panel.start();
  const video = runtime.videos[0];
  video.dispatch('playing');
  video.currentTime = 20;
  video.dispatch('timeupdate');
  runtime.navigator.onLine = false;
  runtime.dispatchGlobal('offline');
  video.error = {code: 3, message: 'decoder stopped'};
  video.dispatch('error');
  await flush();
  runtime.navigator.onLine = true;
  runtime.dispatchGlobal('online');
  await flush();

  assert.strictEqual(panel.state(), 'stopped');
  assert.strictEqual(panel.snapshot().failure.reasonCode, 'client_media_decode_error');
  assert.strictEqual(runtime.createAttempts(), 1, 'decoder failure must not become network recovery merely because the browser is offline');
  assert.strictEqual(runtime.legacyStarts(), 0);
  panel.destroy();
}

async function startupFallbackRemainsAvailable() {
  const runtime = createRuntime();
  const panel = runtime.window.VdrSuiteRecordings2Playback.createPanel({id: 'recording-42'}, 'living-room');
  await panel.start();
  const video = runtime.videos[0];
  video.error = {code: 3, message: 'decode before first media'};
  video.dispatch('error');
  await flush();
  assert.strictEqual(panel.state(), 'fallback', 'pre-first-media failure must retain existing compatibility fallback');
  assert.strictEqual(runtime.legacyStarts(), 1, 'ordinary startup fallback must not be suppressed by the recovery guard');
  panel.destroy();
}

async function failedRecoveryDoesNotFallback() {
  const runtime = createRuntime({failRecoveryCreate: true});
  const panel = runtime.window.VdrSuiteRecordings2Playback.createPanel({id: 'recording-42'}, 'living-room');
  await panel.start();
  const video = runtime.videos[0];
  video.dispatch('playing');
  video.currentTime = 22;
  video.dispatch('timeupdate');
  runtime.navigator.onLine = false;
  runtime.dispatchGlobal('offline');
  video.error = {code: 2, message: 'network error'};
  video.dispatch('error');
  await flush();
  runtime.navigator.onLine = true;
  runtime.dispatchGlobal('online');
  await flush(20);

  const status = panel.element.querySelector('.recordings2-playback-status');
  assert.strictEqual(runtime.createAttempts(), 2, 'one recovery authorization attempt is allowed for the network epoch');
  assert.strictEqual(runtime.legacyStarts(), 0, 'failed automatic recovery must not activate compatibility HLS');
  assert.strictEqual(panel.snapshot().state, 'stopped', 'canonical lifecycle must remain terminal after failed recovery');
  assert.ok(status.textContent.includes('nicht automatisch fortgesetzt'), 'failed online recovery must become terminal and visible');
  runtime.dispatchGlobal('online');
  await flush();
  assert.strictEqual(runtime.createAttempts(), 2, 'failed recovery must not spin an unbounded retry loop');
  panel.destroy();
}

(async function () {
  await successfulRecovery(false);
  await successfulRecovery(true);
  await decoderRemainsTerminal();
  await startupFallbackRemainsAvailable();
  await failedRecoveryDoesNotFallback();
  console.log('phase65d Recording network interruption recovery semantics ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
