'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');

const source = fs.readFileSync(
  path.join(frontendRoot, 'home-now-next.js'),
  'utf8'
);

assert(source.includes('VdrSuiteHomeNowNext'));
assert(source.includes('fetchClientEpgCacheNowNext'));
assert(source.includes('fetchClientEpgCacheNowNextArtwork'));
assert(source.includes("perChannelLimit: '2'"));
assert(!source.includes('fetchClientEpgCacheWindow'));
assert(!source.includes('untilTime'));
assert(!source.includes('fetchClientEpgArtwork'));
assert(!source.includes('fetchClientMetadata'));
assert(!source.includes('/api/epg/cache/artwork?'));
assert(!source.includes('artworkUrl'));
assert(!source.includes('imageUrl'));

const criticalPathStart =
  source.indexOf('function loadPage(options)');

const artworkPathStart =
  source.indexOf('function loadArtworkPage(options)');

assert(criticalPathStart >= 0);
assert(artworkPathStart > criticalPathStart);

const criticalPath =
  source.slice(
    criticalPathStart,
    artworkPathStart
  );

assert(
  !criticalPath.includes(
    'fetchClientEpgCacheNowNextArtwork'
  )
);

let nowNextRequests = 0;
let manifestRequests = 0;
let nowNextOptions = null;
let manifestOptions = null;

const now = Math.floor(Date.now() / 1000);

const response = {
  backendId: 'backend-a',
  eventCount: 4,
  events: [
    {
      id: 'a-now',
      channelId: 'A',
      title: 'A jetzt',
      startTime: String(now - 300),
      endTime: String(now + 600),
      durationSeconds: 900
    },
    {
      id: 'a-next',
      channelId: 'A',
      title: 'A danach',
      startTime: String(now + 600),
      endTime: String(now + 1500),
      durationSeconds: 900
    },
    {
      id: 'b-now',
      channelId: 'B',
      title: 'B jetzt',
      startTime: String(now - 300),
      endTime: String(now + 600),
      durationSeconds: 900
    },
    {
      id: 'b-next',
      channelId: 'B',
      title: 'B danach',
      startTime: String(now + 600),
      endTime: String(now + 1500),
      durationSeconds: 900
    }
  ]
};

const manifest = {
  backendId: 'backend-a',
  eventCount: 4,
  artworkCount: 1,
  items: [
    {
      channelId: 'A',
      eventId: 'a-now',
      artwork: {
        available: true,
        provider: 'tvscraper',
        width: 680,
        height: 1000,
        url: '/api/epg/cache/artwork?backend=backend-a&channelId=A&eventId=a-now'
      }
    }
  ]
};

const clientApi = {
  fetchClientEpgCacheNowNext(options) {
    nowNextRequests += 1;
    nowNextOptions = options;
    return Promise.resolve(response);
  },

  fetchClientEpgCacheNowNextArtwork(options) {
    manifestRequests += 1;
    manifestOptions = options;
    return Promise.resolve(manifest);
  }
};

const window = {
  console,
  Date,
  Promise,
  VdrSuitePlatform: {
    getClientApi() {
      return clientApi;
    }
  }
};

const context = vm.createContext({
  window,
  console,
  Date,
  Promise,
  Object,
  Array,
  String,
  Number,
  Set,
  Map,
  Error
});

vm.runInContext(
  source,
  context,
  {filename: 'web/frontend/home-now-next.js'}
);

assert.ok(window.VdrSuiteHomeNowNext);

(async function () {
  const owner = window.VdrSuiteHomeNowNext;

  const data = await owner.loadPage({
    backendId: 'backend-a',
    channelIds: ['A', 'B']
  });

  /*
   * H2 Critical Path: completing loadPage() must not have started artwork.
   */
  assert.strictEqual(nowNextRequests, 1);
  assert.strictEqual(manifestRequests, 0);

  assert.strictEqual(
    nowNextOptions.query.backend,
    'backend-a'
  );

  assert.strictEqual(
    nowNextOptions.query.channelIds,
    'A,B'
  );

  assert.strictEqual(
    nowNextOptions.query.perChannelLimit,
    '2'
  );

  assert.ok(
    Number(nowNextOptions.query.fromTime) > 0
  );

  assert.strictEqual(
    Object.prototype.hasOwnProperty.call(
      nowNextOptions.query,
      'untilTime'
    ),
    false
  );

  assert.strictEqual(
    Object.prototype.hasOwnProperty.call(
      nowNextOptions.query,
      'limit'
    ),
    false
  );

  assert.strictEqual(data.eventCount, 4);

  assert.strictEqual(
    owner.artworkForEvent(
      'backend-a',
      'A',
      'a-now'
    ),
    null
  );

  await owner.loadArtworkPage({
    backendId: 'backend-a',
    channelIds: ['A', 'B']
  });

  assert.strictEqual(manifestRequests, 1);

  /*
   * Manifest must describe exactly the same bounded page snapshot.
   */
  assert.strictEqual(
    manifestOptions.query.backend,
    nowNextOptions.query.backend
  );

  assert.strictEqual(
    manifestOptions.query.channelIds,
    nowNextOptions.query.channelIds
  );

  assert.strictEqual(
    manifestOptions.query.fromTime,
    nowNextOptions.query.fromTime
  );

  assert.strictEqual(
    manifestOptions.query.perChannelLimit,
    '2'
  );

  const artwork =
    owner.artworkForEvent(
      'backend-a',
      'A',
      'a-now'
    );

  assert.ok(artwork);
  assert.strictEqual(
    artwork.available,
    true
  );

  assert.strictEqual(
    artwork.provider,
    'tvscraper'
  );

  assert.ok(
    artwork.url.includes(
      '/api/epg/cache/artwork?'
    )
  );

  assert.strictEqual(
    owner.artworkForEvent(
      'backend-a',
      'B',
      'b-now'
    ),
    null
  );

  /*
   * Same page/snapshot is deduplicated: no second manifest request.
   */
  await owner.loadArtworkPage({
    backendId: 'backend-a',
    channelIds: ['A', 'B']
  });

  assert.strictEqual(manifestRequests, 1);

  const snapshot = owner.snapshot();

  assert.strictEqual(snapshot.backendId, 'backend-a');
  assert.strictEqual(snapshot.channelCount, 2);
  assert.strictEqual(snapshot.eventCount, 4);
  assert.strictEqual(snapshot.requestCount, 1);
  assert.strictEqual(snapshot.manifestRequestCount, 1);
  assert.strictEqual(snapshot.artworkCount, 1);

  assert.deepStrictEqual(
    Array.from(snapshot.channelIds),
    ['A', 'B']
  );

  console.log(
    'Home Now/Next H2.1 batch artwork-owner contract ok'
  );
}()).catch(error => {
  console.error(error);
  throw error;
});
