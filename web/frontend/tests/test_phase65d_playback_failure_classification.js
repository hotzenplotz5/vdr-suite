'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const helperSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-failure-classification.js'),
  'utf8'
);
const ownerSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'session-frontend-sync.js'),
  'utf8'
);

const window = {};
window.window = window;
vm.runInContext(
  helperSource,
  vm.createContext({window, Object, String, Number, Boolean}),
  {filename: 'playback-failure-classification.js'}
);

const classifier = window.VdrSuitePlaybackFailureClassification;
assert.ok(classifier, 'classified playback failure helper must publish');
assert.strictEqual(typeof classifier.classifyClientTransportError, 'function');
assert.strictEqual(typeof classifier.classifyPlatformMediaError, 'function');

function assertFailure(actual, expected) {
  assert.deepStrictEqual(
    JSON.parse(JSON.stringify(actual)),
    expected
  );
  assert.ok(Object.isFrozen(actual), 'normalized failure must be immutable');
  assert.deepStrictEqual(
    Object.keys(actual).sort(),
    ['category', 'origin', 'stage', 'terminal', 'recoveryClass', 'reasonCode'].sort(),
    'failure shape must stay bounded to the Slice-4 semantic fields'
  );
}

assertFailure(
  classifier.classifyClientTransportError(
    new Error('Browser konnte kontinuierliche fMP4-Daten nicht verarbeiten.')
  ),
  {
    category: 'buffer',
    origin: 'client-transport',
    stage: 'source-buffer-operation',
    terminal: true,
    recoveryClass: 'none',
    reasonCode: 'client_source_buffer_error'
  }
);

const quota = new Error('append failed');
quota.name = 'QuotaExceededError';
assertFailure(
  classifier.classifyClientTransportError(quota),
  {
    category: 'buffer',
    origin: 'client-transport',
    stage: 'source-buffer-append',
    terminal: true,
    recoveryClass: 'none',
    reasonCode: 'client_source_buffer_quota_exceeded'
  }
);

assertFailure(
  classifier.classifyClientTransportError(
    new Error('Kontinuierlicher fMP4-Stream konnte nicht geladen werden (503).')
  ),
  {
    category: 'transport',
    origin: 'client-transport',
    stage: 'stream-fetch',
    terminal: true,
    recoveryClass: 'none',
    reasonCode: 'client_stream_fetch_failed'
  }
);

assertFailure(
  classifier.classifyClientTransportError(
    new Error('Browser-MSE unterstützt die tatsächliche kontinuierliche fMP4-Codec-Konfiguration nicht.')
  ),
  {
    category: 'client-platform',
    origin: 'platform-player',
    stage: 'codec-support',
    terminal: true,
    recoveryClass: 'none',
    reasonCode: 'client_mse_codec_unsupported'
  }
);

assertFailure(
  classifier.classifyPlatformMediaError({code: 3, message: 'decode failed'}),
  {
    category: 'decoder',
    origin: 'platform-player',
    stage: 'decode',
    terminal: true,
    recoveryClass: 'none',
    reasonCode: 'client_media_decode_error'
  }
);

assertFailure(
  classifier.classifyPlatformMediaError({code: 4}),
  {
    category: 'client-platform',
    origin: 'platform-player',
    stage: 'source-support',
    terminal: true,
    recoveryClass: 'none',
    reasonCode: 'client_media_source_not_supported'
  }
);

// Classification is evidence only. The helper must have no owner/session/API
// action surface that could turn recoveryClass into an imperative.
[
  'activateFallback',
  'createRecordingSession',
  'stopRecordingSession',
  'requestJson',
  'legacyFactory',
  '.start('
].forEach(function (forbidden) {
  assert.strictEqual(
    helperSource.indexOf(forbidden),
    -1,
    'classifier must not contain recovery action: ' + forbidden
  );
});

// The production owner keeps its already-accepted startup rescue policy. The
// classifier is consulted only in the post-start terminal branch, so adding a
// class/recoveryClass cannot itself cause the HLS compatibility replacement.
const connectionStart = ownerSource.indexOf(
  'function connectRecordingStream(autoPlay, initialConnection)'
);
const pageHideStart = ownerSource.indexOf('\n    function pageHide()', connectionStart);
assert.ok(connectionStart >= 0 && pageHideStart > connectionStart);
const connection = ownerSource.slice(connectionStart, pageHideStart);
const startupFallback = connection.indexOf(
  'if (initialConnection && !firstMediaReported) activateFallback(error);'
);
const terminalFailure = connection.indexOf('else failStartedPlayback(');
const transportClassification = connection.indexOf('classifyClientTransportError(error)');
assert.ok(startupFallback >= 0, 'existing startup-only compatibility rescue must remain');
assert.ok(terminalFailure > startupFallback, 'post-start path must remain terminal');
assert.ok(
  transportClassification > terminalFailure,
  'transport classification must be data passed to terminal owner handling, not a fallback predicate'
);

const mediaErrorStart = ownerSource.indexOf("video.addEventListener('error', function () {");
const mediaErrorEnd = ownerSource.indexOf('\n    });', mediaErrorStart) + '\n    });'.length;
assert.ok(mediaErrorStart >= 0 && mediaErrorEnd > mediaErrorStart);
const mediaErrorBlock = ownerSource.slice(mediaErrorStart, mediaErrorEnd);
assert.ok(mediaErrorBlock.includes('if (firstMediaReported) {'));
assert.ok(mediaErrorBlock.includes('classifyPlatformMediaError(mediaError)'));
assert.ok(mediaErrorBlock.includes("else {\n          activateFallback("));
assert.ok(
  mediaErrorBlock.indexOf('classifyPlatformMediaError(mediaError)') <
    mediaErrorBlock.indexOf("else {\n          activateFallback("),
  'platform classification must remain confined to the already-started terminal branch'
);

console.log('phase65d classified browser playback failure semantics ok');
