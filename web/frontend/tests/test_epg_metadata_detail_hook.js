'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-metadata-detail-hook.js'),
  'utf8'
);

let enhanced = null;
const document = {
  documentElement: {dataset: {}}
};
const window = {
  VdrSuiteEpgMetadataDetail: {
    enhance(detail, event, channel) {
      enhanced = {detail, event, channel};
    }
  },
  setTimeout(callback) {
    callback();
    return 1;
  }
};

const context = vm.createContext({
  Boolean,
  Object,
  document,
  window
});

vm.runInContext(`
  function createEpgEventDetailCard(event, channel) {
    return {kind: 'detail', event: event, channel: channel};
  }
`, context);
vm.runInContext(source, context, {filename: 'epg-metadata-detail-hook.js'});

assert.ok(window.VdrSuiteEpgMetadataDetailHook);
assert.strictEqual(window.VdrSuiteEpgMetadataDetailHook.installed(), true);
assert.strictEqual(document.documentElement.dataset.epgMetadataDetailHook, 'true');

const detail = vm.runInContext(
  `createEpgEventDetailCard({id:'18829'},{id:'channel-1'})`,
  context
);
assert.strictEqual(detail.kind, 'detail');
assert.ok(enhanced);
assert.strictEqual(enhanced.detail, detail);
assert.strictEqual(enhanced.event.id, '18829');
assert.strictEqual(enhanced.channel.id, 'channel-1');

assert.ok(source.includes('createEpgEventDetailCard = function'));
assert.ok(!source.includes('renderEpgTimeView'));
assert.ok(!source.includes('visibleEpgChannelsFromData'));
assert.ok(!source.includes('appendEpgVerticalTimelineTicks'));
assert.ok(!source.includes('createEpgEventCard'));
assert.ok(!source.includes('createEpgProgramEventButton'));

console.log('test_epg_metadata_detail_hook passed');
