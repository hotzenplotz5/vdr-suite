'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = [
  fs.readFileSync(path.join(frontendRoot, 'platform', 'helpers.js'), 'utf8'),
  fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8')
].join('\n');

const context = {
  window: {},
  console
};

context.window.window = context.window;
context.window.document = null;
context.window.VdrSuitePlatform = {
  getSelectedBackendId() { return 'default'; },
  getSelectedModule() { return 'overview'; },
  getClientApi() { return null; }
};

vm.createContext(context);
vm.runInContext(source, context);

const api = context.window.VdrSuiteHomeRecordingDiscovery;
assert(api && api._test);

const fallbackRecording = {
  recordingId: 'band-fallback',
  backendId: 'default',
  backendNativeId: '/srv/vdr/video/Serien/Band_Of_Brothers/04_Die_Neuen/recording.rec',
  path: 'Serien/Band_Of_Brothers/04_Die_Neuen',
  title: 'Band_Of_Brothers/04_Die_Neuen',
  metadata: {
    provider: {},
    presentation: {
      posterUrl: '/epg/fallback-poster.jpg'
    }
  }
};

const nativeRecording = {
  recordingId: 'band-native',
  backendId: 'default',
  backendNativeId: '/srv/vdr/video/Serien/Band_Of_Brothers/09_Warum_wir_kaempfen/recording.rec',
  path: 'Serien/Band_Of_Brothers/09_Warum_wir_kaempfen',
  title: 'Band_Of_Brothers/09_Warum_wir_kaempfen',
  metadata: {
    provider: {},
    presentation: {
      posterUrl: '/epg/second-fallback.jpg'
    }
  }
};

const richArtworkUrl = '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=native-band&kind=preferred&index=0';

const fallbackMember = api._test.seriesMemberProjection(
  fallbackRecording,
  null,
  'default'
);

const nativeMember = api._test.seriesMemberProjection(
  nativeRecording,
  {
    available: true,
    provider: 'tvscraper',
    mediaType: 'series',
    providerId: -74205,
    title: 'Band of Brothers',
    episodeName: 'Warum wir kämpfen',
    seasonNumber: 1,
    episodeNumber: 9,
    preferredArtwork: {
      available: true,
      url: richArtworkUrl
    }
  },
  'default'
);

assert.strictEqual(fallbackMember.nativeMetadataAvailable, false);
assert.strictEqual(fallbackMember.nativeArtworkAvailable, false);
assert.strictEqual(nativeMember.nativeMetadataAvailable, true);
assert.strictEqual(nativeMember.nativeArtworkAvailable, true);
assert.strictEqual(fallbackMember.seriesKey, nativeMember.seriesKey);

const projection = api._test.buildSeriesProjection([
  fallbackMember,
  nativeMember
]);

assert.strictEqual(projection.length, 1);
assert.strictEqual(projection[0].title, 'Band of Brothers');
assert.strictEqual(projection[0].posterUrl, richArtworkUrl);
assert.strictEqual(projection[0].nativeMetadataAvailable, true);
assert.strictEqual(projection[0].nativeArtworkAvailable, true);
assert.strictEqual(projection[0].episodes.length, 2);
assert.deepStrictEqual(
  Array.from(projection[0].seasons, (season) => season.number),
  [1, 0]
);

console.log('phase66 native series group priority coverage ok');
