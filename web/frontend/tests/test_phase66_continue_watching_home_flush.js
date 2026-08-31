'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const homeSource = fs.readFileSync(path.join(frontendRoot, 'home-continue-watching.js'), 'utf8');
const syncSource = fs.readFileSync(path.join(frontendRoot, 'api', 'continue-watching-sync.js'), 'utf8');

assert(syncSource.includes('function flushPending()'));
assert(syncSource.includes('flush: flushPending'));
assert(homeSource.includes("typeof sync.flush === 'function'"));
assert(homeSource.includes('refreshHomeAfterPersistence'));
assert(homeSource.includes('VdrSuiteHomeRecentlyWatched'));

let assignedPlayback = {};
let releaseContinue;
let releaseHistory;
const syncContext = {
  window: {},
  console,
  setTimeout,
  clearTimeout,
  MutationObserver: class { observe() {} disconnect() {} },
  fetch(requestPath) {
    if (requestPath === '/api/media/continue-watching') {
      return new Promise(resolve => { releaseContinue = () => resolve({ok: true}); });
    }
    if (requestPath === '/api/media/recently-watched') {
      return new Promise(resolve => { releaseHistory = () => resolve({ok: true}); });
    }
    return Promise.resolve({ok: true});
  }
};
syncContext.window.window = syncContext.window;
syncContext.window.fetch = syncContext.fetch;
syncContext.window.setTimeout = setTimeout;
syncContext.window.clearTimeout = clearTimeout;
syncContext.window.MutationObserver = syncContext.MutationObserver;
syncContext.window.VdrSuiteBrowserSession = {csrfHeaders() { return {}; }};
Object.defineProperty(syncContext.window, 'VdrSuiteRecordings2Playback', {
  configurable: true,
  enumerable: true,
  get() { return assignedPlayback; },
  set(value) { assignedPlayback = value; }
});
vm.createContext(syncContext);
vm.runInContext(syncSource, syncContext, {filename: 'continue-watching-sync.js'});
const sync = syncContext.window.VdrSuiteContinueWatchingSync;
assert(sync && typeof sync.flush === 'function');

function flushMicrotasks(count = 8) {
  let promise = Promise.resolve();
  for (let index = 0; index < count; index += 1) promise = promise.then(() => Promise.resolve());
  return promise;
}

(async function () {
  const continueWrite = sync.__test.enqueue({
    operation: 'progress', backendId: 'default', recordingId: 'r1',
    positionSeconds: 42, resumeSupported: true, operationId: 'cw-flush-test'
  });
  const historyWrite = sync.__test.enqueueHistory({
    operation: 'activity', backendId: 'default', recordingId: 'r1',
    positionSeconds: 42, resumeSupportKnown: true, resumeSupported: true,
    ended: false, operationId: 'rw-flush-test'
  });
  await flushMicrotasks();
  assert.strictEqual(typeof releaseContinue, 'function');
  assert.strictEqual(typeof releaseHistory, 'function');

  let flushResolved = false;
  const pendingFlush = sync.flush().then(() => { flushResolved = true; });
  await flushMicrotasks();
  assert.strictEqual(flushResolved, false, 'flush must wait for both persistence queues');
  releaseContinue();
  await continueWrite;
  await flushMicrotasks();
  assert.strictEqual(flushResolved, false, 'History persistence is still pending');
  releaseHistory();
  await historyWrite;
  await pendingFlush;
  assert.strictEqual(flushResolved, true);

  let releaseHomeFlush;
  let continueLists = 0;
  let recentRefreshes = 0;
  const scheduled = [];
  const homeContext = {
    window: {},
    document: {
      readyState: 'loading',
      addEventListener() {},
      querySelector() { return null; }
    },
    console,
    setTimeout(callback, delay) { scheduled.push({callback, delay}); return scheduled.length; },
    clearTimeout() {},
    fetch: async function (requestPath, options) {
      if (requestPath === '/api/media/continue-watching') {
        continueLists += 1;
        const body = JSON.parse(options.body);
        assert.strictEqual(body.operation, 'list');
      }
      return {ok: true, json: async () => ({items: []})};
    }
  };
  homeContext.window.window = homeContext.window;
  homeContext.window.document = homeContext.document;
  homeContext.window.fetch = homeContext.fetch;
  homeContext.window.setTimeout = homeContext.setTimeout;
  homeContext.window.clearTimeout = homeContext.clearTimeout;
  homeContext.window.VdrSuiteContinueWatchingSync = {
    flush() { return new Promise(resolve => { releaseHomeFlush = resolve; }); }
  };
  homeContext.window.VdrSuiteHomeRecentlyWatched = {
    refresh() { recentRefreshes += 1; return Promise.resolve(true); }
  };
  vm.createContext(homeContext);
  vm.runInContext(homeSource, homeContext, {filename: 'home-continue-watching.js'});
  const home = homeContext.window.VdrSuiteHomeContinueWatching;
  assert(home && home._test);

  const refreshAfterStop = home._test.refreshHomeAfterPersistence();
  await flushMicrotasks();
  assert.strictEqual(continueLists, 0, 'Home must not read Continue Watching before queued Stop persistence completes');
  assert.strictEqual(recentRefreshes, 0, 'History must not race ahead of queued Stop persistence');
  releaseHomeFlush(true);
  await refreshAfterStop;
  assert.strictEqual(continueLists, 1);
  assert.strictEqual(recentRefreshes, 1, 'both Home rails refresh from committed server state');

  console.log('phase66 Stop-to-Home refresh waits for Continue/History persistence');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
