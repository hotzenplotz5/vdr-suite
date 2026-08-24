'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const sourcePath = 'web/frontend/live-tv-view.js';
const source = fs.readFileSync(sourcePath, 'utf8');

function node() {
  return {
    id: '',
    className: '',
    dataset: {},
    style: {},
    textContent: '',
    hidden: false,
    appendChild() {},
    setAttribute() {},
    addEventListener() {},
    classList: {add() {}, remove() {}}
  };
}

const document = {
  readyState: 'complete',
  head: {appendChild() {}},
  createElement() { return node(); },
  getElementById() { return null; },
  querySelector() { return null; },
  querySelectorAll() { return []; },
  addEventListener() {}
};

const window = {document};

vm.runInNewContext(source, {
  window,
  document,
  Object,
  String,
  Number,
  Array,
  Boolean,
  Promise,
  RegExp,
  Error,
  Date,
  Math
}, {filename: sourcePath});

assert.ok(window.VdrSuiteLiveTvView);
const test = window.VdrSuiteLiveTvView.__test;

assert.strictEqual(test.channelHasEncryptionInfo({encrypted: true}), true);
assert.strictEqual(test.channelHasEncryptionInfo({encrypted: false}), true);
assert.strictEqual(test.channelHasEncryptionInfo({caids: ['09C7']}), true);
assert.strictEqual(test.channelHasEncryptionInfo({}), false);

assert.strictEqual(test.channelIsEncrypted({encrypted: true}), true);
assert.strictEqual(test.channelIsEncrypted({encrypted: false}), false);
assert.strictEqual(test.channelIsEncrypted({encrypted: 'false'}), false);
assert.strictEqual(test.channelIsEncrypted({scrambled: '1'}), true);
assert.strictEqual(test.channelIsEncrypted({caids: ['09C7', '09EF']}), true);
assert.strictEqual(test.channelIsEncrypted({caids: []}), false);

assert.strictEqual(
  test.channelAvailabilityText({number: 46, encrypted: true, enabled: true}),
  'Kanal 46 · verschlüsselt'
);
assert.strictEqual(
  test.channelAvailabilityText({number: 1, encrypted: false, enabled: true}),
  'Kanal 1 · frei'
);
assert.strictEqual(
  test.channelAvailabilityText({number: 2, enabled: true}),
  'Kanal 2 · verfügbar'
);
assert.strictEqual(
  test.channelAvailabilityText({number: 3, encrypted: true, enabled: false}),
  'Kanal 3 · deaktiviert'
);

const encryptedError = test.liveErrorForChannel(
  new Error('live_source_receiver_unavailable'),
  {name: 'Sky Test', encrypted: true},
  'fallback'
);
assert.ok(encryptedError.includes('Sky Test'));
assert.ok(encryptedError.includes('verschlüsselt'));
assert.ok(encryptedError.includes('VDR konnte aktuell keinen Live-Empfang'));
assert.ok(!encryptedError.includes('live_source_receiver_unavailable'));

assert.strictEqual(
  test.liveErrorForChannel(
    new Error('live_source_receiver_unavailable'),
    {name: 'Das Erste HD', encrypted: false},
    'fallback'
  ),
  'live_source_receiver_unavailable'
);
assert.strictEqual(
  test.liveErrorForChannel(
    new Error('Security accountability persistence is unavailable'),
    {name: 'Sky Test', encrypted: true},
    'fallback'
  ),
  'Security accountability persistence is unavailable'
);

assert.ok(
  source.includes("state.liveError = liveErrorForChannel(error, channel, 'Live-TV konnte nicht gestartet werden.');"),
  'the owning Live-TV view must classify failed startup with the selected channel metadata'
);
assert.ok(
  source.includes("const meta = addText(doc.createElement('span'), channelAvailabilityText(channel));"),
  'the owning Live-TV tile must expose encryption status before playback starts'
);

console.log('Live-TV encryption presentation and failure context contract ok');
