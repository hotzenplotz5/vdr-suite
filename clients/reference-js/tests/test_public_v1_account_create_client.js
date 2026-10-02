'use strict';

const assert = require('assert');
const path = require('path');
const api = require(path.resolve(__dirname, '..', 'public-v1-client.js'));

function response(status, payload, headers) {
  const normalized = {};
  Object.keys(headers || {}).forEach(function (name) {
    normalized[name.toLowerCase()] = headers[name];
  });
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: {get(name) {
      const value = normalized[String(name).toLowerCase()];
      return value === undefined ? null : value;
    }},
    text() {
      return Promise.resolve(payload === null ? '' : JSON.stringify(payload));
    }
  };
}

async function run() {
  const requests = [];
  let responseFactory = () => response(201, {
    accountId: 'account-created',
    actorId: 'actor-created',
    displayName: 'Viewer',
    active: true,
    links: {self: '/api/v1/accounts/account-created'}
  }, {
    ETag: '"account-etag-1"',
    Location: '/api/v1/accounts/account-created'
  });

  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  const created = await client.createAccount({
    loginName: 'viewer',
    displayName: 'Viewer',
    password: 'pw-one',
    idempotencyKey: 'idem-create-1',
    headers: {'X-CSRF-Token': 'csrf-test-value'}
  });

  assert.strictEqual(created.status, 201);
  assert.strictEqual(created.etag, '"account-etag-1"');
  assert.strictEqual(created.location, '/api/v1/accounts/account-created');
  assert.strictEqual(requests[0].url, 'https://suite.example/api/v1/accounts');
  assert.strictEqual(requests[0].options.method, 'POST');
  assert.strictEqual(requests[0].options.headers['X-CSRF-Token'], 'csrf-test-value');
  assert.strictEqual(requests[0].options.headers['Idempotency-Key'], 'idem-create-1');
  assert.strictEqual(requests[0].options.headers['Content-Type'], 'application/json');
  assert.deepStrictEqual(JSON.parse(requests[0].options.body), {
    loginName: 'viewer',
    displayName: 'Viewer',
    password: 'pw-one'
  });

  responseFactory = () => response(201, {
    accountId: 'account-created',
    actorId: 'actor-created',
    displayName: 'Viewer',
    active: true
  }, {
    ETag: '"account-etag-1"',
    Location: '/api/v1/accounts/account-created'
  });

  const replayed = await client.createAccount({
    loginName: 'viewer',
    displayName: 'Viewer',
    password: 'pw-two',
    idempotencyKey: 'idem-create-1'
  });
  assert.strictEqual(replayed.status, 201);
  assert.strictEqual(replayed.data.accountId, 'account-created');

  const beforeInvalid = requests.length;
  assert.throws(() => client.createAccount({
    loginName: '', displayName: 'Viewer', password: 'x', idempotencyKey: 'idem'
  }), /loginName/);
  assert.throws(() => client.createAccount({
    loginName: 'viewer', displayName: '', password: 'x', idempotencyKey: 'idem'
  }), /displayName/);
  assert.throws(() => client.createAccount({
    loginName: 'viewer', displayName: 'Viewer', password: '', idempotencyKey: 'idem'
  }), /password/);
  assert.throws(() => client.createAccount({
    loginName: 'viewer', displayName: 'Viewer', password: 'x', idempotencyKey: ''
  }), /idempotencyKey/);
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(409, {
    title: 'Idempotency conflict',
    status: 409,
    code: 'idempotency_conflict'
  });

  await assert.rejects(
    client.createAccount({
      loginName: 'other',
      displayName: 'Other',
      password: 'x',
      idempotencyKey: 'idem-create-1'
    }),
    error => error instanceof api.PublicClientError
      && error.status === 409
      && error.code === 'idempotency_conflict'
  );

  console.log('test_public_v1_account_create_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
