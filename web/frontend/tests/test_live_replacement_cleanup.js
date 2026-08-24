'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const sourcePath = 'web/frontend/api/live-replacement-cleanup.js';
const source = fs.readFileSync(sourcePath, 'utf8');
const requests = [];
let currentPlayback = null;

function panel(result) {
  return Object.freeze({
    element: {},
    destroy() {},
    start() {
      if (result instanceof Error) return Promise.reject(result);
      return Promise.resolve(result);
    },
    sessionId() { return ''; },
    relinquishForReplacement() { return Promise.resolve(''); }
  });
}

function playbackWith(result) {
  return Object.freeze({
    createPanel() { return {kind: 'recording'}; },
    createLivePanel() { return panel(result); }
  });
}

const window = {
  VdrSuiteBrowserSession: {
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-live-cleanup'}; }
  },
  VdrSuiteClientApi: {
    requestJson(path, options) {
      requests.push({path, options});
      return Promise.resolve({mediaSession: {state: 'ended'}});
    }
  }
};

currentPlayback = playbackWith('new-live-session');
Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
  configurable: true,
  enumerable: true,
  get() { return currentPlayback; },
  set(value) { currentPlayback = value; }
});

vm.runInNewContext(source, {
  window,
  Object,
  String,
  Promise,
  RegExp,
  Error,
  JSON,
  Boolean
}, {filename: sourcePath});

assert.ok(window.VdrSuiteLiveReplacementCleanup);
assert.strictEqual(
  window.VdrSuiteLiveReplacementCleanup.__test.safeSessionId('live-session:A_1.2'),
  'live-session:A_1.2'
);
assert.strictEqual(window.VdrSuiteLiveReplacementCleanup.__test.safeSessionId('../bad'), '');
assert.strictEqual(typeof window.VdrSuiteRecordings2Playback.createLivePanel, 'function');

(async function () {
  requests.length = 0;
  const successful = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'B'},
    'living-room',
    {replacesSessionId: 'old-live-session'}
  );
  assert.strictEqual(await successful.start(), 'new-live-session');
  assert.strictEqual(requests.length, 0, 'successful replacement must not stop the yielded session in the browser');

  currentPlayback = playbackWith('');
  window.VdrSuiteRecordings2Playback = currentPlayback;
  requests.length = 0;
  const failed = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'C'},
    'living-room',
    {replacesSessionId: 'old-live-session'}
  );
  assert.strictEqual(await failed.start(), '');
  assert.strictEqual(requests.length, 1, 'failed replacement must cleanup exactly once');
  assert.strictEqual(requests[0].path, '/api/media/sessions');
  const failedBody = JSON.parse(requests[0].options.body);
  assert.strictEqual(failedBody.resourceKind, 'live-channel');
  assert.strictEqual(failedBody.backendId, 'living-room');
  assert.strictEqual(failedBody.sessionId, 'old-live-session');
  assert.strictEqual(failedBody.operation, 'stop');
  assert.strictEqual(requests[0].options.headers['X-CSRF-Token'], 'csrf-live-cleanup');
  await failed.start();
  assert.strictEqual(requests.length, 1, 'repeated start calls must not duplicate cleanup');

  currentPlayback = playbackWith(new Error('replacement request failed'));
  window.VdrSuiteRecordings2Playback = currentPlayback;
  requests.length = 0;
  const rejected = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'D'},
    'living-room',
    {replacesSessionId: 'old-live-session-2'}
  );
  await assert.rejects(rejected.start(), /replacement request failed/);
  assert.strictEqual(requests.length, 1, 'rejected replacement must cleanup the yielded session');
  const rejectedBody = JSON.parse(requests[0].options.body);
  assert.strictEqual(rejectedBody.sessionId, 'old-live-session-2');

  currentPlayback = playbackWith('');
  window.VdrSuiteRecordings2Playback = currentPlayback;
  requests.length = 0;
  const ordinary = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'E'},
    'living-room',
    {}
  );
  assert.strictEqual(await ordinary.start(), '');
  assert.strictEqual(requests.length, 0, 'ordinary failed startup has no yielded session to cleanup');

  console.log('live replacement cleanup contract ok');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
