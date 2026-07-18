'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const sourcePath = path.resolve(__dirname, '..', 'channel-logos.js');
const source = fs.readFileSync(sourcePath, 'utf8');

assert.ok(source.includes("'/frontend/channel-day-program.js'"));
assert.ok(source.includes("'/frontend/channel-day-program-compat.js'"));
assert.ok(source.includes("'/frontend/recording-trash-ux.js'"));
assert.ok(source.includes("window.addEventListener('load', startVdrSuiteDeferredFrontendRuntimes"));
assert.ok(source.includes("script.src = src + '?runtime='"));
assert.ok(source.includes('window.VdrSuiteChannelDayProgram'));
assert.ok(source.includes('window.VdrSuiteRecordingTrashUx'));

console.log('test_deferred_frontend_runtime_loader passed');
