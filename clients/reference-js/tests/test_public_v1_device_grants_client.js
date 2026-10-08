'use strict';

const assert = require('assert');
const api = require('../public-v1-client.js');

async function run() {
  const sent = [];
  const etag = '"grant-set:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"';
  const fetch = (url, options) => {
    sent.push({url, options});
    return Promise.resolve({
      ok: true,
      status: 200,
      headers: {get(name) { return name.toLowerCase() === 'etag' ? etag : null; }},
      text() { return Promise.resolve(JSON.stringify({
        deviceId: 'device_10001', actorId: 'actor_device_10001', items: []
      })); }
    });
  };
  const client = api.createClient({
    baseUrl: 'https://suite.example',
    fetch,
    credentials: 'include'
  });

  const result = await client.getDeviceGrants({
    deviceId: 'device_10001', credentials: 'include'
  });
  assert.strictEqual(result.etag, etag);
  assert.strictEqual(sent[0].url,
    'https://suite.example/api/v1/devices/device_10001/grants');
  assert.strictEqual(sent[0].options.method, 'GET');

  const changed = await client.setDeviceGrant({
    deviceId: 'device_10001',
    permission: 'channels.view',
    backendId: 'default',
    active: true,
    ifMatch: etag,
    headers: {'X-CSRF-Token': 'browser-csrf-token'}
  });
  assert.strictEqual(changed.etag, etag);
  assert.strictEqual(sent[1].options.method, 'POST');
  assert.strictEqual(sent[1].options.headers['If-Match'], etag);
  assert.strictEqual(
    sent[1].options.headers['X-CSRF-Token'], 'browser-csrf-token');
  assert.deepStrictEqual(JSON.parse(sent[1].options.body), {
    permission: 'channels.view', backendId: 'default', active: true
  });

  assert.throws(() => client.getDeviceGrants({deviceId: '../../bad'}));
  assert.throws(() => client.setDeviceGrant({
    deviceId: 'device_10001', permission: 'channels.view',
    backendId: 'default', active: true
  }));
  assert.throws(() => client.setDeviceGrant({
    deviceId: 'device_10001', permission: 'channels.view',
    backendId: 'default', active: 'true', ifMatch: etag
  }));
}
run().catch(err => { console.error(err); process.exitCode = 1; });
