'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const frontend = path.join(__dirname, '..');
const index = fs.readFileSync(path.join(frontend, 'index.html'), 'utf8');
const app = fs.readFileSync(path.join(frontend, 'app.js'), 'utf8');
const view = fs.readFileSync(path.join(frontend, 'modules/media-import.js'), 'utf8');
const clientFactory = require('../api/public-v1-client.js');

assert(index.includes('data-module="media-import" hidden disabled'));
assert(index.indexOf('modules/media-import.js') < index.indexOf('frontend/app.js'));
assert(app.includes('refreshMediaImportVisibility(backendId);'));
assert(app.includes('resetMediaImportVisibility(true);'));
assert(!view.includes('.innerHTML'));
assert(!view.includes('localStorage'));
assert(!view.includes('sessionStorage'));

class Element {
  constructor(name) {
    this.tagName = name.toUpperCase();
    this.children = [];
    this.listeners = {};
    this.attributes = {};
    this.dataset = {};
    this.textContent = '';
    this.className = '';
    this.hidden = false;
    this.disabled = false;
    this.value = '';
    this.classList = {toggle() {}};
  }
  appendChild(child) { this.children.push(child); return child; }
  replaceChildren(...children) { this.children = children; }
  addEventListener(name, callback) { this.listeners[name] = callback; }
  setAttribute(name, value) { this.attributes[name] = value; }
}
function findClass(node, cls) {
  if ((node.className || '').split(/\s+/).includes(cls)) return node;
  for (const child of node.children || []) {
    const found = findClass(child, cls);
    if (found) return found;
  }
  return null;
}
function flush() {
  return new Promise(function(resolve) { setImmediate(resolve); });
}
async function settle() {
  for (let i = 0; i < 14; i++) await flush();
}
let allow = true;
let sourcesStatus = 200;
let state = 'dispatching';
let submitMode = 'accepted';
let operationReads = 0;
let submits = 0;
let counter = 0;
const requests = [];
let sessionRestores = 0;
const SOURCE = 'source_0123456789abcde';
const OP = 'op_12345678901234567890123456789012';
function reply(status, value, headers) {
  return {
    ok: status >= 200 && status < 300,
    status: status,
    headers: {get(name) { return (headers || {})[name] || null; }},
    text() { return Promise.resolve(JSON.stringify(value)); }
  };
}
function transport(url, options) {
  const path = String(url).replace(/^https?:\/\/[^/]+/, '');
  requests.push({path, options});
  if (path.endsWith('/capabilities/media-import')) {
    return Promise.resolve(reply(200, {
      backendId: 'backend-a', capability: 'addons.media.import', available: allow
    }));
  }
  if (path.endsWith('/media-import-sources')) {
    return Promise.resolve(sourcesStatus === 200
      ? reply(200, {items: [{sourceId: SOURCE}], meta: {partial: false}})
      : reply(sourcesStatus, {detail: 'Source unavailable'}));
  }
  if (path.endsWith('/media-imports')) {
    ++submits;
    if (submitMode === 'uncertain') return Promise.reject(new Error('lost response'));
    return Promise.resolve(reply(202, {operationId: OP, backendId: 'backend-a',
      state: 'dispatching'}, {'Location': '/api/v1/operations/' + OP}));
  }
  if (path === '/api/v1/operations/' + OP) {
    ++operationReads;
    return Promise.resolve(reply(200, {operationId: OP, backendId: 'backend-a',
      state: state, links: {self: path}}, {'ETag': '"revision-1"'}));
  }
  throw new Error('unexpected route: ' + path);
}
const tab = new Element('button');
tab.dataset.module = 'media-import';
tab.hidden = true;
tab.disabled = true;
const mount = new Element('section');
const timerCallbacks = [];
const modules = {};
const context = {
  console, Promise, Uint8Array, setImmediate,
  document: {
    createElement(name) { return new Element(name); },
    querySelector(sel) {
      return sel === '.module-tab[data-module="media-import"]' ? tab : null;
    },
    querySelectorAll(sel) {
      return sel === '.module-tab' ? [tab] : [];
    }
  },
  crypto: {
    getRandomValues(bytes) {
      ++counter;
      bytes.fill(counter);
      return bytes;
    }
  },
  location: {origin: 'https://suite.test'},
  setTimeout(fn) { timerCallbacks.push(fn); return timerCallbacks.length; },
  clearTimeout() {},
  VdrSuitePlatform: {
    registerModule(name, api) { modules[name] = api; },
    getModule(name) { return modules[name] || null; }
  },
  VdrSuitePublicV1Client: {
    createClient() {
      return clientFactory.createClient({fetch: transport, credentials: 'same-origin'});
    }
  },
  VdrSuiteBrowserSession: {
    restore() { ++sessionRestores; return Promise.resolve({authenticated: true}); },
    csrfHeaders() { return {'X-CSRF-Token': 'test-csrf'}; }
  }
};
context.window = context;
vm.createContext(context);
vm.runInContext(view, context);

function slice(first, last) {
  const start = app.indexOf(first);
  const end = app.indexOf(last, start);
  assert(start >= 0 && end > start, 'app composition anchor missing: ' + first);
  return app.slice(start, end);
}
const gateCode = slice("let selectedModule = 'overview';", "let homeResumeBackendId = '';");
const renderCode = slice('function renderSelectedModule(data) {',
  'function publishHomeResume(previousModule) {');
const selectionCode = slice('function selectModule(moduleName) {',
  'function markSelected(backendId) {');
vm.runInContext(gateCode + '\n' + renderCode + '\n' + selectionCode, context);
vm.runInContext(`
let selectedBackendId = 'backend-a';
let currentSnapshot = {};
let detailDataElement = window.testMount;
let browserSessionFrontendActive = true;
let windowUnused = 0;
function frontendPlatformModule(name, legacy) {
  return window.VdrSuitePlatform.getModule(name) || legacy;
}
function selectedEpgBackendId() { return selectedBackendId; }
function renderSnapshotMetrics() {}
function shouldPublishHomeResume() { return false; }
function publishHomeResume() {}
`, Object.assign(context, {testMount: mount}));

(async function () {
  // Real app capability-gate -> production module selection -> actual DOM buttons.
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  assert.strictEqual(tab.hidden, false);
  context.selectModule('media-import');
  await settle();
  let select = findClass(mount, 'media-import-source');
  let start = findClass(mount, 'media-import-start');
  assert(select && start, 'production detail owner must mount real controls');
  assert.strictEqual(start.disabled, true);
  select.value = SOURCE;
  select.listeners.change();
  assert.strictEqual(start.disabled, false);
  await start.listeners.click();
  await settle();
  assert.strictEqual(submits, 1);
  const post = requests.find(x => x.options.method === 'POST');
  assert(post, 'user-style click must dispatch canonical v1 request');
  assert.strictEqual(post.path, '/api/v1/backends/backend-a/media-imports');
  assert.strictEqual(post.options.body, JSON.stringify({sourceId: SOURCE}));
  assert.strictEqual(post.options.headers['X-CSRF-Token'], 'test-csrf');
  assert(post.options.headers['Idempotency-Key'].startsWith('media-import-'));
  assert(sessionRestores >= 1);
  assert(operationReads >= 1);
  assert(findClass(mount, 'media-import-operation').textContent.includes(OP));
  assert.strictEqual(start.disabled, true);
  await start.listeners.click();
  assert.strictEqual(submits, 1, 'double-click must not submit a second import');

  // Owner transition must not delete Core operation tracking.
  context.selectModule('overview');
  context.selectModule('media-import');
  await settle();
  assert(findClass(mount, 'media-import-operation').textContent.includes(OP));
  assert.strictEqual(submits, 1);
  state = 'outcome_unknown';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes('ungewiss'));
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, true);
  state = 'succeeded';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes('unabhängig bestätigt'));
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, false);
  await findClass(mount, 'media-import-new').listeners.click();
  await settle();

  // A new explicit user action gets its own random key, no retry path.
  select = findClass(mount, 'media-import-source');
  select.value = SOURCE;
  select.listeners.change();
  submitMode = 'uncertain';
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, 2);
  assert(findClass(mount, 'status').textContent.includes('ungewiss'));
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true);
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, true);
  const posts = requests.filter(x => x.options.method === 'POST');
  assert.notStrictEqual(posts[0].options.headers['Idempotency-Key'],
    posts[1].options.headers['Idempotency-Key']);

  // Login/out and permissions must hide and invalidate the production module.
  context.resetMediaImportVisibility(true);
  assert.strictEqual(tab.hidden, true);
  allow = false;
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), false);
  assert.strictEqual(tab.hidden, true);
  context.selectModule('media-import');
  assert.strictEqual(tab.hidden, true);
  allow = true;
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  sourcesStatus = 503;
  await settle();
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes('abgelaufen'));
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true);
  console.log('A7.5 real web composition and media import lifecycle: PASS');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
