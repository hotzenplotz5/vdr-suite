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
  const keyUrl = key => typeof key === 'string' ? key : key.url;
  const caches = {async open(name) {
    if (fail) throw Error('quota');
    if (!stores.has(name)) stores.set(name, new Map());
    const rows = stores.get(name);
    return {async keys(key, options) {return [...rows.keys()].filter(k => key === undefined || (options?.ignoreSearch ? k.split('?')[0] === keyUrl(key).split('?')[0] : k === keyUrl(key))).map(url => ({url}));}, async put(key, response) {rows.set(keyUrl(key), response.clone());}, async match(key) {return rows.get(keyUrl(key))?.clone();}, async delete(key) {return rows.delete(keyUrl(key));}};
  }, async delete(name) {return stores.delete(name);}};
  const responseHeaders = () => ({'Content-Type':'image/jpeg', ETag:tag, 'X-VDR-Suite-Artwork-Scope':partition, 'X-VDR-Suite-Artwork-Bytes':String(Buffer.byteLength(content)), 'Cache-Control':'private, no-cache'});
  const context = {scope, URL, Response, Request, Headers, caches, self:{registration:{scope:scope.href},addEventListener:(name, cb)=>{listeners[name]=cb;}}, fetch:async request=>{
    last = request;
    if (offline) throw Error('offline');
    if (status !== 200) return new Response('failure', {status});
    if ((request.headers.get('If-None-Match') || '').split(', ').includes(tag)) return new Response(null,{status:304,headers:responseHeaders()});
    return new Response(content,{headers:responseHeaders()});
  }};
  vm.createContext(context); vm.runInContext(code, context);
  const invoke = (name, args) => {context.args = args; return vm.runInContext(name+'(...args)',context);};
  async function image(address = url) {
    const request = new Request(address,{credentials:'same-origin'});
    Object.defineProperty(request,'destination',{value:'image'});
    assert.equal(invoke('artworkRequest',[request]),true);
    const pending = [];
    let result; listeners.fetch({request, waitUntil:p=>pending.push(p), respondWith:p=>{result=p;}});
    assert(result, 'production fetch handler must intercept Home previews');
    const response = await result;
    await Promise.all(pending); return response;
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
  content='body'; for(let i=0;i<193;i++) await image(url.replace('recordingId=7','recordingId='+i));
  assert.equal([...stores.values()][0].size,192);
  for (const address of [url.replace('variant=home','variant=full'),url+'&token=secret',url+'&backend=other',url.replace('suite.test','else.test')]) {
    const request=new Request(address); Object.defineProperty(request,'destination',{value:'image'});
    assert.equal(invoke('artworkRequest',[request]),false);
  }
  const logout = new Request('https://suite.test'+prefix+'/api/security/browser-sessions/logout',{method:'POST'});
  let result; listeners.fetch({request:logout,respondWith:p=>{result=p;}});
  assert(result); await result;
  assert.equal(stores.size,0);
}
(async()=>{await run('');await run('/vdr-suite');console.log('PWA.3 artwork cache contract PASS');})().catch(e=>{console.error(e);process.exitCode=1;});
