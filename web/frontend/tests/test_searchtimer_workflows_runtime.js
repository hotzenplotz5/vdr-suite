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
        {name: 'Movies', path: 'Movies', singleRecordingLeaf: false},
        {name: 'Tatort', path: 'Tatort', singleRecordingLeaf: true},
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
// Public-v1 supports native epgsearch regular expressions (mode 4).
assert.strictEqual(api.stagedCreateValidation({
  query: 'Terra X', mode: 4, tolerance: 0,
  compareTitle: true, compareSubtitle: false, compareSummary: false
}).length, 0);
assert.ok(api.stagedCreateValidation({
  query: 'Terra X', mode: 6, tolerance: 0,
  compareTitle: true, compareSubtitle: false, compareSummary: false
}).length > 0);
assert.ok(api.stagedCreateValidation({
  query: 'Terra X', mode: 0, tolerance: 1,
  compareTitle: true, compareSubtitle: false, compareSummary: false
}).length > 0);

assert.ok(
  source.includes('mutatePublicSearchTimer') && !source.includes('fetchClientSearchTimerDeleteAction')
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
    createClientOperationUpdateSource(operationId) {
      assert(/^op_[0-9a-f]{32}$/.test(operationId));
      return new FakeEventSource(
        '/api/v1/operations/' + encodeURIComponent(operationId) + '/events',
        {withCredentials: true});
    },
    fetchClientSearchTimerCreateStaged(options) {
      writes.push(options);
      if (rejectCreate === 'validation') {
        return Promise.reject(Object.assign(new Error('validation_error status=422'),
          {status: 422, code: 'validation_error'}));
      }
      if (rejectCreate === 'network') {
        return Promise.reject(new Error('network disconnected after POST'));
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
  assert.strictEqual(form.elements.startTime.type, 'time');
  assert.strictEqual(form.elements.stopTime.type, 'time');
  assert.strictEqual(form.elements.startTime.step, '60');
  assert.strictEqual(form.elements.startTime.value, '00:00');
  assert.strictEqual(form.elements.stopTime.value, '00:00');
  assert(form.elements.channelFilterMode);
  assert(form.elements.useExtendedEpgInfo);
  assert(form.elements.deleteMode);
  form.elements.query.value = 'Tatort';
  form.elements.priority.value = '50';
  form.elements.active.checked = true;
  form.elements.useTime.checked = true;
  form.elements.startTime.value = '09:30';
  form.elements.stopTime.value = '11:30';
  form.elements.useDayOfWeek.checked = true;
  form.elements.dayOfWeek.value = '31';
  form.elements.manualUseChannel.value = '2';
  form.elements.manualChannels.value = 'ARD';
  form.elements.useDuration.checked = true;
  form.elements.durationMinMinutes.value = '35';
  form.elements.durationMaxMinutes.value = '100';
  form.elements.avoidRepeats.checked = true;
  form.elements.repeatCompareTitle.checked = false;
  form.elements.repeatCompareSubtitle.checked = true;
  form.elements.repeatCompareSummary.checked = true;
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
  assert.strictEqual(request.payload.compareTitle, true);
  assert.strictEqual(request.payload.repeatCompareTitle, false);
  assert.strictEqual(request.payload.repeatCompareSubtitle, true);
  assert.strictEqual(request.payload.repeatCompareSummary, true);
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
  assert.strictEqual(stream.options.withCredentials, true,
    'Client API SSE must preserve Browser session credentials');
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
  const createFeedback = second.children.find(child => String(child.className).startsWith('searchtimer-feedback'));
  const cancel = second.children.flatMap(child => child.children).find(child => child.textContent === 'Abbrechen / Zurück');
  assert(cancel && cancel.type === 'button');
  const panel = second.parentElement.parentElement;
  panel.open = true;
  cancel.listeners.click();
  assert.strictEqual(panel.open, false);
  assert.strictEqual(second.elements.query.value, 'Tatort');
  panel.open = true;
  second.listeners.submit({preventDefault() {}});
  assert.strictEqual(writes.length, 2, 'back/reopen cannot resend an uncertain CREATE');
  assert.notStrictEqual(second.elements.useDuration.parentElement.parentElement,
    second.elements.avoidRepeats.parentElement.parentElement, 'duration and repeats have separate groups');
  assert.strictEqual(second.elements.summaryMatch.parentElement.parentElement,
    second.elements.avoidRepeats.parentElement.parentElement, 'summary similarity belongs to repeats');
  assert.strictEqual(second.elements.compareTime.parentElement.parentElement,
    second.elements.avoidRepeats.parentElement.parentElement, 'time comparison belongs to repeats');
  const inspect = findOperationButton(second);
  assert(inspect && !inspect.hidden);
  inspect.listeners.click();
  assert.strictEqual(createFeedback.textContent, 'Anlagevorgang wird geprüft …');
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(operationReads.length, 4);
  assert(createFeedback.textContent.startsWith('Status geprüft: '));
  assert(createFeedback.textContent.includes('outcome_unknown'));
  assert.strictEqual(writes.length, 2);

  mod.renderList({searchTimers: []});
  assert.strictEqual(stream.closed, true, 'replacing the editor closes its SSE subscription');
  const rejected = findForm(mount);
  rejectCreate = 'validation';
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
  const retryButton = rejected.children
    .flatMap(child => child.children)
    .find(child => child.tagName === 'BUTTON' &&
      child.dataset.searchtimerAction === 'save');
  assert(retryButton && !retryButton.disabled,
    'definite 422 rejection unlocks the submit button');
  // The promise chain must settle through its finally handler.
  for (let step = 0; step < 8; step++) await Promise.resolve();
  rejectCreate = false;
  rejected.elements.query.value = 'Tatort korrigiert';
  rejected.listeners.submit({preventDefault() {}});
  for (let step = 0; step < 8; step++) await Promise.resolve();
  assert.strictEqual(writes.length, 4,
    'user may explicitly resubmit corrected input after 422');

  mod.renderList({searchTimers: []});
  const ambiguous = findForm(mount);
  ambiguous.elements.query.value = 'Ungewisser Tatort';
  rejectCreate = 'network';
  ambiguous.listeners.submit({preventDefault() {}});
  for (let step = 0; step < 8; step++) await Promise.resolve();
  assert.strictEqual(writes.length, 5);
  ambiguous.listeners.submit({preventDefault() {}});
  assert.strictEqual(writes.length, 5,
    'ambiguous transport failure must never dispatch a second CREATE');
}

async function checkPublicMutations() {
  const mount = new FormElement('main');
  const document = {
    head: new FormElement('head'),
    createElement(tag) { return new FormElement(tag); },
    getElementById() { return null; }
  };
  const searchTimerId = 'st_' + 'a'.repeat(32);
  const revision = 'sr_' + 'b'.repeat(32);
  const backend = 'living-room';
  const writes = [];
  const reads = [];
  const streams = [];
  let reloads = 0;
  let keyCounter = 0;
  let current = null;
  let mutationState = 'succeeded';
  let operationState = 'accepted';
  let rejectVersion = false;
  let loseResponse = false;
  const browser = {
    setTimeout(callback, delay) { if (delay === 300) callback(); },
    confirm() { return true; },
    crypto: {getRandomValues(bytes) { bytes.fill(++keyCounter); return bytes; }}
  };
  const operationId = 'op_' + 'c'.repeat(32);
  function operation(state) { return {operationId, backendId: backend, state}; }
  const client = {
    async fetchPublicSearchTimerDetail() {
      throw new Error('DETAIL returned HTTP 503');
    },
    async fetchPublicSearchTimerVersion(options) {
      assert.strictEqual(options.searchTimerId, searchTimerId);
      assert.strictEqual(options.backend, backend);
      if (rejectVersion) throw new Error('SearchTimer version unavailable');
      return {searchTimerId, backendId: backend, revision,
        query: current.query, state: current.state, repeatCompareSubtitleMode:2,
        repeatCompareTimeMode:3, repeatCompareCategoriesMask:'17', activeMode:current.state === 'active' ? 1 : 0};
    },
    async mutatePublicSearchTimer(options) {
      writes.push(options);
      if (loseResponse) throw new Error('response lost');
      return operation(mutationState);
    },
    async fetchClientSearchTimerOperation(options) {
      assert.strictEqual(options.operationId, operationId);
      reads.push(options.operationId);
      return operation(operationState);
    },
    createClientOperationUpdateSource(id) {
      assert.strictEqual(id, operationId);
      const stream = {
        listeners: {}, closed: false,
        addEventListener(name, callback) { this.listeners[name] = callback; },
        close() { this.closed = true; },
        emit(name, payload) { return this.listeners[name]({data: JSON.stringify(payload)}); }
      };
      streams.push(stream);
      return stream;
    },
    async fetchPublicSearchTimerPreview(options) {
      assert.strictEqual(options.searchTimerId, searchTimerId);
      assert.strictEqual(options.backend, backend);
      return {searchTimerId, backendId: backend, totalCount: 1,
        items: [{title:'Tatort',channelId:'ARD',startTime:'2026-11-17T20:15:00Z'}],
        meta:{partial:false,epgInput:'ready'}};
    }
  };
  vm.runInNewContext(source, {
    window: browser, document, Promise, Object, Array, Boolean, Number,
    Date, String, Set, Math, JSON, Uint8Array
  }, {filename: sourcePath});
  const mod = browser.VdrSuiteSearchTimerBrowser;
  mod.configureContext({
    detailDataElement: mount, clientApi: client,
    getSelectedBackendId() { return backend; }, reload() { reloads++; }
  });

  function find(node, predicate) {
    if (predicate(node)) return node;
    for (const child of node.children) {
      const matched = find(child, predicate);
      if (matched) return matched;
    }
    return null;
  }
  function open(state) {
    current = {searchTimerId, backendId: backend, query:'Tatort', name:'Tatort', state,
      settings:{recordingOptions:{directory:'Doku',priority:50},
        filterOptions:{useChannelMode:2,useTime:true,startTime:930,stopTime:1120},
        channelOptions:{channels:'ARD'}, comparisonOptions:{compareTitle:true,repeatCompareTitle:true,repeatCompareSubtitle:true,repeatCompareSummary:false}}};
    mod.renderList({items:[current]});
    const normalized = mod.normalizeSearchTimer(current, 0);
    assert.strictEqual(normalized.directory, 'Doku');
    assert.strictEqual(normalized.priority, 50);
    assert.strictEqual(normalized.startTime, 930);
    assert.strictEqual(normalized.searchTimerId, searchTimerId);
    assert.strictEqual(normalized.backendNativeId, '');
    return find(mount, node => node.tagName === 'DETAILS' && node.className.startsWith('searchtimer-card'));
  }
  function button(card, label) {
    const found = find(card, node => node.tagName === 'BUTTON' && node.textContent === label);
    assert(found, 'missing action: ' + label);
    return found;
  }

  for (const action of ['update','activate','deactivate','delete']) {
    const card = open(action === 'deactivate' ? 'active' : 'inactive');
    let control;
    if (action === 'update') {
      const edit = button(card,'Suchtimer bearbeiten');
      assert.strictEqual(edit.disabled, false);
      edit.listeners.click();
      const editor = findForm(card);
      assert(editor && editor.dataset.searchtimerEditorForm === 'update');
      assert.strictEqual(editor.elements.query.value,'Tatort');
      assert.strictEqual(editor.elements.priority.value,'50');
      assert.strictEqual(editor.elements.repeatCompareTitle.checked,true);
      editor.elements.query.value = 'Tatort neu';
      editor.elements.priority.value = '70';
      editor.elements.repeatCompareTitle.checked = false;
      editor.elements.repeatCompareSubtitleMode.value = '1';
      await Promise.resolve();
      assert.strictEqual(editor.elements.repeatCompareTimeMode.value,'3');
      control = button(card,'Änderung speichern');
    } else {
      control = button(card, {activate:'Aktivieren',deactivate:'Deaktivieren',delete:'Suchtimer löschen'}[action]);
    }
    assert.strictEqual(control.disabled, false);
    const count = writes.length;
    const pending = control.listeners.click();
    control.listeners.click();
    await pending;
    assert.strictEqual(writes.length, count + 1);
    const request = writes[count];
    assert.strictEqual(request.searchTimerId, searchTimerId);
    assert.strictEqual(request.backend, backend);
    assert.strictEqual(request.revision, revision);
    assert.strictEqual(request.payload.action, action);
    assert.strictEqual(request.payload.confirmed, true);
    assert(/^search-web-[0-9a-f]{32}$/.test(request.idempotencyKey));
    if (action === 'update') {
      assert.strictEqual(request.payload.query, 'Tatort neu');
      assert.strictEqual(request.payload.priority,70);
      assert.strictEqual(request.payload.repeatCompareTitle,false);
      assert.strictEqual(request.payload.repeatCompareSubtitleMode,1);
      assert.strictEqual(request.payload.channels,undefined);
      assert.strictEqual(request.payload.directory,undefined);
      assert.strictEqual(request.payload.repeatCompareTimeMode,undefined);
      assert.strictEqual(request.payload.repeatCompareCategoriesMask,undefined);
      assert.strictEqual(request.payload.active,undefined);
    }
    assert.strictEqual(request.payload.backendNativeId, undefined);
  }
  assert.strictEqual(reloads, 4);
  assert.strictEqual(reads.length, 0);

  // An asynchronous write waits for actor-owned Operation GET, not an
  // untrusted SSE state object or a repeated POST.
  mutationState = 'accepted';
  let card = open('inactive');
  let activate = button(card,'Aktivieren');
  await activate.listeners.click();
  assert.strictEqual(writes.length, 5);
  assert.strictEqual(activate.disabled, true);
  const stream = streams[0];
  assert(stream);
  const mutationFeedback = find(card,node => node.className.startsWith('searchtimer-feedback'));
  const beforePreview = mutationFeedback.textContent;
  await button(card,'Vorschau').listeners.click();
  assert.strictEqual(mutationFeedback.textContent,beforePreview,'preview must not erase operation feedback');
  stream.emit('operation', {operationId:'op_' + 'd'.repeat(32),state:'succeeded'});
  assert.strictEqual(reads.length, 0);
  await stream.emit('hello',{});
  assert.strictEqual(reads.length, 1);
  assert.strictEqual(reloads, 4);
  operationState = 'succeeded';
  stream.emit('operation',{operationId,state:'accepted'});
  for (let i=0;i<6;i++) await Promise.resolve();
  assert.strictEqual(reads.length, 2);
  assert.strictEqual(reloads, 5);
  assert.strictEqual(stream.closed, true);
  assert.strictEqual(writes.length, 5);

  // A changed state in the exact version observation must not dispatch.
  card = open('inactive');
  current = Object.assign({}, current, {state:'active'});
  await button(card,'Aktivieren').listeners.click();
  assert.strictEqual(writes.length, 5, 'stale card must never write');
  card = open('inactive');
  current = Object.assign({}, current, {query:'Andere Regel'});
  await button(card,'Suchtimer löschen').listeners.click();
  assert.strictEqual(writes.length, 5, 'stale query must never delete');

  card = open('inactive');
  rejectVersion = true;
  let remove = button(card,'Suchtimer löschen');
  await remove.listeners.click();
  assert.strictEqual(writes.length, 5, 'missing native version must never write');
  assert.strictEqual(remove.disabled, false);
  rejectVersion = false;
  loseResponse = true;
  await remove.listeners.click();
  assert.strictEqual(writes.length, 6);
  assert.strictEqual(remove.disabled, true);
  await remove.listeners.click();
  assert.strictEqual(writes.length, 6, 'ambiguous transport must never write twice');
  loseResponse = false;

  card = open('active');
  const activeEdit = button(card,'Suchtimer bearbeiten');
  assert.strictEqual(activeEdit.disabled, false);
  await button(card,'Vorschau').listeners.click();
  assert(find(card,node => node.textContent === 'Tatort'));
  assert(find(card,node => node.textContent === 'EPG-Treffer, keine verbindliche Aufnahmeprognose.'));
  assert.strictEqual(writes.length, 6);
  // An active rule uses the same full editor and unchanged saves never write.
  activeEdit.listeners.click();
  const activeForm = findForm(card);
  await Promise.resolve();
  const activeSave = button(card,'Änderung speichern');
  await activeSave.listeners.click();
  assert.strictEqual(writes.length, 6);
  activeForm.elements.lifetime.value = '99';
  const cancel = button(card,'Abbrechen / Zurück');
  cancel.listeners.click();
  assert.strictEqual(activeForm.parentElement.hidden,true);
  activeEdit.listeners.click();
  assert.strictEqual(activeForm.elements.lifetime.value,'99');
  await activeSave.listeners.click();
  assert.strictEqual(writes.length,7);
  assert.strictEqual(writes[6].payload.lifetime,99);
  assert.strictEqual(writes[6].payload.activeMode,undefined);
}
async function checkNamedSelectors() {
  const mount = new FormElement('main'), tasks = [];
  const document = {head:new FormElement('head'),createElement:tag=>new FormElement(tag),getElementById:()=>null};
  const browser = {setTimeout:fn=>tasks.push(fn)};
  vm.runInNewContext(source, {window:browser,document,Promise,Object,Array,Boolean,Number,Date,String,Set,Math,JSON,Uint8Array});
  const mod=browser.VdrSuiteSearchTimerBrowser;
  mod.configureContext({detailDataElement:mount,getSelectedBackendId:()=>'living-room',clientApi:{
    fetchClientChannels:async()=>({channels:[
      {id:'ARD',name:'Das Erste HD',number:1,group:'Public'},
      {id:'ZDF',name:'ZDF HD',number:2,group:'Public'}
    ]}),
    fetchClientRecordingFolder:async()=>({folders:[
      {path:'Filme',singleRecordingLeaf:false},
      {path:'Tatort',singleRecordingLeaf:true}
    ]})
  }});
  mod.renderList({searchTimers:[]});
  tasks.splice(0).forEach(fn=>fn());
  for(let i=0;i<8;i++) await Promise.resolve();
  const form=findForm(mount);
  assert.deepStrictEqual(form.elements.directory.children.map(o=>o.value),['','Filme']);
  assert.strictEqual(form.elements.channelId.children.length,3,
    'initially populate von Sender as well as bis Sender');
  assert.strictEqual(form.elements.channelEndId.children.length,3);
  assert.strictEqual(form.elements.channelId.parentElement.parentElement.hidden,true,
    'all channels hides channel range');
  assert.strictEqual(form.elements.durationMinMinutes.parentElement.hidden,true);
  assert.strictEqual(form.elements.allowedRepeats.parentElement.hidden,true);
  assert.strictEqual(form.elements.startTime.parentElement.hidden,true);
  assert.strictEqual(form.elements.dayOfWeek.parentElement.hidden,true,
    'weekday mask is hidden until weekday filtering is enabled');
  assert.strictEqual(form.elements.useTime.parentElement.parentElement,
    form.elements.startTime.parentElement.parentElement,
    'time toggle and dependent fields share a dedicated group');
  assert.notStrictEqual(form.elements.useTime.parentElement.parentElement,
    form.elements.useDuration.parentElement.parentElement,
    'time and duration filters have separate groups');
  assert.strictEqual(form.elements.manualUseChannel.parentElement.parentElement.hidden,true,
    'all channels conceals manual channel filter');
  form.elements.useDuration.checked=true;
  form.elements.useDuration.listeners.change();
  assert.strictEqual(form.elements.durationMinMinutes.parentElement.hidden,false);
  form.elements.avoidRepeats.checked=true;
  form.elements.avoidRepeats.listeners.change();
  assert.strictEqual(form.elements.allowedRepeats.parentElement.hidden,false);
  form.elements.useTime.checked=true;
  form.elements.useTime.listeners.change();
  assert.strictEqual(form.elements.startTime.parentElement.hidden,false);
  assert.strictEqual(form.elements.stopTime.parentElement.hidden,false);
  assert.strictEqual(form.elements.startTime.type,'time',
    'clock fields must use native browser time controls');
  form.elements.startTime.value='20:00';
  form.elements.stopTime.value='22:00';
  const clockPayload=mod.buildCreatePayload(form);
  assert.strictEqual(clockPayload.startTime,2000);
  assert.strictEqual(clockPayload.stopTime,2200);
  assert.deepStrictEqual(Array.from(mod.validateCreatePayload(clockPayload)),[]);
  form.elements.startTime.value='';
  const incompleteClock=mod.buildCreatePayload(form);
  assert(mod.validateCreatePayload(incompleteClock).some(msg => msg.includes('HH:MM')),
    'empty active clock is rejected rather than becoming 00:00');
  form.elements.startTime.value='22:00';
  form.elements.useTime.checked=false;
  form.elements.useTime.listeners.change();
  assert.strictEqual(form.elements.startTime.parentElement.hidden,true);
  assert.strictEqual(form.elements.startTime.value,'22:00',
    'hidden time values are retained when toggling the control');
  form.elements.useDayOfWeek.checked=true;
  form.elements.useDayOfWeek.listeners.change();
  assert.strictEqual(form.elements.dayOfWeek.parentElement.hidden,false);
  form.elements.useDayOfWeek.checked=false;
  form.elements.useDayOfWeek.listeners.change();
  assert.strictEqual(form.elements.dayOfWeek.parentElement.hidden,true);
  form.elements.useDuration.checked=false;
  form.elements.useDuration.listeners.change();
  assert.strictEqual(form.elements.durationMinMinutes.parentElement.hidden,true,
    'toggle hides fields without erasing values');
  assert(source.includes('.searchtimer-module [hidden]{display:none!important}'));
  form.elements.channelFilterMode.value='1';
  form.elements.channelFilterMode.listeners.change();
  assert.strictEqual(form.elements.manualUseChannel.parentElement.parentElement.hidden,false,
    'manual fallback stays accessible for a channel range');
  assert.strictEqual(form.elements.channelFilterGroup.parentElement.hidden,true,
    'channel group filtering is hidden when choosing a channel range');
  assert(form.elements.channelId.children.some(o=>o.textContent==='1 · Das Erste HD'));
  assert(form.elements.channelEndId.children.some(o=>o.textContent==='2 · ZDF HD'));
  assert.strictEqual(form.elements.channelId.disabled,false,'sender selection works without a group prerequisite');
  form.elements.channelId.value='ARD';
  form.elements.channelEndId.value='ZDF';
  form.elements.channelId.listeners.change();
  assert.strictEqual(form.elements.channelEndId.value,'','new start sender clears old endpoint');
  form.elements.channelEndId.value='ZDF';
  const payload=mod.buildCreatePayload(form);
  assert.strictEqual(payload.channelMin,'ARD');
  assert.strictEqual(payload.channelMax,'ZDF');
  assert.strictEqual(payload.useChannel,1);
  form.elements.channelFilterMode.value='2';
  form.elements.channelFilterMode.listeners.change();
  assert.strictEqual(form.elements.channelId.parentElement.parentElement.hidden,true);
  assert.strictEqual(form.elements.channelFilterGroup.parentElement.hidden,false);
  form.elements.channelFilterMode.value='3';
  form.elements.channelFilterMode.listeners.change();
  assert.strictEqual(form.elements.channelFilterGroup.parentElement.hidden,true);
  assert.strictEqual(form.elements.manualUseChannel.parentElement.parentElement.hidden,true);
  form.elements.channelFilterMode.value='0';
  form.elements.channelFilterMode.listeners.change();
  assert.strictEqual(form.elements.channelId.parentElement.parentElement.hidden,true);
  assert.strictEqual(form.elements.manualUseChannel.parentElement.parentElement.hidden,true);
  assert.strictEqual(mod.buildCreatePayload(form).useChannel,0,
    'hidden selectors do not leak a stale active channel filter');

  // A template may be selected before the deferred catalogue population.
  mod.renderList({items:[{backendId:'living-room',searchTimerId:'st_'+'a'.repeat(32),
    query:'Vorlage',state:'inactive',settings:{recordingOptions:{directory:'Archiv'},
      filterOptions:{useChannelMode:1,useDuration:true,useTime:true,
        startTime:2045,useDayOfWeek:true,dayOfWeek:31},
      channelOptions:{channelMinId:'ARD',channelMaxId:'ZDF'}}}]});
  const copied=findForm(mount);
  copied.closest=()=>copied.parentElement.parentElement;
  copied.scrollIntoView=()=>{};
  function findCopy(node) {
    if (node.tagName==='BUTTON' && node.textContent==='Als Vorlage öffnen') return node;
    for (const child of node.children) {
      const match=findCopy(child);
      if(match) return match;
    }
    return null;
  }
  findCopy(mount).listeners.click();
  copied.elements.manualDirectory.value='Mein~Archiv';
  tasks.splice(0).forEach(fn=>fn());
  for(let i=0;i<8;i++) await Promise.resolve();
  const copiedPayload=mod.buildCreatePayload(copied);
  assert.strictEqual(copied.elements.directory.value,'Archiv');
  assert.strictEqual(copiedPayload.directory,'Mein~Archiv','late catalogue must not erase manual input');
  assert.strictEqual(copiedPayload.useChannel,1);
  assert.strictEqual(copiedPayload.channelMin,'ARD');
  assert.strictEqual(copiedPayload.channelMax,'ZDF');
  assert.strictEqual(copied.elements.durationMinMinutes.parentElement.hidden,false,
    'template populating reopens active duration options');
  assert.strictEqual(copied.elements.startTime.parentElement.hidden,false,
    'template populating reopens active time options');
  assert.strictEqual(copied.elements.dayOfWeek.parentElement.hidden,false,
    'template populating reopens active weekday options');
  assert.strictEqual(copiedPayload.useTime,true);
  assert.strictEqual(copied.elements.startTime.value,'20:45',
    'a native HHMM rule is presented as a human-readable clock');
  assert.strictEqual(copiedPayload.startTime,2045);
  assert(copied.elements.channelId.children.some(o=>o.textContent==='1 · Das Erste HD'));
}
checkEditorFullWrites().then(checkPublicMutations).then(checkNamedSelectors).then(function() {
  console.log('test_searchtimer_workflows_runtime passed');
}).catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
