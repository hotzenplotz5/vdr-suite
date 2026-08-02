'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const clientPath = path.resolve(
  __dirname,
  '..',
  'api',
  'client-api.js'
);
const epgActionsPath = path.resolve(
  __dirname,
  '..',
  'epg-searchtimer-actions.js'
);
const loaderPath = path.resolve(
  __dirname,
  '..',
  'platform',
  'deferred-runtime-loader.js'
);

const requests = [];
const activeCsrf = 'slice-2p-active-session-csrf';
let previewPrimaryFails = false;

function response(ok, status, payload) {
  return {
    ok,
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
  VdrSuiteBrowserSession: {
    csrfHeaders() {
      return {'X-CSRF-Token': activeCsrf};
    }
  },
  fetch(url, options) {
    requests.push({url, options});
    if (previewPrimaryFails &&
        String(url).startsWith('/api/vdr/searchtimers/preview/cache/refresh')) {
      return Promise.resolve(response(false, 404, {error: 'missing'}));
    }
    return Promise.resolve(response(true, 200, {
      backendId: 'living-room',
      status: 'ready',
      available: true,
      eventCount: 7
    }));
  }
};
context.window = context;

vm.createContext(context);
vm.runInContext(
  fs.readFileSync(clientPath, 'utf8'),
  context,
  {filename: clientPath}
);

const api = context.VdrSuiteClientApi;
assert.ok(api);
assert.strictEqual(
  typeof api.fetchClientSearchTimerPreviewCacheRefresh,
  'function'
);
assert.strictEqual(typeof api.fetchClientEpgCacheRefresh, 'function');

function latestRequest() {
  return requests[requests.length - 1];
}

function assertScopedMutation(request, pathName) {
  const parsed = new URL(request.url, 'https://vdr-suite.test/');
  assert.strictEqual(parsed.pathname, pathName);
  assert.strictEqual(parsed.searchParams.get('backend'), 'living-room');
  assert.strictEqual(request.options.method, 'POST');
  assert.strictEqual(
    request.options.headers['X-CSRF-Token'],
    activeCsrf
  );
  assert.strictEqual(request.options.headers['X-Caller'], 'preserved');
}

async function run() {
  await api.fetchClientEpgCacheRefresh({
    backendId: 'living-room',
    query: {from: -1, timespan: 3600},
    headers: {
      'X-CSRF-Token': 'caller-must-not-override',
      'X-Caller': 'preserved'
    },
    cache: 'no-store',
    credentials: 'same-origin'
  });
  assertScopedMutation(latestRequest(), '/api/epg/cache/refresh');

  await api.fetchClientSearchTimerPreviewCacheRefresh({
    backendId: 'living-room',
    query: {
      from: -1,
      timespan: 1209600,
      limit: 0,
      channelEventLimit: 96
    },
    headers: {
      'X-CSRF-Token': 'caller-must-not-override',
      'X-Caller': 'preserved'
    },
    cache: 'no-store',
    credentials: 'same-origin'
  });
  assertScopedMutation(
    latestRequest(),
    '/api/vdr/searchtimers/preview/cache/refresh'
  );

  previewPrimaryFails = true;
  const beforeFallback = requests.length;
  await api.fetchClientSearchTimerPreviewCacheRefresh({
    backendId: 'living-room',
    headers: {
      'X-CSRF-Token': 'caller-must-not-override',
      'X-Caller': 'preserved'
    }
  });
  const fallbackRequests = requests.slice(beforeFallback);
  assert.strictEqual(fallbackRequests.length, 2);
  assertScopedMutation(
    fallbackRequests[0],
    '/api/vdr/searchtimers/preview/cache/refresh'
  );
  assertScopedMutation(
    fallbackRequests[1],
    '/api/searchtimers/preview/cache/refresh'
  );

  const epgActionsSource = fs.readFileSync(epgActionsPath, 'utf8');
  assert.ok(epgActionsSource.includes(
    'fetchClientSearchTimerPreviewCacheRefresh'
  ));
  assert.ok(!epgActionsSource.includes(
    "client.requestJson('/api/vdr/searchtimers/preview/cache/refresh'"
  ));
  assert.ok(!epgActionsSource.includes(
    "client.requestJson('/api/searchtimers/preview/cache/refresh'"
  ));

  const loaderSource = fs.readFileSync(loaderPath, 'utf8');
  assert.ok(loaderSource.includes(
    'fetchClientSearchTimerPreviewCacheRefresh'
  ));
  assert.ok(!loaderSource.includes(
    "'/api/vdr/searchtimers/preview/cache/refresh'"
  ));
  assert.ok(!loaderSource.includes(
    "'/api/searchtimers/preview/cache/refresh'"
  ));

  console.log('test_query_cache_refresh_security_runtime passed');
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
