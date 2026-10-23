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
const shrinkReviewId = 'shk_' + 'c'.repeat(32);
const shrinkOperationId = 'op_' + 'd'.repeat(32);
const shrinkPublishOperationId = 'op_' + 'e'.repeat(32);
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
    if (url.endsWith('/media-shrink-stages'))
      return Promise.resolve(response(202, {
        operationId: shrinkOperationId, backendId: backend, state: 'dispatching'
      }));
    if (url.endsWith('/media-shrink-publications'))
      return Promise.resolve(response(202, {
        operationId: shrinkPublishOperationId, backendId: backend,
        state: 'dispatching'
      }));
    if (url.endsWith('/media-shrink-operations/' + shrinkOperationId))
      return Promise.resolve(response(200, {
        operationId: shrinkOperationId, backendId: backend,
        action: 'stage', state: 'succeeded', resultReconciled: true,
        reviewId: shrinkReviewId
      }));
    if (url.endsWith('/media-shrink-operations/' + shrinkPublishOperationId))
      return Promise.resolve(response(200, {
        operationId: shrinkPublishOperationId, backendId: backend,
        action: 'publish', state: 'dispatching', resultReconciled: false
      }));
    if (url.endsWith('/media-shrink-reviews/' + shrinkReviewId))
      return Promise.resolve(response(200, {
        backendId: backend, recordingId: id, reviewId: shrinkReviewId,
        state: 'staged', revision: 1, readOnly: true, publishAvailable: false
      }));
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
  assert.throws(() => api.getMediaShrinkReview({
    backendId: backend, reviewId: '/tmp/staged.rec'
  }));
  const review = await api.getMediaShrinkReview({
    backendId: backend, reviewId: shrinkReviewId
  });
  assert.strictEqual(review.state, 'staged');
  assert.strictEqual(review.publishAvailable, false);
  assert.strictEqual(review.readOnly, true);
  const reviewRequest = requests[requests.length - 1];
  assert(reviewRequest.url.endsWith('/media-shrink-reviews/' + shrinkReviewId));
  assert.notStrictEqual(reviewRequest.options.method, 'POST');
  // Production v1 client: opaque Stage -> Operation -> Review -> explicit
  // Publish -> independent Operation. Never allow a client-supplied path.
  const mutationKey = 'hevc-' + '01234567';
  assert.throws(() => api.submitMediaShrinkStage({
    backendId: backend, recordingId: '/srv/vdr/malicious.rec',
    encoder: 'software', maxHeight: 720, nightMode: false,
    idempotencyKey: mutationKey
  }));
  assert.throws(() => api.submitMediaShrinkStage({
    backendId: backend, recordingId: id,
    encoder: 'unsafe;rm', maxHeight: 720, nightMode: false,
    idempotencyKey: mutationKey
  }));
  assert.throws(() => api.submitMediaShrinkStage({
    backendId: backend, recordingId: id,
    encoder: 'software', maxHeight: 720, nightMode: false,
    idempotencyKey: ''
  }));
  const stageResult = await api.submitMediaShrinkStage({
    backendId: backend, recordingId: id, encoder: 'software',
    maxHeight: 720, nightMode: true, idempotencyKey: mutationKey,
    headers: {'X-CSRF-Token': 'stage-csrf'}
  });
  assert.strictEqual(stageResult.operationId, shrinkOperationId);
  const stageRequest = requests[requests.length - 1];
  assert.strictEqual(stageRequest.options.method, 'POST');
  assert(stageRequest.url.endsWith('/media-shrink-stages'));
  assert.strictEqual(stageRequest.options.headers['Idempotency-Key'], mutationKey);
  assert.strictEqual(stageRequest.options.headers['X-CSRF-Token'], 'stage-csrf');
  assert.strictEqual(stageRequest.options.body, JSON.stringify({
    recordingId: id, encoder: 'software', maxHeight: 720, nightMode: true
  }));
  assert.throws(() => api.getMediaShrinkOperation({
    backendId: backend, operationId: '/tmp/operation'
  }));
  const stageStatus = await api.getMediaShrinkOperation({
    backendId: backend, operationId: shrinkOperationId
  });
  assert.strictEqual(stageStatus.resultReconciled, true);
  assert.strictEqual(stageStatus.reviewId, shrinkReviewId);
  assert.throws(() => api.submitMediaShrinkPublish({
    backendId: backend, reviewId: shrinkReviewId,
    revision: 2, idempotencyKey: mutationKey
  }));
  assert.throws(() => api.submitMediaShrinkPublish({
    backendId: backend, reviewId: '/tmp/candidate.rec',
    revision: 1, idempotencyKey: mutationKey
  }));
  const publishResult = await api.submitMediaShrinkPublish({
    backendId: backend, reviewId: shrinkReviewId, revision: 1,
    idempotencyKey: 'hevc-publish-1234'
  });
  assert.strictEqual(publishResult.operationId, shrinkPublishOperationId);
  const publishRequest = requests[requests.length - 1];
  assert(publishRequest.url.endsWith('/media-shrink-publications'));
  assert.strictEqual(publishRequest.options.body,
    JSON.stringify({reviewId: shrinkReviewId, revision: 1}));
  const publishStatus = await api.getMediaShrinkOperation({
    backendId: backend, operationId: shrinkPublishOperationId
  });
  assert.strictEqual(publishStatus.resultReconciled, false);

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
  assert(hero.includes("client.submitMediaShrinkStage({"));
  assert(hero.includes("client.submitMediaShrinkPublish({"));
  assert(hero.includes("client.getMediaShrinkOperation({"));
  assert(hero.includes("global.confirm("));
  assert(hero.includes("remember('stage', {key: key, operationId: ''})"));
  assert(hero.includes("remember('publish', {key: key, operationId: ''})"));
  console.log('MEDIA_CHECK_API_V1_FRONTEND=PASS HEVC_STAGE_PUBLISH_CLIENT=PASS');
})().catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
