'use strict';

const assert = require('assert');
const fs = require('fs');

const source = fs.readFileSync('web/frontend/recordings2-hero-detail.js', 'utf8');
const metadataDetail = fs.readFileSync('web/frontend/recordings2-metadata-detail.js', 'utf8');
const packaging = fs.readFileSync('mk/recordings2.mk', 'utf8');

assert(source.includes('global.VdrSuiteRecordings2HeroDetail = Object.freeze'),
  'hero detail runtime must export a stable owner');
assert(source.includes('position:fixed;inset:0;z-index:1200'),
  'hero detail must render as its own full-page surface');
assert(source.includes("root.dataset.recordings2HeroMode = 'detail'"),
  'recording detail must open in hero detail mode first');
assert(source.includes('[data-recordings2-hero-mode="detail"]>.recordings2-playback'),
  'playback must be hidden while the primary detail page is active');
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
assert(metadataDetail.includes('heroDetail.enhance(root, recording, backendId, presented)'),
  'metadata enhancement must activate the hero detail after canonical metadata load');
assert(packaging.includes('web/frontend/recordings2-hero-detail.js'),
  'hero detail runtime must be bundled into the installed Recordings 2 browser runtime');
assert(packaging.includes('global.VdrSuiteRecordings2HeroDetail = Object.freeze'),
  'install staging must verify the bundled hero owner');

console.log('recordings2 hero-first detail contract ok');
