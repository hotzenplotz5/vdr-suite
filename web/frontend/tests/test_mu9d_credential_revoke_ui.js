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

assert(adapterSource.includes('getAccountCredential('));
assert(adapterSource.includes('revokeAccountCredential('));
assert(adapterSource.includes('VdrSuiteBrowserSession'));
assert(adapterSource.includes('csrfHeaders'));
assert(uiSource.includes('settings-account-admin-credential-revoke'));
assert(uiSource.includes('accountAdminRevokeCredentialConfirm'));
assert(uiSource.includes('credentialMutationErrorText'));
assert(!uiSource.includes('.innerHTML'));

const calls = [];
let credentialActive = true;
let mutationStatus = 200;
let confirmResult = true;
let restores = 0;
let credentialRead = 0;

const publicClient = {
  getAccounts() {
    return Promise.resolve({
      items: [{accountId: 'account-a', actorId: 'actor-a',
        displayName: 'Admin A', active: true}],
      page: {limit: 25, hasMore: false, nextCursor: null}
    });
  },
  getAccount() {
    return Promise.resolve({status: 200, etag: '"account-rev"', data: {
      accountId: 'account-a', actorId: 'actor-a',
      displayName: 'Admin A', active: true
    }});
  },
  getAccountGrants() {
    return Promise.resolve({status: 200, etag: '"grant-rev"', data: {items: []}});
  },
  getAccountCredentials() {
    return Promise.resolve({items: [
      {
        credentialId: 'credential-human-a',
        credentialType: 'human-password',
        active: credentialActive,
        expired: false,
        revoked: !credentialActive,
        expiresAt: '2099-01-01T00:00:00Z',
        createdAt: '2026-10-05T12:00:00Z'
      },
      {
        credentialId: 'credential-browser-a',
        credentialType: 'browser-session',
        active: true,
        expired: false,
        revoked: false,
        expiresAt: '2099-01-01T00:00:00Z',
        createdAt: '2026-10-05T12:00:00Z'
      }
    ]});
  },
  getAccountSessions() { return Promise.resolve({items: []}); },
  getAccountCredential(options) {
    credentialRead += 1;
    calls.push({name: 'getAccountCredential', options});
    return Promise.resolve({
      status: 200,
      etag: '"credential-rev-' + credentialRead + '"',
      data: {
        credentialId: 'credential-human-a',
        credentialType: 'human-password',
        active: true,
        expired: false,
        revoked: false
      }
    });
  },
  revokeAccountCredential(options) {
    calls.push({name: 'revokeAccountCredential', options});
    if (mutationStatus !== 200) return Promise.reject({status: mutationStatus});
    credentialActive = false;
    return Promise.resolve({
      status: 200,
      etag: '"credential-terminal"',
      data: {
        credentialId: 'credential-human-a',
        credentialType: 'human-password',
        active: false,
        expired: false,
        revoked: true
      }
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
    if ((current.className || '').split(/\s+/).includes(className)) found.push(current);
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
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-mu9d'}; }
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

(async function() {
  let root = await renderRoot();
  let revokeButtons = findAllByClass(root, 'settings-account-admin-credential-revoke');
  assert.strictEqual(revokeButtons.length, 1,
    'only the active human-password Credential may expose revoke control');

  await revokeButtons[0].listeners.click();

  const reads = calls.filter(call => call.name === 'getAccountCredential');
  const revokes = calls.filter(call => call.name === 'revokeAccountCredential');
  assert.strictEqual(reads.length, 1);
  assert.strictEqual(revokes.length, 1);
  assert.strictEqual(reads[0].options.accountId, 'account-a');
  assert.strictEqual(reads[0].options.credentialId, 'credential-human-a');
  assert.strictEqual(revokes[0].options.accountId, 'account-a');
  assert.strictEqual(revokes[0].options.credentialId, 'credential-human-a');
  assert.strictEqual(revokes[0].options.ifMatch, '"credential-rev-1"');
  assert.strictEqual(revokes[0].options.credentials, 'same-origin');
  assert.strictEqual(revokes[0].options.headers['X-CSRF-Token'], 'csrf-mu9d');
  assert(restores >= 1);
  assert.strictEqual(
    findByClass(root, 'settings-account-admin-credential-revoke'),
    null,
    'terminal Credential must no longer expose revoke control'
  );

  credentialActive = true;
  mutationStatus = 412;
  root = await renderRoot();
  let revoke = findByClass(root, 'settings-account-admin-credential-revoke');
  assert(revoke);
  const beforeStale =
    calls.filter(call => call.name === 'revokeAccountCredential').length;
  await revoke.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'revokeAccountCredential').length,
    beforeStale + 1,
    'stale Credential revoke must not retry implicitly'
  );
  let status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('zwischenzeitlich geändert'));

  mutationStatus = 409;
  root = await renderRoot();
  revoke = findByClass(root, 'settings-account-admin-credential-revoke');
  assert(revoke);
  const beforeFinalAdmin =
    calls.filter(call => call.name === 'revokeAccountCredential').length;
  await revoke.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'revokeAccountCredential').length,
    beforeFinalAdmin + 1,
    'final-admin rejection must not retry implicitly'
  );
  status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('kein nutzbarer Administrator'));

  mutationStatus = 200;
  confirmResult = false;
  root = await renderRoot();
  revoke = findByClass(root, 'settings-account-admin-credential-revoke');
  assert(revoke);
  const readsBeforeCancel =
    calls.filter(call => call.name === 'getAccountCredential').length;
  const revokesBeforeCancel =
    calls.filter(call => call.name === 'revokeAccountCredential').length;
  await revoke.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'getAccountCredential').length,
    readsBeforeCancel,
    'cancelled confirmation must not read Credential revision'
  );
  assert.strictEqual(
    calls.filter(call => call.name === 'revokeAccountCredential').length,
    revokesBeforeCancel,
    'cancelled confirmation must not mutate'
  );

  console.log('test_mu9d_credential_revoke_ui passed');
})().catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
