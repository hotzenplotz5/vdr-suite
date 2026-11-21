'use strict';

const assert = require('assert');
const api = require('../public-v1-client.js');

async function run() {
  const calls = [];
  const etag = '"vsr-6465766963652d6c6966656379636c653a6465766963655f6d75313065323a616374697665"';
  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    fetch(url, options) {
      calls.push({url, options});
      return Promise.resolve({
        ok: true, status: 200,
        headers: {get(name) {
          return name.toLowerCase() === 'etag' ? etag : null;
        }},
        text() {
          return Promise.resolve(JSON.stringify({
            deviceId: 'device_mu10e2', actorId: 'actor_mu10e2',
            credentialId: '', active: true, revoked: false
          }));
        }
      });
    }
  });
  await client.getDeviceLifecycle({deviceId: 'device_mu10e2'});
  assert.strictEqual(calls[0].url,
    'https://suite.example/api/v1/devices/device_mu10e2/lifecycle');
  assert.strictEqual(calls[0].options.method, 'GET');

  await client.revokeDevice({
    deviceId: 'device_mu10e2', ifMatch: etag,
    headers: {'X-CSRF-Token': 'browser-csrf'}
  });
  assert.strictEqual(calls[1].options.method, 'POST');
  assert.strictEqual(calls[1].options.headers['If-Match'], etag);
  assert.strictEqual(calls[1].options.headers['X-CSRF-Token'], 'browser-csrf');
  assert.strictEqual(calls[1].options.body, '{}');

  await client.getDeviceCredentialLifecycle({
    deviceId: 'device_mu10e2', credentialId: 'credential_mu10e2'
  });
  assert.strictEqual(calls[2].url,
    'https://suite.example/api/v1/devices/device_mu10e2/'
      + 'credentials/credential_mu10e2/lifecycle');

  await client.revokeDeviceCredential({
    deviceId: 'device_mu10e2', credentialId: 'credential_mu10e2',
    ifMatch: etag
  });
  assert.strictEqual(calls[3].options.method, 'POST');
  assert.strictEqual(calls[3].options.body, '{}');
  assert.throws(() => client.getDeviceLifecycle({deviceId: '../wrong'}));
  assert.throws(() => client.revokeDeviceCredential({
    deviceId: 'device_mu10e2', credentialId: '../wrong', ifMatch: etag
  }));
  assert.throws(() => client.revokeDevice({
    deviceId: 'device_mu10e2'
  }));
}
run().catch(error => { console.error(error); process.exitCode = 1; });
