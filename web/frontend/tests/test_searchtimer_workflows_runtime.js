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
  api.previewChannelLabel({channelId: 'S19.2E-1-1019-10301'}, channels),
  'Das Erste HD'
);
assert.strictEqual(
  api.previewChannelLabel({channel: 'S19.2E-1-1011-11110'}, channels),
  'ZDF HD'
);
assert.strictEqual(
  api.previewChannelLabel({channelId: 'unknown'}, channels),
  'unknown'
);
assert.strictEqual(
  api.previewChannelLabel({channelName: 'Kanalname'}, []),
  'Kanalname'
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
assert.ok(source.includes('fetchClientSearchTimerCreateStaged'));
assert.strictEqual(api.stagedCreateValidation({
  query: 'Terra X', mode: 1, tolerance: 0,
  compareTitle: true, compareSubtitle: false, compareSummary: false
}).length, 0);
assert.ok(api.stagedCreateValidation({
  query: 'Terra X', mode: 4, tolerance: 0,
  compareTitle: true, compareSubtitle: false, compareSummary: false
}).length > 0);
assert.ok(api.stagedCreateValidation({
  query: 'Terra X', mode: 0, tolerance: 1,
  compareTitle: true, compareSubtitle: false, compareSummary: false
}).length > 0);

assert.deepStrictEqual(
  Array.from(api.unsupportedStagedOptions({
    active: false, directory: '', priority: 0, lifetime: 0,
    marginStartMinutes: 0, marginStopMinutes: 0, useVps: false,
    useChannel: 0, useDuration: false, avoidRepeats: false,
    blacklistMode: 0, blacklistIds: '', matchCase: false
  })), []
);
assert.deepStrictEqual(
  Array.from(api.unsupportedStagedOptions({
    active: true, directory: 'Series', priority: 50,
    useChannel: 1, avoidRepeats: true, matchCase: true
  })),
  ['Sofort aktivieren', 'Aufnahmeverzeichnis', 'Priorität',
   'Kanalfilter', 'Wiederholungen', 'Groß-/Kleinschreibung']
);
assert(source.includes('createDirectoryFields(advancedGrid, source)'));
assert(source.includes('createChannelFilterFields(advancedGrid, source)'));
assert(source.includes('Keine Anlage ausgeführt.'));
assert(source.includes("state === 'succeeded'"));
assert(source.includes("pendingKey = '';"));
assert(source.includes("error.requestId"));
assert.deepStrictEqual(
  Array.from(api.unsupportedCopiedSettings({
    useDayOfWeek: true, compareCategories: true,
    deleteRecordingsAfterDays: 3
  })),
  ['Wochentagsfilter', 'EPG-Kategorienvergleich',
   'Aufnahmen löschen nach Tagen']
);
assert.deepStrictEqual(Array.from(api.unsupportedCopiedSettings(null)), []);
assert(api.createErrorText({
  status: 503,
  payload: {detail: 'SearchTimer CREATE requires confirmed VDR pre/post margins and the native timer-create capability.'}
}).includes('VDR-Vor-/Nachlaufwerte'));
assert(api.createErrorText({
  status: 503,
  payload: {detail: 'SearchTimer CREATE requires a valid, online backend agent.'}
}).includes('Backend-Agent'));
assert.strictEqual(api.createErrorText({status: 409, message: 'Konflikt'}), 'Konflikt');
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

// Browser form regression: advanced fields must never be discarded by a
// superficially successful Public-v1 staged CREATE. This test exercises the
// real createForm submission handlers with a tiny DOM fixture.
class FormElement {
  constructor(tag) {
    this.tagName = String(tag).toUpperCase();
    this.children = [];
    this.dataset = {};
    this.style = {};
    this.listeners = {};
    this.className = '';
    this.textContent = '';
    this.value = '';
    this.checked = false;
    this.disabled = false;
    this.type = '';
    this.name = '';
    this.hidden = false;
  }
  appendChild(child) {
    this.children.push(child);
    child.parentElement = this;
    return child;
  }
  replaceChildren(...children) {
    this.children = [];
    children.forEach(child => this.appendChild(child));
  }
  addEventListener(name, handler) { this.listeners[name] = handler; }
  setAttribute(name, value) { this[name] = value; }
  get elements() {
    const controls = {};
    function visit(current) {
      if (current.name) controls[current.name] = current;
      current.children.forEach(visit);
    }
    visit(this);
    return controls;
  }
}
function findForm(node) {
  if (node.tagName === 'FORM') return node;
  for (const child of node.children) {
    const found = findForm(child);
    if (found) return found;
  }
  return null;
}
async function checkEditorStagedWrites() {
  const mount = new FormElement('main');
  const document = {
    head: new FormElement('head'),
    createElement(tag) { return new FormElement(tag); },
    getElementById() { return null; }
  };
  const writes = [];
  let readCount = 0;
  const browser = {
    setTimeout() {},
    confirm() { return true; },
    crypto: {
      getRandomValues(bytes) {
        for (let i = 0; i < bytes.length; i++) bytes[i] = i + 1;
        return bytes;
      }
    }
  };
  const client = {
    fetchClientSearchTimerCreateStaged(options) {
      writes.push(options);
      return Promise.resolve({operation: {state: 'accepted',
        operationId: 'op_00000000000000000000000000000001',
        backendId: 'living-room'}});
    },
    requestJson(path, options) {
      assert.strictEqual(path,
        '/api/v1/operations/op_00000000000000000000000000000001');
      assert.strictEqual(options.cache, 'no-store');
      readCount += 1;
      return Promise.resolve({state: readCount === 1 ? 'dispatching' : 'succeeded',
        operationId: 'op_00000000000000000000000000000001',
        backendId: 'living-room'});
    }
  };
  vm.runInNewContext(source, {
    window: browser, document, Promise, Object, Array, Boolean, Number,
    Date, String, Set, Math, JSON, Uint8Array
  }, {filename: sourcePath});
  browser.VdrSuiteSearchTimerBrowser.configureContext({
    detailDataElement: mount,
    clientApi: client,
    getSelectedBackendId() { return 'living-room'; }
  });
  browser.VdrSuiteSearchTimerBrowser.renderList({searchTimers: []});
  const form = findForm(mount);
  assert(form);
  assert(form.elements.active, 'immediate activation option must be visible');
  assert(form.elements.priority, 'recording priority must be editable');
  assert(form.elements.channelFilterMode, 'channel selector must be present');
  form.elements.query.value = 'Tatort';
  form.listeners.submit({preventDefault() {}});
  await Promise.resolve();
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(writes.length, 1);
  assert.strictEqual(writes[0].payload.active, false);
  assert.strictEqual(writes[0].payload.query, 'Tatort');
  const actions = form.children.filter(element =>
    element.className === 'searchtimer-actions')[0];
  assert(actions);
  const statusButton = actions.children.filter(element =>
    element.textContent === 'Anlagevorgang prüfen')[0];
  assert(statusButton, 'durable operation readback button must exist');
  assert.strictEqual(statusButton.hidden, false);
  assert.strictEqual(statusButton.disabled, false);
  statusButton.listeners.click();
  await Promise.resolve();
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(writes.length, 1,
    'checking an operation must never trigger another create');
  assert.strictEqual(statusButton.hidden, false,
    'dispatching is not evidence of successful CREATE');
  assert.strictEqual(statusButton.disabled, false);
  statusButton.listeners.click();
  await Promise.resolve();
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(readCount, 2);
  assert.strictEqual(statusButton.hidden, true,
    'only a confirmed succeeded operation hides readback');
  assert.strictEqual(writes.length, 1,
    'readbacks cannot duplicate the original CREATE');
  form.elements.priority.value = '50';
  form.listeners.submit({preventDefault() {}});
  assert.strictEqual(writes.length, 1,
    'unsupported recording priority must not be silently discarded');
  form.elements.priority.value = '0';
  form.elements.active.checked = true;
  form.listeners.submit({preventDefault() {}});
  assert.strictEqual(writes.length, 1,
    'activation intent must not bypass inactive-only Public-v1 CREATE');
}
checkEditorStagedWrites().then(function() {
  console.log('test_searchtimer_workflows_runtime passed');
}).catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
