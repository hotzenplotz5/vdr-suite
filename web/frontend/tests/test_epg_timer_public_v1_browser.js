'use strict';
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const root = path.resolve(__dirname, '..');
const clientSource = fs.readFileSync(path.join(root,'api','client-api.js'),'utf8');
const appSource = fs.readFileSync(path.join(root,'app.js'),'utf8');
const channelSource = fs.readFileSync(path.join(root,'channel-day-program.js'),'utf8');
const calls=[];
let etag='"epg1:sha256:0123"';
let readStatus=200, createStatus=202, channel='C-1';
const window={
  crypto:{getRandomValues(bytes){bytes.fill(7);return bytes;}},
  VdrSuiteBrowserSession:{csrfHeaders(){return {'X-CSRF-Token':'session-proof'};}}
};
const context={window, URLSearchParams, Uint8Array, Promise, Error, Array, Object,
  Number, String, JSON,
  fetch(url,options){
    calls.push({url,options});
    const read=url.startsWith('/api/v1/epg/event?');
    return Promise.resolve({
      ok:read ? readStatus===200 : createStatus===202,
      status:read ? readStatus : createStatus,
      headers:{get(name){return name==='ETag'?etag:null;}},
      text(){return Promise.resolve(JSON.stringify(read
        ? (readStatus===200 ? {backendId:'home',channelId:channel,eventId:'81234',
            startTime:'1792222800',endTime:'1792226400'} : {detail:'EPG offline'})
        : (createStatus===202
            ? {operation:{operationId:'op_0123456789abcdef0123456789abcdef',state:'accepted'}}
            : {detail:'revision conflict'})));
      }
    });
  }
};
vm.runInNewContext(clientSource,context,{filename:'client-api.js'});
const client=window.VdrSuiteClientApi;
const identity={backendId:'home',channelId:'C-1',eventId:'81234'};

(async()=>{
  const result=await client.fetchClientEpgTimerIntentCreate(identity);
  assert.strictEqual(result.operation.state,'accepted');
  assert.strictEqual(calls.length,2);
  const first=calls[0], second=calls[1];
  assert.strictEqual(first.options.method,'GET');
  assert.strictEqual(first.options.cache,'no-store');
  const q=new URLSearchParams(first.url.split('?')[1]);
  assert.strictEqual(q.get('backendId'),'home');
  assert.strictEqual(q.get('channelId'),'C-1');
  assert.strictEqual(q.get('eventId'),'81234');
  assert.strictEqual(second.url,'/api/v1/epg/timer-intents');
  assert.strictEqual(second.options.method,'POST');
  assert.strictEqual(second.options.headers['If-Match'],etag);
  assert.strictEqual(second.options.headers['X-CSRF-Token'],'session-proof');
  assert.match(second.options.headers['Idempotency-Key'],/^epg-web-[0-9a-f]{32}$/);
  assert.deepStrictEqual(JSON.parse(second.options.body),identity);
  assert.strictEqual(second.options.credentials,'same-origin');

  calls.length=0;
  await assert.rejects(client.fetchClientEpgTimerIntentCreate({...identity,eventId:''}));
  assert.strictEqual(calls.length,0);

  calls.length=0;
  readStatus=503;
  await assert.rejects(client.fetchClientEpgTimerIntentCreate(identity),/EPG offline/);
  assert.strictEqual(calls.length,1);
  readStatus=200;

  calls.length=0;
  etag='W/"weak"';
  await assert.rejects(client.fetchClientEpgTimerIntentCreate(identity),/ETag/);
  assert.strictEqual(calls.length,1);
  etag='"epg1:sha256:0123"';

  calls.length=0;
  channel='C-2';
  await assert.rejects(client.fetchClientEpgTimerIntentCreate(identity),/EPG-Ereignis/);
  assert.strictEqual(calls.length,1);
  channel='C-1';

  calls.length=0;
  createStatus=409;
  await assert.rejects(client.fetchClientEpgTimerIntentCreate(identity),/revision conflict/);
  assert.strictEqual(calls.length,2);
  assert.strictEqual(calls.filter(c=>c.url==='/api/v1/epg/timer-intents').length,1);

  assert.ok(appSource.includes('fetchClientEpgTimerIntentCreate(payload)'));
  assert.ok(channelSource.includes('fetchClientEpgTimerIntentCreate(identity)'));
  assert.ok(!appSource.includes('window.VdrSuiteClientApi.fetchClientTimerCreateAction({'));
  assert.ok(!channelSource.includes('client.fetchClientTimerCreateAction({payload: timerPayload('));
  assert.ok(appSource.includes('Timerauftrag angenommen'));
  assert.ok(channelSource.includes('Auftrag prüfen'));
  console.log('EPG browser public-v1 GET/ETag/POST contract passed');
})().catch(e=>{console.error(e);process.exitCode=1;});
