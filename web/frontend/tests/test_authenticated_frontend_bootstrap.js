'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const appSource = fs.readFileSync(path.join(frontendRoot, 'app.js'), 'utf8');

const start = appSource.indexOf('function loadBackendSelection()');
const end = appSource.indexOf('// Phase 58.90b:', start);
assert(start >= 0 && end > start, 'authenticated frontend bootstrap source block must be discoverable');

const bootstrapSource = appSource.slice(start, end);
assert(bootstrapSource.includes('window.VdrSuiteBrowserSession'));
assert(bootstrapSource.includes('session.subscribe(synchronizeBrowserSessionFrontend)'));
assert(bootstrapSource.includes('session.restore()'));
assert(bootstrapSource.includes("credentials: 'same-origin'"));
assert(bootstrapSource.includes('renderAuthenticationRequiredFrontend()'));

function node() {
  return {
    hidden: false,
    className: '',
    textContent: '',
    disabled: false,
    children: [],
    replaceChildren() {
      this.children = Array.from(arguments);
    },
    appendChild(child) {
      this.children.push(child);
      return child;
    }
  };
}

const statusElement = node();
const backendsElement = node();
const detailMetaElement = node();
const detailDataElement = node();
const refreshDetailButton = node();
const additionalSections = node();

let sessionListener = null;
let restoreCalls = 0;
let backendFetches = 0;
let detailLoads = 0;
let lastFetchOptions = null;

const session = {
  subscribe(listener) {
    sessionListener = listener;
    listener({authenticated: false, expiresAt: '', reason: ''});
    return function () {};
  },
  restore() {
    restoreCalls += 1;
    return Promise.resolve({authenticated: false, expiresAt: '', reason: 'authentication_required'});
  }
};

const context = {
  console,
  Promise,
  String,
  Array,
  Boolean,
  Object,
  window: {VdrSuiteBrowserSession: session},
  document: {
    createElement() { return node(); },
    querySelector(selector) {
      return selector === '[data-home-zone="additional-sections"]'
        ? additionalSections
        : null;
    }
  },
  statusElement,
  backendsElement,
  detailMetaElement,
  detailDataElement,
  refreshDetailButton,
  frontendPlatformClientApi() {
    return {
      fetchClientBackends(options) {
        backendFetches += 1;
        lastFetchOptions = options;
        return Promise.resolve({
          backends: [{backendId: 'default', backendName: 'Default'}]
        });
      }
    };
  },
  renderBackend(backend) {
    return {backend};
  },
  loadBackendDetails() {
    detailLoads += 1;
  }
};

vm.createContext(context);
vm.runInContext(
  `var selectedBackendId = 'default';
var selectedBackend = {backendId: 'default'};
var currentSnapshot = {recordingCount: 1};
var currentChannels = {channels: [1]};
var currentEvents = {events: [1]};
var currentTimers = {timers: [1]};
var currentTimerConflicts = {items: [1]};
var currentSearchTimers = {items: [1]};
var currentRecordings = {recordings: [1]};
var currentRecordingsBackendId = 'default';
${bootstrapSource}`,
  context,
  {filename: 'authenticated-frontend-bootstrap.js'}
);

async function flush() {
  await Promise.resolve();
  await Promise.resolve();
  await Promise.resolve();
}

(async function () {
  await flush();

  assert.strictEqual(restoreCalls, 1,
    'frontend bootstrap must participate in the single browser-session restore');
  assert.strictEqual(backendFetches, 0,
    'anonymous bootstrap must not request backend content');
  assert.strictEqual(detailLoads, 0,
    'anonymous bootstrap must not enter Home/backend details');
  assert.strictEqual(detailMetaElement.textContent,
    'Bitte anmelden, um VDR-Suite zu verwenden.');
  assert.strictEqual(detailDataElement.children.length, 1);
  assert.strictEqual(detailDataElement.children[0].textContent,
    'Anmeldung erforderlich.');

  sessionListener({authenticated: true, expiresAt: '2099-01-01T00:00:00Z', reason: ''});
  await flush();

  assert.strictEqual(backendFetches, 1,
    'successful authentication must start backend discovery exactly once');
  assert.strictEqual(detailLoads, 1,
    'successful authentication must enter the selected backend');
  assert.strictEqual(lastFetchOptions.credentials, 'same-origin');

  sessionListener({authenticated: true, expiresAt: '2099-01-01T00:00:00Z', reason: ''});
  await flush();
  assert.strictEqual(backendFetches, 1,
    'repeated authenticated notifications must not duplicate Home bootstrap');

  sessionListener({authenticated: false, expiresAt: '', reason: 'logout'});
  assert.strictEqual(backendsElement.children.length, 0,
    'logout must remove visible backend choices');
  assert.strictEqual(additionalSections.children.length, 0,
    'logout must clear additional Home projections');

  sessionListener({authenticated: true, expiresAt: '2099-01-01T00:00:00Z', reason: ''});
  await flush();
  assert.strictEqual(backendFetches, 2,
    'a later login must rebuild Home from authenticated data');

  console.log('authenticated frontend bootstrap: PASS');
}()).catch(function (error) {
  console.error(error && error.stack ? error.stack : error);
  process.exitCode = 1;
});
