'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const repositoryRoot = path.join(frontendRoot, '..', '..');
const homeSource = fs.readFileSync(path.join(frontendRoot, 'home-recently-watched.js'), 'utf8');
const syncSource = fs.readFileSync(path.join(frontendRoot, 'api', 'continue-watching-sync.js'), 'utf8');
const pathsSource = fs.readFileSync(path.join(repositoryRoot, 'core', 'http', 'src', 'TestHttpServerPaths.inc'), 'utf8');
const apiSource = fs.readFileSync(path.join(repositoryRoot, 'api', 'rest', 'src', 'ContinueWatchingApiRuntime.cpp'), 'utf8');
const historyHeader = fs.readFileSync(path.join(repositoryRoot, 'core', 'media', 'include', 'RecentlyWatched.h'), 'utf8');
const securitySource = fs.readFileSync(path.join(repositoryRoot, 'core', 'security', 'include', 'ContinueWatchingSecurityRequest.h'), 'utf8');

assert(homeSource.includes('/api/media/recently-watched'));
assert(syncSource.includes("const HISTORY_ENDPOINT = '/api/media/recently-watched'"));
assert(syncSource.includes('snapshotActive(latestSnapshot)'));
assert(syncSource.includes('owner.subscribe(lifecycleChanged)'));
assert(syncSource.includes('syncHistory(true, true)'));
assert(!homeSource.includes('localStorage'));
assert(!syncSource.includes('localStorage'));
assert(!homeSource.includes("document.createElement('video')"));
assert(homeSource.includes('VdrSuiteRecordings2.openRecording'));
assert(homeSource.includes("typeof preview.cancel === 'function'"));
assert(pathsSource.includes(
  '{"/frontend/home-recording-discovery.js", "home-recording-discovery.js", "application/javascript; charset=utf-8", "home-recently-watched.js"}'
));
assert(apiSource.includes('RecentlyWatchedRoute = "/api/media/recently-watched"'));
assert(apiSource.includes('recentlyWatchedService_->recordActivity'));
assert(securitySource.includes('/api/media/recently-watched'));
assert(historyHeader.includes('MaxItemsPerActorBackend = 100'));
assert(historyHeader.includes('canonical-recording-playback-owner'));
assert(!historyHeader.toLowerCase().includes('recommend'));

const homeContext = {
  window: {},
  document: {
    readyState: 'loading',
    addEventListener() {},
    querySelector() { return null; }
  },
  fetch: async () => ({ok: true, json: async () => ({items: []})}),
  console,
  setTimeout,
  clearTimeout
};
homeContext.window.window = homeContext.window;
homeContext.window.document = homeContext.document;
homeContext.window.fetch = homeContext.fetch;
vm.createContext(homeContext);
vm.runInContext(homeSource, homeContext);
const home = homeContext.window.VdrSuiteHomeRecentlyWatched;
assert(home && home._test);

const base = {
  backendId: 'default',
  recordingId: 'r1',
  positionKnown: true,
  positionSeconds: 42,
  completionKnown: true,
  completed: false,
  resumeRelevanceKnown: true,
  resumeRelevant: true,
  sourceEvidence: 'canonical-recording-playback-owner',
  recording: {id: 'r1', recordingId: 'r1', backendId: 'default', title: 'Film'}
};
const normalized = home._test.normalizeItem(base, 'default');
assert(normalized);
assert.strictEqual(normalized.recordingId, 'r1');
assert.strictEqual(home._test.activityLabel(normalized), 'Fortsetzbar · 0:42');
assert.strictEqual(home._test.activityLabel({...normalized, completed: true}), 'Angesehen');
assert.strictEqual(home._test.normalizeItem({...base, backendId: 'other'}, 'default'), null);

const opened = [];
homeContext.window.selectModule = moduleName => opened.push('module:' + moduleName);
homeContext.window.VdrSuiteRecordings2 = {
  openRecording(recording, options) { opened.push({recording, options}); }
};

const syncRequests = [];
let assignedPlayback = {};
const syncContext = {
  window: {},
  fetch: async (requestPath, options) => {
    syncRequests.push({path: requestPath, options});
    return {ok: true};
  },
  console,
  setTimeout,
  clearTimeout,
  MutationObserver: class { observe() {} disconnect() {} }
};
syncContext.window.window = syncContext.window;
syncContext.window.fetch = syncContext.fetch;
syncContext.window.setTimeout = setTimeout;
syncContext.window.clearTimeout = clearTimeout;
syncContext.window.MutationObserver = syncContext.MutationObserver;
syncContext.window.VdrSuiteBrowserSession = {
  csrfHeaders() { return {'X-VDR-Suite-CSRF': 'phase66-history-csrf'}; }
};
Object.defineProperty(syncContext.window, 'VdrSuiteRecordings2Playback', {
  configurable: true,
  enumerable: true,
  get() { return assignedPlayback; },
  set(value) { assignedPlayback = value; }
});
vm.createContext(syncContext);
vm.runInContext(syncSource, syncContext);
const sync = syncContext.window.VdrSuiteContinueWatchingSync;
assert(sync && sync.__test);

(async function () {
  await sync.__test.postHistory({
    operation: 'activity',
    backendId: 'default',
    recordingId: 'r1',
    positionSeconds: 42,
    resumeSupportKnown: true,
    resumeSupported: true,
    ended: false,
    operationId: 'rw-test-1'
  });
  assert.strictEqual(syncRequests.length, 1);
  assert.strictEqual(syncRequests[0].path, '/api/media/recently-watched');
  assert.strictEqual(syncRequests[0].options.credentials, 'same-origin');
  assert.strictEqual(syncRequests[0].options.headers['X-VDR-Suite-CSRF'], 'phase66-history-csrf');

  const historyRequests = [];
  homeContext.window.VdrSuiteBrowserSession = {
    csrfHeaders() { return {'X-VDR-Suite-CSRF': 'phase66-history-home'}; }
  };
  homeContext.window.fetch = async (requestPath, options) => {
    historyRequests.push({path: requestPath, options});
    return {ok: true, json: async () => ({items: []})};
  };
  await home._test.post({operation: 'list', backendId: 'default'});
  assert.strictEqual(historyRequests.length, 1);
  assert.strictEqual(historyRequests[0].path, '/api/media/recently-watched');
  assert.strictEqual(historyRequests[0].options.headers['X-VDR-Suite-CSRF'], 'phase66-history-home');

  assert.strictEqual(await home._test.openItem(normalized), true);
  assert.strictEqual(opened[0], 'module:recordings2');
  assert.strictEqual(opened[1].recording.id, 'r1');
  assert.strictEqual(opened[1].options.backendId, 'default');
  assert.strictEqual(opened[1].options.autoStartPlayback, undefined,
    'History opens the canonical Recording surface without inventing resume/playback ownership');

  console.log('phase66 recently watched contract ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
