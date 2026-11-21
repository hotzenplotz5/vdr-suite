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
    headers: {Authorization: 'Bearer operation-token'},
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  responseFactory = () => response(200, {
    operationId: 'op_0123456789abcdef0123456789abcdef',
    state: 'outcome_unknown',
    backendId: 'backend-one',
    links: {
      self: '/api/v1/operations/op_0123456789abcdef0123456789abcdef'
    }
  }, {
    ETag: '"opaque-operation-9"',
    'Cache-Control': 'no-store'
  });

  const first = await client.getOperation({
    operationId: 'op_0123456789abcdef0123456789abcdef',
    cache: 'no-store'
  });

  assert.strictEqual(first.status, 200);
  assert.strictEqual(first.etag, '"opaque-operation-9"');
  assert.strictEqual(first.data.state, 'outcome_unknown');
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/operations/op_0123456789abcdef0123456789abcdef'
  );
  assert.strictEqual(requests[0].options.method, 'GET');
  assert.strictEqual(requests[0].options.cache, 'no-store');
  assert.strictEqual(requests[0].options.headers.Authorization, 'Bearer operation-token');

  responseFactory = () => response(304, null, {
    ETag: '"opaque-operation-9"'
  });
  const notModified = await client.getOperation({
    operationId: 'op_0123456789abcdef0123456789abcdef',
    ifNoneMatch: first.etag
  });

  assert.strictEqual(notModified.status, 304);
  assert.strictEqual(notModified.etag, '"opaque-operation-9"');
  assert.strictEqual(notModified.data, null);
  assert.strictEqual(
    requests[1].options.headers['If-None-Match'],
    '"opaque-operation-9"'
  );

  const beforeInvalid = requests.length;
  for (const invalidOptions of [
    {},
    {operationId: ''},
    {operationId: 'op/one'},
    {operationId: 'op?one'},
    {operationId: 'op#one'},
    {operationId: 'op-one', ifNoneMatch: ''}
  ]) {
    assert.throws(() => client.getOperation(invalidOptions));
  }
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(400, {
    type: 'urn:vdr-suite:error:invalid-request',
    title: 'Invalid request',
    status: 400,
    detail: 'If-None-Match is malformed.',
    code: 'invalid_request',
    requestId: 'req-operation-400'
  });
  let malformed = null;
  try {
    await client.getOperation({
      operationId: 'op-one',
      ifNoneMatch: '*, "opaque-operation-9"'
    });
  } catch (error) {
    malformed = error;
  }
  assert.ok(malformed);
  assert.strictEqual(malformed.status, 400);
  assert.strictEqual(malformed.code, 'invalid_request');

  responseFactory = () => response(404, {
    type: 'urn:vdr-suite:error:not-found',
    title: 'Resource not found',
    status: 404,
    detail: 'The requested public API resource is not available.',
    code: 'not_found',
    requestId: 'req-operation-404'
  });
  let hidden = null;
  try {
    await client.getOperation({operationId: 'op-hidden'});
  } catch (error) {
    hidden = error;
  }
  assert.ok(hidden);
  assert.strictEqual(hidden.status, 404);
  assert.strictEqual(hidden.code, 'not_found');

  responseFactory = () => response(503, {
    type: 'urn:vdr-suite:error:service-unavailable',
    title: 'Service unavailable',
    status: 503,
    detail: 'Durable operation lookup is unavailable.',
    code: 'service_unavailable',
    requestId: 'req-operation-503'
  });
  let unavailable = null;
  try {
    await client.getOperation({operationId: 'op-unavailable'});
  } catch (error) {
    unavailable = error;
  }
  assert.ok(unavailable);
  assert.strictEqual(unavailable.status, 503);
  assert.strictEqual(unavailable.code, 'service_unavailable');
  assert.strictEqual(requests.length, beforeInvalid + 3);

  console.log('test_public_v1_operation_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
