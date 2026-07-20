'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-searchtimer-actions.js'),
  'utf8'
);

const scheduled = [];
const document = {
  documentElement: {dataset: {}},
  head: {
    children: [],
    appendChild(element) {
      this.children.push(element);
    }
  },
  getElementById(id) {
    return this.head.children.find(element => element.id === id) || null;
  },
  createElement() {
    return {
      id: '',
      dataset: {},
      style: {},
      classList: {add() {}},
      appendChild() {},
      setAttribute() {},
      addEventListener() {},
      querySelector() { return null; },
      querySelectorAll() { return []; }
    };
  },
  querySelector() { return null; },
  addEventListener() {}
};
const window = {
  VdrSuitePlatform: {
    getSelectedBackendId() {
      return 'default';
    }
  },
  VdrSuiteClientApi: null,
  setTimeout(callback) {
    scheduled.push(callback);
    return scheduled.length;
  },
  alert() {}
};

const context = vm.createContext({
  Array,
  Boolean,
  Date,
  Error,
  Event: function Event() {},
  Map,
  Math,
  Number,
  Object,
  Promise,
  Set,
  String,
  console,
  document,
  window
});

vm.runInContext(source, context, {filename: 'epg-searchtimer-actions.js'});
assert.ok(window.VdrSuiteEpgSearchTimerActions);
assert.strictEqual(window.VdrSuiteEpgSearchTimerActions.timelineEnhancementsReady(), false);
assert.strictEqual(scheduled.length, 1);

vm.runInContext(`
  let epgChannelOffset = 0;
  let selectedEpgDetail = null;
  const EPG_VISIBLE_CHANNEL_LIMIT = 15;
  const detailDataElement = document.createElement('section');
  function visibleEpgChannelsFromData(data) { return data.channels || []; }
  function createEpgEventCard() { return document.createElement('button'); }
  function createEpgProgramEventButton() { return document.createElement('button'); }
  function createEpgEventDetailCard() { return document.createElement('article'); }
  function renderEpgTimeView() {}
  function loadEpgTimeline() {}
`, context);

scheduled.shift()();
assert.strictEqual(window.VdrSuiteEpgSearchTimerActions.timelineEnhancementsReady(), true);
assert.strictEqual(document.documentElement.dataset.epgTimelineEnhancements, 'true');
assert.strictEqual(scheduled.length, 0);

console.log('test_epg_timeline_deferred_install passed');
