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
const csrfToken = 'channel-move-csrf-token-for-runtime-test';

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
  context.__vdrSuiteChannelMoveMutationCsrfWrapped,
  true
);
assert.strictEqual(
  context.__vdrSuiteTimerMutationCsrfWrapped,
  true
);
assert.strictEqual(typeof listeners.DOMContentLoaded, 'function');

function latestRequest() {
  return requests[requests.length - 1];
}

function hasCsrfHeader(request) {
  return Object.prototype.hasOwnProperty.call(
    request.init.headers,
    'X-CSRF-Token'
  );
}

(async function () {
  const channelMovePaths = [
    '/api/vdr/channels/move',
    '/api/vdr/channels/actions/move'
  ];

  for (const channelMovePath of channelMovePaths) {
    await context.fetch(channelMovePath, {
      method: 'POST',
      headers: {
        Accept: 'application/json',
        'X-CSRF-Token': 'caller-must-not-override'
      }
    });

    const request = latestRequest();
    assert.strictEqual(request.input, channelMovePath);
    assert.strictEqual(request.init.method, 'POST');
    assert.strictEqual(
      request.init.headers.Accept,
      'application/json'
    );
    assert.strictEqual(
      request.init.headers['X-CSRF-Token'],
      csrfToken
    );
  }

  await context.fetch(
    '/api/vdr/channels/actions/move?source=browser',
    {
      method: 'POST',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(
    latestRequest().init.headers['X-CSRF-Token'],
    csrfToken
  );

  await context.fetch(
    'https://vdr-suite.test/api/vdr/channels/move?source=absolute',
    {
      method: 'POST',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(
    latestRequest().init.headers['X-CSRF-Token'],
    csrfToken
  );

  await context.fetch(
    '/api/vdr/channels/move/',
    {
      method: 'POST',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(hasCsrfHeader(latestRequest()), false);

  await context.fetch(
    '/api/vdr/channels/actions/move/',
    {
      method: 'POST',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(hasCsrfHeader(latestRequest()), false);

  await context.fetch(
    '/api/vdr/channels/move',
    {
      method: 'GET',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(hasCsrfHeader(latestRequest()), false);

  context.VdrSuiteBrowserSession = {
    csrfHeaders() {
      return {};
    }
  };

  await context.fetch(
    '/api/vdr/channels/actions/move',
    {
      method: 'POST',
      headers: {Accept: 'application/json'}
    }
  );
  assert.strictEqual(hasCsrfHeader(latestRequest()), false);

  console.log('channel move security frontend runtime ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
