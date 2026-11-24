'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

const script = fs.readFileSync('web/frontend/settings-addons.js', 'utf8');

function harness(reply, restored, initialSources) {
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
  const sourceStore = initialSources || {
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
          if (body.videoRoot !== undefined) {
            assert.equal(sourceStore.videoRoot, '',
              'only a missing initial video root can be supplied');
            sourceStore.videoRoot = decodeURIComponent(body.videoRoot);
          }
          sourceStore.sources = decodeURIComponent(body.entries)
            .split('\n').map(function (line) {
              const [path, name] = line.split('\t');
              return {path, name};
            });
          sourceStore.revision = '1234567890abcdf0';
          sourceStore.initialSetup = false;
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
    e.textContent === 'Media Tools deaktiviert'));
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
    e.textContent === 'Media Tools aktiviert und betriebsbereit'));
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

  // An offline Agent must not disable the administrator's enable button.
  const unverified = harness(() => response(200, {
    backendId: 'default', desiredEnabled: false, revision: 0,
    providerVerified: false, agentReady: false
  }), {count: 0});
  unverified.window.VdrSuiteAddonSettings.render(unverified.parent, 'default');
  await flush();
  assert.ok(unverified.elements.some(e =>
    e.textContent === 'Media Tools deaktiviert'));
  const disabledEnable = unverified.elements.find(e => e.textContent === 'Aktivieren');
  assert.ok(disabledEnable && !disabledEnable.disabled);
  assert.equal(unverified.requests.filter(r => r.options.method === 'POST').length, 0);
  assert.equal(unverified.requests.filter(r =>
    r.url.startsWith('/api/v1/addons/rectools/sources')).length, 1,
    'administrator may inspect configured roots while Agent is offline');

  // A stale, already-enabled activation MUST be renewable without first
  // disabling the addon. That disables operation access unnecessarily and
  // destroys the current intent/provider binding before the new admin CAS.
  let bindingCurrent = false;
  let bindingRevision = 3;
  const staleBinding = harness((url, options) => {
    if (options.method === 'POST') {
      assert.equal(url, '/api/v1/addons/rectools/activation');
      assert.equal(options.credentials, 'same-origin');
      assert.equal(options.headers['X-CSRF-Token'], 'test-token');
      assert.match(options.headers['X-Correlation-ID'],
        /^addon-activation-[0-9a-f]{24}$/);
      assert.deepEqual(JSON.parse(options.body), {
        backendId: 'default', desiredEnabled: true,
        expectedRevision: bindingRevision
      }, 'renewal may only repost enabled=true at the current revision');
      bindingCurrent = true;
      bindingRevision += 1;
    }
    return response(200, {
      backendId: 'default', desiredEnabled: true,
      revision: bindingRevision,
      providerVerified: true, agentReady: true,
      activationBoundToProvider: bindingCurrent
    });
  }, {count: 0});
  staleBinding.window.VdrSuiteAddonSettings.render(staleBinding.parent, 'default');
  await flush();
  assert.ok(staleBinding.elements.some(e =>
    e.textContent === 'Media Tools aktiviert; Agent-Freigabe ist veraltet'));
  const renew = staleBinding.elements.find(e =>
    e.textContent === 'Agent-Freigabe erneuern');
  assert(renew && !renew.disabled && !renew.hidden);
  assert.equal(staleBinding.requests.filter(r =>
    r.options.method === 'POST').length, 0,
    'status reload must NEVER automatically reactivate');
  renew.handlers.click();
  await flush();
  assert.equal(staleBinding.requests.filter(r =>
    r.options.method === 'POST').length, 1);
  assert.equal(bindingRevision, 4);
  assert.equal(bindingCurrent, true);
  assert.equal(renew.hidden, true);
  assert.ok(staleBinding.elements.some(e =>
    e.textContent === 'Media Tools aktiviert und betriebsbereit'));
  assert.ok(staleBinding.elements.some(e => e.textContent === 'Deaktivieren'));

  // Without a verified provider, a saved enable is a read-only preference;
  // no shortcut to an unbound/persisted activation may be presented.

  const agentOffline = harness(() => response(200, {
    backendId: 'default', desiredEnabled: false, revision: 0,
    providerVerified: true, agentReady: false, activationBoundToProvider: false
  }), {count: 0});
  agentOffline.window.VdrSuiteAddonSettings.render(agentOffline.parent, 'default');
  await flush();
  assert.ok(!agentOffline.elements.find(e => e.textContent === 'Aktivieren').disabled);

  // A stale desired-enable must never be labelled an installed or usable
  // add-on; disabling it remains permitted even after trust is lost.
  const stale = harness(() => response(200, {
    backendId: 'default', desiredEnabled: true, revision: 2,
    providerVerified: false, agentReady: false
  }), {count: 0});
  stale.window.VdrSuiteAddonSettings.render(stale.parent, 'default');
  await flush();
  assert.ok(stale.elements.some(e =>
    e.textContent === 'Media Tools aktiviert; Worker-Verfügbarkeit wird beim Auftrag geprüft'));
  assert.ok(!stale.elements.find(e => e.textContent === 'Deaktivieren').disabled);
  assert.ok(stale.elements.find(e =>
    e.textContent === 'Agent-Freigabe erneuern').hidden);
  assert.equal(stale.requests.filter(r =>
    r.options.method === 'POST').length, 0);

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

  // MLD first installation: no root-owned source TSV yet. A successful
  // authenticated GET returns an empty editor with the trusted VDR video root.
  // Creating the very first source uses the same CSRF-protected POST contract.
  const firstStore = {
    backendId: 'default', videoRoot: '/data/tv',
    revision: 'cbf29ce484222325', sources: [], initialSetup: true
  };
  const first = harness(() => response(200, {
    backendId: 'default', desiredEnabled: false, revision: 0,
    providerVerified: false, agentReady: false
  }), {count: 0}, firstStore);
  first.window.VdrSuiteAddonSettings.render(first.parent, 'default');
  await flush();
  const firstEditor = first.elements.find(e =>
    e.className === 'settings-media-import-source-editor');
  const firstList = first.elements.find(e =>
    e.className === 'settings-media-import-source-list');
  const firstAdd = first.elements.find(e =>
    e.textContent === '+ Importordner hinzufügen');
  const firstSave = first.elements.find(e => e.textContent === 'Speichern');
  assert(firstEditor && !firstEditor.hidden && firstList);
  assert.equal(firstList.children.length, 0);
  assert(firstAdd && !firstAdd.disabled);
  assert(firstSave && firstSave.disabled, 'empty source rows cannot be saved');
  firstAdd.handlers.click();
  firstList.children[0].children[0].children[0].value = '/data/import';
  firstList.children[0].children[0].children[0].handlers.input();
  firstList.children[0].children[1].children[0].value = 'Import';
  firstList.children[0].children[1].children[0].handlers.input();
  assert.equal(firstSave.disabled, false);
  firstSave.handlers.click();
  await flush();
  assert.equal(firstStore.initialSetup, false);
  assert.deepEqual(firstStore.sources, [{path: '/data/import', name: 'Import'}]);
  assert(first.elements.some(e => e.textContent.includes('Gespeichert.')));

  // Absence of a trustworthy VDR video root must stay visible to admin.
  // Never silently substitute the Ubuntu default or a browser-defined root.
  const noVideoRoot = harness(() => response(200, {
    backendId: 'default', desiredEnabled: false, revision: 0
  }), {count: 0}, {
    backendId: 'default', videoRoot: '',
    revision: 'cbf29ce484222325', sources: [], initialSetup: true
  });
  noVideoRoot.window.VdrSuiteAddonSettings.render(noVideoRoot.parent, 'default');
  await flush();
  assert.equal(noVideoRoot.elements.find(e =>
    e.className === 'settings-media-import-source-editor').hidden, false);
  const manualTarget = noVideoRoot.elements.find(e =>
    e.textContent === 'VDR-Aufnahmeverzeichnis (einmalig)');
  const manualSave = noVideoRoot.elements.find(e =>
    e.textContent === 'Speichern');
  const manualAdd = noVideoRoot.elements.find(e =>
    e.textContent === '+ Importordner hinzufügen');
  const manualRows = noVideoRoot.elements.find(e =>
    e.className === 'settings-media-import-source-list');
  assert(manualTarget && !manualTarget.hidden && manualSave.disabled);
  assert(noVideoRoot.elements.some(e =>
    e.textContent.includes('VDR-Aufnahmeverzeichnis konnte nicht automatisch ermittelt')));
  manualAdd.handlers.click();
  manualRows.children[0].children[0].children[0].value = '/data/video/Filme';
  manualRows.children[0].children[0].children[0].handlers.input();
  manualRows.children[0].children[1].children[0].value = 'Filme';
  manualRows.children[0].children[1].children[0].handlers.input();
  assert(manualSave.disabled, 'missing VDR root not silently guessed');
  manualTarget.children[0].value = '/data/tv';
  manualTarget.children[0].handlers.input();
  assert(!manualSave.disabled, 'explicit VDR target enables initial save');
  manualSave.handlers.click();
  await flush();
  assert.equal(noVideoRoot.sourceStore.videoRoot, '/data/tv');
  assert.deepEqual(noVideoRoot.sourceStore.sources,
    [{path: '/data/video/Filme', name: 'Filme'}]);
  assert.equal(manualTarget.hidden, true,
    'destination cannot be edited after initial save');
  assert(noVideoRoot.requests.some(r => r.options.method === 'POST' &&
    JSON.parse(r.options.body).videoRoot === '%2Fdata%2Ftv'));
  assert(noVideoRoot.elements.some(e => e.textContent ===
    'Aufnahmeziel: /data/tv (bleibt unverändert)'));

  // Empty source arrays are valid ONLY with the explicit first-run marker.
  const invalidEmpty = harness(() => response(200, {
    backendId: 'default', desiredEnabled: false, revision: 0
  }), {count: 0}, {
    backendId: 'default', videoRoot: '/data/tv',
    revision: 'cbf29ce484222325', sources: []
  });
  invalidEmpty.window.VdrSuiteAddonSettings.render(invalidEmpty.parent, 'default');
  await flush();
  assert.equal(invalidEmpty.elements.find(e =>
    e.textContent === 'Speichern').disabled, true);

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
