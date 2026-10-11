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
    baseUrl: 'https://suite.example',
    headers: {Authorization: 'Bearer device-secret'},
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(response(200, {
        items: [{ recordingId: 'rec_abc', backendId: 'backend-a',
          title: 'Film', recordedAt: '2026-10-09', durationSeconds: 3600 }],
        page: {limit: 50, nextCursor: null, hasMore: false}
      }));
    }
  });

  const result = await client.getRecordings({
    query: {backendId: 'backend-a', limit: 50, sort: 'recordingId', order: 'asc'}
  });
  assert.strictEqual(result.items[0].recordingId, 'rec_abc');
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/recordings?backendId=backend-a&limit=50&sort=recordingId&order=asc');
  assert.strictEqual(requests[0].options.headers.Authorization, 'Bearer device-secret');
  assert.strictEqual(requests[0].options.method, 'GET');

  const prior = requests.length;
  for (const query of [{}, {backendId: 'backend-a', limit: 0},
    {backendId: 'backend-a', limit: 101},
    {backendId: 'backend-a', sort: 'title'},
    {backendId: 'backend-a', order: 'desc'},
    {backendId: 'backend-a', offset: 1},
    {backendId: '../bad'}]) {
    assert.throws(() => client.getRecordings({query}));
  }
  assert.strictEqual(requests.length, prior);
  console.log('test_public_v1_recording_client passed');
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
