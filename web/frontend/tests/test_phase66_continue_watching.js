'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-continue-watching.js'), 'utf8');
const syncSource = fs.readFileSync(path.join(frontendRoot, 'api', 'continue-watching-sync.js'), 'utf8');
const playbackSource = fs.readFileSync(path.join(frontendRoot, 'recordings2-playback.js'), 'utf8');
const fallbackSource = fs.readFileSync(path.join(frontendRoot, 'api', 'recording-fallback-controls.js'), 'utf8');
const previewSource = fs.readFileSync(path.join(frontendRoot, 'home-live-preview.js'), 'utf8');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');

// Architectural fences: server/actor truth, canonical owner, no browser-local truth.
assert(source.includes('/api/media/continue-watching'));
assert(syncSource.includes('/api/media/continue-watching'));
assert(!source.includes('localStorage'));
assert(!syncSource.includes('localStorage'));
assert(source.includes('VdrSuiteRecordings2.openRecording'));
assert(!source.includes('MediaSession'));
assert(!source.includes('document.createElement(\'video\')'));
assert(playbackSource.includes('startAtAbsolute'));
assert(fallbackSource.includes('startAtAbsolute'));
assert(previewSource.includes('[data-home-continue-action]'));
assert(indexSource.includes('data-home-zone="additional-sections"'));

// Execute the production formatting/filter helpers. Unknown duration must never
// fabricate a percentage; position zero/completed/non-capable entries fail closed.
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

console.log('phase66 continue watching contract ok');
