'use strict';

const fs = require('fs');
const path = require('path');
const vm = require('vm');
const assert = require('assert');

const clientSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'live-remote-client-api.js'),
  'utf8'
);
const source = fs.readFileSync(
  path.join(__dirname, '..', 'modules', 'remote.js'),
  'utf8'
);

assert(!source.includes('/remote/seq'));
assert(!source.includes('/remote/kbd'));
assert(!source.includes('/osd'));
assert(!source.includes('restfulapi'));
assert(!source.includes('/api/vdr/remote/actions'));
assert(!source.includes('/api/vdr/live/overlay'));
assert(!source.includes('/api/vdr/live'));
assert(clientSource.includes("'/api/vdr/remote/actions'"));
assert(clientSource.includes("'/api/vdr/live/overlay'"));
assert(clientSource.includes("'/api/vdr/live'"));
assert(source.includes("['overview','channels2','recordings2','epg','channelsort','timers','searchtimers']"));

const serverPaths = fs.readFileSync(
  path.join(__dirname, '..', '..', '..', 'core', 'http', 'src', 'TestHttpServerPaths.inc'),
  'utf8'
);
const liveRemoteMake = fs.readFileSync(
  path.join(__dirname, '..', '..', '..', 'mk', 'live-remote.mk'),
  'utf8'
);
assert(serverPaths.includes('{"/frontend/app.js", "app.js", "application/javascript; charset=utf-8", "modules/remote.js"}'));
assert(serverPaths.includes('{"/frontend/api/client-api.js", "api/client-api.js", "application/javascript; charset=utf-8", "api/live-remote-client-api.js"}'));
assert(liveRemoteMake.includes('web/frontend/modules/remote.js'));
assert(liveRemoteMake.includes('web/frontend/api/live-remote-client-api.js'));

const created = [];
const document = {
  head: { appendChild: function () {} },
  createElement: function (tag) {
    const node = {
      tagName: tag.toUpperCase(),
      dataset: {},
      classList: { add: function () {}, remove: function () {}, toggle: function () {} },
      setAttribute: function () {},
      removeAttribute: function () {},
      appendChild: function () {},
      addEventListener: function () {},
      querySelector: function () { return null; },
      querySelectorAll: function () { return []; }
    };
    created.push(node);
    return node;
  },
  querySelector: function () { return null; },
  querySelectorAll: function () { return []; },
  getElementById: function () { return null; },
  addEventListener: function () {}
};

const requests = [];
const baseApi = Object.freeze({
  requestJson: function (url, options) {
    requests.push({url: url, options: options});
    return Promise.resolve({success: true});
  },
  fetchClientBackends: function () { return Promise.resolve({backends: []}); }
});

const context = {
  window: null,
  document: document,
  console: console,
  Date: Date,
  JSON: JSON,
  Object: Object,
  Promise: Promise,
  Set: Set,
  URLSearchParams: URLSearchParams,
  setTimeout: setTimeout,
  clearTimeout: clearTimeout,
  VdrSuiteClientApi: baseApi
};
context.window = context;
vm.createContext(context);
vm.runInContext(clientSource, context);
vm.runInContext(source, context);

assert(context.VdrSuiteRemote);
assert.deepStrictEqual(
  Array.from(context.VdrSuiteRemote.actions),
  [
    'up', 'down', 'left', 'right', 'ok', 'back', 'menu', 'info',
    'red', 'green', 'yellow', 'blue',
    'zero', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine',
    'channelUp', 'channelDown', 'volumeUp', 'volumeDown', 'mute',
    'play', 'pause', 'stop', 'record', 'fastForward', 'rewind', 'next', 'previous',
    'switchChannel'
  ]
);
assert.strictEqual(typeof context.VdrSuiteClientApi.fetchClientRemoteAction, 'function');
assert.strictEqual(typeof context.VdrSuiteClientApi.fetchClientLiveOverlay, 'function');
assert.strictEqual(
  context.VdrSuiteRemote.canControlBackend({
    enabled: true,
    online: true,
    frontendSelector: {canWrite: false},
    capabilities: {remoteControl: true}
  }),
  false
);
assert.strictEqual(
  context.VdrSuiteRemote.canControlBackend({
    enabled: true,
    online: true,
    frontendSelector: {canWrite: true},
    capabilities: {remoteControl: true}
  }),
  true
);

context.VdrSuiteClientApi.fetchClientRemoteAction({
  payload: {backendId: 'default', operationId: 'remote-1', action: 'ok'}
});
context.VdrSuiteClientApi.fetchClientLiveOverlay({backendId: 'default'});

assert.strictEqual(requests[0].url, '/api/vdr/remote/actions');
assert.strictEqual(requests[0].options.method, 'POST');
assert.strictEqual(requests[1].url, '/api/vdr/live/overlay');
console.log('remote frontend contract ok');
