'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const frontendRoot = path.join(__dirname, '..');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');
const heroSource = fs.readFileSync(path.join(frontendRoot, 'home-live-hero.js'), 'utf8');
const continueSource = fs.readFileSync(path.join(frontendRoot, 'home-continue-watching.js'), 'utf8');
const discoverySource = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8');
const historySource = fs.readFileSync(path.join(frontendRoot, 'home-recently-watched.js'), 'utf8');

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
