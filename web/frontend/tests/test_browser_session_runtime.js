'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const {TextEncoder} = require('util');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'live-remote-client-api.js'),
  'utf8'
);
const baseClientSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'client-api.js'),
  'utf8'
);

assert(source.includes("const LOGIN_PATH = '/api/security/browser-sessions'"));
assert(source.includes("const LOGOUT_PATH = '/api/security/browser-sessions/logout'"));
assert(source.includes("const CSRF_HEADER = 'X-CSRF-Token'"));
assert(source.includes('sessionApi.csrfHeaders()'));
assert(!source.includes('localStorage'));
assert(!source.includes('sessionStorage'));
assert(!source.includes('indexedDB'));
assert(!source.includes('document.cookie'));
assert(source.includes("global.addEventListener('pagehide'"));
assert(source.includes("clear('authentication_required')"));
assert(source.includes("button.id = 'vdr-suite-session-button'"));
assert(source.includes("password.type = 'password'"));

function response(status, payload) {
  const text = payload === null || payload === undefined
    ? ''
    : JSON.stringify(payload);

  function build() {
    return {
      status,
      ok: status >= 200 && status < 300,
      text() {
        return Promise.resolve(text);
      },
      clone() {
        return build();
      }
    };
  }

  return build();
}

const fetchQueue = [];
const fetchRequests = [];
const apiRequests = [];
const globalListeners = {};

const document = {
  readyState: 'complete',
  documentElement: {lang: 'de'},
  head: {appendChild() {}},
  body: {appendChild() {}},
  querySelector() { return null; },
  getElementById() { return null; },
  createElement() {
    return {
      className: '',
      classList: {toggle() {}, add() {}, remove() {}},
      setAttribute() {},
      append() {},
      appendChild() {},
      addEventListener() {},
      removeAttribute() {},
      focus() {},
      style: {},
      textContent: '',
      value: ''
    };
  }
};

const baseApi = Object.freeze({
  requestJson(url, options) {
    apiRequests.push({url, options});
    return Promise.resolve({success: true});
  }
});

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
  TextEncoder,
  setTimeout,
  clearTimeout,
  btoa(value) {
    return Buffer.from(value, 'binary').toString('base64');
  },
  addEventListener(type, listener) {
    globalListeners[type] = listener;
  },
  fetch(input, init) {
    fetchRequests.push({input, init});
    if (!fetchQueue.length) {
      return Promise.reject(new Error('No mocked fetch response queued'));
    }
    return Promise.resolve(fetchQueue.shift());
  },
  VdrSuiteClientApi: baseApi
};
context.window = context;

vm.createContext(context);
vm.runInContext(source, context);

assert(context.VdrSuiteBrowserSession);
assert.strictEqual(context.VdrSuiteBrowserSession.isAuthenticated(), false);
assert.deepStrictEqual(
  JSON.parse(JSON.stringify(context.VdrSuiteBrowserSession.snapshot())),
  {authenticated: false, expiresAt: '', reason: ''}
);
assert.deepStrictEqual(
  JSON.parse(JSON.stringify(context.VdrSuiteBrowserSession.csrfHeaders())),
  {}
);

(async function () {
  await assert.rejects(
    context.VdrSuiteClientApi.fetchClientRemoteAction({
      payload: {
        backendId: 'default',
        operationId: 'slice-2e-anonymous-remote',
        action: 'ok'
      }
    }),
    function (error) {
      return error.message.includes('Bitte anmelden');
    }
  );
  assert.strictEqual(apiRequests.length, 0);

  const firstToken = 'c'.repeat(48);
  fetchQueue.push(response(200, {
    csrfToken: firstToken,
    expiresAt: '2099-01-01T00:00:00Z',
    requestId: 'req-login'
  }));

  const signedIn = await context.VdrSuiteBrowserSession.login(
    'vdr-suite',
    'secret-password'
  );

  assert.strictEqual(signedIn.authenticated, true);
  assert.strictEqual(
    Object.prototype.hasOwnProperty.call(signedIn, 'csrfToken'),
    false
  );
  assert.strictEqual(fetchRequests[0].input, '/api/security/browser-sessions');
  assert.strictEqual(fetchRequests[0].init.method, 'POST');
  assert.strictEqual(fetchRequests[0].init.cache, 'no-store');
  assert.strictEqual(fetchRequests[0].init.credentials, 'same-origin');
  assert.strictEqual(
    Buffer.from(
      fetchRequests[0].init.headers.Authorization.slice('Basic '.length),
      'base64'
    ).toString('utf8'),
    'vdr-suite:secret-password'
  );
  assert.deepStrictEqual(
    JSON.parse(JSON.stringify(context.VdrSuiteBrowserSession.csrfHeaders())),
    {'X-CSRF-Token': firstToken}
  );

  await context.VdrSuiteClientApi.fetchClientRemoteAction({
    payload: {
      backendId: 'default',
      operationId: 'slice-2e-remote',
      action: 'ok'
    },
    headers: {'X-CSRF-Token': 'caller-must-not-override'}
  });

  assert.strictEqual(apiRequests.length, 1);
  assert.strictEqual(apiRequests[0].url, '/api/vdr/remote/actions');
  assert.strictEqual(apiRequests[0].options.method, 'POST');
  assert.strictEqual(
    apiRequests[0].options.headers['X-CSRF-Token'],
    firstToken
  );

  fetchQueue.push(response(204, null));
  await context.VdrSuiteBrowserSession.logout();

  assert.strictEqual(fetchRequests[1].input, '/api/security/browser-sessions/logout');
  assert.strictEqual(fetchRequests[1].init.headers['X-CSRF-Token'], firstToken);
  assert.strictEqual(context.VdrSuiteBrowserSession.isAuthenticated(), false);
  assert.deepStrictEqual(
    JSON.parse(JSON.stringify(context.VdrSuiteBrowserSession.csrfHeaders())),
    {}
  );

  await assert.rejects(
    context.VdrSuiteClientApi.fetchClientRemoteAction({
      payload: {
        backendId: 'default',
        operationId: 'slice-2e-logged-out-remote',
        action: 'ok'
      }
    }),
    function (error) {
      return error.message.includes('Bitte anmelden');
    }
  );
  assert.strictEqual(apiRequests.length, 1);

  const structuredErrorContext = {
    window: null,
    URLSearchParams,
    fetch() {
      return Promise.resolve(response(403, {
        error: {
          code: 'permission_denied',
          message: 'Remote permission denied'
        }
      }));
    }
  };
  structuredErrorContext.window = structuredErrorContext;
  vm.createContext(structuredErrorContext);
  vm.runInContext(baseClientSource, structuredErrorContext);

  await assert.rejects(
    structuredErrorContext.VdrSuiteClientApi.requestJson('/api/test'),
    function (error) {
      return error.message === 'Remote permission denied';
    }
  );

  const secondToken = 'd'.repeat(48);
  fetchQueue.push(response(200, {
    csrfToken: secondToken,
    expiresAt: '2099-01-01T00:00:00Z'
  }));
  await context.VdrSuiteBrowserSession.login('vdr-suite', 'another-password');
  assert.strictEqual(context.VdrSuiteBrowserSession.isAuthenticated(), true);

  fetchQueue.push(response(401, {
    error: {
      code: 'credential_revoked',
      message: 'The authenticated identity is no longer active'
    }
  }));
  const denied = await context.fetch('/api/vdr/status', {
    credentials: 'same-origin'
  });
  assert.strictEqual(denied.status, 401);
  assert.strictEqual(context.VdrSuiteBrowserSession.isAuthenticated(), false);
  assert.strictEqual(
    context.VdrSuiteBrowserSession.snapshot().reason,
    'credential_revoked'
  );
  assert(
    context.VdrSuiteBrowserSession.lastSecurityMessage().includes('widerrufen')
  );

  const thirdToken = 'e'.repeat(48);
  fetchQueue.push(response(200, {
    csrfToken: thirdToken,
    expiresAt: '2099-01-01T00:00:00Z'
  }));
  await context.VdrSuiteBrowserSession.login(
    'vdr-suite',
    'pagehide-password'
  );
  assert.strictEqual(context.VdrSuiteBrowserSession.isAuthenticated(), true);
  assert.strictEqual(typeof globalListeners.pagehide, 'function');

  globalListeners.pagehide();

  assert.strictEqual(context.VdrSuiteBrowserSession.isAuthenticated(), false);
  assert.strictEqual(
    context.VdrSuiteBrowserSession.snapshot().reason,
    'authentication_required'
  );

  await assert.rejects(
    context.VdrSuiteClientApi.fetchClientRemoteAction({
      payload: {
        backendId: 'default',
        operationId: 'slice-2e-pagehide-remote',
        action: 'ok'
      }
    }),
    function (error) {
      return error.message.includes('Bitte anmelden');
    }
  );
  assert.strictEqual(apiRequests.length, 1);

  console.log('browser session frontend runtime ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
