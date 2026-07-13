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
assert.strictEqual(typeof api.parentFolderPath, 'function');
assert.strictEqual(typeof api.isTrashDryRunReady, 'function');
assert.strictEqual(typeof api.trashCandidateMatches, 'function');
assert.strictEqual(typeof api.trashSourceOffset, 'function');
assert.strictEqual(typeof api.trashFolderHasContent, 'function');

assert.strictEqual(api.parentFolderPath(''), '');
assert.strictEqual(api.parentFolderPath('Ghibli'), '');
assert.strictEqual(api.parentFolderPath('Ghibli/Anime'), 'Ghibli');
assert.strictEqual(api.trashSourceOffset(
  { title: 'Archiv/Testaufnahme' },
  { path: 'Archiv', offset: 50 }
), 50);
assert.strictEqual(api.trashSourceOffset(
  { title: 'Archiv/Testaufnahme' },
  { path: '', offset: 50 }
), 0);
assert.strictEqual(api.trashFolderHasContent({
  folders: [],
  recordings: [],
  recordingCount: 50
}), true);
assert.strictEqual(api.trashFolderHasContent({
  folders: [],
  recordings: [],
  recordingCount: 0
}), false);

assert.strictEqual(api.isTrashDryRunReady({
  success: false,
  message: 'dry-run backend execution skipped',
  warnings: ['dry-run only'],
  errors: []
}), true);
assert.strictEqual(api.isTrashDryRunReady({
  success: false,
  message: 'recording action execution blocked by safety policy',
  warnings: [],
  errors: ['missing permission: recording.permission.delete']
}), false);
assert.strictEqual(api.isTrashDryRunReady({
  success: true,
  message: 'unexpected real execution',
  warnings: [],
  errors: []
}), false);

const pending = {
  identity: {
    nativeLeaf: '2026-07-13.05.00.1-0.rec',
    title: 'Testaufnahme',
    start: '1783911600',
    duration: '3600'
  }
};

assert.strictEqual(api.trashCandidateMatches({
  backendNativeId: '/srv/vdr/video/VDR-SUITE-TEST/Testaufnahme/2026-07-13.05.00.1-0.rec',
  title: 'VDR-SUITE-TEST/Testaufnahme',
  startTime: '1783911600',
  durationSeconds: 3600
}, { path: 'VDR-SUITE-TEST' }, pending), true);

assert.strictEqual(api.trashCandidateMatches({
  backendNativeId: '/srv/vdr/video/VDR-SUITE-TEST/Andere_Aufnahme/2026-07-13.06.00.1-0.rec',
  title: 'VDR-SUITE-TEST/Andere Aufnahme',
  startTime: '1783915200',
  durationSeconds: 1800
}, { path: 'VDR-SUITE-TEST' }, pending), false);

console.log('recording trash workflow helpers ok');
