'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const requests = [];
const videos = [];
let failFastRecording = false;
let legacyStartCalls = 0;
let clock = 1000;

function node(tagName) {
  const listeners = {};
  const value = {
    tagName: String(tagName || '').toUpperCase(),
    children: [],
    className: '',
    classList: {
      toggle() {}, add() {}, remove() {}
    },
    style: {},
    textContent: '',
    hidden: false,
    disabled: false,
    controls: false,
    autoplay: false,
    playsInline: false,
    preload: '',
    src: '',
    replacement: null,
    appendChild(child) { this.children.push(child); return child; },
    setAttribute() {},
    removeAttribute(name) { if (name === 'src') this.src = ''; },
    addEventListener(name, callback) { listeners[name] = callback; },
    dispatch(name) { if (listeners[name]) listeners[name](); },
    replaceWith(replacement) { this.replacement = replacement; },
    pause() { this.paused = true; },
    load() { this.loaded = (this.loaded || 0) + 1; },
    play() { this.played = (this.played || 0) + 1; return Promise.resolve(); }
  };
  if (value.tagName === 'VIDEO') videos.push(value);
  return value;
}

const window = {
  console,
  performance: {
    now() { clock += 125; return clock; }
  },
  VdrSuitePublicUrl: {
    basePath: '/vdr-suite',
    resolvePath(path) { return '/vdr-suite' + path; }
  },
  VdrSuiteBrowserSession: {
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-live-token'}; },
    subscribe() {}
  },
  VdrSuiteClientApi: {
    requestJson(path, options) {
      requests.push({path, options});
      const body = JSON.parse(options.body);
      if (body.operation === 'stop') {
        return Promise.resolve({mediaSession: {id: body.sessionId, state: 'ended'}});
      }
      if (body.resourceKind === 'live-channel') {
        return Promise.resolve({
          mediaSession: {
            id: 'live_session_test',
            resourceKind: 'live-channel',
            state: 'ready',
            presentationProfileId: 'live-progressive-fmp4',
            mediaPath: '/api/media/sessions/live_session_test/live/stream.mp4'
          }
        });
      }
      if (failFastRecording) {
        return Promise.reject(new Error('progressive recording unavailable'));
      }
      return Promise.resolve({
        mediaSession: {
          id: 'recording_session_test',
          resourceKind: 'recording',
          state: 'ready',
          presentationProfileId: 'progressive-fmp4',
          mediaPath: '/api/media/sessions/recording_session_test/recording/stream.mp4'
        }
      });
    }
  },
  fetch(path, options) {
    requests.push({path, options});
    return Promise.resolve({ok: true});
  },
  document: {
    readyState: 'complete',
    visibilityState: 'visible',
    createElement: node,
    addEventListener() {}
  },
  addEventListener() {},
  removeEventListener() {},
  setTimeout
};

const context = vm.createContext({
  window,
  document: window.document,
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
  setTimeout
});

const sourcePath = 'web/frontend/api/session-frontend-sync.js';
const source = fs.readFileSync(sourcePath, 'utf8');
vm.runInContext(source, context, {filename: sourcePath});

assert.ok(window.VdrSuiteLivePlayback);
assert.ok(window.VdrSuiteRecordingFastPlayback);
assert.ok(window.VdrSuiteRecordings2Playback);
assert.strictEqual(typeof window.VdrSuiteRecordings2Playback.createLivePanel, 'function');

const liveTest = window.VdrSuiteLivePlayback.__test;
assert.deepStrictEqual(
  JSON.parse(JSON.stringify(liveTest.liveCapabilities())),
  {
    protocols: ['progressive'],
    containers: ['fmp4'],
    videoCodecs: ['h264'],
    audioCodecs: ['aac'],
    supportsByteRanges: false,
    maxVideoWidth: 1920,
    maxVideoHeight: 1080,
    maxAudioChannels: 2
  }
);
assert.strictEqual(
  liveTest.safeLiveMediaPath('/api/media/sessions/live_session_test/live/stream.mp4'),
  '/api/media/sessions/live_session_test/live/stream.mp4'
);
assert.strictEqual(
  liveTest.publicLiveMediaPath('/api/media/sessions/live_session_test/live/stream.mp4'),
  '/vdr-suite/api/media/sessions/live_session_test/live/stream.mp4'
);
assert.strictEqual(liveTest.safeLiveMediaPath('/api/media/sessions/live_session_test/hls/master.m3u8'), '');
assert.strictEqual(liveTest.safeLiveMediaPath('unix:///run/vdr/live.sock'), '');
assert.strictEqual(liveTest.safeLiveMediaPath('/api/media/sessions/live_session_test/live/stream.mp4?token=x'), '');
assert.strictEqual(liveTest.publicLiveMediaPath('/api/media/sessions/live_session_test/live/stream.mp4?token=x'), '');

const recordingTest = window.VdrSuiteRecordingFastPlayback.__test;
assert.deepStrictEqual(
  JSON.parse(JSON.stringify(recordingTest.recordingCapabilities())),
  {
    protocols: ['progressive'],
    containers: ['fmp4'],
    videoCodecs: ['h264'],
    audioCodecs: ['aac'],
    supportsByteRanges: false,
    maxVideoWidth: 1920,
    maxVideoHeight: 1080,
    maxAudioChannels: 2
  }
);
assert.strictEqual(
  recordingTest.safeRecordingMediaPath(
    '/api/media/sessions/recording_session_test/recording/stream.mp4'
  ),
  '/api/media/sessions/recording_session_test/recording/stream.mp4'
);
assert.strictEqual(
  recordingTest.publicRecordingMediaPath(
    '/api/media/sessions/recording_session_test/recording/stream.mp4'
  ),
  '/vdr-suite/api/media/sessions/recording_session_test/recording/stream.mp4'
);
assert.strictEqual(recordingTest.safeRecordingMediaPath('/srv/vdr/video.00/test/00001.ts'), '');
assert.strictEqual(
  recordingTest.safeRecordingMediaPath(
    '/api/media/sessions/recording_session_test/recording/stream.mp4?token=x'
  ),
  ''
);

(async function () {
  requests.length = 0;
  videos.length = 0;

  const playback = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'C-1-1079-10351', name: 'Das Erste HD'},
    'living-room',
    {replacesSessionId: 'live_session_a'}
  );
  assert.ok(playback.element);
  assert.strictEqual(typeof playback.start, 'function');
  assert.strictEqual(typeof playback.relinquishForReplacement, 'function');

  const sessionId = await playback.start();
  assert.strictEqual(sessionId, 'live_session_test');
  assert.strictEqual(requests.length, 1);
  assert.strictEqual(requests[0].path, '/api/media/sessions');
  const createBody = JSON.parse(requests[0].options.body);
  assert.strictEqual(createBody.resourceKind, 'live-channel');
  assert.strictEqual(createBody.backendId, 'living-room');
  assert.strictEqual(createBody.channelId, 'C-1-1079-10351');
  assert.strictEqual(createBody.replacesSessionId, 'live_session_a');
  assert.deepStrictEqual(createBody.capabilities.protocols, ['progressive']);
  assert.deepStrictEqual(createBody.capabilities.containers, ['fmp4']);
  assert.strictEqual(requests[0].options.headers['X-CSRF-Token'], 'csrf-live-token');

  assert.strictEqual(videos.length, 1);
  assert.strictEqual(
    videos[0].src,
    '/vdr-suite/api/media/sessions/live_session_test/live/stream.mp4'
  );
  assert.strictEqual(videos[0].autoplay, true);
  assert.strictEqual(videos[0].played, 1);

  const relinquished = await playback.relinquishForReplacement();
  assert.strictEqual(relinquished, 'live_session_test');
  assert.strictEqual(requests.length, 1, 'replacement handoff must not STOP A in the browser');
  playback.destroy();
  assert.strictEqual(requests.length, 1);

  // Loading the recording HLS runtime later must preserve Live-TV direct and
  // replace only the Recording entrypoint with the Phase-65.C fast facade.
  window.VdrSuiteRecordings2Playback = Object.freeze({
    createPanel() {
      const element = node('section');
      return Object.freeze({
        element,
        start() { legacyStartCalls += 1; return Promise.resolve('legacy_session'); },
        destroy() {},
        sessionId() { return 'legacy_session'; }
      });
    }
  });
  assert.strictEqual(typeof window.VdrSuiteRecordings2Playback.createPanel, 'function');
  assert.strictEqual(typeof window.VdrSuiteRecordings2Playback.createLivePanel, 'function');

  requests.length = 0;
  videos.length = 0;
  const recordingPlayback = window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-42', title: 'Schnelle Aufnahme'},
    'living-room'
  );
  const recordingSessionId = await recordingPlayback.start();
  assert.strictEqual(recordingSessionId, 'recording_session_test');
  assert.strictEqual(requests.length, 1);
  const recordingBody = JSON.parse(requests[0].options.body);
  assert.strictEqual(recordingBody.backendId, 'living-room');
  assert.strictEqual(recordingBody.recordingId, 'recording-42');
  assert.deepStrictEqual(recordingBody.capabilities.protocols, ['progressive']);
  assert.deepStrictEqual(recordingBody.capabilities.containers, ['fmp4']);
  assert.strictEqual(recordingBody.capabilities.supportsByteRanges, false);
  assert.strictEqual(videos.length, 1);
  assert.strictEqual(
    videos[0].src,
    '/vdr-suite/api/media/sessions/recording_session_test/recording/stream.mp4'
  );
  assert.strictEqual(videos[0].played, 1);
  videos[0].dispatch('playing');
  assert.ok(recordingPlayback.element.children[1].textContent.includes('Start 0.'));
  recordingPlayback.destroy();
  assert.ok(requests.some(entry => {
    if (!entry.options || !entry.options.body) return false;
    const body = JSON.parse(entry.options.body);
    return body.operation === 'stop' && body.sessionId === 'recording_session_test';
  }));
  assert.strictEqual(legacyStartCalls, 0, 'successful fast recording must not enter HLS fallback');

  // A source that cannot use progressive-fmp4 must fall back to the accepted
  // existing Recording HLS player rather than weakening source truth.
  requests.length = 0;
  failFastRecording = true;
  const fallbackPlayback = window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'growing-recording', title: 'Wachsende Aufnahme'},
    'living-room'
  );
  const fallbackSession = await fallbackPlayback.start();
  assert.strictEqual(fallbackSession, 'legacy_session');
  assert.strictEqual(legacyStartCalls, 1);
  assert.ok(fallbackPlayback.element.replacement);
  fallbackPlayback.destroy();
  failFastRecording = false;

  const second = window.VdrSuiteRecordings2Playback.createLivePanel(
    {channelId: 'C-1-1079-10352', name: 'NDR FS HH HD'},
    'living-room',
    {}
  );
  await second.start();
  second.destroy();
  assert.ok(requests.some(entry => {
    if (!entry.options || !entry.options.body) return false;
    const body = JSON.parse(entry.options.body);
    return body.operation === 'stop' && body.sessionId === 'live_session_test';
  }));

  assert.ok(source.includes("protocols: ['progressive']"));
  assert.ok(source.includes('publicLiveMediaPath'));
  assert.ok(source.includes('publicRecordingMediaPath'));
  assert.ok(source.includes('recording\\/stream\\.mp4'));
  assert.ok(source.includes('recording playback first-media'));
  assert.ok(!source.includes('STARTUP_BUFFER_SECONDS'));
  assert.ok(!source.includes('master.m3u8'));

  console.log('direct Live TV and low-latency Recording browser contracts ok');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
