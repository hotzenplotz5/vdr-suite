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
      replaceChildren(...children) { this.children = children; },
      setAttribute() {},
      addEventListener(event, handler) { this.handlers[event] = handler; }
    };
    elements.push(obj);
    return obj;
  }
  const sourceStore = {
    backendId: 'default', videoRoot: '/srv/vdr/video.00',
    revision: '1234567890abcdef',
    sources: [{path: '/srv/video/Filme', name: 'Filme'}]
  };
  const window = {
    document: {createElement: element},
    crypto: {getRandomValues(bytes) { bytes.fill(0x4a); return bytes; }},
    VdrSuiteBrowserSession: {
      restore() { restored.count++; return Promise.resolve(); },
      csrfHeaders() { return {'X-CSRF-Token': 'test-token'}; }
    },
    confirm() { return true; },
    fetch(url, options) {
      requests.push({url, options});
      if (url.startsWith('/api/v1/addons/rectools/sources')) {
        if (options.method === 'POST') {
          assert.equal(url, '/api/v1/addons/rectools/sources');
          assert.equal(options.credentials, 'same-origin');
          assert.equal(options.headers['X-CSRF-Token'], 'test-token');
          const body = JSON.parse(options.body);
          assert.equal(body.backendId, sourceStore.backendId);
          assert.equal(body.revision, sourceStore.revision);
          sourceStore.sources = decodeURIComponent(body.entries)
            .split('\n').map(function (line) {
              const [path, name] = line.split('\t');
              return {path, name};
            });
          sourceStore.revision = '1234567890abcdf0';
        } else {
          assert.equal(url,
            '/api/v1/addons/rectools/sources?backendId=default');
        }
        return Promise.resolve(response(200, sourceStore));
      }
      return Promise.resolve(reply(url, options));
    }
  };
  vm.runInNewContext(script, {window});
  const parent = element('section');
  return {window, parent, elements, requests, sourceStore};
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
      assert.equal(options.headers['X-CSRF-Token'], 'test-token');
      assert.match(options.headers['X-Correlation-ID'],
        /^addon-activation-[0-9a-f]{24}$/);
      const body = JSON.parse(options.body);
      assert.equal(body.backendId, 'default');
      assert.equal(body.expectedRevision, revision);
      desired = body.desiredEnabled;
      revision++;
    } else {
      assert.equal(url, '/api/v1/addons/rectools/activation?backendId=default');
    }
    return response(200, {backendId: 'default', desiredEnabled: desired,
      revision, providerVerified: true, agentReady: true,
      activationBoundToProvider: desired});
  }, restored);
  h.window.VdrSuiteAddonSettings.render(h.parent, 'default');
  await flush();
  assert.ok(h.elements.some(e =>
    e.textContent === 'Handler verifiziert, Addon deaktiviert'));
  const toggle = h.elements.find(e => e.textContent === 'Aktivieren');
  assert.ok(toggle && !toggle.disabled, 'enabled admin activation control');
  assert.equal(h.parent.children[0].hidden, false, 'authorized admin card visible');
  toggle.handlers.click();
  await flush();
  assert.equal(h.requests.filter(r => r.options.method === 'POST').length, 1);
  assert.equal(desired, true);
  assert.equal(toggle.textContent, 'Deaktivieren');
  toggle.handlers.click();
  await flush();
  assert.equal(h.requests.filter(r => r.options.method === 'POST').length, 2,
    'both admin switch directions use authenticated POST');
  assert.equal(desired, false);
  assert.equal(toggle.textContent, 'Aktivieren');
  assert.equal(revision, 2);
  assert.ok(restored.count >= 5, 'session restored for both POSTs and readbacks');

  const installed = harness(() => response(200, {
    backendId: 'default', desiredEnabled: true, activatedByInstall: true,
    providerVerified: true, agentReady: true,
    activationBoundToProvider: true, revision: 0
  }), {count: 0});
  installed.window.VdrSuiteAddonSettings.render(installed.parent, 'default');
  await flush();
  assert.ok(installed.elements.some(e =>
    e.textContent === 'Media Tools technisch freigegeben (Benutzerrechte separat prüfen)'));
  assert.ok(installed.elements.some(e => e.textContent === 'Deaktivieren'));
  assert.ok(!h.elements.some(e => e.textContent === 'Installieren' ||
    e.textContent === 'Deinstallieren'),
    'UI must not present nonfunctional or privileged package actions');

  const refused = harness((_url, options) => options.method === 'POST'
    ? response(409, {error: 'provider_not_eligible'})
    : response(200, {backendId: 'default', desiredEnabled: false,
        providerVerified: true, agentReady: true, revision: 0}),
  {count: 0});
  refused.window.VdrSuiteAddonSettings.render(refused.parent, 'default');
  await flush();
  refused.elements.find(e => e.textContent === 'Aktivieren').handlers.click();
  await flush();
  const refusedStatus = refused.elements.find(e => e.tag === 'p' &&
    e.textContent.includes('provider_not_eligible'));
  assert.ok(refusedStatus, 'server rejection remains visible after readback');
  assert.equal(refused.parent.children[0].hidden, false);
  assert.equal(refused.requests.filter(r => r.options.method === 'POST').length, 1);

  // A flat 403 from the Core activation service must remain visible without
  // hiding the verified administrator's entire settings panel.
  const denied = harness((_url, options) => options.method === 'POST'
    ? response(403, {error: 'activation_denied'})
    : response(200, {backendId: 'default', desiredEnabled: true,
        providerVerified: true, agentReady: true,
        activationBoundToProvider: true, revision: 3}),
  {count: 0});
  denied.window.VdrSuiteAddonSettings.render(denied.parent, 'default');
  await flush();
  denied.elements.find(e => e.textContent === 'Deaktivieren').handlers.click();
  await flush();
  assert.equal(denied.parent.children[0].hidden, false);
  assert.ok(denied.elements.some(e => e.textContent.includes(
    'Änderung abgelehnt: activation_denied (403)')));
  assert.equal(denied.requests.filter(r => r.options.method === 'POST').length, 1);

  // Fail closed: an unverified provider is not proof of an absent package.
  // Its activation button cannot start an attempted enable mutation.
  const unverified = harness(() => response(200, {
    backendId: 'default', desiredEnabled: false, revision: 0,
    providerVerified: false, agentReady: false
  }), {count: 0});
  unverified.window.VdrSuiteAddonSettings.render(unverified.parent, 'default');
  await flush();
  assert.ok(unverified.elements.some(e =>
    e.textContent === 'Backend-Agent nicht bereit; lokale Installation wird hier nicht geprüft'));
  const disabledEnable = unverified.elements.find(e => e.textContent === 'Aktivieren');
  assert.ok(disabledEnable && disabledEnable.disabled);
  assert.ok(disabledEnable.title.includes('Backend-Agent nicht bereit'));
  assert.equal(unverified.requests.filter(r => r.options.method === 'POST').length, 0);
  assert.equal(unverified.requests.filter(r =>
    r.url.startsWith('/api/v1/addons/rectools/sources')).length, 0,
    'unverified provider must not expose or request source configuration');

  // Byte-verified provider with a stale Agent generation is not activated.
  const staleBinding = harness(() => response(200, {
    backendId: 'default', desiredEnabled: true, revision: 3,
    providerVerified: true, agentReady: true, activationBoundToProvider: false
  }), {count: 0});
  staleBinding.window.VdrSuiteAddonSettings.render(staleBinding.parent, 'default');
  await flush();
  assert.ok(staleBinding.elements.some(e =>
    e.textContent === 'Aktivierungswunsch vorhanden; Agent-/Handlerbindung nicht bestätigt'));

  const agentOffline = harness(() => response(200, {
    backendId: 'default', desiredEnabled: false, revision: 0,
    providerVerified: true, agentReady: false, activationBoundToProvider: false
  }), {count: 0});
  agentOffline.window.VdrSuiteAddonSettings.render(agentOffline.parent, 'default');
  await flush();
  assert.ok(agentOffline.elements.find(e => e.textContent === 'Aktivieren').disabled);

  // A stale desired-enable must never be labelled an installed or usable
  // add-on; disabling it remains permitted even after trust is lost.
  const stale = harness(() => response(200, {
    backendId: 'default', desiredEnabled: true, revision: 2,
    providerVerified: false, agentReady: false
  }), {count: 0});
  stale.window.VdrSuiteAddonSettings.render(stale.parent, 'default');
  await flush();
  assert.ok(stale.elements.some(e =>
    e.textContent === 'Aktivierungswunsch gespeichert; Agent bestätigt Handler aktuell nicht'));
  assert.ok(!stale.elements.find(e => e.textContent === 'Deaktivieren').disabled);

  // MT.UX2: actual administrator save, no generated shell command.
  const managed = harness(() => response(200, {
    backendId: 'default', desiredEnabled: false, revision: 0,
    providerVerified: true, agentReady: true
  }), {count: 0});
  managed.window.VdrSuiteAddonSettings.render(managed.parent, 'default');
  await flush();
  const editor = managed.elements.find(e =>
    e.className === 'settings-media-import-source-editor');
  const rows = managed.elements.find(e =>
    e.className === 'settings-media-import-source-list');
  const add = managed.elements.find(e =>
    e.textContent === '+ Importordner hinzufügen');
  const save = managed.elements.find(e => e.textContent === 'Speichern');
  assert(editor && rows && add && save && !editor.hidden);
  assert.equal(rows.children.length, 1);
  assert.equal(rows.children[0].children[0].children[0].value,
    '/srv/video/Filme');
  add.handlers.click();
  assert.equal(rows.children.length, 2);
  rows.children[1].children[0].children[0].value = '/srv/video/Serien';
  rows.children[1].children[0].children[0].handlers.input();
  rows.children[1].children[1].children[0].value = 'Serien';
  rows.children[1].children[1].children[0].handlers.input();
  const setDefault = rows.children[1].children.find(e =>
    e.textContent === 'Als Standard festlegen');
  setDefault.handlers.click();
  assert.equal(rows.children[0].children[0].children[0].value,
    '/srv/video/Serien');
  save.handlers.click();
  await flush();
  assert.equal(managed.requests.filter(r =>
    r.url === '/api/v1/addons/rectools/sources' &&
    r.options.method === 'POST').length, 1);
  assert.deepEqual(managed.sourceStore.sources, [
    {path: '/srv/video/Serien', name: 'Serien'},
    {path: '/srv/video/Filme', name: 'Filme'}
  ]);
  assert(managed.elements.some(e => e.textContent.includes(
    'Gespeichert. Der Backend-Agent übernimmt')));

  // Remove one existing entry; save again without restarting the Agent.
  rows.children[1].children.find(e =>
    e.textContent === 'Entfernen').handlers.click();
  save.handlers.click();
  await flush();
  assert.equal(managed.sourceStore.sources.length, 1);
  assert.equal(managed.sourceStore.sources[0].path, '/srv/video/Serien');

  // Invalid paths never produce a mutation, even with a valid admin session.
  add.handlers.click();
  rows.children[1].children[0].children[0].value = '/srv/video/Serien/sub';
  rows.children[1].children[0].children[0].handlers.input();
  rows.children[1].children[1].children[0].value = 'Unsicher';
  rows.children[1].children[1].children[0].handlers.input();
  const beforeInvalid = managed.requests.filter(r =>
    r.url === '/api/v1/addons/rectools/sources').length;
  save.handlers.click();
  await flush();
  assert.equal(managed.requests.filter(r =>
    r.url === '/api/v1/addons/rectools/sources').length, beforeInvalid,
    'overlapping roots must never be sent to the server');
  assert(!managed.elements.some(e => e.tag === 'pre'),
    'no generated shell command belongs in the settings UX');

  const forbidden = harness(() => response(403, {error: 'forbidden'}), {count: 0});
  forbidden.window.VdrSuiteAddonSettings.render(forbidden.parent, 'default');
  await flush();
  const protectedToggle = forbidden.elements.find(e => e.textContent === 'Aktivieren');
  assert.ok(protectedToggle && protectedToggle.disabled, 'unauthorized controls disabled');
  assert.equal(forbidden.parent.children[0].hidden, true,
    'admin-only card must be hidden on HTTP 403');
  assert.equal(forbidden.requests.length, 1);

  // An unavailable admin API cannot reveal the source draft even though
  // the existing card may still show a read-only error message.
  const unavailable = harness(() => response(503, {error: 'unavailable'}),
    {count: 0});
  unavailable.window.VdrSuiteAddonSettings.render(unavailable.parent, 'default');
  await flush();
  const unavailableDraft = unavailable.elements.find(e =>
    e.className === 'settings-media-import-source-editor');
  assert.ok(unavailableDraft && unavailableDraft.hidden,
    'source editor must not appear before a successful protected admin GET');

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
