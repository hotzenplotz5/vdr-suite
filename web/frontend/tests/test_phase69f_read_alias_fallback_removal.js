'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const clientPath = path.resolve(__dirname, '..', 'api', 'client-api.js');
const source = fs.readFileSync(clientPath, 'utf8');
const requests = [];

function response(status, payload) {
  return {
    ok: status >= 200 && status < 300,
    status,
    text() {
      return Promise.resolve(JSON.stringify(payload));
    }
  };
}

const context = {
  window: null,
  Headers,
  URLSearchParams,
  Promise,
  Object,
  Array,
  String,
  JSON,
  Error,
  Number,
  Boolean,
  fetch(url, options) {
    requests.push({url: String(url), options});
    return Promise.resolve(response(503, {
      error: {
        code: 'backend_unavailable',
        message: 'Backend unavailable',
        requestId: 'req-phase69f-read-alias'
      }
    }));
  }
};
context.window = context;

vm.createContext(context);
vm.runInContext(source, context, {filename: clientPath});

const api = context.VdrSuiteClientApi;
assert.ok(api);

async function captureFailure(call) {
  try {
    await call();
  } catch (error) {
    return error;
  }
  assert.fail('request unexpectedly succeeded');
}

async function verifyOneShot(functionName, expectedPath, options) {
  const before = requests.length;
  const failure = await captureFailure(function () {
    return api[functionName](options);
  });
  const emitted = requests.slice(before);

  assert.strictEqual(api.isClientError(failure), true);
  assert.strictEqual(failure.status, 503);
  assert.strictEqual(failure.code, 'backend_unavailable');
  assert.strictEqual(failure.requestId, 'req-phase69f-read-alias');
  assert.strictEqual(emitted.length, 1);

  const parsed = new URL(emitted[0].url, 'https://vdr-suite.test/');
  assert.strictEqual(parsed.pathname, expectedPath);
  assert.strictEqual(emitted[0].options.method, 'GET');

  return parsed;
}

async function run() {
  assert.strictEqual(
    (source.match(/return requestJsonWithFallback\(/g) || []).length,
    0
  );

  await verifyOneShot(
    'fetchClientVdrOverview',
    '/api/vdr/overview'
  );

  await verifyOneShot(
    'fetchClientPersons',
    '/api/vdr/persons'
  );

  const recordingPersonsUrl = await verifyOneShot(
    'fetchClientRecordingPersons',
    '/api/v1/search/recordings',
    {
      backendId: 'living-room',
      query: {
        query: 'Ada',
        limit: 12
      }
    }
  );
  assert.strictEqual(recordingPersonsUrl.searchParams.get('backendId'), 'living-room');
  assert.strictEqual(recordingPersonsUrl.searchParams.get('match'), 'person');
  assert.strictEqual(recordingPersonsUrl.searchParams.get('query'), 'Ada');
  assert.strictEqual(recordingPersonsUrl.searchParams.get('limit'), '12');

  assert.ok(!source.includes(
    "requestJsonWithFallback('/api/vdr/overview', '/api/vdr'"
  ));
  assert.ok(!source.includes(
    "'/api/persons'"
  ));
  assert.ok(!source.includes(
    "'/api/recordings/persons/search'"
  ));

  const publicId = 'rec_' + 'c'.repeat(32);
  context.fetch = (url, options) => {
    requests.push({url:String(url), options});
    return Promise.resolve(response(200, {items:[{kind:'recording', recordingId:publicId,
      backendId:'living-room', title:'Film', startTime:'1780000000',
      matchedPerson:'Ada', matchedRole:'actor', matchReason:'person',
      description:'Synopsis', preferredArtworkId:'d'.repeat(32)}],
      page:{offset:0,limit:20,totalCount:1,hasMore:false}}));
  };
  const found = await api.fetchClientRecordingPersons({backendId:'living-room', query:{name:'Ada'}});
  assert.strictEqual(found.matches.length,1);
  assert.strictEqual(found.matches[0].recording.recordingId,publicId);
  assert.strictEqual(found.matches[0].recording.recordedAt,'1780000000');
  assert.strictEqual(found.matches[0].recording.backendNativeId,undefined);
  assert.strictEqual(found.matches[0].recording.path,undefined);
  assert.strictEqual(found.matches[0].recording.description,'Synopsis');
  assert.strictEqual(found.matches[0].recording.preferredArtworkId,'d'.repeat(32));
  assert.strictEqual(found.matches[0].person.originalName,'Ada');
  console.log('test_phase69f_read_alias_fallback_removal passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
