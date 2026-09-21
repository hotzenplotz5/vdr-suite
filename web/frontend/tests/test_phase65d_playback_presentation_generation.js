'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-owner-lifecycle.js'),
  'utf8'
);

const window = {};
window.window = window;
vm.runInContext(source, vm.createContext({window, Object, String, Set}));

const lifecycle = window.VdrSuitePlaybackOwnerLifecycle.create({
  state: 'idle',
  sessionId: null,
  transport: 'none'
});

let snapshot = lifecycle.snapshot();
assert.strictEqual(snapshot.lifecycleRevision, 0);
assert.strictEqual(snapshot.generation, undefined,
  'Slice-2 lifecycleRevision must remain distinct from Slice-3 presentation generation');
assert.strictEqual(snapshot.continuity.generation, 0);
assert.strictEqual(snapshot.continuity.state, 'idle');

snapshot = lifecycle.publish({
  transition: 'session-started',
  state: 'playing',
  sessionId: 'progressive-session-1',
  transport: 'progressive-fmp4'
});
assert.strictEqual(snapshot.lifecycleRevision, 1);
assert.strictEqual(snapshot.continuity.generation, 1);
assert.strictEqual(snapshot.continuity.state, 'stable');

const firstGeneration = snapshot.continuity.generation;
snapshot = lifecycle.publish({
  transition: 'play',
  state: 'playing',
  sessionId: 'progressive-session-1',
  transport: 'progressive-fmp4'
});
assert.strictEqual(snapshot.lifecycleRevision, 2);
assert.strictEqual(snapshot.continuity.generation, firstGeneration,
  'ordinary lifecycle publication must not be a decoder discontinuity');

// routeEpoch is a separate MediaRoute/fencing identity. Changing it cannot
// mutate browser playback continuity because it is not an input to this owner.
const route = {routeEpoch: 41};
route.routeEpoch = 42;
snapshot = lifecycle.publish({
  transition: 'pause',
  state: 'paused',
  sessionId: 'progressive-session-1',
  transport: 'progressive-fmp4'
});
assert.strictEqual(route.routeEpoch, 42);
assert.strictEqual(snapshot.continuity.generation, firstGeneration);
assert.strictEqual(snapshot.lifecycleRevision, 3);

snapshot = lifecycle.publish({
  transition: 'transport-replacing',
  state: 'replacing',
  sessionId: 'progressive-session-1',
  transport: 'progressive-fmp4'
});
assert.strictEqual(snapshot.continuity.generation, 1);
assert.strictEqual(snapshot.continuity.state, 'replacing');

snapshot = lifecycle.publish({
  transition: 'transport-replaced',
  state: 'starting',
  sessionId: null,
  transport: 'hls-compatibility'
});
assert.strictEqual(snapshot.continuity.generation, 2,
  'replacing an authoritative transport must create one new presentation generation');
assert.strictEqual(snapshot.continuity.state, 'replacing');

snapshot = lifecycle.publish({
  transition: 'session-replaced',
  state: 'playing',
  sessionId: 'hls-session-1',
  transport: 'hls-compatibility'
});
assert.strictEqual(snapshot.continuity.generation, 2,
  'replacement MediaSession must stabilize the transport generation without double counting');
assert.strictEqual(snapshot.continuity.state, 'stable');

snapshot = lifecycle.publish({
  transition: 'session-replaced',
  state: 'playing',
  sessionId: 'hls-session-2',
  transport: 'hls-compatibility'
});
assert.strictEqual(snapshot.continuity.generation, 3,
  'session replacement without a prior transport replacement is a new presentation');

snapshot = lifecycle.publish({
  transition: 'seek-started',
  state: 'seeking',
  sessionId: 'hls-session-2',
  transport: 'progressive-fmp4'
});
assert.strictEqual(snapshot.continuity.generation, 3);
assert.strictEqual(snapshot.continuity.state, 'replacing');

snapshot = lifecycle.publish({
  transition: 'seek-completed',
  state: 'playing',
  sessionId: 'hls-session-2',
  transport: 'progressive-fmp4'
});
assert.strictEqual(snapshot.continuity.generation, 4,
  'same-session reposition completion must identify the new decoder presentation');
assert.strictEqual(snapshot.continuity.state, 'stable');

// A fallback owner constructs its first transport before authorization. With no
// previous authoritative session that setup is continuity-neutral.
const freshHls = window.VdrSuitePlaybackOwnerLifecycle.create({
  state: 'idle',
  sessionId: null,
  transport: 'hls-compatibility'
});
freshHls.publish({
  transition: 'transport-replaced',
  state: 'starting',
  sessionId: null,
  transport: 'hls-compatibility'
});
assert.strictEqual(freshHls.snapshot().continuity.generation, 0);
freshHls.publish({
  transition: 'session-started',
  state: 'playing',
  sessionId: 'hls-first',
  transport: 'hls-compatibility'
});
assert.strictEqual(freshHls.snapshot().continuity.generation, 1);

console.log('phase65d playback presentation generation semantics ok');
