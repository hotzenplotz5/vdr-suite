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
    baseUrl: 'https://suite.example/',
    headers: {Authorization: 'Bearer timer-assignment-token'},
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  responseFactory = () => response(200, {
    items: [{
      timerAssignmentId: 'assignment:a',
      backendId: 'backend-one',
      links: {
        self: '/api/v1/timer-assignments/assignment:a?backend=backend-one'
      }
    }],
    page: {limit: 2, nextCursor: 'ta1_after-a', hasMore: true},
    meta: {partial: false},
    links: {self: '/api/v1/timer-assignments?backend=backend-one&limit=2', next: null}
  });

  const first = await client.getTimerAssignments({
    query: {
      backendId: 'backend-one',
      limit: 2,
      cursor: 'ta1_after-previous',
      sort: 'timerAssignmentId',
      order: 'asc'
    },
    cache: 'no-store'
  });

  assert.strictEqual(first.items[0].timerAssignmentId, 'assignment:a');
  assert.strictEqual(first.meta.partial, false);
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/timer-assignments?backend=backend-one&limit=2&cursor=ta1_after-previous&sort=timerAssignmentId&order=asc'
  );
  assert.strictEqual(requests[0].options.method, 'GET');
  assert.strictEqual(requests[0].options.cache, 'no-store');
  assert.strictEqual(
    requests[0].options.headers.Authorization,
    'Bearer timer-assignment-token'
  );

  const beforeInvalid = requests.length;
  for (const invalidQuery of [
    {},
    {backendId: ''},
    {backendId: 'backend-one', limit: 0},
    {backendId: 'backend-one', limit: 101},
    {backendId: 'backend-one', cursor: ''},
    {backendId: 'backend-one', sort: 'state'},
    {backendId: 'backend-one', order: 'desc'},
    {backendId: 'backend-one', offset: 1}
  ]) {
    assert.throws(() => client.getTimerAssignments({query: invalidQuery}));
  }
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(503, {
    type: 'urn:vdr-suite:error:service-unavailable',
    title: 'Service unavailable',
    status: 503,
    detail: 'TimerAssignment repository is unavailable.',
    code: 'service_unavailable',
    requestId: 'req-ta-collection-503'
  });

  let failure = null;
  try {
    await client.getTimerAssignments({
      query: {backendId: 'backend-one'}
    });
  } catch (error) {
    failure = error;
  }

  assert.ok(failure);
  assert.strictEqual(api.isClientError(failure), true);
  assert.strictEqual(failure.status, 503);
  assert.strictEqual(failure.code, 'service_unavailable');
  assert.strictEqual(failure.requestId, 'req-ta-collection-503');
  assert.strictEqual(requests.length, beforeInvalid + 1);

  console.log('test_public_v1_timer_assignment_collection_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
