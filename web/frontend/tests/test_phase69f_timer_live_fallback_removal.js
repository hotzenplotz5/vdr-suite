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
    headers: { get() { return null; } },
    text() { return Promise.resolve(JSON.stringify(payload)); },
    arrayBuffer() { return Promise.resolve(new ArrayBuffer(0)); }
  };
}

const context = {
  window: null, Headers, URLSearchParams, Promise, Object, Array, String, JSON,
  Error, Number, Boolean, Uint8Array, ArrayBuffer,
  fetch(url, options) {
    requests.push({url: String(url), options});
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

  console.log('test_phase69f_timer_live_fallback_removal passed');
}

run().catch(error => { console.error(error); process.exitCode = 1; });
