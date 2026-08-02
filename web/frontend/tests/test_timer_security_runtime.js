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
const csrfToken = 'timer-csrf-token-for-runtime-test';

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

assert.strictEqual(context.__vdrSuiteTimerMutationCsrfWrapped, true);
assert.strictEqual(typeof listeners.DOMContentLoaded, 'function');

(async function () {
  const timerPaths = [
    '/api/vdr/timers/actions/create',
    '/api/vdr/timers/actions/update',
    '/api/vdr/timers/actions/delete'
  ];

  for (const timerPath of timerPaths) {
    await context.fetch(timerPath, {
      method: 'POST',
      headers: {
        Accept: 'application/json',
        'X-CSRF-Token': 'caller-must-not-override'
      }
    });

    const request = requests[requests.length - 1];
    assert.strictEqual(request.input, timerPath);
    assert.strictEqual(request.init.method, 'POST');
    assert.strictEqual(request.init.headers.Accept, 'application/json');
    assert.strictEqual(request.init.headers['X-CSRF-Token'], csrfToken);
  }

  await context.fetch(
    '/api/vdr/timers/actions/update?source=browser',
    {method: 'POST', headers: {Accept: 'application/json'}}
  );
  assert.strictEqual(
    requests[requests.length - 1].init.headers['X-CSRF-Token'],
    csrfToken
  );

  await context.fetch(
    '/api/vdr/timers/actions/update/',
    {method: 'POST', headers: {Accept: 'application/json'}}
  );
  assert.strictEqual(
    Object.prototype.hasOwnProperty.call(
      requests[requests.length - 1].init.headers,
      'X-CSRF-Token'
    ),
    false
  );

  await context.fetch(
    '/api/vdr/timers/actions/update',
    {method: 'GET', headers: {Accept: 'application/json'}}
  );
  assert.strictEqual(
    Object.prototype.hasOwnProperty.call(
      requests[requests.length - 1].init.headers,
      'X-CSRF-Token'
    ),
    false
  );

  context.VdrSuiteBrowserSession = {
    csrfHeaders() { return {}; }
  };
  await context.fetch(
    '/api/vdr/timers/actions/delete',
    {method: 'POST', headers: {Accept: 'application/json'}}
  );
  assert.strictEqual(
    Object.prototype.hasOwnProperty.call(
      requests[requests.length - 1].init.headers,
      'X-CSRF-Token'
    ),
    false
  );

  console.log('timer security frontend runtime ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
