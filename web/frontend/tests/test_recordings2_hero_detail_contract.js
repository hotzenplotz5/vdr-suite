'use strict';

const assert = require('assert');
const fs = require('fs');

const source = fs.readFileSync('web/frontend/recordings2-hero-detail.js', 'utf8');
const visibility = fs.readFileSync('web/frontend/recordings2-hero-visibility.js', 'utf8');
const metadataDetail = fs.readFileSync('web/frontend/recordings2-metadata-detail.js', 'utf8');
const packaging = fs.readFileSync('mk/recordings2.mk', 'utf8');

assert(source.includes('global.VdrSuiteRecordings2HeroDetail = Object.freeze'),
  'hero detail runtime must export a stable owner');
assert(source.includes('position:fixed;inset:0;z-index:1200'),
  'hero detail must render as its own full-page surface');
assert(source.includes("root.dataset.recordings2HeroMode = 'detail'"),
  'recording detail must open in hero detail mode first');
assert(source.includes("showMode(root, 'playback', '.recordings2-playback')"),
  'playback UI must only be revealed by the explicit play action');
assert(!source.includes('owner.start()'),
  'hero play action must not auto-start playback before showing the playback page');
assert(source.includes("showMode(root, 'marks', '.recordings2-marks-detail', 'Marke setzen')"),
  'marks action must reveal the existing marks/playback page on demand');
assert(source.includes("showMode(root, 'marks', '.recordings2-marks-detail', 'Schneiden')"),
  'cut action must reveal the existing marks/playback page on demand');
assert(source.includes("showMode(root, 'metadata', '.recordings2-metadata-tabs')"),
  'metadata action must reveal the existing metadata page on demand');
assert(source.includes("makeButton('← Details'"),
  'technical subpages must provide a return path to the hero detail page');
assert(source.includes("entry.orientation === 'landscape' || entry.orientation === 'banner'"),
  'hero backdrop must prefer landscape/banner metadata artwork');
assert(source.includes('metadata.preferredArtwork'),
  'hero backdrop must have an artwork fallback when no landscape image exists');
assert(source.includes("query: {name: actor.name, limit: 20}"),
  'related rail must reuse the existing local recording-person search');
assert(source.includes("'Weitere Filme mit ' + actor.name"),
  'related rail must be labelled by the selected main actor');
assert(source.includes('!sameRecording(candidate, recording)'),
  'current recording must be excluded from the related rail');
assert(source.includes('runtime.openRecording(recording'),
  'related cards must open the existing Recordings 2 detail runtime');
assert(!source.includes('similar') && !source.includes('Ähnliche Filme'),
  'v1 must not introduce a separate similarity/recommendation engine');

assert(visibility.includes("setHidden(root, '.recordings2-playback', !(playback || marks))"),
  'real DOM visibility wiring must hide playback in primary detail mode');
assert(visibility.includes("setHidden(root, '.recordings2-metadata-assignment', !metadata)"),
  'manual metadata assignment must stay hidden outside metadata mode');
assert(visibility.includes("setHidden(root, '.recordings2-detail-hero', !detail)"),
  'technical modes must hide the hero content explicitly');
assert(visibility.includes('new global.MutationObserver'),
  'visibility wiring must react when hero mode or async child content changes');
assert(visibility.includes('owner.enhance(root, recording, backendId, metadata)'),
  'visibility wiring must wrap, not replace, the existing hero owner behavior');

assert(metadataDetail.includes('heroDetail.enhance(root, recording, backendId, presented)'),
  'metadata enhancement must activate the hero detail after canonical metadata load');
assert(packaging.includes('web/frontend/recordings2-hero-detail.js'),
  'hero detail runtime must be bundled into the installed Recordings 2 browser runtime');
assert(packaging.includes('web/frontend/recordings2-hero-visibility.js'),
  'hero visibility runtime must be bundled immediately after the hero owner');
assert(packaging.includes('node --check web/frontend/recordings2-hero-visibility.js'),
  'hero visibility runtime must be syntax checked with Recordings 2 tests');

console.log('recordings2 hero-first detail contract ok');
