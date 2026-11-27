'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

function deferred() {
  let resolve;
  let reject;
  const promise = new Promise((ok, fail) => {
    resolve = ok;
    reject = fail;
  });
  return {promise, resolve, reject};
}

const source = fs.readFileSync(
  path.join(__dirname, '..', 'modules', 'global-search.js'),
  'utf8'
);

const document = {
  readyState: 'loading',
  addEventListener: function () {},
  getElementById: function () { return null; },
  querySelector: function () { return null; },
  head: {appendChild: function () {}},
  body: {appendChild: function () {}},
  createElement: function () { throw new Error('DOM creation is not expected in this contract test'); }
};

const selectedModules = [];
const scrollCalls = [];
const window = {
  AbortController,
  Promise,
  Date,
  Intl,
  setTimeout,
  clearTimeout,
  selectModule(moduleName) {
    selectedModules.push(moduleName);
  },
  scrollTo(options) {
    scrollCalls.push(options);
  }
};
window.window = window;

vm.runInNewContext(source, {
  window,
  document,
  console,
  Promise,
  Date,
  Intl,
  AbortController,
  setTimeout,
  clearTimeout
}, {filename: 'global-search.js'});

const testApi = window.VdrSuiteGlobalSearch.__test;
assert.strictEqual(testApi.minimumQueryLength, 2);
assert.strictEqual(testApi.debounceMs, 280);
assert.strictEqual(testApi.requestTimeoutMs, 12000);
assert(source.includes("homeLabel: '⌂ Home'"));
assert(source.includes('focusNavigation: true'));
assert(source.includes('onHome: returnHome'));
testApi.returnHome();
assert.deepStrictEqual(selectedModules, ['overview']);
assert.strictEqual(scrollCalls.length, 1);
assert.strictEqual(scrollCalls[0].top, 0);
assert.strictEqual(scrollCalls[0].left, 0);
assert.strictEqual(scrollCalls[0].behavior, 'auto');

const event = {backendId:'living-room',channelId:'S19.2E-1-2',eventId:'42',
  artwork:{available:true,url:'/api/epg/cache/artwork?backend=living-room&channelId=S19.2E-1-2&eventId=42'}};
const artworkSources = testApi.epgArtworkSources(event);
assert.strictEqual(artworkSources.length,2);
assert.strictEqual(artworkSources[0],'/api/v1/epg/artwork?backendId=living-room&channelId=S19.2E-1-2&eventId=42&variant=home');
assert.strictEqual(artworkSources[1],event.artwork.url);
assert.strictEqual(testApi.epgArtworkSources({artwork:{available:false}}).length,0);
assert.strictEqual(testApi.epgArtworkSources({artwork:event.artwork})[0],event.artwork.url);
let errorHandler = null;
let missing = 0;
const image = {src:'',addEventListener(name, handler, options) {
  assert.strictEqual(name,'error'); assert.strictEqual(options.once,true);
  errorHandler = handler;
}};
testApi.setEpgArtworkImage(image,artworkSources,()=>{missing+=1;});
assert.strictEqual(image.src,artworkSources[0]);
errorHandler();
assert.strictEqual(image.src,artworkSources[1]);
assert.strictEqual(missing,0);
errorHandler();
assert.strictEqual(missing,1);

(async function run() {
  const first = deferred();
  const second = deferred();
  let calls = 0;
  const coordinator = testApi.createRequestCoordinator(function () {
    calls += 1;
    return calls === 1 ? first.promise : second.promise;
  });

  const oldRequest = coordinator.run('John');
  const newRequest = coordinator.run('John Travolta');
  second.resolve({query: 'John Travolta'});
  const newResult = await newRequest;
  assert.strictEqual(newResult.current, true);
  assert.strictEqual(newResult.payload.query, 'John Travolta');

  first.resolve({query: 'John'});
  const oldResult = await oldRequest;
  assert.strictEqual(oldResult.current, false);
  assert.strictEqual(oldResult.payload.query, 'John');

  const pending = deferred();
  const invalidated = testApi.createRequestCoordinator(function () { return pending.promise; });
  const invalidatedRequest = invalidated.run('Pulp Fiction');
  invalidated.invalidate();
  pending.resolve({query: 'Pulp Fiction'});
  const invalidatedResult = await invalidatedRequest;
  assert.strictEqual(invalidatedResult.current, false);


  const timedOut = testApi.createRequestCoordinator(function (_value, signal) {
    return new Promise(function (_resolve, reject) {
      signal.addEventListener('abort', function () {
        const error = new Error('aborted');
        error.name = 'AbortError';
        reject(error);
      }, {once: true});
    });
  }, 5);
  const timeoutResult = await timedOut.run('Langsame Suche');
  assert.strictEqual(timeoutResult.current, true);
  assert(timeoutResult.error);
  assert.match(timeoutResult.error.message, /dauerte zu lange/);


  // Exercise the actual Browser client wrapper, not a replacement matcher.
  const clientSource = fs.readFileSync(path.join(__dirname,'..','api','client-api.js'),'utf8');
  const clientWindow = {};
  const requests = [];
  let replies = [];
  const fetch = (url, options) => {
    requests.push({url:String(url),options});
    assert(replies.length, 'Unexpected global search request');
    const reply = replies.shift();
    return Promise.resolve({
      ok:reply.status >= 200 && reply.status < 300,
      status:reply.status,
      text:() => Promise.resolve(JSON.stringify(reply.data))
    });
  };
  vm.runInNewContext(clientSource,{window:clientWindow,fetch,console,Promise,Date},
    {filename:'client-api.js'});
  const api = clientWindow.VdrSuiteClientApi;
  const recId = 'rec_0123456789abcdef0123456789abcdef';
  const portraitId = 'rec_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
  const artworkId = 'abcdef0123456789abcdef0123456789';
  const portrait = {recordingId:portraitId,artworkId};
  const rec = {kind:'recording',backendId:'living-room',recordingId:recId,
    title:'Manueller Titel',subtitle:'Folge',startTime:'1791565200',durationSeconds:3600,
    sizeMb:2048,preferredArtworkId:artworkId,personKey:'bruce willis\nactor',
    matchedPerson:'Bruce Willis',matchedRole:'actor',matchReason:'person',personPortrait:portrait};
  const e1 = {kind:'epg',backendId:'living-room',channelId:'C-1',eventId:'12',
    title:'EPG-Titel',subtitle:'Untertitel',startTime:'1791565200',endTime:'1791568800',
    artworkAvailable:true,personKey:'bruce willis\nactor',matchedPerson:'Bruce Willis',
    matchedRole:'actor',personPortrait:portrait};
  const e2 = Object.assign({},e1,{eventId:'13',personKey:'ewan mcgregor\nactor',
    matchedPerson:'Ewan McGregor'});
  const payload = {recordings:{items:[rec],personProjectionVersion:1,
    page:{offset:0,limit:24,totalCount:28,hasMore:true}},
    epg:{items:[e1,e2],personProjectionVersion:1,
    page:{offset:0,limit:24,totalCount:2,hasMore:false}}};
  const signal = new AbortController().signal;
  const options = {backendId:'living-room',query:{query:'Hobbit',limit:24,offset:0,_:123},
    cache:'no-store',signal};
  replies = [{status:200,data:payload}];
  const actual = await api.fetchClientGlobalSearch(options);
  assert.strictEqual(requests.length,1);
  assert.strictEqual(requests[0].url.split('?')[0],'/api/v1/search');
  const query = new URL(requests[0].url,'https://vdr-suite.test').searchParams;
  assert.strictEqual(query.get('backendId'),'living-room');
  assert.strictEqual(query.get('query'),'Hobbit');
  assert.strictEqual(query.get('limit'),'24');
  assert.strictEqual(query.get('offset'),'0');
  assert.strictEqual(query.has('_'),false);
  assert.strictEqual(requests[0].options.signal,signal);
  assert.strictEqual(actual.recordingTotal,28);
  assert.strictEqual(actual.epgTotal,2);
  assert.strictEqual(actual.hasMore,true);
  assert.strictEqual(actual.recordings[0].title,'Manueller Titel');
  assert.strictEqual(actual.recordings[0].recordedAt,'1791565200');
  assert.strictEqual(actual.recordings[0].metadata.subtitle,'Folge');
  assert.strictEqual(actual.recordings[0].preferredArtworkId,artworkId);
  assert.strictEqual(actual.recordings[0].backendNativeId,undefined);
  assert.strictEqual(actual.epg[0].id,'12');
  assert(testApi.epgArtworkSources(actual.epg[0]).length === 1);
  assert.strictEqual(actual.people.length,2);
  assert.strictEqual(actual.people[0].recordingCount,1);
  assert.strictEqual(actual.people[0].epgCount,1);
  assert.strictEqual(actual.people[1].recordingCount,0);
  assert.strictEqual(actual.people[1].epgCount,1);
  assert.strictEqual(actual.people[1].image.url,
    '/api/v1/recordings/'+portraitId+'/artwork/'+artworkId+'?backendId=living-room');
  assert(testApi.publicPersonImageUrl(actual.people[1].image.url));
  assert(!testApi.publicPersonImageUrl('https://foreign.invalid/image'));
  requests.length=0;
  replies=[{status:404,data:{code:'not_found'}},{status:200,data:{recordings:[],epg:[],status:'ready'}}];
  assert.strictEqual((await api.fetchClientGlobalSearch(options)).status,'ready');
  assert.strictEqual(requests.length,2);
  assert.strictEqual(requests[1].url.split('?')[0],'/api/search');
  for (const status of [401,503]) {
    requests.length=0;
    replies=[{status,data:{code:status===401?'authentication_required':'service_unavailable'}}];
    await assert.rejects(api.fetchClientGlobalSearch(options),error=>error.status===status);
    assert.strictEqual(requests.length,1);
  }
  requests.length=0;
  replies=[{status:403,data:{code:'permission_denied'}},{status:200,data:{recordings:[],epg:[],status:'ready'}}];
  await api.fetchClientGlobalSearch(options);
  assert.strictEqual(requests.length,2);
  assert.strictEqual(requests[1].url.split('?')[0],'/api/search');
  requests.length=0;
  replies=[{status:403,data:{code:'actor_revoked'}}];
  await assert.rejects(api.fetchClientGlobalSearch(options),error=>error.status===403);
  assert.strictEqual(requests.length,1);
  requests.length=0;
  const bad=JSON.parse(JSON.stringify(payload));
  bad.recordings.items[0].backendId='other';
  replies=[{status:200,data:bad}];
  await assert.rejects(api.fetchClientGlobalSearch(options),/Aufnahmeidentität/);
  assert.strictEqual(requests.length,1);

  console.log('global search frontend runtime tests passed');
})().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
