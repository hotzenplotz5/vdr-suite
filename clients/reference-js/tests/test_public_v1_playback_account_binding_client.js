'use strict';

const assert = require('assert').strict;
const {createClient} = require('../public-v1-client.js');
const deviceId = 'device_0123456789abcdef';
const path = 'https://suite.example/api/v1/devices/'+deviceId+'/playback-account';
let state = {deviceId, accountId:null, revision:0};
const calls = [];
const client = createClient({
  baseUrl:'https://suite.example',
  credentials:'include',
  fetch:async (url,options)=>{
    calls.push({url,options});
    assert.strictEqual(url,path);
    assert.strictEqual(options.credentials,'include');
    assert.strictEqual(options.headers['X-CSRF-Token'],'session-csrf');
    if(options.method==='GET')
      return {ok:true,status:200,text:async()=>JSON.stringify(state)};
    assert.strictEqual(options.method,'POST');
    assert.strictEqual(options.headers['Content-Type'],'application/json');
    const body=JSON.parse(options.body);
    assert.deepStrictEqual(Object.keys(body).sort(),['accountId','expectedRevision'].sort());
    if(body.expectedRevision!==state.revision)
      return {ok:false,status:409,text:async()=>JSON.stringify({
        error:{code:'playback_binding_mutation_rejected'}
      })};
    state={deviceId,accountId:body.accountId||null,revision:state.revision+1};
    return {ok:true,status:200,text:async()=>JSON.stringify(state)};
  }
});
const options = {deviceId,headers:{'X-CSRF-Token':'session-csrf'}};
(async()=>{
  const unlinked=await client.getDevicePlaybackAccount(options);
  assert.strictEqual(unlinked.revision,0);
  assert.strictEqual(unlinked.accountId,null);
  const assigned=await client.setDevicePlaybackAccount({
    ...options,accountId:'account_alice',expectedRevision:0
  });
  assert.strictEqual(assigned.revision,1);
  assert.strictEqual(assigned.accountId,'account_alice');
  assert.strictEqual((await client.getDevicePlaybackAccount(options)).accountId,'account_alice');
  await assert.rejects(client.setDevicePlaybackAccount({
    ...options,accountId:'account_bob',expectedRevision:0
  }), err=>err.status===409);
  assert.strictEqual((await client.getDevicePlaybackAccount(options)).accountId,'account_alice');
  const revoked=await client.setDevicePlaybackAccount({
    ...options,accountId:'',expectedRevision:1
  });
  assert.strictEqual(revoked.revision,2);
  assert.strictEqual(revoked.accountId,null);
  assert.throws(()=>client.getDevicePlaybackAccount({deviceId:'../other'}));
  assert.throws(()=>client.setDevicePlaybackAccount({
    ...options,accountId:'account_bob',expectedRevision:-1
  }));
  assert(calls.every(item=>item.url===path));
  console.log('Public-v1 playback account binding client PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
