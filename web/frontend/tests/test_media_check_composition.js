'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');
const clientFactory = require('../api/public-v1-client.js');
const hero = fs.readFileSync('web/frontend/recordings2-hero-detail.js', 'utf8');
const packaging = fs.readFileSync('mk/recordings2.mk', 'utf8');
assert(hero.includes("makeButton('Aufnahme prüfen'"));
assert(hero.includes("makeButton('HEVC-Vorprüfung'"));
assert(hero.includes("renderMediaShrinkPlan(root, copy, recording, backendId, actions);"));
assert(hero.includes("client.submitMediaShrinkPlan({"));
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
let ambiguousRecordings = false;
function response(status, payload) {
  return {status, ok: status >= 200 && status < 300,
    headers: {get() {return null;}},
    text() {return Promise.resolve(JSON.stringify(payload));}};
}
const api = clientFactory.createClient({
  fetch(url, options) {
    requests.push({url, options});
    if (url.endsWith('/capabilities/media-shrink'))
      return Promise.resolve(response(200, {
        backendId: backend, capability: 'addons.media.shrink',
        planningAvailable: true, available: false
      }));
    if (url.endsWith('/media-shrink-plans'))
      return Promise.resolve(response(200, {
        backendId: backend, recordingId: id,
        readOnly: true, stageAvailable: false, sourceBytes: 32768,
        segments: 1
      }));
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
    if (url.startsWith('/api/v1/recordings?')) {
      const items = [{
        recordingId: id, backendId: backend, recordedAt: '2026-10-22T10:00:00Z',
        title: 'Beispiel'
      }];
      if (ambiguousRecordings) items.push({
        recordingId: 'rec_' + 'c'.repeat(32), backendId: backend,
        recordedAt: '2026-10-22T10:00:00Z', title: 'Anderer Titel'
      });
      return Promise.resolve(response(200, {
        items, page: {hasMore: false}
      }));
    }
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
  const shrinkCapability =
    await api.getMediaShrinkCapability({backendId: backend});
  assert.strictEqual(shrinkCapability.planningAvailable, true);
  assert.strictEqual(shrinkCapability.available, false);
  assert.throws(() => api.submitMediaShrinkPlan({
    backendId: backend, recordingId: '/tmp/bad.rec'
  }));
  const plan = await api.submitMediaShrinkPlan({
    backendId: backend, recordingId: id,
    headers: {'X-CSRF-Token': 'hevc-csrf'}
  });
  assert.strictEqual(plan.readOnly, true);
  assert.strictEqual(plan.stageAvailable, false);
  const sentPlan = requests[requests.length - 1];
  assert(sentPlan.url.endsWith('/media-shrink-plans'));
  assert.strictEqual(sentPlan.options.method, 'POST');
  assert.strictEqual(sentPlan.options.headers['X-CSRF-Token'], 'hevc-csrf');
  assert.deepStrictEqual(JSON.parse(sentPlan.options.body),
    {recordingId: id});
  const context = {
    window: null, console,
    VdrSuiteRecordings2Shared: {
      text(value) { return value == null ? '' : String(value); },
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
  const normalizedTitle = await functions.resolveMediaCheckIdentity(
    api, {title: 'Beispiel (Metadaten)', startTime: '2026-10-22T10:00:00Z'},
    backend);
  assert.strictEqual(normalizedTitle, id);
  ambiguousRecordings = true;
  const exactTitle = await functions.resolveMediaCheckIdentity(
    api, {title: 'Beispiel', startTime: '2026-10-22T10:00:00Z'}, backend);
  assert.strictEqual(exactTitle, id);
  await assert.rejects(function () {
    return functions.resolveMediaCheckIdentity(api, {
      title: 'Kein Treffer', startTime: '2026-10-22T10:00:00Z'
    }, backend);
  });
  assert(functions.mediaCheckSummary({status: 'errors', segments: 2,
    segmentReports: [{name: '00002.ts', status: 'errors'}]})
    .includes('00002.ts'));
  const enriched = functions.mediaCheckSummary({
    status: 'warnings', segments: 2,
    sidecarReports: [{name: 'de.srt', status: 'warnings'}],
    ffprobe: {status: 'ok', details: JSON.stringify({
      streams: [{codec_type: 'video', codec_name: 'h264',
        width: 1920, height: 1080},
        {codec_type: 'audio', codec_name: 'aac', channels: 2}],
      format: {format_name: 'mpegts'}
    })}
  });
  assert(enriched.includes('de.srt'));
  assert(enriched.includes('h264 1920×1080'));
  assert(enriched.includes('aac (2 Kanäle)'));
  assert(enriched.includes('Container: mpegts'));
  console.log('MEDIA_CHECK_API_V1_FRONTEND=PASS');
})().catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
