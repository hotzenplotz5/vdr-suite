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

assert(adapterSource.includes('getAccountSession('));
assert(adapterSource.includes('revokeAccountSession('));
assert(adapterSource.includes('VdrSuiteBrowserSession'));
assert(adapterSource.includes('csrfHeaders'));
assert(uiSource.includes('settings-account-admin-session-revoke'));
assert(uiSource.includes('accountAdminRevokeSessionConfirm'));
assert(uiSource.includes('sessionMutationErrorText'));
assert(uiSource.includes('settings-account-admin-session-history'));
assert(uiSource.includes('historicalSessions'));
assert(!uiSource.includes('renderItems(sessions, overview.sessions'));
assert(!uiSource.includes('.innerHTML'));

const calls = [];
let sessionActive = true;
let conflict = false;
let confirmResult = true;
let restores = 0;
let sessionRead = 0;

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
  getAccountCredentials() { return Promise.resolve({items: []}); },
  getAccountSessions() {
    return Promise.resolve({items: [{
      sessionId: 'session-a',
      deviceId: 'living-room',
      issuedFromCredentialId: 'credential-a',
      active: sessionActive,
      expired: false,
      revoked: !sessionActive,
      lastSeenAt: '2026-10-05T12:00:00Z',
      expiresAt: '2026-10-06T12:00:00Z'
    }, {
      sessionId: 'session-old',
      deviceId: 'old-browser',
      issuedFromCredentialId: 'credential-a',
      active: false,
      expired: true,
      revoked: false,
      lastSeenAt: '2026-10-01T12:00:00Z',
      expiresAt: '2026-10-02T12:00:00Z'
    }]});
  },
  getAccountSession(options) {
    sessionRead += 1;
    calls.push({name: 'getAccountSession', options});
    return Promise.resolve({
      status: 200,
      etag: '"session-rev-' + sessionRead + '"',
      data: {sessionId: 'session-a', active: true, revoked: false, expired: false}
    });
  },
  revokeAccountSession(options) {
    calls.push({name: 'revokeAccountSession', options});
    if (conflict) return Promise.reject({status: 412});
    sessionActive = false;
    return Promise.resolve({
      status: 200,
      etag: '"session-terminal"',
      data: {sessionId: 'session-a', active: false, revoked: true, expired: false}
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

function findByClass(node, className) {
  if ((node.className || '').split(/\s+/).includes(className)) return node;
  for (const child of node.children || []) {
    const found = findByClass(child, className);
    if (found) return found;
  }
  return null;
}

function findAllByClass(node, className) {
  const matches = [];
  function visit(current) {
    if ((current.className || '').split(/\s+/).includes(className)) {
      matches.push(current);
    }
    (current.children || []).forEach(visit);
  }
  visit(node);
  return matches;
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
    csrfHeaders() { return {'X-CSRF-Token': 'csrf-mu9c'}; }
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
  let revoke = findByClass(root, 'settings-account-admin-session-revoke');
  assert(revoke, 'active Session must expose revoke control');
  assert.strictEqual(
    findAllByClass(root, 'settings-account-admin-session-revoke').length,
    1,
    'historical Session must not expose revoke control'
  );
  let history = findByClass(root, 'settings-account-admin-session-history');
  assert(history, 'historical Sessions must be grouped separately');
  assert.strictEqual(history.tagName, 'DETAILS');
  assert.strictEqual(history.children[0].tagName, 'SUMMARY');
  assert.strictEqual(history.children[0].textContent, 'Frühere Sitzungen (1)');

  await revoke.listeners.click();

  const reads = calls.filter(call => call.name === 'getAccountSession');
  const revokes = calls.filter(call => call.name === 'revokeAccountSession');
  assert.strictEqual(reads.length, 1);
  assert.strictEqual(revokes.length, 1);
  assert.strictEqual(reads[0].options.accountId, 'account-a');
  assert.strictEqual(reads[0].options.sessionId, 'session-a');
  assert.strictEqual(revokes[0].options.accountId, 'account-a');
  assert.strictEqual(revokes[0].options.sessionId, 'session-a');
  assert.strictEqual(revokes[0].options.ifMatch, '"session-rev-1"');
  assert.strictEqual(revokes[0].options.credentials, 'same-origin');
  assert.strictEqual(revokes[0].options.headers['X-CSRF-Token'], 'csrf-mu9c');
  assert(restores >= 1);
  assert.strictEqual(
    findByClass(root, 'settings-account-admin-session-revoke'),
    null,
    'terminal Session must no longer expose revoke control'
  );
  history = findByClass(root, 'settings-account-admin-session-history');
  assert(history, 'revoked Session must move into history');
  assert.strictEqual(history.children[0].textContent, 'Frühere Sitzungen (2)');

  sessionActive = true;
  conflict = true;
  root = await renderRoot();
  revoke = findByClass(root, 'settings-account-admin-session-revoke');
  assert(revoke);
  const revokeCountBeforeConflict =
    calls.filter(call => call.name === 'revokeAccountSession').length;
  await revoke.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'revokeAccountSession').length,
    revokeCountBeforeConflict + 1,
    'stale Session revoke must not retry implicitly'
  );
  const status = findByClass(root, 'settings-account-admin-status');
  assert(status.textContent.includes('zwischenzeitlich geändert'));

  conflict = false;
  confirmResult = false;
  root = await renderRoot();
  revoke = findByClass(root, 'settings-account-admin-session-revoke');
  assert(revoke);
  const itemReadsBeforeCancel =
    calls.filter(call => call.name === 'getAccountSession').length;
  const revokesBeforeCancel =
    calls.filter(call => call.name === 'revokeAccountSession').length;
  await revoke.listeners.click();
  assert.strictEqual(
    calls.filter(call => call.name === 'getAccountSession').length,
    itemReadsBeforeCancel,
    'cancelled confirmation must not read Session revision'
  );
  assert.strictEqual(
    calls.filter(call => call.name === 'revokeAccountSession').length,
    revokesBeforeCancel,
    'cancelled confirmation must not mutate'
  );

  console.log('test_mu9c_session_revoke_ui passed');
})().catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
