'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const sourcePath = path.resolve(__dirname, '..', 'channel-day-program.js');
const source = fs.readFileSync(sourcePath, 'utf8');
const marker = 'global.VdrSuiteChannels2 = moduleApi;';
const instrumentedSource = source.replace(
  marker,
  'global.__VdrSuiteChannelDayNavigationTest=Object.freeze({' +
    'dateNavigationLabel,sameDay,renderInlineProgram,state});' + marker
);

assert.notStrictEqual(instrumentedSource, source);

class MockElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.attributes = {};
    this.className = '';
    this.dataset = {};
    this.onclick = null;
    this.style = {};
    this.textContent = '';
    this.type = '';
    this.value = '';
    this.parentNode = null;
    this.classList = {
      add: (...names) => {
        const values = new Set(this.className.split(/\s+/).filter(Boolean));
        names.forEach(name => values.add(name));
        this.className = Array.from(values).join(' ');
      },
      contains: name => this.className.split(/\s+/).filter(Boolean).includes(name),
      remove: (...names) => {
        const removed = new Set(names);
        this.className = this.className
          .split(/\s+/)
          .filter(name => name && !removed.has(name))
          .join(' ');
      }
    };
  }

  append(...children) {
    children.forEach(child => this.appendChild(child));
  }

  appendChild(child) {
    this.children.push(child);
    if (child && typeof child === 'object') child.parentNode = this;
    return child;
  }

  querySelector(selector) {
    if (!selector.startsWith('.')) return null;
    const className = selector.slice(1);
    return this.find(element => element.classList.contains(className));
  }

  find(predicate) {
    for (const child of this.children) {
      if (!(child instanceof MockElement)) continue;
      if (predicate(child)) return child;
      const nested = child.find(predicate);
      if (nested) return nested;
    }
    return null;
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }
}

const window = {
  VdrSuitePlatform: {
    getSelectedBackendId() { return 'living-room'; },
    getClientApi() { return null; },
    getMountTarget() { return null; }
  },
  setTimeout,
  clearTimeout
};

const document = {
  head: new MockElement('head'),
  createElement(tagName) { return new MockElement(tagName); },
  getElementById() { return null; },
  querySelector() { return null; }
};

vm.runInNewContext(instrumentedSource, {
  Array,
  Boolean,
  Date,
  Event: function Event() {},
  JSON,
  Map,
  Math,
  Number,
  Object,
  Promise,
  Set,
  String,
  document,
  window
}, {filename: sourcePath});

const testApi = window.__VdrSuiteChannelDayNavigationTest;
assert.ok(testApi);

const selectedDay = new Date(2035, 6, 27);
assert.strictEqual(testApi.dateNavigationLabel(selectedDay), '27.07.2035');
assert.strictEqual(testApi.sameDay(selectedDay, new Date(2035, 6, 27, 23, 59)), true);
assert.strictEqual(testApi.sameDay(selectedDay, new Date(2035, 6, 28)), false);

testApi.state.day = selectedDay;
testApi.state.events = [];
testApi.state.event = null;
testApi.state.encryptionAvailable = false;

const channel = {
  id: 'C-1-1-1',
  name: 'Das Erste HD',
  number: 1,
  group: 'DieOeffentlichen',
  enabled: true
};

const futureSection = testApi.renderInlineProgram(channel);
const controls = futureSection.querySelector('.channels2-date');
assert.ok(controls);
const currentLabel = controls.querySelector('.channels2-date-current');
assert.ok(currentLabel);
assert.strictEqual(currentLabel.tagName, 'SPAN');
assert.strictEqual(currentLabel.textContent, '27.07.2035');
assert.strictEqual(currentLabel.attributes['aria-label'], 'Ausgewählter Tag: 27.07.2035');

const todayButton = controls.querySelector('.channels2-date-today');
assert.ok(todayButton);
assert.strictEqual(todayButton.tagName, 'BUTTON');
assert.strictEqual(todayButton.textContent, 'Programm heute');
assert.strictEqual(typeof todayButton.onclick, 'function');
todayButton.onclick();
assert.strictEqual(testApi.sameDay(testApi.state.day, new Date()), true);

const todaySection = testApi.renderInlineProgram(channel);
assert.strictEqual(todaySection.querySelector('.channels2-date-today'), null);
assert.strictEqual(
  todaySection.querySelector('.channels2-date-current').textContent,
  testApi.dateNavigationLabel(new Date())
);

assert.ok(source.includes("const current = addText(document.createElement('span'), dateNavigationLabel(selectedDay));"));
assert.ok(source.includes("'Programm heute'"));
assert.ok(!source.includes("const today = addText(document.createElement('button'), 'Heute');"));

console.log('test_channel_day_navigation_runtime passed');
