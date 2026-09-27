'use strict';

const assert = require('assert');
const path = require('path');

const api = require(path.resolve(__dirname, '..', 'public-v1-client.js'));

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
    baseUrl: 'https://suite.example',
    headers: {Authorization: 'Bearer channel-token'},
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  responseFactory = () => response(200, {
    items: [{
      backendId: 'backend-a',
      channelId: 'S19.2E-1-1019-10301',
      channelNumber: 1,
      name: 'Channel One'
    }],
    page: {limit: 2, nextCursor: 'ch1_next', hasMore: true},
    meta: {
      partial: true,
      sources: [
        {backendId: 'backend-a', state: 'ok'},
        {backendId: 'backend-b', state: 'unavailable', code: 'backend_unavailable'}
      ]
    }
  });

  const first = await client.getChannels({
    query: {
      backendIds: ['backend-c', 'backend-a', 'backend-b'],
      limit: 2,
      cursor: 'ch1_after-a',
      sort: 'backendId,channelId',
      order: 'asc'
    },
    cache: 'no-store'
  });

  assert.strictEqual(first.meta.partial, true);
  assert.strictEqual(first.meta.sources[1].code, 'backend_unavailable');
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/channels?backendId=backend-c&backendId=backend-a&backendId=backend-b&limit=2&cursor=ch1_after-a&sort=backendId,channelId&order=asc'
  );
  assert.strictEqual(requests[0].options.method, 'GET');
  assert.strictEqual(requests[0].options.cache, 'no-store');
  assert.strictEqual(requests[0].options.headers.Authorization, 'Bearer channel-token');

  const beforeInvalid = requests.length;
  for (const invalidQuery of [
    {},
    {backendIds: []},
    {backendIds: Array.from({length: 17}, (_, i) => 'backend-' + i)},
    {backendIds: ['backend-a'], limit: 0},
    {backendIds: ['backend-a'], sort: 'channelId'},
    {backendIds: ['backend-a'], order: 'desc'},
    {backendIds: ['backend-a'], offset: 1}
  ]) {
    assert.throws(() => client.getChannels({query: invalidQuery}));
  }
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(409, {
    type: 'urn:vdr-suite:error:cursor-expired',
    title: 'Cursor expired',
    status: 409,
    detail: 'The requested backend source scope changed.',
    code: 'cursor_expired',
    requestId: 'req-channel-cursor'
  });

  let failure = null;
  try {
    await client.getChannels({
      query: {
        backendIds: ['backend-a', 'backend-c'],
        cursor: 'ch1_scope-bound'
      }
    });
  } catch (error) {
    failure = error;
  }

  assert.ok(failure);
  assert.strictEqual(api.isClientError(failure), true);
  assert.strictEqual(failure.status, 409);
  assert.strictEqual(failure.code, 'cursor_expired');
  assert.strictEqual(failure.requestId, 'req-channel-cursor');
  assert.strictEqual(requests.length, beforeInvalid + 1);

  console.log('test_public_v1_channel_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
