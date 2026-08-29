'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const frontend = path.resolve(__dirname, '..');
const heroSource = fs.readFileSync(path.join(frontend, 'home-live-hero.js'), 'utf8');
const shellSource = fs.readFileSync(path.join(frontend, 'channel-day-program-compat.js'), 'utf8');
const liveViewSource = fs.readFileSync(path.join(frontend, 'live-tv-view.js'), 'utf8');
const sessionSource = fs.readFileSync(path.join(frontend, 'api', 'session-frontend-sync.js'), 'utf8');

function requires(source, expression, message) {
  assert(expression.test(source), message);
}

// Slice 66.3 is intentionally browse-first. Focus changes must be fenced by a
// replaceable token and a bounded, private settle timer before any preview
// playback request is made.
requires(heroSource, /focusToken/, '66.3 rapid-focus race: Home preview must fence work with a focus token');
requires(heroSource, /previewSettleMs|PREVIEW_SETTLE_MS/, '66.3 settle race: Home preview must defer startup behind a bounded settle timer');
requires(heroSource, /cancel(?:Pending)?Preview|cancelPreview/, '66.3 pending race: superseded preview intent must be cancellable');
requires(heroSource, /schedulePreview|queuePreview/, '66.3 settled-focus path: Home must explicitly schedule deferred preview work');

// Home may use only the existing public frontend playback facade. It must not
// create a provider/native stream path or own MediaSession directly.
assert(!heroSource.includes('/api/media/sessions'), '66.3 architecture: Home must not call MediaSession REST directly');
assert(!heroSource.includes('createLiveSession('), '66.3 architecture: Home must not create MediaSession directly');
requires(heroSource, /VdrSuiteRecordings2Playback/, '66.3 production composition: Home preview must use the existing playback facade');
requires(heroSource, /ownerIntent\s*:\s*['"]preview['"]/, '66.3 ownership: preview intent must be explicit at the canonical playback boundary');

// The canonical shell remains the only lifecycle authority and must publish
// enough truth to distinguish preview from explicit full playback. A preview
// must never silently displace an already active full owner.
requires(shellSource, /ownerIntent/, '66.3 shell contract: canonical createLive path must receive owner intent');
requires(shellSource, /intent\s*:/, '66.3 shell contract: owner state/snapshot must expose preview vs full intent');
requires(shellSource, /preview/, '66.3 shell contract: canonical owner must contain explicit preview handling');
requires(shellSource, /full/, '66.3 shell contract: canonical owner must contain explicit full-playback handling');

// Live-TV is the explicit full-playback intent. Synchronization must not turn a
// Home preview into a full-playback view merely because the shell is active.
requires(liveViewSource, /ownerIntent\s*:\s*['"]full['"]/, '66.3 explicit playback: Watch Live must identify full ownership explicitly');
requires(liveViewSource, /snapshot\(\).*preview|intent.*preview/s, '66.3 synchronization: Live-TV view must distinguish an active Home preview');

// Proven stale in-flight race in the raw adapter: destruction can happen while
// POST /api/media/sessions is still pending. If the POST wins later, that newly
// created session must be explicitly stopped instead of being left unattached.
requires(sessionSource, /if\s*\(destroyed\)[\s\S]{0,500}stopLiveSession/, '66.3 in-flight race: a session created after panel destruction must be stopped');

console.log('phase66 deferred live preview ownership contract: PASS');
