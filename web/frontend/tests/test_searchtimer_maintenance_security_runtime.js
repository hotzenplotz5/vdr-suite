'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const loaderPath = path.resolve(
  __dirname,
  '..',
  'platform',
  'deferred-runtime-loader.js'
);
const loaderSource = fs.readFileSync(
  loaderPath,
  'utf8'
);
const requests = [];
const listeners = {};
const csrfToken =
  'searchtimer-mutation-csrf-runtime-test';

const document = {
  readyState: 'loading',
  documentElement: {lang: 'de'},
  head: {appendChild() {}},
  getElementById() {
    return null;
  },
  createElement() {
    return {
      dataset: {},
      addEventListener() {}
    };
  },
  addEventListener(type, listener) {
    listeners[type] = listener;
  }
};

const context = {
  window: null,
  document,
  console,
  Date,
  Error,
  JSON,
  Math,
  Number,
  Object,
  Promise,
  String,
  Array,
  URL,
  URLSearchParams,
  location: {
    href: 'https://vdr-suite.test/'
  },
  VdrSuiteBrowserSession: {
    csrfHeaders() {
      return {
        'X-CSRF-Token': csrfToken
      };
    }
  },
  fetch(input, init) {
    requests.push({input, init});
    return Promise.resolve({
      status: 200,
      ok: true,
      clone() {
        return this;
      },
      text() {
        return Promise.resolve('{}');
      }
    });
  }
};

context.window = context;

vm.createContext(context);
vm.runInContext(
  loaderSource,
  context,
  {filename: loaderPath}
);

assert.strictEqual(
  context.__vdrSuiteSearchTimerCreateMutationCsrfWrapped,
  true
);
assert.strictEqual(
  context.__vdrSuiteSearchTimerMaintenanceMutationCsrfWrapped,
  true
);
assert.strictEqual(
  typeof listeners.DOMContentLoaded,
  'function'
);

function latestRequest() {
  return requests[requests.length - 1];
}

function csrfHeader() {
  const request = latestRequest();
  return request &&
    request.init &&
    request.init.headers &&
    request.init.headers['X-CSRF-Token'];
}

const exactRoutes = [
  '/api/searchtimers',
  '/api/vdr/searchtimers',
  '/api/searchtimers/update',
  '/api/vdr/searchtimers/update',
  '/api/searchtimers/delete',
  '/api/vdr/searchtimers/delete'
];

for (const route of exactRoutes) {
  context.fetch(route, {
    method: 'POST',
    headers: {
      Accept: 'application/json',
      'X-CSRF-Token': 'caller-must-not-override'
    }
  });

  assert.strictEqual(csrfHeader(), csrfToken);
}

for (const route of [
  '/api/vdr/searchtimers?source=browser',
  '/api/vdr/searchtimers/update?source=browser',
  'https://vdr-suite.test/api/searchtimers/delete?absolute=1'
]) {
  context.fetch(route, {
    method: 'POST',
    headers: {Accept: 'application/json'}
  });

  assert.strictEqual(csrfHeader(), csrfToken);
}

for (const route of [
  '/api/vdr/searchtimers/',
  '/api/vdr/searchtimers/update/',
  '/api/vdr/searchtimers/delete/'
]) {
  context.fetch(route, {
    method: 'POST',
    headers: {Accept: 'application/json'}
  });

  assert.strictEqual(csrfHeader(), undefined);
}

context.fetch(
  '/api/vdr/searchtimers/update',
  {
    method: 'GET',
    headers: {Accept: 'application/json'}
  }
);
assert.strictEqual(csrfHeader(), undefined);

for (const route of [
  '/api/vdr/searchtimers/validate',
  '/api/vdr/searchtimers/plan'
]) {
  context.fetch(route, {
    method: 'POST',
    headers: {Accept: 'application/json'}
  });

  assert.strictEqual(csrfHeader(), undefined);
}

console.log(
  'test_searchtimer_maintenance_security_runtime passed'
);
