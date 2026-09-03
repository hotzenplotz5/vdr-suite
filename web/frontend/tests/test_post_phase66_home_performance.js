'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'home-live-hero.js'),
  'utf8'
);

function between(startMarker, endMarker) {
  const start = source.indexOf(startMarker);
  const end = source.indexOf(endMarker, start + startMarker.length);
  assert(start >= 0, 'missing source marker: ' + startMarker);
  assert(end > start, 'missing source end marker: ' + endMarker);
  return source.slice(start, end);
}

assert(source.includes('eventsByChannel: new Map()'));
assert(source.includes('function rebuildEventIndex()'));
assert(source.includes('state.eventsByChannel = index;'));

const channelEventsSource = between(
  'function channelEvents(channel, events)',
  'function currentEventForChannel('
);
assert(channelEventsSource.includes('events === state.events'));
assert(channelEventsSource.includes('state.eventsByChannel.get(id) || []'));

const applyProgramsSource = between(
  'function applyPrograms(data, append)',
  'function loadProgrammePage('
);
assert.strictEqual(
  (applyProgramsSource.match(/rebuildEventIndex\(\)/g) || []).length,
  2,
  'both reset and append EPG updates must rebuild the channel index'
);

const renderSource = between(
  'function render(options)',
  'function selectIndex('
);
assert(renderSource.includes("if (config.programmeRails !== false) renderProgrammeRails();"));

const selectSource = between(
  'function selectIndex(index)',
  'function selectOffset('
);
assert(selectSource.includes('render({programmeRails: false});'));
assert(!selectSource.includes('renderProgrammeRails'));

assert(source.includes('function clearPrograms()'));
assert(source.includes('state.eventsByChannel = new Map();'));

console.log('post-phase66 Home hero performance hotpath contract ok');
