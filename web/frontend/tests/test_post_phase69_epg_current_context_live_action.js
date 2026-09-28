'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'app.js'),
  'utf8'
);

function functionSlice(name, nextName) {
  const start = source.indexOf('function ' + name + '(');
  assert(start >= 0, name + ' must exist');
  const end = source.indexOf('function ' + nextName + '(', start + 1);
  assert(end > start, name + ' must end before ' + nextName);
  return source.slice(start, end);
}

const boundsSource = functionSlice(
  'epgTimelineBounds',
  'epgTimelineDisplayBounds'
);
const displayBoundsSource = functionSlice(
  'epgTimelineDisplayBounds',
  'epgTimelineCurrentWindowEvents'
);
const currentEventsSource = functionSlice(
  'epgTimelineCurrentWindowEvents',
  'epgTimelinePercent'
);
const nowLineSource = functionSlice(
  'appendEpgNowLine',
  'appendEpgVerticalTimelineTicks'
);
const positionSource = functionSlice(
  'epgEventPositionForBounds',
  'appendEpgTimelineTicks'
);
const liveActionSource = functionSlice(
  'openEpgChannelLive',
  'createEpgEventDetailCard'
);
const detailSource = functionSlice(
  'createEpgEventDetailCard',
  'renderEpgSideDetail'
);

assert(source.includes('const EPG_TIMELINE_CONTEXT_BEFORE_SECONDS = 60 * 60;'));
assert(source.includes('const EPG_TIMELINE_MIN_CONTEXT_BEFORE_SECONDS = 30 * 60;'));
assert(nowLineSource.includes('epgTimelinePercent(nowSeconds, bounds)'));
assert(positionSource.includes('epgTimelinePercent(visibleStart, bounds)'));

const context = vm.createContext({
  Math,
  Number,
  Array,
  Set,
  Object,
  String
});

vm.runInContext(`
  const EPG_TIMELINE_VISIBLE_SECONDS = 24 * 60 * 60;
  const EPG_TIMELINE_CONTEXT_BEFORE_SECONDS = 60 * 60;
  const EPG_TIMELINE_MIN_CONTEXT_BEFORE_SECONDS = 30 * 60;
  const EPG_TIMELINE_WINDOW_ANCHOR_SECONDS = 30 * 60;
  const EPG_TIMELINE_MAX_PAGE_OFFSET = 1;
  let epgTimeWindowPageOffset = 0;

  function firstValue(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
        return object[key];
      }
    }
    return fallback;
  }
  function parseFrontendEventEpoch(value) {
    const number = Number(value);
    return Number.isFinite(number) && number > 0 ? Math.floor(number) : 0;
  }
  function frontendChannelId(channel) {
    return String(firstValue(channel, ['id', 'channelId', 'nativeId'], '')).trim();
  }
  function frontendEventChannelId(event) {
    return String(firstValue(event, ['channelId', 'channel', 'channel_id'], '')).trim();
  }
  function frontendEventEnd(event, start) {
    const explicitEnd = parseFrontendEventEpoch(firstValue(event, ['endTime', 'end', 'stopTime'], ''));
    if (explicitEnd > start) return explicitEnd;
    const duration = Number(firstValue(event, ['durationSeconds', 'duration'], 0));
    return Number.isFinite(duration) && duration > 0 && start > 0 ? start + duration : 0;
  }

  ${boundsSource}
  ${displayBoundsSource}
  ${currentEventsSource}
`, context);

const now = 100000;
const channels = [{id: 'a'}, {id: 'b'}];
const longRunning = {channelId: 'a', start: 90000, end: 101000};
const recentRunning = {channelId: 'b', start: 99000, end: 101000};
const completed = {channelId: 'a', start: 95000, end: 99999};
const future = {channelId: 'a', start: 100500, end: 101500};

context.now = now;
context.channels = channels;
context.events = [longRunning, recentRunning, completed, future];

const longBounds = vm.runInContext(
  'epgTimelineDisplayBounds(now, channels, events)',
  context
);
assert.strictEqual(
  longBounds.start,
  now - 60 * 60,
  'a very long current programme must be clipped at the bounded 60-minute lookback'
);

context.events = [recentRunning, completed, future];
const recentBounds = vm.runInContext(
  'epgTimelineDisplayBounds(now, channels, events)',
  context
);
assert.strictEqual(
  recentBounds.start,
  now - 30 * 60,
  'the current view must keep at least 30 minutes of context even when programmes started recently'
);

context.events = [completed, future];
const noCurrentBounds = vm.runInContext(
  'epgTimelineDisplayBounds(now, channels, events)',
  context
);
assert.strictEqual(noCurrentBounds.start, now - 30 * 60);

context.events = [longRunning, recentRunning, completed, future];
const currentWindow = vm.runInContext(
  'epgTimelineCurrentWindowEvents(events, now)',
  context
);
assert.deepStrictEqual(
  Array.from(currentWindow, event => event.start),
  [90000, 99000, 100500],
  'completed predecessor programmes must not be forced into the current timeline'
);

vm.runInContext('epgTimeWindowPageOffset = 1', context);
const nextWindow = vm.runInContext(
  'epgTimelineCurrentWindowEvents(events, now)',
  context
);
assert.strictEqual(
  nextWindow.length,
  4,
  'future-page browsing must retain the full fetched event window'
);

assert(liveActionSource.includes('window.VdrSuiteLiveTvView'));
assert(liveActionSource.includes('liveEntry.click();'));
assert(liveActionSource.includes('liveOwner.startChannel(channel)'));
assert(!liveActionSource.includes('VdrSuiteClientApi'));
assert(!liveActionSource.includes('/api/media/sessions'));
assert(!liveActionSource.includes('createLivePanel'));
assert(detailSource.includes("'Sender live'"));
assert(detailSource.includes('openEpgChannelLive(detail, channel'));
assert(!detailSource.includes("createEpgDetailAction('HbbTV'"));
assert(!detailSource.includes("createEpgDetailAction('EPG'"));

console.log('post-phase69 EPG current context and canonical Live action ok');
