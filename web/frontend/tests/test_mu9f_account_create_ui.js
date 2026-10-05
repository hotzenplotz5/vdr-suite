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

assert(adapterSource.includes('createAccount('));
assert(adapterSource.includes('VdrSuiteBrowserSession'));
assert(adapterSource.includes('csrfHeaders'));
assert(uiSource.includes('settings-account-admin-create-submit'));
assert(uiSource.includes('createAccountIdempotencyKey'));
assert(uiSource.includes("createPasswordInput.value = ''"));
assert(uiSource.includes('createAccountErrorText'));
assert(!uiSource.includes('.innerHTML'));
assert(!uiSource.includes('localStorage'));
assert(!uiSource.includes('sessionStorage'));

const calls = [];
let accounts = [
  {accountId: 'account-a', actorId: 'actor-a', displayName: 'Admin A', active: true}
];
let createStatus = 201;
let createCode = '';
let listLoads = 0;
let restores = 0;
let uuidCounter = 0;

function accountById(accountId) {
  return accounts.find(account => account.accountId === accountId) || accounts[0];
}

const publicClient = {
  getAccounts() {
    listLoads += 1;
    return Promise.resolve({
      items: accounts.map(account => Object.assign({}, account)),
      page: {limit: 25, hasMore: false, nextCursor: null}
    });
  },
  getAccount(options) {
    const account = accountById(options.accountId);
    return Promise.resolve({
      status: 200,
      etag: '"rev-' + account.accountId + '"',
      data: Object.assign({}, account)
    });
  },
  getAccountGrants() {
    return Promise.resolve({status: 200, etag: '"grant-rev"', data: {items: []}});
  },
  getAccountCredentials() { return Promise.resolve({items: []}); },
  getAccountSessions() { return Promise.resolve({items: []}); },
  createAccount(options) {
    calls.push({name: 'createAccount', options: Object.assign({}, options)});
    if (createStatus !== 201) {
      return Promise.reject({status: createStatus, code: createCode});
    }
    const created = {
      accountId: 'account-created-' + calls.filter(call => call.name === 'createAccount').length,
      actorId: 'actor-created',
      displayName: options.displayName,
      active: true
    };
    accounts.push(created);
    return Promise.resolve({
      status: 201,
      etag: '"created-rev"',
      location: '/api/v1/accounts/' + created.accountId,
      data: Object.assign({}, created)
    });
  },
  setAccountGrant(options) {
    calls.push({name: 'setAccountGrant', options});
    return Promise.resolve({status: 200});
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
  crypto: {
    randomUUID() {
      uuidCounter += 1;
      return '00000000-0000-4000-8000-' + String(uuidCounter).padStart(12, '0');
    }
  },
  confirm() { return true; },
  Promise, Object, Array, String, Set,
  VdrSuiteI18n: {t(key, parameters, fallback) { return fallback || key; }},
  VdrSuiteBrowserSession: {
    restore() { restores += 1; return Promise.resolve({authenticated: true}); },
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-mu9f'}; }
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

function fillCreate(root, loginName, displayName, password) {
  const login = findByClass(root, 'settings-account-admin-create-login');
  const display = findByClass(root, 'settings-account-admin-create-display-name');
  const passwordInput = findByClass(root, 'settings-account-admin-create-password');
  const submit = findByClass(root, 'settings-account-admin-create-submit');
  assert(login && display && passwordInput && submit);
  login.value = loginName;
  display.value = displayName;
  passwordInput.value = password;
  return {login, display, passwordInput, submit};
}

(async function() {
  const root = await renderRoot();
  const initialListLoads = listLoads;
  let form = fillCreate(root, 'viewer', 'Viewer', 'pw-one');
  await form.submit.listeners.click();

  let creates = calls.filter(call => call.name === 'createAccount');
  assert.strictEqual(creates.length, 1, 'one user action must emit one Account CREATE');
  assert.deepStrictEqual(
    {
      loginName: creates[0].options.loginName,
      displayName: creates[0].options.displayName,
      password: creates[0].options.password,
      idempotencyKey: creates[0].options.idempotencyKey,
      csrf: creates[0].options.headers['X-CSRF-Token'],
      credentials: creates[0].options.credentials
    },
    {
      loginName: 'viewer',
      displayName: 'Viewer',
      password: 'pw-one',
      idempotencyKey: 'mu9f-00000000-0000-4000-8000-000000000001',
      csrf: 'csrf-mu9f',
      credentials: 'same-origin'
    },
    'Account CREATE must reuse the canonical browser/Public-v1 options'
  );
  assert.strictEqual(form.passwordInput.value, '',
    'password input must be cleared when CREATE is dispatched');
  assert(listLoads > initialListLoads,
    'successful Account CREATE must refresh the Account list');
  assert(restores >= 1);
  assert.strictEqual(calls.filter(call => call.name === 'setAccountGrant').length, 0,
    'Account CREATE must not assign grants automatically');

  const cases = [
    {status: 401, code: '', expected: 'Browser-Anmeldung'},
    {status: 403, code: '', expected: 'globale Administrationsberechtigung'},
    {status: 409, code: 'operation_conflict', expected: 'Loginname'},
    {status: 409, code: 'idempotency_conflict', expected: 'widersprüchlichen Daten'},
    {status: 400, code: '', expected: 'Anfrage ist ungültig'},
    {status: 415, code: '', expected: 'Anfrage ist ungültig'},
    {status: 422, code: '', expected: 'ungültig'},
    {status: 503, code: '', expected: 'Sicherheitskomponente'}
  ];

  for (const entry of cases) {
    createStatus = entry.status;
    createCode = entry.code;
    form = fillCreate(root, 'user-' + entry.status + '-' + entry.code, 'User', 'pw-error');
    const before = calls.filter(call => call.name === 'createAccount').length;
    await form.submit.listeners.click();
    const after = calls.filter(call => call.name === 'createAccount').length;
    assert.strictEqual(after, before + 1,
      'failed Account CREATE must not retry implicitly: ' + entry.status + '/' + entry.code);
    const status = findByClass(root, 'settings-account-admin-status');
    assert(status.textContent.includes(entry.expected),
      'expected error text for ' + entry.status + '/' + entry.code);
    assert.strictEqual(form.passwordInput.value, '',
      'failed dispatched CREATE must still clear the password field');
  }

  creates = calls.filter(call => call.name === 'createAccount');
  assert.strictEqual(new Set(creates.map(call => call.options.idempotencyKey)).size,
    creates.length, 'each explicit create action must own a fresh Idempotency-Key');

  createStatus = 201;
  createCode = '';
  form = fillCreate(root, '   ', 'Viewer', 'pw');
  let before = calls.filter(call => call.name === 'createAccount').length;
  await form.submit.listeners.click();
  assert.strictEqual(calls.filter(call => call.name === 'createAccount').length, before,
    'blank Account CREATE fields must fail before mutation');
  let status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('dürfen nicht leer sein'));

  const savedCrypto = context.crypto;
  context.crypto = {};
  form = fillCreate(root, 'safe-user', 'Safe User', 'pw');
  before = calls.filter(call => call.name === 'createAccount').length;
  await form.submit.listeners.click();
  assert.strictEqual(calls.filter(call => call.name === 'createAccount').length, before,
    'missing secure Idempotency-Key source must fail before mutation');
  status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('nicht sicher verfügbar'));
  context.crypto = savedCrypto;

  console.log('test_mu9f_account_create_ui passed');
})().catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
