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

const sourcePath = path.resolve(__dirname, '..', 'modules', 'timers.js');
const source = fs.readFileSync(sourcePath, 'utf8');

vm.runInNewContext(source, {
  Array,
  Boolean,
  Date,
  Map,
  Math,
  Number,
  Object,
  Promise,
  Set,
  String,
  window
}, {filename: sourcePath});

const api = window.VdrSuiteTimerBrowser;
assert.ok(api);
assert.strictEqual(registered.timers, api);

api.configureContext({
  getSelectedBackendId() {
    return 'living-room';
  }
});

const timer = api.normalizeTimer({
  id: '42',
  channelId: 'C-1-2-3',
  channelName: 'Das Erste HD',
  title: 'Tagesschau',
  directory: 'News',
  day: '2026-07-19',
  weekdays: '-------',
  startTime: '2015',
  endTime: '2030',
  flags: 5,
  priority: 50,
  lifetime: 99,
  enabled: true,
  vps: true,
  recording: false,
  pending: true,
  aux: '<epgsearch></epgsearch>'
}, 0);

assert.strictEqual(timer.timerId, '42');
assert.strictEqual(timer.directory, 'News');
assert.strictEqual(timer.start, 2015);
assert.strictEqual(timer.stop, 2030);
assert.strictEqual(timer.active, true);
assert.strictEqual(timer.vps, true);
assert.strictEqual(timer.pending, true);
assert.strictEqual(api.timeToInput(2015), '20:15');
assert.strictEqual(api.inputTimeToHhmm('20:30'), 2030);
assert.strictEqual(api.inputTimeToHhmm('25:00'), 0);
assert.strictEqual(api.weekdaysFromValues([0, 2, 4]), 'M-W-F--');
assert.strictEqual(api.normalizeWeekdays('MTWTF--'), 'MTWTF--');

const payload = api.timerActionPayload(timer, {active: false});
assert.strictEqual(payload.backendId, 'living-room');
assert.strictEqual(payload.timerId, '42');
assert.strictEqual(payload.channelId, 'C-1-2-3');
assert.strictEqual(payload.title, 'Tagesschau');
assert.strictEqual(payload.day, '2026-07-19');
assert.strictEqual(payload.weekdays, '-------');
assert.strictEqual(payload.start, 2015);
assert.strictEqual(payload.stop, 2030);
assert.strictEqual(payload.active, false);
assert.strictEqual(payload.vps, true);
assert.deepStrictEqual(Array.from(api.validateTimerPayload(payload, true)), []);

const invalid = api.timerActionPayload(timer, {
  timerId: '',
  channelId: '',
  title: '',
  day: '',
  weekdays: '-------',
  start: 0,
  stop: 0
});
assert.ok(api.validateTimerPayload(invalid, true).length >= 4);

const channels = [
  api.normalizeChannel({id: 'S19.2E-1-1019-10301', number: 1, name: 'Das Erste HD', group: 'Öffentlich-rechtlich'}, 0),
  api.normalizeChannel({id: 'S19.2E-1-1011-11110', number: 2, name: 'ZDF HD', group: 'Öffentlich-rechtlich'}, 1),
  api.normalizeChannel({id: 'S19.2E-1-1089-12003', number: 3, name: 'RTL Television', group: 'Privat'}, 2),
  api.normalizeChannel({id: 'S19.2E-1-1101-28106', number: 4, name: 'Radio Test', group: '', radio: true}, 3)
];

assert.deepStrictEqual(
  Array.from(api.channelGroups(channels)),
  ['Öffentlich-rechtlich', 'Privat', '__ungrouped__']
);
assert.deepStrictEqual(
  Array.from(api.channelsForGroup(channels, 'Öffentlich-rechtlich')).map(channel => channel.id),
  ['S19.2E-1-1019-10301', 'S19.2E-1-1011-11110']
);
assert.strictEqual(api.channelOptionLabel(channels[0]), '1 · Das Erste HD');
assert.deepStrictEqual(
  Array.from(api.channelGroups([
    api.normalizeChannel({id: 'A', number: 1, name: 'A', group: ''}, 0),
    api.normalizeChannel({id: 'B', number: 2, name: 'B', group: ''}, 1)
  ])),
  []
);

assert.ok(!source.includes('fetch('));
assert.ok(source.includes('function renderList(data, conflictReport)'));
assert.ok(source.includes('function renderConflicts(report, timers, error)'));
assert.ok(source.includes("createField('Kanalgruppe', groupSelect, true)"));
assert.ok(source.includes("createField('Kanal auswählen', channelSelect, true)"));
assert.ok(source.includes('Expertenoption: Kanal-ID manuell eingeben'));
assert.ok(source.includes('fetchClientChannels'));

console.log('test_timer_workflows_runtime passed');
