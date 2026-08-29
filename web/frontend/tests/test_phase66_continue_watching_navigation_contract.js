'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-continue-watching.js'), 'utf8');
const appSource = fs.readFileSync(path.join(frontendRoot, 'app.js'), 'utf8');

assert(appSource.includes('function selectModule(moduleName)'));
assert(source.includes("selectShellModule('recordings2')"));
assert(source.includes("backLabel: '← Zurück zu Home'"));
assert(source.includes('onClose: returnHome'));
assert(!source.includes("document.querySelector('[data-module=\"recordings2\"]')"));

const transitions = [];
const opened = [];
let activeModule = 'overview';

const document = {
  readyState: 'loading',
  addEventListener() {},
  querySelector() { return null; }
};

const context = {
  window: {},
  document,
  fetch: async () => ({ok: true, json: async () => ({items: []})}),
  console,
  setTimeout(callback) { return {callback}; },
  clearTimeout() {}
};
context.window.window = context.window;
context.window.document = document;
context.window.fetch = context.fetch;
context.window.setTimeout = context.setTimeout;
context.window.clearTimeout = context.clearTimeout;
context.window.selectModule = function (moduleName) {
  activeModule = moduleName;
  transitions.push(moduleName);
};
context.window.VdrSuiteRecordings2Playback = {createPanel() {}};
context.window.VdrSuiteContinueWatchingSync = {
  clear() { return Promise.resolve(true); }
};
context.window.VdrSuiteRecordings2 = {
  openRecording(recording, options) {
    opened.push({recording, options, activeModule});
  }
};

vm.createContext(context);
vm.runInContext(source, context);

const api = context.window.VdrSuiteHomeContinueWatching;
assert(api && api._test);

const item = {
  backendId: 'default',
  recordingId: 'recording-1',
  title: 'Ein unmoralisches Angebot',
  resumePositionSeconds: 238,
  durationKnown: true,
  durationSeconds: 6585
};

(async function () {
  assert.strictEqual(await api._test.openItem(item, true), true);
  assert.deepStrictEqual(transitions, ['recordings2']);
  assert.strictEqual(opened.length, 1);
  assert.strictEqual(opened[0].activeModule, 'recordings2');
  assert.strictEqual(opened[0].recording.id, 'recording-1');
  assert.strictEqual(opened[0].options.backendId, 'default');
  assert.strictEqual(opened[0].options.autoStartPlayback, true);
  assert.strictEqual(opened[0].options.playbackStartPositionSeconds, 238);
  assert.strictEqual(opened[0].options.continueWatching, true);
  assert.strictEqual(opened[0].options.backLabel, '← Zurück zu Home');
  assert.strictEqual(typeof opened[0].options.onClose, 'function');

  opened[0].options.onClose();
  assert.strictEqual(activeModule, 'overview');
  assert.deepStrictEqual(transitions, ['recordings2', 'overview']);

  console.log('phase66 continue watching navigation contract ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
