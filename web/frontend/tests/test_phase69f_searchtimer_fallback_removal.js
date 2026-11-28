'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const clientPath = path.resolve(__dirname, '..', 'api', 'client-api.js');
const source = fs.readFileSync(clientPath, 'utf8');
const requests = [];

function response(status, payload) {
  return {
    ok: status >= 200 && status < 300,
    status,
    text() {
      return Promise.resolve(JSON.stringify(payload));
    }
  };
}

const context = {
  window: null,
  Headers,
  URLSearchParams,
  Promise,
  Object,
  Array,
  String,
  JSON,
  Error,
  Number,
  Boolean,
  fetch(url, options) {
    requests.push({url: String(url), options});
    return Promise.resolve(response(503, {
      error: {
        code: 'backend_unavailable',
        message: 'Backend unavailable',
        requestId: 'req-phase69f-searchtimer'
      }
    }));
  }
};
context.window = context;

vm.createContext(context);
vm.runInContext(source, context, {filename: clientPath});

const api = context.VdrSuiteClientApi;
assert.ok(api);

async function captureFailure(call) {
  try {
    await call();
  } catch (error) {
    return error;
  }
  assert.fail('SearchTimer request unexpectedly succeeded');
}

async function verifyOneShot(functionName, expectedPath, expectedMethod, options) {
  const before = requests.length;
  const failure = await captureFailure(function () {
    return api[functionName](options);
  });
  const emitted = requests.slice(before);

  assert.strictEqual(api.isClientError(failure), true);
  assert.strictEqual(failure.status, 503);
  assert.strictEqual(failure.code, 'backend_unavailable');
  assert.strictEqual(failure.requestId, 'req-phase69f-searchtimer');
  assert.strictEqual(emitted.length, 1);

  const parsed = new URL(emitted[0].url, 'https://vdr-suite.test/');
  assert.strictEqual(parsed.pathname, expectedPath);
  assert.strictEqual(emitted[0].options.method, expectedMethod);

  if (options && options.backendId) {
    assert.strictEqual(parsed.searchParams.get(expectedPath === '/api/v1/search-timers/preview-cache/refresh' ? 'backendId' : 'backend'), options.backendId);
  }
}

async function run() {
  assert.ok(!source.includes('function requestJsonWithFallbacks(paths, options)'));
  assert.ok(!source.includes('/api/vdr/searchtimers/live'));

  for (const name of ["fetchClientTimerCreateAction","fetchClientTimerUpdateAction","fetchClientTimerDeleteAction","fetchClientSearchTimers","fetchClientSearchTimerDiscovery","fetchClientSearchTimerPreview","fetchClientSearchTimerPlan","fetchClientSearchTimerValidate","fetchClientSearchTimerExecute","fetchClientSearchTimerRealTest","fetchClientSearchTimerUpdateAction","fetchClientSearchTimerDeleteAction"]) {
    assert.strictEqual(typeof api[name], 'undefined', 'retired wrapper returned: ' + name);
  }
  assert.ok(!source.includes('/api/vdr/searchtimers'));
  assert.ok(!source.includes('/api/searchtimers'));
  assert.ok(!source.includes('/api/vdr/timers/actions/'));

  await verifyOneShot(
    'fetchPublicSearchTimerDraftPreview',
    '/api/v1/search-timers/draft-preview',
    'GET',
    {backend: 'living-room', limit: 40, payload: {query:'Tatort & Co',mode:4,
      compareTitle:true,compareSubtitle:false,compareSummary:true,useChannel:2,
      channels:'Öffentlich',useTime:true,startTime:2200,stopTime:200,
      useDuration:true,durationMinMinutes:30,durationMaxMinutes:130,
      useDayOfWeek:true,dayOfWeek:-62,blacklistIds:'4',priority:50}}
  );
  const draft = requests.at(-1);
  const draftParams = new URL(draft.url,'https://vdr-suite.test/').searchParams;
  for (const [key,value] of Object.entries({backend:'living-room',query:'Tatort & Co',mode:'4',
    limit:'40',compareTitle:'1',compareSubtitle:'0',compareSummary:'1',useChannel:'2',
    channels:'Öffentlich',useTime:'1',startTime:'2200',stopTime:'200',useDuration:'1',
    durationMinMinutes:'30',durationMaxMinutes:'130',useDayOfWeek:'1',dayOfWeek:'-62'}))
    assert.strictEqual(draftParams.get(key),value);
  assert(!draftParams.has('blacklistIds') && !draftParams.has('priority') && !draftParams.has('tolerance'));
  assert.strictEqual(draft.options.credentials,'same-origin');
  await verifyOneShot(
    'fetchClientSearchTimerPreviewCacheRefresh',
    '/api/v1/search-timers/preview-cache/refresh',
    'POST',
    {backendId: 'living-room'}
  );
  const id = 'st_' + 'a'.repeat(32);
  const revision = 'sr_' + 'b'.repeat(32);
  const backend = 'living-room';
  context.VdrSuiteBrowserSession = {csrfHeaders() {
    return {'X-CSRF-Token': 'test-csrf'};
  }};
  await verifyOneShot('fetchPublicSearchTimerPage', '/api/v1/search-timers',
    'GET', {backend, limit: 30, cursor: 'cursor-example'});
  const pageRequest = new URL(requests[requests.length - 1].url, 'https://vdr-suite.test/');
  assert.strictEqual(pageRequest.searchParams.get('backend'), backend);
  assert.strictEqual(pageRequest.searchParams.get('limit'), '30');
  assert.strictEqual(pageRequest.searchParams.get('cursor'), 'cursor-example');
  await verifyOneShot('fetchPublicSearchTimerDetail', '/api/v1/search-timers/' + id,
    'GET', {backend, searchTimerId: id});
  await verifyOneShot('fetchPublicSearchTimerPreview', '/api/v1/search-timers/' + id + '/preview',
    'GET', {backend, searchTimerId: id});
  await verifyOneShot('fetchPublicSearchTimerVersion', '/api/v1/search-timers/' + id + '/version',
    'GET', {backend, searchTimerId: id});
  for (const action of ['update', 'activate', 'deactivate', 'delete']) {
    const payload = {action, confirmed: true};
    if (action === 'update') payload.query = 'Tatort';
    await verifyOneShot('mutatePublicSearchTimer',
      '/api/v1/search-timers/' + id + '/mutations', 'POST',
      {backend, searchTimerId: id, revision, idempotencyKey: 'mutation-' + action, payload});
    const request = requests[requests.length - 1];
    assert.strictEqual(new URL(request.url, 'https://vdr-suite.test/').searchParams.get('backend'), backend);
    assert.strictEqual(request.options.headers['If-Match'], '"' + revision + '"');
    assert.strictEqual(request.options.headers['Idempotency-Key'], 'mutation-' + action);
    assert.strictEqual(request.options.headers['X-CSRF-Token'], 'test-csrf');
    assert.strictEqual(request.options.headers['Content-Type'], 'application/json');
    assert.deepStrictEqual(JSON.parse(request.options.body), payload);
  }
  const previousCount = requests.length;
  await captureFailure(() => api.mutatePublicSearchTimer({backend, searchTimerId: '0'}));
  assert.strictEqual(requests.length, previousCount);

  // The editor catalog reuses the real Public-v1 client, follows every
  // page in VDR order and retains the private read only for its existing
  // permission/compatibility boundary. Ordinary browser consumers are unchanged.
  const publicPath = path.resolve(__dirname, '../../../clients/reference-js/public-v1-client.js');
  vm.runInContext(fs.readFileSync(publicPath, 'utf8'), context, {filename: publicPath});
  const originalFetch = context.fetch;
  const editorOptions = {editorCatalog:true, query:{backend:'living-room'}, cache:'no-store'};
  const channel = (id, number) => ({backendId:'living-room',channelId:id,
    channelNumber:number,name:id,groupName:'TV',enabled:true,radio:false});
  let calls = [];
  let replies = [];
  context.fetch = (url, options) => {
    calls.push(String(url));
    assert.strictEqual(options.method,'GET');
    assert.ok(replies.length, 'unexpected catalog request');
    return Promise.resolve(replies.shift());
  };
  try {
    replies = [
      response(200,{items:[channel('a',1)],page:{hasMore:true,nextCursor:'ch2_next'},meta:{partial:false}}),
      response(200,{items:[channel('b',2)],page:{hasMore:false,nextCursor:null},meta:{partial:false}})
    ];
    const catalog = await api.fetchClientChannels(editorOptions);
    assert.strictEqual(JSON.stringify(catalog.channels.map(x=>x.channelId)), '["a","b"]');
    assert.strictEqual(calls.length,2);
    const first = new URL(calls[0],'https://vdr-suite.test');
    assert.strictEqual(first.pathname,'/api/v1/channels');
    assert.strictEqual(first.searchParams.get('backendId'),'living-room');
    assert.strictEqual(first.searchParams.get('sort'),'backendId,channelNumber,channelId');
    assert.strictEqual(new URL(calls[1],'https://vdr-suite.test').searchParams.get('cursor'),'ch2_next');
    calls = [];
    replies = [response(403,{code:'permission_denied'}),response(200,{channels:[channel('legacy',3)]})];
    assert.strictEqual((await api.fetchClientChannels(editorOptions)).channels[0].channelId,'legacy');
    assert.strictEqual(new URL(calls[1],'https://vdr-suite.test').pathname,'/api/vdr/channels');
    calls = [];
    replies = [response(503,{code:'backend_unavailable'})];
    await assert.rejects(api.fetchClientChannels(editorOptions), e=>e.status===503);
    assert.strictEqual(calls.length,1);
    calls = [];
    replies = [response(200,{channels:[]})];
    await api.fetchClientChannels({query:{backend:'living-room'}});
    assert.strictEqual(new URL(calls[0],'https://vdr-suite.test').pathname,'/api/vdr/channels');
  } finally { context.fetch = originalFetch; }

  console.log('test_phase69f_searchtimer_fallback_removal passed');
}

run().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
