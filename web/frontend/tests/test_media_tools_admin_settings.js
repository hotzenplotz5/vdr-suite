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

  const installed = harness(() => response(200, {
    backendId: 'default', desiredEnabled: true, activatedByInstall: true,
    revision: 0
  }), {count: 0});
  installed.window.VdrSuiteAddonSettings.render(installed.parent, 'default');
  await flush();
  assert.ok(installed.elements.some(e =>
    e.textContent === 'Durch Administratorinstallation aktiviert'));
  assert.ok(installed.elements.some(e => e.textContent === 'Deaktivieren'));

  const refused = harness((_url, options) => options.method === 'POST'
    ? response(409, {error: 'provider_not_eligible'})
    : response(200, {backendId: 'default', desiredEnabled: false, revision: 0}),
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

  // UX2 draft stays within the already authorized addon administration card.
  // No source directory is ever POSTed to an API, and even a valid draft
  // produces only a root-admin read-only --check command.
  const draft = harness(() => response(200, {
    backendId: 'default', desiredEnabled: false, revision: 0
  }), {count: 0});
  draft.window.VdrSuiteAddonSettings.render(draft.parent, 'default');
  await flush();
  assert.equal(draft.parent.children[0].hidden, false);
  const draftInputs = draft.elements.filter(e => e.tag === 'input');
  assert.equal(draftInputs.length, 3);
  draftInputs[0].value = '/srv/video/Filme';
  draftInputs[1].value = 'Filme';
  draftInputs[2].value = '/srv/vdr/video.00';
  const addRoot = draft.elements.find(e =>
    e.textContent === 'Weitere Quelle hinzufügen');
  const prepare = draft.elements.find(e =>
    e.textContent === 'Prüfkommando vorbereiten');
  const preview = draft.elements.find(e => e.tag === 'pre');
  assert.ok(addRoot && prepare && preview);
  addRoot.handlers.click();
  const extraInputs = draft.elements.filter(e => e.tag === 'input');
  assert.equal(extraInputs.length, 5);
  extraInputs[3].value = '/srv/video/Serien';
  extraInputs[4].value = 'Serien';
  prepare.handlers.click();
  assert.ok(preview.textContent.startsWith(
    'sudo bash tools/addons/configure_media_tools_paths.sh '));
  assert.ok(preview.textContent.includes(
    "'/srv/video/Filme' '/srv/vdr/video.00' --check --default-name 'Filme'"));
  assert.ok(preview.textContent.includes(
    "--add-root '/srv/video/Serien' 'Serien'"));
  assert.equal(draft.requests.filter(r => r.options.method === 'POST').length, 0,
    'admin source drafts are purely local browser presentation');

  extraInputs[3].value = '/srv/video/Filme/child';
  extraInputs[3].handlers.input();
  assert.equal(preview.textContent, '', 'an edited command must be invalidated');
  prepare.handlers.click();
  assert.equal(preview.textContent, '',
    'nested or overlapping source roots cannot produce a command');

  extraInputs[3].value = '/srv/video/Serien';
  extraInputs[4].value = "Eintrag'; rm -rf /";
  prepare.handlers.click();
  assert.equal(preview.textContent, '',
    'shell metacharacters and paths in labels are rejected');
  assert.equal(draft.requests.length, 1, 'only protected admin GET was sent');

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
