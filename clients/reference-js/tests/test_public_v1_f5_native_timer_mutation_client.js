'use strict';

const assert = require('assert').strict;
const {createClient} = require('../public-v1-client.js');
const timerId = 'timer:abc-12';
const etag = '"native-binding-revision-7"';
const calls = [];
function response(status, payload, location) {
  return {ok:status >= 200 && status < 300,status,
    headers:{get(name){return name === 'Location' ? location : null;}},
    text:async()=>JSON.stringify(payload)};
}
const client = createClient({
  baseUrl:'https://suite.example',
  credentials:'include',
  fetch:async (url, init)=>{
    calls.push({url,init});
    assert(url.startsWith('https://suite.example/api/v1/native-timers/'+timerId));
    assert.strictEqual(init.credentials,'include');
    assert.strictEqual(init.headers['If-Match'],etag);
    assert.strictEqual(init.headers['X-CSRF-Token'],'fixture-csrf');
    assert.strictEqual(init.headers['Idempotency-Key'].startsWith('idem-'),true);
    if (init.method === 'POST') {
      assert.strictEqual(url,
        'https://suite.example/api/v1/native-timers/'+timerId+
        '/modifications?backendId=default');
      assert.strictEqual(init.headers['Content-Type'],'application/json');
      assert.deepStrictEqual(JSON.parse(init.body),{kind:'toggle',enabled:false});
      if (init.headers['Idempotency-Key'] === 'idem-conflict')
        return response(409,{code:'native_timer_binding_conflict'});
      return response(202,{operation:{operationId:'op_toggle_1',state:'accepted'}},
        '/api/v1/operations/op_toggle_1');
    }
    if (init.method === 'DELETE') {
      assert.strictEqual(url,
        'https://suite.example/api/v1/native-timers/'+timerId+
        '?backendId=default');
      assert.strictEqual(init.body,undefined,'DELETE has no JSON body');
      return response(202,{operation:{operationId:'op_delete_1',state:'accepted'}},
        '/api/v1/operations/op_delete_1');
    }
    throw Error('Unexpected mutation '+init.method);
  }
});
(async()=>{
  const args={backendId:'default',timerId,ifMatch:etag,
    headers:{'X-CSRF-Token':'fixture-csrf'}};
  const accepted=await client.modifyNativeTimer({...args,
    idempotencyKey:'idem-toggle',payload:{kind:'toggle',enabled:false}});
  assert.strictEqual(accepted.status,202);
  assert.strictEqual(accepted.data.operation.operationId,'op_toggle_1');
  assert.strictEqual(accepted.location,'/api/v1/operations/op_toggle_1');
  await assert.rejects(client.modifyNativeTimer({...args,
    idempotencyKey:'idem-conflict',payload:{kind:'toggle',enabled:false}}),
    error=>error.status===409);
  const removed=await client.deleteNativeTimer({...args,idempotencyKey:'idem-delete'});
  assert.strictEqual(removed.status,202);
  assert.strictEqual(removed.data.operation.operationId,'op_delete_1');
  assert.strictEqual(calls.length,3,'each write is dispatched exactly once');
  assert.throws(()=>client.modifyNativeTimer({...args,payload:{kind:'toggle',enabled:false}}));
  assert.throws(()=>client.deleteNativeTimer({...args,idempotencyKey:'',ifMatch:etag}));
  assert.throws(()=>client.deleteNativeTimer({...args,idempotencyKey:'idem-invalid',
    timerId:'../other'}));
  assert.throws(()=>client.modifyNativeTimer({...args,idempotencyKey:'idem-invalid',
    payload:null}));
  assert.strictEqual(calls.length,3,'invalid writes must not dispatch');
  assert(calls.every(call=>!call.url.includes('/api/vdr/')));
  // Slice 3 direct external/unbound actions are synchronous and never
  // impersonate a managed Binding ETag or an asynchronous Operation.
  const directCalls = [];
  const directClient = createClient({
    baseUrl:'https://suite.example',
    credentials:'include',
    fetch:async(url,init)=>{
      directCalls.push({url,init});
      assert.equal(init.method,'POST');
      assert.equal(init.credentials,'include');
      assert.equal(init.headers['X-CSRF-Token'],'fixture-csrf');
      assert.equal(init.headers['If-Match'],undefined);
      assert.equal(init.headers['Idempotency-Key'],undefined);
      return response(200,{success:true});
    }
  });
  const directOptions={backendId:'default',timerId,
    headers:{'X-CSRF-Token':'fixture-csrf'}};
  const externalUpdate=await directClient.updateExternalNativeTimer({
    ...directOptions,payload:{title:'External',aux:'<epgsearch/>',active:false}
  });
  assert.equal(externalUpdate.status,200);
  assert.equal(externalUpdate.data.success,true);
  assert.equal(directCalls[0].url,
    'https://suite.example/api/v1/native-timers/'+timerId+
    '/direct-update?backendId=default');
  assert.deepStrictEqual(JSON.parse(directCalls[0].init.body),
    {title:'External',aux:'<epgsearch/>',active:false,
     backendId:'default',timerId});
  await directClient.deleteExternalNativeTimer(directOptions);
  assert.equal(directCalls[1].url,
    'https://suite.example/api/v1/native-timers/'+timerId+
    '/direct-delete?backendId=default');
  assert.deepStrictEqual(JSON.parse(directCalls[1].init.body),
    {backendId:'default',timerId});
  assert.throws(()=>directClient.updateExternalNativeTimer({
    ...directOptions,payload:{backendId:'foreign',title:'bad'}}));
  assert.equal(directCalls.length,2);
  const schedule = {backendId:'default', channelId:'C-1', title:'Weekly',
    directory:'News', day:'', weekdays:'M-W-F--', start:2015, stop:2030,
    priority:0, lifetime:0, active:false, vps:true, aux:'<epgsearch/>'};
  const created = await directClient.createNativeTimer({
    backendId:'default', payload:schedule, headers:{'X-CSRF-Token':'fixture-csrf'}});
  assert.equal(created.status,200);
  assert.equal(directCalls[2].url,'https://suite.example/api/v1/native-timers/actions/create');
  assert.deepStrictEqual(JSON.parse(directCalls[2].init.body),{...schedule,timerId:''});
  assert.throws(()=>directClient.createNativeTimer({backendId:'default',payload:{...schedule,backendId:'other'}}));
  assert.throws(()=>directClient.createNativeTimer({backendId:'default',payload:{...schedule,timerId:'17'}}));
  assert.equal(directCalls.length,3);
  const conflictClient = createClient({baseUrl:'https://suite.example', fetch:async(url,init)=>{
    assert.equal(url,'https://suite.example/api/v1/native-timers/conflicts/live?backendId=default');
    assert.equal(init.method,'GET');
    return response(200,{available:true,count:1,total:1,conflicts:[{conflictTime:1795550400}]});
  }});
  const conflicts = await conflictClient.getNativeTimerConflicts({backendId:'default'});
  assert.equal(conflicts.count,1);
  assert.throws(()=>conflictClient.getNativeTimerConflicts({backendId:'../foreign'}));
    console.log('Public-v1 F5 Native Timer modification/delete client PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
