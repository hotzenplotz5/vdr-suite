'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const frontend = path.join(__dirname, '..');
const index = fs.readFileSync(path.join(frontend, 'index.html'), 'utf8');
const app = fs.readFileSync(path.join(frontend, 'app.js'), 'utf8');
const view = fs.readFileSync(path.join(frontend, 'modules/media-import.js'), 'utf8');
const clientFactory = require('../api/public-v1-client.js');

assert(index.includes('data-module="media-import" hidden disabled'));
const nav = index.slice(index.indexOf('<nav id="module-nav"'),
                        index.indexOf('</nav>', index.indexOf('<nav id="module-nav"')));
assert(nav.indexOf('data-module="overview"') < nav.indexOf('data-module="recordings2"'));
assert(nav.indexOf('data-module="recordings2"') < nav.indexOf('data-module="media-import"'));
assert(nav.indexOf('data-module="media-import"') < nav.indexOf('data-module="genres"'),
  'MT.UX0 Media Import must sit directly after recordings');
assert(index.indexOf('modules/media-import.js') < index.indexOf('frontend/app.js'));
assert(app.includes('refreshMediaImportVisibility(backendId);'));
assert(app.includes('resetMediaImportVisibility(true);'));
assert(!view.includes('.innerHTML'));
assert(!view.includes('localStorage'));
assert(!view.includes('sessionStorage'));

class Element {
  constructor(name) {
    this.tagName = name.toUpperCase();
    this.children = [];
    this.listeners = {};
    this.attributes = {};
    this.dataset = {};
    this.textContent = '';
    this.className = '';
    this.hidden = false;
    this.disabled = false;
    this.value = '';
    this.classList = {toggle() {}};
  }
  appendChild(child) { this.children.push(child); return child; }
  replaceChildren(...children) { this.children = children; }
  addEventListener(name, callback) { this.listeners[name] = callback; }
  setAttribute(name, value) { this.attributes[name] = value; }
}
function findClass(node, cls) {
  if ((node.className || '').split(/\s+/).includes(cls)) return node;
  for (const child of node.children || []) {
    const found = findClass(child, cls);
    if (found) return found;
  }
  return null;
}
function flush() {
  return new Promise(function(resolve) { setImmediate(resolve); });
}
async function settle() {
  for (let i = 0; i < 14; i++) await flush();
}
let allow = true;
let confirmLocal = true;
let confirmCalls = 0;
let sourcesStatus = 200;
let state = 'dispatching';
let submitMode = 'accepted';
let batchSources = null;
let markedSources = [];
let sourcePartial = false;
let sourceDisplayOverride = null;
const sourceFolders = Object.create(null);
const sourceDisplayLabels = Object.create(null);
let pagedSourceIds = null;
let batchOperations = [];
const BATCH_OP_A = 'op_' + 'd'.repeat(32);
const BATCH_OP_B = 'op_' + 'e'.repeat(32);
const SOURCE_B = 'source_987654321012345';
const MARK_PC = 'source_marked_pc_123456';
const MARK_SKIP = 'source_marked_sk_123456';
const MARK_DUP = 'source_marked_du_123456';
let batchStates = Object.create(null);
let delayedSubmit = null;
let operationReads = 0;
let submits = 0;
let counter = 0;
const requests = [];
let sessionRestores = 0;
let previewMode = 'ready';
let previewInspectionStatus = 'ready';
let previewVideoCodec = 'h264';
const batchProbeOverrides = Object.create(null);
let resolveDelayedPreview = null;
const SOURCE = 'source_0123456789abcde';
const OP = 'op_12345678901234567890123456789012';
function reply(status, value, headers) {
  return {
    ok: status >= 200 && status < 300,
    status: status,
    headers: {get(name) { return (headers || {})[name] || null; }},
    text() { return Promise.resolve(JSON.stringify(value)); }
  };
}
function transport(url, options) {
  const path = String(url).replace(/^https?:\/\/[^/]+/, '');
  requests.push({path, options});
  const sourcePreview = /^\/api\/v1\/backends\/backend-a\/media-import-sources\/([A-Za-z0-9_-]{16,64})\/preview$/.exec(path);
  if (sourcePreview) {
    const id = sourcePreview[1];
    if (previewMode === 'defer-source' && id === SOURCE) {
      previewMode = 'ready';
      return new Promise(function (resolve) { resolveDelayedPreview = resolve; });
    }
    if (previewMode === 'pending-first' && id === SOURCE_B) {
      previewMode = 'ready';
      return Promise.resolve(reply(202, {
        backendId: 'backend-a', sourceId: id, state: 'pending'
      }));
    }
    if (previewMode === 'forbidden' && id === SOURCE) {
      return Promise.resolve(reply(403, {detail: 'Denied'}));
    }
    const override = batchProbeOverrides[id] || {};
    const status = override.status || previewInspectionStatus;
    const codec = override.codec ||
      (status === 'insufficient' ? 'vp9' : previewVideoCodec);
    return Promise.resolve(reply(200, {
      backendId: 'backend-a', sourceId: id,
      state: 'ready', status: status,
      title: id === SOURCE_B ? 'Backend geprüfter Andor-Film' :
        'Backend geprüfter Film',
      targetFolder: id === SOURCE_B
        ? 'Serien/Andor/Weiterer Film' : 'Die Herrschaft des Feuers',
      videoCodec: codec,
      sourceBytes: 2500000000,
      availableBytes: 12000000000, requiredBytes: 7600000000,
      videoReencodeRequired: codec === 'vp9'
    }));
  }
  if (path.endsWith('/capabilities/media-import')) {
    return Promise.resolve(reply(200, {
      backendId: 'backend-a', capability: 'addons.media.import', available: allow
    }));
  }
  if (path.endsWith('/media-import-sources') ||
      path.includes('/media-import-sources?cursor=')) {
    if (pagedSourceIds) {
      const match = /[?]cursor=mi1_1_([0-9]+)$/.exec(path);
      const offset = match ? Number(match[1]) : 0;
      const ids = pagedSourceIds.slice(offset, offset + 32);
      const next = offset + 32 < pagedSourceIds.length
        ? '/api/v1/backends/backend-a/media-import-sources?cursor=mi1_1_' +
          (offset + 32) : undefined;
      return Promise.resolve(reply(200, {
        items: ids.map(function (id) {
          return {sourceId: id, label: sourceDisplayLabels[id] || 'Film ' + id,
            ...(Object.prototype.hasOwnProperty.call(sourceFolders, id)
              ? {folder: sourceFolders[id]} : {})};
        }),
        meta: {partial: true},
        links: next ? {next: next} : {}
      }));
    }
    return Promise.resolve(sourcesStatus === 200
      ? reply(200, {items: (batchSources || [SOURCE]).concat(markedSources).map(function (id) {
        return {sourceId: id, label: id === SOURCE
          ? (sourceDisplayOverride || 'Die Herrschaft des Feuers') :
            id === MARK_PC ? 'Film.pc_encode' :
            id === MARK_SKIP ? 'Film.skipped' :
            id === MARK_DUP ? 'Film.duplicate' : 'Weiterer Film',
          ...(Object.prototype.hasOwnProperty.call(sourceFolders, id)
            ? {folder: sourceFolders[id]} : {})};
      }), meta: {partial: sourcePartial}})
      : reply(sourcesStatus, {detail: 'Source unavailable'}));
  }
  if (path.endsWith('/media-imports')) {
    ++submits;
    if (submitMode === 'batch') {
      const operationId = batchOperations.length === 0 ? BATCH_OP_A : BATCH_OP_B;
      batchOperations.push(operationId);
      return Promise.resolve(reply(202, {
        operationId, backendId: 'backend-a', state: 'dispatching'
      }));
    }
    if (submitMode === 'deferred') {
      return new Promise(function (resolve, reject) {
        assert.strictEqual(delayedSubmit, null, 'only one outstanding import POST');
        delayedSubmit = {resolve, reject};
      });
    }
    if (submitMode === 'uncertain') return Promise.reject(new Error('lost response'));
    if (submitMode === 'expired') return Promise.resolve(reply(422, {detail: 'Selection expired'}));
    if (submitMode === 'denied') return Promise.resolve(reply(403, {detail: 'Permission denied'}));
    return Promise.resolve(reply(202, {operationId: OP, backendId: 'backend-a',
      state: 'dispatching'}, {'Location': '/api/v1/operations/' + OP}));
  }
  if (path === '/api/v1/operations/' + BATCH_OP_A ||
      path === '/api/v1/operations/' + BATCH_OP_B) {
    ++operationReads;
    return Promise.resolve(reply(200, {
      operationId: path.split('/').pop(), backendId: 'backend-a',
      state: batchStates[path.split('/').pop()] || 'dispatching'
    }));
  }
  if (path === '/api/v1/operations/' + OP) {
    ++operationReads;
    return Promise.resolve(reply(200, {operationId: OP, backendId: 'backend-a',
      state: state, links: {self: path}}, {'ETag': '"revision-1"'}));
  }
  throw new Error('unexpected route: ' + path);
}
const tab = new Element('button');
tab.dataset.module = 'media-import';
tab.hidden = true;
tab.disabled = true;
const mount = new Element('section');
const timerCallbacks = [];
const modules = {};
const context = {
  console, Promise, Uint8Array, setImmediate,
  document: {
    createElement(name) { return new Element(name); },
    querySelector(sel) {
      return sel === '.module-tab[data-module="media-import"]' ? tab : null;
    },
    querySelectorAll(sel) {
      return sel === '.module-tab' ? [tab] : [];
    }
  },
  crypto: {
    getRandomValues(bytes) {
      ++counter;
      bytes.fill(counter);
      return bytes;
    }
  },
  location: {origin: 'https://suite.test'},
  confirm() { ++confirmCalls; return confirmLocal; },
  setTimeout(fn) { timerCallbacks.push(fn); return timerCallbacks.length; },
  clearTimeout() {},
  VdrSuitePlatform: {
    registerModule(name, api) { modules[name] = api; },
    getModule(name) { return modules[name] || null; }
  },
  VdrSuitePublicV1Client: {
    createClient() {
      return clientFactory.createClient({fetch: transport, credentials: 'same-origin'});
    }
  },
  VdrSuiteBrowserSession: {
    restore() { ++sessionRestores; return Promise.resolve({authenticated: true}); },
    csrfHeaders() { return {'X-CSRF-Token': 'test-csrf'}; }
  }
};
context.window = context;
vm.createContext(context);
vm.runInContext(view, context);

function slice(first, last) {
  const start = app.indexOf(first);
  const end = app.indexOf(last, start);
  assert(start >= 0 && end > start, 'app composition anchor missing: ' + first);
  return app.slice(start, end);
}
const gateCode = slice("let selectedModule = 'overview';", "let homeResumeBackendId = '';");
const renderCode = slice('function renderSelectedModule(data) {',
  'function publishHomeResume(previousModule) {');
const selectionCode = slice('function selectModule(moduleName) {',
  'function markSelected(backendId) {');
vm.runInContext(gateCode + '\n' + renderCode + '\n' + selectionCode, context);
vm.runInContext(`
let selectedBackendId = 'backend-a';
let currentSnapshot = {};
let detailDataElement = window.testMount;
let browserSessionFrontendActive = true;
let windowUnused = 0;
function frontendPlatformModule(name, legacy) {
  return window.VdrSuitePlatform.getModule(name) || legacy;
}
function selectedEpgBackendId() { return selectedBackendId; }
function renderSnapshotMetrics() {}
function shouldPublishHomeResume() { return false; }
function publishHomeResume() {}
`, Object.assign(context, {testMount: mount}));

(async function () {
  // Real app capability-gate -> production module selection -> actual DOM buttons.
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  assert.strictEqual(tab.hidden, false);
  context.selectModule('media-import');
  await settle();
  let select = findClass(mount, 'media-import-source');
  let start = findClass(mount, 'media-import-start');
  assert(select && start, 'production detail owner must mount real controls');
  assert(select.children[1].textContent.includes('Die Herrschaft des Feuers'),
    'backend-approved film title must be visible without HTML rendering');
  assert(!select.children[1].textContent.includes(SOURCE),
    'opaque source IDs must stay in collapsed technical details');
  assert(findClass(mount, 'media-import-technical-details'),
    'MT.UX1 must retain optional technical details');
  assert(findClass(mount, 'media-import-scan-notice').textContent.includes('Scan abgeschlossen'),
    'backend completeness must be displayed only when explicitly certified');
  const postBeforeScan = requests.filter(r => r.options.method === 'POST').length;
  sourcePartial = true;
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  assert(findClass(mount, 'media-import-scan-notice').textContent.includes('Scan unvollständig'));
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    postBeforeScan, 'scan is strictly read-only');
  sourcePartial = false;
  assert.strictEqual(start.disabled, true);
  select.value = SOURCE;
  select.listeners.change();
  assert(start.disabled, 'pending preview must not enable an import');
  const noPreviewPost = submits;
  await start.listeners.click();
  assert.strictEqual(submits, noPreviewPost, 'an early click must not bypass diagnosis');
  await settle();
  assert.strictEqual(start.disabled, false);
  assert.strictEqual(start.textContent, 'Diesen Film importieren',
    'single-file action must state its effect');
  const importPanel = findClass(mount, 'media-import-panel');
  assert(importPanel.children.indexOf(start) >
    importPanel.children.indexOf(findClass(mount, 'media-import-preview')) &&
    importPanel.children.indexOf(start) <
    importPanel.children.indexOf(findClass(mount, 'media-import-marked-label')),
    'primary import action must be adjacent to the preview, before marker administration');
  assert(findClass(mount, 'media-import-preview').textContent.includes('Nächster Schritt'),
    'ready preview must explain the next action');
  assert(findClass(mount, 'media-import-decision-label').hidden,
    'compatible source needs no encoding decision');
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'technisch für den VDR-Import geeignet'));
  assert(findClass(mount, 'media-import-preview').textContent.includes('Die Herrschaft des Feuers'),
    'MT.UX4 selected film is visible before confirmation');
  assert(findClass(mount, 'media-import-source-details').textContent.includes(SOURCE),
    'opaque selection token is retained only in technical details');
  const beforeFirstImport = submits;
  confirmLocal = false;
  await start.listeners.click();
  assert.strictEqual(submits, beforeFirstImport, 'cancelled preview never posts an import');
  confirmLocal = true;
  await start.listeners.click();
  await settle();
  assert.strictEqual(submits, 1);
  const post = requests.find(x => x.options.method === 'POST');
  assert(post, 'user-style click must dispatch canonical v1 request');
  assert.strictEqual(post.path, '/api/v1/backends/backend-a/media-imports');
  assert.strictEqual(post.options.body, JSON.stringify({sourceId: SOURCE}));
  assert.strictEqual(post.options.headers['X-CSRF-Token'], 'test-csrf');
  assert(post.options.headers['Idempotency-Key'].startsWith('media-import-'));
  assert(sessionRestores >= 1);
  assert(operationReads >= 1);
  assert(findClass(mount, 'media-import-operation').textContent.includes(OP));
  assert.strictEqual(start.disabled, true);
  await start.listeners.click();
  assert.strictEqual(submits, 1, 'double-click must not submit a second import');

  // Owner transition must not delete Core operation tracking.
  context.selectModule('overview');
  context.selectModule('media-import');
  await settle();
  assert(findClass(mount, 'media-import-operation').textContent.includes(OP));
  assert.strictEqual(submits, 1);
  state = 'outcome_unknown';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes('ungewiss'));
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, true);
  state = 'succeeded';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes('unabhängig bestätigt'));
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, false);
  await findClass(mount, 'media-import-new').listeners.click();
  await settle();

  // A new explicit user action gets its own random key, no retry path.
  select = findClass(mount, 'media-import-source');
  select.value = SOURCE;
  select.listeners.change();
  await settle();
  submitMode = 'uncertain';
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, 2);
  assert(findClass(mount, 'status').textContent.includes('ungewiss'));
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true);
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, true);
  const posts = requests.filter(x => x.options.method === 'POST');
  assert.notStrictEqual(posts[0].options.headers['Idempotency-Key'],
    posts[1].options.headers['Idempotency-Key']);

  // Login/out and permissions must hide and invalidate the production module.
  context.resetMediaImportVisibility(true);
  assert.strictEqual(tab.hidden, true);
  allow = false;
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), false);
  assert.strictEqual(tab.hidden, true);
  context.selectModule('media-import');
  assert.strictEqual(tab.hidden, true);
  allow = true;
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();

  // An explicit HTTP denial is not an indeterminate transport failure.
  submitMode = 'expired';
  select = findClass(mount, 'media-import-source');
  select.value = SOURCE;
  select.listeners.change();
  await settle();
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes('abgelaufen'));
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true);
  assert.strictEqual(findClass(mount, 'media-import-reload').disabled, false);

  submitMode = 'denied';
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  select = findClass(mount, 'media-import-source');
  select.value = SOURCE;
  select.listeners.change();
  await settle();
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes('Berechtigung'));

  sourcesStatus = 503;
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes('abgelaufen'));
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true);

  // A pending POST must survive a real production tab/owner replacement.
  sourcesStatus = 200;
  submitMode = 'deferred';
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  select = findClass(mount, 'media-import-source');
  select.value = SOURCE;
  select.listeners.change();
  await settle();
  const acceptedResult = findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert(delayedSubmit, 'POST must have reached the transport');
  const acceptedPostCount = submits;
  context.selectModule('overview');
  context.selectModule('media-import');
  await settle();
  assert(findClass(mount, 'status').textContent.includes('läuft'));
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true);
  delayedSubmit.resolve(reply(202, {
    operationId: OP, backendId: 'backend-a', state: 'dispatching'
  }, {'Location': '/api/v1/operations/' + OP}));
  delayedSubmit = null;
  await acceptedResult;
  await settle();
  assert.strictEqual(submits, acceptedPostCount, 'owner handoff must not repost');
  assert(findClass(mount, 'media-import-operation').textContent.includes(OP),
    'late accepted POST must reconcile into the currently mounted owner');
  assert.strictEqual(findClass(mount, 'media-import-refresh').disabled, false);
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, false);
  await findClass(mount, 'media-import-new').listeners.click();
  await settle();

  // A definite HTTP denial after the same handoff must release the stale lock.
  select = findClass(mount, 'media-import-source');
  select.value = SOURCE;
  select.listeners.change();
  await settle();
  const deniedResult = findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert(delayedSubmit);
  context.selectModule('overview');
  context.selectModule('media-import');
  await settle();
  delayedSubmit.resolve(reply(422, {detail: 'Selection expired'}));
  delayedSubmit = null;
  await deniedResult;
  await settle();
  assert(findClass(mount, 'status').textContent.includes('abgelaufen'));
  assert.strictEqual(findClass(mount, 'media-import-reload').disabled, false);
  assert.strictEqual(findClass(mount, 'media-import-refresh').hidden, true);
  assert.strictEqual(findClass(mount, 'media-import-operation').textContent, '');

  // An indeterminate POST remains blocked after handoff (never auto-retry).
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  select = findClass(mount, 'media-import-source');
  select.value = SOURCE;
  select.listeners.change();
  await settle();
  const uncertainResult = findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert(delayedSubmit);
  const uncertainPostCount = submits;
  context.selectModule('overview');
  context.selectModule('media-import');
  await settle();
  delayedSubmit.reject(new Error('lost acceptance response'));
  delayedSubmit = null;
  await uncertainResult;
  await settle();
  assert(findClass(mount, 'status').textContent.includes('ungewiss'));
  await findClass(mount, 'media-import-start').listeners.click();
  assert.strictEqual(submits, uncertainPostCount);
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, true);

  // Logout drops the unknown attempt and revokes its in-memory association.
  context.resetMediaImportVisibility(true);
  assert.strictEqual(tab.hidden, true);
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-operation').textContent, '');

  // A late accepted response from a previous authenticated view must not leak
  // an operation across the logout boundary or resurrect the old attempt.
  select = findClass(mount, 'media-import-source');
  select.value = SOURCE;
  select.listeners.change();
  await settle();
  const logoutResult = findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert(delayedSubmit, 'logout fixture must have an outstanding POST');
  context.resetMediaImportVisibility(true);
  delayedSubmit.resolve(reply(202, {
    operationId: OP, backendId: 'backend-a', state: 'dispatching'
  }));
  delayedSubmit = null;
  await logoutResult;
  await settle();
  assert.strictEqual(tab.hidden, true);
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-operation').textContent, '');
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true);


  // User-style batch click submits only the first source. Unknown outcomes
  // must never generate a follow-on POST.
  submitMode = 'batch';
  batchSources = [SOURCE, SOURCE_B];
  context.selectModule('overview');
  context.selectModule('media-import');
  await settle();
  const batchButton = findClass(mount, 'media-import-batch');
  assert(batchButton && !batchButton.disabled);
  const countBefore = submits;
  await batchButton.listeners.click();
  await settle();
  assert.strictEqual(submits, countBefore, 'read-only batch diagnosis must never submit');
  assert(findClass(mount, 'media-import-batch-summary').textContent.includes('2 direkt importierbar'));
  assert.strictEqual(findClass(mount, 'media-import-batch-review').children.length, 2);
  await findClass(mount, 'media-import-batch-confirm').listeners.click();
  await settle();
  assert.strictEqual(submits, countBefore + 1);
  assert.strictEqual(batchOperations.length, 1);
  batchStates[BATCH_OP_A] = 'outcome_unknown';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert.strictEqual(batchOperations.length, 1);
  assert.strictEqual(findClass(mount, 'media-import-batch-stop').hidden, true);

  // Logout clears in-session batch authority. A new deliberate click can
  // start two sequential Operations, provided VDR confirms the first.
  context.resetMediaImportVisibility(true);
  batchOperations = [];
  batchStates = Object.create(null);
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  await findClass(mount, 'media-import-batch').listeners.click();
  await settle();
  assert.strictEqual(batchOperations.length, 0, 'diagnosis alone cannot create an Operation');
  await findClass(mount, 'media-import-batch-confirm').listeners.click();
  await settle();
  assert.strictEqual(batchOperations.length, 1);
  batchStates[BATCH_OP_A] = 'succeeded';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert.strictEqual(batchOperations.length, 2,
    'second source can dispatch only after independent readback');
  batchStates[BATCH_OP_B] = 'failed_verified';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert.strictEqual(batchOperations.length, 2);
  assert.strictEqual(findClass(mount, 'media-import-batch-stop').hidden, true);

  // UX5: a mixed four-film set gets individual, read-only diagnoses.
  // VP9 and duplicates are held back; confirmed H.264/HEVC films continue.
  context.resetMediaImportVisibility(true);
  const SOURCE_C = 'source_334455667788990';
  const SOURCE_D = 'source_445566778899001';
  batchSources = [SOURCE, SOURCE_B, SOURCE_C, SOURCE_D];
  batchProbeOverrides[SOURCE_B] = {codec: 'vp9'};
  batchProbeOverrides[SOURCE_C] = {status: 'duplicate'};
  batchProbeOverrides[SOURCE_D] = {codec: 'hevc'};
  batchOperations = [];
  batchStates = Object.create(null);
  submitMode = 'batch';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const postsBeforeMixed = submits;
  await findClass(mount, 'media-import-batch').listeners.click();
  await settle();
  const mixedSummary = findClass(mount, 'media-import-batch-summary').textContent;
  assert(mixedSummary.includes('2 direkt importierbar') &&
    mixedSummary.includes('1 zurückgestellt') &&
    mixedSummary.includes('1 Dubletten'), mixedSummary);
  const mixedRows = findClass(mount, 'media-import-batch-review').children;
  assert.strictEqual(mixedRows.length, 4);
  assert(mixedRows.some(function (row) {
    return row.textContent.includes('Umkodierung nötig');
  }));
  assert(mixedRows.some(function (row) {
    return row.textContent.includes('Aufnahme vorhanden');
  }));
  assert.strictEqual(submits, postsBeforeMixed,
    'no mixed-film mutation before explicit batch approval');
  const mixedReviewConfirm = findClass(mount, 'media-import-batch-confirm');
  const beforeMixedConfirm = confirmCalls;
  confirmLocal = false;
  await mixedReviewConfirm.listeners.click();
  assert.strictEqual(submits, postsBeforeMixed);
  assert.strictEqual(confirmCalls, beforeMixedConfirm + 1);
  confirmLocal = true;
  await mixedReviewConfirm.listeners.click();
  await settle();
  assert.strictEqual(batchOperations.length, 1);
  batchStates[BATCH_OP_A] = 'succeeded';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert.strictEqual(batchOperations.length, 2,
    'after independent VDR readback the next compatible film proceeds');
  batchStates[BATCH_OP_B] = 'succeeded';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert.strictEqual(batchOperations.length, 2, 'held-back VP9 is never implicitly encoded');
  assert(findClass(mount, 'media-import-batch-summary').textContent.includes(
    '2 bereits bestätigt'));
  assert(findClass(mount, 'media-import-batch-review').children.some(function (row) {
    return row.textContent.includes('Umkodierung nötig');
  }));
  assert.strictEqual(findClass(mount, 'media-import-batch-stop').hidden, true);
  // UX5.3: returning to an incompatible film reuses the single-film
  // preview/decision; it does not silently submit or authorize encoding.
  const deferredRow = findClass(mount, 'media-import-batch-review').children.find(
    function (row) { return row.textContent.includes('Umkodierung nötig'); });
  const deferredAction = findClass(deferredRow, 'media-import-batch-deferred');
  assert(deferredAction && !deferredAction.disabled);
  const postsBeforeDeferred = submits;
  submitMode = 'accepted';
  state = 'succeeded';
  await deferredAction.listeners.click();
  await settle();
  assert.strictEqual(submits, postsBeforeDeferred);
  assert.strictEqual(findClass(mount, 'media-import-source').value, SOURCE_B);
  assert(!findClass(mount, 'media-import-decision-label').hidden);
  assert.strictEqual(findClass(mount, 'media-import-decision').value, '');
  assert(findClass(mount, 'media-import-start').disabled);
  const deferredChoice = findClass(mount, 'media-import-decision');
  deferredChoice.value = 'desktop';
  deferredChoice.listeners.change();
  confirmLocal = false;
  await findClass(mount, 'media-import-start').listeners.click();
  assert.strictEqual(submits, postsBeforeDeferred);
  confirmLocal = true;
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, postsBeforeDeferred + 1);
  assert.strictEqual(requests.filter(x => x.options.method === 'POST').slice(-1)[0].options.body,
    JSON.stringify({sourceId: SOURCE_B, videoDecision: 'desktop'}));
  delete batchProbeOverrides[SOURCE_B];
  delete batchProbeOverrides[SOURCE_C];
  delete batchProbeOverrides[SOURCE_D];
  batchSources = [SOURCE];

  // Encoding is offered only when the read-only video probe requires it.
  previewVideoCodec = 'vp9';
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE];
  submitMode = 'accepted';
  state = 'dispatching';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  select = findClass(mount, 'media-import-source');
  const encode = findClass(mount, 'media-import-local-reencode');
  const encodeChoice = findClass(mount, 'media-import-decision');
  assert(encode && !encode.checked && findClass(mount, 'media-import-encode-label').hidden,
    'there must be no generic re-encode checkbox');
  select.value = SOURCE;
  select.listeners.change();
  await settle();
  assert(!findClass(mount, 'media-import-decision-label').hidden,
    'incompatible video must present a choice');
  assert.strictEqual(encodeChoice.value, '',
    'there must be no implicit local-encoding approval');
  assert(findClass(mount, 'media-import-start').disabled);
  encodeChoice.value = 'import';
  encodeChoice.listeners.change();
  assert(encode.checked && !findClass(mount, 'media-import-start').disabled);
  const beforeLocal = submits;
  const confirmationsBefore = confirmCalls;
  confirmLocal = false;
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, beforeLocal,
    'refused local conversion confirmation must not dispatch');
  assert.strictEqual(confirmCalls, confirmationsBefore + 1);
  confirmLocal = true;
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, beforeLocal + 1);
  const lastPost = requests.filter(x => x.options.method === 'POST').slice(-1)[0];
  assert.strictEqual(lastPost.options.body,
    JSON.stringify({sourceId: SOURCE, videoDecision: 'local'}));
  assert(findClass(mount, 'media-import-local-reencode').disabled,
    'consent cannot change during an active operation');


  // Desktop/Skip must be chosen explicitly; no claim of a recorded VDR film.
  for (const [choice, expectedLabel] of [
    ['desktop', 'Für PC/HandBrake markiert'],
    ['skip', 'Übersprungen']
  ]) {
    context.resetMediaImportVisibility(true);
    state = 'succeeded';
    assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
    context.selectModule('media-import');
    await settle();
    const sourceSelect = findClass(mount, 'media-import-source');
    const choiceSelect = findClass(mount, 'media-import-decision');
    const toggle = findClass(mount, 'media-import-local-reencode');
    assert(choiceSelect && choiceSelect.value === 'import');
    sourceSelect.value = SOURCE;
    sourceSelect.listeners.change();
    await settle();
    assert.strictEqual(choiceSelect.value, '');
    assert(!findClass(mount, 'media-import-decision-label').hidden);
    choiceSelect.value = choice;
    choiceSelect.listeners.change();
    assert(findClass(mount, 'media-import-preview').textContent.includes('Kein Import'),
      'MT.UX4 marked decision preview must never describe a VDR import');
    assert(toggle.disabled && !toggle.checked);
    assert(findClass(mount, 'media-import-batch').disabled);
    const before = submits, priorConfirms = confirmCalls;
    confirmLocal = false;
    await findClass(mount, 'media-import-start').listeners.click();
    await settle();
    assert.strictEqual(submits, before);
    assert.strictEqual(confirmCalls, priorConfirms + 1);
    confirmLocal = true;
    await findClass(mount, 'media-import-start').listeners.click();
    await settle();
    assert.strictEqual(submits, before + 1);
    const post = requests.filter(x => x.options.method === 'POST').slice(-1)[0];
    assert.strictEqual(post.options.body, JSON.stringify({sourceId: SOURCE, videoDecision: choice}));
    assert(findClass(mount, 'status').textContent.includes(expectedLabel));
    assert(findClass(mount, 'status').textContent.includes('keine VDR-Aufnahme'));
    assert(findClass(mount, 'media-import-decision').disabled);
    assert(findClass(mount, 'media-import-operation').textContent.includes(OP));
  }
  context.resetMediaImportVisibility(true);
  state = 'executed_unverified';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const pendingSource = findClass(mount, 'media-import-source');
  pendingSource.value = SOURCE;
  pendingSource.listeners.change();
  await settle();
  const pendingChoice = findClass(mount, 'media-import-decision');
  pendingChoice.value = 'desktop';
  pendingChoice.listeners.change();
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes('Core-Bestätigung noch ausstehend'));
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, true);
  previewVideoCodec = 'h264';

  
  // The actual production module separates marked files from ordinary
  // import and dispatches a deliberate release via the existing v1 route.
  markedSources = [MARK_PC, MARK_SKIP, MARK_DUP];
  batchSources = [SOURCE];
  submitMode = 'accepted';
  for (const markedId of markedSources) {
    context.resetMediaImportVisibility(true);
    state = 'succeeded';
    assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
    context.selectModule('media-import');
    await settle();
    const ordinarySelect = findClass(mount, 'media-import-source');
    const markedSelect = findClass(mount, 'media-import-marked-source');
    const release = findClass(mount, 'media-import-release');
    assert(ordinarySelect && ordinarySelect.children.length === 2,
      'marked sources must not appear among ordinary imports');
    assert(markedSelect && markedSelect.children.length === 4);
    assert(release.disabled);
    markedSelect.value = markedId;
    markedSelect.listeners.change();
    assert.strictEqual(release.disabled, false);
    const before = submits;
    confirmLocal = false;
    await release.listeners.click();
    await settle();
    assert.strictEqual(submits, before, 'declined release must not dispatch');
    confirmLocal = true;
    await release.listeners.click();
    await settle();
    assert.strictEqual(submits, before + 1);
    const last = requests.filter(x => x.options.method === 'POST').slice(-1)[0];
    assert.strictEqual(last.options.body,
      JSON.stringify({sourceId: markedId, videoDecision: 'release'}));
    assert.strictEqual(last.options.headers['X-CSRF-Token'], 'test-csrf');
    assert(findClass(mount, 'status').textContent.includes('ausdrücklich freigegeben'));
    assert(findClass(mount, 'status').textContent.includes('noch nicht importiert'));
    assert(release.disabled, 'no duplicate release while Core operation is held');
  }

  // A bounded multi-page backend snapshot is fully retrieved for the current
  // session. Browsing pages cannot dispatch an import, and later IDs are
  // visible/selectable without arbitrary client filesystem paths.
  context.resetMediaImportVisibility(true);
  pagedSourceIds = Array.from({length: 70}, function (_, i) {
    return 'source_' + String(10000000000 + i);
  });
  markedSources = [];
  batchSources = null;
  const beforePagedPosts = requests.filter(r => r.options.method === 'POST').length;
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const allSourceSelect = findClass(mount, 'media-import-source');
  assert.strictEqual(allSourceSelect.children.length, 71);
  assert(allSourceSelect.children[70].textContent.includes(pagedSourceIds[69]));
  assert(requests.some(r => r.path.includes('?cursor=mi1_1_32')));
  assert(requests.some(r => r.path.includes('?cursor=mi1_1_64')));
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    beforePagedPosts, 'paginated scans must remain read-only');
  assert(findClass(mount, 'media-import-scan-notice').textContent.includes('Scan unvollständig'));
  allSourceSelect.value = pagedSourceIds[69];
  allSourceSelect.listeners.change();
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, false);
  // UX3: local search narrows only the already authorized source snapshot.
  // A search change invalidates a selection; only visible IDs may be batched.
  const search = findClass(mount, 'media-import-search');
  const filter = findClass(mount, 'media-import-filter');
  assert(search && filter, 'Media Import search and status filter are mounted');
  search.value = pagedSourceIds[69];
  search.listeners.input();
  assert.strictEqual(allSourceSelect.children.length, 2);
  assert.strictEqual(allSourceSelect.children[1].value, pagedSourceIds[69]);
  assert(findClass(mount, 'media-import-start').disabled,
    'changing search removes any earlier action selection');
  assert(findClass(mount, 'media-import-batch').disabled,
    'single search result cannot launch the previous 70-film batch');
  assert(findClass(mount, 'media-import-visible-count').textContent.includes('1 importierbare'));
  search.value = '';
  search.listeners.input();
  assert.strictEqual(allSourceSelect.children.length, 71);
  filter.value = 'marked';
  filter.listeners.change();
  assert.strictEqual(allSourceSelect.children.length, 1);
  assert(findClass(mount, 'media-import-source').disabled);
  filter.value = 'all';
  filter.listeners.change();
  assert.strictEqual(allSourceSelect.children.length, 71);
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    beforePagedPosts, 'search and status filter are read-only');
  // UX3: multi-select executes only the checked visible IDs after a separate
  // confirmation. The Core operation API is not contacted on cancellation.
  const checkRows = findClass(mount, 'media-import-selection');
  const selectedBatch = findClass(mount, 'media-import-batch-selected');
  const selectAllVisible = findClass(mount, 'media-import-select-all');
  assert(checkRows && selectedBatch && selectAllVisible);
  assert.strictEqual(checkRows.children.length, 70);
  assert(selectedBatch.disabled);
  checkRows.children[1].children[0].checked = true;
  checkRows.children[1].children[0].listeners.change();
  checkRows.children[2].children[0].checked = true;
  checkRows.children[2].children[0].listeners.change();
  assert.strictEqual(selectedBatch.disabled, false);
  confirmLocal = false;
  const beforeSelectionConfirmation = confirmCalls;
  await selectedBatch.listeners.click();
  await settle();
  assert.strictEqual(confirmCalls, beforeSelectionConfirmation,
    'selection diagnosis is strictly read-only');
  assert.strictEqual(findClass(mount, 'media-import-batch-review').children.length, 2);
  await findClass(mount, 'media-import-batch-confirm').listeners.click();
  assert.strictEqual(confirmCalls, beforeSelectionConfirmation + 1);
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    beforePagedPosts);
  confirmLocal = true;
  search.value = pagedSourceIds[69];
  search.listeners.input();
  assert(selectedBatch.disabled, 'filter changes revoke earlier multi-selection');
  assert.strictEqual(checkRows.children.length, 1);
  search.value = '';
  search.listeners.input();
  selectAllVisible.listeners.click();
  assert.strictEqual(findClass(mount, 'media-import-selection-count').textContent,
    '70 von 70 sichtbaren Filmen für den Stapel gewählt.');
  // The requested product boundary: exactly 100 films are offered.
  context.resetMediaImportVisibility(true);
  pagedSourceIds = Array.from({length: 100}, function (_, i) {
    return 'source_' + String(10000000000 + i);
  });
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const select100 = findClass(mount, 'media-import-source');
  assert.strictEqual(select100.children.length, 101);
  assert.strictEqual(findClass(mount, 'media-import-batch').disabled, false);
  assert(findClass(mount, 'media-import-scan-notice').textContent.includes('100 Filme'));
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    beforePagedPosts, 'listing 100 films is a read-only operation');

  // An invalid 101st item supplied by an incompatible server must not
  // become an executable selection or silently start a batch.
  context.resetMediaImportVisibility(true);
  pagedSourceIds.push('source_10000000100');
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  assert(findClass(mount, 'media-import-source').disabled);
  assert(findClass(mount, 'media-import-batch').disabled);
  assert(findClass(mount, 'media-import-scan-notice').textContent.includes('Maximal 100 Filme'));
  assert(findClass(mount, 'status').textContent.includes('höchstens 100 Filme'));
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    beforePagedPosts, '101 sources must never dispatch an import');
  pagedSourceIds = null;

  // REAL-VDR regression: the Agent may return shuffled episodes. The root
  // overview, single-select and batch checkboxes use identical natural order.
  // Folder names also sort numerically (Staffel 2 before Staffel 10).
  context.resetMediaImportVisibility(true);
  const shuffledIds = [
    'source_19000000010', 'source_19000000009', 'source_19000000002',
    'source_19000000001', 'source_29000000010', 'source_29000000002'
  ];
  const shuffledLabels = [
    'Andor - S02E10 - Mach, dass es aufhört',
    'Andor - S01E09 - Niemand hört zu',
    'Andor - S02E02 - Sagrona Teema',
    'Andor - S01E02 - Episode 2',
    'Zusatzfilm 10', 'Zusatzfilm 2'
  ];
  shuffledIds.forEach(function (id, index) {
    sourceDisplayLabels[id] = shuffledLabels[index];
    sourceFolders[id] = index < 4 ? 'Serien/Andor' :
      index === 4 ? 'Serien/Staffel 10' : 'Serien/Staffel 2';
  });
  pagedSourceIds = shuffledIds;
  const beforeSortedPosts = requests.filter(r => r.options.method === 'POST').length;
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const orderedIds = [
    shuffledIds[3], shuffledIds[1], shuffledIds[2], shuffledIds[0],
    shuffledIds[5], shuffledIds[4]
  ];
  const sortedSelect = findClass(mount, 'media-import-source');
  assert.strictEqual(sortedSelect.children.slice(1).map(function (option) {
    return option.value;
  }).join('|'), orderedIds.join('|'), 'source IDs remain tied to naturally sorted episodes');
  const sortedCheckboxes = findClass(mount, 'media-import-selection');
  assert.strictEqual(sortedCheckboxes.children.map(function (row) {
    return row.textContent;
  }).join('|'), orderedIds.map(function (id) {
    return sourceDisplayLabels[id];
  }).join('|'), 'batch checkboxes use the same order as single selection');
  const sortedFolders = findClass(mount, 'media-import-folders');
  sortedFolders.children[0].listeners.click(); // Serien
  assert.strictEqual(sortedSelect.children.slice(1).map(function (item) {
    return item.value;
  }).join('|'), orderedIds.join('|'),
  'selecting a series folder shows all of its descendant episodes immediately');
  assert.strictEqual(sortedFolders.children.map(function (child) {
    return child.textContent;
  }).join('|'), '📁 Andor|📁 Staffel 2|📁 Staffel 10');
  sortedFolders.children[0].listeners.click(); // Andor
  assert.strictEqual(sortedSelect.children.slice(1).map(function (item) {
    return item.value;
  }).join('|'), orderedIds.slice(0, 4).join('|'),
  'Andor directory lists seasons then episodes in numeric order');
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    beforeSortedPosts, 'local sorting and navigation never post imports');
  pagedSourceIds = null;
  shuffledIds.forEach(function (id) {
    delete sourceDisplayLabels[id];
    delete sourceFolders[id];
  });

  // UX2: trusted backend display alias is text-only presentation;
  // it never becomes a browser-supplied absolute root path or a new grant.
  context.resetMediaImportVisibility(true);
  sourceDisplayOverride = 'Standard: Filme · Die Herrschaft des Feuers';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const aliasSelect = findClass(mount, 'media-import-source');
  assert(aliasSelect.children[1].textContent.includes(sourceDisplayOverride));
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    beforePagedPosts, 'displaying root aliases must be read-only');
  sourceDisplayOverride = null;

  // UX3: folder drilldown uses ONLY the root-relative folder sent by the
  // Agent/Core public snapshot and never submits a filesystem path.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE, SOURCE_B];
  sourceFolders[SOURCE] = 'Fantasy';
  sourceFolders[SOURCE_B] = 'Serien/Andor';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const folderList = findClass(mount, 'media-import-folders');
  const breadcrumb = findClass(mount, 'media-import-breadcrumb');
  const folderSelect = findClass(mount, 'media-import-source');
  assert.strictEqual(folderSelect.children.length, 3,
    'recursive root overview includes films from all observed subfolders');
  assert.strictEqual(folderSelect.children[1].value, SOURCE);
  assert.strictEqual(folderSelect.children[2].value, SOURCE_B);
  assert(findClass(mount, 'media-import-visible-count').textContent.includes('2 importierbare'));
  assert.strictEqual(folderList.children.length, 2);
  const pendingRescan = findClass(mount, 'media-import-reload').listeners.click();
  assert(findClass(mount, 'media-import-scan-notice').textContent.includes('Scan läuft'));
  assert(findClass(mount, 'media-import-visible-count').textContent.includes('Scan läuft'));
  assert(folderSelect.disabled && folderSelect.children.length === 0,
    'a pending rescan never retains stale selectable options');
  await pendingRescan;
  await settle();
  assert.strictEqual(folderSelect.children.length, 3);
  folderList.children.find(function (child) {
    return child.textContent.includes('Serien');
  }).listeners.click(); // Serien
  assert.strictEqual(folderSelect.children.length, 2,
    'selecting the series parent must show episodes below it');
  assert.strictEqual(folderSelect.children[1].value, SOURCE_B);
  assert(findClass(mount, 'media-import-visible-count').textContent.includes('1 importierbare'));
  folderList.children[0].listeners.click(); // Andor
  assert.strictEqual(folderSelect.children.length, 2);
  assert.strictEqual(folderSelect.children[1].value, SOURCE_B);
  // UX4: preview only claims the authoritative source-relative folder;
  // destination/codec/size/capacity are not fabricated by the browser.
  folderSelect.value = SOURCE_B;
  folderSelect.listeners.change();
  await settle();
  await settle();
  const nestedPreview = findClass(mount, 'media-import-preview').textContent;
  assert(nestedPreview.includes('Quellordner: Serien › Andor.'));
  assert(nestedPreview.includes('VDR-Zielordner: Serien/Andor/Weiterer Film'),
    'authenticated backend preview displays the real selected target');
  assert(nestedPreview.includes('Video-Codec: h264') &&
    nestedPreview.includes('Frei:') && nestedPreview.includes('Benötigt'),
    'real read-only codec and capacity are visible');
  assert.strictEqual(breadcrumb.children.length, 3);
  breadcrumb.children[0].listeners.click(); // root
  assert.strictEqual(folderSelect.children.length, 3);
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    beforePagedPosts, 'folder browse is read-only');

  context.resetMediaImportVisibility(true);
  delete sourceFolders[SOURCE];
  sourceFolders[SOURCE_B] = '../escape';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  assert(findClass(mount, 'media-import-source').disabled);
  assert(findClass(mount, 'media-import-scan-notice').textContent.includes(
    'Scan nicht abgeschlossen'));
  delete sourceFolders[SOURCE_B];
  batchSources = null;

  // UX4.2: production selection -> v1 preview -> UI, including a delayed
  // response from film A after film B has become the current selection.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE, SOURCE_B];
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const raceSelect = findClass(mount, 'media-import-source');
  const racePosts = requests.filter(r => r.options.method === 'POST').length;
  previewMode = 'defer-source';
  raceSelect.value = SOURCE;
  raceSelect.listeners.change();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'Film wird geprüft'));
  assert.strictEqual(typeof resolveDelayedPreview, 'function');
  raceSelect.value = SOURCE_B;
  raceSelect.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'Backend geprüfter Andor-Film'));
  resolveDelayedPreview(reply(200, {
    backendId: 'backend-a', sourceId: SOURCE, state: 'ready', status: 'ready',
    title: 'Veralteter Film', targetFolder: 'Falsches Ziel', videoCodec: 'h264',
    sourceBytes: 1, availableBytes: 9000000000, requiredBytes: 5000000000,
    videoReencodeRequired: false
  }));
  resolveDelayedPreview = null;
  await settle();
  assert(!findClass(mount, 'media-import-preview').textContent.includes(
    'Veralteter Film'), 'late source A must never replace source B preview');

  // HTTP 202 is intentionally visible until the bounded next GET completes.
  previewMode = 'pending-first';
  raceSelect.value = SOURCE_B;
  raceSelect.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'Film wird geprüft'));
  const retryPreview = timerCallbacks[timerCallbacks.length - 1];
  retryPreview();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'VDR-Zielordner: Serien/Andor/Weiterer Film'));

  previewInspectionStatus = 'duplicate';
  raceSelect.value = SOURCE;
  raceSelect.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'Bereits als VDR-Aufnahme vorhanden'));
  assert(findClass(mount, 'media-import-start').disabled);

  previewInspectionStatus = 'insufficient';
  raceSelect.listeners.change();
  await settle();
  const insufficientPreview = findClass(mount, 'media-import-preview').textContent;
  assert(insufficientPreview.includes('Nicht genügend freier Platz') &&
    insufficientPreview.includes('Video-Codec: vp9'));
  assert(findClass(mount, 'media-import-start').disabled);
  assert(!findClass(mount, 'media-import-decision-label').hidden,
    'space pressure must not hide PC delegation for unsupported video');
  const insufficientDecision = findClass(mount, 'media-import-decision');
  insufficientDecision.value = 'import';
  insufficientDecision.listeners.change();
  assert(findClass(mount, 'media-import-start').disabled,
    'local encoding must stay blocked without space');
  insufficientDecision.value = 'desktop';
  insufficientDecision.listeners.change();
  assert(!findClass(mount, 'media-import-start').disabled,
    'PC marker must remain available despite a local space shortage');

  previewInspectionStatus = 'unavailable';
  raceSelect.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'konnte vom Backend nicht sicher geprüft werden'));
  assert(findClass(mount, 'media-import-start').disabled);

  previewInspectionStatus = 'ready';
  previewMode = 'forbidden';
  raceSelect.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'Keine Berechtigung für diese Importvorschau'));
  previewMode = 'ready';
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true,
    'authorization error must prevent import submission');
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    racePosts, 'all preview transitions are read-only');

  // A compatible film has no codec choice but retains an explicit optional
  // skip action. Even that marker uses the canonical Core operation.
  raceSelect.value = SOURCE;
  raceSelect.listeners.change();
  await settle();
  const compatibleSkip = findClass(mount, 'media-import-skip');
  assert(findClass(mount, 'media-import-decision-label').hidden);
  assert(compatibleSkip && !compatibleSkip.hidden && !compatibleSkip.disabled);
  state = 'succeeded';
  submitMode = 'accepted';
  const skipBefore = submits;
  confirmLocal = false;
  await compatibleSkip.listeners.click();
  assert.strictEqual(submits, skipBefore, 'cancelled skip creates no operation');
  confirmLocal = true;
  await compatibleSkip.listeners.click();
  await settle();
  assert.strictEqual(submits, skipBefore + 1);
  assert.strictEqual(requests.filter(x => x.options.method === 'POST').slice(-1)[0].options.body,
    JSON.stringify({sourceId: SOURCE, videoDecision: 'skip'}));

  // The alternative PC path must be dispatchable without enough local
  // staging space; it only marks the original, not a VDR recording.
  context.resetMediaImportVisibility(true);
  previewInspectionStatus = 'insufficient';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const spaceSelect = findClass(mount, 'media-import-source');
  spaceSelect.value = SOURCE;
  spaceSelect.listeners.change();
  await settle();
  const spaceDecision = findClass(mount, 'media-import-decision');
  assert(!findClass(mount, 'media-import-decision-label').hidden);
  spaceDecision.value = 'desktop';
  spaceDecision.listeners.change();
  assert(!findClass(mount, 'media-import-start').disabled);
  const pcBefore = submits;
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, pcBefore + 1);
  assert.strictEqual(requests.filter(x => x.options.method === 'POST').slice(-1)[0].options.body,
    JSON.stringify({sourceId: SOURCE, videoDecision: 'desktop'}));
  previewInspectionStatus = 'ready';

  // A revoked permission fails the whole read-only batch review without
  // any Core write. Controls recover so the user may run a fresh diagnosis.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE, SOURCE_B];
  previewMode = 'forbidden';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const postsBeforeForbidden = submits;
  await findClass(mount, 'media-import-batch').listeners.click();
  await settle();
  assert.strictEqual(submits, postsBeforeForbidden);
  assert(findClass(mount, 'media-import-batch-confirm').hidden);
  assert(!findClass(mount, 'media-import-batch').disabled,
    'failed read-only diagnosis should permit a new attempt');

  // A delayed GET must not resurrect a review after filter changes.
  previewMode = 'ready';
  const concurrentSelect = findClass(mount, 'media-import-source');
  concurrentSelect.value = SOURCE;
  concurrentSelect.listeners.change();
  await settle();
  assert(!findClass(mount, 'media-import-start').disabled);
  previewMode = 'defer-source';
  const staleBatch = findClass(mount, 'media-import-batch').listeners.click();
  await settle();
  assert.strictEqual(typeof resolveDelayedPreview, 'function');
  await findClass(mount, 'media-import-start').listeners.click();
  assert.strictEqual(submits, postsBeforeForbidden,
    'one-film POST must not interleave with a pending batch diagnosis');
  const staleSearch = findClass(mount, 'media-import-search');
  staleSearch.value = 'keine Übereinstimmung';
  staleSearch.listeners.input();
  resolveDelayedPreview(reply(200, {
    backendId: 'backend-a', sourceId: SOURCE, state: 'ready', status: 'ready',
    title: 'Alter Film', targetFolder: 'Altes Ziel', videoCodec: 'h264',
    sourceBytes: 1, availableBytes: 9000000000, requiredBytes: 5000000000,
    videoReencodeRequired: false
  }));
  resolveDelayedPreview = null;
  await staleBatch;
  await settle();
  assert(findClass(mount, 'media-import-batch-confirm').hidden);
  assert.strictEqual(findClass(mount, 'media-import-batch-review').children.length, 0);
  assert.strictEqual(submits, postsBeforeForbidden);
  previewMode = 'ready';

  console.log('A7.5 real web composition and media import lifecycle: PASS');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
