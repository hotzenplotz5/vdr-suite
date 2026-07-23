'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.join(__dirname, '..', 'modules', 'remote.js');
const source = fs.readFileSync(sourcePath, 'utf8');

assert(source.includes("'/api/vdr/remote/actions'"));
assert(source.includes("'/api/vdr/live/overlay'"));
assert(!source.includes("'/remote/"));
assert(!source.includes("'/osd"));
assert(!source.toLowerCase().includes('restfulapi'));
assert(!source.toLowerCase().includes('streamdev'));
assert(!source.toLowerCase().includes('suitebridge'));

const requests = [];
const registeredModules = {};
const backends = {
  backends: [
    {
      backendId: 'house-a',
      accessMode: 'read-write',
      readOnly: false,
      enabled: true,
      online: true,
      canRemoteControl: true
    },
    {
      backendId: 'house-b',
      accessMode: 'read-only',
      readOnly: true,
      enabled: true,
      online: true,
      canRemoteControl: false
    }
  ]
};

const clientApi = {
  fetchClientBackends() {
    return Promise.resolve(backends);
  },
  requestJson(route, options) {
    requests.push({ route, options });

    if (route === '/api/vdr/live/overlay') {
      return Promise.resolve({
        backendId: 'house-a',
        channel: { available: false },
        present: { available: false },
        following: { available: false }
      });
    }

    return Promise.resolve({ success: true, message: 'Remote action executed' });
  }
};

let selectedBackendId = 'house-a';
const platform = {
  getClientApi() {
    return clientApi;
  },
  getI18n() {
    return { getLocale: () => 'de' };
  },
  getSelectedBackendId() {
    return selectedBackendId;
  },
  hasModule(name) {
    return Object.prototype.hasOwnProperty.call(registeredModules, name);
  },
  registerModule(name, api) {
    registeredModules[name] = api;
    return api;
  }
};

const windowObject = {
  VdrSuitePlatform: platform,
  VdrSuiteClientApi: clientApi,
  document: null,
  Date,
  Promise,
  Set,
  Object,
  String,
  Number,
  Array,
  Error
};
windowObject.window = windowObject;

vm.runInNewContext(source, { window: windowObject }, { filename: sourcePath });

const remote = windowObject.VdrSuiteRemoteControl;
assert(remote);
assert.strictEqual(registeredModules.remote, remote);
assert(Object.isFrozen(remote));
assert(Object.isFrozen(remote.allowedActions));
assert(remote.allowedActions.includes('volumeUp'));
assert(remote.allowedActions.includes('switchChannel'));
assert(!remote.allowedActions.includes('power'));
assert(!remote.allowedActions.includes('kbd'));
assert(!remote.allowedActions.includes('seq'));

assert.strictEqual(remote.controlStateForBackend(backends.backends[0]).allowed, true);
assert.strictEqual(remote.controlStateForBackend(backends.backends[1]).allowed, false);
assert(remote.controlStateForBackend(backends.backends[1]).reason.includes('schreibgeschützt'));

(async () => {
  await remote.sendAction('volumeUp');

  assert.strictEqual(requests.length, 1);
  assert.strictEqual(requests[0].route, '/api/vdr/remote/actions');
  assert.strictEqual(requests[0].options.method, 'POST');
  assert.strictEqual(requests[0].options.credentials, 'same-origin');

  const firstPayload = JSON.parse(requests[0].options.body);
  assert.strictEqual(firstPayload.backendId, 'house-a');
  assert.strictEqual(firstPayload.action, 'volumeUp');
  assert(/^remote-[a-z0-9]+-[a-z0-9]+$/.test(firstPayload.operationId));

  await remote.sendAction('switchChannel', 'C-1-1079-10351');
  const secondPayload = JSON.parse(requests[1].options.body);
  assert.strictEqual(secondPayload.action, 'switchChannel');
  assert.strictEqual(secondPayload.channelId, 'C-1-1079-10351');

  await assert.rejects(
    () => remote.sendAction('rawCommand'),
    /not allowlisted/
  );
  assert.strictEqual(requests.length, 2);

  selectedBackendId = 'house-b';
  await assert.rejects(
    () => remote.sendAction('up'),
    /schreibgeschützt/
  );
  assert.strictEqual(requests.length, 2);

  selectedBackendId = 'house-a';
  await remote.fetchOverlay();
  assert.strictEqual(requests[2].route, '/api/vdr/live/overlay');
  assert.strictEqual(requests[2].options.query.backend, 'house-a');

  console.log('test_remote_runtime passed');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
