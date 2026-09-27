'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const clientPath = path.resolve(__dirname, '..', 'api', 'client-api.js');
const requests = [];
let responseFactory = null;

function jsonResponse(status, payload) {
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: {
      get() {
        return null;
      }
    },
    text() {
      return Promise.resolve(
        payload === null || payload === undefined
          ? ''
          : JSON.stringify(payload)
      );
    },
    arrayBuffer() {
      return Promise.resolve(new ArrayBuffer(0));
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
  Uint8Array,
  ArrayBuffer,
  fetch(url, options) {
    requests.push({url, options});
    if (typeof responseFactory !== 'function') {
      throw new Error('responseFactory is not configured');
    }
    return Promise.resolve(responseFactory(url, options));
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
assert.strictEqual(typeof api.requestJson, 'function');
assert.strictEqual(typeof api.requestBinary, 'function');
assert.strictEqual(typeof api.isClientError, 'function');

async function captureFailure(call) {
  try {
    await call();
  } catch (error) {
    return error;
  }
  assert.fail('request unexpectedly succeeded');
}

async function verifyProblemDetailsError() {
  responseFactory = function () {
    return jsonResponse(412, {
      code: 'revision_conflict',
      title: 'Resource revision conflict',
      status: 412,
      detail: 'The resource changed after it was read.',
      instance: '/api/v1/example',
      requestId: 'req-phase69f-1',
      correlationId: 'corr-phase69f-1'
    });
  };

  const error = await captureFailure(function () {
    return api.requestJson('/api/v1/example');
  });

  assert.strictEqual(api.isClientError(error), true);
  assert.strictEqual(error.name, 'VdrSuiteClientError');
  assert.strictEqual(error.path, '/api/v1/example');
  assert.strictEqual(error.status, 412);
  assert.strictEqual(error.code, 'revision_conflict');
  assert.strictEqual(error.requestId, 'req-phase69f-1');
  assert.strictEqual(error.correlationId, 'corr-phase69f-1');
  assert.strictEqual(error.message, 'The resource changed after it was read.');
  assert.strictEqual(error.payload.status, 412);
}

async function verifyLegacyStructuredError() {
  responseFactory = function () {
    return jsonResponse(403, {
      error: {
        code: 'permission_denied',
        message: 'Permission denied',
        requestId: 'req-phase69f-2'
      }
    });
  };

  const error = await captureFailure(function () {
    return api.requestJson('/api/vdr/example');
  });

  assert.strictEqual(api.isClientError(error), true);
  assert.strictEqual(error.status, 403);
  assert.strictEqual(error.code, 'permission_denied');
  assert.strictEqual(error.requestId, 'req-phase69f-2');
  assert.strictEqual(error.correlationId, null);
  assert.strictEqual(error.message, 'Permission denied');
}

async function verifyBinaryErrorUsesSameRepresentation() {
  responseFactory = function () {
    return jsonResponse(503, {
      code: 'service_unavailable',
      detail: 'Presentation backend is unavailable.',
      requestId: 'req-phase69f-3'
    });
  };

  const error = await captureFailure(function () {
    return api.requestBinary('/api/vdr/broadcast/hbbtv/sessions/presentation');
  });

  assert.strictEqual(api.isClientError(error), true);
  assert.strictEqual(error.status, 503);
  assert.strictEqual(error.code, 'service_unavailable');
  assert.strictEqual(error.requestId, 'req-phase69f-3');
  assert.strictEqual(error.message, 'Presentation backend is unavailable.');
}

async function verifyMutationDoesNotFallback(functionName, expectedPath) {
  const before = requests.length;
  responseFactory = function () {
    return jsonResponse(503, {
      error: {
        code: 'backend_unavailable',
        message: 'Backend unavailable',
        requestId: 'req-' + functionName
      }
    });
  };

  const error = await captureFailure(function () {
    return api[functionName]({
      payload: {name: 'phase69f'},
      headers: {'X-Test': 'phase69f'}
    });
  });

  const emitted = requests.slice(before);
  assert.strictEqual(api.isClientError(error), true);
  assert.strictEqual(error.status, 503);
  assert.strictEqual(error.code, 'backend_unavailable');
  assert.strictEqual(emitted.length, 1);
  assert.strictEqual(emitted[0].url, expectedPath);
  assert.strictEqual(emitted[0].options.method, 'POST');
}

async function run() {
  await verifyProblemDetailsError();
  await verifyLegacyStructuredError();
  await verifyBinaryErrorUsesSameRepresentation();

  await verifyMutationDoesNotFallback(
    'fetchClientSearchTimerExecute',
    '/api/vdr/searchtimers/execute'
  );
  await verifyMutationDoesNotFallback(
    'fetchClientSearchTimerCreateAction',
    '/api/vdr/searchtimers'
  );
  await verifyMutationDoesNotFallback(
    'fetchClientSearchTimerUpdateAction',
    '/api/vdr/searchtimers/update'
  );
  await verifyMutationDoesNotFallback(
    'fetchClientSearchTimerDeleteAction',
    '/api/vdr/searchtimers/delete'
  );

  console.log('test_phase69f_client_error_mutation_fallback passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
