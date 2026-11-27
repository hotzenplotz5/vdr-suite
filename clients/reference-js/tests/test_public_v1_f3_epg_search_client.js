'use strict';

const assert = require('assert');
const path = require('path');
const api = require(path.resolve(__dirname, '..', 'public-v1-client.js'));

function response(status, body) {
  return {
    ok: status >= 200 && status < 300,
    status,
    headers: {get() { return null; }},
    text() { return Promise.resolve(JSON.stringify(body)); }
  };
}

async function run() {
  const calls = [];
  const replies = [];
  const base = 'https://suite.example';
  const credential = 'VDR-Suite-Device dv_test.opaque-secret';
  const client = api.createClient({
    baseUrl: base,
    headers: {Authorization: credential},
    fetch(url, options) {
      calls.push({url, options});
      if (!replies.length) throw new Error('Unexpected transport request');
      return Promise.resolve(replies.shift());
    }
  });

  replies.push(response(200, {items: [
    {backendId: 'home-vdr', name: 'Wohnzimmer', enabled: true, online: true}
  ], page: {limit: 50, hasMore: false, nextCursor: null}, meta: {partial: false}}));
  const backends = await client.getBackends();
  assert.equal(backends.items[0].backendId, 'home-vdr');
  assert.equal(calls[0].url, base + '/api/v1/backends');

  replies.push(response(200, {
    items: [
      {backendId: 'home-vdr', channelId: 'S19.2E-1-1019-10301',
        channelNumber: 1, name: 'Das Erste', enabled: true},
      {backendId: 'home-vdr', channelId: 'S19.2E-1-1101-28106',
        channelNumber: 2, name: 'ZDF', enabled: true}
    ],
    page: {limit: 2, hasMore: true, nextCursor: 'ch2_continuation'},
    meta: {partial: false, sources: [{backendId: 'home-vdr', state: 'ok'}]}
  }));
  const channels = await client.getChannels({
    query: {backendIds: ['home-vdr'], limit: 2,
      sort: 'backendId,channelNumber,channelId', order: 'asc'}
  });
  assert.equal(channels.items[1].name, 'ZDF');
  assert.equal(channels.page.nextCursor, 'ch2_continuation');
  assert.equal(calls[1].url, base +
    '/api/v1/channels?backendId=home-vdr&limit=2&sort=backendId,channelNumber,channelId&order=asc');

  replies.push(response(200, {items: [
    {backendId:'home-vdr', channelId:'S19.2E-1-1101-28106',
      channelNumber:2, name:'ZDF'}
  ], page: {limit:1,hasMore:false,nextCursor:null},meta:{partial:false}}));
  const neighbor = await client.getAdjacentChannel({
    query: {backendId:'home-vdr', relativeTo:'S19.2E-1-1019-10301',
      direction:'up'}
  });
  assert.equal(neighbor.items[0].name, 'ZDF');
  assert.equal(calls[2].url, base +
    '/api/v1/channels?backendId=home-vdr&limit=1&relativeTo=S19.2E-1-1019-10301&direction=up');

  const start = '1763733600';
  const until = '1763906400';
  const firstEvents = {
    items: [
      {backendId:'home-vdr',channelId:'S19.2E-1-1019-10301',
        title:'Tagesschau',startTime:start,eventId:'123'},
      {backendId:'home-vdr',channelId:'S19.2E-1-1019-10301',
        title:'Spielfilm',startTime:'1763740800',eventId:'124'}
    ],
    page:{limit:2,hasMore:true,nextCursor:'ew1_next'},meta:{partial:false}
  };
  replies.push(response(200, firstEvents));
  const window1 = await client.getEpgWindow({
    query:{backendId:'home-vdr',channelId:'S19.2E-1-1019-10301',
      fromTime:start,untilTime:until,limit:2},cache:'no-store'
  });
  assert.deepEqual(window1, firstEvents);
  assert.equal(calls[3].url,base+
    '/api/v1/epg/window?backendId=home-vdr&channelId=S19.2E-1-1019-10301&fromTime='+start+'&untilTime='+until+'&limit=2');
  assert.equal(calls[3].options.cache,'no-store');

  replies.push(response(200, {items:[{title:'Nachrichten'}],
    page:{limit:2,hasMore:false,nextCursor:null},meta:{partial:false}}));
  const window2 = await client.getEpgWindow({
    query:{backendId:'home-vdr',channelId:'S19.2E-1-1019-10301',
      fromTime:start,untilTime:until,limit:2,cursor:window1.page.nextCursor}
  });
  assert.equal(window2.page.hasMore,false);
  assert.equal(calls[4].url, calls[3].url+'&cursor=ew1_next');

  replies.push(response(200, {items:[{title:'Jetzt'},{title:'Danach'}],
    page:{limit:2,hasMore:false,nextCursor:null}}));
  const nowNext = await client.getEpgNowNext({
    query:{backendId:'home-vdr',channelId:'S19.2E-1-1019-10301',fromTime:start,limit:2}
  });
  assert.equal(nowNext.items.length,2);
  assert.equal(calls[5].url,base+
    '/api/v1/epg/now-next?backendId=home-vdr&channelId=S19.2E-1-1019-10301&fromTime='+start+'&limit=2');

  const epgResult = {items:[
    {kind:'epg',backendId:'home-vdr',title:'Tatort München',subtitle:'Folge',
      channelId:'S19.2E-1-1019-10301',eventId:'139',
      startTime:start,endTime:'1763740000'}
  ],page:{offset:0,limit:30,totalCount:3,hasMore:true}};
  replies.push(response(200, epgResult));
  const foundEpg = await client.searchEpg({
    query:{backendId:'home-vdr',query:'Tatort München',limit:30,offset:0}
  });
  assert.deepEqual(foundEpg,epgResult);
  assert.equal(calls[6].url,base+
    '/api/v1/search/epg?backendId=home-vdr&query=Tatort%20M%C3%BCnchen&limit=30&offset=0');

  const recordingId='rec_0123456789abcdef0123456789abcdef';
  const recordingResult={items:[{kind:'recording',backendId:'home-vdr',
    title:'Café & Urlaub',recordingId}],
    page:{offset:30,limit:30,totalCount:31,hasMore:false}};
  replies.push(response(200,recordingResult));
  const foundRec=await client.searchRecordings({
    query:{backendId:'home-vdr',query:'Café & Urlaub',offset:30}
  });
  assert.deepEqual(foundRec,recordingResult);
  assert.equal(foundRec.items[0].recordingId,recordingId);
  assert.equal(calls[7].url,base+
    '/api/v1/search/recordings?backendId=home-vdr&query=Caf%C3%A9%20%26%20Urlaub&offset=30');

  const eventQuery = {backendId:'home-vdr',channelId:'S19.2E-1-1019-10301',eventId:'139'};
  const tag = '"epg1:sha256:example"';
  replies.push({...response(200, { ...eventQuery, title:'Tatort' }),
    headers:{get(name){return name.toLowerCase() === 'etag' ? tag : null;}}});
  const detail = await client.getEpgEvent({query:eventQuery});
  assert.equal(detail.status, 200);
  assert.equal(detail.etag, tag);
  assert.equal(detail.data.eventId, '139');
  assert.equal(calls.at(-1).url,base+'/api/v1/epg/event?backendId=home-vdr&channelId=S19.2E-1-1019-10301&eventId=139');
  replies.push({status:304,ok:false,headers:{get(){return tag;}}});
  assert.deepEqual(await client.getEpgEvent({query:eventQuery,ifNoneMatch:tag}),
    {status:304,etag:tag,data:null});
  assert.equal(calls.at(-1).options.headers['If-None-Match'],tag);
  const genres = {items:[{genreId:'adventure',contentClass:'movie',label:'Abenteuer',count:31}],
    page:{offset:0,limit:30,totalCount:31,hasMore:true},meta:{partial:false}};
  replies.push(response(200,genres));
  assert.deepEqual(await client.getEpgGenres({query:{backendId:'home-vdr',limit:30}}),genres);
  assert.equal(calls.at(-1).url,base+'/api/v1/genres/epg?backendId=home-vdr&limit=30');
  const genreEvents={items:[{kind:'epg',...eventQuery,title:'Tatort'}],
    page:{offset:30,limit:30,totalCount:31,hasMore:false},meta:{partial:false}};
  replies.push(response(200,genreEvents));
  assert.deepEqual(await client.getEpgGenreEvents({query:{backendId:'home-vdr',contentClass:'movie',genreId:'adventure',offset:30}}),genreEvents);
  assert.equal(calls.at(-1).url,base+'/api/v1/genres/epg/events?backendId=home-vdr&contentClass=movie&genreId=adventure&offset=30');
  const beforeInvalid=calls.length;
  for(const invalid of [
    () => client.getEpgEvent({query:{...eventQuery,eventId:'../bad'}}),
    () => client.getEpgEvent({query:{...eventQuery,legacy:true}}),
    () => client.getEpgGenres({query:{backendId:'home-vdr',limit:31}}),
    () => client.getEpgGenreEvents({query:{backendId:'home-vdr',contentClass:'series',genreId:'adventure'}}),
    () => client.getEpgWindow({query:{backendId:'home-vdr',channelId:'one',fromTime:start,untilTime:start}}),
    () => client.getEpgWindow({query:{backendId:'home-vdr',channelId:'one',fromTime:start,untilTime:until,limit:193}}),
    () => client.getEpgNowNext({query:{backendId:'home-vdr',channelId:'one',fromTime:start,limit:3}}),
    () => client.getEpgNowNext({query:{backendId:'home-vdr',channelId:'one',fromTime:start,cursor:'ew1_test'}}),
    () => client.searchEpg({query:{backendId:'home-vdr',query:'a'}}),
    () => client.searchEpg({query:{backendId:'home-vdr',query:'long enough',offset:-1}}),
    () => client.searchRecordings({query:{backendId:'home-vdr',query:'test',limit:31}}),
    () => client.getAdjacentChannel({query:{backendId:'home-vdr',relativeTo:'channel-1',direction:'left'}})
  ]) assert.throws(invalid);
  assert.equal(calls.length,beforeInvalid);

  replies.push(response(503,{
    type:'urn:vdr-suite:error:service-unavailable',status:503,
    code:'service_unavailable',requestId:'req-epg-failure',
    detail:'EPG index is unavailable'
  }));
  await assert.rejects(
    client.searchEpg({query:{backendId:'home-vdr',query:'Tatort'}}),
    error => api.isClientError(error) &&
      error.status===503 && error.code==='service_unavailable' &&
      error.requestId==='req-epg-failure'
  );
  assert.equal(calls.length,beforeInvalid+1);
  assert.equal(replies.length,0);
  for(const call of calls) {
    assert.equal(call.options.method,'GET');
    assert.equal(call.options.headers.Authorization,credential);
    assert.equal(call.url.includes('/api/vdr/'),false);
    assert.equal(call.url.includes('/api/epg/search'),false);
  }
  console.log('API_V1_F3_CLIENT_READS=PASS');
}

run().catch(error=>{console.error(error);process.exitCode=1;});
