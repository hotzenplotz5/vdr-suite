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
  if (path === '/api/v1/recording-playback-sessions/capabilities' &&
      init.method === 'GET') {
    assert.deepStrictEqual(Object.fromEntries(new URL(url).searchParams.entries()), {
      appVersion:'b20261011.24', firmwareVersion:'vidaa9',
      playerVersion:'mse_v1', engine:'mse'
    });
    assert.strictEqual(new URL(url).searchParams.has('deviceId'),false,
      'Device identity must come from credential, not query');
    return response(200,{availability:'available'});
  }
  if (path === '/api/v1/recording-playback-sessions' && init.method === 'POST') {
    const input = JSON.parse(init.body);
    const native = input.nativeMediaAccess === true;
    assert.deepStrictEqual(input, native ? {
      backendId:'default', recordingId, capabilities,
      audioTrackId:'audio-2', nativeMediaAccess:true
    } : {
      backendId:'default', recordingId, capabilities, startPositionSeconds:2100
    });
    return response(201, Object.assign({mediaSession:{
      id:sessionId, backendId:'default', recordingId, state:'ready',
      presentationProfileId:'hls-ts', mediaPath:'/vdr-suite/api/v1/media/sessions/'+sessionId+'/hls/index.m3u8',
      playback:{durationSeconds:3000, resume:{supported:true,preparing:false}}
    }}, native ? {nativeMediaAccess:{
      transport:'http-cookie', cookie:'vdr_suite_media=fixture-only',
      headerName:'X-VDR-Suite-Media-Authorization', headerValue:'Bearer fixture-only'
    }} : {}));
  }
  if (path === '/api/v1/recording-playback-sessions/'+sessionId && init.method === 'GET')
    return response(200,{mediaSession:{id:sessionId,state:'ready',backendId:'default',
      playback:{resume:{supported:true,preparing:false},durationSeconds:3000}}});
  if (path === '/api/v1/recording-playback-sessions/'+sessionId+'/tracks' &&
      init.method === 'GET')
    return response(200,{mediaSession:{id:sessionId,backendId:'default',
      state:'ready',tracks:{audio:[],subtitles:[]}}});
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
  assert.strictEqual(Object.prototype.hasOwnProperty.call(session,'nativeMediaAccess'),false,
    'default Device session must not opt into bearer projection');

  const nativeSession = await client.createRecordingPlaybackSession({
    ...identity, capabilities, audioTrackId:'audio-2', nativeMediaAccess:true
  });
  assert.strictEqual(nativeSession.nativeMediaAccess.transport,'http-cookie');
  assert.strictEqual(nativeSession.nativeMediaAccess.headerName,
    'X-VDR-Suite-Media-Authorization');
  const tracks = await client.getRecordingPlaybackTracks({sessionId});
  assert.strictEqual(tracks.mediaSession.id,sessionId);
  assert.deepStrictEqual(tracks.mediaSession.tracks.audio,[]);
  assert.strictEqual(requests.length,5,'opt-in CREATE and tracks are single requests');

  assert.throws(() => client.createRecordingPlaybackSession({...identity,
    capabilities, startPositionSeconds:-1}));
  assert.throws(() => client.createRecordingPlaybackSession({...identity,
    startPositionSeconds:2100}));
  assert.throws(() => client.createRecordingPlaybackSession({...identity,
    capabilities:{...capabilities, videoCodecs:['made-up-codec']}}));
  assert.throws(() => client.getRecordingPlaybackSession({sessionId:'../foreign'}));
  assert.throws(() => client.stopRecordingPlaybackSession({sessionId,backendId:'../invalid'}));
  assert.throws(() => client.getRecordingPlaybackTracks({sessionId:'../foreign'}));
  assert.throws(() => client.createRecordingPlaybackSession({
    ...identity, capabilities, nativeMediaAccess:'true'
  }));
  assert.throws(() => client.createRecordingPlaybackSession({
    ...identity, capabilities, audioTrackId:'../native'
  }));
  assert.strictEqual(requests.length,5,'invalid Device options must not dispatch');
  const profile = await client.getRecordingPlaybackCapabilities({
    appVersion:'b20261011.24', firmwareVersion:'vidaa9',
    playerVersion:'mse_v1', engine:'mse'
  });
  assert.strictEqual(profile.availability,'available');
  assert.throws(() => client.getRecordingPlaybackCapabilities({
    appVersion:'b20261011.24', engine:'mse'
  }));
  assert.strictEqual(requests.length,6,'profile check is exactly one read');
  assert(requests.every(x => x.headers.Authorization ===
    'VDR-Suite-Device demo.credential'));
  const browserRequests = [];
  const browser = createClient({baseUrl:'https://example.invalid',credentials:'same-origin',
    fetch: async (url, init) => {
      browserRequests.push({url, ...init});
      const body = JSON.parse(init.body);
      if (body.operation === 'select-subtitle-track') {
        const off = body.subtitleTrackId === 'off';
        return {
          ok:true,status:200,
          headers:{get: () => off ? 'application/json' : 'text/vtt; charset=utf-8'},
          text:async () => off
            ? JSON.stringify({mediaSession:{id:sessionId,subtitleTrackId:'off'}})
            : 'WEBVTT\n\n00:00:01.000 --> 00:00:02.000\nTest'
        };
      }
      return response(201,{mediaSession:{id:sessionId,recordingId}});
    }});
  await browser.createBrowserRecordingPlaybackSession({
    ...identity, capabilities, startPositionSeconds:2100, audioTrackId:'audio-2',
    headers:{'X-CSRF-Token':'browser-fixture'}
  });
  assert.strictEqual(browserRequests.length,1);
  assert.strictEqual(new URL(browserRequests[0].url).pathname,'/api/v1/browser/recording-playback-sessions');
  assert.strictEqual(browserRequests[0].credentials,'same-origin');
  assert.strictEqual(browserRequests[0].headers['X-CSRF-Token'],'browser-fixture');
  assert.deepStrictEqual(JSON.parse(browserRequests[0].body),
    {...identity,capabilities,startPositionSeconds:2100,audioTrackId:'audio-2'});
  assert.throws(() => browser.createBrowserRecordingPlaybackSession({
    ...identity,capabilities,nativeMediaAccess:true
  }));
  assert.strictEqual(browserRequests.length,1);
  const browserControl = {backendId:identity.backendId, sessionId,
    headers:{'X-CSRF-Token':'browser-fixture'}};
  await browser.seekBrowserRecordingPlaybackSession({...browserControl,positionSeconds:42});
  await browser.getBrowserRecordingPlaybackStatus(browserControl);
  await browser.stopBrowserRecordingPlaybackSession(browserControl);
  assert.strictEqual(browserRequests.length,4);
  const controls = browserRequests.slice(1);
  assert(controls.every(entry => new URL(entry.url).pathname ===
    '/api/v1/browser/recording-playback-sessions/control'));
  assert(controls.every(entry => entry.credentials === 'same-origin' &&
    entry.headers['X-CSRF-Token'] === 'browser-fixture'));
  assert.deepStrictEqual(controls.map(entry => JSON.parse(entry.body)),[
    {operation:'seek',backendId:identity.backendId,sessionId,positionSeconds:42},
    {operation:'playback-status',backendId:identity.backendId,sessionId},
    {operation:'stop',backendId:identity.backendId,sessionId}
  ]);
  assert.throws(() => browser.seekBrowserRecordingPlaybackSession({...browserControl,positionSeconds:-1}));
  assert.strictEqual(browserRequests.length,4);
  await browser.getBrowserRecordingPlaybackTracks(browserControl);
  await browser.selectBrowserRecordingAudioTrack({...browserControl,
    audioTrackId:'audio-2',positionSeconds:42,capabilities});
  const subtitle = await browser.selectBrowserRecordingSubtitle({...browserControl,
    subtitleTrackId:'subtitle-1',streamBasePositionSeconds:0});
  assert(subtitle.text.startsWith('WEBVTT'));
  const off = await browser.selectBrowserRecordingSubtitle({...browserControl,
    subtitleTrackId:'off',streamBasePositionSeconds:0});
  assert.strictEqual(off.data.mediaSession.subtitleTrackId,'off');
  const trackRequests = browserRequests.slice(4);
  assert.strictEqual(trackRequests.length,4);
  assert(trackRequests.every(entry => new URL(entry.url).pathname === '/api/v1/browser/recording-playback-sessions/control'));
  assert(trackRequests.every(entry => entry.credentials === 'same-origin' &&
    entry.headers['X-CSRF-Token'] === 'browser-fixture'));
  assert.deepStrictEqual(trackRequests.map(entry => JSON.parse(entry.body)),[
    {operation:'track-status',backendId:identity.backendId,sessionId},
    {operation:'select-audio-track',backendId:identity.backendId,sessionId,
      positionSeconds:42,audioTrackId:'audio-2',capabilities},
    {operation:'select-subtitle-track',backendId:identity.backendId,sessionId,
      subtitleTrackId:'subtitle-1',streamBasePositionSeconds:0},
    {operation:'select-subtitle-track',backendId:identity.backendId,sessionId,
      subtitleTrackId:'off',streamBasePositionSeconds:0}
  ]);
  const browserHistory = {...identity,headers:{'X-CSRF-Token':'browser-fixture'}};
  await browser.getBrowserRecordingProgress(browserHistory);
  await browser.updateBrowserRecordingProgress({...browserHistory,
    positionSeconds:42,expectedRevision:2,operationId:'progress-fixture',
    resumeSupported:true,notifyLive:true});
  await browser.clearBrowserRecordingProgress({...browserHistory,
    expectedRevision:3,operationId:'clear-fixture'});
  await browser.recordBrowserRecordingActivity({...browserHistory,
    positionSeconds:42,operationId:'activity-fixture',resumeSupportKnown:true,
    resumeSupported:true,ended:false});
  const historyRequests = browserRequests.slice(8);
  assert.deepStrictEqual(historyRequests.map(entry=>new URL(entry.url).pathname),
    ['/api/v1/browser/recording-playback-sessions/progress','/api/v1/browser/recording-playback-sessions/progress','/api/v1/browser/recording-playback-sessions/progress','/api/v1/browser/recording-playback-sessions/activity']);
  assert(historyRequests.every(entry=>entry.credentials==='same-origin' &&
    entry.headers['X-CSRF-Token']==='browser-fixture'));
  assert.deepStrictEqual(historyRequests.map(entry=>JSON.parse(entry.body)),[
    {operation:'revision',...identity},
    {operation:'progress',...identity,operationId:'progress-fixture',expectedRevision:2,
      positionSeconds:42,resumeSupported:true,notifyLive:true},
    {operation:'clear',...identity,operationId:'clear-fixture',expectedRevision:3},
    {operation:'activity',...identity,operationId:'activity-fixture',positionSeconds:42,
      resumeSupportKnown:true,resumeSupported:true,ended:false}
  ]);
  assert.throws(()=>browser.updateBrowserRecordingProgress({...browserHistory,
    positionSeconds:42,operationId:'missing-revision'}));
  assert.strictEqual(browserRequests.length,12);
  await browser.getBrowserContinueWatching(browserHistory);
  await browser.getBrowserRecentlyWatched(browserHistory);
  const browserLists = browserRequests.slice(12);
  assert.deepStrictEqual(browserLists.map(entry=>new URL(entry.url).pathname),
    ['/api/v1/browser/recording-playback-sessions/progress', '/api/v1/browser/recording-playback-sessions/activity']);
  assert(browserLists.every(entry=>JSON.parse(entry.body).operation==='list' &&
    JSON.parse(entry.body).backendId===identity.backendId &&
    !Object.prototype.hasOwnProperty.call(JSON.parse(entry.body),'recordingId')));
  console.log('Public-v1 recording resume/native-media/tracks and Browser CREATE/control/tracks/history client PASS');
})().catch(error => {console.error(error); process.exitCode=1;});
