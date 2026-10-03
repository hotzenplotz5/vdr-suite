'use strict';

const assert = require('assert');
const api = require('../public-v1-client.js');

function response(status, payload) {
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: {get() { return null; }},
    text() { return Promise.resolve(JSON.stringify(payload)); }
  };
}

async function run() {
  const requests = [];
  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(response(200, {
        accountId: 'account-a',
        actorId: 'actor-a',
        items: []
      }));
    }
  });

  const credentials = await client.getAccountCredentials({
    accountId: 'account-a'
  });
  assert.strictEqual(credentials.status, 200);
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/accounts/account-a/credentials'
  );
  assert.strictEqual(requests[0].options.method, 'GET');

  const sessions = await client.getAccountSessions({
    accountId: 'account-a'
  });
  assert.strictEqual(sessions.status, 200);
  assert.strictEqual(
    requests[1].url,
    'https://suite.example/api/v1/accounts/account-a/sessions'
  );
  assert.strictEqual(requests[1].options.method, 'GET');

  const count = requests.length;
  assert.throws(() => client.getAccountCredentials({
    accountId: ''
  }), /accountId/);
  assert.throws(() => client.getAccountSessions({
    accountId: 'bad/id'
  }), /path delimiter/);
  assert.strictEqual(requests.length, count);

  console.log('test_public_v1_account_security_metadata_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
