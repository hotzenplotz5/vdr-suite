'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const frontendRoot = path.join(__dirname, '..');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');
const appSource = fs.readFileSync(path.join(frontendRoot, 'app.js'), 'utf8');

function occurrences(source, needle) {
  return source.split(needle).length - 1;
}

// Slice 1 must extend the installed browser composition root, not create a
// parallel Home application or a second navigation/state owner.
assert(indexSource.includes('<body class="media-app-shell">'));
assert.strictEqual(occurrences(indexSource, 'id="module-nav"'), 1);
assert.strictEqual(occurrences(indexSource, 'id="detail-data"'), 1);
assert(indexSource.includes('data-brand-module="overview"'));
assert(indexSource.includes('data-brand-module="channels2"'));
assert(indexSource.includes('data-brand-module="recordings2"'));
assert(indexSource.includes('data-brand-module="search"'));
assert(indexSource.includes('data-brand-module="settings"'));
assert(indexSource.includes('class="module-tab active" data-module="overview"'));

// The semantic Home zones are static composition only. Later Phase-66 media
// semantics must not be fabricated in Slice 1.
assert(indexSource.includes('data-home-zone="hero"'));
assert(indexSource.includes('data-home-zone="primary-rail"'));
assert(indexSource.includes('data-home-zone="additional-sections"'));
assert(indexSource.includes('class="media-home-only media-home-hero"'));
assert(indexSource.includes('class="media-home-only media-home-additional-sections"'));
assert(!indexSource.includes('<video'));
assert(!indexSource.includes('<audio'));
assert(!indexSource.includes('continue-watching'));
assert(!indexSource.includes('MediaSession'));
assert(!indexSource.includes('startPositionSeconds'));

// Production app.js remains the sole view/navigation state owner. A user
// action on either the existing module tab or the shell navigation therefore
// reaches selectModule(), toggles the real active module DOM state and renders
// the existing real backend snapshot into the same mount.
assert(appSource.includes("let selectedModule = 'overview';"));
assert(appSource.includes('function selectModule(moduleName)'));
assert(appSource.includes("document.querySelectorAll('.module-tab').forEach(button =>"));
assert(appSource.includes("button.addEventListener('click', () => selectModule(button.dataset.module));"));
assert(appSource.includes("document.querySelectorAll('[data-brand-module]').forEach(button =>"));
assert(appSource.includes('selectModule(button.dataset.brandModule);'));
assert(appSource.includes("button.classList.toggle('active', button.dataset.module === moduleName);"));
assert(appSource.includes("if (selectedModule === 'overview')"));
assert(appSource.includes('renderSnapshotMetrics(data);'));
assert(appSource.includes("createMetric('Kanäle', valueOrZero(data.channelCount))"));
assert(appSource.includes("createMetric('Aufnahmen', valueOrZero(data.recordingCount))"));

// Home composition is driven only by that existing UI state. The primary
// content shell reuses real snapshot data as a horizontal rail rather than
// inventing Continue Watching, preview or demo media data.
assert(indexSource.includes('#detail:has(.module-tab.active[data-module="overview"])'));
assert(indexSource.includes('#detail:has(.module-tab.active[data-module="overview"]) #detail-data'));
assert(indexSource.includes('overflow-x: auto;'));
assert(indexSource.includes('scroll-snap-type: x proximity;'));

// Responsive recomposition: distinct desktop/tablet/mobile/landscape geometry,
// touch-sized controls, no page-wide horizontal scrolling, and reduced motion.
assert(indexSource.includes('@media (min-width: 72.01rem)'));
assert(indexSource.includes('@media (min-width: 46.01rem) and (max-width: 72rem)'));
assert(indexSource.includes('@media (max-width: 46rem)'));
assert(indexSource.includes('@media (max-height: 34rem) and (min-width: 40rem) and (max-width: 64rem)'));
assert(indexSource.includes('@media (prefers-reduced-motion: reduce)'));
assert(indexSource.includes('overflow-x: hidden;'));
assert(indexSource.includes('min-height: 2.75rem;'));
assert(indexSource.includes('min-width: min(78vw, 19rem);'));

console.log('phase66 home shell production composition ok');
