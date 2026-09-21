'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

let request = null;
let keepaliveRequest = null;
const window = {
  VdrSuiteBrowserSession: {
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-test-token'}; }
  },
  VdrSuiteClientApi: {
    requestJson(path, options) {
      request = {path, options};
      return Promise.resolve({
        mediaSession: {
          id: 'mediasess_test',
          state: 'ready',
          mediaPath: '/api/media/sessions/mediasess_test/hls/master.m3u8'
        }
      });
    }
  },
  fetch(path, options) {
    keepaliveRequest = {path, options};
    return Promise.resolve({ok: true});
  },
  MediaSource: {
    isTypeSupported(type) { return type.indexOf('avc1.640016') !== -1; }
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

const playbackSource = fs.readFileSync('web/frontend/recordings2-playback.js', 'utf8');
vm.runInContext(
  playbackSource,
  context,
  {filename: 'web/frontend/recordings2-playback.js'}
);

const playback = window.VdrSuiteRecordings2Playback;
assert.ok(playback);
const test = playback.__test;

const frontendHttpPaths = fs.readFileSync(
  'core/http/src/TestHttpServerPaths.inc',
  'utf8'
);
assert.ok(
  frontendHttpPaths.includes(
    '{"/frontend/recordings2-playback.js", "recordings2-playback.js", "application/javascript; charset=utf-8",'
  ),
  'recordings2 playback runtime must be exposed by the daemon frontend asset router'
);
assert.ok(
  playbackSource.includes("global.addEventListener('pagehide', handlePageHide)"),
  'recording playback must stop its MediaSession when the page is hidden/unloaded'
);
assert.ok(
  playbackSource.includes("global.removeEventListener('pagehide', handlePageHide)"),
  'recording playback must remove the pagehide handler on ordinary view teardown'
);
assert.ok(
  playbackSource.includes("video.addEventListener('ended', handleEnded)"),
  'recording playback must release the MediaSession at natural playback end'
);
assert.ok(
  playbackSource.includes(
    'if (destroyed || playbackFailed || !playbackStarted || rebuffering) return;'
  ),
  'a terminal playback error must not be overwritten by a later waiting event'
);

assert.deepStrictEqual(
  JSON.parse(JSON.stringify(test.capabilities())),
  {
    protocols: ['hls'],
    containers: ['fmp4'],
    videoCodecs: ['h264'],
    audioCodecs: ['aac'],
    supportsByteRanges: false,
    maxVideoWidth: 1920,
    maxVideoHeight: 1080,
    maxAudioChannels: 2
  }
);
assert.strictEqual(test.recordingId({recordingId: 'rec_public'}), 'rec_public');
assert.strictEqual(test.recordingId({id: 'rec_fallback'}), 'rec_fallback');
assert.strictEqual(test.recordingId({nativeId: '/srv/vdr/video/private.rec'}), '');
assert.strictEqual(test.recordingId({backendNativeId: '/srv/vdr/video/private.rec'}), '');
assert.strictEqual(test.safeSessionId('mediasess_test'), 'mediasess_test');
assert.strictEqual(test.safeSessionId('../mediasess_test'), '');

const parsed = test.parsePlaylist([
  '#EXTM3U',
  '#EXT-X-VERSION:7',
  '#EXT-X-MAP:URI="init.mp4"',
  '#EXTINF:4.0,',
  'segment-000001.m4s',
  '#EXTINF:4.0,',
  'segment-000002.m4s',
  '#EXT-X-ENDLIST',
  ''
].join('\n'));
assert.strictEqual(parsed.initSegment, 'init.mp4');
assert.deepStrictEqual(Array.from(parsed.segments), ['segment-000001.m4s', 'segment-000002.m4s']);
assert.deepStrictEqual(Array.from(parsed.segmentDurations), [4, 4]);
assert.strictEqual(parsed.ended, true);
assert.strictEqual(test.safeArtifactName('../secret.m4s'), '');
assert.strictEqual(test.safeArtifactName('/tmp/secret.m4s'), '');
assert.strictEqual(test.safeArtifactName('segment-000001.m4s?token=x'), '');
assert.strictEqual(test.safeArtifactName('segment-000001.m4s'), 'segment-000001.m4s');
assert.strictEqual(
  test.artifactUrl('/api/media/sessions/mediasess_test/hls/master.m3u8', 'init.mp4'),
  '/api/media/sessions/mediasess_test/hls/init.mp4'
);
assert.throws(
  () => test.artifactUrl('/api/media/sessions/mediasess_test/hls/master.m3u8', '../secret.m4s'),
  /Ungültiger MediaSession-Artefaktpfad/
);
assert.throws(
  () => test.artifactUrl('/elsewhere/master.m3u8', 'init.mp4'),
  /außerhalb des HLS-Gateways/
);

assert.strictEqual(test.startupBufferSeconds, 12);
assert.strictEqual(test.rebufferResumeSeconds, 12);

const startupTooShort = test.parsePlaylist([
  '#EXTM3U',
  '#EXT-X-MAP:URI="init.mp4"',
  '#EXTINF:4.0,',
  'segment-000001.m4s',
  '#EXTINF:4.0,',
  'segment-000002.m4s'
].join('\n'));
const shortBatch = test.startupBatch(startupTooShort, test.startupBufferSeconds);
assert.strictEqual(shortBatch.ready, false);
assert.strictEqual(shortBatch.duration, 8);
assert.deepStrictEqual(Array.from(shortBatch.segments), [
  'segment-000001.m4s',
  'segment-000002.m4s'
]);

const startupReady = test.parsePlaylist([
  '#EXTM3U',
  '#EXT-X-MAP:URI="init.mp4"',
  '#EXTINF:4.0,',
  'segment-000001.m4s',
  '#EXTINF:4.0,',
  'segment-000002.m4s',
  '#EXTINF:4.0,',
  'segment-000003.m4s'
].join('\n'));
const readyBatch = test.startupBatch(startupReady, test.startupBufferSeconds);
assert.strictEqual(readyBatch.ready, true);
assert.strictEqual(readyBatch.duration, 12);
assert.deepStrictEqual(Array.from(readyBatch.segments), [
  'segment-000001.m4s',
  'segment-000002.m4s',
  'segment-000003.m4s'
]);

const shortEndedBatch = test.startupBatch(parsed, test.startupBufferSeconds);
assert.strictEqual(shortEndedBatch.ready, true);
assert.strictEqual(shortEndedBatch.duration, 8);

const bufferedRange = {
  buffered: {
    length: 1,
    start() { return 0; },
    end() { return 12; }
  }
};
assert.strictEqual(test.bufferedAheadSeconds(bufferedRange, {currentTime: 0}), 12);
assert.strictEqual(test.bufferedAheadSeconds(bufferedRange, {currentTime: 5}), 7);
assert.strictEqual(
  test.bufferReady(bufferedRange, {currentTime: 0}, test.startupBufferSeconds, false),
  true
);
assert.strictEqual(
  test.bufferReady(bufferedRange, {currentTime: 5}, test.rebufferResumeSeconds, false),
  false
);
assert.strictEqual(
  test.bufferReady(bufferedRange, {currentTime: 5}, test.rebufferResumeSeconds, true),
  true
);

function mp4Box(type, payload) {
  const result = Buffer.alloc(8 + payload.length);
  result.writeUInt32BE(result.length, 0);
  result.write(type, 4, 4, 'ascii');
  Buffer.from(payload).copy(result, 8);
  return result;
}

function arrayBufferFromBuffer(buffer) {
  return buffer.buffer.slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength);
}

// Regression for the real yaVDR "Angel Has Fallen" acceptance recording:
// H.264 High profile, constraint byte 0x00, level_idc 0x16 (Level 2.2), AAC-LC.
const high22Init = arrayBufferFromBuffer(Buffer.concat([
  mp4Box('avcC', [0x01, 0x64, 0x00, 0x16, 0xff]),
  mp4Box('esds', [0x00, 0x00, 0x00, 0x00, 0x05, 0x02, 0x12, 0x10])
]));
assert.strictEqual(test.avcCodecFromInitSegment(high22Init), 'avc1.640016');
assert.strictEqual(test.aacCodecFromInitSegment(high22Init), 'mp4a.40.2');
assert.strictEqual(
  test.mimeTypeFromInitSegment(high22Init),
  'video/mp4; codecs="avc1.640016,mp4a.40.2"'
);
assert.strictEqual(
  test.supportedMimeType(high22Init),
  'video/mp4; codecs="avc1.640016,mp4a.40.2"'
);

const high40Init = arrayBufferFromBuffer(Buffer.concat([
  mp4Box('avcC', [0x01, 0x64, 0x00, 0x28, 0xff]),
  mp4Box('esds', [0x00, 0x00, 0x00, 0x00, 0x05, 0x02, 0x12, 0x10])
]));
assert.strictEqual(test.avcCodecFromInitSegment(high40Init), 'avc1.640028');
assert.strictEqual(
  test.mimeTypeFromInitSegment(new ArrayBuffer(0)),
  ''
);

(async function () {
  await test.createSession('default', {recordingId: 'rec_public'});
  assert.ok(request);
  assert.strictEqual(request.path, '/api/media/sessions');
  assert.strictEqual(request.options.method, 'POST');
  assert.strictEqual(request.options.credentials, 'same-origin');
  assert.strictEqual(request.options.cache, 'no-store');
  assert.strictEqual(request.options.headers['Content-Type'], 'application/json');
  assert.strictEqual(request.options.headers['X-CSRF-Token'], 'csrf-test-token');

  const body = JSON.parse(request.options.body);
  assert.strictEqual(body.backendId, 'default');
  assert.strictEqual(body.recordingId, 'rec_public');
  assert.deepStrictEqual(body.capabilities.protocols, ['hls']);
  assert.deepStrictEqual(body.capabilities.containers, ['fmp4']);
  assert.deepStrictEqual(body.capabilities.videoCodecs, ['h264']);
  assert.deepStrictEqual(body.capabilities.audioCodecs, ['aac']);
  assert.strictEqual(body.capabilities.maxAudioChannels, 2);
  assert.strictEqual(Object.prototype.hasOwnProperty.call(body, 'accessCredential'), false);
  assert.strictEqual(request.path.indexOf('token='), -1);
  assert.strictEqual(request.path.indexOf('credential='), -1);

  await test.stopSession('default', 'mediasess_test');
  assert.strictEqual(request.path, '/api/media/sessions');
  assert.strictEqual(request.options.method, 'POST');
  assert.strictEqual(request.options.credentials, 'same-origin');
  assert.strictEqual(request.options.cache, 'no-store');
  assert.strictEqual(request.options.headers['X-CSRF-Token'], 'csrf-test-token');
  const stopBody = JSON.parse(request.options.body);
  assert.deepStrictEqual(stopBody, {
    operation: 'stop',
    backendId: 'default',
    sessionId: 'mediasess_test'
  });
  assert.strictEqual(Object.prototype.hasOwnProperty.call(stopBody, 'accessCredential'), false);
  assert.strictEqual(request.path.indexOf('token='), -1);
  assert.strictEqual(request.path.indexOf('credential='), -1);

  keepaliveRequest = null;
  await test.stopSessionKeepalive('default', 'mediasess_keepalive');
  assert.ok(keepaliveRequest);
  assert.strictEqual(keepaliveRequest.path, '/api/media/sessions');
  assert.strictEqual(keepaliveRequest.options.method, 'POST');
  assert.strictEqual(keepaliveRequest.options.credentials, 'same-origin');
  assert.strictEqual(keepaliveRequest.options.cache, 'no-store');
  assert.strictEqual(keepaliveRequest.options.keepalive, true);
  assert.strictEqual(
    keepaliveRequest.options.headers['X-CSRF-Token'],
    'csrf-test-token'
  );
  assert.deepStrictEqual(JSON.parse(keepaliveRequest.options.body), {
    operation: 'stop',
    backendId: 'default',
    sessionId: 'mediasess_keepalive'
  });

  keepaliveRequest = null;
  await test.stopSessionKeepalive('default', '../invalid');
  assert.strictEqual(keepaliveRequest, null);

  console.log('recordings2 playback browser contract ok');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
