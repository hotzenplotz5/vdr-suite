'use strict';

const assert = require('assert');
const path = require('path');

const api = require(path.resolve(__dirname, '..', 'public-v1-client.js'));

function response(status, payload, headers) {
  const normalizedHeaders = {};
  Object.keys(headers || {}).forEach(function (name) {
    normalizedHeaders[name.toLowerCase()] = headers[name];
  });
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: {
      get(name) {
        const value = normalizedHeaders[String(name).toLowerCase()];
        return value === undefined ? null : value;
      }
    },
    text() {
      return Promise.resolve(payload === null ? '' : JSON.stringify(payload));
    }
  };
}

async function run() {
  const requests = [];
  let responseFactory = () => response(202, null);

  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    headers: {Authorization: 'Bearer timer-create-token'},
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  const mutationOptions = {
    timerAssignmentId: 'assignment:one',
    backendId: 'backend-one',
    ifMatch: '"opaque-assignment-7"',
    idempotencyKey: 'idem-client-create-1',
    headers: {'X-CSRF-Token': 'csrf-client-1'}
  };

  responseFactory = () => response(202, {
    operationId: 'op_0123456789abcdef0123456789abcdef',
    state: 'accepted',
    backendId: 'backend-one',
    links: {
      self: '/api/v1/operations/op_0123456789abcdef0123456789abcdef'
    }
  }, {
    Location: '/api/v1/operations/op_0123456789abcdef0123456789abcdef',
    ETag: '"opaque-operation-1"'
  });

  const accepted = await client.submitTimerCreate(mutationOptions);

  assert.strictEqual(accepted.status, 202);
  assert.strictEqual(
    accepted.location,
    '/api/v1/operations/op_0123456789abcdef0123456789abcdef'
  );
  assert.strictEqual(accepted.etag, '"opaque-operation-1"');
  assert.strictEqual(
    accepted.data.operationId,
    'op_0123456789abcdef0123456789abcdef'
  );
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/timer-assignments/assignment:one?backend=backend-one'
  );
  assert.strictEqual(requests[0].options.method, 'POST');
  assert.strictEqual(requests[0].options.body, '{}');
  assert.strictEqual(requests[0].options.headers['Content-Type'], 'application/json');
  assert.strictEqual(requests[0].options.headers['If-Match'], '"opaque-assignment-7"');
  assert.strictEqual(requests[0].options.headers['Idempotency-Key'], 'idem-client-create-1');
  assert.strictEqual(requests[0].options.headers['X-CSRF-Token'], 'csrf-client-1');
  assert.strictEqual(requests[0].options.headers.Authorization, 'Bearer timer-create-token');

  // Exact replay is an explicit caller action. The client neither invents a new
  // key nor sends an automatic retry.
  const replay = await client.submitTimerCreate(mutationOptions);
  assert.strictEqual(replay.status, 202);
  assert.strictEqual(requests.length, 2);
  assert.strictEqual(requests[1].options.headers['If-Match'], '"opaque-assignment-7"');
  assert.strictEqual(requests[1].options.headers['Idempotency-Key'], 'idem-client-create-1');

  const beforeInvalid = requests.length;
  for (const invalidOptions of [
    {},
    {timerAssignmentId: 'assignment:one', backendId: 'backend-one', idempotencyKey: 'idem'},
    {timerAssignmentId: 'assignment:one', backendId: 'backend-one', ifMatch: '"tag"'},
    {timerAssignmentId: 'assignment/one', backendId: 'backend-one', ifMatch: '"tag"', idempotencyKey: 'idem'}
  ]) {
    assert.throws(() => client.submitTimerCreate(invalidOptions));
  }
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(412, {
    type: 'urn:vdr-suite:error:revision-conflict',
    title: 'Revision conflict',
    status: 412,
    detail: 'TimerAssignment revision changed.',
    code: 'revision_conflict',
    requestId: 'req-timer-create-412'
  });
  let stale = null;
  try {
    await client.submitTimerCreate({
      timerAssignmentId: 'assignment:one',
      backendId: 'backend-one',
      ifMatch: '"opaque-assignment-7"',
      idempotencyKey: 'idem-client-create-stale'
    });
  } catch (error) {
    stale = error;
  }
  assert.ok(stale);
  assert.strictEqual(api.isClientError(stale), true);
  assert.strictEqual(stale.status, 412);
  assert.strictEqual(stale.code, 'revision_conflict');

  responseFactory = () => response(409, {
    type: 'urn:vdr-suite:error:idempotency-conflict',
    title: 'Idempotency conflict',
    status: 409,
    detail: 'Idempotency key conflicts with another request.',
    code: 'idempotency_conflict',
    requestId: 'req-timer-create-409'
  });
  let conflict = null;
  try {
    await client.submitTimerCreate({
      timerAssignmentId: 'assignment:one',
      backendId: 'backend-one',
      ifMatch: '"opaque-assignment-8"',
      idempotencyKey: 'idem-client-create-1'
    });
  } catch (error) {
    conflict = error;
  }
  assert.ok(conflict);
  assert.strictEqual(conflict.status, 409);
  assert.strictEqual(conflict.code, 'idempotency_conflict');
  assert.strictEqual(requests.length, beforeInvalid + 2);

  console.log('test_public_v1_timer_create_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
