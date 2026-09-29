'use strict';

const assert = require('assert');
const path = require('path');

const clientPath = path.resolve(__dirname, '..', 'public-v1-client.js');
const api = require(clientPath);

function response(status, payload) {
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: {get() { return null; }},
    text() {
      return Promise.resolve(payload === null ? '' : JSON.stringify(payload));
    }
  };
}

async function run() {
  const requests = [];
  let responseFactory = () => response(200, {items: []});

  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    headers: {Authorization: 'Bearer account-reader'},
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  responseFactory = () => response(200, {
    items: [{
      accountId: 'account-a',
      actorId: 'actor-a',
      displayName: 'Admin A',
      active: true
    }],
    page: {limit: 25, hasMore: false, nextCursor: null},
    meta: {partial: false}
  });

  const result = await client.getAccounts({
    query: {
      limit: 25,
      cursor: 'ac1_after-a',
      sort: 'accountId',
      order: 'asc'
    }
  });

  assert.strictEqual(result.items[0].accountId, 'account-a');
  assert.strictEqual(result.items[0].actorId, 'actor-a');
  assert.strictEqual(result.items[0].displayName, 'Admin A');
  assert.strictEqual(result.items[0].active, true);
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/accounts?limit=25&cursor=ac1_after-a&sort=accountId&order=asc'
  );
  assert.strictEqual(requests[0].options.method, 'GET');
  assert.strictEqual(
    requests[0].options.headers.Authorization,
    'Bearer account-reader'
  );

  const beforeInvalid = requests.length;
  assert.throws(
    () => client.getAccounts({query: {offset: 1}}),
    /unsupported account query field: offset/
  );
  assert.throws(
    () => client.getAccounts({query: {limit: 0}}),
    /account query limit/
  );
  assert.throws(
    () => client.getAccounts({query: {sort: 'displayName'}}),
    /account query sort/
  );
  assert.throws(
    () => client.getAccounts({query: {order: 'desc'}}),
    /account query order/
  );
  assert.strictEqual(requests.length, beforeInvalid);

  console.log('test_public_v1_account_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
