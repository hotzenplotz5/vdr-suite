'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(path.join(__dirname, '..', 'home-continue-watching.js'), 'utf8');

function runtime(overrides) {
  const window = {};
  const document = {
    readyState: 'loading',
    addEventListener() {},
    querySelector() { return null; }
  };
  const context = {
    window,
    document,
    fetch: async () => ({ok: true, json: async () => ({items: []})}),
    console,
    setTimeout,
    clearTimeout,
    Promise,
    Object,
    String,
    Number,
    Array,
    Boolean,
    JSON
  };
  Object.assign(window, overrides || {});
  window.window = window;
  window.document = document;
  window.fetch = context.fetch;
  vm.createContext(context);
  vm.runInContext(source, context, {filename: 'home-continue-watching.js'});
  return {window, api: window.VdrSuiteHomeContinueWatching};
}

const item = {
  backendId: 'default',
  recordingId: 'rec-deferred',
  title: 'Deferred Recording',
  resumePositionSeconds: 93,
  durationKnown: true,
  durationSeconds: 600
};

(async function () {
  let previewCancels = 0;
  let loads = 0;
  const opened = [];
  const restartClears = [];
  const events = [];
  const first = runtime({
    VdrSuiteHomeLivePreview: {
      cancel(reason) {
        previewCancels += 1;
        assert.strictEqual(reason, 'Continue Watching geöffnet');
      }
    }
  });
  first.window.VdrSuiteDeferredFrontendRuntimes = {
    loadRecordings2() {
      loads += 1;
      first.window.VdrSuiteContinueWatchingSync = {
        clear(backendId, recordingId) {
          restartClears.push({backendId, recordingId});
          events.push('clear:' + backendId + ':' + recordingId);
          return Promise.resolve(true);
        }
      };
      first.window.VdrSuiteRecordings2 = {
        openRecording(recording, options) {
          opened.push({recording, options});
          events.push('open:' + options.playbackStartPositionSeconds);
        }
      };
      return Promise.resolve();
    }
  };

  assert.strictEqual(await first.api._test.openItem(item, true), true);
  assert.strictEqual(loads, 1, 'fresh Home must load the canonical deferred Recordings runtime');
  assert.strictEqual(previewCancels, 1, 'active/pending preview intent must be released before Recording playback');
  assert.strictEqual(opened.length, 1);
  assert.strictEqual(opened[0].recording.id, 'rec-deferred');
  assert.strictEqual(opened[0].recording.backendId, 'default');
  assert.strictEqual(opened[0].options.autoStartPlayback, true);
  assert.strictEqual(opened[0].options.playbackStartPositionSeconds, 93);
  assert.strictEqual(opened[0].options.continueWatching, true);
  assert.strictEqual(restartClears.length, 0, 'Continue must preserve the saved resume state');

  assert.strictEqual(await first.api._test.openItem(item, false), true);
  assert.strictEqual(loads, 1, 'already loaded Recordings runtime must not be loaded twice');
  assert.strictEqual(opened.length, 2);
  assert.strictEqual(opened[1].options.playbackStartPositionSeconds, 0);
  assert.deepStrictEqual(restartClears, [{backendId: 'default', recordingId: 'rec-deferred'}]);
  assert.deepStrictEqual(events, [
    'open:93',
    'clear:default:rec-deferred',
    'open:0'
  ], 'From beginning must clear the old resume truth before opening canonical playback at zero');

  const unavailable = runtime({
    VdrSuiteHomeLivePreview: {cancel() {}}
  });
  assert.strictEqual(await unavailable.api._test.openItem(item, true), false);

  let testOnlyCancels = 0;
  const testOnlyPreview = runtime({
    VdrSuiteHomeLivePreview: {
      __test: {
        cancelPreview() { testOnlyCancels += 1; }
      }
    },
    VdrSuiteRecordings2: {
      openRecording() {}
    }
  });
  assert.strictEqual(await testOnlyPreview.api._test.openItem(item, true), true);
  assert.strictEqual(testOnlyCancels, 0, 'production Continue Watching must not call preview __test hooks');

  console.log('phase66 continue watching Home runtime composition ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
