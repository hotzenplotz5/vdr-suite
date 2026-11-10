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

const completeMldTimer = api.normalizeSearchTimer({
  backendId: 'default',
  filterOptions: {
    useChannelMode: 2, useTime: true, startTime: 930,
    stopTime: 1120, useDayOfWeek: true, dayOfWeek: 31
  },
  channelOptions: {
    channels: 'Öffentlich',
    channelMinId: 'S19.2E-1-1011-11110',
    channelMaxId: 'S19.2E-1-1011-11111'
  }
}, 0);
assert.strictEqual(completeMldTimer.useChannel, 2);
assert.strictEqual(completeMldTimer.useTime, true);
assert.strictEqual(completeMldTimer.startTime, 930);
assert.strictEqual(completeMldTimer.stopTime, 1120);
assert.strictEqual(completeMldTimer.dayOfWeek, 31);
assert.strictEqual(completeMldTimer.channelMin, 'S19.2E-1-1011-11110');
assert.strictEqual(completeMldTimer.channelMax, 'S19.2E-1-1011-11111');

assert.ok(!source.includes('fetch('));
assert.ok(
  source.includes('fetchClientSearchTimerPreview')
);
assert.ok(
  source.includes('fetchClientSearchTimerCreateStaged')
);
// The full Browser editor uses Public-v1 admission, never a legacy writer.
assert.ok(!source.includes('api.fetchClientSearchTimerCreateAction({'));
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

// F1.3 browser form E2E: active full-provider CREATE, native-ID readback,
// selected backend, and no duplicate writes even on an ambiguous result.
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
async function checkEditorFullWrites() {
  const mount = new FormElement('main');
  const document = {
    head: new FormElement('head'),
    createElement(tag) { return new FormElement(tag); },
    getElementById() { return null; }
  };
  const writes = [];
  const operationReads = [];
  const eventStreams = [];
  let operationReadState = 'outcome_unknown';
  class FakeEventSource {
    constructor(url, options) {
      this.url = url;
      this.options = options;
      this.listeners = {};
      this.closed = false;
      eventStreams.push(this);
    }
    addEventListener(name, callback) { this.listeners[name] = callback; }
    emit(name, payload) {
      if (this.listeners[name])
        this.listeners[name]({data: JSON.stringify(payload)});
    }
    close() { this.closed = true; }
  }
  let state = 'succeeded';
  let rejectCreate = false;
  let keyCounter = 0;
  const browser = {
    EventSource: FakeEventSource,
    setTimeout() {},
    confirm() { return true; },
    crypto: {
      getRandomValues(bytes) {
        keyCounter++;
        bytes.fill(keyCounter);
        return bytes;
      }
    }
  };
  const client = {
    fetchClientSearchTimerCreateStaged(options) {
      writes.push(options);
      if (rejectCreate) {
        return Promise.reject(new Error('validation_error status=422'));
      }
      return Promise.resolve({
        operationId: 'op_' + writes.length.toString(16).padStart(32, '0'),
        backendId: options.backend,
        state
      });
    },
    fetchClientSearchTimerOperation(options) {
      operationReads.push(options.operationId);
      return Promise.resolve({
        operationId: options.operationId, backendId: 'living-room',
        state: operationReadState
      });
    }
  };
  vm.runInNewContext(source, {
    window: browser, document, Promise, Object, Array, Boolean, Number,
    Date, String, Set, Math, JSON, Uint8Array
  }, {filename: sourcePath});
  const mod = browser.VdrSuiteSearchTimerBrowser;
  mod.configureContext({
    detailDataElement: mount,
    clientApi: client,
    getSelectedBackendId() { return 'living-room'; }
  });
  mod.renderList({searchTimers: []});
  const form = findForm(mount);
  assert(form);
  assert(form.elements.active);
  assert(form.elements.priority);
  assert(form.elements.useTime);
  assert(form.elements.channelFilterMode);
  assert(form.elements.useExtendedEpgInfo);
  assert(form.elements.deleteMode);
  form.elements.query.value = 'Tatort';
  form.elements.priority.value = '50';
  form.elements.active.checked = true;
  form.elements.useTime.checked = true;
  form.elements.startTime.value = '930';
  form.elements.stopTime.value = '1130';
  form.elements.useDayOfWeek.checked = true;
  form.elements.dayOfWeek.value = '31';
  form.elements.manualUseChannel.value = '2';
  form.elements.manualChannels.value = 'ARD';
  form.elements.useDuration.checked = true;
  form.elements.durationMinMinutes.value = '35';
  form.elements.durationMaxMinutes.value = '100';
  form.elements.avoidRepeats.checked = true;
  form.elements.allowedRepeats.value = '1';
  form.elements.blacklistMode.value = '2';
  form.elements.blacklistIds.value = '4, 5';
  form.elements.useExtendedEpgInfo.checked = true;
  form.elements.extendedEpgInfo.value = 'category=movie; actor=Jane';
  form.listeners.submit({preventDefault() {}});
  form.listeners.submit({preventDefault() {}});
  await Promise.resolve();
  await Promise.resolve();
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(writes.length, 1, 'one user action => one CREATE');
  const request = writes[0];
  assert.strictEqual(request.backend, 'living-room');
  assert(/^search-web-[0-9a-f]{32}$/.test(request.idempotencyKey));
  assert.strictEqual(request.payload.backendId, undefined,
    'backend authority comes only from scoped query');
  assert.strictEqual(request.payload.confirmed, true);
  assert.strictEqual(request.payload.active, true);
  assert.strictEqual(request.payload.priority, 50);
  assert.strictEqual(request.payload.useTime, true);
  assert.strictEqual(request.payload.startTime, 930);
  assert.strictEqual(request.payload.stopTime, 1130);
  assert.strictEqual(request.payload.dayOfWeek, 31);
  assert.strictEqual(request.payload.useChannel, 2);
  assert.strictEqual(request.payload.channels, 'ARD');
  assert.strictEqual(request.payload.durationMinMinutes, 35);
  assert.strictEqual(request.payload.blacklistIds, '4, 5');
  assert.strictEqual(request.payload.extendedEpgInfo,
    'category=movie; actor=Jane');
  assert.strictEqual(request.payload.marginStartMinutes, undefined,
    'unset margins use live VDR defaults');
  assert.strictEqual(request.payload.marginStopMinutes, undefined);
  assert.strictEqual(operationReads.length, 0,
    'succeeded means the backend already verified full readback');
  assert.strictEqual(form.elements.active.checked, true);

  // A still-unverified operation must not trigger another write.
  mod.renderList({searchTimers: []});
  const second = findForm(mount);
  state = 'outcome_unknown';
  second.elements.query.value = 'Tatort';
  second.elements.active.checked = false;
  second.listeners.submit({preventDefault() {}});
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(writes.length, 2);
  assert.strictEqual(writes[1].payload.active, false);
  assert.strictEqual(eventStreams.length, 1);
  const stream = eventStreams[0];
  assert.strictEqual(stream.url,
    '/api/v1/operations/op_00000000000000000000000000000002/events');
  stream.emit('operation', {operationId: 'op_00000000000000000000000000000001'});
  assert.strictEqual(operationReads.length, 0,
    'unrelated operation hints must not read this operation');
  // Actual SSE frames contain no state. A hello followed by completion
  // during an in-flight GET must still read the final state.
  stream.emit('hello', {daemonEpoch: 'test'});
  stream.emit('operation', {
    operationId: 'op_00000000000000000000000000000002',
    daemonEpoch: 'test', subscriptionSequence: 1
  });
  for (let step = 0; step < 8; step++) await Promise.resolve();
  assert.strictEqual(operationReads.length, 2,
    'hello and concurrent completion hint require two GETs, no polling');
  stream.emit('resync', {});
  for (let step = 0; step < 6; step++) await Promise.resolve();
  assert.strictEqual(operationReads.length, 3,
    'resync must recover missed operation changes');
  second.listeners.submit({preventDefault() {}});
  assert.strictEqual(writes.length, 2);
  // The user can inspect the exact existing operation, but not re-CREATE it.
  function findOperationButton(node) {
    if (node.tagName === 'BUTTON' && node.textContent === 'Anlagevorgang prüfen')
      return node;
    for (const child of node.children) {
      const match = findOperationButton(child);
      if (match) return match;
    }
    return null;
  }
  const inspect = findOperationButton(second);
  assert(inspect && !inspect.hidden);
  inspect.listeners.click();
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(operationReads.length, 4);
  assert.strictEqual(writes.length, 2);

  mod.renderList({searchTimers: []});
  const rejected = findForm(mount);
  rejectCreate = true;
  rejected.elements.query.value = 'Tatort';
  rejected.elements.active.checked = true;
  rejected.listeners.submit({preventDefault() {}});
  await Promise.resolve();
  await Promise.resolve();
  const feedback = rejected.children.find(child =>
    String(child.className).indexOf('searchtimer-feedback') === 0);
  assert(feedback);
  assert(feedback.textContent.includes('validation_error status=422'));
  assert.strictEqual(writes.length, 3);
  rejected.listeners.submit({preventDefault() {}});
  assert.strictEqual(writes.length, 3);
}
checkEditorFullWrites().then(function() {
  console.log('test_searchtimer_workflows_runtime passed');
}).catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
