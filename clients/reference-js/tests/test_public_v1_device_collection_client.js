'use strict';
const assert = require('assert');
const api = require('../public-v1-client.js');
async function run() {
  const requests = [];
  const client = api.createClient({
    baseUrl: 'https://suite.example',
    credentials: 'same-origin',
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve({
        ok: true, status: 200,
        headers: {get() { return null; }},
        text() { return Promise.resolve(JSON.stringify({
          items: [{deviceId: 'device_a', actorId: 'actor_a',
            displayName: 'Hisense Wohnzimmer', active: true, revoked: false}],
          page: {limit: 25, hasMore: false, nextCursor: null}
        })); }
      });
    }
  });
  const first = await client.getDevices({query: {limit: 25}});
  assert.strictEqual(first.items[0].displayName, 'Hisense Wohnzimmer');
  assert.strictEqual(requests[0].url,
    'https://suite.example/api/v1/devices?limit=25');
  assert.strictEqual(requests[0].options.method, 'GET');
  await client.getDevices({query: {limit: 25, cursor: 'dv1_device_a'}});
  assert.strictEqual(requests[1].url,
    'https://suite.example/api/v1/devices?limit=25&cursor=dv1_device_a');
  assert.throws(() => client.getDevices({query: {limit: 101}}));
  assert.throws(() => client.getDevices({query: {cursor: '../private'}}));
  assert.throws(() => client.getDevices({query: {extra: 1}}));
  assert.throws(() => client.getDevices({query: {sort: 'displayName'}}));
}
run().catch(error => { console.error(error); process.exitCode = 1; });
