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

  const recId = 'rec_0123456789abcdef0123456789abcdef';
  const artId = 'fedcba9876543210fedcba9876543210';
  const artClient = api.createClient({
    baseUrl: 'https://suite.example',
    headers: {Authorization: 'Bearer device-secret'},
    fetch(url, options) {
      requests.push({url, options});
      if (url.endsWith('/artwork?backendId=backend-a')) {
        return Promise.resolve(response(200, {
          recordingId: recId, backendId: 'backend-a',
          items: [{kind: 'poster', artworkId: artId,
            url: '/api/v1/recordings/' + recId + '/artwork/' + artId +
              '?backendId=backend-a'}]
        }));
      }
      return Promise.resolve({
        ok: true, status: 200,
        headers: {get(name) { return name === 'Content-Type' ? 'image/png' : null; }},
        arrayBuffer() { return Promise.resolve(Uint8Array.from([137,80,78,71]).buffer); }
      });
    }
  });
  const artworks = await artClient.getRecordingArtwork({
    recordingId: recId, backendId: 'backend-a'
  });
  assert.strictEqual(artworks.items[0].artworkId, artId);
  const image = await artClient.getRecordingArtworkImage({
    recordingId: recId, artworkId: artId, backendId: 'backend-a'
  });
  assert.strictEqual(image.contentType, 'image/png');
  assert.deepStrictEqual(Array.from(new Uint8Array(image.bytes)), [137,80,78,71]);
  const artworkCalls = requests.slice(1);
  assert.strictEqual(artworkCalls.length, 2);
  assert.strictEqual(artworkCalls[0].options.headers.Authorization,
    'Bearer device-secret');
  assert.strictEqual(artworkCalls[1].options.headers.Authorization,
    'Bearer device-secret');
  assert.strictEqual(artworkCalls[1].options.headers.Accept,
    'image/png, image/jpeg, image/webp');
  assert(artworkCalls[1].url.endsWith('/artwork/' + artId +
    '?backendId=backend-a'));
  assert(!artworkCalls[1].url.includes('device-secret'));
  const epg = await artClient.getEpgArtworkImage({
    backendId: 'backend-a', channelId: 'S19.2E-1-100',
    eventId: '81234'
  });
  assert.strictEqual(epg.contentType, 'image/png');
  assert(requests[requests.length - 1].url.includes('/api/v1/epg/artwork?'));
  assert.strictEqual(requests[requests.length - 1].options.headers.Authorization,
    'Bearer device-secret');
  const preferred = await artClient.getEpgPreferredArtworkImage({
    backendId: 'backend-a', channelId: 'S19.2E-1-100',
    eventId: '81234'
  });
  assert.strictEqual(preferred.contentType, 'image/png');
  assert(requests[requests.length - 1].url.includes('/api/v1/epg/preferred-artwork?'));
  assert.strictEqual(requests[requests.length - 1].options.headers.Authorization,
    'Bearer device-secret');
  const priorImage = requests.length;
  for (const invalid of [
    {recordingId: 'rec_bad', backendId: 'backend-a'},
    {recordingId: recId, backendId: '../bad'},
    {recordingId: recId, backendId: 'backend-a', artworkId: '../bad'}
  ]) assert.throws(() => artClient.getRecordingArtworkImage(invalid));
  assert.strictEqual(requests.length, priorImage);

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
