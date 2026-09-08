'use strict';
const assert = require('node:assert/strict');
const { summarize, createProbe } = require('./browser_artwork_probe');

const entries = [
  { name: 'https://example.test/cover?id=1', initiatorType: 'img', transferSize: 120, encodedBodySize: 100, decodedBodySize: 200, responseStatus: 200 },
  { name: 'https://example.test/cover?id=1', initiatorType: 'img', transferSize: 0, encodedBodySize: 100, decodedBodySize: 200, responseStatus: 304 },
  { name: 'https://example.test/api', initiatorType: 'fetch', transferSize: 50 }
];
const images = [{ url: entries[0].name }, { url: entries[0].name }];
const result = summarize(entries, images, 3, [{ duration: 60 }]);
assert.equal(result.imageElements, 2);
assert.equal(result.uniqueImageUrls, 1);
assert.equal(result.duplicateImageElements, 1);
assert.equal(result.resourceRequests, 2);
assert.equal(result.repeatedResourceRequests, 1);
assert.equal(result.transferBytes, 120);
assert.equal(result.encodedBytes, 200);
assert.equal(result.zeroTransferEntries, 1);
assert.equal(result.longTaskMs, 60);
assert.equal(result.mutations, 3);
assert.equal(JSON.stringify(result).includes('cover?id=1'), false);

let elapsed = 0;
const observers = [];
class Observer {
  constructor(callback) { this.callback = callback; this.pending = []; observers.push(this); }
  observe(options) { this.options = options; }
  takeRecords() { const records = this.pending; this.pending = []; return records; }
  disconnect() { this.disconnected = true; }
}
const image = { src: entries[0].name, currentSrc: entries[0].name, complete: true, naturalWidth: 100, loading: 'lazy' };
const env = {
  document: { querySelectorAll(selector) { return selector === 'img' ? [image] : [{ scrollLeft: 30 }]; } },
  performance: { now() { return elapsed; } },
  PerformanceObserver: Observer,
  MutationObserver: Observer,
  location: { href: 'https://example.test/app', origin: 'https://example.test' },
  console: { table() {} },
  fetch: async function () { throw new Error('Unexpected network request'); }
};
const probe = createProbe(env);
probe.start('warm');
assert.throws(() => probe.start('overlap'), /Finish/);
observers[0].callback({ getEntries() { return [entries[0]]; } });
observers[1].pending.push({ entryType: 'longtask', duration: 70 });
observers[2].callback([{}]);
elapsed = 80;
const capture = probe.stop();
assert.equal(capture.metrics.resourceRequests, 1);
assert.equal(capture.metrics.longTasks, 1);
assert.equal(capture.metrics.mutations, 1);
assert.equal(capture.completeImages, 1);
assert.equal(capture.lazyImages, 1);
assert.deepEqual(capture.railScrollBefore, [30]);
assert.equal(probe.report().length, 1);
assert.ok(observers.every(observer => observer.disconnected));
assert.throws(() => probe.stop(), /No active/);
console.log('browser artwork probe aggregation and lifecycle ok');
