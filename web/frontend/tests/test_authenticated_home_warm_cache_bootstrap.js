'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const continueSource = fs.readFileSync(
  path.join(frontendRoot, 'home-continue-watching.js'),
  'utf8'
);
const historySource = fs.readFileSync(
  path.join(frontendRoot, 'home-recently-watched.js'),
  'utf8'
);

function eventDocument() {
  const listeners = new Map();
  return {
    readyState: 'complete',
    head: {appendChild() {}},
    body: {appendChild() {}},
    getElementById() { return null; },
    querySelector() { return null; },
    createElement() {
      return {
        style: {},
        dataset: {},
        classList: {
          add() {},
          remove() {},
          toggle() {},
          contains() { return false; }
        },
        setAttribute() {},
        appendChild() {},
        append() {},
        replaceChildren() {},
        addEventListener() {}
      };
    },
    addEventListener(name, listener) {
      const values = listeners.get(name) || [];
      values.push(listener);
      listeners.set(name, values);
    },
    dispatch(name) {
      (listeners.get(name) || []).slice().forEach(function (listener) {
        listener({type: name});
      });
    }
  };
}

function pendingResponse() {
  return new Promise(function () {});
}

(function verifyContinueWatchingGate() {
  const authenticated = {value: false};
  const requests = [];
  const document = eventDocument();
  const window = {
    document,
    console,
    VdrSuitePlatform: {
      getSelectedBackendId() { return 'default'; }
    },
    VdrSuiteBrowserSession: {
      isAuthenticated() { return authenticated.value; },
      snapshot() { return {authenticated: authenticated.value}; },
      csrfHeaders() { return {'X-CSRF-Token': 'test-token'}; }
    },
    fetch(url, options) {
      requests.push({url, options});
      return pendingResponse();
    }
  };
  window.window = window;

  const context = vm.createContext({
    window,
    document,
    console,
    Promise,
    Object,
    String,
    Number,
    Array,
    Boolean,
    JSON,
    Set,
    Date,
    Math,
    fetch: window.fetch,
    setTimeout,
    clearTimeout
  });

  vm.runInContext(
    continueSource,
    context,
    {filename: 'home-continue-watching.js'}
  );

  assert.strictEqual(
    requests.length,
    0,
    'Continue Watching install must not issue a private request before authentication'
  );

  document.dispatch('vdr-suite:home-resume');
  assert.strictEqual(
    requests.length,
    0,
    'Continue Watching must remain dormant on an anonymous Home event'
  );

  authenticated.value = true;
  document.dispatch('vdr-suite:home-resume');
  assert.strictEqual(
    requests.length,
    1,
    'authenticated Home resume must start Continue Watching immediately'
  );
  assert.strictEqual(requests[0].url, '/api/media/continue-watching');
}());

(function verifyHistoryAndRecentMoviesGate() {
  const authenticated = {value: false};
  const historyRequests = [];
  const recordingRequests = [];
  const document = eventDocument();

  const client = {
    fetchClientRecordings(options) {
      recordingRequests.push(options);
      return pendingResponse();
    }
  };

  const window = {
    document,
    console,
    Date,
    setTimeout,
    clearTimeout,
    VdrSuitePlatform: {
      getSelectedBackendId() { return 'default'; },
      getSelectedModule() { return 'overview'; },
      getClientApi() { return client; }
    },
    VdrSuiteBrowserSession: {
      isAuthenticated() { return authenticated.value; },
      snapshot() { return {authenticated: authenticated.value}; },
      csrfHeaders() { return {'X-CSRF-Token': 'test-token'}; }
    },
    VdrSuiteHomeRecordingDiscovery: {
      _test: {
        canonicalRecordings(payload) {
          return payload && Array.isArray(payload.recordings)
            ? payload.recordings
            : [];
        },
        recordingPosterUrl() { return ''; },
        openRecording() { return Promise.resolve(true); }
      }
    },
    fetch(url, options) {
      historyRequests.push({url, options});
      return pendingResponse();
    }
  };
  window.window = window;

  const context = vm.createContext({
    window,
    document,
    console,
    Promise,
    Object,
    String,
    Number,
    Array,
    Boolean,
    JSON,
    Set,
    Date,
    Math,
    fetch: window.fetch,
    setTimeout,
    clearTimeout
  });

  vm.runInContext(
    historySource,
    context,
    {filename: 'home-recently-watched.js'}
  );

  assert.strictEqual(
    historyRequests.length,
    0,
    'Recently Watched install must not issue a private request before authentication'
  );
  assert.strictEqual(
    recordingRequests.length,
    0,
    'Recent Movies install must not issue Recording queries before authentication'
  );

  document.dispatch('vdr-suite:home-resume');
  assert.strictEqual(
    historyRequests.length,
    0,
    'Recently Watched must remain dormant on an anonymous Home event'
  );
  assert.strictEqual(
    recordingRequests.length,
    0,
    'Recent Movies must remain dormant on an anonymous Home event'
  );

  authenticated.value = true;
  document.dispatch('vdr-suite:home-resume');

  assert.strictEqual(
    historyRequests.length,
    1,
    'authenticated Home resume must start actor-scoped history immediately'
  );
  assert.strictEqual(historyRequests[0].url, '/api/media/recently-watched');
  assert.strictEqual(
    recordingRequests.length,
    1,
    'authenticated Home resume must start independent cache-backed Recording discovery immediately'
  );
  assert.strictEqual(recordingRequests[0].query.backend, 'default');
}());

console.log('authenticated Home warm-cache bootstrap: PASS');
