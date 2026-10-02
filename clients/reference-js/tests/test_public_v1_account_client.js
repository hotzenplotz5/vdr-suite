'use strict';

const assert = require('assert');
const path = require('path');

const clientPath = path.resolve(__dirname, '..', 'public-v1-client.js');
const api = require(clientPath);

function response(status, payload, headers) {
  const values = headers || {};
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: {
      get(name) {
        const wanted = String(name).toLowerCase();
        const key = Object.keys(values).find(
          candidate => candidate.toLowerCase() === wanted
        );
        return key === undefined ? null : values[key];
      }
    },
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

  responseFactory = () => response(
    200,
    {
      accountId: 'account-a',
      actorId: 'actor-a',
      displayName: 'Admin A',
      active: true,
      links: {self: '/api/v1/accounts/account-a'}
    },
    {ETag: '"vsr-account-a"'}
  );

  const item = await client.getAccount({
    accountId: 'account-a',
    ifNoneMatch: '"vsr-stale"'
  });

  assert.strictEqual(item.status, 200);
  assert.strictEqual(item.etag, '"vsr-account-a"');
  assert.strictEqual(item.data.accountId, 'account-a');
  assert.strictEqual(item.data.actorId, 'actor-a');
  assert.strictEqual(item.data.displayName, 'Admin A');
  assert.strictEqual(item.data.active, true);
  assert.strictEqual(
    requests[1].url,
    'https://suite.example/api/v1/accounts/account-a'
  );
  assert.strictEqual(requests[1].options.method, 'GET');
  assert.strictEqual(
    requests[1].options.headers['If-None-Match'],
    '"vsr-stale"'
  );

  responseFactory = () => response(
    304,
    null,
    {ETag: '"vsr-account-a"'}
  );
  const notModified = await client.getAccount({
    accountId: 'account-a',
    ifNoneMatch: '"vsr-account-a"'
  });
  assert.strictEqual(notModified.status, 304);
  assert.strictEqual(notModified.etag, '"vsr-account-a"');
  assert.strictEqual(notModified.data, null);

  responseFactory = () => response(
    200,
    {
      accountId: 'account-a',
      actorId: 'actor-a',
      displayName: 'Renamed Admin',
      active: true,
      links: {self: '/api/v1/accounts/account-a'}
    },
    {ETag: '"vsr-account-a-8"'}
  );
  const renamed = await client.updateAccountDisplayName({
    accountId: 'account-a',
    displayName: 'Renamed Admin',
    ifMatch: '"vsr-account-a"',
    headers: {'X-CSRF-Token': 'csrf-token'}
  });
  assert.strictEqual(renamed.status, 200);
  assert.strictEqual(renamed.etag, '"vsr-account-a-8"');
  assert.strictEqual(renamed.data.displayName, 'Renamed Admin');
  assert.strictEqual(requests[3].options.method, 'POST');
  assert.strictEqual(
    requests[3].options.headers['If-Match'],
    '"vsr-account-a"'
  );
  assert.strictEqual(
    requests[3].options.headers['X-CSRF-Token'],
    'csrf-token'
  );
  assert.strictEqual(
    requests[3].options.headers['Content-Type'],
    'application/json'
  );
  assert.deepStrictEqual(
    JSON.parse(requests[3].options.body),
    {displayName: 'Renamed Admin'}
  );

  responseFactory = () => response(
    200,
    {
      accountId: 'account-a',
      actorId: 'actor-a',
      displayName: 'Renamed Admin',
      active: false,
      links: {self: '/api/v1/accounts/account-a'}
    },
    {ETag: '"vsr-account-a-9"'}
  );
  const deactivated = await client.deactivateAccount({
    accountId: 'account-a',
    ifMatch: '"vsr-account-a-8"'
  });
  assert.strictEqual(deactivated.data.active, false);
  assert.deepStrictEqual(
    JSON.parse(requests[4].options.body),
    {active: false}
  );

  responseFactory = () => response(
    200,
    {
      accountId: 'account-a',
      actorId: 'actor-a',
      displayName: 'Renamed Admin',
      active: true,
      links: {self: '/api/v1/accounts/account-a'}
    },
    {ETag: '"vsr-account-a-10"'}
  );
  const activated = await client.activateAccount({
    accountId: 'account-a',
    ifMatch: '"vsr-account-a-9"'
  });
  assert.strictEqual(activated.data.active, true);
  assert.deepStrictEqual(
    JSON.parse(requests[5].options.body),
    {active: true}
  );

  const beforeInvalidMutation = requests.length;
  assert.throws(
    () => client.updateAccountDisplayName({
      accountId: 'account-a',
      displayName: '',
      ifMatch: '"vsr-account-a"'
    }),
    /displayName/
  );
  assert.throws(
    () => client.deactivateAccount({
      accountId: 'account-a',
      ifMatch: ''
    }),
    /ifMatch/
  );
  assert.strictEqual(requests.length, beforeInvalidMutation);

  responseFactory = () => response(
    412,
    {
      type: 'about:blank',
      title: 'Resource revision conflict',
      status: 412,
      code: 'revision_conflict',
      detail: 'The Account changed after it was read.'
    }
  );
  await assert.rejects(
    client.activateAccount({
      accountId: 'account-a',
      ifMatch: '"vsr-stale"'
    }),
    error => error instanceof api.PublicClientError
      && error.status === 412
      && error.code === 'revision_conflict'
  );

  const beforeInvalidItem = requests.length;
  assert.throws(
    () => client.getAccount({accountId: ''}),
    /Account item accountId/
  );
  assert.throws(
    () => client.getAccount({accountId: 'account/a'}),
    /path delimiter/
  );
  assert.throws(
    () => client.getAccount({accountId: 'account-a', ifNoneMatch: ''}),
    /ifNoneMatch/
  );
  assert.strictEqual(requests.length, beforeInvalidItem);

  console.log('test_public_v1_account_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
