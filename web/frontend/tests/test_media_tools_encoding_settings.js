'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

const source = fs.readFileSync('web/frontend/settings-media-tools-encoding.js', 'utf8');
const bootstrap = fs.readFileSync('web/frontend/settings-series-artwork.js', 'utf8');
const gate = fs.readFileSync('core/security/include/SecurityHttpGate.h', 'utf8');
const agent = fs.readFileSync('apps/agent/main.cpp', 'utf8');
const header = fs.readFileSync('core/daemon/include/LocalMediaEncodingAdmin.h', 'utf8');
new vm.Script(source);
assert.match(bootstrap, /loadMediaToolsEncodingSettingsRuntime\(\);/);
assert.match(gate, /isAddonEncodingRead/);
assert.match(gate, /isAddonEncodingMutation/);
assert.match(gate, /requestToAuthorize\.permission = "role.admin";/);
assert.match(agent, /currentMediaEncoding/);
assert.match(header, /backend-agent\.conf/);

class Element {
  constructor(tag) {
    this.tag = tag;
    this.children = [];
    this.dataset = {};
    this.listeners = {};
    this.id = '';
    this.textContent = '';
    this.value = '';
    this.disabled = false;
  }
  appendChild(child) {
    this.children.push(child);
    child.parentNode = this;
    return child;
  }
  setAttribute(name, value) { this[name] = value; }
  addEventListener(name, callback) { this.listeners[name] = callback; }
  remove() {
    if (this.parentNode)
      this.parentNode.children = this.parentNode.children.filter((x) => x !== this);
  }
  get options() { return this.children.filter((x) => x.tag === 'option'); }
}
function walk(root, predicate) {
  if (predicate(root)) return root;
  for (const child of root.children) {
    const found = walk(child, predicate);
    if (found) return found;
  }
  return null;
}
const panel = new Element('div');
const head = new Element('head');
const snapshot = {
  backendId: 'default', revision: '0123456789abcdef',
  importCrf: 23, importPreset: 'medium',
  shrinkCrf: 27, shrinkPreset: 'medium',
  applies: 'next_job'
};
const calls = [];
const global = {
  VdrSuitePlatform: {
    getSelectedModule: () => 'settings',
    getSelectedBackendId: () => 'default'
  },
  VdrSuiteBrowserSession: {
    restore: () => Promise.resolve(),
    csrfHeaders: () => ({'X-CSRF-Token': 'test'})
  },
  fetch: (url, options) => {
    calls.push({url, options});
    return Promise.resolve({
      ok: true,
      text: () => Promise.resolve(JSON.stringify(snapshot))
    });
  }
};
const document = {
  documentElement: {lang: 'de'},
  readyState: 'complete',
  head,
  createElement: (tag) => new Element(tag),
  querySelector: (selector) => selector === '.settings-panel' ? panel : null,
  getElementById: (id) => walk(panel, (el) => el.id === id) ||
    walk(head, (el) => el.id === id)
};
vm.runInNewContext(source, {
  window: global,
  document,
  MutationObserver: class { observe() {} },
  Promise,
  console
});
function tick() { return new Promise((resolve) => setImmediate(resolve)); }
(async () => {
  await tick();
  const card = document.getElementById('media-tools-encoding-settings-card');
  assert.ok(card, 'encoding card mounted');
  assert.equal(calls.length, 1, 'opening the card issues one request only');
  assert.equal(calls[0].options.method, 'GET', 'no auto mutation on open');
  const save = walk(card, (el) => el.tag === 'button');
  assert.ok(save && !save.disabled);
  const selects = [];
  (function collect(el) {
    if (el.tag === 'select') selects.push(el);
    for (const child of el.children) collect(child);
  })(card);
  assert.equal(selects.length, 4);
  assert.equal(selects[0].value, '23');
  assert.equal(selects[2].value, '27');
  selects[0].value = '19';
  selects[1].value = 'slow';
  selects[2].value = '30';
  selects[3].value = 'fast';
  save.listeners.click();
  await tick();
  assert.equal(calls.length, 2);
  assert.equal(calls[1].options.method, 'POST');
  const posted = JSON.parse(calls[1].options.body);
  assert.deepEqual(posted, {
    backendId: 'default', revision: '0123456789abcdef',
    importCrf: 19, importPreset: 'slow',
    shrinkCrf: 30, shrinkPreset: 'fast'
  });
  assert.ok(calls[1].options.headers['X-CSRF-Token']);
  console.log('MT_ENC3_2_FRONTEND_GET_POST=PASS');
})().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
