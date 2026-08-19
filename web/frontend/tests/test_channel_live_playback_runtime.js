'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const sourcePath = 'web/frontend/channel-day-program.js';
const source = fs.readFileSync(sourcePath, 'utf8');
const testHookMarker = 'global.VdrSuiteChannels2 = moduleApi;';
const instrumentedSource = source
  .replace('adoptCanonicalChannelNavigation();', '')
  .replace(
    testHookMarker,
    'global.__VdrSuiteChannelLiveTest=Object.freeze({' +
      'ensureLivePlaybackRuntime,startLivePlayback,destroyLivePlayback,state});' +
      testHookMarker
  );
assert.notStrictEqual(instrumentedSource, source);

const created = [];
let deferredReplacementResolve = null;
let useDeferredReplacement = false;

function makePlayback(channel, options) {
  const id = channel.id;
  const instance = {
    element: {channelId: id},
    started: 0,
    destroyed: 0,
    relinquished: 0,
    start() {
      this.started += 1;
      return Promise.resolve(id === 'A' ? 'session-a' : 'session-' + id.toLowerCase());
    },
    destroy() {
      this.destroyed += 1;
    },
    relinquishForReplacement() {
      this.relinquished += 1;
      if (!useDeferredReplacement) return Promise.resolve('session-a');
      return new Promise(resolve => { deferredReplacementResolve = resolve; });
    }
  };
  created.push({channel, options, instance});
  return instance;
}

const mount = {
  classList: {add() {}, remove() {}},
  replaceChildren() {},
  appendChild() {},
  prepend() {}
};
const window = {
  VdrSuitePlatform: {
    getSelectedBackendId() { return 'living-room'; },
    getClientApi() { return null; },
    getMountTarget() { return mount; }
  },
  VdrSuiteRecordings2Playback: {
    createLivePanel(channel, backendId, options) {
      assert.strictEqual(backendId, 'living-room');
      return makePlayback(channel, options);
    }
  },
  setTimeout,
  clearTimeout
};
const document = {
  head: {appendChild() {}},
  createElement() {
    return {
      children: [],
      className: '',
      classList: {add() {}, remove() {}},
      style: {},
      append() {},
      appendChild() {},
      setAttribute() {},
      replaceChildren() {},
      textContent: '',
      value: '',
      disabled: false
    };
  },
  getElementById() { return {}; },
  querySelector() { return null; }
};

vm.runInNewContext(instrumentedSource, {
  Array,
  Boolean,
  Date,
  Event: function Event() {},
  JSON,
  Map,
  Math,
  Number,
  Object,
  Promise,
  Set,
  String,
  document,
  window
}, {filename: sourcePath});

(async function () {
  const test = window.__VdrSuiteChannelLiveTest;
  assert.ok(test);
  test.state.active = true;

  const channelA = {id: 'A', name: 'Sender A', enabled: true};
  const channelB = {id: 'B', name: 'Sender B', enabled: true};

  test.state.channel = channelA;
  await test.startLivePlayback(channelA);
  assert.strictEqual(created.length, 1);
  assert.strictEqual(created[0].options.replacesSessionId, '');
  assert.strictEqual(created[0].instance.started, 1);
  assert.strictEqual(test.state.liveChannelId, 'A');

  useDeferredReplacement = true;
  test.state.channel = channelB;
  const switching = test.startLivePlayback(channelB);
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(created[0].instance.relinquished, 1);
  assert.strictEqual(created.length, 1, 'B must not be created before A yielded a session id');
  assert.strictEqual(test.state.liveSwitching, true);

  deferredReplacementResolve('session-a');
  await switching;
  assert.strictEqual(created.length, 2);
  assert.strictEqual(created[1].channel.id, 'B');
  assert.strictEqual(created[1].options.replacesSessionId, 'session-a');
  assert.strictEqual(created[1].instance.started, 1);
  assert.strictEqual(test.state.liveChannelId, 'B');
  assert.strictEqual(test.state.liveSwitching, false);

  const activeB = created[1].instance;
  test.destroyLivePlayback();
  assert.strictEqual(activeB.destroyed, 1);
  assert.strictEqual(test.state.livePlayback, null);
  assert.strictEqual(test.state.liveChannelId, '');

  assert.ok(source.includes("'/frontend/recordings2-playback.js'"));
  assert.ok(source.includes('createLivePanel'));
  assert.ok(source.includes('relinquishForReplacement'));
  assert.ok(source.includes('replacesSessionId'));
  assert.ok(source.includes('destroyLivePlayback();'));
  assert.ok(!source.includes('fetch('));

  console.log('channel Live TV switch lifecycle ok');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
