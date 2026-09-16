'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

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
assert(visibility.includes("setHidden(root, '.recordings2-detail-hero', !(detail || metadata))"),
  'hero visibility must allow the metadata-owned recording panel to show its recording tab');
assert(visibility.includes('function recordingPanel(root)'),
  'visibility wiring must identify the metadata panel that owns the hero');
assert(visibility.includes("panel.querySelector('.recordings2-detail-hero')"),
  'recording panel detection must follow the real nested hero DOM');
assert(visibility.includes('recordings2-hero-recording-panel'),
  'nested recording panel must receive an explicit hero layout hook');
assert(visibility.includes('function releaseDisplay(element)'),
  'metadata mode must release wrapper-owned inline display state back to the metadata tab owner');
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

function makeStyle() {
  const values = Object.create(null);
  return {
    values,
    setProperty(name, value, priority) {
      values[name] = {value, priority: priority || ''};
    },
    removeProperty(name) {
      delete values[name];
    }
  };
}

function makeNode(name) {
  const classes = new Set();
  return {
    name,
    hidden: false,
    style: makeStyle(),
    classList: {
      add(value) { classes.add(value); },
      contains(value) { return classes.has(value); }
    },
    querySelector() { return null; },
    getAttribute() { return null; }
  };
}

const hero = makeNode('hero');
const grid = makeNode('grid');
const related = makeNode('related');
const playbackPanel = makeNode('playback');
const actions = makeNode('actions');
const tabsNode = makeNode('tabs');
const assignment = makeNode('assignment');
const assignmentError = makeNode('assignmentError');
const back = makeNode('back');
const recordingPanelNode = makeNode('recordingPanel');
const scraperPanelNode = makeNode('scraperPanel');
const castPanelNode = makeNode('castPanel');

recordingPanelNode.querySelector = function (selector) {
  return selector === '.recordings2-detail-hero' ? hero : null;
};

let selectedTab = 1;
const tabNodes = [0, 1, 2].map(function (index) {
  const tab = makeNode('tab' + index);
  tab.getAttribute = function (name) {
    return name === 'aria-selected' && index === selectedTab ? 'true' : 'false';
  };
  return tab;
});

const selectorMap = new Map([
  ['.recordings2-detail-hero', [hero]],
  ['.recordings2-detail-grid', [grid]],
  ['.recordings2-hero-related', [related]],
  ['.recordings2-playback', [playbackPanel]],
  ['.recordings2-actions', [actions]],
  ['.recordings2-metadata-tabs', [tabsNode]],
  ['.recordings2-metadata-panel', [recordingPanelNode, scraperPanelNode, castPanelNode]],
  ['.recordings2-metadata-tab', tabNodes],
  ['.recordings2-metadata-assignment', [assignment]],
  ['[data-recordings2-metadata-assignment-error]', [assignmentError]]
]);

const root = {
  dataset: {recordings2HeroMode: 'detail'},
  querySelectorAll(selector) { return selectorMap.get(selector) || []; },
  querySelector(selector) {
    return selector === '.recordings2-hero-mode-back' ? back : null;
  }
};

function MutationObserver() {
  this.observe = function () {};
}

const sandbox = {
  window: {
    VdrSuiteRecordings2HeroDetail: Object.freeze({enhance() { return root; }}),
    MutationObserver,
    document: {getElementById() { return {}; }}
  },
  console
};
sandbox.window.window = sandbox.window;
vm.runInNewContext(visibility, sandbox, {filename: 'recordings2-hero-visibility.js'});
const visibilityTest = sandbox.window.VdrSuiteRecordings2HeroDetail.__test;

visibilityTest.apply(root);
assert.strictEqual(recordingPanelNode.hidden, false,
  'detail mode must keep the metadata panel that owns the hero visible');
assert.strictEqual(scraperPanelNode.hidden, true,
  'detail mode must hide non-recording metadata panels');
assert.strictEqual(hero.hidden, false, 'detail mode must show the hero');
assert.strictEqual(grid.hidden, false, 'detail mode must show recording facts');
assert.strictEqual(playbackPanel.hidden, true, 'detail mode must hide playback');
assert.strictEqual(
  recordingPanelNode.style.values.display && recordingPanelNode.style.values.display.value,
  'block',
  'detail mode must override the legacy CSS rule that hides metadata panels'
);

root.dataset.recordings2HeroMode = 'metadata';
visibilityTest.apply(root);
assert.strictEqual(recordingPanelNode.hidden, true,
  'metadata mode must preserve the selected non-recording tab');
assert.strictEqual(scraperPanelNode.hidden, false,
  'metadata mode must reveal the selected scraper panel');
assert.strictEqual(recordingPanelNode.style.values.display, undefined,
  'metadata mode must release inline display ownership on recording panel');
assert.strictEqual(scraperPanelNode.style.values.display, undefined,
  'metadata mode must release inline display ownership on selected panel');
assert.strictEqual(tabsNode.hidden, false, 'metadata mode must show metadata tabs');
assert.strictEqual(playbackPanel.hidden, true, 'metadata mode must keep playback hidden');

selectedTab = 2;
visibilityTest.apply(root);
assert.strictEqual(scraperPanelNode.hidden, true,
  'metadata mode must follow a changed tab selection');
assert.strictEqual(castPanelNode.hidden, false,
  'metadata mode must reveal the newly selected tab panel');
assert.strictEqual(castPanelNode.style.values.display, undefined,
  'metadata panel switching must not be blocked by stale inline display');

root.dataset.recordings2HeroMode = 'detail';
visibilityTest.apply(root);
assert.strictEqual(recordingPanelNode.hidden, false,
  'returning to detail must restore the hero-owning recording panel');
assert.strictEqual(playbackPanel.hidden, true,
  'returning to detail must hide playback again');

root.dataset.recordings2HeroMode = 'playback';
visibilityTest.apply(root);
assert.strictEqual(playbackPanel.hidden, false,
  'playback mode must reveal the playback panel');
assert.strictEqual(recordingPanelNode.hidden, true,
  'playback mode must hide the recording metadata panel');
assert.strictEqual(hero.hidden, true,
  'playback mode must hide hero content');

console.log('recordings2 hero-first detail contract ok');
