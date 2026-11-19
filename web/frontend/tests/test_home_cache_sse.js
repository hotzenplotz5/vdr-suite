'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const window = {indexedDB: require('./home_persistence_idb_double.js')(), URL,
  VdrSuitePublicUrl: {basePath: '/sse-test'}};
for (const name of ['home-persistence', 'home-cache-runtime'])
  vm.runInNewContext(fs.readFileSync(path.join(__dirname, '../platform', name + '.js'), 'utf8'), {window});
const A = 'vsr-' + 'a'.repeat(64), epoch = 'sse_' + '1'.repeat(32) + ':';
const epoch2 = 'sse_' + '2'.repeat(32) + ':';
const paths = {'newly-recorded':'/api/vdr/recordings/query', 'series':'/api/metadata/genres/recordings',
  'genres':'/api/metadata/genres', 'continue-watching':'/api/media/continue-watching'};
const scope = backendId => ({accountId:A, backendId, permissionRevision:A});
const cards = [{id:'one', title:'One'}];
const snap = revision => ({complete:true, revision, items:cards});
const disk = window.VdrSuiteHomePersistence.create({authorize: async s => ({...s, authorized:true})});
const paints = [];
function make(backend='alpha') {
  const r = window.VdrSuiteHomeCache.createRuntime({active:()=>true, bindEvents:false,
    paint(rail,title,items) {const entry={rail,items,removed:false}; paints.push(entry); return ()=>{entry.removed=true;};}});
  r.selectBackend(backend); return r;
}
function resync(r, seq, reason='initial', prefix=epoch) {
  return r.liveEvent('resync', {lastEventId:prefix+seq, data:JSON.stringify({reason, latestSequenceNumber:seq})});
}
function update(r, seq, changedDomains, backendId='alpha', prefix=epoch) {
  return r.liveEvent('update', {lastEventId:prefix+seq, data:JSON.stringify({sequenceNumber:seq, backendId, changedDomains})});
}
async function drain() { for (let i=0;i<70;i++) await new Promise(resolve=>setImmediate(resolve)); }
function observe(r, route, stamp=r.requestStamp(route,'alpha')) {
  const payload={};
  r.observe(stamp,{ok:true,headers:new Map([['X-VDR-Suite-Home-Backend','alpha'],
    ['X-VDR-Suite-Home-Recording-Scope',A],['X-VDR-Suite-Home-History-Scope',A]])},payload);
  return payload;
}
function begin(r, rail) {return r.begin(rail,'alpha',rail,()=>true);}
async function save(r,rail) {const ticket=begin(r,rail); assert(ticket.accept(observe(r,paths[rail]))); assert(await ticket.finish(cards));}
(async()=>{
  const r=make(); resync(r,10); await drain();
  await save(r,'newly-recorded'); await save(r,'series'); await save(r,'continue-watching');
  assert(await disk.write(scope('beta'),'newly-recorded',snap(epoch+10)));
  const restart=make(); const warm=begin(restart,'newly-recorded');
  observe(restart,'/api/backends/alpha/snapshot'); await drain(); assert.equal(paints.length,0);
  resync(restart,10); await drain(); assert.equal(paints.length,1,'matching fresh cursor permits warm preview');
  update(restart,11,['continueWatching']); await drain();
  assert.equal(paints[0].removed,false,'history update preserves recording preview');
  assert(await disk.read(scope('alpha'),'newly-recorded'));
  assert.equal(await disk.read(scope('alpha'),'continue-watching'),null);
  update(restart,11,['recordings']); await drain();
  assert(await disk.read(scope('alpha'),'newly-recorded'),'duplicate ignored');
  update(restart,12,['recordings'],'beta'); await drain();
  assert.equal(await disk.read(scope('beta'),'newly-recorded'),null);
  assert(await disk.read(scope('alpha'),'newly-recorded'),'other backend unchanged');
  const staleStamp=restart.requestStamp(paths.series,'alpha'); const stale=begin(restart,'series');
  assert(stale.accept(observe(restart,paths.series,staleStamp)));
  update(restart,13,['recordings']); update(restart,14,['recordings']); await drain();
  assert(paints[0].removed); assert.equal(await stale.finish(cards),false);
  assert.equal(await warm.finish(cards),false);
  assert.equal(await disk.read(scope('alpha'),'series'),null);
  assert.equal(await disk.write(scope('alpha'),'series',snap(epoch+12)),false,'late write in another owner rejected by durable floor');
  await save(restart,'series');
  assert.equal((await disk.read(scope('alpha'),'series')).revision,epoch+14);
  update(restart,16,['timers']); await drain();
  assert.equal(await disk.read(scope('alpha'),'series'),null,'gap invalidates every Home snapshot');
  assert.equal(update(restart,18,['timers']),true,'gap requests existing resync path'); await drain();
  await save(restart,'series');
  restart.liveEvent('error',{}); const disconnected=begin(restart,'series');
  assert.equal(disconnected.accept(observe(restart,paths.series)),false); disconnected.cancel();
  restart.liveEvent('open',{}); await save(restart,'series');
  resync(restart,0,'epoch_changed',epoch2); await drain();
  assert.equal(await disk.read(scope('alpha'),'series'),null);
  assert.equal(await disk.write(scope('alpha'),'series',snap(epoch+18)),false);
  await save(restart,'series');
  assert.equal(await disk.discard('alpha',['series'],epoch+19),false,'retired epoch cannot delete a newer row');
  assert.equal((await disk.read(scope('alpha'),'series')).revision,epoch2+0);
  restart.sessionBoundary(); await drain(); assert.equal(await disk.read(scope('alpha'),'series'),null);
  console.log('PWA.5 SSE cursor and persistent invalidation contract PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
