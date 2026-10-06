'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const clientPath = path.resolve(__dirname, '..', 'public-v1-client.js');
const source = fs.readFileSync(clientPath, 'utf8');
const api = require(clientPath);

function response(status, payload, headers) {
  const normalized = {};
  Object.keys(headers || {}).forEach(function (name) {
    normalized[String(name).toLowerCase()] = headers[name];
  });
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: {
      get(name) {
        const value = normalized[String(name).toLowerCase()];
        return value === undefined ? null : value;
      }
    },
    text() {
      return Promise.resolve(payload === null ? '' : JSON.stringify(payload));
    }
  };
}

async function run() {
  const requests = [];
  let responseFactory = () => response(200, null);

  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  responseFactory = () => response(201, {
    pairingRequestId: 'dpr_0123456789abcdef0123456789abcdef',
    userCode: 'ABCD-EFGH',
    pairingToken: 'pairing-token-0123456789abcdef0123456789',
    status: 'pending',
    expiresAt: '2026-10-06T18:00:00Z',
    pollIntervalSeconds: 3,
    client: {
      displayName: 'Hisense 43A6K',
      clientKind: 'vidaa',
      appVersion: 'phase1'
    }
  }, {
    Location: '/api/v1/device-pairings/dpr_0123456789abcdef0123456789abcdef'
  });

  const created = await client.createDevicePairing({
    displayName: 'Hisense 43A6K',
    clientKind: 'vidaa',
    appVersion: 'phase1'
  });

  assert.strictEqual(created.status, 201);
  assert.strictEqual(created.data.userCode, 'ABCD-EFGH');
  assert.strictEqual(
    requests[0].url,
    'https://suite.example/api/v1/device-pairings'
  );
  assert.strictEqual(requests[0].options.method, 'POST');
  assert.strictEqual(
    requests[0].options.headers['Content-Type'],
    'application/json'
  );
  assert.strictEqual(
    Object.prototype.hasOwnProperty.call(
      requests[0].options.headers,
      'Idempotency-Key'
    ),
    false
  );
  assert.deepStrictEqual(
    JSON.parse(requests[0].options.body),
    {
      displayName: 'Hisense 43A6K',
      clientKind: 'vidaa',
      appVersion: 'phase1'
    }
  );

  responseFactory = () => response(200, {
    pairingRequestId: 'dpr_0123456789abcdef0123456789abcdef',
    status: 'pending',
    expiresAt: '2026-10-06T18:00:00Z',
    pollIntervalSeconds: 3,
    client: {
      displayName: 'Hisense 43A6K',
      clientKind: 'vidaa',
      appVersion: 'phase1'
    }
  });

  const pending = await client.getDevicePairing({
    pairingRequestId:
      'dpr_0123456789abcdef0123456789abcdef',
    pairingToken:
      'pairing-token-0123456789abcdef0123456789',
    cache: 'no-store'
  });
  assert.strictEqual(pending.status, 'pending');
  assert.strictEqual(
    requests[1].url,
    'https://suite.example/api/v1/device-pairings/'
      + 'dpr_0123456789abcdef0123456789abcdef'
  );
  assert.strictEqual(requests[1].options.method, 'GET');
  assert.strictEqual(requests[1].options.cache, 'no-store');
  assert.strictEqual(
    requests[1].options.headers['X-VDR-Suite-Pairing-Token'],
    'pairing-token-0123456789abcdef0123456789'
  );

  const beforeInvalid = requests.length;
  for (const invalidOptions of [
    {},
    {pairingRequestId: '', pairingToken: 'x'},
    {pairingRequestId: 'bad/id', pairingToken: 'x'},
    {pairingRequestId: 'dpr_good', pairingToken: ''}
  ]) {
    assert.throws(() => client.getDevicePairing(invalidOptions));
  }
  assert.throws(
    () => client.createDevicePairing({
      displayName: '',
      clientKind: 'vidaa'
    })
  );
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(410, {
    type: 'urn:vdr-suite:error:pairing-expired',
    title: 'Pairing request expired',
    status: 410,
    detail: 'The Device Pairing request has expired.',
    code: 'pairing_expired',
    requestId: 'req-pairing-expired'
  });
  let expired = null;
  try {
    await client.getDevicePairing({
      pairingRequestId:
        'dpr_0123456789abcdef0123456789abcdef',
      pairingToken: 'expired-token'
    });
  } catch (error) {
    expired = error;
  }
  assert.ok(expired);
  assert.strictEqual(expired.status, 410);
  assert.strictEqual(expired.code, 'pairing_expired');

  assert.ok(!source.includes('setInterval('));
  assert.ok(!source.includes('setTimeout('));
  assert.ok(!source.includes('/api/security/browser-sessions'));
  assert.ok(!source.includes('Basic '));

  console.log('test_public_v1_device_pairing_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
