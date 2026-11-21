'use strict';

// Also callable with the actual persistence/runtime sources and test double.
async function runHomeCacheIsolation(window) {
  const check = (condition, message) => { if (!condition) throw Error(message); };
  const disk = () => window.VdrSuiteHomePersistence.create({authorize:async s=>({...s,authorized:true})});
  const a=disk(), b=disk();
  const scope = {accountId:'account-a',backendId:'alpha',permissionRevision:'grants-a'};
  const snapshot = {complete:true, revision:'test', items:[{id:'one',title:'Private title'}]};
  const stale = {...scope, sessionEpoch:a.captureSession()};
  check(await a.write(stale,'series',snapshot),'initial write');
  check(await b.read(stale,'series'),'same proven scope');
  check(await b.clear(),'shared logout fence');
  check(await a.read(stale,'series')===null,'old proof cannot read after clear');
  check(await a.write(stale,'series',snapshot)===false,'old proof cannot write after clear');
  const fresh = {...scope, sessionEpoch:a.captureSession()};
  check(await a.write(fresh,'series',snapshot),'fresh proof can write');
  for (const other of [{...fresh,accountId:'account-b'}, {...fresh,backendId:'beta'}, {...fresh,permissionRevision:'grants-b'}])
    check(await b.read(other,'series')===null,'scope isolation');
  const A='vsr-'+'a'.repeat(64), B='vsr-'+'b'.repeat(64);
  const route='/api/vdr/recordings/query';
  let clock=1000;
  const runtime=()=>{
    const r=window.VdrSuiteHomeCache.createRuntime({requireCursor:false,bindEvents:false,active:()=>true,
      now:()=>clock,paint:()=>()=>{}});
    r.selectBackend('alpha'); return r;
  };
  function observe(r,stamp,partition=A) {
    const payload={};
    r.observe(stamp,{ok:true,headers:new Map([['X-VDR-Suite-Home-Backend','alpha'],
      ['X-VDR-Suite-Home-Recording-Scope',partition]])},payload);
    return payload;
  }
  const x=runtime(), ticket=x.begin('newly-recorded','alpha','Neu',()=>true);
  const captured=x.requestStamp(route,'alpha');
  await captured.sessionEpoch;
  check(await b.clear(),'logout while another tab is awaiting network');
  // Simulate another browser context without BroadcastChannel delivery.
  ticket.accept(observe(x,captured));
  check(await ticket.finish(snapshot.items)===false,'late network response cannot recreate old cache');
  const z=runtime(), current=z.begin('newly-recorded','alpha','Neu',()=>true);
  check(current.accept(observe(z,z.requestStamp(route,'alpha'),B)),'new account proof accepted');
  check(await current.finish(snapshot.items),'new scope survives old response');
  const invalid=z.begin('newly-recorded','alpha','Neu',()=>true);
  const changed=observe(z,z.requestStamp(route,'alpha'),A);
  check(!invalid.accept(changed),'drift cancels old ticket');
  check(await invalid.finish(snapshot.items)===false,'drift never relabels rows');
  const late=runtime(), expired=late.begin('series','alpha','Series',()=>true);
  const stamp=late.requestStamp('/api/metadata/genres/recordings','alpha');
  check(expired.accept(observe(late,stamp,B)),'valid server proof');
  clock+=6000;
  check(await expired.finish(snapshot.items)===false,'expired proof never writes');
  await b.clear();
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports=runHomeCacheIsolation;
  if (require.main === module) {
    const fs=require('node:fs'), path=require('node:path'), vm=require('node:vm');
    const window={indexedDB:require('./home_persistence_idb_double.js')(), URL,
      VdrSuitePublicUrl:{basePath:'/pwa7-isolation-test'}};
    for (const name of ['home-persistence','home-cache-runtime'])
      vm.runInNewContext(fs.readFileSync(path.join(__dirname,'../platform',name+'.js'),'utf8'),{window});
    runHomeCacheIsolation(window).then(()=>console.log('PWA.7 cache isolation contract PASS'))
      .catch(error=>{console.error(error);process.exitCode=1;});
  }
}
