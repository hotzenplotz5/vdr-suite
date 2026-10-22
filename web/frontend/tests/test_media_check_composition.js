'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');
const clientFactory = require('../api/public-v1-client.js');
const hero = fs.readFileSync('web/frontend/recordings2-hero-detail.js', 'utf8');
const packaging = fs.readFileSync('mk/recordings2.mk', 'utf8');
assert(hero.includes("makeButton('Aufnahme prüfen'"));
assert(hero.includes('renderMediaCheck(root, copy, recording, backendId, actions);'));
assert(hero.includes('VdrSuiteBrowserSession'));
assert(hero.includes('session.csrfHeaders()'));
assert(hero.includes('mediaCheckSummary(result.report)'));
assert(hero.includes('resolveMediaCheckIdentity(client, recording, backendId)'));
assert(!hero.includes('innerHTML'));
assert(packaging.includes('node web/frontend/tests/test_media_check_composition.js'));

const backend = 'local';
const id = 'rec_' + 'a'.repeat(32);
const checkId = 'chk_' + 'b'.repeat(32);
const requests = [];
function response(status, payload) {
  return {status, ok: status >= 200 && status < 300,
    headers: {get() {return null;}},
    text() {return Promise.resolve(JSON.stringify(payload));}};
}
const api = clientFactory.createClient({
  fetch(url, options) {
    requests.push({url, options});
    if (url.endsWith('/capabilities/media-check'))
      return Promise.resolve(response(200, {
        backendId: backend, capability: 'addons.media.check', available: true
      }));
    if (url.endsWith('/media-checks/' + checkId))
      return Promise.resolve(response(200, {checkId, backendId: backend,
        state: 'completed', report: {status: 'ok', segments: 2}}));
    if (url.endsWith('/media-checks'))
      return Promise.resolve(response(202, {checkId, backendId: backend,
        state: 'running'}));
    if (url.startsWith('/api/v1/recordings?'))
      return Promise.resolve(response(200, {items: [{
        recordingId: id, backendId: backend, recordedAt: '2026-10-22T10:00:00Z',
        title: 'Beispiel'
      }], page: {hasMore: false}}));
    throw Error('Unexpected URL ' + url);
  }, credentials: 'same-origin'
});

(async function () {
  const capability = await api.getMediaCheckCapability({backendId: backend});
  assert.strictEqual(capability.available, true);
  assert.throws(() => api.submitMediaCheck({backendId: backend,
    recordingId: '/srv/vdr/movie.rec', mode: 'decode'}));
  const accepted = await api.submitMediaCheck({backendId: backend,
    recordingId: id, mode: 'decode', headers: {'X-CSRF-Token': 'abc'}});
  assert.strictEqual(accepted.checkId, checkId);
  assert.strictEqual(requests[1].options.method, 'POST');
  assert.strictEqual(requests[1].options.headers['X-CSRF-Token'], 'abc');
  assert.deepStrictEqual(JSON.parse(requests[1].options.body),
    {recordingId: id, mode: 'decode'});
  const report = await api.getMediaCheckStatus({backendId: backend, checkId});
  assert.strictEqual(report.report.status, 'ok');
  const context = {
    window: null, console,
    VdrSuiteRecordings2Shared: {
      first(item, names, fallback) {
        for (const name of names) if (item && item[name]) return item[name];
        return fallback;
      }
    }
  };
  context.window = context;
  vm.createContext(context);
  vm.runInContext(hero, context);
  const functions = context.VdrSuiteRecordings2HeroDetail.__test;
  const direct = await functions.resolveMediaCheckIdentity(
    api, {recordingId: id}, backend);
  assert.strictEqual(direct, id);
  const resolved = await functions.resolveMediaCheckIdentity(
    api, {title: 'Beispiel', startTime: '2026-10-22T10:00:00Z'}, backend);
  assert.strictEqual(resolved, id);
  assert(functions.mediaCheckSummary({status: 'errors', segments: 2,
    segmentReports: [{name: '00002.ts', status: 'errors'}]})
    .includes('00002.ts'));
  console.log('MEDIA_CHECK_API_V1_FRONTEND=PASS');
})().catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
