'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'continue-watching-sync.js'),
  'utf8'
);

assert(source.includes('waitForResumeReady'));
assert(source.includes('owner.canResume() === true'));
assert(source.includes('return waitForResumeReady(owner, 0);'));
assert(!source.includes('recordingPlaybackStatus'), 'Continue Watching must not create a second index-status poll');

const scheduled = [];
let assignedPlayback = {};
function setTimeoutFake(callback, delay) {
  scheduled.push({callback, delay});
  return scheduled.length;
}

const window = {
  window: null,
  console,
  Date,
  setTimeout: setTimeoutFake,
  clearTimeout() {},
  fetch: async () => ({ok: true}),
  VdrSuiteBrowserSession: {csrfHeaders() { return {}; }}
};
window.window = window;
Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
  configurable: true,
  enumerable: true,
  get() { return assignedPlayback; },
  set(value) { assignedPlayback = value; }
});

const context = vm.createContext({
  window,
  console,
  Date,
  Promise,
  Object,
  String,
  Number,
  Boolean,
  Math,
  Error,
  setTimeout: setTimeoutFake,
  clearTimeout() {}
});
vm.runInContext(source, context, {filename: 'continue-watching-sync.js'});

const api = window.VdrSuiteContinueWatchingSync;
assert(api && api.__test && typeof api.__test.startAtAbsolute === 'function');

function flush(count = 8) {
  let promise = Promise.resolve();
  for (let index = 0; index < count; index += 1) promise = promise.then(() => Promise.resolve());
  return promise;
}

(async function () {
  const calls = [];
  let resumeReady = false;
  const fastOwner = {
    start() {
      calls.push('start');
      return Promise.resolve('fast-session');
    },
    canResume() {
      calls.push('canResume:' + String(resumeReady));
      return resumeReady;
    },
    state() { return 'playing'; },
    seekAbsolute(position) {
      calls.push('seek:' + position);
      return Promise.resolve(true);
    }
  };

  const resume = api.__test.startAtAbsolute(fastOwner, 82);
  await flush();
  assert.strictEqual(calls[0], 'start');
  assert.strictEqual(calls.includes('seek:82'), false, 'seek must not run before canonical resume truth is ready');
  assert.strictEqual(scheduled.length, 1, 'Continue Watching may wait only on in-memory owner readiness');
  assert.strictEqual(scheduled[0].delay, 100);

  resumeReady = true;
  scheduled.shift().callback();
  await resume;
  assert.strictEqual(calls.includes('seek:82'), true, 'seek must run after the canonical owner reports resume readiness');
  assert.ok(calls.indexOf('seek:82') > calls.indexOf('canResume:true'));

  const hlsCalls = [];
  await api.__test.startAtAbsolute({
    startAtAbsolute(position) {
      hlsCalls.push(position);
      return Promise.resolve('hls-session');
    }
  }, 82);
  assert.deepStrictEqual(hlsCalls, [82], 'HLS keeps its canonical server-side absolute-start path');

  await assert.rejects(
    api.__test.startAtAbsolute({
      start() { return Promise.resolve('stopped-session'); },
      canResume() { return false; },
      state() { return 'stopped'; },
      seekAbsolute() { throw new Error('must not seek'); }
    }, 82),
    /stopped before resume became ready/
  );

  console.log('phase66 Continue Watching waits for canonical fast-owner resume readiness');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});