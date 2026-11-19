'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const code = fs.readFileSync(require('node:path').join(__dirname, '..', 'service-worker.js'), 'utf8');
async function run(prefix) {
  const scope = new URL('https://suite.test' + prefix + '/frontend/');
  const stores = new Map();
  const listeners = {};
  const scopeA = 'vsr-' + 'a'.repeat(64), scopeB = 'vsr-' + 'b'.repeat(64);
  const tagA = '"vsr-' + '1'.repeat(64) + '"', tagB = '"vsr-' + '2'.repeat(64) + '"';
  const url = 'https://suite.test' + prefix + '/api/vdr/recordings/metadata/image?backend=default&recordingId=7&variant=home';
  let partition = scopeA, tag = tagA, status = 200, content = 'poster-a', offline = false, fail = false, last;
  let holdImage = null, holdMutation = null;
  let clock = 1700000000000, imageReads = 0, proofReads = 0, fullKeyReads = 0;
  let proofDelay = 0;
  const deferred = () => {
    let resolve;
    const promise = new Promise(done => { resolve = done; });
    return {promise, resolve};
  };
  const keyUrl = key => typeof key === 'string' ? key : key.url;
  const caches = {async open(name) {
    if (fail) throw Error('quota');
    if (!stores.has(name)) stores.set(name, new Map());
    const rows = stores.get(name);
    return {async keys(key, options) {if (key === undefined) fullKeyReads++; return [...rows.keys()].filter(k => key === undefined || (options?.ignoreSearch ? k.split('?')[0] === keyUrl(key).split('?')[0] : k === keyUrl(key))).map(url => ({url}));}, async put(key, response) {rows.set(keyUrl(key), response.clone());}, async match(key) {return rows.get(keyUrl(key))?.clone();}, async delete(key) {return rows.delete(keyUrl(key));}};
  }, async delete(name) {return stores.delete(name);}};
  const responseHeaders = () => ({'Content-Type':'image/jpeg', ETag:tag, 'X-VDR-Suite-Artwork-Scope':partition, 'X-VDR-Suite-Artwork-Bytes':String(Buffer.byteLength(content)), 'Cache-Control':'private, no-cache'});
  const context = {scope, URL, Response, Request, Headers, caches, Date:{now:()=>clock}, self:{registration:{scope:scope.href},addEventListener:(name, cb)=>{listeners[name]=cb;}}, fetch:async request=>{
    last = request;
    if (offline) throw Error('offline');
    if (request.method === 'POST') {
      if (holdMutation) { const waiting = holdMutation; await waiting.promise; }
      return new Response(null, {status:204});
    }
    if (new URL(request.url).pathname.endsWith('/snapshot')) {
      proofReads++;
      clock += proofDelay;
      return new Response('{}', {status, headers: {
        'Cache-Control':'private, no-store', 'X-VDR-Suite-Home-Backend':'default',
        'X-VDR-Suite-Home-Recording-Scope':partition
      }});
    }
    imageReads++;
    if (status !== 200) return new Response('failure', {status});
    const response = (request.headers.get('If-None-Match') || '').split(', ').includes(tag)
      ? new Response(null,{status:304,headers:responseHeaders()})
      : new Response(content,{headers:responseHeaders()});
    if (holdImage) {
      const waiting = holdImage; holdImage = null;
      waiting.entered.resolve(); await waiting.release.promise;
    }
    return response;
  }};
  vm.createContext(context); vm.runInContext(code, context);
  const invoke = (name, args) => {context.args = args; return vm.runInContext(name+'(...args)',context);};
  async function image(address = url, clientId) {
    const request = new Request(address,{credentials:'same-origin'});
    Object.defineProperty(request,'destination',{value:'image'});
    assert.equal(invoke('artworkRequest',[request]),true);
    const pending = [];
    let result; listeners.fetch({request, clientId, waitUntil:p=>pending.push(p), respondWith:p=>{result=p;}});
    assert(result, 'production fetch handler must intercept Home previews');
    const response = await result;
    // Background validation can attach a cache write to the same extendable event.
    for (let offset = 0; offset < pending.length;) {
      const batch = pending.slice(offset); offset += batch.length; await Promise.all(batch);
    }
    return response;
  }
  assert.equal(await (await image()).text(),'poster-a');
  assert.equal(last.cache,'no-store');
  assert.equal((await image()).headers.get('X-VDR-Suite-Artwork-Cache'),'revalidated');
  assert.equal(last.headers.get('If-None-Match'),tagA);
  partition=scopeB; tag=tagB; content='poster-b';
  assert.equal(await (await image()).text(),'poster-b');
  assert.equal((await image()).headers.get('X-VDR-Suite-Artwork-Cache'),'revalidated');
  partition=scopeA; tag=tagA; content='poster-a';
  assert.equal(await (await image()).text(),'poster-a');
  const newTag='"vsr-'+'3'.repeat(64)+'"'; tag=newTag; content='new poster';
  assert.equal(await (await image()).text(),'new poster');
  offline=true; await assert.rejects(image(),/offline/); offline=false;
  status=404; assert.equal((await image()).status,404); status=200;
  await image(); assert.equal(last.headers.get('If-None-Match'),null);
  status=403; assert.equal((await image()).status,403); status=200;
  await image(); assert.equal(last.headers.get('If-None-Match'),null);
  { const pending=[]; listeners.message({data:{type:'vdr-suite-artwork-clear'},waitUntil:p=>pending.push(p)}); await Promise.all(pending); } await image(); assert.equal(last.headers.get('If-None-Match'),null);
  status=401; assert.equal((await image()).status,401); status=200;
  fail=true; assert.equal(await (await image()).text(),'new poster'); fail=false;
  content='x'.repeat(256*1024+1); await image(); assert.equal([...stores.values()].reduce((n,m)=>n+m.size,0),0);
  content='body';
  const scansBefore = fullKeyReads;
  for(let i=0;i<1025;i++) await image(url.replace('recordingId=7','recordingId='+i), 'capacity-tab');
  assert.equal([...stores.values()][0].size,1024);
  assert.ok(fullKeyReads - scansBefore <= 1, 'no full cache-key scan per image write');
  const keys = [...stores.values()][0];
  assert(![...keys.keys()].some(key => key.includes(encodeURIComponent('recordingId=0&'))), 'oldest entry is evicted');
  function measurement(id) {
    let value;
    listeners.message({data:{type:'vdr-suite-artwork-measurement',clientId:'forged'},
      source:{id},ports:[{postMessage:report=>{value=report;}}]});
    return value;
  }
  const capacity = measurement('capacity-tab');
  assert.equal(capacity.capacity,1024);
  assert.equal(capacity.counters.writes,1025);
  assert.equal(capacity.counters.evictions,1);
  assert.equal(measurement('unseen-tab').counters,null);
  assert(!JSON.stringify(capacity).includes('recordingId'));
  for (const address of [url.replace('variant=home','variant=full'),url+'&token=secret',url+'&backend=other',url.replace('suite.test','else.test')]) {
    const request=new Request(address); Object.defineProperty(request,'destination',{value:'image'});
    assert.equal(invoke('artworkRequest',[request]),false);
  }
  // A delayed old 304 must not reanimate stored bytes after a cookie change.
  tag=tagA; partition=scopeA; content='old-session';
  await image();
  const waiting = {entered:deferred(), release:deferred()};
  holdImage = waiting;
  const delayed = image();
  await waiting.entered.promise;
  assert.equal(last.headers.get('If-None-Match'),tagA);
  const logout = new Request('https://suite.test'+prefix+'/api/security/browser-sessions/logout',{method:'POST'});
  holdMutation = deferred();
  let result; listeners.fetch({request:logout,respondWith:p=>{result=p;}});
  assert(result);
  partition=scopeB; tag=tagB; content='new-session';
  waiting.release.resolve();
  const newImage = await delayed;
  assert.equal(await newImage.text(),'new-session');
  assert.equal(newImage.headers.get('X-VDR-Suite-Artwork-Cache'),null);
  assert.equal(last.headers.get('If-None-Match'),null);
  assert.equal(await (await image()).text(),'new-session');
  assert.equal(last.headers.get('If-None-Match'),null);
  holdMutation.resolve(); await result; holdMutation = null;
  assert.equal(stores.size,0);
  await image();
  const claim = new Request('https://suite.test'+prefix+'/api/security/first-admin/claim',{method:'POST'});
  listeners.fetch({request:claim,respondWith:p=>{result=p;}}); await result;
  assert.equal(stores.size,0);
  await image();
  offline=true;
  listeners.fetch({request:logout,respondWith:p=>{result=p;}});
  await assert.rejects(result,/offline/); offline=false;
  assert.equal(stores.size,0,'failed session mutation still removes optional cached bytes');

  async function proof(clientId = 'tab-a') {
    const request = new Request('https://suite.test'+prefix+'/api/backends/default/snapshot',{credentials:'same-origin'});
    Object.defineProperty(request,'destination',{value:''});
    let result;
    const pending=[];
    listeners.fetch({request, clientId, waitUntil:p=>pending.push(p), respondWith:p=>{result=p;}});
    assert(result,'the normal server proof must pass through the production fetch handler');
    const response=await result; await Promise.all(pending); return response;
  }
  partition=scopeA; tag=tagA; content='warm-poster';
  await image(url);
  await proof();
  const before=imageReads;
  const local=await image(url,'tab-a');
  assert.equal(await local.text(),'warm-poster');
  assert.equal(local.headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  assert.equal(imageReads,before,'a fresh scoped cached image must not make an image HTTP request');
  assert.equal(proofReads,1);
  assert.equal(measurement('tab-a').counters.localHits,1);
  assert.equal(measurement('tab-a').counters.imageHttpRequests,0);
  assert.equal(measurement('tab-a').counters.acceptedProofs,1);

  await image(url,'other-tab');
  assert.equal(imageReads,before+1,'another tab needs its own fresh proof');
  clock+=30001;
  await image(url,'tab-a');
  assert.equal(imageReads,before+2,'expired proof uses original revalidation');
  assert.equal(measurement('tab-a').counters.expiredProof,1);
  assert.equal(measurement('other-tab').counters.missingProof,1);
  // Delay must not consume the post-response artwork window.
  proofDelay=9000;
  await proof();
  proofDelay=0;
  clock+=15000;
  assert.equal((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  const direct = 'https://suite.test'+prefix+'/recording-artwork/default/'+'a'.repeat(32)+'?variant=home';
  await image(direct);
  const directReads=imageReads;
  assert.equal((await image(direct,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  assert.equal(imageReads,directReads);
  const otherDirect=direct.replace('/default/','/other/');
  await image(otherDirect);
  assert.notEqual((await image(otherDirect,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  for (const bad of [direct.replace('?variant=home',''), direct+'&token=bad', direct+'&backend=other',
      direct.replace('a'.repeat(32),'not-an-id'), direct.replace('/default/','/%64efault/'),
      direct.replace('suite.test','outside.test')]) {
    const req=new Request(bad); Object.defineProperty(req,'destination',{value:'image'});
    assert.equal(invoke('artworkRequest',[req]),false);
  }
  clock+=16000;
  assert.notEqual((await image(direct,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  proofDelay=60001;
  await proof();
  proofDelay=0;
  assert.notEqual((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  await proof();
  partition=scopeB; tag=tagB; content='different-account';
  await proof();
  const different=await image(url,'tab-a');
  assert.equal(await different.text(),'different-account');
  assert.notEqual(different.headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  await image(url,'tab-a');
  const warmB=imageReads;
  assert.equal((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  assert.equal(imageReads,warmB);
  clock+=5*60*1000+1;
  await proof();
  const stale=await image(url,'tab-a');
  assert.equal(await stale.text(),'different-account');
  assert.equal(stale.headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  assert.equal(imageReads,warmB+1,'stale visible poster is refreshed in the background');
  const refreshed=imageReads;
  assert.equal((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  assert.equal(imageReads,refreshed,'successful 304 refresh renews local freshness');
  clock+=24*60*60*1000+1;
  await proof();
  assert.notEqual((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local', 'over-age artwork must use online validation');
  assert.equal((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  const epg = url.replace('/api/vdr/recordings/metadata/image','/api/epg/cache/artwork');
  await image(epg,'tab-a');
  assert.notEqual((await image(epg,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local', 'recording proof is not EPG authorization');
  status=403;
  await proof(); status=200;
  assert.notEqual((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  await proof();
  listeners.message({data:{type:'vdr-suite-artwork-proof-reset'}});
  assert.notEqual((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  await proof();
  listeners.fetch({request:logout,respondWith:p=>{result=p;}}); await result;
  assert.equal(stores.size,0);
  assert.notEqual((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
}
(async()=>{await run('');await run('/vdr-suite');console.log('PWA.3 artwork cache contract PASS');})().catch(e=>{console.error(e);process.exitCode=1;});
