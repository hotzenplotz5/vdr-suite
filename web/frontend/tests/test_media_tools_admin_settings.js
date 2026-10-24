'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

const script = fs.readFileSync('web/frontend/settings-addons.js', 'utf8');

function harness(reply, restored) {
  const requests = [];
  const elements = [];
  function element(tag) {
    const obj = {
      tag, textContent: '', disabled: false, children: [], handlers: {},
      appendChild(child) { this.children.push(child); return child; },
      setAttribute() {},
      addEventListener(event, handler) { this.handlers[event] = handler; }
    };
    elements.push(obj);
    return obj;
  }
  const window = {
    document: {createElement: element},
    VdrSuiteBrowserSession: {
      restore() { restored.count++; return Promise.resolve(); },
      csrfHeaders() { return {'X-VDR-Suite-CSRF-Token': 'test-token'}; }
    },
    confirm() { return true; },
    fetch(url, options) {
      requests.push({url, options});
      return Promise.resolve(reply(url, options));
    }
  };
  vm.runInNewContext(script, {window});
  const parent = element('section');
  return {window, parent, elements, requests};
}

function response(status, body) {
  return {
    ok: status >= 200 && status < 300,
    status,
    text() { return Promise.resolve(JSON.stringify(body)); }
  };
}

async function flush() {
  for (let i = 0; i < 8; i++) await Promise.resolve();
  await new Promise(resolve => setImmediate(resolve));
}

async function test() {
  const restored = {count: 0};
  let desired = false, revision = 0;
  const h = harness((url, options) => {
    if (options.method === 'POST') {
      assert.equal(url, '/api/v1/addons/rectools/activation');
      assert.equal(options.credentials, 'same-origin');
      assert.equal(options.headers['X-VDR-Suite-CSRF-Token'], 'test-token');
      const body = JSON.parse(options.body);
      assert.equal(body.backendId, 'default');
      assert.equal(body.expectedRevision, revision);
      desired = body.desiredEnabled;
      revision++;
    } else {
      assert.equal(url, '/api/v1/addons/rectools/activation?backendId=default');
    }
    return response(200, {backendId: 'default', desiredEnabled: desired, revision});
  }, restored);
  h.window.VdrSuiteAddonSettings.render(h.parent, 'default');
  await flush();
  const toggle = h.elements.find(e => e.textContent === 'Aktivieren');
  assert.ok(toggle && !toggle.disabled, 'enabled admin activation control');
  assert.equal(h.parent.children[0].hidden, false, 'authorized admin card visible');
  toggle.handlers.click();
  await flush();
  assert.equal(h.requests.filter(r => r.options.method === 'POST').length, 1);
  assert.equal(desired, true);
  assert.equal(toggle.textContent, 'Deaktivieren');
  assert.ok(restored.count >= 3, 'session restored for GET, POST and readback');

  const forbidden = harness(() => response(403, {error: 'forbidden'}), {count: 0});
  forbidden.window.VdrSuiteAddonSettings.render(forbidden.parent, 'default');
  await flush();
  const protectedToggle = forbidden.elements.find(e => e.textContent === 'Aktivieren');
  assert.ok(protectedToggle && protectedToggle.disabled, 'unauthorized controls disabled');
  assert.equal(forbidden.parent.children[0].hidden, true,
    'admin-only card must be hidden on HTTP 403');
  assert.equal(forbidden.requests.length, 1);

  const loggedOut = harness(() => response(401, {error: 'unauthorized'}), {count: 0});
  loggedOut.window.VdrSuiteAddonSettings.render(loggedOut.parent, 'default');
  await flush();
  assert.equal(loggedOut.parent.children[0].hidden, true,
    'admin-only card must be hidden on HTTP 401');

  const invalid = harness(() => {throw Error('must not request');}, {count: 0});
  invalid.window.VdrSuiteAddonSettings.render(invalid.parent, '../bad');
  assert.equal(invalid.requests.length, 0);
  assert.equal(invalid.parent.children[0].hidden, false,
    'invalid backend keeps harmless configuration guidance visible');
  console.log('MEDIA_TOOLS_ADMIN_SETTINGS=PASS');
}
test().catch(error => {console.error(error);process.exitCode = 1;});
