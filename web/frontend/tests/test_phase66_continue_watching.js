'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const repositoryRoot = path.join(frontendRoot, '..', '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-continue-watching.js'), 'utf8');
const homePreviewSource = fs.readFileSync(path.join(frontendRoot, 'home-live-preview.js'), 'utf8');
const syncSource = fs.readFileSync(path.join(frontendRoot, 'api', 'continue-watching-sync.js'), 'utf8');
const fallbackSource = fs.readFileSync(path.join(frontendRoot, 'api', 'recording-fallback-controls.js'), 'utf8');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');
const frontendHttpPaths = fs.readFileSync(path.join(repositoryRoot, 'core', 'http', 'src', 'TestHttpServerPaths.inc'), 'utf8');
const httpServerSource = fs.readFileSync(path.join(repositoryRoot, 'core', 'http', 'src', 'TestHttpServer.cpp'), 'utf8');
const securityRequestSource = fs.readFileSync(path.join(repositoryRoot, 'core', 'security', 'include', 'ContinueWatchingSecurityRequest.h'), 'utf8');
const continueApiSource = fs.readFileSync(path.join(repositoryRoot, 'api', 'rest', 'src', 'ContinueWatchingApiRuntime.cpp'), 'utf8');
const continueHeaderSource = fs.readFileSync(path.join(repositoryRoot, 'core', 'media', 'include', 'ContinueWatching.h'), 'utf8');
const continueServiceSource = fs.readFileSync(path.join(repositoryRoot, 'core', 'media', 'src', 'ContinueWatching.cpp'), 'utf8');

// Architectural fences: server/actor truth, canonical existing playback owner,
// preview release before ownership transfer, and no browser-local cross-client truth.
assert(source.includes('/api/media/continue-watching'));
assert(syncSource.includes('/api/media/continue-watching'));
assert(!source.includes('localStorage'));
assert(!syncSource.includes('localStorage'));
assert(source.includes('VdrSuiteRecordings2.openRecording'));
assert(source.includes('releasePreview()'));
assert(source.includes("typeof preview.cancel === 'function'"));
assert(source.includes('clearBeforeRestart(normalized)'));
assert(syncSource.includes('clear: clearCurrent'));
assert(!source.includes('__test.cancelPreview'));
assert(homePreviewSource.includes('cancel: cancelPreview'));
assert(!source.includes('MediaSession'));
assert(!source.includes("document.createElement('video')"));
assert(!syncSource.includes("document.createElement('video')"));
assert(syncSource.includes('startAtAbsolute'));
assert(!syncSource.includes('owner.resume(target)'));
assert(syncSource.includes("typeof owner.snapshot !== 'function' || typeof owner.subscribe !== 'function'"));
assert(syncSource.includes('unsubscribeLifecycle = owner.subscribe(lifecycleChanged)'));
assert(!syncSource.includes('decorated.stop = function'));
assert(!syncSource.includes('decorated.destroy = function'));
assert(!syncSource.includes('decorated.relinquishForReplacement = function'));
assert(syncSource.includes("return typeof owner.canResume === 'function' && owner.canResume() === true;"));
assert(syncSource.includes('resumeSupported: true'));
assert(!source.includes('playbackCapable'));
assert(!continueApiSource.includes('playbackCapable'));
assert(!continueHeaderSource.includes('playbackCapable'));
assert(!continueServiceSource.includes('playbackCapable'));
assert(fallbackSource.includes('startAtAbsolute: startAt'));
assert(indexSource.includes('data-home-zone="additional-sections"'));
assert(source.includes('VdrSuiteBrowserSession'));
assert(syncSource.includes('VdrSuiteBrowserSession'));
assert(frontendHttpPaths.includes(
    '{"/frontend/recordings2-playback.js", "recordings2-playback.js", "application/javascript; charset=utf-8", "api/continue-watching-sync.js"}'
));
assert(frontendHttpPaths.includes(
    '{"/frontend/channel-day-program.js", "channel-day-program.js", "application/javascript; charset=utf-8", "api/session-frontend-sync.js"}'
));
assert(indexSource.indexOf('../frontend/channel-day-program.js') < indexSource.indexOf('../frontend/app.js'));
assert(httpServerSource.includes('ContinueWatchingSecurityRequest::forAuthorization'));
assert(securityRequestSource.includes('/api/media/continue-watching'));
assert(securityRequestSource.includes('scoped.path = "/api/media/sessions"'));

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
    durationSeconds: 600
};
assert.strictEqual(api._test.normalizeItem(base).recordingId, 'r1');
assert.strictEqual(api._test.normalizeItem({...base, resumePositionSeconds: 0}), null);
assert.strictEqual(api._test.normalizeItem({...base, resumePositionSeconds: 600}), null);

const known = api._test.progressModel(base);
assert.strictEqual(known.percent, 20);
assert.strictEqual(known.hasPercent, true);
const unknown = api._test.progressModel({...base, durationKnown: false, durationSeconds: 0});
assert.strictEqual(unknown.hasPercent, false);
assert.strictEqual(unknown.percent, null);

// Continue and From beginning both route through the same Recordings2 playback owner.
const opened = [];
context.window.VdrSuiteRecordings2 = {
    openRecording(recording, options) { opened.push({recording, options}); }
};

const syncRequests = [];
const scheduled = [];
let assignedPlayback = {};
function setTimeoutFake(callback, delay) {
    scheduled.push({callback, delay, active: true});
    return scheduled.length;
}
function clearTimeoutFake(id) {
    if (scheduled[id - 1]) scheduled[id - 1].active = false;
}
const syncContext = {
    window: {},
    fetch: async (requestPath, options) => {
        syncRequests.push({path: requestPath, options});
        return {ok: true};
    },
    console,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake,
    MutationObserver: class {
        observe() {}
        disconnect() {}
    }
};
syncContext.window.window = syncContext.window;
syncContext.window.fetch = syncContext.fetch;
syncContext.window.setTimeout = setTimeoutFake;
syncContext.window.clearTimeout = clearTimeoutFake;
syncContext.window.MutationObserver = syncContext.MutationObserver;
syncContext.window.VdrSuiteBrowserSession = {
    csrfHeaders() { return {'X-VDR-Suite-CSRF': 'phase66-csrf-token'}; }
};
Object.defineProperty(syncContext.window, 'VdrSuiteRecordings2Playback', {
    configurable: true,
    enumerable: true,
    get() { return assignedPlayback; },
    set(value) { assignedPlayback = value; }
});
vm.createContext(syncContext);
vm.runInContext(syncSource, syncContext);
const syncApi = syncContext.window.VdrSuiteContinueWatchingSync;
assert(syncApi && syncApi.__test);
assert.strictEqual(typeof syncApi.clear, 'function');

function flush(count = 8) {
    let promise = Promise.resolve();
    for (let index = 0; index < count; index += 1) promise = promise.then(() => Promise.resolve());
    return promise;
}

(async function () {
    assert.strictEqual(await api._test.openItem(base, true), true);
    assert.strictEqual(await api._test.openItem(base, false), true);
    assert.strictEqual(opened.length, 2);
    assert.strictEqual(opened[0].recording.id, 'r1');
    assert.strictEqual(opened[0].options.playbackStartPositionSeconds, 120);
    assert.strictEqual(opened[0].options.autoStartPlayback, true);
    assert.strictEqual(opened[1].options.playbackStartPositionSeconds, 0);
    assert.strictEqual(opened[1].options.autoStartPlayback, true);

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

    await syncApi.__test.post({
        operation: 'progress',
        backendId: 'default',
        recordingId: 'r1',
        positionSeconds: 120,
        resumeSupported: true,
        operationId: 'phase66-progress-test'
    });
    assert.strictEqual(syncRequests.length, 1);
    assert.strictEqual(syncRequests[0].path, '/api/media/continue-watching');
    assert.strictEqual(syncRequests[0].options.credentials, 'same-origin');
    assert.strictEqual(syncRequests[0].options.headers['Content-Type'], 'application/json');
    assert.strictEqual(syncRequests[0].options.headers['X-VDR-Suite-CSRF'], 'phase66-csrf-token');

    // Same-client mutations are serialized, so an older in-flight progress write
    // cannot arrive after the public restart clear and recreate stale truth.
    const serialized = [];
    const serializedBodies = [];
    let releaseFirst;
    syncContext.window.fetch = function (requestPath, options) {
        const body = JSON.parse(options.body);
        serializedBodies.push(body);
        serialized.push(body.operation + ':' + (body.positionSeconds || 0));
        if (serialized.length === 1) {
            return new Promise(resolve => { releaseFirst = () => resolve({ok: true}); });
        }
        return Promise.resolve({ok: true});
    };
    const firstMutation = syncApi.__test.enqueue({operation: 'progress', backendId: 'default', recordingId: 'r1', positionSeconds: 30, resumeSupported: true, operationId: 'queue-1'});
    const secondMutation = syncApi.clear('default', 'r1');
    await flush();
    assert.deepStrictEqual(serialized, ['progress:30']);
    releaseFirst();
    await firstMutation;
    await secondMutation;
    assert.deepStrictEqual(serialized, ['progress:30', 'clear:0']);
    assert.strictEqual(serializedBodies[1].backendId, 'default');
    assert.strictEqual(serializedBodies[1].recordingId, 'r1');
    assert.match(serializedBodies[1].operationId, /^cw-clear-/);

    // Lifecycle truth comes from the canonical snapshot/subscribe publication.
    // The production-style internal action below never calls a decorated start/stop method.
    const lifecycleListeners = [];
    let ownerSnapshot = {state: 'idle', sessionId: null, transition: 'snapshot'};
    let absolutePosition = 0;
    let startCalls = 0;
    const owner = {
        element: {querySelectorAll() { return []; }},
        start() { startCalls += 1; return Promise.resolve('unexpected'); },
        position() { return absolutePosition; },
        duration() { return 600; },
        canResume() { return true; },
        snapshot() { return ownerSnapshot; },
        subscribe(callback) {
            lifecycleListeners.push(callback);
            callback(ownerSnapshot);
            return function () {
                const index = lifecycleListeners.indexOf(callback);
                if (index >= 0) lifecycleListeners.splice(index, 1);
            };
        }
    };
    function publish(change) {
        ownerSnapshot = Object.assign({}, ownerSnapshot, change);
        lifecycleListeners.slice().forEach(listener => listener(ownerSnapshot));
    }
    syncContext.window.fetch = async function (requestPath, options) {
        syncRequests.push({path: requestPath, options});
        return {ok: true};
    };
    const decoratedOwner = syncApi.__test.decorateOwner(owner, {id: 'r-lifecycle'}, 'default');
    assert.notStrictEqual(decoratedOwner, owner);
    assert.strictEqual(decoratedOwner.start, owner.start, 'Continue Watching must not intercept start() as lifecycle truth');
    assert.strictEqual(lifecycleListeners.length, 1);
    const beforeLifecycleRequests = syncRequests.length;
    absolutePosition = 47;
    publish({state: 'playing', sessionId: 'session-1', transition: 'session-started'});
    assert.strictEqual(startCalls, 0, 'owner publication must not synthesize playback commands');
    publish({state: 'stopped', sessionId: null, transition: 'stopped'});
    await flush(16);
    assert.strictEqual(syncRequests.length, beforeLifecycleRequests + 1);
    const lifecycleBody = JSON.parse(syncRequests[syncRequests.length - 1].options.body);
    assert.strictEqual(lifecycleBody.operation, 'progress');
    assert.strictEqual(lifecycleBody.recordingId, 'r-lifecycle');
    assert.strictEqual(lifecycleBody.positionSeconds, 47);

    publish({state: 'destroyed', sessionId: null, transition: 'destroyed'});
    assert.strictEqual(lifecycleListeners.length, 0, 'destroyed lifecycle publication must release observation');

    const homeRequests = [];
    context.window.VdrSuiteBrowserSession = {
        csrfHeaders() { return {'X-VDR-Suite-CSRF': 'phase66-home-csrf'}; }
    };
    context.window.fetch = async function (requestPath, options) {
        homeRequests.push({path: requestPath, options});
        return {ok: true, json: async () => ({items: []})};
    };
    await api._test.post({operation: 'list', backendId: 'default'});
    assert.strictEqual(homeRequests.length, 1);
    assert.strictEqual(homeRequests[0].path, '/api/media/continue-watching');
    assert.strictEqual(homeRequests[0].options.credentials, 'same-origin');
    assert.strictEqual(homeRequests[0].options.headers['Content-Type'], 'application/json');
    assert.strictEqual(homeRequests[0].options.headers['X-VDR-Suite-CSRF'], 'phase66-home-csrf');

    console.log('phase66 continue watching contract ok');
}()).catch(function (error) {
    console.error(error);
    process.exitCode = 1;
});
