'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const clientPath = path.resolve(__dirname, '..', 'api', 'client-api.js');
const source = fs.readFileSync(clientPath, 'utf8');
const requests = [];

function response(status, payload, etag, location) {
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: { get(name) { return name === 'ETag' ? (etag || null) :
      name === 'Location' ? (location || null) : null; } },
    text() { return Promise.resolve(status === 304 ? '' : JSON.stringify(payload)); },
    arrayBuffer() { return Promise.resolve(new ArrayBuffer(0)); }
  };
}

const context = {
  window: null, Headers, URL, URLSearchParams, TextEncoder, Set, Promise, Object, Array, String, JSON,
  location: {origin: 'https://suite.example'},
  Error, Number, Boolean, Uint8Array, ArrayBuffer,
  fetch(url, options) {
    const target = String(url);
    requests.push({url: target, options});
    if (target.startsWith('https://suite.example/api/v1/operations/')) {
      const etag = '"operation-revision-1"';
      return Promise.resolve(response(options.headers['If-None-Match'] === etag ? 304 : 200,
        {operationId: 'op_modify_f55', state: 'succeeded'}, etag));
    }
    if (target === 'https://suite.example/api/v1/manual/timer-intents') {
      if (JSON.parse(options.body).title === 'Fail')
        return Promise.resolve(response(503, {error: {code: 'backend_unavailable'}}));
      return Promise.resolve(response(202, {
        operationId: 'op_manual_f55', state: 'accepted', backendId: 'living-room',
        links: {self: '/api/v1/operations/op_manual_f55'}
      }, null, '/api/v1/operations/op_manual_f55'));
    }
    if (target.startsWith('https://suite.example/api/v1/native-timers')) {
      if (options.method === 'POST' || options.method === 'DELETE') {
        if (options.headers['Idempotency-Key'] === 'idem-conflict')
          return Promise.resolve(response(409, {error: {code: 'operation_conflict'}}));
        const id = options.method === 'POST' ? 'op_modify_f55' : 'op_delete_f55';
        return Promise.resolve(response(202, {operation: {operationId: id, state: 'accepted'}},
          null, '/api/v1/operations/' + id));
      }
      if (target.includes('backendId=offline')) return Promise.resolve(response(503, {
        error: {code: 'backend_unavailable', message: 'Native timer unavailable'}
      }));
      if (target.includes('/timer:abc-12?')) {
        const etag = '"native-revision-3"';
        const cached = options.headers['If-None-Match'] === etag;
        return Promise.resolve(response(cached ? 304 : 200, {
          backendId: 'living-room', timerId: 'timer:abc-12',
          title: 'Tagesschau', directory: 'News~Tagesschau',
          eventId: '117721', flags: 5, priority: 68, lifetime: 19, vps: true,
          ownership: 'managed', bindingState: 'verified'
        }, etag));
      }
      return Promise.resolve(response(200, {
        backendId: 'living-room',
        items: [{timerId: 'timer:abc-12', title: 'Tagesschau',
          channelId: 'C-1-2-3', startTime: '2015', endTime: '2030',
          directory: 'News~Tagesschau', eventId: '117721', flags: 5,
          priority: 68, lifetime: 19, vps: true, enabled: true}],
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
context.VdrSuiteBrowserSession = {
  csrfHeaders() { return {'X-CSRF-Token': 'browser-fixture-csrf'}; }
};

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
    directory: 'News~Tagesschau', eventId: '117721', flags: 5,
    priority: 68, lifetime: 19, vps: true
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
  assert.strictEqual(detail.data.eventId, page.items[0].eventId);
  assert.strictEqual(detail.data.flags, page.items[0].flags);
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

  // F5.5: explicit browser-admin Public-v1 write adapters retain the server
  // operation contract, inject existing session CSRF, and never replace CRUD.
  const currentTag = detail.etag;
  const targetSpec = {
    kind: 'update', channelId: 'C-1-2-3', title: 'Tagesschau',
    directory: 'News~Tagesschau', day: '2026-11-24', weekdays: '-------',
    startTime: '2015', endTime: '2030', priority: 0, lifetime: 0,
    enabled: true, vps: true
  };
  const modified = await api.fetchPublicNativeTimerModify({
    backendId: 'living-room', timerId: 'timer:abc-12', ifMatch: currentTag,
    idempotencyKey: 'idem-modify', payload: targetSpec
  });
  assert.strictEqual(modified.status, 202);
  assert.strictEqual(modified.data.operation.operationId, 'op_modify_f55');
  assert.strictEqual(modified.location, '/api/v1/operations/op_modify_f55');
  assert.strictEqual(requests.length, 6);
  assert.strictEqual(requests[5].options.method, 'POST');
  assert.strictEqual(requests[5].url,
    'https://suite.example/api/v1/native-timers/timer:abc-12/modifications?backendId=living-room');
  assert.deepStrictEqual(JSON.parse(requests[5].options.body), targetSpec);
  assert.strictEqual(requests[5].options.headers['If-Match'], currentTag);
  assert.strictEqual(requests[5].options.headers['Idempotency-Key'], 'idem-modify');
  assert.strictEqual(requests[5].options.headers['X-CSRF-Token'], 'browser-fixture-csrf');
  assert.strictEqual(requests[5].options.credentials, 'same-origin');
  const removed = await api.fetchPublicNativeTimerDelete({
    backendId: 'living-room', timerId: 'timer:abc-12', ifMatch: currentTag,
    idempotencyKey: 'idem-delete'
  });
  assert.strictEqual(removed.status, 202);
  assert.strictEqual(removed.data.operation.operationId, 'op_delete_f55');
  assert.strictEqual(requests.length, 7);
  assert.strictEqual(requests[6].options.method, 'DELETE');
  assert.strictEqual(requests[6].options.body, undefined);
  assert.strictEqual(requests[6].options.headers['X-CSRF-Token'], 'browser-fixture-csrf');
  const conflict = await captureFailure(() => api.fetchPublicNativeTimerModify({
    backendId: 'living-room', timerId: 'timer:abc-12', ifMatch: currentTag,
    idempotencyKey: 'idem-conflict', payload: targetSpec
  }));
  assert.strictEqual(conflict.status, 409);
  assert.strictEqual(requests.length, 8, '409 must not trigger a legacy retry');
  assert.throws(() => api.fetchPublicNativeTimerDelete({
    backendId: 'living-room', timerId: '../other', ifMatch: currentTag,
    idempotencyKey: 'idem-invalid'
  }));
  assert.strictEqual(requests.length, 8, 'invalid IDs must not dispatch');
  assert.strictEqual(requests.filter(r => r.url === '/api/vdr/timers/live').length, 1);
  const operation = await api.fetchPublicNativeTimerOperation({
    operationId: 'op_modify_f55'
  });
  assert.strictEqual(operation.status, 200);
  assert.strictEqual(operation.data.operationId, 'op_modify_f55');
  assert.strictEqual(operation.data.state, 'succeeded');
  assert.strictEqual(operation.etag, '"operation-revision-1"');
  assert.strictEqual(requests.length, 9);
  assert.strictEqual(requests[8].url,
    'https://suite.example/api/v1/operations/op_modify_f55');
  assert.strictEqual(requests[8].options.method, 'GET');
  assert.strictEqual(requests[8].options.credentials, 'same-origin');
  const unchanged = await api.fetchPublicNativeTimerOperation({
    operationId: 'op_modify_f55', ifNoneMatch: operation.etag
  });
  assert.strictEqual(unchanged.status, 304);
  assert.strictEqual(unchanged.data, null);
  assert.strictEqual(requests[9].options.headers['If-None-Match'], operation.etag);
  assert.throws(() => api.fetchPublicNativeTimerOperation({operationId: '../invalid'}));
  assert.strictEqual(requests.length, 10, 'invalid operation ID must not dispatch');
  assert.strictEqual(requests.filter(r => r.url === '/api/vdr/timers/live').length, 1);
  // F5.6 manual browser adapter: no EPG If-Match and no Legacy CREATE.
  const manualPayload = {
    backendId: 'living-room', channelId: 'C-1-2-3',
    title: 'Evening news', directory: 'News', startAt: 1795550400,
    stopAt: 1795554000, priority: 50, lifetime: 99, enabled: true, vps: false,
    aux: '<epgsearch>plugin:data</epgsearch>'
  };
  const manual = await api.fetchPublicManualTimerCreate({
    payload: manualPayload, idempotencyKey: 'manual-browser-once-1'
  });
  assert.strictEqual(manual.status, 202);
  assert.strictEqual(manual.location, '/api/v1/operations/op_manual_f55');
  assert.strictEqual(manual.data.operationId, 'op_manual_f55');
  assert.strictEqual(manual.data.state, 'accepted');
  assert.strictEqual(manual.data.backendId, 'living-room');
  assert.strictEqual(manual.data.links.self, manual.location);
  assert.strictEqual(manual.data.operation, undefined);
  assert.strictEqual(requests[10].url,
    'https://suite.example/api/v1/manual/timer-intents');
  assert.strictEqual(requests[10].options.method, 'POST');
  assert.strictEqual(requests[10].options.credentials, 'same-origin');
  assert.strictEqual(requests[10].options.headers['X-CSRF-Token'], 'browser-fixture-csrf');
  assert.strictEqual(requests[10].options.headers['Idempotency-Key'], 'manual-browser-once-1');
  assert.strictEqual(requests[10].options.headers['If-Match'], undefined);
  assert.deepStrictEqual(JSON.parse(requests[10].options.body), manualPayload);
  const failedManual = await captureFailure(() => api.fetchPublicManualTimerCreate({
    payload: {...manualPayload, title: 'Fail'},
    idempotencyKey: 'manual-browser-once-2'
  }));
  assert.strictEqual(failedManual.status, 503);
  assert.strictEqual(requests.length, 12, '503 must not trigger a second write');
  assert.strictEqual(requests.filter(r =>
    r.url === '/api/vdr/timers/actions/create').length, 0);
  assert.throws(() => api.fetchPublicManualTimerCreate({
    payload: {...manualPayload, aux: '<vdr-suite-managed-timer-spoof/>'}, idempotencyKey: 'manual-browser-invalid'
  }));
  assert.strictEqual(requests.length, 12, 'invalid fields must not dispatch');

  console.log('test_phase69f_timer_live_fallback_removal F5.3/F5.5/F5.6 passed');
}

run().catch(error => { console.error(error); process.exitCode = 1; });
