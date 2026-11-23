'use strict';

const assert = require('assert').strict;
const {createClient} = require('../public-v1-client.js');
const calls = [];
const payload = {
  backendId:'default',channelId:'S19.2E-1-100',
  title:'Köln, Spezial',directory:'News',
  startAt:1795547700,stopAt:1795552200,
  priority:0,lifetime:0,enabled:true,vps:false
};
const respond = (status, data, location) => ({
  status, ok:status >= 200 && status < 300,
  headers:{get:k=>k==='Location'?location:null},
  text:async()=>JSON.stringify(data)
});
const client = createClient({baseUrl:'https://suite.example',credentials:'include',
  fetch:async (url, options)=>{
    calls.push({url,options});
    assert.strictEqual(url,'https://suite.example/api/v1/manual/timer-intents');
    assert.strictEqual(options.method,'POST');
    assert.strictEqual(options.cache,'no-store');
    assert.strictEqual(options.credentials,'include');
    assert.strictEqual(options.headers['Content-Type'],'application/json');
    assert.strictEqual(options.headers['X-CSRF-Token'],'fixture-csrf');
    assert.strictEqual(options.headers['If-Match'],undefined);
    assert.strictEqual(options.headers['Idempotency-Key'],'manual-once-1');
    assert.deepStrictEqual(JSON.parse(options.body),payload);
    return calls.length===1
      ? respond(202,{operation:{operationId:'op_1',state:'accepted'}},
          '/api/v1/operations/op_1')
      : respond(409,{code:'idempotency_conflict'});
  }
});
(async()=>{
  const input={idempotencyKey:'manual-once-1',
    headers:{'X-CSRF-Token':'fixture-csrf'},payload};
  const accepted=await client.submitManualTimerIntent(input);
  assert.strictEqual(accepted.status,202);
  assert.strictEqual(accepted.location,'/api/v1/operations/op_1');
  assert.strictEqual(accepted.data.operation.state,'accepted');
  await assert.rejects(client.submitManualTimerIntent(input),
    err=>err.status===409);
  const before=calls.length;
  assert.throws(()=>client.submitManualTimerIntent({...input,payload:{...payload,aux:'unsafe'}}));
  assert.throws(()=>client.submitManualTimerIntent({...input,payload:{...payload,priority:undefined}}));
  assert.throws(()=>client.submitManualTimerIntent({...input,payload:{...payload,lifetime:100}}));
  assert.throws(()=>client.submitManualTimerIntent({...input,payload:{...payload,stopAt:payload.startAt}}));
  assert.throws(()=>client.submitManualTimerIntent({...input,payload:{...payload,startAt:1.5}}));
  assert.throws(()=>client.submitManualTimerIntent({...input,idempotencyKey:'has space'}));
  assert.throws(()=>client.submitManualTimerIntent({...input,headers:{'If-Match':'"epg1"'}}));
  assert.throws(()=>client.submitManualTimerIntent({...input,headers:{'IF-MATCH':'"epg1"'}}));
  assert.throws(()=>client.submitManualTimerIntent({...input,headers:{'iF-mAtCh':'"epg1"'}}));
  // A Unicode string can be short in JS code units but exceed the
  // server's 1024-byte decoded UTF-8 field limit.
  assert.throws(()=>client.submitManualTimerIntent({...input,
    payload:{...payload,title:'ö'.repeat(513)}}));
  assert.throws(()=>client.submitManualTimerIntent({...input,
    payload:{...payload,directory:'🎥'.repeat(257)}}));
  assert.strictEqual(calls.length,before,'invalid writes cannot dispatch');
  assert(calls.every(call=>!call.url.includes('/api/vdr/')));
  // Exactly 1024 UTF-8 bytes remain valid, including non-BMP characters.
  let boundaryCalls=0;
  const boundaryClient=createClient({baseUrl:'https://suite.example',
    fetch:async (url, options)=>{
      boundaryCalls++;
      assert.strictEqual(url,'https://suite.example/api/v1/manual/timer-intents');
      const posted=JSON.parse(options.body);
      assert.strictEqual(posted.title,'ö'.repeat(512));
      assert.strictEqual(posted.directory,'🎥'.repeat(256));
      return respond(202,{operation:{operationId:'op_limit',state:'accepted'}},
        '/api/v1/operations/op_limit');
    }});
  const boundary=await boundaryClient.submitManualTimerIntent({...input,
    payload:{...payload,title:'ö'.repeat(512),directory:'🎥'.repeat(256)}});
  assert.strictEqual(boundary.status,202);
  assert.strictEqual(boundaryCalls,1);
  console.log('Public-v1 F5.5.5 manual TimerIntent client PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
