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
    this.isConnected = true;
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
  const pendingId = 'dpr_' + 'a'.repeat(32);
  const pendingOther = 'dpr_' + 'b'.repeat(32);
  let pendingItems = [{
    pairingRequestId: pendingId, status: 'pending', expiresAt: '2026-10-25T06:15:00Z',
    client: {displayName: 'Samsung TV Wohnzimmer', clientKind: 'tizen', appVersion: 'S3'}
  }];
  let approveConfirmed = false;
  let deviceItems = [
    {deviceId: 'device_a', actorId: 'actor_a',
      displayName: 'Hisense Wohnzimmer', active: true, revoked: false},
    {deviceId: 'device_revoked', actorId: 'actor_revoked',
      displayName: 'Altes TV', active: false, revoked: true}
  ];
  const timers = [];
  const mocked = {
    listDevicePairingsForAdmin(options) {
      calls.push({kind:'pending-list', options});
      return Promise.resolve({items: pendingItems.slice(),
        page: {limit:25,hasMore:false,nextCursor:null}});
    },
    getDevicePairingForAdmin(options) {
      calls.push({kind:'pending-item', options});
      return Promise.resolve({etag:'"pair-revision-1"',
        data: pendingItems.find(p => p.pairingRequestId === options.pairingRequestId)});
    },
    decideDevicePairingForAdmin(options) {
      calls.push({kind:'pending-decision', options});
      pendingItems = pendingItems.filter(p => p.pairingRequestId !== options.pairingRequestId);
      return Promise.resolve({etag:'"pair-revision-2"',data:{
        pairingRequestId:options.pairingRequestId,status:options.decision==='approve'?'approved':'rejected'
      }});
    },
    getDevices(options) {
      calls.push({kind: 'list', options});
      return Promise.resolve({
        items: deviceItems.slice(),
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
            'channels.view', 'searchtimers.create', 'timers.create',
            'timers.modify', 'timers.delete',
            'searchtimers.modify', 'searchtimers.execute', 'searchtimers.delete'
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
    setTimeout(callback, ms) {
      assert.strictEqual(ms, 10000);
      timers.push(callback);
      return timers.length;
    },
    confirm() { confirmations++; return approveConfirmed; },
    VdrSuiteBrowserSession: {
      restore() { restores++; return Promise.resolve(); },
      csrfHeaders() { return {'X-CSRF-Token': 'test-browser-csrf'}; }
    },
    VdrSuitePublicV1Client: {createClient() { return mocked; }}
  };
  const context = {window, document: {
    visibilityState: 'visible',
    createElement(tag) { return new Element(tag); }
  }};
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
  // Fail before Browser Session restoration or transport dispatch.
  for (const permission of [
    'timers.create', 'timers.modify', 'timers.delete',
    'searchtimers.create', 'searchtimers.modify',
    'searchtimers.execute', 'searchtimers.delete'
  ]) {
    await assert.rejects(
      window.VdrSuiteDeviceAdminClientApi.setGrant(
        'device_a', '"grant-r1"', permission, '*', true),
      /Unsafe Device Grant mutation/);
  }
  for (const invalid of ['', '.', '..', 'backend/other']) {
    await assert.rejects(
      window.VdrSuiteDeviceAdminClientApi.setGrant(
        'device_a', '"grant-r1"', 'searchtimers.modify', invalid, true),
      /Unsafe Device Grant mutation/);
  }
  assert.strictEqual(calls.filter(x => x.kind === 'mutation').length, 0);
  const selects = descendants(page).filter(el => el.tagName === 'SELECT');
  assert.strictEqual(selects.length, 2);
  const grantable = selects[0].options.map(el => el.value);
  for (const permission of [
    'timers.modify', 'timers.delete', 'searchtimers.modify',
    'searchtimers.execute', 'searchtimers.delete'
  ]) {
    assert(grantable.includes(permission),
      'Server-supported Device grant missing from UI: ' + permission);
  }
  assert(!grantable.includes('role.admin'));
  selects[0].value = 'searchtimers.create';
  selects[1].value = 'default';
  const grant = named(page, 'button', 'Zugriff hinzufügen');
  approveConfirmed = true;
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

  // Real view composition: pending pairing must not be listed as a Device.
  assert(named(page, 'h3', 'Wartende Geräte'));
  assert(named(page, 'strong', 'Samsung TV Wohnzimmer'));
  assert.strictEqual(calls.filter(c=>c.kind==='pending-list').length, 1);
  assert.strictEqual(calls.filter(c=>c.kind==='pending-decision').length, 0);
  await assert.rejects(
    window.VdrSuiteDeviceAdminClientApi.decidePairing('../admin', '"etag"', 'approve'),
    /Invalid pairing decision/);
  await assert.rejects(
    window.VdrSuiteDeviceAdminClientApi.decidePairing(pendingId, '', 'approve'),
    /Invalid pairing decision/);
  await assert.rejects(
    window.VdrSuiteDeviceAdminClientApi.decidePairing(pendingId, '"etag"', 'grant'),
    /Invalid pairing decision/);
  assert.strictEqual(calls.filter(c=>c.kind==='pending-decision').length, 0);

  // Cancel is genuinely non-mutating even after a fresh GET.
  approveConfirmed = false;
  named(page, 'button', 'Genehmigen').click();
  await settle();
  assert.strictEqual(calls.filter(c=>c.kind==='pending-decision').length, 0);
  assert(named(page,'strong','Samsung TV Wohnzimmer'));

  // Approve: fresh admin GET -> human confirmation -> CSRF + If-Match POST.
  approveConfirmed = true;
  named(page, 'button', 'Genehmigen').click();
  await settle();
  const approves = calls.filter(c=>c.kind==='pending-decision');
  assert.strictEqual(approves.length,1);
  assert.strictEqual(approves[0].options.pairingRequestId,pendingId);
  assert.strictEqual(approves[0].options.ifMatch,'"pair-revision-1"');
  assert.strictEqual(approves[0].options.decision,'approve');
  assert.strictEqual(approves[0].options.credentials,'same-origin');
  assert.strictEqual(approves[0].options.headers['X-CSRF-Token'],'test-browser-csrf');
  assert(!('pairingToken' in approves[0].options));
  assert(!named(page,'strong','Samsung TV Wohnzimmer'),'decided request must disappear');

  // Reject uses exactly the same ETag and CSRF fences.
  pendingItems = [{
    pairingRequestId:pendingOther,status:'pending',expiresAt:'2026-10-25T06:20:00Z',
    client:{displayName:'Nicht vertrauenswürdiger TV',clientKind:'tizen',appVersion:'S3'}
  }];
  named(page,'button','Anfragen aktualisieren').click();
  await settle();
  assert(named(page,'strong','Nicht vertrauenswürdiger TV'));
  named(page,'button','Ablehnen').click();
  await settle();
  const decisions = calls.filter(c=>c.kind==='pending-decision');
  assert.strictEqual(decisions.length,2);
  assert.strictEqual(decisions[1].options.decision,'reject');
  assert.strictEqual(decisions[1].options.pairingRequestId,pendingOther);
  assert(!named(page,'strong','Nicht vertrauenswürdiger TV'));

  // S3 admin UX: auto-refresh sees pairing + paired TV without page reload.
  assert.strictEqual(timers.length, 1, 'one refresh timer for this settings card');
  deviceItems = deviceItems.concat({
    deviceId:'device_samsung',actorId:'actor_samsung',
    displayName:'Samsung Wohnzimmer',active:true,revoked:false
  });
  pendingItems = [{
    pairingRequestId: 'dpr_' + 'c'.repeat(32),
    status:'pending', expiresAt:'2026-10-25T09:00:00Z',
    client:{displayName:'Neuer Samsung TV',clientKind:'tizen',appVersion:'S3'}
  }];
  const listBefore = calls.filter(c => c.kind === 'list').length;
  const pendingBefore = calls.filter(c => c.kind === 'pending-list').length;
  timers.shift()();
  await settle();
  assert.strictEqual(calls.filter(c => c.kind === 'list').length, listBefore + 1);
  assert.strictEqual(calls.filter(c => c.kind === 'pending-list').length, pendingBefore + 1);
  assert(named(page,'button','Samsung Wohnzimmer'));
  assert(named(page,'strong','Neuer Samsung TV'));
  assert(named(page,'button','Hisense Wohnzimmer').attributes['aria-current'] === 'true',
    'keep selected device across list refresh');
  assert.strictEqual(timers.length, 1, 'never create concurrent refresh intervals');
  assert.strictEqual(calls.filter(c => c.kind === 'pending-decision').length, 2,
    'background refresh must never approve/reject');
  assert.strictEqual(calls.filter(c => c.kind === 'mutation').length, 2,
    'background refresh must not mutate grants');

  // Hidden page: no network traffic, no background refresh requests.
  const beforeHidden = calls.length;
  context.document.visibilityState = 'hidden';
  timers.shift()();
  await settle();
  assert.strictEqual(calls.length, beforeHidden);
  assert.strictEqual(timers.length, 1);
  context.document.visibilityState = 'visible';
  timers.shift()();
  await settle();
  assert.strictEqual(calls.filter(c => c.kind === 'pending-list').length, pendingBefore + 2);
  assert.strictEqual(timers.length, 1);

  // The app replaces the Settings card when navigating away. Stop forever.
  page.children[0].isConnected = false;
  const beforeDetached = calls.length;
  timers.shift()();
  await settle();
  assert.strictEqual(calls.length, beforeDetached);
  assert.strictEqual(timers.length, 0, 'detached Settings card must not leak timers');
}

run().catch(error => {console.error(error); process.exitCode = 1;});
