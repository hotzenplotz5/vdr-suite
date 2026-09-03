'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');
const heroSource = fs.readFileSync(path.join(frontendRoot, 'home-live-hero.js'), 'utf8');
const continueSource = fs.readFileSync(path.join(frontendRoot, 'home-continue-watching.js'), 'utf8');
const discoverySource = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8');
const historySource = fs.readFileSync(path.join(frontendRoot, 'home-recently-watched.js'), 'utf8');
const epgMetadataSource = fs.readFileSync(path.join(frontendRoot, 'epg-metadata-detail.js'), 'utf8');
const liveTvSource = fs.readFileSync(path.join(frontendRoot, 'live-tv-view.js'), 'utf8');
const liveTvCompatSource = fs.readFileSync(path.join(frontendRoot, 'channel-day-program-compat.js'), 'utf8');

// Production composition remains the existing responsive Media Home shell.
assert(indexSource.includes('data-home-zone="hero"'));
assert(indexSource.includes('data-home-zone="primary-rail"'));
assert(indexSource.includes('data-home-zone="additional-sections"'));
assert(indexSource.includes('aria-labelledby="media-home-title"'));
assert(indexSource.includes('aria-label="Weitere Media-Home-Bereiche"'));
assert(indexSource.includes('@media (min-width: 72.01rem)'));
assert(indexSource.includes('@media (min-width: 46.01rem) and (max-width: 72rem)'));
assert(indexSource.includes('@media (max-width: 46rem)'));
assert(indexSource.includes('@media (max-height: 34rem) and (min-width: 40rem) and (max-width: 64rem)'));
assert(indexSource.includes('@media (prefers-reduced-motion: reduce)'));

// Hero: keyboard focus must be unmistakable, touch controls practical, live
// progress semantic, and JS scrolling must respect reduced motion.
assert(heroSource.includes("root.setAttribute('role', 'region')"));
assert(heroSource.includes("root.setAttribute('aria-label', 'Live-TV Senderkarussell')"));
assert(heroSource.includes("doc.createElement('progress')"));
assert(heroSource.includes('Fortschritt der laufenden Sendung: '));
assert(heroSource.includes('.media-home-live-neighbor:focus-visible{'));
assert(heroSource.includes('outline:3px solid rgba(125,211,252,.96)'));
assert(!heroSource.includes('.media-home-live-neighbor:hover,.media-home-live-neighbor:focus-visible{opacity:1;border-color:rgba(125,211,252,.46);outline:none'));
assert(heroSource.includes('min-width:2.75rem;min-height:2.75rem'));
assert(heroSource.includes('@media(min-width:120rem)'));
assert(heroSource.includes('@media(prefers-reduced-motion:reduce)'));
assert(heroSource.includes("behavior: prefersReducedMotion() ? 'auto' : 'smooth'"));

// EPG metadata details use the same reduced-motion contract for automatic
// reveal scrolling instead of forcing animated movement after tab/cast actions.
assert(epgMetadataSource.includes("global.matchMedia('(prefers-reduced-motion: reduce)').matches === true"));
assert(epgMetadataSource.includes("behavior: prefersReducedMotion() ? 'auto' : 'smooth'"));
assert(!epgMetadataSource.includes("scrollIntoView({behavior: 'smooth', block: 'start'})"));

// The dedicated Live-TV product runtime remains the primary installed owner.
// Its compat implementation stays a guarded fallback and must not receive a
// duplicate accessibility/runtime implementation.
const liveTvPosition = indexSource.indexOf('<script src="../frontend/live-tv-view.js"></script>');
const liveTvCompatPosition = indexSource.indexOf('<script src="../frontend/channel-day-program-compat.js"></script>');
assert(liveTvPosition >= 0);
assert(liveTvCompatPosition > liveTvPosition);
assert(liveTvCompatSource.includes('if (global.VdrSuiteLiveTvView) return;'));
assert(liveTvSource.includes("function playbackShell() { return global.VdrSuitePlaybackShell || null; }"));
assert(liveTvSource.includes("function playbackApi() { return global.VdrSuiteRecordings2Playback || null; }"));
assert(liveTvSource.includes('global.VdrSuiteLiveTvView = api;'));
assert(!liveTvSource.includes('/api/media/sessions'));

// Primary Live-TV reduced-motion treatment is local to the owning view. The
// normal product behavior stays smooth, while the user preference disables the
// two automatic scroll animations and the preview/tile movement.
assert(liveTvSource.includes("global.matchMedia('(prefers-reduced-motion: reduce)').matches === true"));
assert(liveTvSource.includes("player.scrollIntoView({behavior: prefersReducedMotion() ? 'auto' : 'smooth', block: 'nearest'});"));
assert(liveTvSource.includes("mount.scrollIntoView({behavior: prefersReducedMotion() ? 'auto' : 'smooth', block: 'start'});"));
assert(!liveTvSource.includes("player.scrollIntoView({behavior: 'smooth', block: 'nearest'});"));
assert(!liveTvSource.includes("mount.scrollIntoView({behavior: 'smooth', block: 'start'});"));
assert(liveTvSource.includes('@media(prefers-reduced-motion:reduce){'));
assert(liveTvSource.includes('.vdr-suite-live-tv-channel:hover,.vdr-suite-live-tv-channel:focus-visible{transform:none}'));
assert(liveTvSource.includes('.vdr-suite-live-tv-preview{transition:none;transform:none}'));

// Accessibility changes must not alter canonical Live-TV replacement,
// relinquish, destroy/stop or persistent-shell lifecycle semantics.
assert(liveTvSource.includes("playback.createLivePanel(channel, state.backendId || selectedBackend(), {replacesSessionId: replacesSessionId || ''})"));
assert(liveTvSource.includes('previous.relinquishForReplacement()'));
assert(liveTvSource.includes("if (typeof previous.destroy === 'function') previous.destroy();"));
assert(liveTvSource.includes('state.switchSequence += 1;'));
assert(liveTvSource.includes("if (current && typeof current.destroy === 'function') current.destroy();"));
assert(liveTvSource.includes("if (shell && typeof shell.stop === 'function') shell.stop();"));
assert(liveTvSource.includes("typeof shell.detach === 'function') shell.detach(state.playback)"));

// Exercise the real primary runtime helper and module-open path with the same
// matchMedia preference the browser exposes. This proves normal=smooth and
// reduced=auto for both productive scrollIntoView sites, not only source text.
let reducedMotion = false;
const playerScrollCalls = [];
const mountScrollCalls = [];
const livePlayer = {
  scrollIntoView(options) { playerScrollCalls.push(options || {}); }
};
const liveMount = {
  classList: {remove() {}},
  replaceChildren() {},
  appendChild() {},
  contains() { return false; },
  querySelector(selector) { return selector === '.vdr-suite-live-tv-player' ? livePlayer : null; },
  scrollIntoView(options) { mountScrollCalls.push(options || {}); }
};
function liveTestNode() {
  return {
    id: '',
    className: '',
    dataset: {},
    style: {},
    textContent: '',
    hidden: false,
    parentNode: null,
    appendChild() {},
    setAttribute() {},
    addEventListener() {},
    classList: {add() {}, remove() {}}
  };
}
const liveDocument = {
  readyState: 'complete',
  head: {appendChild() {}},
  createElement() { return liveTestNode(); },
  getElementById(id) { return id === 'detail-data' ? liveMount : null; },
  querySelector() { return null; },
  querySelectorAll() { return []; },
  addEventListener() {}
};
const liveWindow = {
  document: liveDocument,
  matchMedia(query) {
    assert.strictEqual(query, '(prefers-reduced-motion: reduce)');
    return {matches: reducedMotion};
  }
};
vm.runInNewContext(liveTvSource, {
  window: liveWindow,
  document: liveDocument,
  Object,
  String,
  Number,
  Array,
  Boolean,
  Promise,
  RegExp,
  Error,
  Date,
  Math
}, {filename: 'live-tv-view.js'});

const liveTvRuntimeTest = liveWindow.VdrSuiteLiveTvView.__test;
assert.strictEqual(liveTvRuntimeTest.prefersReducedMotion(), false);
assert.strictEqual(liveTvRuntimeTest.scrollPlayerIntoView(), true);
assert.strictEqual(playerScrollCalls.length, 1);
assert.strictEqual(playerScrollCalls[0].behavior, 'smooth');
assert.strictEqual(playerScrollCalls[0].block, 'nearest');
liveWindow.VdrSuiteLiveTvView.open();
assert.strictEqual(mountScrollCalls.length, 1);
assert.strictEqual(mountScrollCalls[0].behavior, 'smooth');
assert.strictEqual(mountScrollCalls[0].block, 'start');

reducedMotion = true;
assert.strictEqual(liveTvRuntimeTest.prefersReducedMotion(), true);
assert.strictEqual(liveTvRuntimeTest.scrollPlayerIntoView(), true);
assert.strictEqual(playerScrollCalls.length, 2);
assert.strictEqual(playerScrollCalls[1].behavior, 'auto');
assert.strictEqual(playerScrollCalls[1].block, 'nearest');
liveWindow.VdrSuiteLiveTvView.open();
assert.strictEqual(mountScrollCalls.length, 2);
assert.strictEqual(mountScrollCalls[1].behavior, 'auto');
assert.strictEqual(mountScrollCalls[1].block, 'start');

// Live programme rails reuse the Hero-owned channel/EPG projection. They sit
// immediately above Continue Watching and use the same compact Home portrait
// scale without introducing another endpoint or playback owner.
assert(heroSource.includes('const PROGRAMME_RAIL_LIMIT = 24;'));
assert(heroSource.includes("renderProgrammeRail('now', 'Was läuft jetzt', true);"));
assert(heroSource.includes("renderProgrammeRail('next', 'Was läuft danach', false);"));
assert(heroSource.includes('.media-home-live-guide-now{order:10}.media-home-live-guide-next{order:20}.media-home-continue-watching{order:30}'));
assert(heroSource.includes('.media-home-live-guide-rail{display:grid;grid-auto-flow:column;grid-auto-columns:minmax(11rem,15rem)'));
assert(heroSource.includes('.media-home-live-guide-artwork{display:grid;place-items:center;width:100%;aspect-ratio:2/3'));
assert(heroSource.includes("image.loading = 'lazy'"));
assert(heroSource.includes("image.decoding = 'async'"));
assert(heroSource.includes("image.fetchPriority = 'low'"));
assert(!heroSource.includes('/api/media/sessions'));

// Continue Watching: preserve the canonical playback owner while improving
// semantics, focus, touch sizing, fallback art and below-fold image behavior.
assert(continueSource.includes("section.setAttribute('aria-labelledby', 'media-home-continue-title')"));
assert(continueSource.includes("rail.setAttribute('aria-label', 'Fortsetzbare Aufnahmen')"));
assert(continueSource.includes("resume.setAttribute('aria-label', item.title + ' fortsetzen')"));
assert(continueSource.includes("restart.setAttribute('aria-label', item.title + ' von vorn abspielen')"));
assert(continueSource.includes("image.loading = 'lazy'"));
assert(continueSource.includes("image.decoding = 'async'"));
assert(continueSource.includes("image.fetchPriority = 'low'"));
assert(continueSource.includes('.media-home-continue-artwork.is-fallback'));
assert(continueSource.includes('.media-home-continue-actions button:focus-visible'));
assert(continueSource.includes('min-width:2.75rem;min-height:2.75rem'));
assert(continueSource.includes('@media(min-width:120rem)'));
assert(continueSource.includes('@media(prefers-reduced-motion:reduce)'));
assert(continueSource.includes('const generation = ++refreshGeneration'));
assert(continueSource.includes('VdrSuiteRecordings2.openRecording'));
assert(!continueSource.includes('localStorage'));
assert(!continueSource.includes('MediaSession'));

// Desktop Home polish: all Recording cover rails share the established
// Recently-Watched / discovery portrait width while mobile keeps its existing
// wider touch card. The inventory rail is visually ordered after every dynamic
// Media-Home section without changing backend/data ownership.
assert(discoverySource.includes('grid-auto-columns:minmax(11rem,15rem)'));
assert(continueSource.includes('@media(min-width:46.01rem){.media-home-continue-rail{grid-auto-columns:minmax(11rem,15rem)'));
assert(continueSource.includes('.media-home-continue-card{grid-template-columns:1fr;gap:0;padding:0;overflow:hidden}'));
assert(continueSource.includes('.media-home-continue-artwork{border-radius:0}'));
assert(continueSource.includes('@media(max-width:46rem){.media-home-continue-rail{grid-auto-columns:minmax(80vw,20rem)}.media-home-continue-card{grid-template-columns:5rem 1fr}}'));
assert(continueSource.includes('#detail:has(.module-tab.active[data-module="overview"]){display:flex;flex-direction:column}'));
assert(continueSource.includes('[data-home-zone="additional-sections"]{order:40}'));
assert(continueSource.includes('[data-home-zone="primary-rail"]{order:50}'));
assert(continueSource.includes('#detail-data{order:60}'));

// Existing discovery / series / history rails keep their established focus,
// geometry, lazy-loading and canonical owner boundaries. Slice 66.7 must not
// change Series membership or introduce a second metadata/recording identity.
assert(discoverySource.includes('.media-home-discovery-card:focus-visible'));
assert(discoverySource.includes('aspect-ratio:2/3'));
assert(discoverySource.includes("image.loading = 'lazy'"));
assert(discoverySource.includes('VdrSuiteRecordings2.openRecording'));
assert(!discoverySource.includes('localStorage'));
assert(historySource.includes("image.loading = 'lazy'"));
assert(historySource.includes('VdrSuiteRecordings2.openRecording'));
assert(!historySource.includes('localStorage'));

console.log('phase66 visual polish and accessibility production contracts ok');
