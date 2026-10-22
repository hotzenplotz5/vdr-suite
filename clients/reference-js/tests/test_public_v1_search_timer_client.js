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
  status=503;
  await assert.rejects(client.getSearchTimers({query:{backendId:'home'}}),
    error=>api.isClientError(error) && error.status===503 &&
      error.code==='service_unavailable');
  assert.equal(calls.length,before+1);
  console.log('test_public_v1_search_timer_client passed');
}
run().catch(error=>{console.error(error);process.exitCode=1;});
