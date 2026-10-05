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

assert(adapterSource.includes('updateAccountDisplayName('));
assert(adapterSource.includes('activateAccount('));
assert(adapterSource.includes('deactivateAccount('));
assert(adapterSource.includes('VdrSuiteBrowserSession'));
assert(adapterSource.includes('csrfHeaders'));
assert(adapterSource.includes('ifMatch'));
assert(uiSource.includes('settings-account-admin-display-name-input'));
assert(uiSource.includes('settings-account-admin-toggle-active'));
assert(uiSource.includes('accountAdminDeactivateConfirm'));
assert(!adapterSource.includes('createAccount('));
assert(!uiSource.includes('.innerHTML'));

const calls = [];
let getAccountCount = 0;
const publicClient = {
  getAccounts() {
    calls.push({name: 'getAccounts'});
    return Promise.resolve({
      items: [{accountId: 'account-a', actorId: 'actor-a',
        displayName: 'Admin A', active: true}],
      page: {limit: 25, hasMore: false, nextCursor: null}
    });
  },
  getAccount(options) {
    getAccountCount += 1;
    calls.push({name: 'getAccount', options});
    return Promise.resolve({
      status: 200,
      etag: getAccountCount === 1 ? '"rev-1"' : '"rev-' + getAccountCount + '"',
      data: {
        accountId: 'account-a',
        actorId: 'actor-a',
        displayName: getAccountCount === 1 ? 'Admin A' : 'Admin B',
        active: getAccountCount !== 3
      }
    });
  },
  getAccountGrants() {
    return Promise.resolve({status: 200, etag: '"grant-a"', data: {items: []}});
  },
  getAccountCredentials() { return Promise.resolve({items: []}); },
  getAccountSessions() { return Promise.resolve({items: []}); },
  updateAccountDisplayName(options) {
    calls.push({name: 'updateAccountDisplayName', options});
    return Promise.resolve({status: 200, etag: '"rev-2"', data: {
      accountId: 'account-a', actorId: 'actor-a', displayName: options.displayName,
      active: true
    }});
  },
  activateAccount(options) {
    calls.push({name: 'activateAccount', options});
    return Promise.resolve({status: 200, etag: '"rev-active"', data: {}});
  },
  deactivateAccount(options) {
    calls.push({name: 'deactivateAccount', options});
    return Promise.resolve({status: 200, etag: '"rev-inactive"', data: {}});
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

function findByClass(node, className) {
  if ((node.className || '').split(/\s+/).includes(className)) return node;
  for (const child of node.children || []) {
    const found = findByClass(child, className);
    if (found) return found;
  }
  return null;
}

let restores = 0;
const context = {
  window: null,
  document: {createElement(tag) { return new Element(tag); }},
  console,
  location: {origin: 'https://suite.example'},
  confirm() { return true; },
  Promise, Object, Array, String, Set,
  VdrSuiteI18n: {t(key, parameters, fallback) { return fallback || key; }},
  VdrSuiteBrowserSession: {
    restore() { restores += 1; return Promise.resolve({authenticated: true}); },
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-mu9b'}; }
  },
  VdrSuitePublicV1Client: {
    createClient() { return publicClient; }
  }
};
context.window = context;
vm.createContext(context);
vm.runInContext(adapterSource, context);
vm.runInContext(uiSource, context);

(async function() {
  const root = new Element('section');
  await context.VdrSuiteAccountAdminSettings.render(root);

  const input = findByClass(root, 'settings-account-admin-display-name-input');
  const save = findByClass(root, 'settings-account-admin-save-name');
  assert(input && save);
  input.value = 'Admin B';
  await save.listeners.click();

  const rename = calls.find(call => call.name === 'updateAccountDisplayName');
  assert(rename);
  assert.strictEqual(rename.options.accountId, 'account-a');
  assert.strictEqual(rename.options.ifMatch, '"rev-1"');
  assert.strictEqual(rename.options.displayName, 'Admin B');
  assert.strictEqual(rename.options.credentials, 'same-origin');
  assert.strictEqual(rename.options.headers['X-CSRF-Token'], 'csrf-mu9b');
  assert.strictEqual(calls.filter(call => call.name === 'updateAccountDisplayName').length, 1);
  assert(restores >= 1);

  const toggle = findByClass(root, 'settings-account-admin-toggle-active');
  assert(toggle);
  await toggle.listeners.click();

  const deactivate = calls.find(call => call.name === 'deactivateAccount');
  assert(deactivate);
  assert.strictEqual(deactivate.options.accountId, 'account-a');
  assert.strictEqual(deactivate.options.ifMatch, '"rev-2"');
  assert.strictEqual(deactivate.options.headers['X-CSRF-Token'], 'csrf-mu9b');
  assert.strictEqual(calls.filter(call => call.name === 'deactivateAccount').length, 1);

  const activateToggle = findByClass(root, 'settings-account-admin-toggle-active');
  assert(activateToggle);
  await activateToggle.listeners.click();

  const activate = calls.find(call => call.name === 'activateAccount');
  assert(activate);
  assert.strictEqual(activate.options.accountId, 'account-a');
  assert.strictEqual(activate.options.ifMatch, '"rev-3"');
  assert.strictEqual(activate.options.headers['X-CSRF-Token'], 'csrf-mu9b');
  assert.strictEqual(calls.filter(call => call.name === 'activateAccount').length, 1);
  assert(restores >= 3);

  const readsBeforeConflict = getAccountCount;
  publicClient.updateAccountDisplayName = function(options) {
    calls.push({name: 'updateAccountDisplayNameConflict', options});
    return Promise.reject({status: 412});
  };
  const conflictInput = findByClass(root, 'settings-account-admin-display-name-input');
  const conflictSave = findByClass(root, 'settings-account-admin-save-name');
  assert(conflictInput && conflictSave);
  conflictInput.value = 'Admin C';
  await conflictSave.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'updateAccountDisplayNameConflict').length, 1);
  assert.strictEqual(getAccountCount, readsBeforeConflict + 1);
  const conflictStatus = findByClass(root, 'settings-account-admin-status');
  assert(conflictStatus.textContent.includes('zwischenzeitlich geändert'));

  const readsBeforeFinalAdmin = getAccountCount;
  publicClient.deactivateAccount = function(options) {
    calls.push({name: 'deactivateAccountFinalAdmin', options});
    return Promise.reject({status: 409});
  };
  const finalAdminToggle = findByClass(root, 'settings-account-admin-toggle-active');
  assert(finalAdminToggle);
  await finalAdminToggle.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'deactivateAccountFinalAdmin').length, 1);
  assert.strictEqual(getAccountCount, readsBeforeFinalAdmin + 1);
  const finalAdminStatus = findByClass(root, 'settings-account-admin-status');
  assert(finalAdminStatus.textContent.includes('letzte nutzbare Administrator'));

  console.log('test_mu9b_account_lifecycle_mutation_ui passed');
})().catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
