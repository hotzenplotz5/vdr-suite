'use strict';

// A Recording MediaSession is reaped after 300 s of inactivity. The 700 ms
// Hero prewarm must never reuse an expired session after an idle Detail stay.
const assert = require('assert').strict;
const fs = require('fs');
const vm = require('vm');

const source = fs.readFileSync('web/frontend/recordings2-hero-prewarm.js', 'utf8');
let clockMs = 0;
const timers = [];
const actions = [];
let sessionSerial = 0;
const FakeDate = class extends Date { static now() { return clockMs; } };
const hero = {
  enhance(root) { return root; },
  __test: {
    resumePosition(recording) { return recording.resumePositionSeconds || 0; },
    showMode(root, mode) { root.dataset.recordings2HeroMode = mode; }
  }
};
const window = {
  VdrSuiteRecordings2HeroDetail: hero,
  VdrSuiteRecordingFastPlayback: {},
  MediaSource: {isTypeSupported() { return true; }},
  fetch() {},
  AbortController: function () {},
  ReadableStream: function () {},
  URL: {createObjectURL() {}, revokeObjectURL() {}},
  document: {visibilityState: 'visible'},
  setTimeout(callback, ms) {
    timers.push({callback, ms});
    return timers.length;
  }
};
window.window = window;
vm.runInNewContext(source, {window, console, Promise, Date: FakeDate});
const prewarm = window.VdrSuiteRecordings2HeroDetail.__test;

function makeDetail() {
  let id = '';
  let createdAt = -1;
  let used = false;
  const player = {
    state() { return id ? 'starting' : 'idle'; },
    start(options) {
      if (id) return Promise.resolve(id);
      id = 'session-' + (++sessionSerial);
      createdAt = clockMs;
      actions.push(['start', id, options && options.autoPlay]);
      return Promise.resolve(id);
    },
    stop() {
      actions.push(['stop', id]);
      id = '';
      return Promise.resolve(true);
    },
    startAtAbsolute(position) {
      actions.push(['seek', id, position]);
      // The server has reaped an idle grant; reuse cannot work.
      if (id && clockMs - createdAt >= 300000) {
        return Promise.reject(new Error('media_session_not_active'));
      }
      return (id ? Promise.resolve(id) : this.start()).then(function (sessionId) {
        actions.push(['seek-confirmed', sessionId, position]);
        return true;
      });
    },
    play() {
      used = true;
      actions.push(['play', id]);
      return Promise.resolve(true);
    }
  };
  const root = {
    dataset: {recordings2HeroMode: 'detail'},
    isConnected: true,
    __vdrSuiteRecordingPlaybackOwner: player,
    querySelector() {return null;}
  };
  return {root, player, wasPlayed() { return used; }};
}
async function drainTimers() {
  while (timers.length) {
    const timer = timers.shift();
    timer.callback();
    for (let n = 0; n < 12; n += 1) await Promise.resolve();
  }
}

(async () => {
  const fresh = makeDetail();
  prewarm.schedulePlaybackPrewarm(fresh.root);
  assert.equal(timers[0].ms, 700);
  await drainTimers();
  assert.deepEqual(actions, [['start', 'session-1', false]]);
  clockMs = 120000; // Recently prepared session still valid.
  prewarm.playPrepared(fresh.root, {resumePositionSeconds: 210});
  await drainTimers();
  assert.equal(fresh.wasPlayed(), true);
  assert.equal(actions.filter(entry => entry[0] === 'stop').length, 0);
  assert.equal(actions.some(entry => entry[0] === 'seek-confirmed' && entry[2] === 210), true);

  actions.length = 0;
  clockMs = 150000;
  const stale = makeDetail();
  prewarm.schedulePlaybackPrewarm(stale.root);
  await drainTimers();
  assert.equal(actions[0][0], 'start');
  assert.equal(actions[0][2], false);
  clockMs += 360000; // Coffee break: server's 300 s idle lifetime elapsed.
  prewarm.playPrepared(stale.root, {resumePositionSeconds: 4800});
  await drainTimers();
  assert.deepEqual(actions.map(entry => entry[0]), [
    'start', 'stop', 'seek', 'start', 'seek-confirmed', 'play'
  ], 'the stale prewarm must stop and create a fresh MediaSession before seeking');
  assert.equal(stale.wasPlayed(), true);

  // No endless timer, background keepalive or spontaneous replay.
  assert.equal(timers.length, 0);
  console.log('Recording Hero expired prewarm -> fresh MediaSession on Play PASS');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
