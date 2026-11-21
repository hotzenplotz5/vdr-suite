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
  let responseFactory = () => response(200, {
    accountId: 'account-a',
    actorId: 'actor-a',
    items: [
      {permission: 'channels.view', backendId: 'default'}
    ],
    links: {self: '/api/v1/accounts/account-a/grants'}
  }, {
    ETag: '"grant-etag-a"'
  });

  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  const read = await client.getAccountGrants({
    accountId: 'account-a',
    ifNoneMatch: '"known-grants"'
  });
  assert.strictEqual(read.status, 200);
  assert.strictEqual(read.etag, '"grant-etag-a"');
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/accounts/account-a/grants'
  );
  assert.strictEqual(requests[0].options.method, 'GET');
  assert.strictEqual(
    requests[0].options.headers['If-None-Match'],
    '"known-grants"'
  );

  responseFactory = () => response(200, {
    accountId: 'account-a',
    actorId: 'actor-a',
    items: [
      {permission: 'channels.view', backendId: 'default'},
      {permission: 'timers.view', backendId: 'default'}
    ]
  }, {
    ETag: '"grant-etag-b"'
  });

  const changed = await client.setAccountGrant({
    accountId: 'account-a',
    permission: 'timers.view',
    backendId: 'default',
    active: true,
    ifMatch: '"grant-etag-a"',
    headers: {'X-CSRF-Token': 'csrf-test'}
  });

  assert.strictEqual(changed.status, 200);
  assert.strictEqual(changed.etag, '"grant-etag-b"');
  assert.strictEqual(requests[1].options.method, 'POST');
  assert.strictEqual(
    requests[1].options.headers['If-Match'],
    '"grant-etag-a"'
  );
  assert.strictEqual(
    requests[1].options.headers['X-CSRF-Token'],
    'csrf-test'
  );
  assert.deepStrictEqual(
    JSON.parse(requests[1].options.body),
    {
      permission: 'timers.view',
      backendId: 'default',
      active: true
    }
  );

  const beforeInvalid = requests.length;
  assert.throws(() => client.setAccountGrant({
    accountId: 'account-a',
    permission: '',
    backendId: 'default',
    active: true,
    ifMatch: '"x"'
  }), /permission/);
  assert.throws(() => client.setAccountGrant({
    accountId: 'account-a',
    permission: 'channels.view',
    backendId: '',
    active: true,
    ifMatch: '"x"'
  }), /backendId/);
  assert.throws(() => client.setAccountGrant({
    accountId: 'account-a',
    permission: 'channels.view',
    backendId: 'default',
    active: 'yes',
    ifMatch: '"x"'
  }), /active/);
  assert.throws(() => client.setAccountGrant({
    accountId: 'account-a',
    permission: 'channels.view',
    backendId: 'default',
    active: true,
    ifMatch: ''
  }), /ifMatch/);
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(412, {
    title: 'Resource revision conflict',
    status: 412,
    code: 'revision_conflict'
  });

  await assert.rejects(
    client.setAccountGrant({
      accountId: 'account-a',
      permission: 'channels.view',
      backendId: 'default',
      active: false,
      ifMatch: '"stale"'
    }),
    error => error instanceof api.PublicClientError
      && error.status === 412
      && error.code === 'revision_conflict'
  );

  console.log('test_public_v1_account_grants_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
