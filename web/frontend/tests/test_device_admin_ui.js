'use strict';
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const root = path.join(__dirname, '..');
const adapter = fs.readFileSync(path.join(root, 'api/device-admin-client-api.js'), 'utf8');
const ui = fs.readFileSync(path.join(root, 'settings-device-admin.js'), 'utf8');
const shell = fs.readFileSync(path.join(root, 'index.html'), 'utf8');
const app = fs.readFileSync(path.join(root, 'app.js'), 'utf8');
assert(!ui.includes('.innerHTML'), 'Device identifiers are never HTML');
assert(!adapter.includes('credentialSecret'));
assert(shell.includes('api/device-admin-client-api.js'));
assert(shell.includes('settings-device-admin.js'));
assert(app.includes('deviceAdminSettings.render(panel)'));
assert(app.includes('window.VdrSuiteDeviceAdminSettings'));

class Element {
  constructor(tag) {
    this.tagName = tag.toUpperCase();
    this.children = [];
    this.listeners = {};
    this.attributes = {};
    this.dataset = {};
    this.className = '';
    this.textContent = '';
    this.value = '';
    this.disabled = false;
    this.type = '';
  }
  appendChild(item) {
    this.children.push(item);
    if (this.tagName === 'SELECT' && this.children.length === 1)
      this.value = item.value;
    return item;
  }
  replaceChildren(...items) { this.children = items; }
  setAttribute(key, val) { this.attributes[key] = val; }
  addEventListener(key, fn) { this.listeners[key] = fn; }
  get options() { return this.children; }
  click() { if (this.listeners.click) return this.listeners.click(); }
  querySelectorAll(query) {
    const all = [];
    function visit(node) {
      node.children.forEach(child => {
        if (query === 'button[data-device-id]' &&
            child.tagName === 'BUTTON' && child.dataset.deviceId) all.push(child);
        visit(child);
      });
    }
    visit(this);
    return all;
  }
}
function descendants(node) {
  const values = [node];
  for (const child of node.children) values.push(...descendants(child));
  return values;
}
function named(node, tag, label) {
  return descendants(node).find(x =>
    x.tagName === tag.toUpperCase() && x.textContent === label);
}
async function settle() {
  for (let i = 0; i < 4; i++) await Promise.resolve();
  await new Promise(setImmediate);
}

async function run() {
  const calls = [];
  let revision = 1;
  let rejectStale = false;
  let confirmations = 0;
  let restores = 0;
  const mocked = {
    getDevices(options) {
      calls.push({kind: 'list', options});
      return Promise.resolve({
        items: [
          {deviceId: 'device_a', actorId: 'actor_a',
            displayName: 'Hisense Wohnzimmer', active: true, revoked: false},
          {deviceId: 'device_revoked', actorId: 'actor_revoked',
            displayName: 'Altes TV', active: false, revoked: true}
        ],
        page: {limit: 25, hasMore: false, nextCursor: null}
      });
    },
    getBackends() {
      return Promise.resolve({items: [
        {backendId: 'default', name: 'yaVDR'},
        {backendId: 'second', name: 'Zweitgerät'}
      ]});
    },
    getDeviceGrants(options) {
      calls.push({kind: 'grants', options});
      return Promise.resolve({
        etag: '"grant-r' + revision + '"',
        data: {
          deviceId: options.deviceId,
          actorId: 'actor_a',
          items: [],
          supportedPermissions: [
            'channels.view', 'searchtimers.create', 'timers.create'
          ]
        }
      });
    },
    setDeviceGrant(options) {
      calls.push({kind: 'mutation', options});
      if (rejectStale) return Promise.reject({status: 412});
      revision++;
      return Promise.resolve({
        etag: '"grant-r' + revision + '"',
        data: {deviceId: options.deviceId, items: []}
      });
    }
  };
  const window = {
    location: {origin: 'https://suite.example'},
    confirm() { confirmations++; return true; },
    VdrSuiteBrowserSession: {
      restore() { restores++; return Promise.resolve(); },
      csrfHeaders() { return {'X-CSRF-Token': 'test-browser-csrf'}; }
    },
    VdrSuitePublicV1Client: {createClient() { return mocked; }}
  };
  const context = {window, document: {createElement(tag) {
    return new Element(tag);
  }}};
  vm.runInNewContext(adapter, context);
  vm.runInNewContext(ui, context);
  const page = new Element('main');
  await window.VdrSuiteDeviceAdminSettings.render(page);
  await settle();
  assert(named(page, 'h3', 'Gekoppelte Geräte (TV & Apps)'));
  const hisense = named(page, 'button', 'Hisense Wohnzimmer');
  assert(hisense && hisense.dataset.deviceId === 'device_a');
  assert(named(page, 'summary', 'Gerätekennung (technisch)'));
  assert(named(page, 'p', 'Device-ID: device_a'));
  const selects = descendants(page).filter(el => el.tagName === 'SELECT');
  assert.strictEqual(selects.length, 2);
  selects[0].value = 'searchtimers.create';
  selects[1].value = 'default';
  const grant = named(page, 'button', 'Zugriff hinzufügen');
  grant.click();
  await settle();
  const changed = calls.filter(x => x.kind === 'mutation');
  assert.strictEqual(changed.length, 1);
  assert.strictEqual(changed[0].options.permission, 'searchtimers.create');
  assert.strictEqual(changed[0].options.backendId, 'default');
  assert.strictEqual(changed[0].options.ifMatch, '"grant-r1"');
  assert.strictEqual(changed[0].options.headers['X-CSRF-Token'],
    'test-browser-csrf');
  assert.strictEqual(changed[0].options.credentials, 'same-origin');
  assert.strictEqual(confirmations, 1);
  assert.strictEqual(restores, 1);

  rejectStale = true;
  const after = descendants(page).filter(el => el.tagName === 'SELECT');
  after[0].value = 'timers.create';
  after[1].value = 'default';
  named(page, 'button', 'Zugriff hinzufügen').click();
  await settle();
  assert.strictEqual(calls.filter(x => x.kind === 'mutation').length, 2,
    '412 must not be retried automatically');
  assert(calls.filter(x => x.kind === 'grants').length >= 3,
    '412 requires a fresh ETag read');
  const oldTv = named(page, 'button', 'Altes TV');
  assert(oldTv);
  oldTv.click();
  await settle();
  assert(!named(page, 'button', 'Zugriff hinzufügen'),
    'Revoked devices have no mutation controls');
}
run().catch(error => {console.error(error); process.exitCode = 1;});
