'use strict';

const assert = require('assert');
const clientFactory = require('../public-v1-client.js');
const goodId = 'rec_0123456789abcdef0123456789abcdef';
const calls = [];
const client = clientFactory.createClient({
  fetch(url, request) {
    calls.push({url, request});
    return Promise.resolve({
      ok: true, status: request.method === 'POST' ? 202 : 200,
      headers: {get() { return null; }},
      text() { return Promise.resolve('{"ok":true}'); }
    });
  },
  headers: {'X-CSRF-Token': 'fake-test-csrf'},
  credentials: 'same-origin'
});

(async function () {
  const scope = {backendId: 'default', recordingId: goodId};
  await client.getRecordingMarks(scope);
  await client.getRecordingCutState(scope);
  const base = '/api/v1/recordings/' + goodId;
  assert.strictEqual(calls[0].url, base + '/marks?backendId=default');
  assert.strictEqual(calls[1].url, base + '/cut?backendId=default');
  assert.strictEqual(calls[0].request.method, 'GET');
  assert.strictEqual(calls[1].request.method, 'GET');
  assert.strictEqual(calls[0].request.credentials, 'same-origin');

  const common = Object.assign({}, scope, {
    operationId: 'marks-op-1',
    operationRevision: 'revision-1',
    expectedMarksRevision: '0123456789abcdef0123456789abcdef'
  });
  await client.submitRecordingMarksMutation(Object.assign({}, scope, {
    payload: Object.assign({}, common, {kind: 'add', targetFrame: 100})
  }));
  await client.submitRecordingCut(Object.assign({}, scope, {payload: common}));
  assert.strictEqual(calls[2].url, base + '/marks?backendId=default');
  assert.strictEqual(calls[3].url, base + '/cut?backendId=default');
  assert.strictEqual(calls[2].request.method, 'POST');
  assert.strictEqual(calls[3].request.method, 'POST');
  assert.strictEqual(calls[2].request.headers['X-CSRF-Token'], 'fake-test-csrf');
  assert.strictEqual(calls[2].request.headers['Content-Type'], 'application/json');
  assert.strictEqual(JSON.parse(calls[2].request.body).targetFrame, 100);
  assert.strictEqual(JSON.parse(calls[3].request.body).operationId, 'marks-op-1');

  const sent = calls.length;
  assert.throws(() => client.getRecordingMarks(
    {backendId:'default', recordingId: '7'}), /recordingId/i);
  assert.throws(() => client.getRecordingCutState(
    {backendId:'..', recordingId: goodId}), /backend/i);
  assert.throws(() => client.submitRecordingCut(Object.assign({}, scope, {
    payload: Object.assign({}, common, {recordingId: 'other'})
  })), /matching backend/i);
  assert.throws(() => client.submitRecordingMarksMutation(Object.assign({}, scope, {
    payload: Object.assign({}, common, {expectedMarksRevision: ''})
  })), /native operation revision/);
  assert.strictEqual(calls.length, sent, 'invalid input must not touch transport');
  console.log('API_V1_F6_RECORDING_EDIT_CLIENT=PASS');
})().catch(error => { console.error(error); process.exitCode = 1; });
