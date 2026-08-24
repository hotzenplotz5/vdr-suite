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
const cleanupTest = window.VdrSuiteLiveReplacementCleanup.__test;
assert.strictEqual(
  cleanupTest.safeSessionId('live-session:A_1.2'),
  'live-session:A_1.2'
);
assert.strictEqual(cleanupTest.safeSessionId('../bad'), '');
assert.strictEqual(cleanupTest.channelIsEncrypted({encrypted: true}), true);
assert.strictEqual(cleanupTest.channelIsEncrypted({encrypted: false}), false);
assert.strictEqual(cleanupTest.channelIsEncrypted({encrypted: 'false'}), false);
assert.strictEqual(cleanupTest.channelIsEncrypted({scrambled: '1'}), true);
assert.strictEqual(typeof window.VdrSuiteRecordings2Playback.createLivePanel, 'function');

// Mirror the persistent playback shell: it installs another configurable
// getter/setter around the already-hardened playback facade and refreshes that
// outer facade whenever a later runtime assigns VdrSuiteRecordings2Playback.
(function installOuterPlaybackWrapper() {
  const descriptor = Object.getOwnPropertyDescriptor(window, 'VdrSuiteRecordings2Playback');
  assert.ok(descriptor && typeof descriptor.get === 'function' && typeof descriptor.set === 'function');
  const baseGet = descriptor.get;
  const baseSet = descriptor.set;
  let outer = null;

  function refresh() {
    const base = baseGet.call(window);
    outer = Object.freeze({
      createPanel: base && base.createPanel,
      createLivePanel(channel, backendId, options) {
        return base.createLivePanel(channel, backendId, options);
      }
    });
  }

  refresh();
  Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
    configurable: true,
    enumerable: true,
    get() { return outer; },
    set(value) {
      baseSet.call(window, value);
      refresh();
    }
  });
}());

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

  currentPlayback = playbackWith(new Error('live_source_receiver_unavailable'));
  window.VdrSuiteRecordings2Playback = currentPlayback;
  requests.length = 0;
  const encrypted = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'PAY', name: 'Sky Test', encrypted: true},
    'living-room',
    {}
  );
  await assert.rejects(encrypted.start(), error => {
    assert.ok(error.message.includes('Sky Test'));
    assert.ok(error.message.includes('verschlüsselt'));
    assert.ok(error.message.includes('VDR konnte aktuell keinen Live-Empfang'));
    assert.ok(!error.message.includes('live_source_receiver_unavailable'));
    return true;
  });
  assert.strictEqual(requests.length, 0, 'encrypted initial failure has no yielded session to cleanup');

  currentPlayback = playbackWith(new Error('live_source_receiver_unavailable'));
  window.VdrSuiteRecordings2Playback = currentPlayback;
  requests.length = 0;
  const free = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'FREE', name: 'Free TV', encrypted: false},
    'living-room',
    {}
  );
  await assert.rejects(free.start(), /live_source_receiver_unavailable/);
  assert.strictEqual(requests.length, 0, 'free-channel receiver failures must not trigger replacement cleanup');

  currentPlayback = playbackWith(new Error('Security accountability persistence is unavailable'));
  window.VdrSuiteRecordings2Playback = currentPlayback;
  requests.length = 0;
  const encryptedSecurityFailure = window.VdrSuiteRecordings2Playback.createLivePanel(
    {id: 'PAY-SECURITY', name: 'Encrypted Security Test', encrypted: true},
    'living-room',
    {}
  );
  await assert.rejects(
    encryptedSecurityFailure.start(),
    /Security accountability persistence is unavailable/
  );
  assert.strictEqual(requests.length, 0, 'unrelated encrypted-channel failures must keep their real cause');

  console.log('live replacement cleanup, encryption context and shell wrapping contract ok');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
