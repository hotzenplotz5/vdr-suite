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
  let responseFactory = () => response(200, null);

  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    headers: {Authorization: 'Bearer timer-item-token'},
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  responseFactory = () => response(200, {
    timerAssignmentId: 'assignment:one',
    backendId: 'backend-one',
    links: {
      self: '/api/v1/timer-assignments/assignment:one?backend=backend-one'
    }
  }, {
    ETag: '"opaque-ta-7"',
    'X-Request-ID': 'req-ta-item-200'
  });

  const first = await client.getTimerAssignment({
    timerAssignmentId: 'assignment:one',
    backendId: 'backend-one',
    cache: 'no-store'
  });

  assert.strictEqual(first.status, 200);
  assert.strictEqual(first.etag, '"opaque-ta-7"');
  assert.strictEqual(first.data.timerAssignmentId, 'assignment:one');
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/timer-assignments/assignment:one?backend=backend-one'
  );
  assert.strictEqual(requests[0].options.method, 'GET');
  assert.strictEqual(requests[0].options.cache, 'no-store');
  assert.strictEqual(
    requests[0].options.headers.Authorization,
    'Bearer timer-item-token'
  );

  responseFactory = () => response(304, null, {
    ETag: '"opaque-ta-7"'
  });
  const notModified = await client.getTimerAssignment({
    timerAssignmentId: 'assignment:one',
    backendId: 'backend-one',
    ifNoneMatch: first.etag
  });

  assert.strictEqual(notModified.status, 304);
  assert.strictEqual(notModified.etag, '"opaque-ta-7"');
  assert.strictEqual(notModified.data, null);
  assert.strictEqual(
    requests[1].options.headers['If-None-Match'],
    '"opaque-ta-7"'
  );

  const beforeInvalid = requests.length;
  for (const invalidOptions of [
    {},
    {timerAssignmentId: '', backendId: 'backend-one'},
    {timerAssignmentId: 'assignment/one', backendId: 'backend-one'},
    {timerAssignmentId: 'assignment:one', backendId: ''},
    {timerAssignmentId: 'assignment:one', backendId: 'backend-one', ifNoneMatch: ''}
  ]) {
    assert.throws(() => client.getTimerAssignment(invalidOptions));
  }
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(400, {
    type: 'urn:vdr-suite:error:invalid-request',
    title: 'Invalid request',
    status: 400,
    detail: 'If-None-Match is malformed.',
    code: 'invalid_request',
    requestId: 'req-ta-item-400'
  });
  let malformed = null;
  try {
    await client.getTimerAssignment({
      timerAssignmentId: 'assignment:one',
      backendId: 'backend-one',
      ifNoneMatch: 'not-an-etag'
    });
  } catch (error) {
    malformed = error;
  }
  assert.ok(malformed);
  assert.strictEqual(api.isClientError(malformed), true);
  assert.strictEqual(malformed.status, 400);
  assert.strictEqual(malformed.code, 'invalid_request');

  responseFactory = () => response(404, {
    type: 'urn:vdr-suite:error:not-found',
    title: 'Resource not found',
    status: 404,
    detail: 'The requested public API resource is not available.',
    code: 'not_found',
    requestId: 'req-ta-item-404'
  });
  let hidden = null;
  try {
    await client.getTimerAssignment({
      timerAssignmentId: 'assignment:one',
      backendId: 'backend-two'
    });
  } catch (error) {
    hidden = error;
  }
  assert.ok(hidden);
  assert.strictEqual(hidden.status, 404);
  assert.strictEqual(hidden.code, 'not_found');
  assert.strictEqual(requests.length, beforeInvalid + 2);

  console.log('test_public_v1_timer_assignment_item_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
