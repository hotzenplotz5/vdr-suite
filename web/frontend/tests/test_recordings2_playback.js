'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

let request = null;
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
  MediaSource: {
    isTypeSupported(type) { return type.indexOf('avc1.640028') !== -1; }
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
  JSON
});

vm.runInContext(
  fs.readFileSync('web/frontend/recordings2-playback.js', 'utf8'),
  context,
  {filename: 'web/frontend/recordings2-playback.js'}
);

const playback = window.VdrSuiteRecordings2Playback;
assert.ok(playback);
const test = playback.__test;

assert.deepStrictEqual(
  JSON.parse(JSON.stringify(test.capabilities())),
  {
    protocols: ['hls'],
    containers: ['fmp4'],
    videoCodecs: ['h264'],
    audioCodecs: ['aac'],
    supportsByteRanges: false,
    maxVideoWidth: 1920,
    maxVideoHeight: 1080
  }
);
assert.strictEqual(test.recordingId({recordingId: 'rec_public'}), 'rec_public');
assert.strictEqual(test.recordingId({id: 'rec_fallback'}), 'rec_fallback');
assert.strictEqual(test.recordingId({backendNativeId: '/srv/vdr/video/private.rec'}), '');

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
assert.ok(test.supportedMimeType().indexOf('avc1.640028') !== -1);

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
  assert.strictEqual(Object.prototype.hasOwnProperty.call(body, 'accessCredential'), false);
  assert.strictEqual(request.path.indexOf('token='), -1);
  assert.strictEqual(request.path.indexOf('credential='), -1);

  console.log('recordings2 playback browser contract ok');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
