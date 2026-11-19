'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const database = require('./home_persistence_idb_double.js')();
const platform = name => fs.readFileSync(path.join(__dirname, '../platform', name + '.js'), 'utf8');
const window = {indexedDB: database, URL, VdrSuitePublicUrl: {basePath: '/test'}};
const context = vm.createContext({window});
vm.runInContext(platform('home-persistence'), context);
vm.runInContext(platform('home-cache-runtime'), context);
const scopeA = 'vsr-' + 'a'.repeat(64), scopeB = 'vsr-' + 'b'.repeat(64);
const pathA = '/api/vdr/recordings/query', seriesPath = '/api/metadata/genres/recordings';
const tick = () => new Promise(resolve => setImmediate(resolve));
async function drain() { for (let i = 0; i < 30; i++) await tick(); }
let authenticated = true, clock = 1000;
const painted = [];
function create() {
  const r = window.VdrSuiteHomeCache.createRuntime({bindEvents:false, requireCursor:false, active:()=>authenticated, now:()=>clock,
    paint(rail, title, items) { const entry = {rail, title, items, removed:false}; painted.push(entry); return ()=>{entry.removed=true;}; }});
  r.selectBackend('alpha');
  return r;
}
function response(scope, backend='alpha', domain='recordings') {
  const headers = new Map([['X-VDR-Suite-Home-Backend',backend],[domain==='history'?'X-VDR-Suite-Home-History-Scope':'X-VDR-Suite-Home-Recording-Scope',scope]]);
  return {ok:true, headers};
}
function observe(r, path, partition=scopeA, reply=response(partition), payload={}) {
  const stamp = r.requestStamp(path,'alpha');
  r.observe(stamp,reply,payload);
  return payload;
}
const cards = [{id:'1',title:'Old poster',subtitle:'Episode',posterUrl:'/api/vdr/recordings/metadata/image?backend=alpha&recordingId=1&kind=preferred',password:'secret'},
  {id:'2',title:'No external',posterUrl:'https://unknown.test/a.jpg'},
  {id:'3',title:'No token',posterUrl:'/api/vdr/recordings/metadata/image?backend=alpha&recordingId=3&token=secret'},
  {id:'4',title:'No foreign',posterUrl:'/api/vdr/recordings/metadata/image?backend=other&recordingId=4'}];
(async()=>{
  const a=create();
  const first=a.begin('newly-recorded','alpha','Neu',()=>true);
  assert.equal(first.accept(observe(a,pathA)),true);
  assert.equal(await first.finish(cards),true);
  const b=create();
  const second=b.begin('newly-recorded','alpha','Neu',()=>true);
  await drain(); assert.equal(painted.length,0,'no paint before fresh scope');
  observe(b,'/api/backends/alpha/snapshot');
  await drain(); assert.equal(painted.length,1);
  assert.equal(painted[0].items[0].posterUrl,cards[0].posterUrl);
  assert(!('password' in painted[0].items[0]));
  assert(painted[0].items.slice(1).every(item=>!item.posterUrl));
  assert(second.accept(observe(b,pathA))); await second.finish([{id:'new',title:'Fresh'}]);
  assert(painted[0].removed,'live result removes preview');
  const c=create(); const alternate=c.begin('newly-recorded','alpha','Neu',()=>true);
  observe(c,'/api/backends/alpha/snapshot',scopeB); await drain(); assert.equal(painted.length,1,'account isolation'); alternate.cancel();
  const d=create(); const mixed=d.begin('series','alpha','Serien',()=>true);
  assert(mixed.accept(observe(d,seriesPath))); assert(!mixed.accept({})); assert.equal(await mixed.finish(cards),false,'all pages must be proven');
  const e=create(); const delayed=e.begin('newly-recorded','alpha','Neu',()=>true);
  const staleStamp=e.requestStamp(pathA,'alpha'); e.selectBackend('beta');
  e.observe(staleStamp,response(scopeA),{}); assert.equal(await delayed.finish(cards),false);
  const f=create(); const preview=f.begin('newly-recorded','alpha','Neu',()=>true);
  observe(f,'/api/backends/alpha/snapshot'); await drain(); assert.equal(painted.length,2);
  observe(f,pathA,scopeA,{ok:true,headers:new Map()}); assert(painted[1].removed,'missing proof after fresh grant removes preview');
  assert.equal(await preview.finish(cards),false);
  await drain();
  const g=create(); const logout=g.begin('newly-recorded','alpha','Neu',()=>true);
  assert(logout.accept(observe(g,pathA))); assert(await logout.finish(cards));
  g.sessionBoundary(); await drain();
  const h=create(); h.begin('newly-recorded','alpha','Neu',()=>true); observe(h,'/api/backends/alpha/snapshot'); await drain(); assert.equal(painted.length,2,'logout purges');
  authenticated=false; assert.equal(h.begin('newly-recorded','alpha','Neu',()=>true),null);
  authenticated=true;
  const i=create(); const expired=i.begin('newly-recorded','alpha','Neu',()=>true); assert(expired.accept(observe(i,pathA))); clock+=6000; assert.equal(await expired.finish(cards),false,'stale proof cannot write');
  console.log('PWA.4 scoped Home preview contract PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
