'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const registered = {};
const window = {
  VdrSuitePlatform: {
    hasModule(name) {
      return Object.prototype.hasOwnProperty.call(
        registered,
        name
      );
    },
    registerModule(name, api) {
      registered[name] = api;
      return api;
    }
  }
};

const sourcePath = path.resolve(
  __dirname,
  '..',
  'modules',
  'searchtimers.js'
);
const source = fs.readFileSync(
  sourcePath,
  'utf8'
);

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
    directory: '/Doku/Wissen/',
    priority: 50,
    lifetime: 99
  },
  scheduleOptions: {
    marginStartMinutes: 5,
    marginStopMinutes: 10,
    useVps: true
  },
  filterOptions: {
    useChannel: 2,
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
    channels: 'Öffentlich-rechtlich',
    channelMin: '',
    channelMax: ''
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
assert.strictEqual(timer.directory, 'Doku~Wissen');
assert.strictEqual(timer.useVps, true);
assert.strictEqual(timer.useChannel, 2);
assert.strictEqual(timer.channels, 'Öffentlich-rechtlich');
assert.strictEqual(timer.useDuration, true);
assert.strictEqual(timer.durationMinMinutes, 30);
assert.strictEqual(timer.avoidRepeats, true);
assert.strictEqual(timer.updateSafe, false);

assert.strictEqual(
  api.normalizeDirectory('/Movies/Action/'),
  'Movies~Action'
);
assert.strictEqual(
  api.recordingFolderPathToDirectory('/Series'),
  'Series'
);
assert.strictEqual(
  api.directoryOptionLabel('Movies~Action'),
  'Movies › Action'
);
assert.strictEqual(
  api.directoryOptionLabel(''),
  'Stammverzeichnis'
);
assert.deepStrictEqual(
  Array.from(api.directoryOptions(
    {
      folders: [
        {name: 'Movies', path: 'Movies'},
        {name: 'Series', path: '/Series'}
      ]
    },
    {
      searchTimers: [
        {recordingOptions: {directory: 'News'}},
        {directory: 'Movies~Action'}
      ]
    },
    'Archive~Old'
  )),
  [
    '',
    'Archive~Old',
    'Movies',
    'Movies~Action',
    'News',
    'Series'
  ]
);

const channels = [
  api.normalizeChannel({
    id: 'S19.2E-1-1019-10301',
    number: 1,
    name: 'Das Erste HD',
    group: 'Öffentlich-rechtlich'
  }, 0),
  api.normalizeChannel({
    id: 'S19.2E-1-1011-11110',
    number: 2,
    name: 'ZDF HD',
    group: 'Öffentlich-rechtlich'
  }, 1),
  api.normalizeChannel({
    id: 'S19.2E-1-1089-12003',
    number: 3,
    name: 'RTL Television',
    group: 'Privat'
  }, 2),
  api.normalizeChannel({
    id: 'S19.2E-1-1101-28106',
    number: 4,
    name: 'Radio Test',
    group: ''
  }, 3)
];

assert.deepStrictEqual(
  Array.from(api.channelGroups(channels, true)),
  ['Öffentlich-rechtlich', 'Privat', '__ungrouped__']
);
assert.deepStrictEqual(
  Array.from(api.channelGroups(channels, false)),
  ['Öffentlich-rechtlich', 'Privat']
);
assert.deepStrictEqual(
  Array.from(
    api.channelsForGroup(
      channels,
      'Öffentlich-rechtlich'
    )
  ).map(channel => channel.id),
  [
    'S19.2E-1-1019-10301',
    'S19.2E-1-1011-11110'
  ]
);
assert.strictEqual(
  api.channelOptionLabel(channels[0]),
  '1 · Das Erste HD'
);
assert.strictEqual(
  api.inferChannelMode(true, 'Privat', '', ''),
  2
);
assert.strictEqual(
  api.inferChannelMode(true, '', 'A', 'A'),
  1
);

function field(type, value, checked) {
  return {
    type: type || 'text',
    value: value === undefined ? '' : String(value),
    checked: Boolean(checked)
  };
}

function baseElements() {
  return {
    name: field('text', 'Terra X Neu'),
    query: field('text', 'Terra X'),
    active: field('checkbox', '', true),
    useVps: field('checkbox', '', true),
    compareTitle: field('checkbox', '', true),
    compareSubtitle: field('checkbox', '', true),
    compareSummary: field('checkbox', '', false),
    avoidRepeats: field('checkbox', '', true),
    directory: field('select-one', 'Doku'),
    manualDirectory: field('text', ''),
    priority: field('number', '50'),
    lifetime: field('number', '99'),
    marginStartMinutes: field('number', '5'),
    marginStopMinutes: field('number', '10'),
    channelFilterMode: field('select-one', '2'),
    channelFilterGroup: field(
      'select-one',
      'Öffentlich-rechtlich'
    ),
    channelId: field('select-one', ''),
    manualUseChannel: field('text', ''),
    manualChannels: field('text', ''),
    manualChannelMin: field('text', ''),
    manualChannelMax: field('text', ''),
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
  };
}

const groupPayload = api.buildCreatePayload(
  {elements: baseElements()},
  timer
);
assert.strictEqual(groupPayload.backendId, 'living-room');
assert.strictEqual(groupPayload.directory, 'Doku');
assert.strictEqual(groupPayload.useChannel, 2);
assert.strictEqual(
  groupPayload.channels,
  'Öffentlich-rechtlich'
);
assert.strictEqual(groupPayload.channelMin, '');
assert.strictEqual(groupPayload.channelMax, '');
assert.deepStrictEqual(
  Array.from(api.validateCreatePayload(groupPayload)),
  []
);

const singleElements = baseElements();
singleElements.channelFilterMode.value = '1';
singleElements.channelFilterGroup.value = '';
singleElements.channelId.value =
  'S19.2E-1-1011-11110';
singleElements.manualDirectory.value =
  '/Series/Tatort/';

const singlePayload = api.buildCreatePayload(
  {elements: singleElements},
  timer
);
assert.strictEqual(
  singlePayload.directory,
  'Series~Tatort'
);
assert.strictEqual(singlePayload.useChannel, 1);
assert.strictEqual(singlePayload.channels, '');
assert.strictEqual(
  singlePayload.channelMin,
  'S19.2E-1-1011-11110'
);
assert.strictEqual(
  singlePayload.channelMax,
  'S19.2E-1-1011-11110'
);
assert.deepStrictEqual(
  Array.from(api.validateCreatePayload(singlePayload)),
  []
);

const allElements = baseElements();
allElements.channelFilterMode.value = '0';
allElements.channelFilterGroup.value = '';
const allPayload = api.buildCreatePayload(
  {elements: allElements},
  timer
);
assert.strictEqual(allPayload.useChannel, 0);
assert.strictEqual(allPayload.channels, '');
assert.strictEqual(allPayload.channelMin, '');
assert.strictEqual(allPayload.channelMax, '');

const intervalElements = baseElements();
intervalElements.manualUseChannel.value = '1';
intervalElements.manualChannelMin.value = 'A';
intervalElements.manualChannelMax.value = 'Z';
const intervalPayload = api.buildCreatePayload(
  {elements: intervalElements},
  timer
);
assert.strictEqual(intervalPayload.useChannel, 1);
assert.strictEqual(intervalPayload.channelMin, 'A');
assert.strictEqual(intervalPayload.channelMax, 'Z');

const invalid = Object.assign({}, groupPayload, {
  query: '',
  compareTitle: false,
  compareSubtitle: false,
  compareSummary: false
});
assert.strictEqual(
  api.validateCreatePayload(invalid).length,
  2
);

assert.ok(!source.includes('fetch('));
assert.ok(
  source.includes('fetchClientSearchTimerPreview')
);
assert.ok(
  source.includes('fetchClientSearchTimerCreateAction')
);
assert.ok(
  source.includes('fetchClientSearchTimerDeleteAction')
);
assert.ok(
  source.includes('fetchClientRecordingFolder')
);
assert.ok(source.includes('fetchClientChannels'));
assert.ok(source.includes(
  "field('Aufnahmeverzeichnis auswählen', directorySelect, true)"
));
assert.ok(source.includes(
  "field('Kanalfilter', modeSelect, true)"
));
assert.ok(source.includes(
  'Expertenoption: Verzeichnis manuell eingeben'
));
assert.ok(source.includes(
  'Expertenoption: Kanalfilter manuell eingeben'
));
assert.ok(source.includes(
  'Nur frei empfangbare Kanäle (FTA)'
));
assert.ok(source.includes(
  'function clearManualChannelFilter()'
));
assert.ok(source.includes(
  "channelGroupSelect.addEventListener('change', clearManualChannelFilter)"
));

// PHASE62_SLICE2J_SEARCHTIMER_CREATE_CSRF_TESTS and
// caller-must-not-override are now exercised by the focused
// test_searchtimer_maintenance_security_runtime.js contract.

console.log('test_searchtimer_workflows_runtime passed');
