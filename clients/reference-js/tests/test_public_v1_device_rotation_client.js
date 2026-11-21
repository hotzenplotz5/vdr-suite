'use strict';
const assert = require('assert');
const api = require('../public-v1-client.js');

async function run() {
  const calls = [];
  const etag = '"vsr-646576696365"';
  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    fetch(url, options) {
      calls.push({url, options});
      return Promise.resolve({
        ok: true, status: 200, headers: {get() { return null; }},
        text() {
          return Promise.resolve(JSON.stringify({
            deviceId: 'device_mu10e3', actorId: 'actor_mu10e3',
            credentialId: 'credential_device_replacement',
            credentialSecret: 'one-time-secret'
          }));
        }
      });
    }
  });
  const rotated = await client.rotateDeviceCredential({
    deviceId: 'device_mu10e3',
    credentialId: 'credential_mu10e3',
    ifMatch: etag, headers: {'X-CSRF-Token':'browser-csrf'}
  });
  assert.strictEqual(rotated.data.credentialSecret, 'one-time-secret');
  assert.strictEqual(calls.length,1);
  assert.strictEqual(calls[0].url,
    'https://suite.example/api/v1/devices/device_mu10e3/credentials/credential_mu10e3/rotate');
  assert.strictEqual(calls[0].options.method,'POST');
  assert.strictEqual(calls[0].options.body,'{}');
  assert.strictEqual(calls[0].options.headers['If-Match'],etag);
  assert.strictEqual(calls[0].options.headers['X-CSRF-Token'],'browser-csrf');
  assert.throws(()=>client.rotateDeviceCredential({
    deviceId:'../bad',credentialId:'credential_mu10e3',ifMatch:etag
  }));
  assert.throws(()=>client.rotateDeviceCredential({
    deviceId:'device_mu10e3',credentialId:'credential_mu10e3'
  }));
}
run().catch(error=>{console.error(error);process.exitCode=1;});
