'use strict';

const assert = require('assert').strict;
const {createClient} = require('../public-v1-client.js');

const recordingId = 'rec_' + 'a'.repeat(32);
const sessionId = 'ms_' + 'b'.repeat(32);
const profile = {
  appVersion: 'b20261011.24', firmwareVersion: 'vidaa9',
  playerVersion: 'native_v1', engine: 'native'
};
const capabilities = {
  protocols: ['hls'], containers: ['mpeg-ts'],
  videoCodecs: ['h264'], audioCodecs: ['aac'],
  supportsByteRanges: true, maxVideoWidth: 1920,
  maxVideoHeight: 1080, maxAudioChannels: 2
};
const requests = [];
let revision = 3;
let positionSeconds = 90;
let activeSession = false;
let activeLiveSession = false;
const liveCapabilities = {...capabilities,protocols:['progressive'],containers:['fmp4']};
const result = (status, body) => ({
  ok: status >= 200 && status < 300,
  status, text: async () => JSON.stringify(body)
});
const client = createClient({
  baseUrl: 'https://suite.example',
  headers: {Authorization: 'VDR-Suite-Device fixture.credential'},
  fetch: async (url, init) => {
    const requestUrl = new URL(url);
    const path = requestUrl.pathname;
    requests.push({url, method: init.method, body: init.body});
    assert.strictEqual(requestUrl.origin, 'https://suite.example');
    assert.strictEqual(init.headers.Authorization,
      'VDR-Suite-Device fixture.credential');
    assert(path.startsWith('/api/v1/'));
    assert(!url.includes('fixture-only'), 'session media bearer is never sent in URL');

    if (path.endsWith('/capabilities') && init.method === 'GET') {
      assert.deepStrictEqual(Object.fromEntries(requestUrl.searchParams), profile);
      assert(!requestUrl.searchParams.has('deviceId'));
      return result(200, {availability: 'available'});
    }
    if (path.endsWith('/progress') && init.method === 'GET') {
      assert.deepStrictEqual(Object.fromEntries(requestUrl.searchParams), {
        backendId: 'default', recordingId
      });
      return result(200, {
        backendId: 'default', recordingId, revision,
        hasProgress: positionSeconds > 0, positionSeconds,
        durationSeconds: 3600, resumeSupport: 'profile_dependent'
      });
    }
    if (path === '/api/v1/recording-playback-sessions' && init.method === 'POST') {
      assert.strictEqual(activeSession, false, 'CREATE is user-triggered once');
      assert.deepStrictEqual(JSON.parse(init.body), {
        backendId: 'default', recordingId, capabilities,
        startPositionSeconds: 90, capabilityProfile: profile,
        nativeMediaAccess: true, audioTrackId: 'audio-2'
      });
      activeSession = true;
      return result(201, {
        mediaSession: {
          id: sessionId, backendId: 'default', recordingId, state: 'ready',
          mediaPath: '/vdr-suite/api/v1/media/sessions/' + sessionId + '/hls/master.m3u8',
          playback: {durationSeconds: 3600, resume: {supported: true, preparing: false}}
        },
        nativeMediaAccess: {
          transport: 'http-cookie', cookie: 'vdr_suite_media=fixture-only',
          headerName: 'X-VDR-Suite-Media-Authorization',
          headerValue: 'Bearer fixture-only'
        }
      });
    }
    if (path.endsWith('/tracks') && init.method === 'GET') {
      assert.strictEqual(path, '/api/v1/recording-playback-sessions/' + sessionId + '/tracks');
      assert.strictEqual(activeSession, true);
      return result(200, {
        mediaSession: {id: sessionId, backendId: 'default', state: 'ready',
          tracks: {audio: [{id: 'audio-2', language: 'de'}], subtitles: []}}
      });
    }
    if (path.endsWith('/progress') && init.method === 'POST') {
      assert.strictEqual(activeSession, true);
      const body = JSON.parse(init.body);
      assert.strictEqual(body.backendId, 'default');
      assert.strictEqual(body.recordingId, recordingId);
      assert.strictEqual(body.sessionId, sessionId);
      assert.strictEqual(body.operationId === 'confirmed-150' ||
        body.operationId === 'stale-140', true);
      if (body.expectedRevision !== revision)
        return result(409, {code: 'recording_progress_revision_conflict',
          currentRevision: revision});
      assert.strictEqual(body.operationId, 'confirmed-150');
      assert.strictEqual(body.positionSeconds, 150);
      positionSeconds = body.positionSeconds;
      revision += 1;
      return result(200, {recordingId, backendId:'default',
        positionSeconds, revision, hasProgress: true});
    }
    if (path.endsWith('/continue-watching') && init.method === 'GET') {
      assert.strictEqual(requestUrl.searchParams.get('backendId'), 'default');
      return result(200, {backendId: 'default', limit: 50,
        items: [{recordingId, positionSeconds, durationSeconds: 3600,
          resumeSupport: 'profile_dependent'}]});
    }
    if (path.endsWith('/recently-watched') && init.method === 'GET') {
      assert.strictEqual(requestUrl.searchParams.get('backendId'),'default');
      return result(200,{backendId:'default',limit:100,items:[{recordingId,positionSeconds}]});
    }
    if (path.endsWith('/trickplay') && init.method === 'GET') {
      assert.strictEqual(activeSession,true);
      assert.strictEqual(requestUrl.searchParams.get('seconds'),'153');
      assert.strictEqual(init.headers.Accept,'image/jpeg');
      return {...result(200,{}),headers:{get(name){return name === 'Content-Type' ?
        'image/jpeg' : name === 'X-Trickplay-Sample-Seconds' ? '150' : null;}},
        arrayBuffer:async()=>Uint8Array.from([255,216,255]).buffer};
    }
    if (path.endsWith('/subtitles') && init.method === 'POST') {
      assert.strictEqual(activeSession,true);
      const body=JSON.parse(init.body);
      assert.deepStrictEqual(body,{operation:'select-subtitle-track',backendId:'default',
        sessionId,subtitleTrackId:body.subtitleTrackId,streamBasePositionSeconds:90});
      if (body.subtitleTrackId==='off')
        return {...result(200,{subtitleTrackId:'off'}),headers:{get(){return 'application/json';}}};
      assert.strictEqual(body.subtitleTrackId,'subtitle-1');
      return {ok:true,status:200,headers:{get(){return 'text/vtt; charset=utf-8';}},
        text:async()=> 'WEBVTT\n\n'};
    }
    if (path === '/api/v1/live-playback-sessions' && init.method === 'POST') {
      assert.strictEqual(activeLiveSession,false);
      assert.deepStrictEqual(JSON.parse(init.body),{resourceKind:'live-channel',
        backendId:'default',channelId:'S19.2E-1-1019-10301',capabilities:liveCapabilities});
      activeLiveSession=true;
      return result(201,{mediaSession:{id:sessionId,state:'ready'}});
    }
    if (path === '/api/v1/live-playback-sessions/' + sessionId && init.method === 'GET') {
      assert.strictEqual(activeLiveSession,true);
      return result(200,{mediaSession:{id:sessionId,state:'ready'}});
    }
    if (path === '/api/v1/live-playback-sessions/' + sessionId + '/stop' && init.method === 'POST') {
      assert.deepStrictEqual(JSON.parse(init.body),{operation:'stop',resourceKind:'live-channel',backendId:'default',sessionId});
      activeLiveSession=false;
      return result(200,{stopped:true});
    }
    if (path.endsWith('/stop') && init.method === 'POST') {
      assert.strictEqual(path,
        '/api/v1/recording-playback-sessions/' + sessionId + '/stop');
      assert.strictEqual(activeSession, true);
      assert.deepStrictEqual(JSON.parse(init.body), {
        operation:'stop', backendId:'default', sessionId
      });
      activeSession = false;
      return result(200, {stopped:true});
    }
    throw Error('Unexpected request ' + init.method + ' ' + url);
  }
});

(async () => {
  const identity = {backendId: 'default', recordingId};
  const ready = await client.getRecordingPlaybackCapabilities(profile);
  assert.strictEqual(ready.availability, 'available');
  const previous = await client.getRecordingProgress(identity);
  assert.strictEqual(previous.revision, 3);
  assert.strictEqual(previous.positionSeconds, 90);
  assert.strictEqual(requests.filter(r => r.method === 'POST').length, 0,
    'profile/read-only progress must not start playback');

  const created = await client.createRecordingPlaybackSession({
    ...identity, capabilities, capabilityProfile: profile,
    nativeMediaAccess: true, audioTrackId: 'audio-2',
    startPositionSeconds: previous.positionSeconds
  });
  assert.strictEqual(created.mediaSession.playback.resume.supported, true);
  assert.strictEqual(created.nativeMediaAccess.transport, 'http-cookie');
  assert(!created.mediaSession.mediaPath.includes('fixture-only'));
  const tracks = await client.getRecordingPlaybackTracks({sessionId});
  assert.strictEqual(tracks.mediaSession.tracks.audio[0].id, 'audio-2');

  const saved = await client.updateRecordingProgress({
    ...identity, sessionId, positionSeconds: 150,
    expectedRevision: previous.revision, operationId: 'confirmed-150'
  });
  assert.strictEqual(saved.revision, 4);
  await assert.rejects(client.updateRecordingProgress({
    ...identity, sessionId, positionSeconds: 140,
    expectedRevision: previous.revision, operationId: 'stale-140'
  }), error => error.status === 409);
  const fresh = await client.getRecordingProgress(identity);
  assert.strictEqual(fresh.positionSeconds, 150);
  assert.strictEqual(fresh.revision, 4);
  const rail = await client.getContinueWatching({backendId:'default'});
  assert.strictEqual(rail.items[0].recordingId, recordingId);
  assert.strictEqual(rail.items[0].positionSeconds, 150);

  const history = await client.getRecentlyWatched({backendId:'default'});
  assert.strictEqual(history.items[0].recordingId,recordingId);
  const thumb = await client.getRecordingTrickplay({sessionId,seconds:153});
  assert.strictEqual(thumb.sampleSeconds,150);
  assert.strictEqual(thumb.bytes.byteLength,3);
  const sub = {sessionId,backendId:'default',streamBasePositionSeconds:90};
  assert.strictEqual((await client.selectRecordingSubtitle({...sub,subtitleTrackId:'subtitle-1'})).text,'WEBVTT\n\n');
  assert.strictEqual((await client.selectRecordingSubtitle({...sub,subtitleTrackId:'off'})).data.subtitleTrackId,'off');
  await client.stopRecordingPlaybackSession({sessionId, backendId:'default'});
  assert.strictEqual(activeSession, false);
  const live = await client.createLivePlaybackSession({backendId:'default',
    channelId:'S19.2E-1-1019-10301',capabilities:liveCapabilities});
  assert.strictEqual(live.mediaSession.id,sessionId);
  assert.strictEqual((await client.getLivePlaybackSession({sessionId})).mediaSession.state,'ready');
  await client.stopLivePlaybackSession({sessionId,backendId:'default'});
  assert.strictEqual(activeLiveSession,false);
  assert.strictEqual(requests.length, 16);
  assert.strictEqual(requests.filter(r =>
    r.method === 'POST' && new URL(r.url).pathname ===
      '/api/v1/recording-playback-sessions').length, 1,
    'stale progress must not trigger a new playback session');
  assert.strictEqual(requests.filter(r =>
    r.method === 'POST' && new URL(r.url).pathname.endsWith('/progress')).length, 2,
    '409 is passed to caller without a blind retry');
  console.log('Public-v1 F7 Device profile -> resume -> tracks -> progress -> stop PASS');
})().catch(error => {console.error(error); process.exitCode = 1;});
