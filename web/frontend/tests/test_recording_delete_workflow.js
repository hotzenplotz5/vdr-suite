"use strict";

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const registeredModules = Object.create(null);
global.window = {
  VdrSuitePlatform: {
    hasModule(name) {
      return Object.prototype.hasOwnProperty.call(registeredModules, name);
    },
    registerModule(name, moduleApi) {
      registeredModules[name] = moduleApi;
    }
  }
};

vm.runInThisContext(
  fs.readFileSync('web/frontend/modules/recordings.js', 'utf8'),
  { filename: 'web/frontend/modules/recordings.js' }
);

const api = window.VdrSuiteRecordingBrowser;
assert.ok(api);
assert.strictEqual(typeof api.sameRecording, 'function');
assert.strictEqual(typeof api.folderContainsRecording, 'function');

const expected = {
  recordingId: 'rec-1',
  path: '/srv/vdr/video/Test/2026-01-01.00.00.1-0.rec',
  title: 'Test',
  startTime: 1767222000,
  durationSeconds: 3600
};

assert.strictEqual(api.sameRecording(expected, {
  recordingId: 'rec-1',
  path: '/different/path'
}), true);
assert.strictEqual(api.sameRecording(expected, {
  recordingId: 'rec-2',
  path: expected.path
}), false);
assert.strictEqual(api.sameRecording(
  { title: 'Fallback', startTime: 1, durationSeconds: 2 },
  { title: 'Fallback', startTime: 1, durationSeconds: 2 }
), true);
assert.strictEqual(api.folderContainsRecording({ recordings: [expected] }, expected), true);
assert.strictEqual(api.folderContainsRecording({ recordings: [] }, expected), false);
assert.strictEqual(api.folderContainsRecording({ recordings: [{
  recordingId: 'other',
  path: '/srv/vdr/video/Other/2026.rec'
}] }, expected), false);

const source = fs.readFileSync('web/frontend/modules/recordings.js', 'utf8');
assert.ok(source.includes("mode: 'validate'"));
assert.ok(source.includes("action: 'DELETE'"));
assert.ok(source.includes("dryRun: false"));
assert.ok(source.includes('recordingBrowserScheduleDeleteFolderReload();'));
assert.ok(!source.includes('Löschen Dry-Run (keine Ausführung)'));

console.log('recording delete workflow helpers ok');
