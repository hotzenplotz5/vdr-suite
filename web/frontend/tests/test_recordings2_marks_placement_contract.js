'use strict';

const assert = require('assert');
const fs = require('fs');

const timeline = fs.readFileSync(
  'web/frontend/recordings2-marks-timeline.js',
  'utf8'
);
const detail = fs.readFileSync(
  'web/frontend/recordings2-marks-detail.js',
  'utf8'
);
const editor = fs.readFileSync(
  'web/frontend/recordings2-marks-editor.js',
  'utf8'
);

assert(
  timeline.includes('canonicalTimeline: canonicalTimeline'),
  'timeline module must expose its canonical playback timeline'
);

assert(
  detail.includes('timeline.canonicalTimeline(root)'),
  'marks panel must use the canonical playback timeline'
);

const placementStart = detail.indexOf(
  'ensureTimelineRuntime().then(function (timeline)'
);
assert(placementStart >= 0);

const placement = detail.slice(
  placementStart,
  detail.indexOf('return true;', placementStart)
);

assert(
  !placement.includes('nativeMarksVisible'),
  'marks panel placement must not depend on marker rendering'
);

console.log(
  'recordings2 marks panel placement is independent of marker visibility'
);

assert(
  editor.includes("timeline.canonicalTimeline(root)") &&
  editor.includes("range.insertAdjacentElement('afterend', panel.section)"),
  'editor must restore the marks panel after playback-owner lifecycle changes'
);

console.log(
  'recordings2 marks panel follows playback-owner lifecycle changes'
);

assert(
  timeline.includes('height:2.25rem!important') &&
  timeline.includes('overflow:visible!important') &&
  timeline.includes('top:.38rem;bottom:0;width:2px'),
  'marker stem must end inside the marker hit box at the playback timeline'
);
