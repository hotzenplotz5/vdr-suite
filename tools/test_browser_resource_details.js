'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.resolve(__dirname, '../web/frontend/browser-artwork-probe.js'), 'utf8');
const { summarizeResources, summarize, createProbe } = require('../web/frontend/browser-artwork-probe');
const base = 'https://example.test/vdr-suite/frontend/';
const secret = 'private-recording-123';
const entry = (path, status, size, initiator = 'fetch') => ({
  name: new URL(path, base).href, initiatorType: initiator,
  responseStatus: status, transferSize: size + 300,
  encodedBodySize: size, decodedBodySize: size, entryType: 'resource'
});
const entries = [
  entry('/vdr-suite/api/vdr/recordings/metadata?backendNativeId=' + secret, 200, 4096),
  entry('/vdr-suite/api/vdr/recordings/metadata?backendNativeId=' + secret, 200, 4096),
  entry('/vdr-suite/api/recordings/metadata?backendNativeId=another', 404, 0),
  entry('/vdr-suite/api/vdr/recordings/query?title=' + secret, 200, 80000),
  entry('/vdr-suite/api/vdr/recordings/folders?path=' + secret, 200, 500),
  entry('/vdr-suite/api/vdr/recordings?backend=default', 200, 900),
  entry('/vdr-suite/api/vdr/recordings/persons/search?name=' + secret, 200, 100),
  entry('/vdr-suite/api/backends/' + secret + '/recordings/metadata/seasons', 200, 300),
  entry('/vdr-suite/api/vdr/recordings/metadata/image?kind=poster&index=0&backendNativeId=' + secret + '&assignmentRevision=' + secret + '&variant=home', 200, 1048577, 'img'),
  entry('/vdr-suite/api/vdr/recordings/metadata/image?kind=poster&index=0&backendNativeId=' + secret + '&assignmentRevision=' + secret + '&variant=home', 200, 1048577, 'img'),
  entry('/vdr-suite/api/recordings/metadata/image?kind=' + secret + '&backendNativeId=' + secret + '&variant=full', 404, 0, 'img'),
  entry('/vdr-suite/recording-artwork/' + secret + '?kind=poster&variant=home', 200, 1000, 'img'),
  entry('/vdr-suite/api/epg/cache/metadata/image?kind=banner&eventId=' + secret, 200, 2000, 'img'),
  entry('https://outside.test/' + secret, 0, 0, 'img')
];
const report = summarizeResources(entries, base);
assert.equal(report.observedEntries, entries.length);
assert.equal(report.rows.reduce((sum, row) => sum + row.count, 0), entries.length);
const recording = report.recordingApiDetails;
const artwork = report.artworkVariantDetails;
const find = (rows, category) => rows.filter(row => row.category === category);
assert.equal(find(recording, 'metadata-read').reduce((n, row) => n + row.count, 0), 3);
assert.equal(find(recording, 'metadata-read').find(row => row.status === '200').repeatedRequests, 1);
assert.equal(find(recording, 'metadata-read').find(row => row.status === '404').count, 1);
for (const category of ['recording-query', 'folder-read', 'recording-list', 'person-search', 'manual-metadata-workflow']) {
  assert.equal(find(recording, category).length, 1, category);
}
assert.equal(recording.reduce((n, row) => n + row.count, 0), 8);
const posters = artwork.find(row => row.category === 'recording-metadata-image' && row.variant === 'poster');
assert.equal(posters.count, 2);
assert.equal(posters.previewVariant, 'home');
assert.equal(posters.revision, 'revisioned');
assert.equal(posters.repeatedRequests, 1);
assert.equal(posters.sizeDistribution['over-1MiB'], 2);
assert.equal(artwork.find(row => row.status === '404').variant, 'other');
assert.equal(artwork.find(row => row.status === '404').previewVariant, 'other');
assert.equal(find(artwork, 'recording-artwork')[0].previewVariant, 'home');
assert.equal(find(artwork, 'recording-artwork')[0].revision, 'unversioned');
assert.equal(find(artwork, 'epg-metadata-image')[0].previewVariant, 'unspecified');
assert.equal(artwork.reduce((n, row) => n + row.count, 0), 5);
assert.ok(!JSON.stringify(report).includes(secret));
assert.ok(!JSON.stringify(report).includes('example.test'));
assert.ok(!JSON.stringify(report).includes('backendNativeId'));
assert.ok(!JSON.stringify(report).includes('assignmentRevision='));
assert.ok(!JSON.stringify(report).includes('https://'));
const old = summarize(entries, [], 3, [{duration: 60}], base);
assert.equal(old.resourceRequests, 6);
assert.equal(old.longTaskMs, 60);
assert.equal(old.mutations, 3);
assert.equal(old.resourceDiagnostics.recordingApiDetails.length, recording.length);
const unknown = summarizeResources([entry('/api/vdr/recordings/metadata?secret=' + secret, undefined, 0)], base);
assert.equal(unknown.recordingApiDetails[0].status, 'unavailable');
assert.equal(unknown.recordingApiDetails[0].sizeDistribution.zero, 1);
const many = Array.from({length: 1000}, (_, i) => entry('/api/vdr/recordings/metadata?backendNativeId=' + secret + i, 200, 1));
const bounded = summarizeResources(many, base);
assert.equal(bounded.recordingApiDetails.length, 1);
assert.equal(bounded.recordingApiDetails[0].count, 1000);
assert.ok(!JSON.stringify(bounded).includes(secret));
const ctx = {window: {document: {}, performance: {}, location: {href: base}}, URL, Map, Set, Number, Array, Object, String, JSON};
vm.runInNewContext(source, ctx, {filename: 'browser-artwork-probe.js'});
assert.equal(typeof ctx.window.VdrSuiteArtworkProbe.start, 'function');
assert.ok(source.includes('resourceDiagnostics: summarizeResources(entries, baseUrl)'));
assert.ok(!source.includes('fetch(entry.name'));
let now = 0;
const observers = [];
class Observer {
  constructor(callback) { this.callback = callback; this.pending = []; observers.push(this); }
  observe(options) { this.options = options; }
  takeRecords() { const result = this.pending; this.pending = []; return result; }
  disconnect() { this.disconnected = true; }
}
class MutationObserverMock extends Observer {
  observe(target, options) {
    assert.equal(target, env.document.documentElement);
    assert.equal(arguments.length, 2);
    assert.equal(options.subtree, true);
  }
}
const env = {
  document: {documentElement: {}, querySelectorAll() { return []; }},
  performance: {now() { return now; }}, PerformanceObserver: Observer,
  MutationObserver: MutationObserverMock, location: {href: base, origin: 'https://example.test'},
  fetch() { throw new Error('Unexpected network request'); }
};
const probe = createProbe(env);
probe.start('initial-home', {buffered: true});
assert.equal(observers[0].options.buffered, true);
observers[0].pending.push(...entries);
observers[2].pending.push({});
now = 100;
const capture = probe.stop();
assert.equal(capture.metrics.resourceDiagnostics.recordingApiDetails.length, recording.length);
assert.equal(capture.metrics.resourceDiagnostics.artworkVariantDetails.length, artwork.length);
assert.equal(capture.metrics.mutations, 1);
assert.ok(observers.every(observer => observer.disconnected));
assert.ok(!JSON.stringify(probe.report()).includes(secret));
console.log('browser recording resource details, production lifecycle, bounded aggregation and privacy ok');
