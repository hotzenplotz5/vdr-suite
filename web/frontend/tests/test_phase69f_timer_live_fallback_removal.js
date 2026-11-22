'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const clientPath = path.resolve(__dirname, '..', 'api', 'client-api.js');
const source = fs.readFileSync(clientPath, 'utf8');
const requests = [];

function response(status, payload, etag) {
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: { get(name) { return name === 'ETag' ? (etag || null) : null; } },
    text() { return Promise.resolve(status === 304 ? '' : JSON.stringify(payload)); },
    arrayBuffer() { return Promise.resolve(new ArrayBuffer(0)); }
  };
}

const context = {
  window: null, Headers, URL, URLSearchParams, Set, Promise, Object, Array, String, JSON,
  location: {origin: 'https://suite.example'},
  Error, Number, Boolean, Uint8Array, ArrayBuffer,
  fetch(url, options) {
    const target = String(url);
    requests.push({url: target, options});
    if (target.startsWith('https://suite.example/api/v1/native-timers')) {
      if (target.includes('backendId=offline')) return Promise.resolve(response(503, {
        error: {code: 'backend_unavailable', message: 'Native timer unavailable'}
      }));
      if (target.includes('/timer:abc-12?')) {
        const etag = '"native-revision-3"';
        const cached = options.headers['If-None-Match'] === etag;
        return Promise.resolve(response(cached ? 304 : 200, {
          backendId: 'living-room', timerId: 'timer:abc-12',
          title: 'Tagesschau', directory: 'News~Tagesschau',
          priority: 68, lifetime: 19, vps: true,
          ownership: 'managed', bindingState: 'verified'
        }, etag));
      }
      return Promise.resolve(response(200, {
        backendId: 'living-room',
        items: [{timerId: 'timer:abc-12', title: 'Tagesschau',
          channelId: 'C-1-2-3', startTime: '2015', endTime: '2030',
          directory: 'News~Tagesschau', priority: 68, lifetime: 19,
          vps: true, enabled: true}],
        page: {offset: 0, limit: 30, hasMore: false, nextOffset: null},
        meta: {partial: false}
      }));
    }
    return Promise.resolve(response(503, {
      error: {
        code: 'backend_unavailable',
        message: 'Live VDR timer read unavailable',
        requestId: 'req-phase69f-timer-live'
      }
    }));
  }
};
context.window = context;

vm.createContext(context);
const referencePath = path.resolve(__dirname, '..', '..', '..',
  'clients', 'reference-js', 'public-v1-client.js');
vm.runInContext(fs.readFileSync(referencePath, 'utf8'), context,
  {filename: referencePath});
vm.runInContext(source, context, {filename: clientPath});
const api = context.VdrSuiteClientApi;
assert.ok(api);

async function captureFailure(call) {
  try { await call(); } catch (error) { return error; }
  assert.fail('request unexpectedly succeeded');
}

async function run() {
  const start = source.indexOf('function fetchClientTimers(options)');
  const end = source.indexOf('function fetchClientTimerConflicts(options)', start);
  assert.ok(start >= 0 && end > start, 'cannot bound fetchClientTimers');
  const body = source.slice(start, end);

  assert.ok(body.includes("requestJson('/api/vdr/timers/live', options)"));
  assert.ok(!body.includes("'/api/vdr/timers'"));
  assert.ok(!body.includes('requestJsonWithFallback'));
  assert.ok(!source.includes('function requestJsonWithFallback(path, fallbackPath, options)'));

  const failure = await captureFailure(() => api.fetchClientTimers({cache: 'no-store'}));
  assert.strictEqual(api.isClientError(failure), true);
  assert.strictEqual(failure.status, 503);
  assert.strictEqual(failure.code, 'backend_unavailable');
  assert.strictEqual(failure.requestId, 'req-phase69f-timer-live');
  assert.strictEqual(requests.length, 1);
  assert.strictEqual(requests[0].url, '/api/vdr/timers/live');
  assert.strictEqual(requests[0].options.method, 'GET');
  assert.strictEqual(requests[0].options.cache, 'no-store');

  // F5.3: explicit, read-only Public-v1 access must not reinterpret the
  // public projection as the existing full Legacy CRUD edit contract.
  const page = await api.fetchPublicNativeTimerPage({
    backendId: 'living-room', offset: 0, limit: 30
  });
  assert.strictEqual(page.items[0].timerId, 'timer:abc-12');
  assert.strictEqual(page.meta.partial, false);
  assert.strictEqual(requests[1].url,
    'https://suite.example/api/v1/native-timers?backendId=living-room&offset=0&limit=30');
  assert.strictEqual(requests[1].options.method, 'GET');
  assert.strictEqual(requests[1].options.credentials, 'same-origin');
  assert.strictEqual(requests[1].options.headers['X-CSRF-Token'], undefined);
  for (const [field, value] of Object.entries({
    directory: 'News~Tagesschau', priority: 68, lifetime: 19, vps: true
  })) {
    assert.strictEqual(page.items[0][field], value);
  }
  assert.strictEqual(Object.prototype.hasOwnProperty.call(page.items[0], 'aux'), false,
    'opaque provider aux is outside the Public-v1 revision contract');
  const detail = await api.fetchPublicNativeTimerDetail({
    backendId: 'living-room', timerId: 'timer:abc-12'
  });
  assert.strictEqual(detail.status, 200);
  assert.strictEqual(detail.etag, '"native-revision-3"');
  assert.strictEqual(detail.data.bindingState, 'verified');
  assert.strictEqual(detail.data.directory, page.items[0].directory);
  assert.strictEqual(detail.data.priority, page.items[0].priority);
  assert.strictEqual(detail.data.lifetime, page.items[0].lifetime);
  assert.strictEqual(detail.data.vps, page.items[0].vps);
  assert.strictEqual(Object.prototype.hasOwnProperty.call(detail.data, 'aux'), false);
  const cached = await api.fetchPublicNativeTimerDetail({
    backendId: 'living-room', timerId: 'timer:abc-12', ifNoneMatch: detail.etag
  });
  assert.strictEqual(cached.status, 304);
  assert.strictEqual(cached.data, null);
  assert.strictEqual(requests[3].options.headers['If-None-Match'], detail.etag);
  const publicFailure = await captureFailure(() => api.fetchPublicNativeTimerPage({
    backendId: 'offline', offset: 0, limit: 30
  }));
  assert.strictEqual(publicFailure.status, 503);
  assert.strictEqual(requests.length, 5, 'Public-v1 503 is not followed by a legacy retry');
  assert.strictEqual(requests[4].url.includes('backendId=offline'), true);
  assert.strictEqual(requests.filter(r => r.url === '/api/vdr/timers/live').length, 1);
  assert.throws(() => api.fetchPublicNativeTimerDetail({
    backendId: 'living-room', timerId: '../private'
  }));
  assert.strictEqual(requests.length, 5);

  console.log('test_phase69f_timer_live_fallback_removal and F5.3 public reads passed');
}

run().catch(error => { console.error(error); process.exitCode = 1; });
