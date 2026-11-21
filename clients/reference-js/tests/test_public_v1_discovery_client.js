'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const clientPath = path.resolve(__dirname, '..', 'public-v1-client.js');
const source = fs.readFileSync(clientPath, 'utf8');
const api = require(clientPath);

function response(status, payload, headers) {
  const headerMap = Object.assign({}, headers || {});
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: {
      get(name) {
        return Object.prototype.hasOwnProperty.call(headerMap, name)
          ? headerMap[name]
          : null;
      }
    },
    text() {
      return Promise.resolve(payload === null ? '' : JSON.stringify(payload));
    }
  };
}

async function run() {
  const requests = [];
  let responseFactory = () => response(200, {ok: true});

  const client = api.createClient({
    baseUrl: 'https://suite.example/',
    headers: {
      Authorization: 'Bearer test-token',
      'X-Correlation-ID': 'corr-client-1'
    },
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(responseFactory(url, options));
    }
  });

  responseFactory = () => response(200, {
    apiVersion: 'v1',
    links: {capabilities: '/api/v1/capabilities', backends: '/api/v1/backends'}
  });
  const root = await client.getApiRoot();
  assert.strictEqual(root.apiVersion, 'v1');
  assert.strictEqual(requests[0].url, 'https://suite.example/api/v1');

  responseFactory = () => response(200, {
    capabilities: [{id: 'public-api.backends-read', version: 1, availability: 'available'}]
  });
  const capabilities = await client.getCapabilities({cache: 'no-store'});
  assert.strictEqual(capabilities.capabilities[0].id, 'public-api.backends-read');
  assert.strictEqual(requests[1].url, 'https://suite.example/api/v1/capabilities');
  assert.strictEqual(requests[1].options.cache, 'no-store');

  responseFactory = () => response(200, {
    items: [{backendId: 'backend-a', name: 'Living Room', enabled: true, online: true}],
    page: {hasMore: false, nextCursor: null},
    meta: {partial: false}
  });
  const backends = await client.getBackends({
    query: {
      limit: 25,
      cursor: 'be1_after-a',
      sort: 'backendId',
      order: 'asc'
    }
  });
  assert.strictEqual(backends.items[0].backendId, 'backend-a');
  assert.strictEqual(
    requests[2].url,
    'https://suite.example/api/v1/backends?limit=25&cursor=be1_after-a&sort=backendId&order=asc'
  );
  assert.strictEqual(requests[2].options.method, 'GET');
  assert.strictEqual(requests[2].options.headers.Authorization, 'Bearer test-token');
  assert.strictEqual(requests[2].options.headers['X-Correlation-ID'], 'corr-client-1');

  const beforeInvalid = requests.length;
  assert.throws(
    () => client.getBackends({query: {offset: 1}}),
    /unsupported backend query field: offset/
  );
  assert.strictEqual(requests.length, beforeInvalid);

  responseFactory = () => response(503, {
    type: 'urn:vdr-suite:error:backend-unavailable',
    title: 'Backend unavailable',
    status: 503,
    detail: 'Backend discovery is unavailable.',
    code: 'backend_unavailable',
    requestId: 'req-public-client-1',
    correlationId: 'corr-public-client-1'
  });
  let failure = null;
  try {
    await client.getBackends();
  } catch (error) {
    failure = error;
  }
  assert.ok(failure);
  assert.strictEqual(api.isClientError(failure), true);
  assert.strictEqual(failure.status, 503);
  assert.strictEqual(failure.code, 'backend_unavailable');
  assert.strictEqual(failure.requestId, 'req-public-client-1');
  assert.strictEqual(failure.correlationId, 'corr-public-client-1');
  assert.strictEqual(requests.length, beforeInvalid + 1);

  assert.ok(!source.includes('/api/vdr/'));
  assert.ok(!source.includes('/api/security/browser-sessions'));
  assert.ok(!source.includes('/api/epg/'));
  assert.ok(!source.includes('requestJsonWithFallback'));
  assert.ok(!source.includes('retry'));

  console.log('test_public_v1_discovery_client passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
