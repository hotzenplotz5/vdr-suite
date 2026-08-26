'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-contract-consumer.js'),
  'utf8'
);

let nextPayload = null;
let requestCalls = 0;
const window = {
  VdrSuiteClientApi: Object.freeze({
    requestJson() {
      requestCalls += 1;
      return Promise.resolve(nextPayload);
    },
    existingHelper() { return 'kept'; }
  })
};
window.window = window;
vm.runInContext(source, vm.createContext({window, Object, String, Number, Boolean, Promise, Math}));

assert.strictEqual(window.VdrSuiteClientApi.__vdrSuitePlaybackContractConsumer, true);
assert.strictEqual(window.VdrSuiteClientApi.existingHelper(), 'kept');
assert.strictEqual(typeof window.VdrSuitePlaybackContractConsumer.consume, 'function');

function progressive() {
  return {
    mediaSession: {
      id: 'recording-1',
      state: 'ready',
      presentationProfileId: 'progressive-fmp4',
      playback: {
        positionSeconds: 0,
        durationSeconds: null,
        seek: {supported: false, preparing: true},
        resume: {supported: false, preparing: true}
      },
      tracks: {
        audio: {selectionSupported: false},
        subtitles: {selectionSupported: false, offSupported: false}
      },
      playbackContract: {
        contractVersion: 1,
        resourceMode: 'recording',
        presentationProfileId: 'progressive-fmp4',
        playback: {
          positionSeconds: 42,
          durationSeconds: 5530,
          presentationBasePositionSeconds: 42,
          pauseSupported: true,
          resumeSupported: true,
          restart: {supported: true, preparing: false}
        },
        seek: {
          supported: true,
          mode: 'in-session-reposition',
          preparing: false,
          window: {startSeconds: 0, endSeconds: 5530}
        },
        tracks: {
          audioSelection: {supported: true},
          subtitleSelection: {supported: true},
          subtitleOff: {supported: true}
        },
        continuity: {generation: null, state: null},
        failure: null
      }
    }
  };
}

(async function () {
  nextPayload = progressive();
  const projected = await window.VdrSuiteClientApi.requestJson('/api/media/sessions', {});
  assert.strictEqual(requestCalls, 1);
  assert.strictEqual(projected.mediaSession.playback.positionSeconds, 42);
  assert.strictEqual(projected.mediaSession.playback.durationSeconds, 5530);
  assert.strictEqual(projected.mediaSession.playback.seek.supported, true,
    'normalized in-session seek must override contradictory legacy capability');
  assert.strictEqual(projected.mediaSession.playback.seek.preparing, false);
  assert.deepStrictEqual(
    projected.mediaSession.playback.seek.window,
    {startSeconds: 0, endSeconds: 5530}
  );
  assert.strictEqual(projected.mediaSession.playback.resume.supported, true);
  assert.strictEqual(projected.mediaSession.tracks.audio.selectionSupported, true);
  assert.strictEqual(projected.mediaSession.tracks.subtitles.selectionSupported, true);
  assert.strictEqual(projected.mediaSession.tracks.subtitles.offSupported, true);

  const hls = progressive();
  hls.mediaSession.presentationProfileId = 'hls-fmp4';
  hls.mediaSession.playback.seek = {
    supported: true,
    preparing: true,
    window: {startSeconds: 1, endSeconds: 2}
  };
  hls.mediaSession.playback.resume = {supported: false, preparing: true};
  hls.mediaSession.playbackContract.presentationProfileId = 'hls-fmp4';
  hls.mediaSession.playbackContract.playback.restart = {supported: true, preparing: false};
  hls.mediaSession.playbackContract.seek = {
    supported: true,
    mode: 'replacement-session-restart',
    preparing: false,
    window: {startSeconds: 0, endSeconds: 5530}
  };
  const projectedHls = window.VdrSuitePlaybackContractConsumer.consume(hls);
  assert.strictEqual(projectedHls.mediaSession.playback.seek.supported, false,
    'replacement-session seek must not be exposed as legacy in-session seek');
  assert.strictEqual(projectedHls.mediaSession.playback.seek.preparing, false);
  assert.strictEqual('window' in projectedHls.mediaSession.playback.seek, false);
  assert.strictEqual(projectedHls.mediaSession.playback.resume.supported, true,
    'existing HLS restart owner must derive capability from normalized restart truth');

  const tracksOnly = {
    mediaSession: {
      state: 'ready',
      presentationProfileId: 'hls-fmp4',
      tracks: {
        audio: {selectionSupported: true},
        subtitles: {selectionSupported: true, offSupported: true}
      },
      playbackContract: {
        contractVersion: 1,
        presentationProfileId: 'hls-fmp4',
        playback: {
          positionSeconds: null,
          durationSeconds: null,
          restart: {supported: null, preparing: null}
        },
        seek: {supported: null, mode: 'replacement-session-restart', preparing: null},
        tracks: {
          audioSelection: {supported: false},
          subtitleSelection: {supported: false},
          subtitleOff: {supported: false}
        }
      }
    }
  };
  const projectedTracks = window.VdrSuitePlaybackContractConsumer.consume(tracksOnly);
  assert.strictEqual(projectedTracks.mediaSession.tracks.audio.selectionSupported, false);
  assert.strictEqual(projectedTracks.mediaSession.tracks.subtitles.selectionSupported, false);
  assert.strictEqual(projectedTracks.mediaSession.tracks.subtitles.offSupported, false);
  assert.strictEqual('playback' in projectedTracks.mediaSession, false,
    'partial track status must not invent timeline state');

  const unknownVersion = progressive();
  unknownVersion.mediaSession.playbackContract.contractVersion = 2;
  unknownVersion.mediaSession.playback.seek.supported = false;
  window.VdrSuitePlaybackContractConsumer.consume(unknownVersion);
  assert.strictEqual(unknownVersion.mediaSession.playback.seek.supported, false);

  const mismatched = progressive();
  mismatched.mediaSession.playbackContract.presentationProfileId = 'hls-fmp4';
  mismatched.mediaSession.playback.seek.supported = false;
  window.VdrSuitePlaybackContractConsumer.consume(mismatched);
  assert.strictEqual(mismatched.mediaSession.playback.seek.supported, false,
    'profile mismatch must fail closed instead of projecting foreign semantics');

  console.log('phase65d playback contract consumer ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
