'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const publicBrowserPath = path.join(__dirname, '..', 'api', 'public-v1-client.js');
const canonicalPublicPath = path.resolve(
  __dirname, '..', '..', '..', 'clients', 'reference-js', 'public-v1-client.js'
);
assert.strictEqual(fs.realpathSync(publicBrowserPath), canonicalPublicPath);

const adapterSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'account-admin-client-api.js'), 'utf8'
);
const uiSource = fs.readFileSync(
  path.join(__dirname, '..', 'settings-account-admin.js'), 'utf8'
);

[
].forEach(function(marker) {
  assert(!adapterSource.includes(marker), marker + ' must not be used by MU.9A adapter');
  assert(!uiSource.includes(marker), marker + ' must not be used by MU.9A UI');
});
assert(!uiSource.includes('.innerHTML'));

const calls = [];
const configs = [];
const publicClient = {
  getAccounts(options) {
    calls.push({name: 'getAccounts', options});
    return Promise.resolve({
      items: [{accountId: 'account-a', actorId: 'actor-a',
        displayName: 'Admin A', active: true}],
      page: {limit: 25, hasMore: false, nextCursor: null}
    });
  },
  getAccount(options) {
    calls.push({name: 'getAccount', options});
    return Promise.resolve({status: 200, etag: '"account-a"',
      data: {accountId: 'account-a', actorId: 'actor-a',
        displayName: 'Admin A', active: true}});
  },
  getAccountGrants(options) {
    calls.push({name: 'getAccountGrants', options});
    return Promise.resolve({status: 200, etag: '"grant-a"',
      data: {items: [{permission: 'channels.view', backendId: 'default'}]}});
  },
  getAccountCredentials(options) {
    calls.push({name: 'getAccountCredentials', options});
    return Promise.resolve({items: [{
      credentialId: 'credential-a', credentialType: 'human-password',
      active: true, expired: false, revoked: false, expiresAt: null,
      createdAt: '2026-10-04T00:00:00Z'
    }]});
  },
  getAccountSessions(options) {
    calls.push({name: 'getAccountSessions', options});
    return Promise.resolve({items: [{
      sessionId: 'session-a', deviceId: 'living-room',
      issuedFromCredentialId: 'credential-a', active: true,
      expired: false, revoked: false, expiresAt: '2026-10-05T00:00:00Z',
      lastSeenAt: '2026-10-04T01:00:00Z',
      createdAt: '2026-10-04T00:30:00Z'
    }]});
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

function allText(node) {
  return [node.textContent || '']
    .concat((node.children || []).map(allText)).join(' ');
}

const context = {
  window: null,
  document: {createElement(tag) { return new Element(tag); }},
  console,
  location: {origin: 'https://suite.example'},
  Promise, Object, Array, String, Set,
  VdrSuiteI18n: {t(key, parameters, fallback) { return fallback || key; }},
  VdrSuitePublicV1Client: {
    createClient(config) { configs.push(config); return publicClient; }
  }
};
context.window = context;
vm.createContext(context);
vm.runInContext(adapterSource, context);

(async function() {
  const adapter = context.VdrSuiteAccountAdminClientApi;
  const page = await adapter.listAccounts({limit: 25, cursor: 'next-a'});
  assert.strictEqual(page.items[0].accountId, 'account-a');
  assert.strictEqual(configs[0].baseUrl, 'https://suite.example');
  assert.strictEqual(configs[0].credentials, 'same-origin');
  assert.deepStrictEqual(JSON.parse(JSON.stringify(calls[0].options.query)),
    {limit: 25, sort: 'accountId', order: 'asc', cursor: 'next-a'});

  const overview = await adapter.loadAccount('account-a');
  assert.strictEqual(overview.account.displayName, 'Admin A');
  assert.strictEqual(overview.accountEtag, '"account-a"');
  assert.strictEqual(overview.grantsEtag, '"grant-a"');
  assert.strictEqual(overview.credentials[0].credentialType, 'human-password');
  assert.strictEqual(overview.sessions[0].deviceId, 'living-room');
  assert.deepStrictEqual(calls.slice(1).map(call => call.name),
    ['getAccount', 'getAccountGrants', 'getAccountCredentials', 'getAccountSessions']);

  calls.length = 0;
  adapter.resetForTests();
  vm.runInContext(uiSource, context);
  const root = new Element('section');
  const card = await context.VdrSuiteAccountAdminSettings.render(root);
  assert(card);
  const text = allText(root);
  assert(text.includes('Benutzer & Zugriffe'));
  assert(text.includes('Admin A'));
  assert(text.includes('channels.view'));
  assert(text.includes('Passwort-Anmeldung'));
  assert(text.includes('living-room'));
  assert.deepStrictEqual(calls.map(call => call.name),
    ['getAccounts', 'getAccount', 'getAccountGrants',
      'getAccountCredentials', 'getAccountSessions']);

  console.log('test_mu9a_account_admin_read_ui passed');
})().catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
