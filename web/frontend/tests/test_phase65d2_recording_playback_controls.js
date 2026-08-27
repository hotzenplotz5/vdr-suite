'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'session-frontend-sync.js'),
  'utf8'
);
const lifecycleSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-owner-lifecycle.js'),
  'utf8'
);
const failureClassificationSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-failure-classification.js'),
  'utf8'
);

const connectRecordingStart = source.indexOf('function connectRecordingStream(autoPlay, initialConnection)');
const pageHideStart = source.indexOf('\n    function pageHide()', connectRecordingStart);
assert.ok(connectRecordingStart >= 0 && pageHideStart > connectRecordingStart, 'Recording stream connection owner must exist');
const connectRecordingSource = source.slice(connectRecordingStart, pageHideStart);
assert.ok(
  connectRecordingSource.includes('if (initialConnection && !firstMediaReported) activateFallback(error);'),
  'continuous-fMP4 fallback must remain a startup-only rescue before first media'
);
assert.ok(
  !connectRecordingSource.includes('if (initialConnection) activateFallback(error);'),
  'a late continuous-fMP4 failure must not silently switch an already-playing Recording to HLS'
);
assert.ok(
  connectRecordingSource.includes('classifyClientTransportError(error)'),
  'post-start continuous-fMP4 failures must publish classified transport/buffer evidence'
);
assert.ok(
  connectRecordingSource.includes('else failStartedPlayback('),
  'post-start continuous-fMP4 failures must remain terminal owner actions'
);
assert.ok(
  source.includes("? 'Aufnahme-Wiedergabe wurde nach dem Start unterbrochen: ' + error.message"),
  'ordinary post-start playback failures must not claim that a seek happened'
);
assert.ok(
  source.includes("? 'Seek wurde serverseitig ausgeführt, aber der neue Stream konnte nicht wiedergegeben werden: ' + error.message"),
  'a stream failure after an actually confirmed seek must retain specific reposition diagnostics'
);
assert.ok(
  !source.includes('automatisch zurückgesetzt'),
  'cleanup/reset consequences must not be presented as the Recording failure cause'
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
  const settings = Object.assign({
    seekSupported: true,
    durationSeconds: 5530,
    indexPreparing: false,
    immediateTimers: false,
    indexReadyAfterStatusCalls: 1
  }, options || {});
  const requests = [];
  const videos = [];
  let sessionSequence = 0;
  let playbackStatusCalls = 0;

  const schedule = settings.immediateTimers
    ? function (callback) { Promise.resolve().then(callback); return 1; }
    : setTimeout;
  const cancelSchedule = settings.immediateTimers
    ? function () {}
    : clearTimeout;

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

  function playback(position, indexReady) {
    if (settings.indexPreparing && !indexReady) {
      return {
        positionSeconds: position || 0,
        durationSeconds: null,
        seek: {supported: false, preparing: true}
      };
    }
    if (!settings.seekSupported) {
      return {
        positionSeconds: position || 0,
        durationSeconds: null,
        seek: {supported: false, preparing: false}
      };
    }
    return {
      positionSeconds: position || 0,
      durationSeconds: settings.durationSeconds,
      seek: {
        supported: true,
        preparing: false,
        window: {startSeconds: 0, endSeconds: settings.durationSeconds}
      }
    };
  }

  function recordingSession(id, backendId, recordingId, playbackValue) {
    return {
      mediaSession: {
        id,
        state: 'ready',
        backendId,
        recordingId,
        presentationProfileId: 'progressive-fmp4',
        growing: false,
        mediaPath: '/api/media/sessions/' + id + '/recording/stream.mp4',
        playback: playbackValue
      }
    };
  }

  const window = {
    console,
    document,
    performance: {now() { return 1000; }},
    Date,
    setTimeout: schedule,
    clearTimeout: cancelSchedule,
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
          return Promise.resolve(recordingSession(
            body.sessionId,
            body.backendId,
            'recording-42',
            playback(body.positionSeconds, true)
          ));
        }
        if (body.operation === 'playback-status') {
          playbackStatusCalls += 1;
          const ready = playbackStatusCalls >= settings.indexReadyAfterStatusCalls;
          return Promise.resolve(recordingSession(
            body.sessionId,
            body.backendId,
            'recording-42',
            playback(0, ready)
          ));
        }
        sessionSequence += 1;
        const id = 'recording_session_' + sessionSequence;
        return Promise.resolve(recordingSession(
          id,
          body.backendId,
          body.recordingId,
          playback(0, !settings.indexPreparing)
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
    setTimeout: schedule,
    clearTimeout: cancelSchedule
  });
  vm.runInContext(lifecycleSource, context, {filename: 'playback-owner-lifecycle.js'});
  vm.runInContext(
    failureClassificationSource,
    context,
    {filename: 'playback-failure-classification.js'}
  );
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

  const preparing = createRuntime({indexPreparing: true, immediateTimers: true});
  const preparingPlayback = preparing.window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-missing-index'},
    'living-room'
  );
  const preparingId = await preparingPlayback.start();
  await flush();
  await flush();
  const createRequests = preparing.requests.filter(entry => !entry.body.operation);
  const statusRequests = preparing.requests.filter(entry => entry.body.operation === 'playback-status');
  assert.strictEqual(createRequests.length, 1, 'lazy index activation must not create a second MediaSession');
  assert.strictEqual(statusRequests.length, 1, 'lazy index activation must poll the owned MediaSession');
  assert.strictEqual(statusRequests[0].body.sessionId, preparingId);
  assert.strictEqual(preparingPlayback.sessionId(), preparingId, 'index activation must preserve MediaSession identity');
  assert.strictEqual(preparingPlayback.duration(), 5530, 'duration must become available after index activation');
  const preparingTimeline = find(
    preparingPlayback.element,
    item => item.tagName === 'INPUT' && item.type === 'range'
  );
  assert.strictEqual(preparingTimeline.disabled, false, 'timeline must activate without reload');
  assert.strictEqual(preparingTimeline.max, '5529');
  await preparingPlayback.seekAbsolute(60);
  const preparingSeek = preparing.requests.filter(entry => entry.body.operation === 'seek').at(-1);
  assert.strictEqual(preparingSeek.body.sessionId, preparingId);
  assert.strictEqual(preparingSeek.body.positionSeconds, 60);
  preparingPlayback.destroy();

  const startupFailure = createRuntime();
  const startupPlayback = startupFailure.window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-startup-failure'},
    'living-room'
  );
  await startupPlayback.start();
  const startupVideo = startupFailure.videos[0];
  startupVideo.error = {code: 3, message: 'decode before first frame'};
  startupVideo.dispatch('error');
  await flush();
  assert.strictEqual(startupPlayback.state(), 'fallback', 'failure before first media must still activate startup fallback');
  assert.strictEqual(
    startupPlayback.snapshot().failure,
    null,
    'classification must not turn the existing startup fallback decision into a terminal failure publication'
  );

  const lateFailure = createRuntime();
  const latePlayback = lateFailure.window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-late-failure'},
    'living-room'
  );
  const lateId = await latePlayback.start();
  const lateVideo = lateFailure.videos[0];
  lateVideo.dispatch('playing');
  lateVideo.error = {code: 3, message: 'decoder stopped'};
  lateVideo.dispatch('error');
  await flush();
  const lateStatus = find(latePlayback.element, item => item.className === 'recordings2-playback-status');
  assert.ok(lateStatus.textContent.includes('Aufnahme-Wiedergabe wurde nach dem Start unterbrochen'));
  assert.ok(lateStatus.textContent.includes('decoder stopped'), 'media/browser cause must remain visible');
  assert.ok(!lateStatus.textContent.includes('Seek wurde'), 'ordinary late playback failure must not invent a seek');
  assert.ok(!lateStatus.textContent.includes('Cleanup'), 'cleanup must not be presented as the playback cause');
  assert.ok(!lateStatus.textContent.includes('zurückgesetzt'), 'reset/cleanup consequences must not be presented as the cause');
  assert.strictEqual(latePlayback.state(), 'stopped');
  assert.deepStrictEqual(
    JSON.parse(JSON.stringify(latePlayback.snapshot().failure)),
    {
      category: 'decoder',
      origin: 'platform-player',
      stage: 'decode',
      terminal: true,
      recoveryClass: 'none',
      reasonCode: 'client_media_decode_error'
    },
    'post-start platform decoder error must publish canonical Slice-4 failure evidence'
  );
  assert.ok(
    lateFailure.requests.some(entry => entry.body.operation === 'stop' && entry.body.sessionId === lateId),
    'failure-stop must still clean up the active MediaSession'
  );
  assert.strictEqual(
    lateFailure.requests.filter(entry => !entry.body.operation).length,
    1,
    'failure classification must not create a replacement MediaSession'
  );

  const seekFailure = createRuntime();
  const seekFailurePlayback = seekFailure.window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-seek-stream-failure'},
    'living-room'
  );
  const seekFailureId = await seekFailurePlayback.start();
  const seekFailureVideo = seekFailure.videos[0];
  seekFailureVideo.dispatch('playing');
  await seekFailurePlayback.seekAbsolute(90);
  seekFailureVideo.error = {code: 3, message: 'repositioned stream decode failed'};
  seekFailureVideo.dispatch('error');
  await flush();
  const seekFailureStatus = find(
    seekFailurePlayback.element,
    item => item.className === 'recordings2-playback-status'
  );
  assert.ok(seekFailureStatus.textContent.includes('Seek wurde serverseitig ausgeführt'));
  assert.ok(seekFailureStatus.textContent.includes('repositioned stream decode failed'));
  assert.strictEqual(seekFailurePlayback.state(), 'stopped');
  assert.deepStrictEqual(
    JSON.parse(JSON.stringify(seekFailurePlayback.snapshot().failure)),
    {
      category: 'decoder',
      origin: 'platform-player',
      stage: 'decode',
      terminal: true,
      recoveryClass: 'none',
      reasonCode: 'client_media_decode_error'
    }
  );
  assert.ok(
    seekFailure.requests.some(entry => entry.body.operation === 'stop' && entry.body.sessionId === seekFailureId),
    'post-seek stream failure must still clean up the active MediaSession'
  );

  console.log('phase65d2 recording playback controls, truthful seek, lazy index and classified failure semantics ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
