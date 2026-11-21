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
    sessionId: 'session-a',
    deviceId: 'device-a',
    issuedFromCredentialId: 'credential-human',
    active: true,
    expired: false,
    revoked: false,
    expiresAt: '2099-01-01 00:00:00',
    lastSeenAt: null,
    createdAt: '2026-10-03 18:00:00'
  }, {
    ETag: '"session-etag-a"'
  });

  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  const read = await client.getAccountSession({
    accountId: 'account-a',
    sessionId: 'session-a',
    ifNoneMatch: '"known-session"'
  });
  assert.strictEqual(read.status, 200);
  assert.strictEqual(read.etag, '"session-etag-a"');
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/accounts/account-a/sessions/session-a'
  );
  assert.strictEqual(requests[0].options.method, 'GET');
  assert.strictEqual(
    requests[0].options.headers['If-None-Match'],
    '"known-session"'
  );

  responseFactory = () => response(200, {
    accountId: 'account-a',
    actorId: 'actor-a',
    sessionId: 'session-a',
    deviceId: 'device-a',
    issuedFromCredentialId: 'credential-human',
    active: false,
    expired: false,
    revoked: true,
    expiresAt: '2099-01-01 00:00:00',
    lastSeenAt: null,
    createdAt: '2026-10-03 18:00:00'
  }, {
    ETag: '"session-etag-b"'
  });

  const revoked = await client.revokeAccountSession({
    accountId: 'account-a',
    sessionId: 'session-a',
    ifMatch: '"session-etag-a"',
    headers: {'X-CSRF-Token': 'csrf-test'}
  });
  assert.strictEqual(revoked.status, 200);
  assert.strictEqual(revoked.etag, '"session-etag-b"');
  assert.strictEqual(requests[1].options.method, 'POST');
  assert.strictEqual(
    requests[1].options.headers['If-Match'],
    '"session-etag-a"'
  );
  assert.strictEqual(
    requests[1].options.headers['X-CSRF-Token'],
    'csrf-test'
  );
  assert.deepStrictEqual(
    JSON.parse(requests[1].options.body),
    {}
  );

  const beforeInvalid = requests.length;
  assert.throws(() => client.getAccountSession({
    accountId: 'account-a',
    sessionId: ''
  }), /sessionId/);
  assert.throws(() => client.getAccountSession({
    accountId: 'account-a',
    sessionId: 'bad/id'
  }), /path delimiter/);
  assert.throws(() => client.revokeAccountSession({
    accountId: 'account-a',
    sessionId: 'session-a',
    ifMatch: ''
  }), /ifMatch/);
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(412, {
    title: 'Resource revision conflict',
    status: 412,
    code: 'revision_conflict'
  });

  await assert.rejects(
    client.revokeAccountSession({
      accountId: 'account-a',
      sessionId: 'session-a',
      ifMatch: '"stale"'
    }),
    error => error instanceof api.PublicClientError
      && error.status === 412
      && error.code === 'revision_conflict'
  );

  console.log('test_public_v1_account_session_revoke_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
