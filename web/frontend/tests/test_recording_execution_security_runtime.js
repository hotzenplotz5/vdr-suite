'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'platform', 'deferred-runtime-loader.js'),
  'utf8'
);

const requests = [];
const listeners = {};
const csrfToken = 'recording-execution-csrf-runtime-test';

const document = {
  readyState: 'loading',
  documentElement: {lang: 'de'},
  head: {appendChild() {}},
  getElementById() { return null; },
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
  location: {href: 'https://vdr-suite.test/'},
  VdrSuiteBrowserSession: {
    csrfHeaders() {
      return {'X-CSRF-Token': csrfToken};
    }
  },
  fetch(input, init) {
    requests.push({input, init});
    return Promise.resolve({
      status: 200,
      ok: true,
      clone() { return this; },
      text() { return Promise.resolve('{}'); }
    });
  }
};

context.window = context;

vm.createContext(context);
vm.runInContext(source, context);

assert.strictEqual(
  context.__vdrSuiteRecordingExecutionMutationCsrfWrapped,
  true
);
assert.strictEqual(typeof listeners.DOMContentLoaded, 'function');

function latestRequest() {
  return requests[requests.length - 1];
}

function csrfHeader(request) {
  return request &&
    request.init &&
    request.init.headers &&
    request.init.headers['X-CSRF-Token'];
}

(async function () {
  const paths = [
    '/api/recordings/actions/execute',
    '/api/vdr/recordings/actions/execute'
  ];

  for (const route of paths) {
    await context.fetch(route, {
      method: 'POST',
      headers: {
        Accept: 'application/json',
        'X-CSRF-Token': 'caller-must-not-override'
      }
    });

    assert.strictEqual(csrfHeader(latestRequest()), csrfToken);
  }

  await context.fetch(
    '/api/vdr/recordings/actions/execute?source=browser',
    {
      method: 'POST',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(csrfHeader(latestRequest()), csrfToken);

  await context.fetch(
    'https://vdr-suite.test/api/recordings/actions/execute?absolute=1',
    {
      method: 'POST',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(csrfHeader(latestRequest()), csrfToken);

  await context.fetch(
    '/api/vdr/recordings/actions/execute/',
    {
      method: 'POST',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(csrfHeader(latestRequest()), undefined);

  await context.fetch(
    '/api/vdr/recordings/actions/execute',
    {
      method: 'GET',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(csrfHeader(latestRequest()), undefined);

  await context.fetch(
    '/api/vdr/recordings/actions/validate',
    {
      method: 'POST',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(csrfHeader(latestRequest()), undefined);

  context.VdrSuiteBrowserSession = {
    csrfHeaders() {
      return {};
    }
  };

  await context.fetch(
    '/api/vdr/recordings/actions/execute',
    {
      method: 'POST',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(csrfHeader(latestRequest()), undefined);

  console.log('recording execution security frontend runtime ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
