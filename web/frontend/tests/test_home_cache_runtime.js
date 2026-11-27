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
const createHomeRuntime = window.VdrSuiteHomeCache.createRuntime;
const scopeA = 'vsr-' + 'a'.repeat(64), scopeB = 'vsr-' + 'b'.repeat(64);
const pathA = '/api/vdr/recordings/query', seriesPath = '/api/metadata/genres/recordings';
const tick = () => new Promise(resolve => setImmediate(resolve));
async function drain() { for (let i = 0; i < 30; i++) await tick(); }
let authenticated = true, clock = 1000;
const painted = [];
function create() {
  const r = createHomeRuntime({bindEvents:false, requireCursor:false, active:()=>authenticated, now:()=>clock,
    paint(rail, title, items) { const entry = {rail, title, items, removed:false}; painted.push(entry); return ()=>{entry.removed=true;}; }});
  r.selectBackend('alpha');
  return r;
}
function response(scope, backend='alpha', domain='recordings') {
  const headers = new Map([['X-VDR-Suite-Home-Backend',backend],[domain==='history'?'X-VDR-Suite-Home-History-Scope':'X-VDR-Suite-Home-Recording-Scope',scope]]);
  return {ok:true, headers};
}
function observe(r, path, partition=scopeA, reply=response(partition), payload={}, view) {
  const stamp = r.requestStamp(path,'alpha',view);
  r.observe(stamp,reply,payload);
  return payload;
}
const cards = [{id:'1',title:'Old poster',subtitle:'Episode',posterUrl:'/api/vdr/recordings/metadata/image?backend=alpha&recordingId=1&kind=preferred',password:'secret'},
  {id:'2',title:'No external',posterUrl:'https://unknown.test/a.jpg'},
  {id:'3',title:'No token',posterUrl:'/api/vdr/recordings/metadata/image?backend=alpha&recordingId=3&token=secret'},
  {id:'4',title:'No foreign',posterUrl:'/api/vdr/recordings/metadata/image?backend=other&recordingId=4'}];
(async()=>{
  const folders = create();
  const folderRoute = '/api/vdr/recordings/folder';
  const firstFolder = folders.beginFolder('Serien','alpha',()=>true,()=>null);
  assert(firstFolder.accept(observe(folders,folderRoute,scopeA,response(scopeA),
    {path:'Serien',homeCardPage:true,cacheReady:true,offset:0})));
  assert(await firstFolder.finish(cards));
  let paints = 0, removes = 0;
  const reopened = folders.beginFolder('Serien','alpha',()=>true,items=>{
    assert.equal(items[0].title,'Old poster'); paints++; return ()=>{removes++;};
  });
  await drain(); assert.equal(paints,1);
  const other = folders.beginFolder('Action','alpha',()=>true,()=>{throw Error('wrong path');});
  await drain(); assert.equal(paints,1);
  assert(!other.accept(observe(folders,folderRoute,scopeA,response(scopeA),
    {path:'Serien',homeCardPage:true,cacheReady:true,offset:0})));
  assert.equal(await other.finish(cards),false);
  folders.sessionBoundary(); assert.equal(removes,1);
  reopened.cancel();
  // Session boundary purges the test installation. Existing tests below
  // establish their own proof and entries again.
  await drain();
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
  // Use the actual JSON client to hold response body parsing while the
  // header-authorized preview is rendered. The payload remains online-owned.
  const j=create(); const seed=j.begin('series','alpha','Series',()=>true,false);
  const seriesCards=[
    {id:'selected',title:'Selected',posterUrl:'/api/backends/alpha/settings/series-artwork/image?seriesKey=folder%3Aseries&revision=7'},
    {id:'other',title:'Other backend',posterUrl:'/api/backends/beta/settings/series-artwork/image?seriesKey=folder%3Aseries&revision=7'},
    {id:'candidate',title:'Candidate',posterUrl:'/api/backends/alpha/settings/series-artwork/candidate-image?seriesKey=x&revision=1'},
    {id:'token',title:'Token',posterUrl:'/api/backends/alpha/settings/series-artwork/image?seriesKey=x&revision=7&token=private'}];
  assert(seed.accept(observe(j,seriesPath))); assert(await seed.finish(seriesCards));
  const k=create(); const startup=k.begin('series','alpha','Series',()=>true);
  const n=painted.length;
  let releaseBody;
  const reply=response(scopeA);
  reply.text=()=>new Promise(resolve=>{releaseBody=resolve;});
  const wrapper=fs.readFileSync(path.join(__dirname,'../api/client-api.js'),'utf8');
  window.VdrSuiteHomeCache=k;
  vm.runInNewContext(wrapper,{window,URLSearchParams,fetch:()=>Promise.resolve(reply)});
  const fetchPromise=window.VdrSuiteClientApi.requestJson(seriesPath,{query:{backend:'alpha'}});
  await drain();
  assert.equal(painted.length,n+1,'headers authorize local paint while JSON is still pending');
  assert.equal(painted[n].items[0].posterUrl,seriesCards[0].posterUrl);
  assert(painted[n].items.slice(1).every(item=>!item.posterUrl));
  releaseBody('{}'); const parsed=await fetchPromise;
  assert(startup.accept(parsed),'parsed payload retains exact response provenance');
  assert(await startup.finish(seriesCards));
  assert(painted[n].removed);
  const staleDisk=window.VdrSuiteHomePersistence.create({authorize:async scope=>({...scope,authorized:true})});
  const oldScope={accountId:scopeA,backendId:'alpha',permissionRevision:scopeA};
  assert.equal(await staleDisk.read(oldScope,'series',-1),null,'age-bound read rejects stale row');
  // Exercise the actual JSON and genre clients with multiple raw HTTP pages.
  // The synthetic aggregate must never stand in for a proven page.
  const genreRuntime=create();
  window.VdrSuiteHomeCache=genreRuntime;
  const genreTicket=genreRuntime.begin('genres','alpha','Genres',()=>true,false);
  const requests=[];
  let failSecond=false, missingProof=false;
  const allGenres=Array.from({length:31},(_,i)=>({
    genreId:'genre-'+String(i).padStart(2,'0'), label:'Genre '+i,
    labelDe:'Genre '+i, labelEn:'Genre '+i, known:true, count:i+1,
    activeCount:i+1, staleCount:0, conflictCount:0, displayOrder:30-i
  }));
  const genreFetch=(url,options)=>{
    requests.push(url);
    const parsed=new URL(url,'https://example.test');
    assert.equal(parsed.pathname,'/api/v1/genres');
    assert.equal(parsed.searchParams.get('backendId'),'alpha');
    assert.equal(parsed.searchParams.get('limit'),'30');
    assert.equal(options.credentials,'same-origin');
    const offset=Number(parsed.searchParams.get('offset'));
    const items=allGenres.slice(offset,offset+30);
    const reply=response(scopeA);
    if (offset && missingProof) reply.headers=new Map();
    if (offset && failSecond) {
      reply.ok=false; reply.status=403;
      reply.text=()=>Promise.resolve('{"code":"permission_denied"}');
    } else {
      reply.text=()=>Promise.resolve(JSON.stringify({items,totalItems:100,assignmentCount:120,
        page:{limit:30,offset,totalCount:31,hasMore:offset+items.length<31},meta:{partial:false}}));
    }
    return Promise.resolve(reply);
  };
  vm.runInNewContext(wrapper,{window,URLSearchParams,fetch:genreFetch});
  vm.runInNewContext(fs.readFileSync(path.join(__dirname,'../api/genre-client-api.js'),'utf8'),{window});
  const options={backendId:'alpha',locale:'de',onPage:page=>assert(genreTicket.accept(page))};
  const overview=await window.VdrSuiteClientApi.fetchClientGenres({...options,scope:'recordings'});
  assert.equal(requests.length,2);
  assert.equal(overview.totalItems,100);
  assert.equal(overview.assignmentCount,120);
  assert.equal(overview.genres.length,31);
  assert.equal(overview.genres[0].id,'genre-30');
  assert.equal(overview.genres[30].id,'genre-00');
  const genreCards=overview.genres.slice(0,24).map(g=>({id:g.id,title:g.label}));
  assert(await genreTicket.finish(genreCards));
  const synthetic=genreRuntime.begin('genres','alpha','Genres',()=>true,false);
  assert(!synthetic.accept(overview));
  assert.equal(await synthetic.finish(genreCards),false);
  const mixedRoutes=genreRuntime.begin('genres','alpha','Genres',()=>true,false);
  assert(mixedRoutes.accept(observe(genreRuntime,'/api/metadata/genres')));
  assert(!mixedRoutes.accept(observe(genreRuntime,'/api/v1/genres')));
  assert.equal(await mixedRoutes.finish(genreCards),false);
  // Independent cache write before the later denial tests deliberately
  // invalidate and purge the shared installation session epoch.
  const movieRuntime=create();
  const movieTicket=movieRuntime.begin('recent-movies','alpha','Filme',()=>true,false);
  const movieRoute='/api/v1/recordings';
  assert(movieTicket.accept(observe(movieRuntime,movieRoute,scopeA,response(scopeA),{},'movies')));
  const movieArtwork='/api/v1/recordings/rec_'+'a'.repeat(32)+'/artwork/'+'b'.repeat(32)+'?backendId=alpha';
  const movieCards=[{id:'movie',title:'Movie',posterUrl:movieArtwork},
    {id:'foreign',title:'Foreign',posterUrl:movieArtwork.replace('alpha','beta')},
    {id:'token',title:'Token',posterUrl:movieArtwork+'&token=secret'}];
  assert(await movieTicket.finish(movieCards));
  const movieDisk=await staleDisk.read(oldScope,'recent-movies');
  assert.equal(movieDisk.items[0].posterUrl,movieArtwork);
  assert(movieDisk.items.slice(1).every(item=>!item.posterUrl));
  const movieMixed=movieRuntime.begin('recent-movies','alpha','Filme',()=>true,false);
  assert(movieMixed.accept(observe(movieRuntime,movieRoute,scopeA,response(scopeA),{},'movies')));
  assert(!movieMixed.accept(observe(movieRuntime,pathA)));
  assert.equal(await movieMixed.finish(movieCards),false);
  const newRuntime=create();
  const newTicket=newRuntime.begin('newly-recorded','alpha','Neu',()=>true,false);
  window.VdrSuiteHomeCache=newRuntime;
  const newRows=[{kind:'recording', recordingId:'rec_'+'a'.repeat(32), backendId:'alpha', title:'New', recordedAt:'2026-11-27'}];
  const newReply=response(scopeA);
  newReply.text=()=>Promise.resolve(JSON.stringify({items:newRows,page:{limit:12,offset:0,totalCount:1,hasMore:false},meta:{partial:false,cardPresentation:true}}));
  vm.runInNewContext(wrapper,{window,URLSearchParams,fetch:url=>{
    assert(url.includes('view=new')); return Promise.resolve(newReply);
  }});
  let rawNew;
  const newPage=await window.VdrSuiteClientApi.fetchPublicHomeNew({query:{backend:'alpha',limit:12,offset:0},onPage:page=>{rawNew=page;}});
  assert.equal(newPage.publicView,'new');
  assert(newTicket.accept(rawNew));
  assert(await newTicket.finish(movieCards));
  const wrongMovie=newRuntime.begin('recent-movies','alpha','Filme',()=>true,false);
  assert(!wrongMovie.accept(rawNew));
  assert.equal(await wrongMovie.finish(movieCards),false);
  const wrongNew=newRuntime.begin('newly-recorded','alpha','Neu',()=>true,false);
  assert(!wrongNew.accept(observe(newRuntime,movieRoute,scopeA,response(scopeA),{},'movies')));
  assert.equal(await wrongNew.finish(movieCards),false);
  window.VdrSuiteHomeCache=genreRuntime;
  vm.runInNewContext(wrapper,{window,URLSearchParams,fetch:genreFetch});
  const denied=genreRuntime.begin('genres','alpha','Genres',()=>true,false);
  failSecond=true;
  const beforeDenied=requests.length;
  await assert.rejects(window.VdrSuiteClientApi.fetchPublicRecordingGenres({
    backendId:'alpha',onPage:page=>denied.accept(page)}),err=>err.status===403);
  assert.equal(requests.length,beforeDenied+2,'no fallback after authorization failure');
  denied.cancel();
  failSecond=false; missingProof=true;
  const incomplete=genreRuntime.begin('genres','alpha','Genres',()=>true,false);
  const online=await window.VdrSuiteClientApi.fetchPublicRecordingGenres({
    backendId:'alpha',onPage:page=>incomplete.accept(page)});
  assert.equal(online.genres.length,31,'missing optional cache proof does not block online data');
  assert.equal(await incomplete.finish(genreCards),false);
  console.log('PWA.4/PWA.13 scoped Home preview and early headers contract PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
