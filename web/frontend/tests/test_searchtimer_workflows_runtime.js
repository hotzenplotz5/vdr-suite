'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const registered = {};
const window = {
  VdrSuitePlatform: {
    hasModule(name) {
      return Object.prototype.hasOwnProperty.call(registered, name);
    },
    registerModule(name, api) {
      registered[name] = api;
      return api;
    }
  }
};

const sourcePath = path.resolve(__dirname, '..', 'modules', 'searchtimers.js');
const source = fs.readFileSync(sourcePath, 'utf8');

vm.runInNewContext(source, {
  Array,
  Boolean,
  Date,
  JSON,
  Math,
  Number,
  Object,
  Promise,
  Set,
  String,
  window
}, {filename: sourcePath});

const api = window.VdrSuiteSearchTimerBrowser;
assert.ok(api);
assert.strictEqual(registered.searchtimers, api);

api.configureContext({
  getSelectedBackendId() {
    return 'living-room';
  }
});

const timer = api.normalizeSearchTimer({
  backendId: 'living-room',
  backendNativeId: '17',
  name: 'Terra X',
  query: 'Terra X',
  state: 'active',
  recordingOptions: {
    directory: 'Doku',
    priority: 50,
    lifetime: 99
  },
  scheduleOptions: {
    marginStartMinutes: 5,
    marginStopMinutes: 10,
    useVps: true
  },
  filterOptions: {
    useChannel: true,
    useDayOfWeek: false,
    useDuration: true,
    durationMinMinutes: 30,
    durationMaxMinutes: 120
  },
  comparisonOptions: {
    compareTitle: true,
    compareSubtitle: true,
    compareSummary: false
  },
  repeatOptions: {
    avoidRepeats: true,
    allowedRepeats: 1,
    repeatsWithinDays: 14
  },
  channelOptions: {
    channels: '1,2,3',
    channelMin: 1,
    channelMax: 99
  },
  blacklistOptions: {
    blacklistMode: 2,
    blacklistIds: '4,5'
  },
  matchOptions: {
    mode: 5,
    matchCase: false,
    tolerance: 2,
    summaryMatch: 90
  }
}, 0);

assert.strictEqual(timer.backendId, 'living-room');
assert.strictEqual(timer.backendNativeId, '17');
assert.strictEqual(timer.active, true);
assert.strictEqual(timer.directory, 'Doku');
assert.strictEqual(timer.useVps, true);
assert.strictEqual(timer.useChannel, true);
assert.strictEqual(timer.channels, '1,2,3');
assert.strictEqual(timer.useDuration, true);
assert.strictEqual(timer.durationMinMinutes, 30);
assert.strictEqual(timer.avoidRepeats, true);
assert.strictEqual(timer.updateSafe, false);

function field(type, value, checked) {
  return {
    type: type || 'text',
    value: value === undefined ? '' : String(value),
    checked: Boolean(checked)
  };
}

const form = {
  elements: {
    name: field('text', 'Terra X Neu'),
    query: field('text', 'Terra X'),
    active: field('checkbox', '', true),
    useVps: field('checkbox', '', true),
    compareTitle: field('checkbox', '', true),
    compareSubtitle: field('checkbox', '', true),
    compareSummary: field('checkbox', '', false),
    avoidRepeats: field('checkbox', '', true),
    directory: field('text', 'Doku'),
    priority: field('number', '50'),
    lifetime: field('number', '99'),
    marginStartMinutes: field('number', '5'),
    marginStopMinutes: field('number', '10'),
    channels: field('text', '1,2,3'),
    useDuration: field('checkbox', '', true),
    durationMinMinutes: field('number', '30'),
    durationMaxMinutes: field('number', '120'),
    allowedRepeats: field('number', '1'),
    repeatsWithinDays: field('number', '14'),
    mode: field('select-one', '5'),
    matchCase: field('checkbox', '', false),
    tolerance: field('number', '2'),
    blacklistMode: field('number', '2'),
    blacklistIds: field('text', '4,5')
  }
};

const payload = api.buildCreatePayload(form, timer);
assert.strictEqual(payload.backendId, 'living-room');
assert.strictEqual(payload.name, 'Terra X Neu');
assert.strictEqual(payload.query, 'Terra X');
assert.strictEqual(payload.active, true);
assert.strictEqual(payload.useVps, true);
assert.strictEqual(payload.useChannel, 1);
assert.strictEqual(payload.channels, '1,2,3');
assert.strictEqual(payload.useDuration, true);
assert.strictEqual(payload.durationMinMinutes, 30);
assert.strictEqual(payload.durationMaxMinutes, 120);
assert.strictEqual(payload.mode, 5);
assert.deepStrictEqual(Array.from(api.validateCreatePayload(payload)), []);

const invalid = Object.assign({}, payload, {
  query: '',
  compareTitle: false,
  compareSubtitle: false,
  compareSummary: false
});
assert.strictEqual(api.validateCreatePayload(invalid).length, 2);
assert.ok(!source.includes('fetch('));
assert.ok(source.includes('fetchClientSearchTimerPreview'));
assert.ok(source.includes('fetchClientSearchTimerCreateAction'));
assert.ok(source.includes('fetchClientSearchTimerDeleteAction'));

console.log('test_searchtimer_workflows_runtime passed');
