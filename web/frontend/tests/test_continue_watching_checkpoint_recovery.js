'use strict';

const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'continue-watching-sync.js'), 'utf8');

let revision = 2;
let storedPosition = 1531; // Observed stale Home progress: 25:31.
let lastOperation = '';
let baselineFailures = 1;
let writeFailures = 0;
let loseAckFor = '';
const writes = [];
const timers = [];
const alerts = [];
let playback = {};

function response(ok, status, body) {
  return {ok, status, json: async () => body};
}
const window = {
  VdrSuiteBrowserSession: {
    csrfHeaders() { return {'X-CSRF-Token': 'valid'}; }
  },
  document: {
    createElement() {
      return {
        textContent: '',
        setAttribute() {},
        remove() { alerts.push('removed'); }
      };
    }
  },
  setTimeout(callback, delay) {
    if (delay === 5000) timers.push(callback);
    return timers.length + 1;
  },
  clearTimeout() {},
  MutationObserver: class {observe() {} disconnect() {}},
  fetch: async function (url, init) {
    assert.equal(url, '/api/media/continue-watching');
    assert.equal(init.credentials, 'same-origin');
    const body = JSON.parse(init.body);
    if (body.operation === 'revision') {
      if (baselineFailures > 0) {
        baselineFailures -= 1;
        return response(false, 503, null);
      }
      return response(true, 200, {revision});
    }
    if (body.operation === 'progress' || body.operation === 'clear') {
      writes.push(body);
      if (writeFailures > 0) {
        writeFailures -= 1;
        return response(false, 503, null);
      }
      if (body.operationId === lastOperation) {
        assert.equal(body.positionSeconds || 0, storedPosition,
          'exact request replay must retain the first committed checkpoint');
        return response(true, 200, {ok: true, revision, replayed: true});
      }
      if (body.expectedRevision !== revision)
        return response(false, 409, {currentRevision: revision});
      storedPosition = body.operation === 'clear' ? 0 : body.positionSeconds;
      lastOperation = body.operationId;
      revision += 1;
      if (body.operationId === loseAckFor) {
        loseAckFor = '';
        throw new Error('network dropped the successful server acknowledgement');
      }
      return response(true, 200, {ok: true, revision});
    }
    return response(true, 200, {ok: true});
  }
};
Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
  configurable: true,
  get() { return playback; },
  set(value) { playback = value; }
});
window.window = window;
const context = vm.createContext({window, console, Promise, Number, String, Math, Object, Date, Error});
vm.runInContext(source, context, {filename: 'continue-watching-sync.js'});
const sync = window.VdrSuiteContinueWatchingSync.__test;
const progress = (position, id) => ({
  operation: 'progress', backendId: 'default', recordingId: 'medicus2',
  positionSeconds: position, resumeSupported: true, operationId: id
});
const flush = (times = 24) => {
  let ready = Promise.resolve();
  for (let i = 0; i < times; i += 1) ready = ready.then(() => Promise.resolve());
  return ready;
};

(async function () {
  // A temporary failure to read the baseline must NOT disable the owner for
  // the remaining 1h20 of playback. No write can precede a valid baseline.
  const state = {revision: null, loading: null, blocked: false};
  assert.equal(await sync.loadRevision(state, 'default', 'medicus2'), null);
  assert.equal(state.blocked, false);
  assert.equal(writes.length, 0);
  assert.equal(await sync.enqueueVersioned(progress(4800, 'medicus-recovered'), state), true);
  assert.equal(storedPosition, 4800);
  assert.equal(state.revision, 3);

  // Two unanswered 503 writes must retain their CAS and operation ID.
  // When the connection recovers, the pending write is acknowledged first
  // and the latest current position then reaches server truth.
  writeFailures = 2;
  assert.equal(await sync.enqueueVersioned(progress(4810, 'medicus-pending'), state), false);
  assert.equal(storedPosition, 4800);
  assert.equal(state.blocked, false);
  assert.equal(state.pending.operationId, 'medicus-pending');
  assert.equal(await sync.enqueueVersioned(progress(4850, 'medicus-current'), state), true);
  assert.equal(storedPosition, 4850);
  assert.equal(state.pending, null);
  assert.equal(writes.filter(x => x.operationId === 'medicus-pending').length, 3);
  assert.equal(writes.filter(x => x.operationId === 'medicus-current').length, 1);

  // A write may commit while the acknowledgement is lost. A duplicate
  // idempotent operation is a replay, not a second revision/write.
  loseAckFor = 'medicus-lost-ack';
  const beforeAck = revision;
  assert.equal(await sync.enqueueVersioned(progress(4860, 'medicus-lost-ack'), state), true);
  assert.equal(storedPosition, 4860);
  assert.equal(revision, beforeAck + 1);
  assert.equal(writes.filter(x => x.operationId === 'medicus-lost-ack').length, 2);

  // Still preserve P1 Browser/Device CAS fencing. Do not silently rebase a
  // stale old owner after another linked device advanced the position.
  storedPosition = 5400;
  lastOperation = 'another-device';
  revision += 1;
  assert.equal(await sync.enqueueVersioned(progress(4870, 'stale-browser'), state), false);
  assert.equal(storedPosition, 5400);
  assert.equal(state.blocked, true);

  // A fresh active player MUST retry the exact unchanged position after a
  // rejected save: old code marked lastPosition before the acknowledgement
  // and then never tried again at the same position.
  const listeners = [];
  const ownerElement = {
    appendChild(node) { alerts.push(node); },
    querySelectorAll() { return []; }
  };
  const owner = {
    element: ownerElement,
    position() { return 4800; },
    duration() { return 9000; },
    canResume() { return true; },
    snapshot() { return {state: 'playing', sessionId: 'owner-new'}; },
    subscribe(callback) {
      listeners.push(callback);
      callback(this.snapshot());
      return function () {};
    }
  };
  const decorated = sync.decorateOwner(owner, {id: 'medicus2'}, 'default');
  assert(decorated);
  await flush();
  assert.equal(timers.length, 1);
  writeFailures = 2;
  timers.shift()();
  await sync.flush();
  await flush();
  assert.equal(timers.length, 1);
  assert.equal(storedPosition, 5400, 'failure must not claim persistence');
  assert(alerts.some(a => a && a.textContent && a.textContent.includes('nicht gespeichert')),
    'the playback owner must visibly report a failed persistence attempt');
  timers.shift()();
  await sync.flush();
  await flush();
  assert.equal(storedPosition, 4800, 'same-position retry must recover the saved checkpoint');
  assert(alerts.includes('removed'), 'save success clears the warning');

  console.log('Continue Watching checkpoint recovery, CAS and same-position retry PASS');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
