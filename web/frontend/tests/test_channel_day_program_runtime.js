'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.resolve(__dirname, '..', 'channel-day-program.js');
const source = fs.readFileSync(sourcePath, 'utf8');

const window = {
  VdrSuitePlatform: {
    getSelectedBackendId() {
      return 'living-room';
    },
    getClientApi() {
      return null;
    }
  }
};

const document = {
  readyState: 'loading',
  addEventListener() {},
  getElementById() {
    return null;
  }
};

vm.runInNewContext(source, {
  Array,
  Boolean,
  Date,
  Event: function Event() {},
  JSON,
  Math,
  MutationObserver: function MutationObserver() {},
  Number,
  Object,
  Option: function Option() {},
  Promise,
  Set,
  String,
  document,
  window
}, {filename: sourcePath});

const api = window.VdrSuiteChannelDayProgram;
assert.ok(api);

const channel = api.parseChannelButtonMetadata(
  'Nr. 1 · C-1-1051-10301',
  'Das Erste HD'
);
assert.strictEqual(channel.id, 'C-1-1051-10301');
assert.strictEqual(channel.number, '1');
assert.strictEqual(channel.name, 'Das Erste HD');

const bounds = api.dayBounds('2026-07-18');
assert.ok(bounds.start > 0);
assert.ok(bounds.end > bounds.start);
assert.ok(bounds.end - bounds.start >= 23 * 60 * 60);
assert.ok(bounds.end - bounds.start <= 25 * 60 * 60);
assert.strictEqual(api.dateInputValue(new Date(2026, 6, 18, 22, 30)), '2026-07-18');

const events = [
  {
    eventId: 'event-1',
    channelId: 'C-1-1051-10301',
    title: 'Tagesschau',
    startTime: bounds.start + 20 * 60 * 60,
    endTime: bounds.start + 20 * 60 * 60 + 15 * 60
  },
  {
    eventId: 'event-other',
    channelId: 'OTHER',
    title: 'Andere Sendung',
    startTime: bounds.start + 21 * 60 * 60,
    endTime: bounds.start + 22 * 60 * 60
  },
  {
    eventId: 'event-next-day',
    channelId: 'C-1-1051-10301',
    title: 'Morgen',
    startTime: bounds.end + 10,
    endTime: bounds.end + 1000
  }
];

const entries = Array.from(api.eventEntriesForDay(events, channel.id, bounds));
assert.strictEqual(entries.length, 1);
assert.strictEqual(entries[0].event.eventId, 'event-1');

const timerPayload = api.buildTimerPayload(events[0], channel);
assert.strictEqual(timerPayload.backendId, 'living-room');
assert.strictEqual(timerPayload.channelId, channel.id);
assert.strictEqual(timerPayload.title, 'Tagesschau');
assert.strictEqual(timerPayload.day, '2026-07-18');
assert.strictEqual(timerPayload.weekdays, '-------');
assert.strictEqual(timerPayload.active, true);
assert.ok(timerPayload.aux.includes('eventId=event-1'));

const groupedChannel = api.normalizeChannel(Object.assign({}, channel, {
  group: 'DieOeffentlichen'
}));
const baseSearchTimer = api.buildSeriesSearchTimerPayload(events[0], groupedChannel);
assert.strictEqual(baseSearchTimer.backendId, 'living-room');
assert.strictEqual(baseSearchTimer.query, 'Tagesschau');
assert.strictEqual(baseSearchTimer.avoidRepeats, true);
assert.strictEqual(baseSearchTimer.compareTitle, true);
assert.strictEqual(baseSearchTimer.useChannel, 1);
assert.strictEqual(baseSearchTimer.channelMin, groupedChannel.id);
assert.strictEqual(baseSearchTimer.channelMax, groupedChannel.id);

const groupScope = api.applySeriesScope(baseSearchTimer, groupedChannel, 'group');
assert.strictEqual(groupScope.useChannel, 2);
assert.strictEqual(groupScope.channels, 'DieOeffentlichen');
assert.strictEqual(groupScope.channelMin, '');
assert.strictEqual(groupScope.channelMax, '');

const allScope = api.applySeriesScope(baseSearchTimer, groupedChannel, 'all');
assert.strictEqual(allScope.useChannel, 0);
assert.strictEqual(allScope.channels, '');
assert.strictEqual(allScope.channelMin, '');
assert.strictEqual(allScope.channelMax, '');

assert.ok(!source.includes('fetch('));
assert.ok(source.includes('← Zurück zur Kanalliste'));
assert.ok(source.includes('Serie automatisch aufnehmen'));
assert.ok(source.includes('Erweiterter SearchTimer'));
assert.ok(source.includes('fetchClientEpgCacheWindow'));
assert.ok(source.includes('fetchClientEpgChannelWindow'));
assert.ok(source.includes('fetchClientTimerCreateAction'));
assert.ok(source.includes('fetchClientSearchTimerPreview'));
assert.ok(source.includes('fetchClientSearchTimerCreateAction'));

console.log('test_channel_day_program_runtime passed');
