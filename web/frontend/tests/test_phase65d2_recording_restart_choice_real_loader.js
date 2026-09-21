'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'recordings2-browser-view.js'),
  'utf8'
);

const preloadCall = source.indexOf(
  "if (typeof global.loadVdrSuiteDeferredRuntime === 'function') ensureRestartChoiceRuntime()"
);
const firstViewFactory = source.indexOf('function createRecordingCard');
const detailBinding = source.indexOf(
  'ensureRestartChoiceRuntime().then(function (helper)'
);

assert.ok(
  source.includes("'vdr-suite-recording-playback-restart-choice-runtime'"),
  'restart helper preload must have a stable deduplication id'
);
assert.ok(
  source.includes("'/frontend/recording-playback-restart-choice.js'"),
  'restart helper preload must use the installed public frontend path'
);
assert.ok(
  preloadCall >= 0 && firstViewFactory >= 0 && preloadCall < firstViewFactory,
  'restart helper loading must start when the BrowserView runtime loads, before recording interaction'
);
assert.ok(
  detailBinding >= 0,
  'recording detail must bind the actual playback owner through the preloaded runtime promise'
);
assert.ok(
  !source.includes("global.document.createElement('script')"),
  'recording detail must not contain a second late ad-hoc script loader'
);

console.log('phase65d2 recording restart choice preload contract ok');
