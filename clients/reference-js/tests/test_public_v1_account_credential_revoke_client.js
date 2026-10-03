'use strict';

const assert = require('assert');
const api = require('../public-v1-client.js');

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
      return Promise.resolve(
        payload === null ? '' : JSON.stringify(payload)
      );
    }
  };
}

async function run() {
  const requests = [];
  let responseFactory = () => response(200, {
    accountId: 'account-a',
    actorId: 'actor-a',
    credentialId: 'credential-human',
    credentialType: 'human-password',
    active: true,
    expired: false,
    revoked: false,
    expiresAt: null,
    createdAt: '2026-10-03 18:00:00'
  }, {
    ETag: '"credential-etag-a"'
  });

  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  const read = await client.getAccountCredential({
    accountId: 'account-a',
    credentialId: 'credential-human',
    ifNoneMatch: '"known-credential"'
  });
  assert.strictEqual(read.status, 200);
  assert.strictEqual(read.etag, '"credential-etag-a"');
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/accounts/account-a/credentials/credential-human'
  );
  assert.strictEqual(requests[0].options.method, 'GET');
  assert.strictEqual(
    requests[0].options.headers['If-None-Match'],
    '"known-credential"'
  );

  responseFactory = () => response(200, {
    accountId: 'account-a',
    actorId: 'actor-a',
    credentialId: 'credential-human',
    credentialType: 'human-password',
    active: false,
    expired: false,
    revoked: true,
    expiresAt: null,
    createdAt: '2026-10-03 18:00:00'
  }, {
    ETag: '"credential-etag-b"'
  });

  const revoked = await client.revokeAccountCredential({
    accountId: 'account-a',
    credentialId: 'credential-human',
    ifMatch: '"credential-etag-a"',
    headers: {'X-CSRF-Token': 'csrf-test'}
  });
  assert.strictEqual(revoked.status, 200);
  assert.strictEqual(revoked.etag, '"credential-etag-b"');
  assert.strictEqual(requests[1].options.method, 'POST');
  assert.strictEqual(
    requests[1].options.headers['If-Match'],
    '"credential-etag-a"'
  );
  assert.strictEqual(
    requests[1].options.headers['X-CSRF-Token'],
    'csrf-test'
  );
  assert.deepStrictEqual(JSON.parse(requests[1].options.body), {});

  const beforeInvalid = requests.length;
  assert.throws(() => client.getAccountCredential({
    accountId: 'account-a',
    credentialId: ''
  }), /credentialId/);
  assert.throws(() => client.getAccountCredential({
    accountId: 'account-a',
    credentialId: 'bad/id'
  }), /path delimiter/);
  assert.throws(() => client.revokeAccountCredential({
    accountId: 'account-a',
    credentialId: 'credential-human',
    ifMatch: ''
  }), /ifMatch/);
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(412, {
    title: 'Resource revision conflict',
    status: 412,
    code: 'revision_conflict'
  });

  await assert.rejects(
    client.revokeAccountCredential({
      accountId: 'account-a',
      credentialId: 'credential-human',
      ifMatch: '"stale"'
    }),
    error => error instanceof api.PublicClientError
      && error.status === 412
      && error.code === 'revision_conflict'
  );

  responseFactory = () => response(409, {
    title: 'Operation conflict',
    status: 409,
    code: 'operation_conflict'
  });

  await assert.rejects(
    client.revokeAccountCredential({
      accountId: 'account-a',
      credentialId: 'credential-human',
      ifMatch: '"credential-etag-a"'
    }),
    error => error instanceof api.PublicClientError
      && error.status === 409
      && error.code === 'operation_conflict'
  );

  console.log('test_public_v1_account_credential_revoke_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
