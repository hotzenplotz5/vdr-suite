'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'home-recording-discovery.js'),
  'utf8'
);

class FakeElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.attributes = {};
    this.dataset = {};
    this.listeners = {};
    this.parentNode = null;
    this.className = '';
    this.textContent = '';
    this.type = '';
    this.scrollIntoViewCalls = [];
    this.scrollLeft = 0;
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }

  appendChild(child) {
    if (!child) return child;
    child.parentNode = this;
    this.children.push(child);
    return child;
  }

  insertBefore(child, before) {
    if (child.parentNode) child.remove();
    child.parentNode = this;
    const index = before ? this.children.indexOf(before) : -1;
    if (index < 0) this.children.push(child);
    else this.children.splice(index, 0, child);
    return child;
  }

  append() {
    Array.from(arguments).forEach((child) => this.appendChild(child));
  }

  replaceChildren() {
    this.children.forEach((child) => { child.parentNode = null; });
    this.children = [];
    Array.from(arguments).forEach((child) => this.appendChild(child));
  }

  addEventListener(type, handler) {
    if (!this.listeners[type]) this.listeners[type] = [];
    this.listeners[type].push(handler);
  }

  remove() {
    if (!this.parentNode) return;
    const index = this.parentNode.children.indexOf(this);
    if (index >= 0) this.parentNode.children.splice(index, 1);
    this.parentNode = null;
  }

  querySelector(selector) {
    const match = String(selector || '').match(/^\[data-home-discovery-rail="([^"]+)"\]$/);
    if (!match) return null;
    return findElement(this, (element) =>
      element.attributes['data-home-discovery-rail'] === match[1]);
  }

  scrollIntoView(options) {
    this.scrollIntoViewCalls.push(options || {});
  }
}

function findElement(root, predicate) {
  for (const child of root.children || []) {
    if (predicate(child)) return child;
    const nested = findElement(child, predicate);
    if (nested) return nested;
  }
  return null;
}

const host = new FakeElement('div');
const openedRecordings = [];
const selectedModules = [];
let selectedModule = 'overview';

const document = {
  readyState: 'loading',
  head: null,
  querySelector(selector) {
    return selector === '[data-home-zone="additional-sections"]' ? host : null;
  },
  createElement(tagName) {
    return new FakeElement(tagName);
  },
  addEventListener() {},
  getElementById() { return null; }
};

const context = {window: {}, console};
context.window.window = context.window;
context.window.document = document;
context.window.setTimeout = function () { return 1; };
context.window.selectModule = function (moduleName) {
  selectedModule = moduleName;
  selectedModules.push(moduleName);
  return true;
};
context.window.VdrSuiteHomeLivePreview = {cancel() {}};
context.window.VdrSuiteRecordings2 = {
  openRecording(recording, options) {
    openedRecordings.push({recording, options});
  },
  openFolder() {}
};
context.window.VdrSuitePlatform = {
  getSelectedBackendId() { return 'default'; },
  getSelectedModule() { return selectedModule; },
  getClientApi() { return null; }
};

vm.createContext(context);
vm.runInContext(source, context);

const api = context.window.VdrSuiteHomeRecordingDiscovery;
assert(api && api._test);

const recording = {
  recordingId: 'test-series-s02e01',
  backendId: 'default',
  title: 'Serien/Testserie/S02E01 Pilot',
  metadata: {provider: {}, presentation: {}, artwork: {}}
};
const member = {
  recording,
  backendId: 'default',
  episodeNumber: 1,
  episodeTitle: 'Pilot',
  posterUrl: ''
};
const season = {
  number: 2,
  label: 'Staffel 2',
  episodes: [member]
};
const series = {
  key: 'folder:serien/testserie',
  title: 'Testserie',
  posterUrl: '',
  episodes: [member],
  seasons: [season]
};

const otherSeries = Object.assign({}, series, {key: 'other', title: 'Other'});
api._test.renderSeriesRail([series, otherSeries], 'default');
const initialSection = host.querySelector('[data-home-discovery-rail="series"]');
const initialRail = findElement(initialSection, element => element.className === 'media-home-discovery-rail series');
const initialCard = initialRail.children[0];
initialRail.scrollLeft = 280;
api._test.renderSeriesRail([Object.assign({}, series), otherSeries], 'default');
assert.strictEqual(initialRail.parentNode, initialSection, 'metadata retry retains the actual scrolling element');
assert.strictEqual(initialRail.children[0], initialCard, 'unchanged cards and poster nodes survive retries');
assert.strictEqual(initialRail.scrollLeft, 280);
const updatedOther = Object.assign({}, otherSeries, {posterUrl: '/new-poster.jpg'});
api._test.renderSeriesRail([series, updatedOther], 'default');
assert.strictEqual(initialRail.children[0], initialCard, 'one enriched poster does not recreate other cards');
assert.strictEqual(initialRail.children.length, 2);
assert.strictEqual(initialRail.scrollLeft, 280, 'metadata enrichment retains horizontal position');
api._test.renderSeriesRail([series], 'default');
assert.strictEqual(initialRail.children.length, 1, 'withdrawn series disappears');
assert.strictEqual(initialRail.children[0], initialCard);
assert.strictEqual(api._test.renderSeriesDetail(series, season, 'default'), true);
api._test.renderSeriesRail([series], 'default');
assert.strictEqual(initialRail.parentNode, initialSection, 'return from details remounts the original Series list');
assert.strictEqual(initialRail.scrollLeft, 280, 'return from details restores the list position after mounting');
assert.strictEqual(initialRail.children[0], initialCard);
api._test.renderSeriesDetail(series, season, 'default');
const seriesSection = host.querySelector('[data-home-discovery-rail="series"]');
assert(seriesSection);
assert.strictEqual(seriesSection.scrollIntoViewCalls.length, 0);

const episodeCard = findElement(seriesSection, (element) =>
  element.dataset && element.dataset.recordingId === recording.recordingId);
assert(episodeCard);
const seasonRail = findElement(seriesSection, element => element.className === 'media-home-series-season-rail');
const episodeRail = episodeCard.parentNode;
seasonRail.scrollLeft = 140;
episodeRail.scrollLeft = 280;
api._test.renderSeriesDetail(series, season, 'default');
assert.strictEqual(episodeCard.parentNode, episodeRail, 'unchanged detail keeps episode card mounted');
assert.strictEqual(episodeRail.parentNode, seriesSection, 'unchanged detail keeps scrolling rail mounted');
const enrichedSeries = Object.assign({}, series, {posterUrl: '/series-cover.jpg'});
api._test.renderSeriesDetail(enrichedSeries, season, 'default');
assert.strictEqual(episodeRail.parentNode, seriesSection, 'summary enrichment must not rebuild episode rail');
assert.strictEqual(seasonRail.parentNode, seriesSection, 'summary enrichment must not rebuild season rail');
assert.strictEqual(episodeRail.scrollLeft, 280);
assert.strictEqual(seasonRail.scrollLeft, 140);
assert(episodeCard.listeners.click && episodeCard.listeners.click.length === 1);
episodeCard.listeners.click[0]();

Promise.resolve().then(function () {
  return Promise.resolve();
}).then(function () {
  assert.strictEqual(openedRecordings.length, 1);
  assert.strictEqual(openedRecordings[0].recording, recording);
  assert.strictEqual(openedRecordings[0].options.backLabel, '← Zurück zur Staffel');
  assert.strictEqual(typeof openedRecordings[0].options.onClose, 'function');
  assert.strictEqual(selectedModules[0], 'recordings2');

  openedRecordings[0].options.onClose();

  assert.strictEqual(selectedModule, 'overview');
  assert.strictEqual(selectedModules[selectedModules.length - 1], 'overview');
  const returnedSection = host.querySelector('[data-home-discovery-rail="series"]');
  assert.strictEqual(returnedSection, seriesSection);
  assert(findElement(returnedSection, (element) =>
    element.dataset && element.dataset.recordingId === recording.recordingId));
  assert.strictEqual(returnedSection.scrollIntoViewCalls.length, 1);
  assert.strictEqual(returnedSection.scrollIntoViewCalls[0].block, 'start');
  assert.strictEqual(returnedSection.scrollIntoViewCalls[0].behavior, 'auto');

  // Reuse the production model for a second surface; membership must match
  // the real folder, not the global series title or provider grouping.
  const members = [
    {recordingId:'s1',backendId:'default',path:'/Serien/Alpha/ep1.rec',
      title:'Serien/Alpha/S01E01 Pilot',metadata:{provider:{seriesId:'111'}}},
    {recordingId:'s2',backendId:'default',path:'/Serien/Alpha/ep2.rec',
      title:'Serien/Alpha/S01E02 Next',metadata:{provider:{seriesId:'111'}}},
    {recordingId:'outside',backendId:'default',path:'/Elsewhere/Alpha/ep3.rec',
      title:'Alpha S01E03 Outside',metadata:{provider:{seriesId:'111'}}}
  ];
  members[0].metadata.presentation={posterUrl:'/alpha-series-poster.jpg'};
  const folderView = new FakeElement('div');
  host.appendChild(folderView);
  const upper = host.querySelector('[data-home-discovery-rail="series"]');
  let alive = true, fallback = 0;
  // Physical inventory has 464 recordings, the canonical genre is still
  // loading, and some physical recordings are not in that genre.
  assert(api.mountFolderSeries(folderView,'Serien','default',{
    count:()=>464,current:()=>alive,back:()=>{},fallback:()=>{fallback++;}
  }));
  assert(findElement(folderView, element=>element.textContent==='Serien werden gruppiert …'));
  api._test.applySeriesProjection(members,'default');
  assert.strictEqual(api._test.folderSeriesEntries('Serien',464,'default'),null);
  assert.strictEqual(api._test.folderSeriesEntries('Serien',464,'other',true),null);
  assert.strictEqual(api._test.folderSeriesEntries('Serien',464,'default',true).length,1);
  assert.strictEqual(fallback,0,'missing genre members must not replace the whole Series rail');
  const lowerRail = findElement(folderView, element=>element.className==='media-home-discovery-rail series');
  assert.strictEqual(lowerRail.children.length,1);
  const sharedUpperRail = findElement(upper, element=>element.className==='media-home-discovery-rail series');
  assert.strictEqual(lowerRail.children[0].className,sharedUpperRail.children[0].className);
  assert.strictEqual(lowerRail.children[0].children[0].children[0].src,sharedUpperRail.children[0].children[0].children[0].src);
  assert.strictEqual(lowerRail.children[0].children[0].children[0].src,'/alpha-series-poster.jpg');
  assert.strictEqual(lowerRail.children[0].children[1].children[0].textContent,sharedUpperRail.children[0].children[1].children[0].textContent);
  assert.strictEqual(lowerRail.children[0].children[1].children[1].textContent,'1 Staffel · 2 Folgen');
  assert(findElement(folderView, element=>element.textContent==='Alle Aufnahmen im Ordner'));
  lowerRail.scrollLeft=123;
  lowerRail.children[0].listeners.click[0]();
  const lowerSeason = findElement(folderView, element=>element.className==='media-home-series-season-rail');
  assert.strictEqual(lowerSeason.children.length,1);
  lowerSeason.children[0].listeners.click[0]();
  const lowerEpisodes = findElement(folderView, element=>element.className==='media-home-discovery-rail series-episodes');
  assert.deepStrictEqual(lowerEpisodes.children.map(card=>card.dataset.recordingId),['s1','s2']);
  const upperBefore=upper.children.slice();
  const back=findElement(folderView, element=>element.className==='media-home-series-back');
  back.listeners.click[0]();
  assert.strictEqual(lowerRail.parentNode,folderView);
  assert.strictEqual(lowerRail.scrollLeft,123);
  assert.deepStrictEqual(upper.children,upperBefore,'lower navigation leaves upper rail unchanged');
  const originals = findElement(folderView, element=>element.textContent==='Alle Aufnahmen im Ordner');
  originals.listeners.click[0]();
  assert.strictEqual(fallback,1,'unclassified recordings remain reachable by explicit folder navigation');
  alive=false; folderView.__stopFolderSeries();
  console.log('phase66 shared scoped series presentation and return-scroll contract ok');
}).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
