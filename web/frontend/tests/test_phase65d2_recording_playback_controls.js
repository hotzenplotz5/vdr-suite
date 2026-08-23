'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'session-frontend-sync.js'),
  'utf8'
);

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
    removeEventListener() {},
    dispatch(name, event) {
      (listeners[name] || []).forEach(callback => callback(event || {target: this}));
    },
    click() { this.dispatch('click', {target: this}); },
    replaceWith(replacement) { this.replacement = replacement; },
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

function descendants(root) {
  const values = [];
  (function walk(item) {
    if (!item) return;
    values.push(item);
    (item.children || []).forEach(walk);
  }(root));
  return values;
}

function find(root, predicate) {
  return descendants(root).find(predicate);
}

function flush() {
  return Promise.resolve().then(() => Promise.resolve()).then(() => Promise.resolve());
}

function createRuntime(options) {
  const settings = Object.assign({seekSupported: true, durationSeconds: 5530}, options || {});
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
    if (!settings.seekSupported) {
      return {
        positionSeconds: position || 0,
        durationSeconds: null,
        seek: {supported: false}
      };
    }
    return {
      positionSeconds: position || 0,
      durationSeconds: settings.durationSeconds,
      seek: {
        supported: true,
        window: {startSeconds: 0, endSeconds: settings.durationSeconds}
      }
    };
  }

  const window = {
    console,
    document,
    performance: {now() { return 1000; }},
    Date,
    setTimeout,
    VdrSuitePublicUrl: {
      resolvePath(path) { return '/vdr-suite' + path; }
    },
    VdrSuiteBrowserSession: {
      subscribe() {},
      csrfHeaders() { return {'X-CSRF-Token': 'csrf-test'}; }
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
      requestJson(path, requestOptions) {
        const body = JSON.parse(requestOptions.body);
        requests.push({path, options: requestOptions, body});
        if (body.operation === 'stop') {
          return Promise.resolve({mediaSession: {id: body.sessionId, state: 'ended'}});
        }
        if (body.operation === 'seek') {
          return Promise.resolve({
            mediaSession: {
              id: body.sessionId,
              state: 'ready',
              backendId: body.backendId,
              recordingId: 'recording-42',
              presentationProfileId: 'progressive-fmp4',
              growing: false,
              mediaPath: '/api/media/sessions/' + body.sessionId + '/recording/stream.mp4',
              playback: playback(body.positionSeconds)
            }
          });
        }
        sessionSequence += 1;
        const id = 'recording_session_' + sessionSequence;
        return Promise.resolve({
          mediaSession: {
            id,
            state: 'ready',
            backendId: body.backendId,
            recordingId: body.recordingId,
            presentationProfileId: 'progressive-fmp4',
            growing: false,
            mediaPath: '/api/media/sessions/' + id + '/recording/stream.mp4',
            playback: playback(0)
          }
        });
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
    setTimeout
  });
  vm.runInContext(source, context, {filename: 'session-frontend-sync.js'});

  return {window, requests, videos};
}

(async function () {
  const runtime = createRuntime();
  const playback = runtime.window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-42', title: 'Seek Test'},
    'living-room'
  );

  assert.strictEqual(typeof playback.play, 'function');
  assert.strictEqual(typeof playback.pause, 'function');
  assert.strictEqual(typeof playback.stop, 'function');
  assert.strictEqual(typeof playback.position, 'function');
  assert.strictEqual(typeof playback.duration, 'function');
  assert.strictEqual(typeof playback.state, 'function');
  assert.strictEqual(typeof playback.seekAbsolute, 'function');
  assert.strictEqual(typeof playback.seekRelative, 'function');

  const id = await playback.start();
  assert.strictEqual(id, 'recording_session_1');
  assert.strictEqual(playback.sessionId(), id);
  assert.strictEqual(playback.duration(), 5530);
  assert.strictEqual(playback.position(), 0);
  assert.strictEqual(playback.state(), 'playing');
  assert.strictEqual(runtime.videos.length, 1);
  const video = runtime.videos[0];
  assert.strictEqual(video.src, '/vdr-suite/api/media/sessions/' + id + '/recording/stream.mp4');

  const timeline = find(playback.element, item => item.tagName === 'INPUT' && item.type === 'range');
  assert.ok(timeline, 'timeline must exist');
  assert.strictEqual(timeline.disabled, false);
  assert.strictEqual(timeline.min, '0');
  assert.strictEqual(timeline.max, '5529');
  const directTime = find(playback.element, item => item.tagName === 'INPUT' && item.type === 'text');
  const directButton = find(playback.element, item => item.tagName === 'BUTTON' && item.textContent === 'Springen');
  assert.ok(directTime && directButton, 'direct time seek controls must exist');
  const positionLabel = find(playback.element, item => item.className === 'recordings2-playback-position');
  assert.strictEqual(positionLabel.textContent, '00:00:00 / 01:32:10');

  video.currentTime = 18;
  video.dispatch('timeupdate');
  assert.strictEqual(playback.position(), 18);
  assert.strictEqual(positionLabel.textContent, '00:00:18 / 01:32:10');

  await playback.seekRelative(10);
  let seekRequests = runtime.requests.filter(entry => entry.body.operation === 'seek');
  assert.strictEqual(seekRequests.length, 1);
  assert.strictEqual(seekRequests[0].body.sessionId, id);
  assert.strictEqual(seekRequests[0].body.positionSeconds, 28);
  assert.strictEqual(playback.sessionId(), id, 'seek must preserve the MediaSession identity');
  assert.strictEqual(playback.position(), 28);
  assert.strictEqual(video.src, '/vdr-suite/api/media/sessions/' + id + '/recording/stream.mp4');

  assert.strictEqual(playback.pause(), true);
  assert.strictEqual(playback.state(), 'paused');
  const playCountBeforePausedSeek = video.played;
  await playback.seekAbsolute(2530);
  assert.strictEqual(playback.position(), 2530);
  assert.strictEqual(playback.state(), 'paused', 'seek must preserve paused state');
  assert.strictEqual(video.played, playCountBeforePausedSeek, 'paused seek must not autoplay a new transport');
  assert.strictEqual(playback.sessionId(), id);

  await playback.play();
  assert.strictEqual(playback.state(), 'playing');
  video.currentTime = 10;
  await playback.seekRelative(-60);
  seekRequests = runtime.requests.filter(entry => entry.body.operation === 'seek');
  assert.strictEqual(seekRequests.at(-1).body.positionSeconds, 2480);

  await playback.seekAbsolute(5);
  await playback.seekRelative(-10);
  seekRequests = runtime.requests.filter(entry => entry.body.operation === 'seek');
  assert.strictEqual(seekRequests.at(-1).body.positionSeconds, 0, 'relative rewind must clamp at window start');

  await playback.seekAbsolute(5529);
  const requestCountBeforeInvalidEnd = runtime.requests.length;
  await assert.rejects(playback.seekAbsolute(5530));
  assert.strictEqual(runtime.requests.length, requestCountBeforeInvalidEnd, 'window end is exclusive');

  timeline.value = '42';
  timeline.dispatch('change');
  await flush();
  seekRequests = runtime.requests.filter(entry => entry.body.operation === 'seek');
  assert.strictEqual(seekRequests.at(-1).body.positionSeconds, 42, 'timeline change must call server seek');

  directTime.value = '00:42:30';
  directButton.click();
  await flush();
  seekRequests = runtime.requests.filter(entry => entry.body.operation === 'seek');
  assert.strictEqual(seekRequests.at(-1).body.positionSeconds, 2550, 'direct time input must call server seek');
  assert.strictEqual(playback.position(), 2550);

  const stopResult = await playback.stop();
  assert.strictEqual(stopResult, true);
  assert.strictEqual(playback.state(), 'stopped');
  const stopRequests = runtime.requests.filter(entry => entry.body.operation === 'stop');
  assert.strictEqual(stopRequests.at(-1).body.sessionId, id);

  const unsupported = createRuntime({seekSupported: false});
  const unsupportedPlayback = unsupported.window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-no-seek'},
    'living-room'
  );
  await unsupportedPlayback.start();
  assert.strictEqual(unsupportedPlayback.duration(), null);
  const unsupportedTimeline = find(
    unsupportedPlayback.element,
    item => item.tagName === 'INPUT' && item.type === 'range'
  );
  assert.strictEqual(unsupportedTimeline.disabled, true);
  const beforeUnsupportedSeek = unsupported.requests.length;
  await assert.rejects(unsupportedPlayback.seekAbsolute(10));
  assert.strictEqual(unsupported.requests.length, beforeUnsupportedSeek);
  unsupportedPlayback.destroy();

  console.log('phase65d2 recording playback controls and truthful seek ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
