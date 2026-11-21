'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const lifecycleSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-owner-lifecycle.js'),
  'utf8'
);
const ownerSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'session-frontend-sync.js'),
  'utf8'
);
const continuitySource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-timeline-continuity.js'),
  'utf8'
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
    appendChild(child) {
      if (child.parentNode && typeof child.parentNode.removeChild === 'function') {
        child.parentNode.removeChild(child);
      }
      child.parentNode = this;
      this.children.push(child);
      return child;
    },
    removeChild(child) {
      const index = this.children.indexOf(child);
      if (index >= 0) this.children.splice(index, 1);
      child.parentNode = null;
      return child;
    },
    replaceChildren() {
      this.children.forEach(child => { child.parentNode = null; });
      this.children = [];
      Array.from(arguments).forEach(child => this.appendChild(child));
    },
    setAttribute(name, val) { this[name] = String(val); },
    removeAttribute(name) { if (name === 'src') this.src = ''; },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    removeEventListener() {},
    dispatch(name, event) {
      (listeners[name] || []).slice().forEach(callback => callback(event || {target: this}));
    },
    click() { this.dispatch('click', {target: this}); },
    replaceWith(replacement) {
      if (!this.parentNode) {
        this.replacement = replacement;
        return;
      }
      const parent = this.parentNode;
      const index = parent.children.indexOf(this);
      if (index < 0) return;
      if (replacement.parentNode && typeof replacement.parentNode.removeChild === 'function') {
        replacement.parentNode.removeChild(replacement);
      }
      parent.children[index] = replacement;
      replacement.parentNode = parent;
      this.parentNode = null;
    },
    pause() { this.paused = true; this.dispatch('pause', {target: this}); },
    load() { this.loaded = (this.loaded || 0) + 1; },
    play() {
      this.paused = false;
      this.played = (this.played || 0) + 1;
      this.dispatch('play', {target: this});
      return Promise.resolve();
    },
    querySelector(selector) {
      const all = descendants(this);
      if (selector === 'video') return all.find(item => item.tagName === 'VIDEO') || null;
      if (selector === '.recordings2-recording-fallback-shell') {
        return all.find(item =>
          String(item.className || '').split(/\s+/).includes('recordings2-recording-fallback-shell')
        ) || null;
      }
      if (selector === '.recordings2-playback-position') {
        return all.find(item =>
          String(item.className || '').split(/\s+/).includes('recordings2-playback-position')
        ) || null;
      }
      const aria = selector.match(/^input\[aria-label="([^"]+)"\]$/);
      if (aria) {
        return all.find(item => item.tagName === 'INPUT' && item['aria-label'] === aria[1]) || null;
      }
      return null;
    }
  };
  Object.defineProperty(value, 'firstChild', {
    get() { return this.children.length ? this.children[0] : null; }
  });
  return value;
}

function find(root, predicate) {
  return descendants(root).find(predicate);
}

function flush(count = 12) {
  let promise = Promise.resolve();
  for (let index = 0; index < count; index += 1) promise = promise.then(() => Promise.resolve());
  return promise;
}

function createRuntime() {
  const requests = [];
  const videos = [];
  let sessionSequence = 0;

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
      positionSeconds: position,
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

  const window = {
    console,
    document,
    performance: {now() { return 1000; }},
    Date,
    setTimeout,
    clearTimeout,
    VdrSuitePublicUrl: {resolvePath(value) { return '/vdr-suite' + value; }},
    VdrSuiteBrowserSession: {
      subscribe() {},
      csrfHeaders() { return {'X-CSRF-Token': 'csrf-timeline'}; }
    },
    VdrSuiteRecordings2Playback: Object.freeze({
      createPanel() {
        return Object.freeze({
          element: node('section'),
          start() { return Promise.resolve('legacy-session'); },
          destroy() {},
          sessionId() { return 'legacy-session'; }
        });
      }
    }),
    VdrSuiteClientApi: {
      requestJson(requestPath, requestOptions) {
        const body = JSON.parse(requestOptions.body);
        requests.push({path: requestPath, body});
        if (body.operation === 'stop') {
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
          return Promise.resolve(recordingSession(
            body.sessionId,
            body.backendId,
            'recording-42',
            0
          ));
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
    addEventListener() {},
    removeEventListener() {}
  };
  window.window = window;

  const context = vm.createContext({
    window,
    document,
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
  vm.runInContext(lifecycleSource, context, {filename: 'playback-owner-lifecycle.js'});
  vm.runInContext(ownerSource, context, {filename: 'session-frontend-sync.js'});
  vm.runInContext(continuitySource, context, {filename: 'playback-timeline-continuity.js'});

  return {window, requests, videos};
}

(async function () {
  const runtime = createRuntime();
  const playback = runtime.window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-42', title: 'Timeline continuity'},
    'living-room'
  );

  assert.strictEqual(playback.__vdrSuitePlaybackTimelineContinuityDecorated, true);
  assert.strictEqual(typeof playback.presentationBasePosition, 'function');
  assert.strictEqual(playback.snapshot().continuity.generation, 0);
  assert.strictEqual(playback.snapshot().presentationBasePositionSeconds, 0);

  // Exercise the real owner-internal visible Start action. The test must not
  // shortcut lifecycle truth through an intercepted/decorated start() method.
  const startButton = find(
    playback.element,
    item => item.tagName === 'BUTTON' && item.className === 'recordings2-primary'
  );
  assert.ok(startButton, 'production Recording owner must expose the visible Start action');
  startButton.dispatch('click', {target: startButton});
  await flush(16);

  const id = playback.sessionId();
  assert.strictEqual(id, 'recording_session_1');
  assert.strictEqual(playback.snapshot().continuity.generation, 1);
  assert.strictEqual(playback.snapshot().continuity.state, 'stable');
  assert.strictEqual(playback.presentationBasePosition(), 0);

  const video = runtime.videos[0];
  assert.ok(video, 'production Recording owner must own one progressive media element');
  video.currentTime = 18;
  video.dispatch('timeupdate', {target: video});
  assert.strictEqual(playback.position(), 18);
  assert.strictEqual(playback.presentationBasePosition(), 0);
  assert.strictEqual(
    playback.position(),
    playback.presentationBasePosition() + Math.floor(video.currentTime),
    'ordinary playback must expose absolute Recording position as base + transport-local time'
  );

  const timeline = find(
    playback.element,
    item => item.tagName === 'INPUT' && item.type === 'range'
  );
  const positionLabel = find(
    playback.element,
    item => item.className === 'recordings2-playback-position'
  );
  assert.ok(timeline && positionLabel);

  // Production bug proof: owner timeupdate continues while the user owns the
  // range preview. The Slice-3 adapter must restore the user target after the
  // already-installed owner handler until change/pointercancel releases it.
  timeline.value = '420';
  timeline.dispatch('input', {target: timeline});
  assert.strictEqual(positionLabel.textContent, '00:07:00 / 01:32:10');
  video.currentTime = 19;
  video.dispatch('timeupdate', {target: video});
  await flush();
  assert.strictEqual(timeline.value, '420');
  assert.strictEqual(positionLabel.textContent, '00:07:00 / 01:32:10');

  const beforeSeekRevision = playback.snapshot().lifecycleRevision;
  timeline.dispatch('change', {target: timeline});
  await flush(20);
  const seekRequests = runtime.requests.filter(entry => entry.body.operation === 'seek');
  assert.strictEqual(seekRequests.length, 1);
  assert.strictEqual(seekRequests[0].body.sessionId, id);
  assert.strictEqual(seekRequests[0].body.positionSeconds, 420);
  assert.strictEqual(playback.sessionId(), id, 'in-session seek must preserve MediaSession identity');
  assert.strictEqual(playback.position(), 420);
  assert.strictEqual(playback.presentationBasePosition(), 420);
  assert.strictEqual(playback.snapshot().presentationBasePositionSeconds, 420);
  assert.strictEqual(playback.snapshot().continuity.generation, 2);
  assert.ok(
    playback.snapshot().lifecycleRevision > beforeSeekRevision,
    'lifecycle ordering must advance independently from presentation generation'
  );

  video.currentTime = 7;
  video.dispatch('timeupdate', {target: video});
  assert.strictEqual(playback.position(), 427);
  assert.strictEqual(playback.presentationBasePosition(), 420);
  assert.strictEqual(
    playback.position(),
    playback.presentationBasePosition() + Math.floor(video.currentTime)
  );

  // A same-element DOM reparent is not a decoder replacement. No lifecycle
  // publication occurs, so the playback-presentation generation stays stable.
  const generationBeforeReparent = playback.snapshot().continuity.generation;
  const holder = node('div');
  playback.element.appendChild(holder);
  holder.appendChild(video);
  assert.strictEqual(playback.snapshot().continuity.generation, generationBeforeReparent);
  assert.strictEqual(playback.presentationBasePosition(), 420);

  // Cancelled preview ownership returns the timeline to active playback.
  timeline.value = '900';
  timeline.dispatch('input', {target: timeline});
  timeline.dispatch('pointercancel', {target: timeline});
  video.currentTime = 8;
  video.dispatch('timeupdate', {target: video});
  await flush();
  assert.strictEqual(timeline.value, '428');

  console.log('phase65d Slice 3 progressive timeline continuity production composition ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
