'use strict';

const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontend = path.join(__dirname, '..');
const homeSource = fs.readFileSync(path.join(frontend, 'home-continue-watching.js'), 'utf8');
const syncSource = fs.readFileSync(path.join(frontend, 'api', 'continue-watching-sync.js'), 'utf8');

const calls = [];
const opened = [];
let revision = 7;
let persistedPosition = 180;
let releaseClear = null;
let nextClearFailure = false;

const document = {
  readyState: 'loading',
  addEventListener() {},
  querySelector() { return null; }
};

const window = {
  document,
  console,
  setTimeout,
  clearTimeout,
  VdrSuiteBrowserSession: {
    isAuthenticated() { return true; },
    csrfHeaders() { return {'X-VDR-Suite-CSRF': 'cw-h2-test'}; }
  },
  VdrSuiteHomeLivePreview: {
    cancel() { calls.push('preview-cancel'); return true; }
  },
  selectModule(name) { calls.push('module:' + name); }
};
window.window = window;

let playback = {createPanel() {}};
Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
  configurable: true, enumerable: true,
  get() { return playback; },
  set(value) { playback = value; }
});
window.VdrSuiteRecordings2 = {
  openRecording(recording, options) {
    calls.push('open:' + options.playbackStartPositionSeconds);
    opened.push({recording, options});
  }
};
window.fetch = function (url, options) {
  assert.equal(url, '/api/media/continue-watching');
  assert.equal(options.credentials, 'same-origin');
  assert.equal(options.headers['X-VDR-Suite-CSRF'], 'cw-h2-test');
  const body = JSON.parse(options.body);
  calls.push('request:' + body.operation);
  if (body.operation === 'revision') {
    return Promise.resolve({
      ok: true, status: 200, json: async () => ({revision})
    });
  }
  if (body.operation === 'clear') {
    calls.push('expected:' + body.expectedRevision);
    return new Promise(resolve => {
      releaseClear = function () {
        if (nextClearFailure) {
          nextClearFailure = false;
          resolve({ok: false, status: 503});
        } else if (body.expectedRevision !== revision) {
          resolve({ok: false, status: 409,
            json: async () => ({currentRevision: revision})});
        } else {
          revision += 1;
          persistedPosition = 0;
          resolve({ok: true, status: 200,
            json: async () => ({ok: true, revision})});
        }
      };
    });
  }
  if (body.operation === 'progress') {
    if (body.expectedRevision !== revision) {
      return Promise.resolve({ok: false, status: 409});
    }
    persistedPosition = body.positionSeconds;
    revision += 1;
    return Promise.resolve({ok: true, status: 200,
      json: async () => ({ok: true, revision})});
  }
  throw new Error('Unexpected Continue Watching operation: ' + body.operation);
};

const context = vm.createContext({
  window, document, fetch: window.fetch, console, Promise, Object, Array,
  String, Number, Boolean, Math, Date, Map, Set, setTimeout, clearTimeout
});
vm.runInContext(syncSource, context, {filename: 'continue-watching-sync.js'});
vm.runInContext(homeSource, context, {filename: 'home-continue-watching.js'});
const home = window.VdrSuiteHomeContinueWatching._test;
const sync = window.VdrSuiteContinueWatchingSync.__test;

function card() {
  return {__vdrSuiteContinueView: {
    resume: {disabled: false},
    restart: {disabled: false},
    error: {textContent: '', hidden: true}
  }};
}

function item(progressRevision) {
  return {
    backendId: 'default', recordingId: '18', backendNativeId: 'native-18',
    title: 'Film', resumePositionSeconds: 180, progressRevision,
    durationKnown: true, durationSeconds: 8085,
    recording: {id: '18', title: 'Film'}
  };
}

async function flush() {
  for (let n = 0; n < 30; n += 1) await Promise.resolve();
}

(async function () {
  // The API list's revision must survive Home normalization.
  assert.equal(home.normalizeItem(item(7)).progressRevision, 7);
  assert.equal(home.normalizeItem({...item(7), progressRevision: undefined}).progressRevision, null);

  const firstCard = card();
  const first = home.openItem(item(7), false, firstCard);
  const duplicate = home.openItem(item(7), false, firstCard);
  assert.equal(first, duplicate, 'rapid double-click must return the same pending operation');
  assert.equal(firstCard.__vdrSuiteContinueView.restart.disabled, true);
  await flush();
  assert.equal(typeof releaseClear, 'function');
  assert.equal(opened.length, 0, 'do not open a Recording while clear is in flight');
  assert(!calls.includes('preview-cancel'), 'do not relinquish active preview before clear succeeds');
  assert(!calls.includes('request:revision'),
    'Home restart must use its displayed CAS revision, not rebase on another Device');
  assert(calls.includes('expected:7'));

  // Another authorized Device progresses after Home rendered revision 7.
  revision = 8;
  persistedPosition = 500;
  releaseClear();
  assert.equal(await first, false, 'stale Home clear must fail on Device CAS conflict');
  assert.equal(opened.length, 0);
  assert.equal(persistedPosition, 500);
  assert.equal(firstCard.__vdrSuiteContinueView.error.hidden, false,
    'a failed clear must show a real error on the Home card');
  assert.equal(firstCard.__vdrSuiteContinueView.restart.disabled, false);

  // A canonical refresh gives Home revision 8; session may start only after
  // the successful, acknowledged reset of that exact version.
  const secondCard = card();
  const second = home.openItem(item(8), false, secondCard);
  await flush();
  assert.equal(opened.length, 0);
  releaseClear();
  assert.equal(await second, true);
  assert.equal(persistedPosition, 0);
  assert.equal(revision, 9);
  assert.equal(opened.length, 1);
  assert.equal(opened[0].options.playbackStartPositionSeconds, 0);
  assert(calls.indexOf('open:0') > calls.lastIndexOf('expected:8'));

  // A stale second Browser may not clear the new revision or create a
  // duplicate playback owner.
  const stale = home.openItem(item(8), false, card());
  await flush();
  releaseClear();
  assert.equal(await stale, false);
  assert.equal(opened.length, 1);
  assert.equal(persistedPosition, 0);

  // A server-side error must remain visible and must not open a session.
  nextClearFailure = true;
  const failingCard = card();
  const failed = home.openItem(item(9), false, failingCard);
  await flush();
  releaseClear();
  assert.equal(await failed, false);
  assert.equal(opened.length, 1);
  assert.equal(failingCard.__vdrSuiteContinueView.error.hidden, false);

  // A missing list revision is not permission to obtain a fresh revision and
  // erase progress from another Device.
  const missingRevision = await home.openItem(item(undefined), false, card());
  assert.equal(missingRevision, false);
  assert.equal(opened.length, 1);

  // The ordinary Continue action still does not clear the saved progress.
  const continueOpened = await home.openItem(item(9), true, card());
  assert.equal(continueOpened, true);
  assert.equal(opened.length, 2);
  assert.equal(opened[1].options.playbackStartPositionSeconds, 180);

  // The existing old-owner CAS rule still prevents progress from reappearing
  // after the successful reset.
  const oldOwner = {revision: 8, loading: Promise.resolve(8), blocked: false};
  assert.equal(await sync.enqueueVersioned({
    operation: 'progress', backendId: 'default', recordingId: '18',
    positionSeconds: 600, resumeSupported: true, operationId: 'stale-device'
  }, oldOwner), false);
  assert.equal(persistedPosition, 0);

  console.log('CW.H2 restart ordering, visible errors, double-click and cross-device CAS PASS');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
