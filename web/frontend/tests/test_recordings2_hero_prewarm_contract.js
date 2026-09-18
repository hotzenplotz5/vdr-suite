'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const prewarm = fs.readFileSync('web/frontend/recordings2-hero-prewarm.js', 'utf8');
const continueSync = fs.readFileSync('web/frontend/api/continue-watching-sync.js', 'utf8');
const packaging = fs.readFileSync('mk/recordings2.mk', 'utf8');
const legacyPlayback = fs.readFileSync('web/frontend/recordings2-playback.js', 'utf8');
const fastPlayback = fs.readFileSync('web/frontend/api/session-frontend-sync.js', 'utf8');

assert(prewarm.includes('const PREWARM_DELAY_MS = 700;'),
  'Hero prewarm must remain deliberately delayed and bounded');
assert(prewarm.includes('playbackOwner.start({autoPlay: false})'),
  'Hero prewarm must prepare the canonical owner without autoplay');
assert(prewarm.includes("root.dataset.recordings2HeroMode !== 'detail'"),
  'Hero prewarm must not start after navigation away from detail mode');
assert(prewarm.includes("const PREWARM_PROMISE = '__vdrSuiteHeroPlaybackPrewarmPromise';"),
  'Hero click path must share the same prewarm promise instead of creating a second session');
assert(prewarm.includes("typeof playbackOwner.play === 'function'"),
  'prepared playback must be promoted through the canonical owner play action');
assert(prewarm.includes("owner.__test.showMode(root, 'playback', '.recordings2-volume-owner-shell')"),
  'prepared playback must still enter the established playback surface');
assert(!prewarm.includes("document.createElement('video')"),
  'Hero prewarm must never create a second media element');
assert(!prewarm.includes("'/api/media/sessions'"),
  'Hero prewarm must never create a parallel MediaSession directly');
assert(continueSync.includes("state !== 'starting'"),
  'Continue Watching must ignore background session preparation until real playback begins');
assert(packaging.includes('web/frontend/recordings2-hero-prewarm.js'),
  'prewarm runtime must be included in the Recordings 2 browser bundle');
assert(packaging.includes('node --check web/frontend/recordings2-hero-prewarm.js'),
  'prewarm runtime must be syntax checked');
assert(legacyPlayback.includes('function start(startOptions)'),
  'compatibility Recording owner must accept bounded start options');
assert(legacyPlayback.includes('const shouldAutoPlay = settings.autoPlay !== false;'),
  'compatibility Recording owner must preserve explicit no-autoplay intent');
assert(legacyPlayback.includes('const playRequest = shouldAutoPlay ? video.play() : null;'),
  'compatibility Recording prewarm must not call video.play');
assert(legacyPlayback.includes('function play()'),
  'prepared compatibility owner must expose explicit play promotion');
assert(fastPlayback.includes('legacy.start({autoPlay: autoPlay !== false})'),
  'fast owner must propagate prewarm autoplay intent into compatibility fallback');
assert(fastPlayback.includes("typeof fallbackPanel.play === 'function'"),
  'explicit play must delegate to a prepared compatibility fallback');

const scheduled = [];
const requests = [];
let assignedPlayback = {};
const context = {
  window: {},
  console,
  fetch: async (requestPath, options) => {
    requests.push({requestPath, options});
    return {ok: true};
  },
  setTimeout(callback, delay) {
    scheduled.push({callback, delay});
    return scheduled.length;
  },
  clearTimeout() {},
  MutationObserver: class {
    observe() {}
    disconnect() {}
  }
};
context.window.window = context.window;
context.window.fetch = context.fetch;
context.window.setTimeout = context.setTimeout;
context.window.clearTimeout = context.clearTimeout;
context.window.MutationObserver = context.MutationObserver;
context.window.VdrSuiteBrowserSession = {csrfHeaders() { return {}; }};
Object.defineProperty(context.window, 'VdrSuiteRecordings2Playback', {
  configurable: true,
  enumerable: true,
  get() { return assignedPlayback; },
  set(value) { assignedPlayback = value; }
});
vm.createContext(context);
vm.runInContext(continueSync, context, {filename: 'continue-watching-sync.js'});

const sync = context.window.VdrSuiteContinueWatchingSync;
assert(sync && sync.__test, 'Continue Watching test surface must be available');
const listeners = [];
let snapshot = {state: 'idle', sessionId: null, transition: 'snapshot'};
const playbackOwner = {
  element: {querySelectorAll() { return []; }},
  snapshot() { return snapshot; },
  subscribe(callback) {
    listeners.push(callback);
    callback(snapshot);
    return function () {};
  },
  position() { return 0; },
  duration() { return 600; },
  canResume() { return true; }
};
sync.__test.decorateOwner(playbackOwner, {id: 'prewarm-recording'}, 'default');
function publish(change) {
  snapshot = Object.assign({}, snapshot, change);
  listeners.slice().forEach(listener => listener(snapshot));
}

publish({state: 'starting', sessionId: 'prewarm-session', transition: 'session-started'});
assert.strictEqual(scheduled.length, 0,
  'background prewarm must not schedule recently-watched/progress activity');
assert.strictEqual(requests.length, 0,
  'background prewarm must not write Continue Watching or history truth');

publish({state: 'playing', sessionId: 'prewarm-session', transition: 'play'});
assert.ok(scheduled.length >= 2,
  'real playback must activate the established history/progress sampling');

console.log('recordings2 hero playback prewarm contract ok');
