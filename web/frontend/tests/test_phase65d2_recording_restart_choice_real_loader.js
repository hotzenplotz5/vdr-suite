'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'recordings2-browser-view.js'),
  'utf8'
);

assert.ok(
  source.includes("global.document.getElementById(id)"),
  'restart helper loader must deduplicate against an existing script element'
);
assert.ok(
  source.includes("global.document.createElement('script')"),
  'recordings view must be able to create the restart helper script itself'
);
assert.ok(
  source.includes("script.src = '/frontend/recording-playback-restart-choice.js'"),
  'restart helper must use the installed public frontend path'
);
assert.ok(
  source.includes("script.addEventListener('load', installRestartChoice"),
  'restart helper must bind to the actual playback owner after script load'
);
assert.ok(
  !source.includes("typeof global.loadVdrSuiteDeferredRuntime === 'function'"),
  'restart choice must not depend on the optional global deferred loader'
);

console.log('phase65d2 recording restart choice real loader contract ok');
