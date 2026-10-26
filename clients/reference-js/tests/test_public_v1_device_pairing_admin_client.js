'use strict';
const assert = require('assert');
const {createClient} = require('../public-v1-client.js');

const pairingRequestId = 'dpr_' + 'a'.repeat(32);
const calls = [];
let failDecision = false;
function response(status, body, etag) {
  return {ok: status >= 200 && status < 300, status,
    headers: {get(name) { return name.toLowerCase() === 'etag' ? etag || null : null; }},
    text() { return Promise.resolve(JSON.stringify(body)); }};
}
async function fetch(url, options) {
  calls.push({url, options});
  if (url.includes('device-pairings?')) return response(200, {
    items:[{pairingRequestId,status:'pending',expiresAt:'2026-10-25T06:15:00Z',
      client:{displayName:'Samsung TV',clientKind:'tizen',appVersion:'S3'}}],
    page:{limit:25,hasMore:false,nextCursor:null}
  });
  if (options.method === 'GET') return response(200, {
    pairingRequestId,status:'pending',expiresAt:'2026-10-25T06:15:00Z',
    client:{displayName:'Samsung TV',clientKind:'tizen',appVersion:'S3'}
  }, '"revision-1"');
  if (failDecision) return response(412, {code:'revision_conflict'});
  return response(200, {pairingRequestId,status:JSON.parse(options.body).decision === 'approve' ? 'approved':'rejected'}, '"revision-2"');
}
async function run() {
  const api = createClient({baseUrl:'https://example.test/vdr-suite',fetch,credentials:'same-origin'});
  const listed = await api.listDevicePairingsForAdmin({
    query:{limit:25,sort:'pairingRequestId',order:'asc'},cache:'no-store'
  });
  assert.strictEqual(listed.items.length,1);
  assert.strictEqual(calls[0].url,
    'https://example.test/vdr-suite/api/v1/device-pairings?limit=25&sort=pairingRequestId&order=asc');
  assert.strictEqual(calls[0].options.credentials,'same-origin');
  assert.strictEqual(calls[0].options.method,'GET');
  const item = await api.getDevicePairingForAdmin({pairingRequestId});
  assert.strictEqual(item.etag,'"revision-1"');
  assert(!Object.keys(calls[1].options.headers).some(k => /pairing-token/i.test(k)));
  const decided = await api.decideDevicePairingForAdmin({
    pairingRequestId,decision:'approve',ifMatch:item.etag,
    headers:{'X-CSRF-Token':'mock-session-csrf'},credentials:'same-origin'
  });
  assert.strictEqual(decided.data.status,'approved');
  assert.strictEqual(calls[2].url,'https://example.test/vdr-suite/api/v1/device-pairings/'+pairingRequestId);
  assert.strictEqual(calls[2].options.method,'POST');
  assert.strictEqual(calls[2].options.headers['If-Match'],'"revision-1"');
  assert.strictEqual(calls[2].options.headers['X-CSRF-Token'],'mock-session-csrf');
  assert.deepStrictEqual(JSON.parse(calls[2].options.body),{decision:'approve'});
  assert.strictEqual(calls[2].options.credentials,'same-origin');
  assert(!Object.keys(calls[2].options.headers).some(k => /pairing-token/i.test(k)));

  failDecision = true;
  await assert.rejects(api.decideDevicePairingForAdmin({
    pairingRequestId,decision:'reject',ifMatch:item.etag
  }),e => e.status === 412);
  const writes = calls.filter(x => x.options.method === 'POST').length;
  assert.throws(() => api.decideDevicePairingForAdmin({pairingRequestId,
    decision:'invalid',ifMatch:item.etag}),/invalid administrator pairing decision/);
  assert.throws(() => api.getDevicePairingForAdmin({pairingRequestId,
    pairingToken:'must-not-leak'}),/bootstrap token/);
  assert.throws(() => api.listDevicePairingsForAdmin({query:{limit:200}}),/limit/);
  assert.throws(() => api.listDevicePairingsForAdmin({query:{cursor:'../etc'}}),/cursor/);
  assert.strictEqual(calls.filter(x=>x.options.method==='POST').length,writes);
}
run().catch(e=>{console.error(e);process.exitCode=1;});
