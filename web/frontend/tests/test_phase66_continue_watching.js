'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-continue-watching.js'), 'utf8');
const syncSource = fs.readFileSync(path.join(frontendRoot, 'api', 'continue-watching-sync.js'), 'utf8');
const fallbackSource = fs.readFileSync(path.join(frontendRoot, 'api', 'recording-fallback-controls.js'), 'utf8');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');

// Architectural fences: server/actor truth, canonical existing playback owner,
// preview release before ownership transfer, and no browser-local cross-client truth.
assert(source.includes('/api/media/continue-watching'));
assert(syncSource.includes('/api/media/continue-watching'));
assert(!source.includes('localStorage'));
assert(!syncSource.includes('localStorage'));
assert(source.includes('VdrSuiteRecordings2.openRecording'));
assert(source.includes('releasePreview()'));
assert(source.includes('cancelPreview'));
assert(!source.includes('MediaSession'));
assert(!source.includes("document.createElement('video')"));
assert(!syncSource.includes("document.createElement('video')"));
assert(syncSource.includes('startAtAbsolute'));
assert(!syncSource.includes('owner.resume(target)'));
assert(fallbackSource.includes('startAtAbsolute: startAt'));
assert(indexSource.includes('data-home-zone="additional-sections"'));

const context = {
    window: {},
    document: {
        readyState: 'loading',
        addEventListener() {},
        querySelector() { return null; }
    },
    fetch: async () => ({ok: true, json: async () => ({items: []})}),
    console,
    setTimeout,
    clearTimeout
};
context.window.window = context.window;
context.window.document = context.document;
context.window.fetch = context.fetch;
vm.createContext(context);
vm.runInContext(source, context);

const api = context.window.VdrSuiteHomeContinueWatching;
assert(api);
assert(api._test);

const base = {
    backendId: 'default',
    recordingId: 'r1',
    title: 'Film',
    resumePositionSeconds: 120,
    durationKnown: true,
    durationSeconds: 600,
    playbackCapable: true
};
assert.strictEqual(api._test.normalizeItem(base).recordingId, 'r1');
assert.strictEqual(api._test.normalizeItem({...base, resumePositionSeconds: 0}), null);
assert.strictEqual(api._test.normalizeItem({...base, resumePositionSeconds: 600}), null);
assert.strictEqual(api._test.normalizeItem({...base, playbackCapable: false}), null);

const known = api._test.progressModel(base);
assert.strictEqual(known.percent, 20);
assert.strictEqual(known.hasPercent, true);
const unknown = api._test.progressModel({...base, durationKnown: false, durationSeconds: 0});
assert.strictEqual(unknown.hasPercent, false);
assert.strictEqual(unknown.percent, null);

// Continue and From beginning both route through the same Recordings2 playback owner.
let opened = [];
context.window.VdrSuiteRecordings2 = {
    openRecording(recording, options) { opened.push({recording, options}); }
};
api._test.openItem(base, true);
api._test.openItem(base, false);
assert.strictEqual(opened.length, 2);
assert.strictEqual(opened[0].recording.id, 'r1');
assert.strictEqual(opened[0].options.playbackStartPositionSeconds, 120);
assert.strictEqual(opened[0].options.autoStartPlayback, true);
assert.strictEqual(opened[1].options.playbackStartPositionSeconds, 0);
assert.strictEqual(opened[1].options.autoStartPlayback, true);

// Execute the canonical absolute-start helper itself. HLS exposes the existing
// server-side startAt path; fast playback stays on the same owner and performs
// its already-supported absolute seek after start. Position zero is a normal start.
const syncContext = {
    window: {},
    fetch: async () => ({ok: true}),
    console,
    setTimeout,
    clearTimeout,
    MutationObserver: class {
        observe() {}
        disconnect() {}
    }
};
syncContext.window.window = syncContext.window;
syncContext.window.fetch = syncContext.fetch;
syncContext.window.setTimeout = setTimeout;
syncContext.window.clearTimeout = clearTimeout;
syncContext.window.MutationObserver = syncContext.MutationObserver;
vm.createContext(syncContext);
vm.runInContext(syncSource, syncContext);
const syncApi = syncContext.window.VdrSuiteContinueWatchingSync;
assert(syncApi && syncApi.__test);

(async function () {
    const hlsCalls = [];
    await syncApi.__test.startAtAbsolute({
        startAtAbsolute(position) {
            hlsCalls.push(position);
            return Promise.resolve('hls-session');
        }
    }, 120);
    assert.deepStrictEqual(hlsCalls, [120]);

    const fastCalls = [];
    await syncApi.__test.startAtAbsolute({
        start() {
            fastCalls.push('start');
            return Promise.resolve('fast-session');
        },
        seekAbsolute(position) {
            fastCalls.push('seek:' + position);
            return Promise.resolve('fast-replacement');
        }
    }, 120);
    assert.deepStrictEqual(fastCalls, ['start', 'seek:120']);

    const restartCalls = [];
    await syncApi.__test.startAtAbsolute({
        start() {
            restartCalls.push('start');
            return Promise.resolve('restart-session');
        }
    }, 0);
    assert.deepStrictEqual(restartCalls, ['start']);

    console.log('phase66 continue watching contract ok');
}()).catch(function (error) {
    console.error(error);
    process.exitCode = 1;
});
