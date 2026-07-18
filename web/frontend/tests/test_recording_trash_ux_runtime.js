'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.resolve(__dirname, '..', 'recording-trash-ux.js');
const source = fs.readFileSync(sourcePath, 'utf8');

const window = {};
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
  MutationObserver: function MutationObserver() {},
  Object,
  String,
  document,
  window
}, {filename: sourcePath});

const api = window.VdrSuiteRecordingTrashUx;
assert.ok(api);
assert.ok(source.includes("summary.textContent = 'In Papierkorb verschieben'"));
assert.ok(source.includes("status.textContent = 'Papierkorb derzeit nicht verfügbar.'"));
assert.ok(source.includes("validateButton.click()"));
assert.ok(source.includes("executeButton.hidden = true"));
assert.ok(source.includes("Technische Details"));
assert.ok(source.includes("resultBox.hidden = editor.open"));
assert.ok(!source.includes('fetch('));

console.log('test_recording_trash_ux_runtime passed');
