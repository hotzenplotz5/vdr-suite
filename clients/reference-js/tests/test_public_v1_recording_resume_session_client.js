'use strict';

const assert = require('assert').strict;
const {createClient} = require('../public-v1-client.js');

const recordingId = 'rec_' + 'a'.repeat(32);
const sessionId = 'ms_' + 'b'.repeat(32);
const capabilities = Object.freeze({
  protocols: ['hls'], containers: ['mpeg-ts'],
  videoCodecs: ['h264'], audioCodecs: ['aac'],
  supportsByteRanges: true, maxVideoWidth: 1920,
  maxVideoHeight: 1080, maxAudioChannels: 2
});
const requests = [];
function response(status, data) {
  return {ok: status >= 200 && status < 300, status,
    text: async () => JSON.stringify(data)};
}
const fetchMock = async (url, init) => {
  requests.push({url, ...init});
  const path = new URL(url).pathname;
  assert(!path.startsWith('/api/media/'));
  assert.strictEqual(init.headers.Authorization, 'VDR-Suite-Device demo.credential');
  if (path === '/api/v1/recording-playback-sessions' && init.method === 'POST') {
    const input = JSON.parse(init.body);
    assert.deepStrictEqual(input, {
      backendId: 'default', recordingId, capabilities, startPositionSeconds: 2100
    });
    return response(201, {mediaSession:{
      id: sessionId, backendId:'default', recordingId, state:'ready',
      presentationProfileId:'hls-ts', mediaPath:'/vdr-suite/api/v1/media/sessions/'+sessionId+'/hls/index.m3u8',
      playback:{durationSeconds:3000, resume:{supported:true,preparing:false}}
    }});
  }
  if (path === '/api/v1/recording-playback-sessions/'+sessionId && init.method === 'GET')
    return response(200,{mediaSession:{id:sessionId,state:'ready',backendId:'default',
      playback:{resume:{supported:true,preparing:false},durationSeconds:3000}}});
  if (path === '/api/v1/recording-playback-sessions/'+sessionId+'/stop' &&
      init.method === 'POST') {
    assert.deepStrictEqual(JSON.parse(init.body), {
      operation:'stop',backendId:'default',sessionId
    });
    return response(200,{stopped:true});
  }
  throw new Error('Unexpected request '+init.method+' '+path);
};
const client = createClient({baseUrl:'https://example.invalid',headers:{
  Authorization:'VDR-Suite-Device demo.credential'
},fetch:fetchMock});
(async () => {
  const identity = {backendId:'default',recordingId};
  // The player, not the API helper, must explicitly decide to request
  // the measured start position and advertise real capabilities.
  const session = await client.createRecordingPlaybackSession({
    ...identity, capabilities, startPositionSeconds:2100
  });
  assert.strictEqual(session.mediaSession.id, sessionId);
  assert.strictEqual(session.mediaSession.playback.resume.supported, true);
  const status = await client.getRecordingPlaybackSession({sessionId});
  assert.strictEqual(status.mediaSession.id,sessionId);
  const stopped = await client.stopRecordingPlaybackSession({sessionId,backendId:'default'});
  assert.strictEqual(stopped.stopped,true);
  assert.strictEqual(requests.length,3);

  assert.throws(() => client.createRecordingPlaybackSession({...identity,
    capabilities, startPositionSeconds:-1}));
  assert.throws(() => client.createRecordingPlaybackSession({...identity,
    startPositionSeconds:2100}));
  assert.throws(() => client.createRecordingPlaybackSession({...identity,
    capabilities:{...capabilities, videoCodecs:['made-up-codec']}}));
  assert.throws(() => client.getRecordingPlaybackSession({sessionId:'../foreign'}));
  assert.throws(() => client.stopRecordingPlaybackSession({sessionId,backendId:'../invalid'}));
  assert.strictEqual(requests.length,3);
  console.log('Public-v1 recording resume playback client PASS');
})().catch(error => {console.error(error); process.exitCode=1;});
