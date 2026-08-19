'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const requests = [];
const videos = [];

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
    controls: false,
    autoplay: false,
    playsInline: false,
    preload: '',
    src: '',
    appendChild(child) { this.children.push(child); return child; },
    setAttribute() {},
    removeAttribute(name) { if (name === 'src') this.src = ''; },
    addEventListener(name, callback) { listeners[name] = callback; },
    pause() { this.paused = true; },
    load() { this.loaded = (this.loaded || 0) + 1; },
    play() { this.played = (this.played || 0) + 1; return Promise.resolve(); }
  };
  if (value.tagName === 'VIDEO') videos.push(value);
  return value;
}

const window = {
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
  setTimeout
});

const sourcePath = 'web/frontend/api/session-frontend-sync.js';
const source = fs.readFileSync(sourcePath, 'utf8');
vm.runInContext(source, context, {filename: sourcePath});

assert.ok(window.VdrSuiteLivePlayback);
assert.ok(window.VdrSuiteRecordings2Playback);
assert.strictEqual(typeof window.VdrSuiteRecordings2Playback.createLivePanel, 'function');

const test = window.VdrSuiteLivePlayback.__test;
assert.deepStrictEqual(
  JSON.parse(JSON.stringify(test.liveCapabilities())),
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
  test.safeLiveMediaPath('/api/media/sessions/live_session_test/live/stream.mp4'),
  '/api/media/sessions/live_session_test/live/stream.mp4'
);
assert.strictEqual(test.safeLiveMediaPath('/api/media/sessions/live_session_test/hls/master.m3u8'), '');
assert.strictEqual(test.safeLiveMediaPath('unix:///run/vdr/live.sock'), '');
assert.strictEqual(test.safeLiveMediaPath('/api/media/sessions/live_session_test/live/stream.mp4?token=x'), '');

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
  assert.strictEqual(videos[0].src, '/api/media/sessions/live_session_test/live/stream.mp4');
  assert.strictEqual(videos[0].autoplay, true);
  assert.strictEqual(videos[0].played, 1);

  const relinquished = await playback.relinquishForReplacement();
  assert.strictEqual(relinquished, 'live_session_test');
  assert.strictEqual(requests.length, 1, 'replacement handoff must not STOP A in the browser');
  playback.destroy();
  assert.strictEqual(requests.length, 1);

  // Loading the recording playback runtime later must preserve the direct
  // Live-TV override while exposing the recording API alongside it.
  window.VdrSuiteRecordings2Playback = Object.freeze({
    createPanel() { return 'recording'; }
  });
  assert.strictEqual(typeof window.VdrSuiteRecordings2Playback.createPanel, 'function');
  assert.strictEqual(typeof window.VdrSuiteRecordings2Playback.createLivePanel, 'function');

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
  assert.ok(source.includes('/live/stream\\.mp4'));
  assert.ok(!source.includes("STARTUP_BUFFER_SECONDS"));
  assert.ok(!source.includes('master.m3u8'));

  console.log('direct Live TV browser contract ok');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
