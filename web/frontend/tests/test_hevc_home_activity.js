'use strict';
// Focused integration of the real Home HEVC card with the canonical
// Public v1 operation read, actor-owned SSE and Recording 2 navigation.
const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const app = fs.readFileSync('web/frontend/app.js', 'utf8');
const begin = app.indexOf('// A compact shortcut to the already-owned HEVC operation.');
const end = app.indexOf('function renderSelectedModule(data)', begin);
assert(begin >= 0 && end > begin, 'Home HEVC owner not composed in app.js');
const owner = app.slice(begin, end);

class Node {
  constructor(tag) {
    this.tagName = tag;
    this.children = [];
    this.parentNode = null;
    this.attributes = {};
    this.style = {};
    this.textContent = '';
    this.disabled = false;
  }
  setAttribute(key, value) { this.attributes[key] = value; }
  appendChild(child) {
    this.children.push(child); child.parentNode = this; return child;
  }
  prepend(child) {
    this.children.unshift(child); child.parentNode = this; return child;
  }
  remove() {
    if (!this.parentNode) return;
    this.parentNode.children = this.parentNode.children.filter(x => x !== this);
    this.parentNode = null;
  }
  querySelector(selector) {
    const direct = selector === 'button'
      ? node => node.tagName === 'button'
      : node => Object.prototype.hasOwnProperty.call(
        node.attributes, selector.slice(1, -1));
    const visit = node => {
      for (const child of node.children) {
        if (direct(child)) return child;
        const nested = visit(child);
        if (nested) return nested;
      }
      return null;
    };
    return visit(this);
  }
}
const backendId = 'backend-a';
const recordingId = 'rec_' + 'a'.repeat(32);
const operationId = 'op_' + 'b'.repeat(32);
const storage = new Map();
storage.set('vdr-suite-hevc-current-v1.' + backendId, JSON.stringify({
  backendId, recordingId, operationId,
  recording: {title: 'Civil War 360 · Folge 1', recordingId}
}));
const reads = [];
const streams = [];
const openings = [];
const navigation = [];
let stageState = 'outcome_unknown';

const host = new Node('section');
const listeners = {};
const document = {
  querySelector(selector) {
    return selector === '[data-home-zone="additional-sections"]' ? host : null;
  },
  createElement(tag) { return new Node(tag); },
  addEventListener(name, callback) {listeners[name] = callback;}
};
const fakePublicClient = {
  getMediaShrinkOperation(request) {
    reads.push(request);
    return Promise.resolve({
      backendId, operationId, action: 'stage', state: stageState,
      resultReconciled: stageState === 'succeeded',
      reviewId: stageState === 'succeeded'
        ? 'shk_' + 'c'.repeat(32) : ''
    });
  }
};
const window = {
  sessionStorage: {
    getItem(key) {return storage.get(key) || null;},
    setItem(key, value) {storage.set(key, value);},
    get length() {return storage.size;},
    key(index) {return Array.from(storage.keys())[index] || null;}
  },
  VdrSuitePublicV1Client: {createClient() {return fakePublicClient;}},
  VdrSuiteClientApi: {
    createClientOperationUpdateSource(id) {
      assert.strictEqual(id, operationId);
      const source = {
        closed: false, listeners: {},
        addEventListener(name, cb) {this.listeners[name] = cb;},
        close() {this.closed = true;}
      };
      streams.push(source);
      return source;
    }
  },
  VdrSuiteRecordings2: {
    openRecording(recording, options) {
      openings.push({recording, options});
    }
  }
};
const sandbox = {window, document, Promise, console, navigation, openings};
vm.createContext(sandbox);
vm.runInContext(
  'let selectedBackendId = "backend-a";' +
  'let selectedModule = "overview";' +
  'function selectModule(name) { navigation.push(name); selectedModule = name; }' +
  owner + '\nthis.renderHevcHomeActivity = renderHevcHomeActivity;',
  sandbox
);

async function flush() {
  for (let i = 0; i < 12; ++i) await Promise.resolve();
}
(async function run() {
  sandbox.renderHevcHomeActivity();
  await flush();
  const section = host.querySelector('[data-home-hevc-activity]');
  assert(section, 'Home displays saved HEVC operation without folder navigation');
  assert.strictEqual(
    section.querySelector('[data-home-hevc-title]').textContent,
    'Civil War 360 · Folge 1');
  const state = section.querySelector('[data-home-hevc-state]');
  assert(state.textContent.includes('Ergebnis noch ungeklärt'));
  assert.strictEqual(reads.length, 1);
  assert.strictEqual(streams.length, 1);

  streams[0].listeners.operation({data: JSON.stringify({
    operationId: 'op_' + 'f'.repeat(32)
  })});
  await flush();
  assert.strictEqual(reads.length, 1,
    'foreign Operation update never triggers a status read');
  stageState = 'succeeded';
  streams[0].listeners.operation({data: JSON.stringify({operationId})});
  await flush();
  assert(state.textContent.includes('zur Veröffentlichung bereit'));
  assert.strictEqual(reads.length, 2);

  const button = section.querySelector('button');
  assert(button, 'Home has direct action to canonical recording owner');
  button.onclick();
  await flush();
  assert.deepStrictEqual(navigation, ['recordings2']);
  assert.strictEqual(openings.length, 1);
  assert.strictEqual(openings[0].recording.recordingId, recordingId);
  assert.strictEqual(openings[0].recording.__vdrSuiteOpenHevcShrink, true);
  assert.strictEqual(streams[0].closed, true,
    'Home EventSource is closed during recording navigation');
  assert.strictEqual(reads.length, 2,
    'opening HEVC detail never submits a mutation');

  storage.delete('vdr-suite-hevc-current-v1.' + backendId);
  sandbox.renderHevcHomeActivity();
  assert.strictEqual(host.querySelector('[data-home-hevc-activity]'), null);

  // Upgrade from the already-installed stage-only session format. The
  // user must not find the original recording again just to see progress.
  storage.set('vdr-suite-hevc-v1.' + backendId + '.' + recordingId + '.stage',
    JSON.stringify({key: 'existing_hevc_operation', operationId}));
  stageState = 'outcome_unknown';
  sandbox.renderHevcHomeActivity();
  await flush();
  const migrated = host.querySelector('[data-home-hevc-activity]');
  assert(migrated && migrated.querySelector('[data-home-hevc-title]')
    .textContent === 'Früherer HEVC-Auftrag');
  assert(migrated.querySelector('[data-home-hevc-state]').textContent
    .includes('Ergebnis noch ungeklärt'));
  assert(typeof listeners['vdr-suite:hevc-activity-changed'] === 'function');
  console.log('HEVC_HOME_OPERATION_SSE_DIRECT_NAVIGATION=PASS');
})().catch(error => { console.error(error); process.exitCode = 1; });
