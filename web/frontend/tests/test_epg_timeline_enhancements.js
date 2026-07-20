'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-searchtimer-actions.js'),
  'utf8'
);

class MockStyle {
  constructor() {
    this.values = {};
    this.backgroundImage = '';
  }

  setProperty(name, value) {
    this.values[name] = String(value);
  }
}

class MockElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.parentNode = null;
    this.className = '';
    this.dataset = {};
    this.attributes = {};
    this.style = new MockStyle();
    this.textContent = '';
    this.disabled = false;
    this.hidden = false;
    this.value = '';
    this.listeners = {};
  }

  get classList() {
    return {
      add: (...names) => {
        const values = new Set(this.className.split(/\s+/).filter(Boolean));
        names.forEach(name => values.add(name));
        this.className = Array.from(values).join(' ');
      },
      contains: name => this.className.split(/\s+/).filter(Boolean).includes(name)
    };
  }

  appendChild(child) {
    this.children.push(child);
    if (child && typeof child === 'object') child.parentNode = this;
    return child;
  }

  insertBefore(child, reference) {
    const index = this.children.indexOf(reference);
    if (index < 0) return this.appendChild(child);
    this.children.splice(index, 0, child);
    child.parentNode = this;
    return child;
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }

  addEventListener(name, listener) {
    if (!this.listeners[name]) this.listeners[name] = [];
    this.listeners[name].push(listener);
  }

  dispatch(name) {
    (this.listeners[name] || []).forEach(listener => listener({currentTarget: this, target: this}));
  }

  querySelector(selector) {
    return this.querySelectorAll(selector)[0] || null;
  }

  querySelectorAll(selector) {
    const results = [];
    const matches = element => {
      if (!(element instanceof MockElement)) return false;
      if (selector.startsWith('.')) return element.classList.contains(selector.slice(1));
      if (selector === 'button') return element.tagName === 'BUTTON';
      if (selector.startsWith('[') && selector.endsWith(']')) {
        const content = selector.slice(1, -1);
        const [rawName, rawValue] = content.split('=');
        const name = rawName.replace(/^data-/, '').replace(/-([a-z])/g, (_, char) => char.toUpperCase());
        if (rawValue === undefined) return Object.prototype.hasOwnProperty.call(element.dataset, name);
        return String(element.dataset[name]) === rawValue.replace(/^"|"$/g, '');
      }
      return false;
    };
    const visit = element => {
      element.children.forEach(child => {
        if (!(child instanceof MockElement)) return;
        if (matches(child)) results.push(child);
        visit(child);
      });
    };
    visit(this);
    return results;
  }

  closest() {
    return null;
  }
}

const document = {
  documentElement: new MockElement('html'),
  head: new MockElement('head'),
  listeners: {},
  createElement(tagName) {
    return new MockElement(tagName);
  },
  getElementById(id) {
    return this.head.children.find(child => child.id === id) || null;
  },
  querySelector() {
    return null;
  },
  addEventListener(name, listener) {
    this.listeners[name] = listener;
  }
};

const window = {
  VdrSuitePlatform: {
    getSelectedBackendId() {
      return 'default';
    }
  },
  VdrSuiteClientApi: null,
  setTimeout(callback) {
    callback();
    return 1;
  },
  alert() {}
};

const context = vm.createContext({
  Array,
  Boolean,
  Date,
  Error,
  Event: function Event() {},
  Map,
  Math,
  Number,
  Object,
  Promise,
  Set,
  String,
  console,
  document,
  window
});

vm.runInContext(`
  let epgChannelOffset = 0;
  let selectedEpgDetail = null;
  const EPG_VISIBLE_CHANNEL_LIMIT = 15;
  const detailDataElement = document.createElement('section');
  let loadCount = 0;
  let lastRenderedChannels = [];

  function visibleEpgChannelsFromData(data) {
    return data.channels.slice(epgChannelOffset, epgChannelOffset + EPG_VISIBLE_CHANNEL_LIMIT);
  }

  function createEpgEventCard() {
    return document.createElement('button');
  }

  function createEpgProgramEventButton(entry) {
    return entry ? document.createElement('button') : document.createElement('p');
  }

  function createEpgEventDetailCard() {
    const detail = document.createElement('article');
    detail.className = 'epg-event-detail';
    const hero = document.createElement('div');
    hero.className = 'epg-detail-hero';
    detail.appendChild(hero);
    const actions = document.createElement('div');
    const search = document.createElement('button');
    search.className = 'epg-detail-action';
    search.textContent = 'Suchtimer';
    search.disabled = true;
    actions.appendChild(search);
    detail.appendChild(actions);
    return detail;
  }

  function renderEpgTimeView(channelData) {
    lastRenderedChannels = channelData.channels.slice();
    detailDataElement.children = [];
    const intro = document.createElement('article');
    intro.className = 'epg-timeline-intro';
    const description = document.createElement('p');
    description.textContent = 'Zeige Kanäle.';
    intro.appendChild(description);
    const modes = document.createElement('div');
    modes.className = 'epg-view-toggle';
    intro.appendChild(modes);
    const pager = document.createElement('div');
    pager.className = 'epg-pager';
    pager.appendChild(document.createElement('button'));
    pager.appendChild(document.createElement('button'));
    intro.appendChild(pager);
    detailDataElement.appendChild(intro);
  }

  function loadEpgTimeline() {
    loadCount += 1;
  }
`, context);

vm.runInContext(source, context, {filename: 'epg-searchtimer-actions.js'});

assert.ok(window.VdrSuiteEpgSearchTimerActions);
assert.strictEqual(window.VdrSuiteEpgSearchTimerActions.timelineEnhancementsReady(), true);
assert.strictEqual(document.documentElement.dataset.epgTimelineEnhancements, 'true');
assert.ok(document.getElementById('vdr-suite-epg-timeline-enhancements'));

const channels = [
  {id: 'a1', group: 'Öffentlich'},
  {id: 'a2', group: 'Öffentlich'},
  {id: 'b1', group: 'Private'},
  {id: 'c1', group: ''}
];

const visible = vm.runInContext(`visibleEpgChannelsFromData(${JSON.stringify({channels})})`, context);
assert.deepStrictEqual(Array.from(visible, channel => channel.id), ['a1', 'a2']);

vm.runInContext(`renderEpgTimeView(${JSON.stringify({channels})}, {events: []})`, context);
const renderedIds = vm.runInContext('lastRenderedChannels.map(channel => channel.id)', context);
assert.deepStrictEqual(Array.from(renderedIds), ['a1', 'a2']);

const detailData = vm.runInContext('detailDataElement', context);
const groupControl = detailData.querySelector('.epg-group-control');
assert.ok(groupControl);
const select = groupControl.querySelector('.epg-group-select');
assert.ok(select);
assert.strictEqual(select.value, 'Öffentlich');
assert.deepStrictEqual(
  select.children.map(option => option.textContent),
  ['Alle Kanäle (4)', 'Öffentlich (2)', 'Private (1)', 'Weitere Sender (1)']
);
assert.strictEqual(detailData.querySelector('.epg-pager').hidden, true);

select.value = 'Private';
select.dispatch('change');
assert.strictEqual(vm.runInContext('epgChannelOffset', context), 0);
assert.strictEqual(vm.runInContext('loadCount', context), 1);

const publicArtworkUrl = '/api/epg/cache/artwork?backend=default&channelId=C-1&eventId=17';
const artworkCard = vm.runInContext(
  `createEpgEventCard({event:{title:'Film',artwork:{available:true,url:'${publicArtworkUrl}'}}},{id:'C-1'})`,
  context
);
assert.ok(artworkCard.classList.contains('epg-has-artwork'));
assert.strictEqual(artworkCard.style.values['--epg-public-artwork'], `url("${publicArtworkUrl}")`);

const unavailableCard = vm.runInContext(
  `createEpgEventCard({event:{title:'Film',artwork:{available:false,url:'${publicArtworkUrl}'}}},{id:'C-1'})`,
  context
);
assert.ok(!unavailableCard.classList.contains('epg-has-artwork'));

const detail = vm.runInContext(
  `createEpgEventDetailCard({title:'Film',channelId:'C-1',artwork:{available:true,url:'${publicArtworkUrl}'}},{id:'C-1',group:'Öffentlich'})`,
  context
);
assert.ok(detail.classList.contains('epg-has-artwork'));
assert.ok(detail.querySelector('.epg-detail-artwork'));
const searchTimerButton = detail.querySelectorAll('.epg-detail-action')[0];
assert.strictEqual(searchTimerButton.disabled, false);
assert.strictEqual(searchTimerButton.textContent, 'Suchtimer erstellen');

assert.ok(source.includes('visibleEpgChannelsFromData = function'));
assert.ok(source.includes('renderEpgTimeView = function'));
assert.ok(source.includes('createEpgEventCard = function'));
assert.ok(source.includes('createEpgProgramEventButton = function'));
assert.ok(source.includes('createEpgEventDetailCard = function'));
assert.ok(source.includes("button.textContent = 'Suchtimer erstellen'"));
assert.ok(source.includes('artwork.available === true'));
assert.ok(!source.includes('MutationObserver'));

console.log('test_epg_timeline_enhancements passed');
