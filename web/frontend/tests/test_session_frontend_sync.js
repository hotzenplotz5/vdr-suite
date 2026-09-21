'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'session-frontend-sync.js'),
  'utf8'
);

function createRuntime() {
  let subscriber = null;
  let backendRefreshes = 0;
  let reloads = 0;
  const documentListeners = {};

  const context = {
    window: null,
    Boolean,
    Object,
    console,
    document: {
      readyState: 'loading',
      visibilityState: 'visible',
      addEventListener(type, listener) {
        documentListeners[type] = listener;
      }
    },
    location: {
      reload() {
        reloads += 1;
      }
    },
    setTimeout(listener) {
      listener();
      return 1;
    },
    loadBackendSelection() {
      backendRefreshes += 1;
    },
    VdrSuiteBrowserSession: {
      subscribe(listener) {
        subscriber = listener;
        listener({authenticated: false, reason: ''});
        return function () {};
      }
    }
  };

  context.window = context;
  vm.createContext(context);
  vm.runInContext(source, context);

  assert.strictEqual(typeof documentListeners.DOMContentLoaded, 'function');
  documentListeners.DOMContentLoaded();
  assert.strictEqual(typeof subscriber, 'function');
  assert.strictEqual(backendRefreshes, 0);
  assert.strictEqual(reloads, 0);

  return {
    context,
    sessionChanged(state) {
      subscriber(state);
    },
    backendRefreshes() {
      return backendRefreshes;
    },
    reloads() {
      return reloads;
    }
  };
}

const runtime = createRuntime();

runtime.sessionChanged({authenticated: true, reason: ''});
assert.strictEqual(runtime.backendRefreshes(), 1);
assert.strictEqual(runtime.reloads(), 0);

runtime.sessionChanged({authenticated: true, reason: ''});
assert.strictEqual(runtime.backendRefreshes(), 1);
assert.strictEqual(runtime.reloads(), 0);

runtime.sessionChanged({authenticated: false, reason: 'logout'});
assert.strictEqual(runtime.backendRefreshes(), 1);
assert.strictEqual(runtime.reloads(), 1);

runtime.sessionChanged({authenticated: false, reason: 'logout'});
assert.strictEqual(runtime.reloads(), 1);

const leavingRuntime = createRuntime();
leavingRuntime.sessionChanged({authenticated: true, reason: ''});
leavingRuntime.context.document.visibilityState = 'hidden';
leavingRuntime.sessionChanged({authenticated: false, reason: 'authentication_required'});
assert.strictEqual(leavingRuntime.backendRefreshes(), 1);
assert.strictEqual(leavingRuntime.reloads(), 0);

console.log('session frontend synchronization ok');
