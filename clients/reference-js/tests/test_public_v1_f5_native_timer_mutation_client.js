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
  console.log('Public-v1 F5 Native Timer modification/delete client PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
