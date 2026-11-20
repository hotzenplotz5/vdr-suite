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
  let proofDelay = 0, epgAuthorized = false, recordingAuthorized = true;
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
    if (new URL(request.url).pathname.endsWith('/snapshot') || new URL(request.url).pathname.endsWith('/now-next-artwork')) {
      proofReads++;
      clock += proofDelay;
      return new Response('{}', {status, headers: {
        'Cache-Control':'private, no-store', 'X-VDR-Suite-Home-Backend':'default',
        ...(recordingAuthorized ? {'X-VDR-Suite-Home-Recording-Scope':partition} : {}),
        ...(epgAuthorized ? {'X-VDR-Suite-Home-Epg-Scope':partition} : {})
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

  async function proof(clientId = 'tab-a', address = 'https://suite.test'+prefix+'/api/backends/default/snapshot') {
    const request = new Request(address,{credentials:'same-origin'});
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

  // Two concurrent cold images of the exact same URL and authorized client
  // must share one HTTP response, with independent readable response bodies.
  const concurrentUrl = url.replace('recordingId=7', 'recordingId=998877');
  const concurrentHold = {entered: deferred(), release: deferred()};
  holdImage = concurrentHold;
  const firstConcurrent = image(concurrentUrl, 'tab-a');
  await concurrentHold.entered.promise;
  const secondConcurrent = image(concurrentUrl, 'tab-a');
  for (let turn = 0; turn < 30 && !vm.runInContext(
    'Array.from(artworkInFlight.values()).some(value => value.followers > 0)', context
  ); turn++) await Promise.resolve();
  assert(vm.runInContext('Array.from(artworkInFlight.values()).some(value => value.followers > 0)', context),
    'the second request must join the first in-flight HTTP request');
  concurrentHold.release.resolve();
  const [firstResponse, secondResponse] = await Promise.all([firstConcurrent, secondConcurrent]);
  assert.equal(await firstResponse.text(), 'warm-poster');
  assert.equal(await secondResponse.text(), 'warm-poster');
  assert.equal(imageReads, before + 1, 'exact simultaneous requests use one image HTTP request');
  assert.equal(measurement('tab-a').counters.coalescedRequests, 1);
  const afterCoalescing = imageReads;

  await image(url,'other-tab');
  assert.equal(imageReads,afterCoalescing+1,'another tab needs its own fresh proof');
  clock+=30001;
  await image(url,'tab-a');
  assert.equal(imageReads,afterCoalescing+2,'expired proof uses original revalidation');
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
  const epg = 'https://suite.test'+prefix+'/api/epg/cache/artwork?backend=default&channelId=C-1&eventId=17';
  const epgDetail = epg.replace('/artwork?', '/metadata/image?')+'&kind=preferred&index=0';
  const originalTag = tag, originalContent = content;
  const epgTag = '"vsr-'+'4'.repeat(64)+'"';
  tag=epgTag; content='x'.repeat(414*1024);
  await image(epg,'tab-a');
  assert.notEqual((await image(epg,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local', 'recording proof is not EPG authorization');
  await image(epgDetail);
  epgAuthorized=true; recordingAuthorized=false;
  await proof();
  const epgReads = imageReads;
  assert.equal((await image(epg,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local','canonical CSS Hero image uses current EPG proof');
  const detail = await image(epgDetail,'tab-a');
  assert.equal(detail.headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  assert.equal((await detail.text()).length,414*1024);
  assert.equal(imageReads,epgReads);
  assert.notEqual((await image(epg,'other-tab')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  tag=originalTag; content=originalContent;
  assert.notEqual((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local','EPG proof is not recording authorization');
  tag=epgTag; content='x'.repeat(414*1024);
  clock+=30001;
  assert.notEqual((await image(epg,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  await proof('tab-a','https://suite.test'+prefix+'/api/epg/cache/now-next-artwork?backend=default&channelIds=C-1');
  assert.equal((await image(epg,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  clock+=5*60*1000+1;
  await proof();
  const beforeRefresh=imageReads;
  assert.equal((await image(epg,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  assert.equal(imageReads,beforeRefresh+1,'stale EPG image refreshes in background');
  const oversized=epg.replace('eventId=17','eventId=18'); content='x'.repeat(1024*1024+1);
  tag='"vsr-'+'5'.repeat(64)+'"';
  await image(oversized);
  assert.notEqual((await image(oversized,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  for (const bad of [epg+'&token=bad',epg+'&eventId=18',epg.replace('backend=','%62ackend='),
    epg.replace('C-1',''),epg+'&variant=full',epgDetail.replace('index=0','index=-1'),epg.replace('suite.test','other.test')]) {
    const req=new Request(bad); Object.defineProperty(req,'destination',{value:'image'});
    assert.equal(invoke('artworkRequest',[req]),false);
  }
  tag=originalTag; content=originalContent; epgAuthorized=false; recordingAuthorized=true;
  await proof();
  status=403;
  await proof(); status=200;
  assert.notEqual((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  await proof();
  listeners.message({data:{type:'vdr-suite-artwork-proof-reset'}});
  assert.notEqual((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  await proof();
  const selected = 'https://suite.test'+prefix+'/api/backends/default/settings/series-artwork/image?seriesKey=folder%3Aseries&revision=7&variant=home';
  await image(selected);
  const selectedReads = imageReads;
  assert.equal((await image(selected,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  assert.equal(imageReads,selectedReads,'selected cover uses the existing scoped cache');
  assert.notEqual((await image(selected,'other-tab')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
  for (const bad of [selected.replace('/image?','/candidate-image?'),
    selected.replace('&variant=home',''), selected.replace('revision=7','revision=0'),
    selected+'&revision=8',selected+'&token=secret',
    selected.replace('/default/','/default/nested/'),selected.replace('suite.test','other.test')]) {
    const req = new Request(bad); Object.defineProperty(req,'destination',{value:'image'});
    assert.equal(invoke('artworkRequest',[req]),false);
  }
  const changed = new Request('https://suite.test'+prefix+'/api/backends/default/settings/series-artwork',{method:'POST'});
  listeners.fetch({request:changed,respondWith:p=>{result=p;}}); await result;
  await proof();
  const afterChange = imageReads;
  assert.equal((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local','ordinary covers survive a series mutation');
  assert.equal(imageReads,afterChange);
  assert.notEqual((await image(selected,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local','same revision cannot reuse pre-mutation bytes');
  assert.equal(imageReads,afterChange+1);
  await proof();
  listeners.fetch({request:logout,respondWith:p=>{result=p;}}); await result;
  assert.equal(stores.size,0);
  assert.notEqual((await image(url,'tab-a')).headers.get('X-VDR-Suite-Artwork-Cache'),'local');
}
(async()=>{await run('');await run('/vdr-suite');console.log('PWA.3 artwork cache contract PASS');})().catch(e=>{console.error(e);process.exitCode=1;});
