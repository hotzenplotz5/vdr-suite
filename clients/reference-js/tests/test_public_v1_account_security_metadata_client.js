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
  assert.deepStrictEqual(credentials, {
    accountId: 'account-a',
    actorId: 'actor-a',
    items: []
  });
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/accounts/account-a/credentials'
  );
  assert.strictEqual(requests[0].options.method, 'GET');

  const sessions = await client.getAccountSessions({
    accountId: 'account-a'
  });
  assert.deepStrictEqual(sessions, {
    accountId: 'account-a',
    actorId: 'actor-a',
    items: []
  });
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

  const body = {recordingId:'rec_' + 'a'.repeat(32),operation:'set',groupType:'season',
    seasonNumber:3,expectedRevision:2};
  await client.setRecordingSeriesHierarchy({backendId:'living-room',payload:body,
    headers:{'X-VDR-Suite-CSRF':'test-token'}});
  const hierarchy = requests[requests.length-1];
  assert.strictEqual(hierarchy.url,
    'https://suite.example/api/v1/backends/living-room/recordings/series-hierarchy');
  assert.strictEqual(hierarchy.options.method,'POST');
  assert.deepStrictEqual(JSON.parse(hierarchy.options.body),body);
  assert.strictEqual(hierarchy.options.headers['X-VDR-Suite-CSRF'],'test-token');
  assert.throws(() => client.setRecordingSeriesHierarchy({backendId:'living-room',
    payload:Object.assign({},body,{resourceKey:'native'})}), /canonical rec_/);
  assert.strictEqual(requests.length,count+1);

  console.log('test_public_v1_account_security_metadata_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
