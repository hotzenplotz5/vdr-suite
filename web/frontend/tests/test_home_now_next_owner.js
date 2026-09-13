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
assert(source.includes("perChannelLimit: '2'"));
assert(!source.includes('fetchClientEpgCacheWindow'));
assert(!source.includes('untilTime'));
assert(!source.includes('fetchClientEpgArtwork'));
assert(!source.includes('/api/epg/cache/artwork'));
assert(!source.includes('artworkUrl'));
assert(!source.includes('imageUrl'));

let requests = 0;
let lastOptions = null;

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

const clientApi = {
  fetchClientEpgCacheNowNext(options) {
    requests += 1;
    lastOptions = options;
    return Promise.resolve(response);
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

  assert.strictEqual(requests, 1);

  assert.strictEqual(
    lastOptions.query.backend,
    'backend-a'
  );

  assert.strictEqual(
    lastOptions.query.channelIds,
    'A,B'
  );

  assert.strictEqual(
    lastOptions.query.perChannelLimit,
    '2'
  );

  assert.ok(
    Number(lastOptions.query.fromTime) > 0
  );

  assert.strictEqual(
    Object.prototype.hasOwnProperty.call(
      lastOptions.query,
      'untilTime'
    ),
    false
  );

  assert.strictEqual(
    Object.prototype.hasOwnProperty.call(
      lastOptions.query,
      'limit'
    ),
    false
  );

  assert.strictEqual(data.eventCount, 4);

  const snapshot = owner.snapshot();

  assert.strictEqual(snapshot.backendId, 'backend-a');
  assert.strictEqual(snapshot.channelCount, 2);
  assert.strictEqual(snapshot.eventCount, 4);
  assert.strictEqual(snapshot.requestCount, 1);
  assert.deepStrictEqual(
    Array.from(snapshot.channelIds),
    ['A', 'B']
  );

  console.log(
    'Home Now/Next bounded data-owner contract ok'
  );
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
