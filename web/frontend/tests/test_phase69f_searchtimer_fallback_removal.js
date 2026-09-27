'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const clientPath = path.resolve(__dirname, '..', 'api', 'client-api.js');
const source = fs.readFileSync(clientPath, 'utf8');
const requests = [];

function response(status, payload) {
  return {
    ok: status >= 200 && status < 300,
    status,
    text() {
      return Promise.resolve(JSON.stringify(payload));
    }
  };
}

const context = {
  window: null,
  Headers,
  URLSearchParams,
  Promise,
  Object,
  Array,
  String,
  JSON,
  Error,
  Number,
  Boolean,
  fetch(url, options) {
    requests.push({url: String(url), options});
    return Promise.resolve(response(503, {
      error: {
        code: 'backend_unavailable',
        message: 'Backend unavailable',
        requestId: 'req-phase69f-searchtimer'
      }
    }));
  }
};
context.window = context;

vm.createContext(context);
vm.runInContext(source, context, {filename: clientPath});

const api = context.VdrSuiteClientApi;
assert.ok(api);

async function captureFailure(call) {
  try {
    await call();
  } catch (error) {
    return error;
  }
  assert.fail('SearchTimer request unexpectedly succeeded');
}

async function verifyOneShot(functionName, expectedPath, expectedMethod, options) {
  const before = requests.length;
  const failure = await captureFailure(function () {
    return api[functionName](options);
  });
  const emitted = requests.slice(before);

  assert.strictEqual(api.isClientError(failure), true);
  assert.strictEqual(failure.status, 503);
  assert.strictEqual(failure.code, 'backend_unavailable');
  assert.strictEqual(failure.requestId, 'req-phase69f-searchtimer');
  assert.strictEqual(emitted.length, 1);

  const parsed = new URL(emitted[0].url, 'https://vdr-suite.test/');
  assert.strictEqual(parsed.pathname, expectedPath);
  assert.strictEqual(emitted[0].options.method, expectedMethod);

  if (options && options.backendId) {
    assert.strictEqual(parsed.searchParams.get('backend'), options.backendId);
  }
}

async function run() {
  assert.ok(!source.includes('function requestJsonWithFallbacks(paths, options)'));
  assert.ok(!source.includes('/api/vdr/searchtimers/live'));

  await verifyOneShot(
    'fetchClientSearchTimers',
    '/api/vdr/searchtimers',
    'GET'
  );
  await verifyOneShot(
    'fetchClientSearchTimerDiscovery',
    '/api/vdr/searchtimers/discovery',
    'GET',
    {backendId: 'living-room'}
  );
  await verifyOneShot(
    'fetchClientSearchTimerPreview',
    '/api/vdr/searchtimers/preview',
    'GET',
    {backendId: 'living-room'}
  );
  await verifyOneShot(
    'fetchClientSearchTimerPreviewCacheRefresh',
    '/api/vdr/searchtimers/preview/cache/refresh',
    'POST',
    {backendId: 'living-room'}
  );
  await verifyOneShot(
    'fetchClientSearchTimerPlan',
    '/api/vdr/searchtimers/plan',
    'POST',
    {payload: {query: 'phase69f'}}
  );
  await verifyOneShot(
    'fetchClientSearchTimerValidate',
    '/api/vdr/searchtimers/validate',
    'POST',
    {payload: {query: 'phase69f'}}
  );
  await verifyOneShot(
    'fetchClientSearchTimerRealTest',
    '/api/vdr/searchtimers/real-test',
    'POST',
    {payload: {query: 'phase69f'}}
  );

  console.log('test_phase69f_searchtimer_fallback_removal passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
