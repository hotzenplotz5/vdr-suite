'use strict';

const assert = require('assert');
const fs = require('fs');

const source = fs.readFileSync('web/frontend/recordings2-hero-detail.js', 'utf8');
const metadataDetail = fs.readFileSync('web/frontend/recordings2-metadata-detail.js', 'utf8');
const packaging = fs.readFileSync('mk/recordings2.mk', 'utf8');

assert(source.includes('global.VdrSuiteRecordings2HeroDetail = Object.freeze'),
  'hero detail runtime must export a stable owner');
assert(source.includes("entry.orientation === 'landscape' || entry.orientation === 'banner'"),
  'hero backdrop must prefer landscape/banner metadata artwork');
assert(source.includes("query: {name: actor.name, limit: 20}"),
  'related rail must reuse the existing local recording-person search');
assert(source.includes("'Weitere Filme mit ' + actor.name"),
  'related rail must be labelled by the selected main actor');
assert(source.includes('!sameRecording(candidate, recording)'),
  'current recording must be excluded from the related rail');
assert(source.includes("runtime.openRecording(recording"),
  'related cards must open the existing Recordings 2 detail runtime');
assert(source.includes("root.__vdrSuiteRecordingPlaybackOwner"),
  'hero playback action must reuse the existing playback owner');
assert(source.includes("focusSection(root, '.recordings2-marks-detail'"),
  'hero marks/cut actions must reuse the existing marks owner');
assert(!source.includes('similar') && !source.includes('Ähnliche Filme'),
  'v1 must not introduce a separate similarity/recommendation engine');
assert(metadataDetail.includes('heroDetail.enhance(root, recording, backendId, presented)'),
  'metadata enhancement must activate the hero detail after canonical metadata load');
assert(packaging.includes('web/frontend/recordings2-hero-detail.js'),
  'hero detail runtime must be bundled into the installed Recordings 2 browser runtime');
assert(packaging.includes('global.VdrSuiteRecordings2HeroDetail = Object.freeze'),
  'install staging must verify the bundled hero owner');

console.log('recordings2 hero detail contract ok');
