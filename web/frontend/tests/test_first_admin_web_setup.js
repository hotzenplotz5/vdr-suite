'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const {TextEncoder} = require('node:util');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'live-remote-client-api.js'), 'utf8'
);

function response(status, object) {
  const text = JSON.stringify(object);
  const value = {status, ok: status >= 200 && status < 300,
    text: () => Promise.resolve(text), clone: () => response(status, object)};
  return value;
}

function element(tag, nodes) {
  const handlers = new Map();
  const node = {
    tagName: tag.toUpperCase(), children: [], value: '', hidden: false,
    textContent: '', style: {}, className: '', disabled: false, open: false,
    classList: {toggle() {}, add() {}, remove() {}},
    append(...children) { this.children.push(...children); },
    appendChild(child) { this.children.push(child); return child; },
    setAttribute(name, value) { this[name] = value; },
    removeAttribute(name) { delete this[name]; },
    addEventListener(name, callback) { handlers.set(name, callback); },
    focus() {},
    showModal() { this.open = true; },
    close() { this.open = false; },
    fire(name) {
      const callback = handlers.get(name);
      assert(callback, 'expected event handler ' + name);
      return callback({preventDefault() {}});
    }
  };
  nodes.push(node);
  return node;
}

async function scenario(options) {
  const nodes = [];
  const header = element('header', nodes);
  const head = element('head', nodes);
  const body = element('body', nodes);
  const requests = [];
  const document = {
    readyState: 'complete',
    documentElement: {lang: 'de'},
    head, body,
    querySelector(selector) { return selector === '.app-header' ? header : null; },
    getElementById(id) { return nodes.find(node => node.id === id) || null; },
    createElement(tag) { return element(tag, nodes); }
  };

  const context = {
    window: null, document, Date, Error, JSON, Math, Number,
    Object, Array, Promise, String, TextEncoder,
    setTimeout() { return 1; }, clearTimeout() {}, console,
    location: {protocol: 'http:', hostname: options.hostname || '192.168.178.20'},
    btoa(value) { return Buffer.from(value, 'binary').toString('base64'); },
    addEventListener() {},
    fetch(input, config) {
      requests.push({input, config});
      if (input === '/api/security/browser-sessions/current') {
        return Promise.resolve(response(401, {error: {code: 'authentication_required'}}));
      }
      if (input === '/api/security/first-admin/status') {
        return Promise.resolve(response(200, {unclaimed: options.unclaimed}));
      }
      if (input === '/api/security/first-admin/claim') {
        return Promise.resolve(options.denied
          ? response(403, {error: {code: 'lan_setup_only', message: 'Trusted LAN required'}})
          : response(201, {status: 'claimed'}));
      }
      if (input === '/api/security/browser-sessions') {
        return Promise.resolve(response(200, {
          csrfToken: 'c'.repeat(48), expiresAt: '2099-01-01T00:00:00Z'
        }));
      }
      throw Error('Unexpected HTTP request: ' + input);
    },
    VdrSuiteClientApi: {requestJson() { return Promise.resolve({}); }}
  };
  context.window = context;
  vm.createContext(context);
  vm.runInContext(source, context);

  async function settle() {
    for (let tick = 0; tick < 120; tick++) await Promise.resolve();
  }
  await settle();
  await new Promise(resolve => setImmediate(resolve));
  await settle();

  const dialog = document.getElementById('vdr-suite-session-dialog');
  assert(dialog && dialog.open,
    'modal missing hostname=' + options.hostname +
    ' unclaimed=' + options.unclaimed +
    ' found=' + Boolean(dialog) +
    ' open=' + Boolean(dialog && dialog.open) +
    ' requestPaths=' + requests.map(r => r.input).join(','));
  const form = nodes.find(node => node.tagName === 'FORM');
  const input = name => {
    const found = nodes.find(node => node.name === name);
    assert(found, 'missing ' + name);
    return found;
  };
  const title = document.getElementById('vdr-suite-session-title');
  const submit = nodes.find(node => node.className === 'vss-submit');
  assert(requests.some(r => r.input === '/api/security/first-admin/status'));

  if (!options.unclaimed) {
    assert.match(title.textContent, /anmelden/);
    assert(!requests.some(r => r.input === '/api/security/first-admin/claim'));
    return;
  }
  assert.match(title.textContent, /erstmalig einrichten/);
  assert.equal(submit.disabled, false);

  input('username').value = 'admin';
  input('password').value = 'meaningful-local-password';
  input('displayName').value = 'MLD Administrator';
  input('confirmPassword').value = 'meaningful-local-password';
  form.fire('submit');
  await settle();
  if (options.denied) {
    assert(requests.some(r => r.input === '/api/security/first-admin/claim'));
    assert.equal(context.VdrSuiteBrowserSession.isAuthenticated(), false);
    assert.equal(dialog.open, true);
    return;
  }

  const claim = requests.find(r => r.input === '/api/security/first-admin/claim');
  assert(claim);
  assert.equal(claim.config.method, 'POST');
  assert.equal(claim.config.cache, 'no-store');
  assert.equal(claim.config.credentials, 'same-origin');
  assert.equal(claim.config.headers['Content-Type'], 'application/json');
  assert.deepEqual(JSON.parse(claim.config.body), {
    loginName: 'admin',
    password: 'meaningful-local-password',
    displayName: 'MLD Administrator'
  });
  assert.equal(input('password').value, '');
  assert.equal(input('confirmPassword').value, '');
  const login = requests.find(r => r.input === '/api/security/browser-sessions');
  assert(login && login.config.headers.Authorization.startsWith('Basic '));
  assert.equal(context.VdrSuiteBrowserSession.isAuthenticated(), true);
  assert.equal(dialog.open, false);
}

(async () => {
  await scenario({unclaimed: true, hostname: '192.168.178.20'});
  await scenario({unclaimed: false, hostname: '192.168.178.20'});
  await scenario({unclaimed: true, hostname: 'mld.local', denied: true});
  console.log('FIRST_ADMIN_WEB_SETUP=PASS');
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
