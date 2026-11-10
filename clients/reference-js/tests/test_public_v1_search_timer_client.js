'use strict';
const assert = require('node:assert/strict');
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
  const calls=[];
  let status=200;
  const client=api.createClient({
    baseUrl:'https://suite.example/',
    headers:{Authorization:'VDR-Suite-Device test-credential'},
    fetch(url,options) {
      calls.push({url,options});
      return Promise.resolve(response(status,status===200 &&
        url.includes('/api/v1/search-timers/st_') ? {
        searchTimerId:'st_'+'1'.repeat(32),backendId:'home',
        name:'Doku',query:'Doku',state:'inactive',meta:{partial:false}
      }:status===200?{
        items:[{searchTimerId:'st_'+'1'.repeat(32),backendId:'home',
          name:'Doku',query:'Doku',state:'active'}],
        page:{limit:30,nextCursor:null,hasMore:false},meta:{partial:false}
      }:status===202?{
        operationId:'op_'+'1'.repeat(32),state:'succeeded',backendId:'home'
      }:{
        type:'urn:vdr-suite:error:service-unavailable',status:503,
        code:'service_unavailable',requestId:'st-503'
      }));
    }
  });
  const result=await client.getSearchTimers({
    query:{backendId:'home',limit:30,cursor:'st1_'+'ab'.repeat(20)},
    cache:'no-store'
  });
  assert.equal(result.items[0].name,'Doku');
  assert.equal(calls[0].url,
    'https://suite.example/api/v1/search-timers?backend=home&limit=30&cursor=st1_'+
    'ab'.repeat(20));
  assert.equal(calls[0].options.method,'GET');
  assert.equal(calls[0].options.cache,'no-store');
  assert.equal(calls[0].options.headers.Authorization,
    'VDR-Suite-Device test-credential');
  const detailId='st_'+'1'.repeat(32);
  const detail=await client.getSearchTimer({backendId:'home',searchTimerId:detailId});
  assert.equal(detail.searchTimerId,detailId);
  assert.equal(detail.state,'inactive');
  assert.equal(detail.meta.partial,false);
  assert.equal(calls.at(-1).url,
    'https://suite.example/api/v1/search-timers/'+detailId+'?backend=home');
  assert.equal(calls.at(-1).options.method,'GET');
  const beforeDetailErrors=calls.length;
  for(const bad of [
    {backendId:'home',searchTimerId:'0'},
    {backendId:'other/host',searchTimerId:detailId},
    {backendId:'home',searchTimerId:detailId+'/preview'},
    {backendId:'home'},
    {searchTimerId:detailId}
  ]) assert.throws(()=>client.getSearchTimer(bad));
  assert.equal(calls.length,beforeDetailErrors);
  const before=calls.length;
  for(const query of [{},{backendId:'bad/scope'},{backendId:'home',limit:31},
    {backendId:'home',limit:0},{backendId:'home',offset:1},
    {backendId:'home',cursor:'incorrect'},{backendId:'home',sort:'name'}]) {
    assert.throws(()=>client.getSearchTimers({query}));
  }
  assert.equal(calls.length,before);
  // Preview is read-only; it is not a recording count or an activation.
  await client.previewSearchTimerDraft({query:{
    backendId:'home',query:'Terra X',mode:1,
    compareTitle:true,compareSubtitle:false,compareSummary:false,limit:5
  }});
  const previewCall=calls.at(-1);
  assert.equal(previewCall.options.method,'GET');
  assert.equal(previewCall.url,
    'https://suite.example/api/v1/search-timers/draft-preview?backend=home'+
    '&query=Terra%20X&mode=1&compareTitle=1&compareSubtitle=0'+
    '&compareSummary=0&limit=5');
  const beforeStaging=calls.length;
  const sample={
    backendId:'home',name:'Terra X',query:'Terra X',
    active:false,confirmed:true,previewAcknowledged:true,
    idempotencyKey:'safe-searchtimer-1',mode:1,
    compareTitle:true,compareSubtitle:false,compareSummary:false
  };
  for(const change of [
    {active:true},{active:undefined},{previewAcknowledged:false},
    {confirmed:false},{idempotencyKey:''},{name:'Different'},
    {blacklistMode:1},{mode:4},{compareTitle:false},
    {tolerance:2}
  ]) {
    assert.throws(()=>client.createInactiveSearchTimer(
      Object.assign({},sample,change)));
  }
  assert.equal(calls.length,beforeStaging);
  status=202;
  const staged=await client.createInactiveSearchTimer(sample);
  assert.equal(staged.status,202);
  const write=calls.at(-1);
  assert.equal(write.url,'https://suite.example/api/v1/search-timers?backend=home');
  assert.equal(write.options.method,'POST');
  assert.equal(write.options.headers['Idempotency-Key'],'safe-searchtimer-1');
  assert.equal(write.options.headers['Content-Type'],'application/json');
  assert.deepEqual(JSON.parse(write.options.body),{
    name:'Terra X',query:'Terra X',active:false,confirmed:true,mode:1,
    tolerance:0,compareTitle:true,compareSubtitle:false,compareSummary:false
  });
  assert.equal(staged.data.operationId,'op_'+'1'.repeat(32));
  // Full Public-v1 CREATE is usable by every Device client, including
  // active epgsearch rules and advanced provider options.
  const advanced = Object.assign({}, sample, {
    active:true,mode:4,priority:50,useChannel:2,channels:'Öffentlich',
    useTime:true,startTime:930,stopTime:1130,
    avoidRepeats:true,allowedRepeats:2,blacklistIds:'4,5',
    useExtendedEpgInfo:true,extendedEpgInfo:'genre=movie',
    marginStartMinutes:0
  });
  delete advanced.previewAcknowledged;
  const full = await client.createSearchTimer(advanced);
  assert.equal(full.status,202);
  const fullWrite = calls.at(-1);
  assert.equal(fullWrite.url,
    'https://suite.example/api/v1/search-timers?backend=home');
  const body = JSON.parse(fullWrite.options.body);
  assert.equal(body.active,true);
  assert.equal(body.mode,4);
  assert.equal(body.priority,50);
  assert.equal(body.useChannel,2);
  assert.equal(body.channels,'Öffentlich');
  assert.equal(body.startTime,930);
  assert.equal(body.avoidRepeats,true);
  assert.equal(body.blacklistIds,'4,5');
  assert.equal(body.extendedEpgInfo,'genre=movie');
  assert.equal(body.marginStartMinutes,0);
  const beforeInvalid = calls.length;
  for (const delta of [
    {useChannel:4}, {startTime:2360}, {priority:'50'},
    {confirmed:false}, {active:undefined}, {unknownOption:'unsafe'}
  ]) assert.throws(()=>client.createSearchTimer(
    Object.assign({},advanced,delta)));
  assert.equal(calls.length,beforeInvalid);
  status=503;
  const beforeError = calls.length;
  await assert.rejects(client.getSearchTimers({query:{backendId:'home'}}),
    error=>api.isClientError(error) && error.status===503 &&
      error.code==='service_unavailable');
  assert.equal(calls.length,beforeError+1);
  console.log('test_public_v1_search_timer_client passed');
}
run().catch(error=>{console.error(error);process.exitCode=1;});
