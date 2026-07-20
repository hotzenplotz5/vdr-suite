'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const sourcePath = path.resolve(
  __dirname,
  '..',
  'platform',
  'deferred-runtime-loader.js'
);
const source = fs.readFileSync(sourcePath, 'utf8');

assert.ok(source.includes("'/frontend/epg-searchtimer-actions.js'"));
assert.ok(source.includes("'/frontend/recording-trash-ux.js'"));
assert.ok(source.includes('script.src = src;'));
assert.ok(!source.includes("script.src = src + '?runtime='"));
assert.ok(source.includes('window.VdrSuiteEpgSearchTimerActions'));
assert.ok(source.includes('window.VdrSuiteRecordingTrashUx'));
assert.ok(source.includes('window.VdrSuiteDeferredFrontendRuntimes'));
assert.ok(source.includes('start: startVdrSuiteDeferredFrontendRuntimes'));
assert.ok(source.includes("document.addEventListener(\n      'DOMContentLoaded',"));
assert.ok(source.includes('startVdrSuiteDeferredFrontendRuntimes,\n      {once: true}'));
assert.ok(source.includes('} else {\n    startVdrSuiteDeferredFrontendRuntimes();'));
assert.ok(!source.includes("'/frontend/channel-day-program.js'"));
assert.ok(!source.includes("'/frontend/channel-day-program-compat.js'"));

console.log('test_deferred_frontend_runtime_loader passed');
