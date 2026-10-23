'use strict';
// Production Hero owner -> read-only Plan -> fenced Stage -> independent Review
// -> explicit Publish. No native paths or implicit retransmissions.
const assert = require('assert');
const fs = require('fs');
const vm = require('vm');
const hero = fs.readFileSync('web/frontend/recordings2-hero-detail.js', 'utf8');
const backendId = 'backend-a';
const recordingId = 'rec_' + 'a'.repeat(32);
const reviewId = 'shk_' + 'b'.repeat(32);
const stageOperationId = 'op_' + 'c'.repeat(32);
const publishOperationId = 'op_' + 'd'.repeat(32);
const publishedRecordingId = 'rec_' + 'e'.repeat(32);
const calls = [];
const storage = new Map();
let keys = 0;
let confirmations = 0;
class Node {
  constructor(tag, className, label) {
    this.tagName = tag;
    this.className = className || '';
    this.textContent = label || '';
    this.children = [];
    this.parentNode = null;
    this.events = {};
    this.hidden = false;
    this.disabled = false;
    this.isConnected = true;
    this.dataset = {};
    this.style = {setProperty() {}};
    this.classList = {add: (name) => {this.className += ' ' + name;}};
  }
  appendChild(node) {this.children.push(node); node.parentNode = this; return node;}
  append(...nodes) {for (const n of nodes) this.appendChild(n);}
  prepend(node) {this.children.unshift(node); node.parentNode = this;}
  insertBefore(node, reference) {
    const at = reference ? this.children.indexOf(reference) : -1;
    if (at < 0) return this.appendChild(node);
    this.children.splice(at, 0, node); node.parentNode = this; return node;
  }
  get firstChild() {return this.children[0] || null;}
  addEventListener(name, handler) {this.events[name] = handler;}
  setAttribute() {}
  click() {if (!this.disabled && this.events.click) this.events.click();}
  querySelector(selector) {
    if (!selector.startsWith('.')) return null;
    const name = selector.slice(1);
    for (const node of this.children) {
      if (node.className.split(/\s+/).includes(name)) return node;
      const nested = node.querySelector(selector);
      if (nested) return nested;
    }
    return null;
  }
  scrollIntoView() {}
}
const api = {
  getMediaShrinkCapability() {return Promise.resolve({
    backendId, capability: 'addons.media.shrink', planningAvailable: true,
    available: true, encoders: ['software', 'vaapi', 'qsv', 'nvenc'],
    encoderFallbackToSoftware: true
  });},
  submitMediaShrinkPlan(request) {
    calls.push({action: 'plan', request});
    return Promise.resolve({backendId, recordingId, readOnly: true,
      stageAvailable: true, sourceBytes: 1000000, segments: 1});
  },
  submitMediaShrinkStage(request) {
    calls.push({action: 'stage', request});
    return Promise.resolve({backendId, operationId: stageOperationId});
  },
  getMediaShrinkOperation(request) {
    calls.push({action: 'readOperation', request});
    if (request.operationId === stageOperationId)
      return Promise.resolve({backendId, operationId: stageOperationId,
        action: 'stage', state: 'succeeded', resultReconciled: true, reviewId});
    if (request.operationId === publishOperationId)
      return Promise.resolve({backendId, operationId: publishOperationId,
        action: 'publish', state: 'succeeded', resultReconciled: true,
        publishedRecordingId});
    throw new Error('Unexpected operation');
  },
  getMediaShrinkReview(request) {
    calls.push({action: 'readReview', request});
    return Promise.resolve({backendId, recordingId, reviewId, state: 'staged',
      revision: 1, publishAvailable: true, candidateBytes: 400000,
      sourceBytes: 1000000});
  },
  submitMediaShrinkPublish(request) {
    calls.push({action: 'publish', request});
    return Promise.resolve({backendId, operationId: publishOperationId});
  }
};
const context = {console, document: {
  head: new Node('head'),
  getElementById() {return null;},
  createElement(tag) {return new Node(tag);}
}, location: {origin: 'https://example.invalid'},
  crypto: {randomUUID() {return '00000000-0000-4000-8000-' +
    String(++keys).padStart(12, '0');}},
  confirm(message) {assert(message.includes('Original bleibt erhalten')); ++confirmations; return true;},
  setTimeout() {throw new Error('Reconciled operations must not poll again');},
  sessionStorage: {
    setItem(key, value) {storage.set(key, value);},
    getItem(key) {return storage.get(key) || null;}
  },
  VdrSuiteBrowserSession: {
    restore() {return Promise.resolve(true);},
    csrfHeaders() {return {'X-CSRF-Token': 'test-csrf'};}
  },
  VdrSuitePublicV1Client: {createClient() {return api;}},
  VdrSuiteRecordings2Shared: {
    node(tag, classes, label) {return new Node(tag, classes, label);},
    text(value) {return value == null ? '' : String(value);},
    first(item, fields, fallback) {
      for (const field of fields) if (item && item[field] != null) return item[field];
      return fallback;
    },
    clientApi() {return null;}
  }
};
context.window = context;
vm.createContext(context);
vm.runInContext(hero, context);
const root = new Node('main');
const copy = root.appendChild(new Node('div', 'recordings2-detail-copy'));
const recording = {recordingId, title: 'Testaufnahme'};
async function flush() {
  for (let i = 0; i < 20; i++) await Promise.resolve();
}
function button(label) {
  const nodes = [];
  function visit(node) {
    if (node.tagName === 'button' && node.textContent === label) nodes.push(node);
    node.children.forEach(visit);
  }
  visit(root);
  assert.strictEqual(nodes.length, 1, 'Expected one button: ' + label);
  return nodes[0];
}
function nodesByTag(tag) {
  const nodes = [];
  function visit(node) {if (node.tagName === tag) nodes.push(node);
    node.children.forEach(visit);}
  visit(root);
  return nodes;
}
(async function run() {
  context.VdrSuiteRecordings2HeroDetail.enhance(root, recording, backendId, {});
  await flush();
  button('HEVC-Vorprüfung').click();
  button('Vorprüfung starten').click();
  await flush();
  assert.strictEqual(calls.filter(c => c.action === 'plan').length, 1);
  assert.strictEqual(calls[0].request.recordingId, recordingId);
  const select = nodesByTag('select').find(n =>
    n.className === 'recordings2-media-shrink-encoder');
  assert(select);
  assert.deepStrictEqual(select.children.map(n => n.value),
    ['software', 'vaapi', 'qsv', 'nvenc']);
  select.value = 'vaapi';
  const inputs = nodesByTag('input');
  assert.strictEqual(inputs.length, 2);
  inputs[0].checked = true;
  inputs[1].checked = true;
  button('HEVC-Konvertierung starten').click();
  await flush();
  const stages = calls.filter(c => c.action === 'stage');
  assert.strictEqual(stages.length, 1);
  assert.strictEqual(stages[0].request.encoder, 'vaapi');
  assert.strictEqual(stages[0].request.maxHeight, 720);
  assert.strictEqual(stages[0].request.nightMode, true);
  assert.strictEqual(stages[0].request.headers['X-CSRF-Token'], 'test-csrf');
  assert.strictEqual(stages[0].request.recordingId, recordingId);
  assert.strictEqual(calls.filter(c => c.action === 'readReview').length, 1);
  const publish = button('Kandidaten ausdrücklich veröffentlichen');
  assert.strictEqual(publish.hidden, false);
  publish.click();
  await flush();
  assert.strictEqual(confirmations, 1);
  const publishes = calls.filter(c => c.action === 'publish');
  assert.strictEqual(publishes.length, 1);
  assert.strictEqual(publishes[0].request.reviewId, reviewId);
  assert.strictEqual(publishes[0].request.revision, 1);
  assert.strictEqual(publishes[0].request.headers['X-CSRF-Token'], 'test-csrf');
  publish.click();
  button('HEVC-Konvertierung starten').click();
  await flush();
  assert.strictEqual(calls.filter(c => c.action === 'stage').length, 1);
  assert.strictEqual(calls.filter(c => c.action === 'publish').length, 1);
  assert.strictEqual(publish.hidden, true);
  assert(storage.size >= 2);
  for (const entry of calls) assert(!JSON.stringify(entry).includes('/srv/'));
  console.log('MEDIA_SHRINK_HERO_OWNER_STAGE_REVIEW_PUBLISH=PASS');
})().catch(error => {console.error(error); process.exitCode = 1;});
