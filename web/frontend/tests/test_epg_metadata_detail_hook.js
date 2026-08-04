'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const hookSource = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-metadata-detail-hook.js'),
  'utf8'
);
const focusSource = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-detail-desktop-focus.js'),
  'utf8'
);

class ClassState {
  constructor() {
    this.values = new Set();
  }

  toggle(name, enabled) {
    if (enabled) this.values.add(name); else this.values.delete(name);
  }

  remove(...names) {
    names.forEach(name => this.values.delete(name));
  }

  contains(name) {
    return this.values.has(name);
  }
}

let enhanced = null;
const listeners = {};
const headChildren = [];
const document = {
  documentElement: {dataset: {}},
  head: {
    appendChild(element) {
      headChildren.push(element);
      return element;
    }
  },
  createElement(tagName) {
    return {tagName: String(tagName).toUpperCase(), id: '', textContent: ''};
  },
  getElementById(id) {
    return headChildren.find(element => element.id === id) || null;
  },
  addEventListener(name, listener) {
    if (!listeners[name]) listeners[name] = [];
    listeners[name].push(listener);
  },
  querySelectorAll() {
    return [];
  }
};
const window = {
  innerWidth: 1920,
  VdrSuiteEpgMetadataDetail: {
    enhance(detail, event, channel) {
      enhanced = {detail, event, channel};
    }
  },
  setTimeout(callback) {
    callback();
    return 1;
  },
  matchMedia() {
    return {
      matches: true,
      addEventListener() {}
    };
  }
};

const context = vm.createContext({
  Boolean,
  Number,
  Object,
  document,
  window
});

vm.runInContext(`
  function createEpgEventDetailCard(event, channel) {
    return {kind: 'detail', event: event, channel: channel};
  }
`, context);
vm.runInContext(hookSource, context, {filename: 'epg-metadata-detail-hook.js'});

assert.ok(window.VdrSuiteEpgMetadataDetailHook);
assert.strictEqual(window.VdrSuiteEpgMetadataDetailHook.installed(), true);
assert.strictEqual(document.documentElement.dataset.epgMetadataDetailHook, 'true');

const detail = vm.runInContext(
  `createEpgEventDetailCard({id:'18829'},{id:'channel-1'})`,
  context
);
assert.strictEqual(detail.kind, 'detail');
assert.ok(enhanced);
assert.strictEqual(enhanced.detail, detail);
assert.strictEqual(enhanced.event.id, '18829');
assert.strictEqual(enhanced.channel.id, 'channel-1');

vm.runInContext(focusSource, context, {filename: 'epg-detail-desktop-focus.js'});

assert.ok(window.VdrSuiteEpgDetailDesktopFocus);
assert.strictEqual(window.VdrSuiteEpgDetailDesktopFocus.installed(), true);
assert.strictEqual(document.documentElement.dataset.epgDetailDesktopFocus, 'true');
assert.ok(headChildren[0].textContent.includes('@media(min-width:1100px)'));
assert.ok(headChildren[0].textContent.includes(
  '.epg-metadata-tabs{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));width:100%;max-width:100%'
));
assert.ok(headChildren[0].textContent.includes('overflow:visible'));
assert.ok(headChildren[0].textContent.includes(
  '@media(max-width:380px){.epg-metadata-tabs{grid-template-columns:repeat(2,minmax(0,1fr))}}'
));
assert.ok(headChildren[0].textContent.includes('.epg-metadata-tab{min-width:0;width:100%'));
assert.ok(!headChildren[0].textContent.includes('width:max-content'));
assert.ok(!headChildren[0].textContent.includes('overflow-x:auto'));
assert.ok(headChildren[0].textContent.includes(
  '#detail-data.channels2-mount .channels2-detail.has-artwork>.channels2-artwork,#detail-data.channels2-mount .channels2-detail.epg-has-artwork>.epg-detail-artwork{grid-column:1;grid-row:1}'
));
assert.ok(headChildren[0].textContent.includes(
  '#detail-data.channels2-mount .channels2-detail.has-artwork>.epg-detail-hero,#detail-data.channels2-mount .channels2-detail.epg-has-artwork>.epg-detail-hero{grid-column:2;grid-row:1;min-width:0}'
));
assert.ok(headChildren[0].textContent.includes(
  '@media(max-width:720px){#detail-data.channels2-mount .channels2-detail.has-artwork>.channels2-artwork'
));
assert.ok(headChildren[0].textContent.includes(
  '.channels2-detail.epg-has-artwork>.epg-detail-hero{grid-column:1;grid-row:auto}}'
));
assert.ok(headChildren[0].textContent.includes('.epg-detail-sidebar{position:relative;z-index:3;min-width:0}'));
assert.ok(headChildren[0].textContent.includes(
  '.epg-workbench.epg-detail-expanded .epg-side-detail{position:absolute!important;right:0!important;left:auto!important;top:0!important;'
));
assert.ok(headChildren[0].textContent.includes('width:min(42rem,calc(100vw - 3rem))'));
assert.ok(!headChildren[0].textContent.includes('position:fixed'));
assert.ok(!headChildren[0].textContent.includes('@media(max-width:1099px)'));

const workbench = {
  classList: new ClassState(),
  dataset: {}
};
const sideDetail = {
  closest(selector) {
    return selector === '.epg-workbench' ? workbench : null;
  }
};
const detailTarget = {
  closest(selector) {
    if (selector === '.epg-side-detail') return sideDetail;
    if (selector === '.epg-workbench') return workbench;
    return null;
  }
};
const main = {
  closest(selector) {
    return selector === '.epg-workbench' ? workbench : null;
  }
};
const timelineTarget = {
  closest(selector) {
    if (selector === '.epg-workbench-main') return main;
    if (selector === '.epg-workbench') return workbench;
    return null;
  }
};

listeners.pointerdown[0]({target: detailTarget});
assert.strictEqual(workbench.dataset.epgDetailExpanded, 'true');
assert.ok(workbench.classList.contains('epg-detail-expanded'));
assert.ok(!workbench.classList.contains('epg-timeline-foreground'));

listeners.pointerover[0]({target: timelineTarget});
assert.strictEqual(workbench.dataset.epgDetailExpanded, 'false');
assert.ok(!workbench.classList.contains('epg-detail-expanded'));
assert.ok(workbench.classList.contains('epg-timeline-foreground'));

listeners.pointerdown[0]({target: detailTarget});
listeners.keydown[0]({target: detailTarget, key: 'Escape'});
assert.strictEqual(workbench.dataset.epgDetailExpanded, 'false');

assert.ok(hookSource.includes('createEpgEventDetailCard = function'));
assert.ok(!hookSource.includes('renderEpgTimeView'));
assert.ok(!focusSource.includes('renderEpgTimeView'));
assert.ok(!focusSource.includes('visibleEpgChannelsFromData'));
assert.ok(!focusSource.includes('appendEpgVerticalTimelineTicks'));
assert.ok(!focusSource.includes('createEpgEventCard'));
assert.ok(!focusSource.includes('createEpgProgramEventButton'));

console.log('test_epg_metadata_detail_hook passed');