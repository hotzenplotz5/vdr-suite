'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const adapterSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'account-admin-client-api.js'), 'utf8'
);
const uiSource = fs.readFileSync(
  path.join(__dirname, '..', 'settings-account-admin.js'), 'utf8'
);

assert(adapterSource.includes('setAccountGrant('));
assert(adapterSource.includes('VdrSuiteBrowserSession'));
assert(adapterSource.includes('csrfHeaders'));
assert(uiSource.includes('settings-account-admin-grant-ensure'));
assert(uiSource.includes('settings-account-admin-grant-revoke'));
assert(uiSource.includes('grantMutationErrorText'));
assert(uiSource.includes('overview.grantsEtag'));
assert(!uiSource.includes('.innerHTML'));

const calls = [];
let grantItems = [
  {permission: 'channels.view', backendId: 'default'}
];
let grantRevision = 1;
let grantEtagAvailable = true;
let mutationStatus = 200;
let confirmResult = true;
let restores = 0;
const loadCounts = {account: 0, grants: 0, credentials: 0, sessions: 0};

function cloneItems() {
  return grantItems.map(item => Object.assign({}, item));
}

const publicClient = {
  getAccounts() {
    return Promise.resolve({
      items: [{accountId: 'account-a', actorId: 'actor-a',
        displayName: 'Admin A', active: true}],
      page: {limit: 25, hasMore: false, nextCursor: null}
    });
  },
  getAccount() {
    loadCounts.account += 1;
    return Promise.resolve({status: 200, etag: '"account-rev"', data: {
      accountId: 'account-a', actorId: 'actor-a',
      displayName: 'Admin A', active: true
    }});
  },
  getAccountGrants() {
    loadCounts.grants += 1;
    return Promise.resolve({
      status: 200,
      etag: grantEtagAvailable ? '"grant-rev-' + grantRevision + '"' : '',
      data: {items: cloneItems()}
    });
  },
  getAccountCredentials() {
    loadCounts.credentials += 1;
    return Promise.resolve({items: []});
  },
  getAccountSessions() {
    loadCounts.sessions += 1;
    return Promise.resolve({items: []});
  },
  setAccountGrant(options) {
    calls.push({name: 'setAccountGrant', options: Object.assign({}, options)});
    if (mutationStatus !== 200) return Promise.reject({status: mutationStatus});

    const index = grantItems.findIndex(item =>
      item.permission === options.permission && item.backendId === options.backendId);
    if (options.active && index < 0) {
      grantItems.push({permission: options.permission, backendId: options.backendId});
      grantRevision += 1;
    } else if (!options.active && index >= 0) {
      grantItems.splice(index, 1);
      grantRevision += 1;
    }
    return Promise.resolve({
      status: 200,
      etag: '"grant-rev-' + grantRevision + '"',
      data: {items: cloneItems()}
    });
  }
};

class Element {
  constructor(tag) {
    this.tagName = String(tag || '').toUpperCase();
    this.children = [];
    this.dataset = {};
    this.attributes = {};
    this.listeners = {};
    this.className = '';
    this.textContent = '';
    this.type = '';
    this.value = '';
    this.disabled = false;
    this.classList = {
      toggle: (name, enabled) => {
        const set = new Set(this.className.split(/\s+/).filter(Boolean));
        if (enabled) set.add(name); else set.delete(name);
        this.className = Array.from(set).join(' ');
      }
    };
  }
  appendChild(child) { this.children.push(child); return child; }
  replaceChildren(...children) { this.children = children; }
  setAttribute(name, value) { this.attributes[name] = String(value); }
  addEventListener(name, listener) { this.listeners[name] = listener; }
  querySelectorAll(selector) {
    const matches = [];
    function visit(node) {
      if (selector === 'button[data-account-id]' &&
          node.tagName === 'BUTTON' && node.dataset.accountId) {
        matches.push(node);
      }
      (node.children || []).forEach(visit);
    }
    visit(this);
    return matches;
  }
}

function findAllByClass(node, className) {
  const found = [];
  function visit(current) {
    if ((current.className || '').split(/\s+/).includes(className)) {
      found.push(current);
    }
    (current.children || []).forEach(visit);
  }
  visit(node);
  return found;
}

function findByClass(node, className) {
  return findAllByClass(node, className)[0] || null;
}

const context = {
  window: null,
  document: {createElement(tag) { return new Element(tag); }},
  console,
  location: {origin: 'https://suite.example'},
  confirm() { return confirmResult; },
  Promise, Object, Array, String, Set,
  VdrSuiteI18n: {t(key, parameters, fallback) { return fallback || key; }},
  VdrSuiteBrowserSession: {
    restore() { restores += 1; return Promise.resolve({authenticated: true}); },
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-mu9e'}; }
  },
  VdrSuitePublicV1Client: {
    createClient() { return publicClient; }
  }
};
context.window = context;
vm.createContext(context);
vm.runInContext(adapterSource, context);
vm.runInContext(uiSource, context);

async function renderRoot() {
  const root = new Element('section');
  await context.VdrSuiteAccountAdminSettings.render(root);
  return root;
}

function setEnsureFields(root, permission, backendId) {
  const permissionInput =
    findByClass(root, 'settings-account-admin-grant-permission-input');
  const backendInput =
    findByClass(root, 'settings-account-admin-grant-backend-input');
  assert(permissionInput);
  assert(backendInput);
  permissionInput.value = permission;
  backendInput.value = backendId;
  return findByClass(root, 'settings-account-admin-grant-ensure');
}

(async function() {
  let root = await renderRoot();
  assert.strictEqual(
    findAllByClass(root, 'settings-account-admin-grant-revoke').length, 1
  );

  const beforeEnsureLoads = Object.assign({}, loadCounts);
  let ensure = setEnsureFields(root, 'timers.view', 'default');
  assert(ensure);
  await ensure.listeners.click();

  let mutations = calls.filter(call => call.name === 'setAccountGrant');
  assert.strictEqual(mutations.length, 1);
  assert.deepStrictEqual(
    {
      accountId: mutations[0].options.accountId,
      permission: mutations[0].options.permission,
      backendId: mutations[0].options.backendId,
      active: mutations[0].options.active,
      ifMatch: mutations[0].options.ifMatch,
      csrf: mutations[0].options.headers['X-CSRF-Token'],
      credentials: mutations[0].options.credentials
    },
    {
      accountId: 'account-a',
      permission: 'timers.view',
      backendId: 'default',
      active: true,
      ifMatch: '"grant-rev-1"',
      csrf: 'csrf-mu9e',
      credentials: 'same-origin'
    },
    'grant-set ETag must fence ensure'
  );
  assert(loadCounts.account > beforeEnsureLoads.account);
  assert(loadCounts.grants > beforeEnsureLoads.grants);
  assert(loadCounts.credentials > beforeEnsureLoads.credentials);
  assert(loadCounts.sessions > beforeEnsureLoads.sessions);
  assert(restores >= 1);

  let revoke = findByClass(root, 'settings-account-admin-grant-revoke');
  assert(revoke);
  await revoke.listeners.click();
  mutations = calls.filter(call => call.name === 'setAccountGrant');
  assert.strictEqual(mutations.length, 2);
  assert.strictEqual(mutations[1].options.active, false);
  assert.strictEqual(mutations[1].options.ifMatch, '"grant-rev-2"');

  mutationStatus = 412;
  ensure = setEnsureFields(root, 'recordings.view', 'default');
  const beforeStale = calls.filter(call => call.name === 'setAccountGrant').length;
  await ensure.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'setAccountGrant').length,
    beforeStale + 1,
    'stale Grant mutation must not retry implicitly'
  );
  let status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('zwischenzeitlich geändert'));

  mutationStatus = 422;
  ensure = setEnsureFields(root, 'unsupported.permission', 'default');
  const beforeUnsupported =
    calls.filter(call => call.name === 'setAccountGrant').length;
  await ensure.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'setAccountGrant').length,
    beforeUnsupported + 1,
    'unsupported Grant mutation must not retry implicitly'
  );
  status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('nicht unterstützt'));

  mutationStatus = 409;
  revoke = findByClass(root, 'settings-account-admin-grant-revoke');
  assert(revoke);
  const beforeFinalAdmin =
    calls.filter(call => call.name === 'setAccountGrant').length;
  await revoke.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'setAccountGrant').length,
    beforeFinalAdmin + 1,
    'final-admin Grant rejection must not retry implicitly'
  );
  status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('kein nutzbarer Administrator'));

  mutationStatus = 403;
  ensure = setEnsureFields(root, 'epg.view', 'default');
  await ensure.listeners.click();
  status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('globale Administrationsberechtigung'));

  mutationStatus = 401;
  ensure = setEnsureFields(root, 'epg.view', 'default');
  await ensure.listeners.click();
  status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('Browser-Anmeldung'));

  mutationStatus = 200;
  grantEtagAvailable = false;
  root = await renderRoot();
  ensure = setEnsureFields(root, 'epg.view', 'default');
  const beforeMissingRevision =
    calls.filter(call => call.name === 'setAccountGrant').length;
  await ensure.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'setAccountGrant').length,
    beforeMissingRevision,
    'missing grant-set ETag must fail before mutation'
  );
  status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('aktueller Berechtigungsstand'));

  grantEtagAvailable = true;
  confirmResult = false;
  root = await renderRoot();
  revoke = findByClass(root, 'settings-account-admin-grant-revoke');
  assert(revoke);
  const beforeCancel = calls.filter(call => call.name === 'setAccountGrant').length;
  await revoke.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'setAccountGrant').length,
    beforeCancel,
    'cancelled Grant revoke must not mutate'
  );

  ensure = setEnsureFields(root, '   ', 'default');
  const beforeBlank = calls.filter(call => call.name === 'setAccountGrant').length;
  await ensure.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'setAccountGrant').length,
    beforeBlank,
    'blank Grant tuple must fail before mutation'
  );

  console.log('test_mu9e_grant_mutation_ui passed');
})().catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
