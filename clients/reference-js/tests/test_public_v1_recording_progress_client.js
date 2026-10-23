'use strict';

const assert = require('assert').strict;
const {createClient} = require('../public-v1-client.js');

const recordingId = 'rec_' + 'a'.repeat(32);
const requests = [];
let currentRevision = 0;
let currentPosition = 0;
const client = createClient({
  baseUrl: 'https://suite.example',
  headers: {Authorization: 'VDR-Suite-Device credential-id.secret'},
  fetch: async (url, init) => {
    requests.push({url, init});
    const progressPath = 'https://suite.example/api/v1/recording-playback-sessions/progress';
    assert(url.startsWith(progressPath));
    if (init.method === 'GET') {
      assert.strictEqual(url, progressPath + '?backendId=default&recordingId=' + recordingId);
      return {ok: true, status: 200, text: async () => JSON.stringify({
        backendId: 'default', recordingId, positionSeconds: currentPosition,
        hasProgress: currentPosition > 0, revision: currentRevision,
        durationSeconds: 3000, resumeSupport: 'profile_dependent'
      })};
    }
    assert.strictEqual(init.method, 'POST');
    const body = JSON.parse(init.body);
    assert.strictEqual(body.recordingId, recordingId);
    assert.strictEqual(body.backendId, 'default');
    assert.deepStrictEqual(Object.keys(body).sort(),
      ['backendId','expectedRevision','operationId','positionSeconds','recordingId'].sort());
    if (body.expectedRevision !== currentRevision) {
      return {ok: false, status: 409, text: async () => JSON.stringify({
        error: {code: 'recording_progress_revision_conflict'},
        currentRevision
      })};
    }
    currentRevision++;
    currentPosition = body.positionSeconds;
    return {ok: true, status: 200, text: async () => JSON.stringify({
      backendId: 'default', recordingId, revision: currentRevision,
      hasProgress: currentPosition > 0, positionSeconds: currentPosition
    })};
  }
});
const identity = {backendId: 'default', recordingId};

(async () => {
  let state = await client.getRecordingProgress(identity);
  assert.strictEqual(state.hasProgress, false);
  assert.strictEqual(state.revision, 0);

  const written = await client.updateRecordingProgress({
    ...identity, positionSeconds: 2100, expectedRevision: 0,
    operationId: 'confirmed-session-1'
  });
  assert.strictEqual(written.revision, 1);
  state = await client.getRecordingProgress(identity);
  assert.strictEqual(state.positionSeconds, 2100);
  await assert.rejects(client.updateRecordingProgress({
    ...identity, positionSeconds: 110, expectedRevision: 0,
    operationId: 'delayed-session'
  }), error => error.status === 409);
  assert.strictEqual((await client.getRecordingProgress(identity)).positionSeconds, 2100);

  const reset = await client.clearRecordingProgress({
    ...identity, expectedRevision: 1, operationId: 'stop-and-clear'
  });
  assert.strictEqual(reset.hasProgress, false);
  assert.strictEqual(reset.revision, 2);
  assert.strictEqual((await client.getRecordingProgress(identity)).revision, 2);

  assert.throws(() => client.getRecordingProgress({backendId: 'default', recordingId: '../private'}));
  assert.throws(() => client.updateRecordingProgress({
    ...identity, positionSeconds: -5, expectedRevision: 2, operationId: 'bad'
  }));
  assert.throws(() => client.updateRecordingProgress({
    ...identity, positionSeconds: 25, expectedRevision: -1, operationId: 'bad'
  }));
  assert(requests.every(x => x.init.headers.Authorization.includes('VDR-Suite-Device')));
  assert(requests.every(x => x.url.indexOf('/api/media/') === -1));
  console.log('Public-v1 Recording progress client PASS');
})().catch(error => { console.error(error); process.exitCode = 1; });
