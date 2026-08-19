'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

let request = null;
let keepaliveRequest = null;
let failNextRequest = false;
const requests = [];
const window = {
  VdrSuiteBrowserSession: {
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-live-token'}; }
  },
  VdrSuiteClientApi: {
    requestJson(path, options) {
      request = {path, options};
      requests.push(request);
      if (failNextRequest) {
        failNextRequest = false;
        return Promise.reject(new Error('network failed'));
      }
      return Promise.resolve({
        mediaSession: {
          id: 'live_session_test',
          resourceKind: 'live-channel',
          state: 'ready',
          mediaPath: '/api/media/sessions/live_session_test/hls/master.m3u8'
        }
      });
    }
  },
  fetch(path, options) {
    keepaliveRequest = {path, options};
    return Promise.resolve({ok: true});
  },
  MediaSource: {
    isTypeSupported() { return true; }
  }
};

const context = vm.createContext({
  window,
  document: {},
  console,
  Object,
  String,
  Number,
  Array,
  Boolean,
  Promise,
  Set,
  RegExp,
  Error,
  JSON,
  Uint8Array
});

const source = fs.readFileSync('web/frontend/recordings2-playback.js', 'utf8');
vm.runInContext(source, context, {filename: 'web/frontend/recordings2-playback.js'});

const playback = window.VdrSuiteRecordings2Playback;
assert.ok(playback);
assert.strictEqual(typeof playback.createLivePanel, 'function');
const test = playback.__test;

assert.strictEqual(test.liveChannelId({channelId: 'C-1-1079-10351'}), 'C-1-1079-10351');
assert.strictEqual(test.liveChannelId({id: 'C-1-1079-10351'}), 'C-1-1079-10351');
assert.strictEqual(test.safeMediaPath('/api/media/sessions/live_session_test/hls/master.m3u8'), '/api/media/sessions/live_session_test/hls/master.m3u8');
assert.strictEqual(test.safeMediaPath('unix:///run/vdr/vdr-suite-live/session.sock'), '');
assert.strictEqual(test.safeMediaPath('/run/vdr/vdr-suite-live/session.sock'), '');
assert.strictEqual(test.safeMediaPath('https://provider.invalid/live/master.m3u8'), '');
assert.strictEqual(test.safeMediaPath('/api/media/sessions/live_session_test/hls/master.m3u8?token=secret'), '');
assert.strictEqual(test.safeMediaPath('/api/media/sessions/live_session_test/hls/other.m3u8'), '');

(async function () {
  requests.length = 0;
  await test.createLiveSession('living-room', {id: 'C-1-1079-10351'});
  assert.strictEqual(request.path, '/api/media/sessions');
  assert.strictEqual(request.options.method, 'POST');
  assert.strictEqual(request.options.credentials, 'same-origin');
  assert.strictEqual(request.options.cache, 'no-store');
  assert.strictEqual(request.options.headers['Content-Type'], 'application/json');
  assert.strictEqual(request.options.headers['X-CSRF-Token'], 'csrf-live-token');
  assert.deepStrictEqual(JSON.parse(request.options.body), {
    resourceKind: 'live-channel',
    backendId: 'living-room',
    channelId: 'C-1-1079-10351',
    capabilities: {
      protocols: ['hls'],
      containers: ['fmp4'],
      videoCodecs: ['h264'],
      audioCodecs: ['aac'],
      supportsByteRanges: false,
      maxVideoWidth: 1920,
      maxVideoHeight: 1080,
      maxAudioChannels: 2
    }
  });

  await test.createLiveSession(
    'living-room',
    {channelId: 'C-1-1079-10352'},
    'live_session_a'
  );
  assert.deepStrictEqual(JSON.parse(request.options.body), {
    resourceKind: 'live-channel',
    backendId: 'living-room',
    channelId: 'C-1-1079-10352',
    capabilities: {
      protocols: ['hls'],
      containers: ['fmp4'],
      videoCodecs: ['h264'],
      audioCodecs: ['aac'],
      supportsByteRanges: false,
      maxVideoWidth: 1920,
      maxVideoHeight: 1080,
      maxAudioChannels: 2
    },
    replacesSessionId: 'live_session_a'
  });

  await test.stopLiveSession('living-room', 'live_session_test');
  assert.strictEqual(request.path, '/api/media/sessions');
  assert.deepStrictEqual(JSON.parse(request.options.body), {
    resourceKind: 'live-channel',
    backendId: 'living-room',
    sessionId: 'live_session_test',
    operation: 'stop'
  });

  keepaliveRequest = null;
  await test.stopLiveSessionKeepalive('living-room', 'live_session_keepalive');
  assert.ok(keepaliveRequest);
  assert.strictEqual(keepaliveRequest.path, '/api/media/sessions');
  assert.strictEqual(keepaliveRequest.options.keepalive, true);
  assert.deepStrictEqual(JSON.parse(keepaliveRequest.options.body), {
    resourceKind: 'live-channel',
    backendId: 'living-room',
    sessionId: 'live_session_keepalive',
    operation: 'stop'
  });

  const beforeFailure = requests.length;
  failNextRequest = true;
  await assert.rejects(
    () => test.createLiveSession('living-room', {id: 'C-1-1079-10353'}, 'live_session_orphan'),
    /network failed/
  );
  assert.strictEqual(requests.length, beforeFailure + 2);
  assert.deepStrictEqual(JSON.parse(requests[requests.length - 1].options.body), {
    resourceKind: 'live-channel',
    backendId: 'living-room',
    sessionId: 'live_session_orphan',
    operation: 'stop'
  });

  assert.ok(source.includes('relinquishForReplacement'));
  assert.ok(source.includes('stopIssued = true'));
  assert.ok(source.includes('safeMediaPath(mediaSession.mediaPath)'));
  assert.ok(!source.includes('unix:///run/vdr'));

  console.log('live TV browser MediaSession contract ok');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
