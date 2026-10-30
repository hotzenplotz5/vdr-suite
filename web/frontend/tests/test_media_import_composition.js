'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const frontend = path.join(__dirname, '..');
const index = fs.readFileSync(path.join(frontend, 'index.html'), 'utf8');
const app = fs.readFileSync(path.join(frontend, 'app.js'), 'utf8');
const view = fs.readFileSync(path.join(frontend, 'modules/media-import.js'), 'utf8');
const daemonRuntime = fs.readFileSync(path.join(frontend, '../../core/daemon/src/DaemonRuntimeInitialization.cpp'), 'utf8');
const previewOwner = daemonRuntime.split('registerMediaImportPreviewLookup(')[1]
  .split('registerMediaImportCapabilityLookup(')[0];
assert(previewOwner.includes('mayBrowseSources(') &&
  !previewOwner.includes('effectiveMediaImportCapability(') &&
  !previewOwner.includes('trustedMediaToolsProvider('),
  'read-only target preview must not depend on command advertisement or approval while an old command is quarantined');
assert(view.includes('TMDb-Zuordnung erneut versuchen') &&
  view.includes("http + detail + '. Aufnahme bleibt unverändert.'"),
  'failed TMDb target resolution offers retry and reports the actual preview error');
const stylesheet = fs.readFileSync(path.join(frontend, 'style.css'), 'utf8');
assert(stylesheet.includes('.media-import-panel[data-mobile-pane="folders"] .media-import-library'));
assert(stylesheet.includes('.media-import-panel[data-mobile-pane="media"] .media-import-episodes'));
assert(stylesheet.includes('.media-import-panel[data-mobile-pane="details"] .media-import-detail'));
assert(stylesheet.includes('.media-import-select-item .media-import-delete-duplicate-inline'),
  'marked duplicate list action remains visible and reachable at mobile widths');
assert(view.includes("view.attempt.videoDecision === 'release'") &&
  view.includes("refreshSelectedPreview(view, true);") &&
  view.includes('Für lokale VDR-Umkodierung freigeben'),
  'successful PC marker release opens the independently checked source without an automatic import');
assert(view.includes('media-import-existing-metadata') &&
  view.includes('importMetadataSelection(view, plan, source)') &&
  view.includes('applyImportedMetadata(view, repairAttempt)') &&
  view.includes('über den bestehenden Aufnahmeweg aktualisieren?'),
  'confirmed existing recording reuses the authenticated assignment for native info correction');
assert(view.includes('response.nativeInfoUpdated === true') &&
  view.includes('Native VDR-info noch nicht bestätigt.'),
  'the frontend claims native success only after the normal assignment reports write/readback');

const clientFactory = require('../api/public-v1-client.js');

assert(index.includes('data-module="media-import" hidden disabled'));
const nav = index.slice(index.indexOf('<nav id="module-nav"'),
                        index.indexOf('</nav>', index.indexOf('<nav id="module-nav"')));
assert(nav.indexOf('data-module="overview"') < nav.indexOf('data-module="recordings2"'));
assert(nav.indexOf('data-module="recordings2"') < nav.indexOf('data-module="media-import"'));
assert(nav.indexOf('data-module="media-import"') < nav.indexOf('data-module="genres"'),
  'MT.UX0 Media Import must sit directly after recordings');
// Exercise runtime recovery with a real child-order model, not just HTML text.
const placeTabSource = app.match(/function placeMediaImportTab\(\) \{[\s\S]*?\n\}/);
assert(placeTabSource, 'runtime tab placement must be implemented');
const runtimeNav = {
  children: [],
  querySelector(selector) {
    return selector.includes('recordings2') ? this.children.find(x => x.id === 'recordings2') : null;
  },
  insertBefore(node, next) {
    const old = this.children.indexOf(node);
    if (old >= 0) this.children.splice(old, 1);
    const nextIndex = next ? this.children.indexOf(next) : -1;
    this.children.splice(nextIndex >= 0 ? nextIndex : this.children.length, 0, node);
  }
};
const runtimeTabs = ['media-import', 'overview', 'channels2', 'recordings2', 'genres'].map(
  id => ({id, parentNode: runtimeNav}));
runtimeNav.children = runtimeTabs;
Object.defineProperty(runtimeTabs[3], 'nextElementSibling', {
  get() { return runtimeNav.children[runtimeNav.children.indexOf(this) + 1] || null; }
});
vm.runInNewContext(placeTabSource[0] + '\nplaceMediaImportTab();', {
  document: {getElementById(id) { return id === 'module-nav' ? runtimeNav : null; }},
  mediaImportTab: runtimeTabs[0]
});
assert.deepStrictEqual(runtimeNav.children.map(x => x.id),
  ['overview', 'channels2', 'recordings2', 'media-import', 'genres'],
  'runtime placement restores Home-first after an addon changes the DOM');
assert(index.indexOf('modules/media-import.js') < index.indexOf('frontend/app.js'));
assert(app.includes('refreshMediaImportVisibility(backendId);'));
assert(app.includes('resetMediaImportVisibility(true);'));
const capabilityUnavailable = app.match(/function unavailable\(\) \{([\s\S]*?)\n  \}/);
assert(capabilityUnavailable && capabilityUnavailable[1].includes('return true;'),
  'transient Agent capability failure must keep Media Import navigation available');
// The shell contains more than one Media Import branch. Inspect the
// navigation click handler, not the first unrelated occurrence in the file.
const mediaTabBranch = app.match(
  /if \(selectedModule === 'media-import'\) \{\n    \/\/ Navigation is independent[\s\S]*?\n  \}/);
assert(mediaTabBranch &&
  mediaTabBranch[0].includes('renderSelectedModule(currentSnapshot || {});') &&
  !mediaTabBranch[0].includes('refreshMediaImportVisibility('),
  'opening Media Import must render without any Agent capability roundtrip');
const importClickBody = view.split('function startImport(')[1]
  .split('function activate(')[0];
assert(!importClickBody.includes('checkCapability(view.backendId)') &&
  !importClickBody.includes('readSourcePages(view.backendId)') &&
  importClickBody.includes('submitMediaImport({'),
  'one confirmed import/delete click must reach Core without Agent preflight gates');
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
let cleanupState = '';
let submitMode = 'accepted';
let batchSources = null;
let markedSources = [];
let sourcePartial = false;
let sourceStale = false;
let sourceDisplayOverride = null;
const sourceFolders = Object.create(null);
const sourceDisplayLabels = Object.create(null);
const recordingStatuses = Object.create(null);
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
let holdOperationRead = false;
let releaseOperationRead = null;
let submits = 0;
let counter = 0;
const requests = [];
let sessionRestores = 0;
let previewMode = 'ready';
let previewInspectionStatus = 'ready';
let autoDuplicateReadback = false;
let previewVideoCodec = 'h264';
let previewArtworkMode = 'poster';
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
    if (status === 'unavailable') {
      // The real Agent legitimately returns unavailable with no title or
      // media details when its read-only source inspection fails.
      return Promise.resolve(reply(200, {
        backendId: 'backend-a', sourceId: id,
        state: 'ready', status: 'unavailable', title: '',
        targetFolder: '', videoCodec: '', sourceBytes: 0,
        availableBytes: 0, requiredBytes: 0,
        videoReencodeRequired: false,
        failureCode: override.failureCode || ''
      }));
    }
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
      videoReencodeRequired: codec === 'vp9',
      nfoOutline: 'Ein lokaler NFO-Text',
      nfoPlot: 'Eine Handlung aus der tatsächlichen .nfo-Datei.',
      nfoYear: '2024', nfoGenre: 'Drama',
      durationSeconds: 3210,
      posterDataUri: previewArtworkMode === 'bad'
        ? 'data:text/html;base64,PHNjcmlwdD4='
        : previewArtworkMode === 'none' || id !== SOURCE
        ? '' : 'data:image/jpeg;base64,/9j/2Q==',
      fanartDataUri: ''
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
            id === MARK_DUP ? 'Film.duplicate' :
              sourceDisplayLabels[id] || 'Weiterer Film',
          ...(Object.prototype.hasOwnProperty.call(sourceFolders, id)
            ? {folder: sourceFolders[id]} : {}),
          ...(recordingStatuses[id] ? {recordingStatus: recordingStatuses[id]} : {})};
      }), meta: {partial: sourcePartial, stale: sourceStale}})
      : reply(sourcesStatus, {detail: 'Source unavailable'}));
  }
  if (path.endsWith('/media-imports')) {
    ++submits;
    if (JSON.parse(options.body).videoDecision === 'delete_duplicate') {
      const targetId = JSON.parse(options.body).sourceId;
      // Only the successful Core/Agent result removes the fixture source.
      // An outcome_unknown cleanup has no confirmed deletion.
      if (state === 'succeeded') {
        markedSources = markedSources.filter(function (id) {
          return id !== targetId;
        });
        batchSources = (batchSources || [SOURCE]).filter(function (id) {
          return id !== targetId;
        });
      }
      return Promise.resolve(reply(202, {
        operationId: OP, backendId: 'backend-a', state: 'dispatching'
      }));
    }
    if (JSON.parse(options.body).videoDecision === 'duplicate') {
      // The existing Core Operation handles this marker, not the batch import.
      if (autoDuplicateReadback) {
        batchSources = (batchSources || [SOURCE]).filter(function (id) {
          return id !== JSON.parse(options.body).sourceId;
        });
        if (!markedSources.includes(MARK_DUP)) markedSources.push(MARK_DUP);
      }
      return Promise.resolve(reply(202, {
        operationId: OP, backendId: 'backend-a', state: 'dispatching'
      }));
    }
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
    if (holdOperationRead) return new Promise(function (resolve) {
      releaseOperationRead = resolve;
    });
    return Promise.resolve(reply(200, {operationId: OP, backendId: 'backend-a',
      state: state, sourceCleanup: cleanupState || undefined,
      links: {self: path}}, {'ETag': '"revision-1"'}));
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
    // The real app's extracted runtime gate invokes placeMediaImportTab().
    // Model that native DOM lookup instead of throwing during VM startup.
    getElementById(id) { return id === 'module-nav' ? runtimeNav : null; },
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
// MT.META5 uses the real established metadata route shape from the
// production Media Import owner, never an extra import/metadata backend.
const onlineRequests = [];
const metadataAssignments = [];
let metadataFolderOverride = null;
let nativeAssignmentAck = false;
let onlineMode = 'ready';
let resolveOnlineReply = null;
function onlineCandidate(kind, id, title, extra) {
  return Object.assign({
    providerId: 'tmdb', kind: kind,
    externalNamespace: {movie:'movie', series:'tv',
      season:'tv-season', episode:'tv-episode'}[kind],
    externalId: id, parentExternalId: '',
    title: title, overview: 'Echte Provider-Vorschau ' + title,
    releaseDate: '2024-01-01', posterReference: '/preview.jpg',
    seasonNumber: 0, episodeNumber: 0
  }, extra || {});
}
function onlinePage(candidates) {
  return {providerId: 'tmdb', attempted: true,
    providerAvailable: true, candidates: candidates};
}
context.VdrSuiteClientApi = {
  fetchClientRecordingFolder(options) {
    const path = options.query.path;
    return Promise.resolve(metadataFolderOverride || {
      path: path, cacheReady: true, recordingCount: 1,
      recordings: [{
        title: 'Andor S01E01',
        backendNativeId: '/srv/vdr/video.00/' + path + '/2026-10-28.17.06.1-0.rec'
      }]
    });
  },
  requestJson(route, options) {
    if (route.endsWith('/recordings/metadata/assign')) {
      metadataAssignments.push({route, options});
      return Promise.resolve({assigned: true, nativeInfoUpdated: nativeAssignmentAck});
    }
    onlineRequests.push({route, options});
    assert.strictEqual(options.method, 'POST');
    assert.strictEqual(options.headers['X-CSRF-Token'], 'test-csrf');
    assert.strictEqual(options.credentials, 'same-origin');
    if (onlineMode === 'offline') return Promise.reject({status: 503});
    if (onlineMode === 'forbidden') return Promise.reject({status: 403});
    if (onlineMode === 'defer') {
      return new Promise(function (resolve) { resolveOnlineReply = resolve; });
    }
    const body = JSON.parse(options.body);
    if (route.endsWith('/search')) {
      if (onlineMode === 'ambiguous' && body.kind === 'series')
        return Promise.resolve(onlinePage([
          onlineCandidate('series','1399','Andor'),
          onlineCandidate('series','1400','Andor')]));
      return Promise.resolve(body.kind === 'series'
        ? onlinePage([onlineCandidate('series','1399','Andor')])
        : onlinePage([
          onlineCandidate('movie','13','Film mit belegter TMDb-ID'),
          onlineCandidate('movie','14','Falscher Treffer', {providerId:'unknown'}),
          onlineCandidate('movie','15','Anderer Film')]));
    }
    if (route.endsWith('/seasons'))
      return Promise.resolve(onlinePage([onlineCandidate('season','3624','Staffel 1',
        {parentExternalId:'1399',seasonNumber:1})]));
    if (route.endsWith('/episodes'))
      return Promise.resolve(onlinePage([
        onlineCandidate('episode','222','Falsche Folge',
          {parentExternalId:'1399',seasonNumber:1,episodeNumber:2}),
        onlineCandidate('episode','221','Pilot',
          {parentExternalId:'1399',seasonNumber:1,episodeNumber:1})]));
    throw Error('Unexpected online route: '+route);
  }
};
context.window = context;
vm.createContext(context);
// Exercise the real module's internal metadata handoff without exporting a
// production testing hook or modifying the official browser API.
vm.runInContext(view.replace('})(window);',
  "global.__meta6Contract = {importMetadataSelection,applyImportedMetadata,frozenImportMetadata," +
  "resolveExistingRecordingMetadataSelection,activateForTest:function(v){" +
  "current=v;owner=1;}};})(window);"),
  context);

// The existing Recording metadata assignment is the only metadata writer.
async function verifyMeta6Handoff() {
  const metaView = {
    onlineKind: 'series',
    onlineSeries: {
      kind: 'series', providerId: 'tmdb', externalId: '1399',
      title: 'Andor', posterReference: '/series-poster.jpg'
    },
    onlineSelected: onlineCandidate('episode', '221', 'Kassa', {
      parentExternalId: '1399', seasonNumber: 1, episodeNumber: 1
    })
  };
  const selected = context.__meta6Contract.importMetadataSelection(metaView,
    {targetFolder: 'Serien/Andor/Andor S01E01', title: 'Andor S01E01'},
    {folder: 'Serien/Andor', label: 'Andor S01E01'});
  assert(selected && selected.candidate.mediaType === 'episode' &&
    selected.candidate.posterReference === '/series-poster.jpg',
    'exact TMDb episode uses the known series cover for a Recording assignment');
  const wrongEpisode = Object.assign({}, metaView, {
    onlineSelected: Object.assign({}, metaView.onlineSelected, {episodeNumber: 9})
  });
  assert.strictEqual(context.__meta6Contract.importMetadataSelection(
    wrongEpisode, {targetFolder:'Serien/Andor/Andor S01E01', title:'Andor S01E01'},
    {folder:'Serien/Andor',label:'Andor S01E01'}), null,
    'different provider episode must never assign metadata to this import');
  assert.strictEqual(context.__meta6Contract.importMetadataSelection(
    Object.assign({}, metaView, {onlineSeries: Object.assign(
      {}, metaView.onlineSeries, {externalId:'1400'})}),
    {targetFolder:'Serien/Andor/Andor S01E01', title:'Andor S01E01'},
    {folder:'Serien/Andor',label:'Andor S01E01'}), null,
    'a candidate from a different series must not be transferred');
  const targetView = {backendId:'backend-a'};
  const attempt = {metadataSelection:selected, metadataFeedback:''};
  await context.__meta6Contract.applyImportedMetadata(targetView, attempt);
  assert(attempt.metadataFeedback.includes('gespeichert') &&
    attempt.metadataFeedback.includes('noch nicht bestätigt') &&
    metadataAssignments.length === 1,
    'Suite assignment without native readback cannot claim native info success');
  const assignment = metadataAssignments[0];
  const data = JSON.parse(assignment.options.body);
  assert.strictEqual(data.externalId, '221');
  assert.strictEqual(data.seriesTitle, 'Andor');
  assert.strictEqual(data.posterReference, '/series-poster.jpg');
  assert.strictEqual(data.resourceKey,
    '/srv/vdr/video.00/Serien/Andor/Andor S01E01/2026-10-28.17.06.1-0.rec');
  assert.strictEqual(assignment.options.headers['X-CSRF-Token'], 'test-csrf');
  const existingSnapshot = context.__meta6Contract.frozenImportMetadata(metaView, selected);
  assert.strictEqual(decodeURIComponent(existingSnapshot.split('|')[9]),
    '/series-poster.jpg',
    'frozen v2 import carries the same already selected series poster, not the episode still');
  metadataFolderOverride = {path:selected.folder,cacheReady:true,recordingCount:2,
    recordings:[{title:selected.title,backendNativeId:'/one.rec'},
      {title:selected.title,backendNativeId:'/two.rec'}]};
  const ambiguous = {metadataSelection:selected,metadataFeedback:''};
  await context.__meta6Contract.applyImportedMetadata(targetView, ambiguous);
  assert(ambiguous.metadataFeedback.includes('nicht übernommen') &&
    metadataAssignments.length === 1,
    'ambiguous target may not create an assignment against an unrelated recording');
  metadataFolderOverride = null;
  nativeAssignmentAck = true;
  const confirmedNative = {metadataSelection: selected, metadataFeedback:''};
  await context.__meta6Contract.applyImportedMetadata(targetView, confirmedNative);
  assert(confirmedNative.metadataFeedback.includes('native VDR-info aktualisiert') &&
    metadataAssignments.length === 2,
    'one existing authenticated assignment can confirm real native info readback');
  nativeAssignmentAck = false;
  // MT.META6: the inventory-backed duplicate card has no targetFolder.
  // Only an explicit metadata action may resolve it using the existing
  // read-only Agent preview; it must not start a media import.
  const viewForRepair = Object.assign({}, metaView, {
    backendId: 'backend-a', owner: 1, previewGeneration: 4,
    select: {value: SOURCE}
  });
  context.__meta6Contract.activateForTest(viewForRepair);
  const originalStatus = previewInspectionStatus;
  previewInspectionStatus = 'duplicate';
  const initialPosts = submits;
  const resolution = await context.__meta6Contract
    .resolveExistingRecordingMetadataSelection(viewForRepair,
      {sourceId: SOURCE, status: 'duplicate', title: 'Andor S01E01',
       targetFolder: ''},
      {sourceId: SOURCE, folder: 'Serien/Andor', label: 'Andor S01E01'});
  assert(resolution && resolution.folder === 'Die Herrschaft des Feuers' &&
    resolution.candidate.externalId === '221',
    'explicit repair resolves the exact backend target for an already imported episode');
  assert.strictEqual(submits, initialPosts,
    'metadata target resolution does not submit any import or marker');
  viewForRepair.select.value = '';
  await assert.rejects(context.__meta6Contract
    .resolveExistingRecordingMetadataSelection(viewForRepair,
      {status: 'duplicate', targetFolder: ''},
      {sourceId: SOURCE, folder: 'Serien/Andor', label: 'Andor S01E01'}),
    'changed source selection must cancel metadata target resolution');
  previewInspectionStatus = originalStatus;
  context.__meta6Contract.activateForTest(null);
}

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
  await verifyMeta6Handoff();
  // Real app capability-gate -> production module selection -> actual DOM buttons.
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  assert.strictEqual(tab.hidden, false);
  // An intermittent Agent lease loss is only a status observation.
  // Media Import remains navigable and the previously loaded view survives.
  allow = false;
  const capabilityCallbacksBefore = timerCallbacks.length;
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true,
    'temporary Agent capability loss must not block navigation');
  assert.strictEqual(tab.hidden, false);
  assert.strictEqual(tab.disabled, false);
  assert(tab.title.includes('Agent-Status momentan unbestätigt'));
  assert.strictEqual(timerCallbacks.length, capabilityCallbacksBefore,
    'a missing capability must not schedule an endless five-second gate retry');
  context.selectModule('media-import');
  await settle();
  let sourceListOnTransientFailure = findClass(mount, 'media-import-source');
  assert(sourceListOnTransientFailure && sourceListOnTransientFailure.children.length > 1,
    'an installed media import view must still list sources during a transient capability failure');
  const listBeforeRecovered = sourceListOnTransientFailure;
  allow = true;
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  assert.strictEqual(tab.hidden, false);
  assert.strictEqual(tab.disabled, false);
  assert.strictEqual(findClass(mount, 'media-import-source'), listBeforeRecovered,
    'capability recovery must never remount an already browsable source list');
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
  assert.strictEqual(findClass(mount, 'media-import-panel').dataset.mobilePane, 'details',
    'choosing a source opens its detail view on phones');
  assert(start.disabled, 'pending preview must not enable an import');
  const noPreviewPost = submits;
  await start.listeners.click();
  assert.strictEqual(submits, noPreviewPost, 'an early click must not bypass diagnosis');
  await settle();
  assert.strictEqual(start.disabled, false);
  assert.strictEqual(start.textContent, 'Diesen Film importieren',
    'single-file action must state its effect');
  const importPanel = findClass(mount, 'media-import-panel');
  const threePanes = findClass(importPanel, 'media-import-workspace');
  const detailPane = findClass(importPanel, 'media-import-detail');
  assert(threePanes && threePanes.children.length === 3 &&
    findClass(threePanes, 'media-import-library') &&
    findClass(threePanes, 'media-import-episodes') && detailPane,
    'production module composes genuine folder/episode/detail columns');
  // MT.META1: Mobile navigation changes the visible pane only, with no
  // second import route, selection or backend mutation.
  const mobileNav = findClass(mount, 'media-import-mobile-nav');
  assert(mobileNav && mobileNav.children.length === 3);
  assert.strictEqual(importPanel.dataset.mobilePane, 'details');
  assert.strictEqual(mobileNav.children[2].attributes['aria-pressed'], 'true');
  const mobilePosts = requests.filter(r => r.options.method === 'POST').length;
  await mobileNav.children[0].listeners.click();
  assert.strictEqual(importPanel.dataset.mobilePane, 'folders');
  assert.strictEqual(mobileNav.children[0].attributes['aria-pressed'], 'true');
  assert.strictEqual(mobileNav.children[1].attributes['aria-pressed'], 'false');
  await mobileNav.children[2].listeners.click();
  assert.strictEqual(importPanel.dataset.mobilePane, 'details');
  await findClass(mount, 'media-import-mobile-back').listeners.click();
  assert.strictEqual(importPanel.dataset.mobilePane, 'media');
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    mobilePosts, 'view navigation must never submit or mark sources');
  const previewCard = findClass(mount, 'media-import-preview-card');
  assert(previewCard && detailPane.children.indexOf(previewCard) >= 0,
    'the visible preview card is a first-class element of the detail pane');
  const technicalRecovery = findClass(mount, 'media-import-technical-details');
  const markedRecovery = findClass(mount, 'media-import-marked-label');
  const inlineActions = findClass(previewCard, 'media-import-preview-actions');
  const inlineFeedback = findClass(previewCard, 'media-import-import-feedback');
  assert(inlineActions &&
    findClass(inlineActions, 'media-import-start') === start &&
    inlineFeedback && inlineFeedback.hidden &&
    detailPane.children.indexOf(technicalRecovery) >
      detailPane.children.indexOf(previewCard) &&
    technicalRecovery.children.includes(markedRecovery) &&
    findClass(technicalRecovery, 'media-import-operation') &&
    !detailPane.children.includes(markedRecovery),
    'one existing import action sits beside readiness; operation details stay collapsed');
  assert(findClass(mount, 'media-import-preview-facts'),
    'codec, source bytes and VDR target are rendered as actual preview facts');
  const artwork = findClass(mount, 'media-import-artwork');
  assert(artwork && artwork.children.length === 2,
    'poster and fanart have independent states in the actual detail DOM');
  assert(findClass(artwork, 'media-import-artwork-image').src ===
    'data:image/jpeg;base64,/9j/2Q==');
  assert(findClass(artwork, 'media-import-artwork-empty').textContent ===
    'Keine lokale Fanart');
  // Malformed MIME cannot become a live image src or a second network URL.
  previewArtworkMode = 'bad';
  await findClass(mount, 'media-import-preview-retry').listeners.click();
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-artwork-image'), null,
    'invalid image type must fall back to neutral placeholders');
  previewArtworkMode = 'poster';
  await findClass(mount, 'media-import-preview-retry').listeners.click();
  await settle();
  assert(findClass(mount, 'media-import-artwork-image'),
    're-inspection restores only genuinely observed artwork');
  // Filtering away the selected source also retires its preview image.
  const artworkSearch = findClass(mount, 'media-import-search');
  artworkSearch.value = 'kein solches medium';
  artworkSearch.listeners.input();
  assert.strictEqual(findClass(mount, 'media-import-artwork-image'), null,
    'stale poster never survives a source/filter change');
  artworkSearch.value = '';
  artworkSearch.listeners.input();
  select.value = SOURCE;
  select.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-artwork-image'),
    'new verified source inspection repopulates real artwork');
  const nfo = findClass(mount, 'media-import-local-metadata');
  assert(nfo && findClass(nfo, 'media-import-nfo-outline').textContent ===
    'Ein lokaler NFO-Text');
  assert(findClass(nfo, 'media-import-nfo-plot').textContent.includes('.nfo-Datei'));
  assert(nfo.children.some(function (child) {
    return child.className === 'media-import-preview-facts' &&
      child.children.some(function (fact) {
        return fact.children[1].textContent === 'Drama';
      });
  }), 'NFO genre and year are displayed as verified metadata');
  assert(findClass(mount, 'media-import-preview-facts').children.some(
    function (fact) { return fact.children[0].textContent === 'Laufzeit (FFprobe)'; }),
    'FFprobe duration must be real data in the detail pane');
  assert(findClass(mount, 'media-import-preview').textContent.includes('Nächster Schritt'),
    'ready preview must explain the next action');
  assert(findClass(mount, 'media-import-decision-label').hidden,
    'compatible source needs no encoding decision');
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'technisch für den VDR-Import geeignet'));
  assert(findClass(mount, 'media-import-preview-state').textContent ===
    'Direktimport möglich', 'compatible source has a clear direct-import status, not an encoding form');
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
  assert.strictEqual(findClass(mount, 'media-import-import-title').textContent,
    'Import läuft', 'processing state appears directly in the preview');
  assert(findClass(mount, 'media-import-import-description').textContent.includes(
    'Aufnahme wird verarbeitet'), 'the status uses ordinary language without backend jargon');
  assert(findClass(inlineFeedback, 'media-import-refresh') &&
    findClass(inlineFeedback, 'media-import-new'),
    'status controls are colocated with the existing operation');
  assert.strictEqual(start.disabled, true);
  await start.listeners.click();
  assert.strictEqual(submits, 1, 'double-click must not submit a second import');

  // Film B remains searchable and technically inspectable while film A runs.
  const runningProgress = findClass(mount, 'media-import-operation-progress');
  assert(runningProgress && !runningProgress.hidden,
    'running import has an always-visible native progress bar');
  assert.strictEqual(findClass(runningProgress,
    'media-import-operation-progress-bar').tagName, 'PROGRESS');
  assert(findClass(runningProgress,
    'media-import-operation-progress-text').textContent.includes('Verstrichen:'),
    'elapsed time is displayed, never an invented percentage');
  batchSources = [SOURCE, SOURCE_B];
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  const nextFilm = findClass(mount, 'media-import-source');
  assert.strictEqual(nextFilm.disabled, false,
    'source selection stays active while A imports');
  assert.strictEqual(findClass(mount, 'media-import-search').disabled, false,
    'search stays active while A imports');
  assert.strictEqual(findClass(mount, 'media-import-filter').disabled, false,
    'filter stays active while A imports');
  nextFilm.value = SOURCE_B;
  nextFilm.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-preview-state').textContent.includes(
    'Direktimport möglich'), 'film B preview works while film A imports');
  assert(findClass(mount, 'media-import-start').disabled,
    'film B inspection does not dispatch another import');
  assert.strictEqual(submits, 1, 'preview of film B never repeats import A');
  assert(!findClass(mount, 'media-import-operation-progress').hidden,
    'operation A remains visible while inspecting B');
  const runningSearch = findClass(mount, 'media-import-search');
  runningSearch.value = 'Weiterer';
  runningSearch.listeners.input();
  assert.strictEqual(findClass(mount, 'media-import-source').children.length, 2,
    'search filters the inventory during import');
  runningSearch.value = '';
  runningSearch.listeners.input();
  batchSources = null;

  // A manual read must produce an observable result even if Core reports
  // exactly the same dispatching state. It is never a new import POST.
  const checkedBefore = operationReads;
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert.strictEqual(operationReads, checkedBefore + 1);
  assert(findClass(mount, 'media-import-import-description').textContent.includes(
    'Zuletzt geprüft:'), 'manual refresh visibly confirms the returned Core state');
  assert.strictEqual(submits, 1);

  // When automatic polling is already fetching, the click must queue one
  // additional read of the same durable operation instead of being dropped.
  holdOperationRead = true;
  const pendingStatus = findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert(releaseOperationRead, 'first status query is in flight');
  const statusReadsBeforeQueue = operationReads;
  await findClass(mount, 'media-import-refresh').listeners.click();
  assert(findClass(mount, 'media-import-import-description').textContent.includes(
    'nach der laufenden Abfrage'), 'the user sees the queued manual check');
  holdOperationRead = false;
  const finishHeldRead = releaseOperationRead;
  releaseOperationRead = null;
  finishHeldRead(reply(200, {operationId: OP, backendId: 'backend-a',
    state: 'dispatching', links: {self:'/api/v1/operations/' + OP}}));
  await pendingStatus;
  await settle();
  assert.strictEqual(operationReads, statusReadsBeforeQueue + 1,
    'queued click requests exactly one follow-up read');
  assert(findClass(mount, 'media-import-import-description').textContent.includes(
    'Zuletzt geprüft:'), 'queued check finishes with visible result');
  assert.strictEqual(submits, 1,
    'rechecking an existing operation cannot submit a new import');

  // Longer imports exceeded the old 15-read / 60-second automatic status
  // window, losing their final confirmation until a manual user click.
  const timerCountBeforeLongImport = timerCallbacks.length;
  const readsBeforeLongImport = operationReads;
  for (let step = 0; step < 17; ++step) {
    await findClass(mount, 'media-import-refresh').listeners.click();
    await settle();
  }
  assert.strictEqual(operationReads, readsBeforeLongImport + 17);
  assert(timerCallbacks.length >= timerCountBeforeLongImport + 17,
    'a long-running single import continues automatic polling after 15 reads');
  assert.strictEqual(submits, 1,
    'status monitoring must never create a second import operation');

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
  assert(findClass(mount, 'media-import-import-title').textContent.includes(
    'Ergebnis ungewiss'), 'unknown outcomes must never look like a success');
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, true);
  // An uncertain earlier operation may never be silently retried. It must
  // also not lock source selection and *read-only* preview for other films.
  assert.strictEqual(findClass(mount, 'media-import-source').disabled, false,
    'unknown operation does not disable harmless film selection');
  assert.strictEqual(findClass(mount, 'media-import-reload').disabled, false,
    'unknown operation does not disable read-only source refresh');
  assert.strictEqual(findClass(mount, 'media-import-search').disabled, false,
    'unknown operation does not disable search');
  assert.strictEqual(findClass(mount, 'media-import-filter').disabled, false,
    'unknown operation does not disable filter');
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true,
    'no second import while Core outcome remains unknown');
  const unknownDiagnostic = findClass(mount, 'media-import-operation-diagnostics');
  assert(unknownDiagnostic && unknownDiagnostic.textContent.includes(
    'Core-Status: outcome_unknown'), 'technical details show the actual Core state');
  assert(unknownDiagnostic.textContent.includes('Operation: ' + OP),
    'technical details correlate the exact original Operation');
  assert(unknownDiagnostic.textContent.includes('Agent-Ausführungsstufe'),
    'UI explicitly reports unavailable Agent/filename evidence instead of guessing');
  assert(findClass(mount, 'media-import-import-description').textContent.includes(
    'Betroffener Auftrag:'), 'unknown action is tied to the original film');
  const unknownPosts = submits;
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  const whileUnknown = findClass(mount, 'media-import-source');
  whileUnknown.value = SOURCE;
  whileUnknown.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-preview-state').textContent.includes(
    'Direktimport möglich'), 'another source can be inspected while uncertain');
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true);
  assert.strictEqual(submits, unknownPosts,
    'read-only browsing may not create another mutation');
  state = 'succeeded';
  recordingStatuses[SOURCE] = 'recording_present';
  const beforeConfirmedScan = requests.filter(r =>
    r.path.endsWith('/media-import-sources')).length;
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert(requests.filter(r => r.path.endsWith('/media-import-sources')).length >
    beforeConfirmedScan,
    'completed VDR readback automatically refreshes the inventory');
  const confirmedRow = findClass(mount, 'media-import-selection').children.find(
    row => row.dataset.sourceId === SOURCE);
  assert(confirmedRow && confirmedRow.children[2].textContent.includes(
    'Bereits importiert'), 'new duplicate status appears without page reload');
  assert.strictEqual(submits, 1, 'refresh is read-only, not a second import');
  assert(findClass(mount, 'status').textContent.includes('unabhängig bestätigt'));
  assert.strictEqual(findClass(mount, 'media-import-import-title').textContent,
    'Import abgeschlossen ✓', 'verified VDR readback is shown near the action');
  assert(findClass(mount, 'media-import-import-feedback').className.includes('is-success'),
    'only Core-confirmed success gets a visible green completion state');
  assert(findClass(mount, 'media-import-completed-notice').textContent.includes(
    'VDR-Aufnahme bestätigt'), 'a confirmed recording also appears in a persistent completion receipt');
  // A finished import is retired automatically when the next episode is
  // selected; the old "Neuen Import vorbereiten" button is not required.
  batchSources = [SOURCE, SOURCE_B];
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  const nextAfterSuccess = findClass(mount, 'media-import-source');
  nextAfterSuccess.value = SOURCE_B;
  nextAfterSuccess.listeners.change();
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-new').hidden, true,
    'no manual reset of the previous successful import');
  assert.strictEqual(findClass(mount, 'media-import-refresh').hidden, true,
    'old successful operation no longer offers a status recheck for film B');
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, false,
    'film B is importable immediately after its own verified preview');
  assert.strictEqual(submits, 1,
    'selecting film B does not itself start a new import');
  assert(findClass(mount, 'media-import-completed-notice').textContent.includes(
    'Import abgeschlossen'), 'past completion remains visible as a receipt');
  delete recordingStatuses[SOURCE];
  batchSources = null;
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  context.selectModule('overview');
  context.selectModule('media-import');
  await settle();
  assert(findClass(mount, 'media-import-completed-notice').textContent.includes(
    'VDR-Aufnahme bestätigt'), 'same authenticated session retains the confirmed completion');

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

  // Explicit logout still closes the navigation and clears session
  // state. A transient Agent outage never has that authority.
  context.resetMediaImportVisibility(true);
  vm.runInContext('browserSessionFrontendActive = false;', context);
  assert.strictEqual(tab.hidden, true);
  allow = false;
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), false);
  assert.strictEqual(tab.hidden, true);
  context.selectModule('media-import');
  assert.strictEqual(tab.hidden, true);
  vm.runInContext('browserSessionFrontendActive = true;', context);
  // A new authenticated session may open installed Media Tools even before
  // the Agent's first capability result recovers.
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true,
    're-authenticated session may navigate without an Agent capability');
  assert.strictEqual(tab.hidden, false);
  assert.strictEqual(tab.disabled, false);
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
  const beforeRejectedPost = submits;
  const beforeRejectedReads = requests.filter(function (req) {
    return req.path.endsWith('/media-import-sources') &&
      req.options.method !== 'POST';
  }).length;
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, beforeRejectedPost + 1,
    'HTTP 422 sent exactly one explicitly approved import POST');
  assert(findClass(mount, 'status').textContent.includes('HTTP 422') &&
    findClass(mount, 'status').textContent.includes('Metadatenformat'),
    'server rejection must not be mislabeled as a proven expired source');
  assert(requests.filter(function (req) {
    return req.path.endsWith('/media-import-sources') &&
      req.options.method !== 'POST';
  }).length > beforeRejectedReads,
  'a confirmed 422 automatically reloads inventory through GET only');
  assert.strictEqual(submits, beforeRejectedPost + 1,
    'refresh after rejection never automatically retries a mutation');
  assert.strictEqual(findClass(mount, 'media-import-source').disabled, false,
    'new source selection is available without a manual reload');
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true,
    'fresh source and preview still need an explicit selection');
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

  // A 200 response containing the last Agent-observed catalog must remain
  // visibly stale, without discarding an already selected film or its preview.
  sourceStale = true;
  const staleSelection = findClass(mount, 'media-import-source');
  staleSelection.value = SOURCE;
  staleSelection.listeners.change();
  await settle();
  const stalePosts = submits;
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  assert.strictEqual(staleSelection.value, SOURCE,
    'read-only stale inventory does not clear the selected episode');
  assert(findClass(mount, 'media-import-scan-notice').textContent.includes(
    'Zuletzt geladene Quellenliste'));
  assert.strictEqual(submits, stalePosts,
    'stale read never retries a mutation');
  sourceStale = false;

  // Agent command/lease trouble must NOT wipe an already loaded library.
  // The next scan may return 503, but selection, folders, and source rows
  // remain navigable. A mutation is not retried by this read-only fallback.
  sourcesStatus = 503;
  const inventoryBefore503 = findClass(mount, 'media-import-selection').children.length;
  const postsBefore503 = submits;
  select = findClass(mount, 'media-import-source');
  select.value = SOURCE;
  select.listeners.change();
  await settle();
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes('bleibt erhalten'));
  assert(findClass(mount, 'media-import-scan-notice').textContent.includes(
    'Letzte erfolgreich geladene'));
  assert.strictEqual(findClass(mount, 'media-import-selection').children.length,
    inventoryBefore503, 'an unsuccessful scan may not erase the list');
  assert.strictEqual(findClass(mount, 'media-import-source').value, SOURCE,
    'failed refresh must retain the selected film');
  assert.strictEqual(submits, postsBefore503,
    'preserving the failed inventory is strictly read-only');
  context.selectModule('overview');
  context.selectModule('media-import');
  await settle();
  assert(findClass(mount, 'media-import-source').children.length > 1,
    'last verified source list survives a tab switch during Agent 503');
  assert.strictEqual(submits, postsBefore503,
    'reopening a cached inventory never executes any mutation');

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
  assert(findClass(mount, 'media-import-import-title').textContent.includes(
    'Anfrage wird übermittelt'), 'pending acceptance is visible beside the preview');
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
  assert(findClass(mount, 'media-import-import-title').textContent,
    'operation status stays visible after owner replacement');
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
  autoDuplicateReadback = true;
  state = 'succeeded';
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
    return row.textContent.includes('Als .duplicate markiert');
  }));
  assert.strictEqual(submits, postsBeforeMixed + 1,
    'only the proven duplicate is marked automatically, never VDR imported');
  assert.strictEqual(batchOperations.length, 0,
    'duplicate marker does not use a second import queue');
  const mixedReviewConfirm = findClass(mount, 'media-import-batch-confirm');
  const beforeMixedConfirm = confirmCalls;
  confirmLocal = false;
  await mixedReviewConfirm.listeners.click();
  assert.strictEqual(submits, postsBeforeMixed + 1);
  assert.strictEqual(confirmCalls, beforeMixedConfirm + 1);
  confirmLocal = true;
  await mixedReviewConfirm.listeners.click();
  await settle();
  assert.strictEqual(batchOperations.length, 1);
  batchStates[BATCH_OP_A] = 'succeeded';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert.strictEqual(batchOperations.length, 2,
    'after independent VDR readback the next compatible film proceeds' +
    '; status=' + findClass(mount, 'status').textContent +
    '; operation=' + findClass(mount, 'media-import-operation').textContent +
    '; source=' + findClass(mount, 'media-import-source').value +
    '; batchReview=' + findClass(mount, 'media-import-batch-summary').textContent +
    '; lastPosts=' + JSON.stringify(requests.filter(function (r) {
      return r.path.endsWith('/media-imports') && r.options.method === 'POST';
    }).slice(-3).map(function (r) { return JSON.parse(r.options.body); })));
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
  autoDuplicateReadback = false;
  markedSources = [];
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
    assert(!choiceSelect.children.some(function (option) { return option.value === 'skip'; }),
      'encoding choice has only VDR and PC, not Skip');
    const markerAction = choice === 'skip'
      ? findClass(mount, 'media-import-skip')
      : findClass(mount, 'media-import-start');
    if (choice === 'desktop') {
      choiceSelect.value = choice;
      choiceSelect.listeners.change();
      assert(findClass(mount, 'media-import-preview').textContent.includes('Kein Import'),
        'PC delegation never describes a VDR import');
    } else {
      assert(!markerAction.hidden && !markerAction.disabled,
        'Skip remains its own explicit Rectools action');
    }
    assert(toggle.disabled && !toggle.checked);
    assert(findClass(mount, 'media-import-batch').disabled);
    const before = submits, priorConfirms = confirmCalls;
    confirmLocal = false;
    await markerAction.listeners.click();
    await settle();
    assert.strictEqual(submits, before);
    assert.strictEqual(confirmCalls, priorConfirms + 1);
    confirmLocal = true;
    await markerAction.listeners.click();
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

  
  // Rectools PC handoff: a separately returned same-name video in the same
  // import folder appears as a normal candidate; the parked .pc_encode
  // original is not released, deleted or silently imported.
  context.resetMediaImportVisibility(true);
  sourceDisplayOverride = 'Film';
  sourceFolders[SOURCE] = 'Serien/Test';
  sourceFolders[MARK_PC] = 'Serien/Test';
  batchSources = [SOURCE];
  markedSources = [MARK_PC];
  submitMode = 'accepted';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const parkedRows = findClass(mount, 'media-import-selection').children;
  const parkedRow = parkedRows.find(function (row) {
    return (row.className || '').includes('media-import-marked-item');
  });
  assert(parkedRow && parkedRow.children[1].textContent.includes('PC-Version gefunden'),
    'returning same-name file is shown beside parked Rectools PC marker');
  const localVdrRelease = parkedRow.children.find(function (child) {
    return child.className === 'media-import-pc-local-release';
  });
  assert(localVdrRelease && localVdrRelease.textContent.includes('lokale VDR-Umkodierung'),
    'parked .pc_encode has a visible local-conversion release action');
  assert.strictEqual(localVdrRelease.disabled, false,
    'release action uses existing explicit Core operation, not a disabled placeholder');
  const postsBeforeReturn = submits;
  await parkedRow.children[0].listeners.click();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'separater Importkandidat'), 'same name is only a hint, not proof of identity');
  assert.strictEqual(submits, postsBeforeReturn, 'marker inspection never writes');
  const recoveredDetail = findClass(mount, 'media-import-detail');
  const recoveredTechnical = findClass(mount, 'media-import-technical-details');
  const recoverButton = findClass(mount, 'media-import-release');
  assert(!recoveredDetail.children.includes(recoverButton) &&
    recoveredTechnical.children.includes(recoverButton),
    'release is not a normal media-import action');
  const returnedSelect = findClass(mount, 'media-import-source');
  returnedSelect.value = SOURCE;
  returnedSelect.listeners.change();
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-start').textContent,
    'Diesen Film importieren',
    're-encoded return goes through normal codec preview and explicit import');
  assert.strictEqual(submits, postsBeforeReturn, 'scanning never auto-imports');
  assert(findClass(mount, 'media-import-pc-return-hint').textContent.includes(
    'Codec und Importfähigkeit'), 'new PC source gets its own backend codec preview');
  assert(findClass(mount, 'media-import-selection').children[0].children[2].textContent.includes(
    'Möglicher PC-Encode-Rücklauf'), 'returned candidate is identified in the normal list');
  const pcOpen = findClass(mount, 'media-import-pc-return-open');
  assert(pcOpen, 'parked original offers a read-only shortcut to inspect PC output');
  const beforeShortcut = submits;
  await pcOpen.listeners.click();
  await settle();
  assert.strictEqual(submits, beforeShortcut,
    'opening the returned PC file must never auto-import or delete it');

  // Only a verified Core operation can announce the new VDR recording; a
  // handler exit or an unverified state is not success and never cleanup.
  state = 'executed_unverified';
  const beforePcImport = submits;
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, beforePcImport + 1,
    'returned file imports only after an explicit user action');
  assert(!findClass(mount, 'status').textContent.includes('VDR-Readback bestätigt'));
  state = 'succeeded';
  cleanupState = 'removed';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes(
    'VDR-Readback bestätigt den Import'));
  assert(findClass(mount, 'status').textContent.includes(
    'nicht ihre Medienidentität'), 'title-only PC association is not verified media identity');
  assert(findClass(mount, 'status').textContent.includes(
    'Importierte Quelldatei erfolgreich entfernt'),
    'UI displays only a journal-backed cleanup result');
  cleanupState = 'failed';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes(
    'Quellenbereinigung fehlgeschlagen'),
    'cleanup failure must not invalidate the succeeded VDR recording');
  cleanupState = 'outcome_unknown';
  await findClass(mount, 'media-import-refresh').listeners.click();
  await settle();
  assert(findClass(mount, 'status').textContent.includes(
    'Agent-Unterbrechung unbestätigt'),
    'recovered cleanup interruption must never remain pending forever');
  cleanupState = '';
  // Readback evidence remains associated with the immutable source observation
  // when the next ordinary scan still lists the unmodified source. An agent
  // 'executed_unverified' result must never create this display status.
  await findClass(mount, 'media-import-new').listeners.click();
  await settle();
  assert(findClass(mount, 'media-import-selection').children.some(function (row) {
    return row.dataset.sourceId === SOURCE &&
      row.children[2].textContent.includes('Frühere VDR-Operation bestätigt') &&
      row.children[2].textContent.includes('aktueller Quellinhalt ungeprüft');
  }), 'rescan retains historical readback without certifying current file bytes');
  const pcPostsAfterReadback = submits;
  const sourceAfterReadback = findClass(mount, 'media-import-source');
  sourceAfterReadback.value = SOURCE;
  sourceAfterReadback.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-confirmed-readback').textContent.includes(
    'KEIN Nachweis für den aktuell'),
    'past success must not certify the current file with a reusable source ID');
  assert.strictEqual(submits, pcPostsAfterReadback,
    'rescan and source reinspection never perform a second import');

  // Root-configured (non-scan) inventory entries can retain the SAME source
  // ID after someone replaces the media bytes. A changed label is observable
  // here, but a replacement with unchanged label is equally possible.
  sourceDisplayOverride = 'Ausgetauschter Film';
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  const sameIdReplacement = findClass(mount, 'media-import-selection').children
    .find(function (row) { return row.dataset.sourceId === SOURCE; });
  assert(sameIdReplacement && sameIdReplacement.children[1].textContent.includes(
    'Ausgetauschter Film'), 'new inventory observation is rendered');
  assert(sameIdReplacement.children[2].textContent.includes(
    'aktueller Quellinhalt ungeprüft'),
    'old operation success is never presented as current file verification');
  assert.strictEqual(submits, pcPostsAfterReadback,
    'same source ID with replaced media cannot authorize cleanup or another import');
  sourceDisplayOverride = 'Film';

  // Same folder and label but a different Agent observation/source ID:
  // neither a possible filename match nor a VDR result for the old source
  // may transfer the confirmed status to a newly placed/modified video.
  batchSources = [SOURCE_B];
  sourceDisplayLabels[SOURCE_B] = 'Film';
  sourceFolders[SOURCE_B] = 'Serien/Test';
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  assert(findClass(mount, 'media-import-selection').children.some(function (row) {
    return row.dataset.sourceId === SOURCE_B &&
      row.children[2].textContent.includes('Möglicher PC-Encode-Rücklauf') &&
      !row.children[2].textContent.includes('Frühere VDR-Operation bestätigt');
  }), 'a changed source observation never inherits the former recording result');
  assert.strictEqual(submits, pcPostsAfterReadback,
    'rescan with changed source ID never starts an import');
  batchSources = [SOURCE];
  delete sourceDisplayLabels[SOURCE_B];
  delete sourceFolders[SOURCE_B];

  // A new account/session cannot inherit the former browser-only evidence.
  context.resetMediaImportVisibility(true);
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  assert(!findClass(mount, 'media-import-selection').children.some(function (row) {
    return row.dataset.sourceId === SOURCE &&
      row.children[2].textContent.includes('Frühere VDR-Operation bestätigt');
  }), 'sign-out/reset discards ephemeral display evidence');

  // Two extensions with the same stem may be different films. Neither can
  // be paired arbitrarily to the single parked original.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE, SOURCE_B];
  sourceDisplayLabels[SOURCE_B] = 'Film';
  sourceFolders[SOURCE_B] = 'Serien/Test';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-pc-return-open'), null);
  assert(findClass(mount, 'media-import-selection').children.some(function (row) {
    return row.children.some(function (child) {
      return child.textContent.includes('PC-Rücklauf mehrdeutig');
    });
  }), 'ambiguous same-name return remains visible without first-match assignment');

  // Another folder or a different episode is never an inferred PC return.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE_B];
  sourceFolders[SOURCE_B] = 'Serien/Andere Folge';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-pc-return-open'), null);
  assert(findClass(mount, 'media-import-selection').children.some(function (row) {
    return row.children.some(function (child) {
      return child.textContent.includes('Wartet auf PC-Bearbeitung');
    });
  }), 'same title in a different folder is not related');

  // Multiple parked originals with the same label are also ambiguous.
  const MARK_PC2 = 'source_marked_pc_987654';
  sourceDisplayLabels[MARK_PC2] = 'Film.pc_encode';
  sourceFolders[MARK_PC2] = 'Serien/Test';
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE];
  markedSources = [MARK_PC, MARK_PC2];
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-pc-return-open'), null,
    'two parked original extensions must never bind by array order');
  assert.strictEqual(submits, beforePcImport + 1,
    'all ambiguity/rescan checks remain read-only');
  delete sourceDisplayLabels[SOURCE_B];
  delete sourceDisplayLabels[MARK_PC2];
  delete sourceFolders[SOURCE_B];
  delete sourceFolders[MARK_PC2];
  sourceDisplayOverride = null;
  delete sourceFolders[SOURCE];
  delete sourceFolders[MARK_PC];

  // The existing recovery operation stays available explicitly in technical
  // details for a wrongly marked source, never required for normal PC return.
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
  assert(findClass(mount, 'media-import-visible-count').textContent.includes('1 unmarkierte'));
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
    '70 von 70 noch nicht importierten Filmen für den Stapel gewählt.');
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
    return row.children[1].textContent;
  }).join('|'), orderedIds.map(function (id) {
    return sourceDisplayLabels[id];
  }).join('|'), 'episode buttons use the same natural order as batch checkboxes');
  const beforeEpisodeClick = submits;
  sortedCheckboxes.children[0].children[1].listeners.click();
  await settle();
  assert.strictEqual(sortedSelect.value, orderedIds[0],
    'clicking an episode row selects the real source ID for preview');
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    sourceDisplayLabels[orderedIds[0]]) ||
    findClass(mount, 'media-import-preview').textContent.includes('Backend geprüfter Film'),
    'episode row click triggers one-source technical preview');
  assert.strictEqual(submits, beforeEpisodeClick,
    'episode row navigation alone never starts an import');
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
  assert(findClass(mount, 'media-import-visible-count').textContent.includes('2 unmarkierte'));
  assert.strictEqual(folderList.children.length, 2);
  const pendingRescan = findClass(mount, 'media-import-reload').listeners.click();
  assert(findClass(mount, 'media-import-scan-notice').textContent.includes(
    'Quellen werden aktualisiert'));
  assert(findClass(mount, 'media-import-visible-count').textContent.includes(
    '2 unmarkierte'));
  assert(!folderSelect.disabled && folderSelect.children.length === 3,
    'a pending read-only rescan must preserve the already loaded library');
  await pendingRescan;
  await settle();
  assert.strictEqual(folderSelect.children.length, 3);
  folderList.children.find(function (child) {
    return child.textContent.includes('Serien');
  }).listeners.click(); // Serien
  assert.strictEqual(folderSelect.children.length, 2,
    'selecting the series parent must show episodes below it');
  assert.strictEqual(folderSelect.children[1].value, SOURCE_B);
  assert(findClass(mount, 'media-import-visible-count').textContent.includes('1 unmarkierte'));
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
    nestedPreview.includes('Frei:') &&
    nestedPreview.includes('Geschätzter Zwischenbedarf (nur Info):'),
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
    'Quellenaktualisierung fehlgeschlagen'));
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
  await findClass(mount, 'media-import-preview-retry').listeners.click();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'Film wird geprüft'));
  const retryPreview = timerCallbacks[timerCallbacks.length - 1];
  retryPreview();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'VDR-Zielordner: Serien/Andor/Weiterer Film'));

  // A batched source can return pending on its first Agent observation.
  // The batch must poll the existing preview endpoint, not misclassify it.
  previewMode = 'pending-first';
  const pendingBatchPosts = submits;
  const pendingBatch = findClass(mount, 'media-import-batch').listeners.click();
  await settle();
  assert(findClass(mount, 'media-import-batch-summary').textContent.includes(
    '1 von 2'), 'first episode inspected before second pending result');
  const batchRetry = timerCallbacks[timerCallbacks.length - 1];
  batchRetry();
  await pendingBatch;
  await settle();
  assert(findClass(mount, 'media-import-batch-summary').textContent.includes(
    '2 direkt importierbar'), 'second preview finishes after polling');
  assert.strictEqual(submits, pendingBatchPosts,
    'batch preview re-poll is strictly read-only');

  // A space shortage and an unsupported codec allow only the desktop path.
  previewInspectionStatus = 'insufficient';
  raceSelect.value = SOURCE;
  raceSelect.listeners.change();
  await findClass(mount, 'media-import-preview-retry').listeners.click();
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
  await findClass(mount, 'media-import-preview-retry').listeners.click();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'konnte vom Backend nicht sicher geprüft werden'));
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'Agent konnte Quelle, Codec oder Ziel nicht vollständig prüfen'));
  assert(!findClass(mount, 'media-import-preview').textContent.includes(
    'Diagnose ausstehend'), 'known unavailable is not an unknown preview');
  assert(findClass(mount, 'media-import-start').disabled);
  const retryUnavailable = findClass(mount, 'media-import-preview-retry');
  assert(retryUnavailable && !retryUnavailable.hidden && !retryUnavailable.disabled);
  const getBeforeRetry = requests.filter(r => r.path.endsWith('/preview')).length;
  await retryUnavailable.listeners.click();
  await settle();
  assert.strictEqual(requests.filter(r => r.path.endsWith('/preview')).length,
    getBeforeRetry + 1, 'retry must inspect exactly the currently selected source');
  assert(findClass(mount, 'media-import-start').disabled,
    'unavailable remains blocked after a repeat inspection');

  // Usability First: while the read-only rescan is pending, the last
  // successful inventory and its selected episode remain usable. Only a
  // successfully validated replacement inventory resets the preview.
  const beforeRetryReset = requests.filter(r => r.path.endsWith('/preview')).length;
  const failedPreviewRescan = findClass(mount, 'media-import-reload').listeners.click();
  assert(!retryUnavailable.hidden && !retryUnavailable.disabled,
    'pending rescan preserves the old preview retry and episode selection');
  assert.strictEqual(raceSelect.value, SOURCE,
    'pending rescan retains the currently selected episode');
  await failedPreviewRescan;
  await settle();
  assert.strictEqual(raceSelect.value, '',
    'rescan has cleared the previously selected episode');
  assert(retryUnavailable.hidden,
    'retry stays hidden until a new episode is selected and fails preview');
  await retryUnavailable.listeners.click();
  assert.strictEqual(requests.filter(r => r.path.endsWith('/preview')).length,
    beforeRetryReset, 'a cleared selection does not issue an orphan preview');
  raceSelect.value = SOURCE;

  previewInspectionStatus = 'unavailable';
  batchProbeOverrides[SOURCE] = {status: 'unavailable', failureCode: 'ffprobe_failed'};
  raceSelect.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-preview-card').children.some(function (child) {
    return (child.className || '').includes('media-import-preview-hint') &&
      child.textContent.includes('FFprobe');
  }), 'agent failure stage is visible for the actual selected source');
  delete batchProbeOverrides[SOURCE];
  previewInspectionStatus = 'ready';
  previewMode = 'forbidden';
  raceSelect.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'HTTP 403: Keine Berechtigung für diese Importvorschau'));
  assert(!findClass(mount, 'media-import-preview-retry').hidden,
    'a concrete HTTP error must offer a retry without switching films');
  previewMode = 'ready';
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, true,
    'authorization error must prevent import submission');
  assert.strictEqual(requests.filter(r => r.options.method === 'POST').length,
    racePosts, 'all preview transitions are read-only');

  // Temporary capability loss is advisory, not a fabricated HTTP 403.
  // An already selected and previewed source is submitted ONCE to Core, which
  // is the sole action authorizer; no second Agent capability/inventory GET.
  raceSelect.value = SOURCE;
  raceSelect.listeners.change();
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, false);
  const beforeRejectedAdmission = submits;
  const beforeCapabilityRead = requests.filter(function (r) {
    return r.path.endsWith('/capabilities/media-import');
  }).length;
  const beforeSourceRead = requests.filter(function (r) {
    return r.path.endsWith('/media-import-sources');
  }).length;
  allow = false;
  submitMode = 'denied';
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, beforeRejectedAdmission + 1,
    'a transient false capability must not block the canonical Core POST');
  assert.strictEqual(requests.filter(function (r) {
    return r.path.endsWith('/capabilities/media-import');
  }).length, beforeCapabilityRead, 'click must not recheck Agent capability');
  assert.strictEqual(requests.filter(function (r) {
    return r.path.endsWith('/media-import-sources');
  }).length, beforeSourceRead, 'click must not rescan sources in the browser');
  assert(findClass(mount, 'status').textContent.includes(
    'Media-Import-Berechtigung wurde verweigert'),
    'only Core HTTP 403 may report denial, never local capability inference');
  allow = true;
  submitMode = 'accepted';
  await findClass(mount, 'media-import-reload').listeners.click();
  await settle();
  raceSelect.value = SOURCE;
  raceSelect.listeners.change();
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, false,
    'a definite Core rejection recovers through the existing read-only refresh');
  assert.strictEqual(submits, beforeRejectedAdmission + 1,
    'read-only recovery must not submit another import');

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

  // Two confirmed duplicates in one batch are marked serially through the
  // existing Core operation; the ready film stays available for import.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE, SOURCE_B, SOURCE_C];
  markedSources = [];
  batchProbeOverrides[SOURCE_B] = {status: 'duplicate'};
  batchProbeOverrides[SOURCE_C] = {status: 'duplicate'};
  previewInspectionStatus = 'ready';
  state = 'succeeded';
  submitMode = 'accepted';
  autoDuplicateReadback = true;
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const beforeTwoDuplicates = submits;
  await findClass(mount, 'media-import-batch').listeners.click();
  await settle();
  assert.strictEqual(submits, beforeTwoDuplicates + 2,
    'two duplicates require exactly two serial Core marker operations');
  assert(requests.filter(x => x.options.method === 'POST').slice(-2).every(function (x) {
    return JSON.parse(x.options.body).videoDecision === 'duplicate';
  }), 'batch duplicate steps do not dispatch VDR imports');
  assert(findClass(mount, 'media-import-batch-review').children.filter(function (row) {
    return row.textContent.includes('Als .duplicate markiert');
  }).length === 2, 'each duplicate requires independently confirmed readback');
  assert(!findClass(mount, 'media-import-batch-confirm').disabled,
    'remaining compatible film is available after duplicate marking');
  delete batchProbeOverrides[SOURCE_B];
  delete batchProbeOverrides[SOURCE_C];
  autoDuplicateReadback = false;
  markedSources = [];

  // A confirmed duplicate is automatically sent through the existing
  // Core/Agent marker operation. It never displays a manual duplicate button.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE];
  markedSources = [];
  previewInspectionStatus = 'duplicate';
  autoDuplicateReadback = true;
  state = 'succeeded';
  submitMode = 'accepted';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const dupSelect = findClass(mount, 'media-import-source');
  const preDuplicatePosts = submits;
  const preDuplicateConfirmations = confirmCalls;
  dupSelect.value = SOURCE;
  dupSelect.listeners.change();
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-duplicate'), null,
    'no manual duplicate action is mounted');
  assert.strictEqual(confirmCalls, preDuplicateConfirmations,
    'a proven duplicate is marked without an additional dialog');
  assert.strictEqual(submits, preDuplicatePosts + 1,
    'confirmed preview creates exactly one Core marker operation');
  assert.strictEqual(requests.filter(x => x.options.method === 'POST').slice(-1)[0].options.body,
    JSON.stringify({sourceId: SOURCE, videoDecision: 'duplicate'}));
  assert(findClass(mount, 'media-import-marked-source').children.some(function (option) {
    return option.value === MARK_DUP;
  }), 'marker success rereads the real .duplicate source from the Agent');
  assert.strictEqual(findClass(mount, 'media-import-source').disabled, true,
    'already marked source must not remain importable');
  assert.strictEqual(submits, preDuplicatePosts + 1);
  autoDuplicateReadback = false;
  previewInspectionStatus = 'ready';

  // When automatic .duplicate marking fails for an already imported film,
  // the failed file action must unlock navigation; ordinary duplicate cleanup
  // must still be offered without renaming the original first.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE];
  markedSources = [];
  previewInspectionStatus = 'duplicate';
  autoDuplicateReadback = false;
  state = 'failed_verified';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const failedDuplicateSelect = findClass(mount, 'media-import-source');
  const failedDuplicatePosts = submits;
  failedDuplicateSelect.value = SOURCE;
  failedDuplicateSelect.listeners.change();
  await settle();
  assert.strictEqual(submits, failedDuplicatePosts + 1,
    'automatic marker was attempted exactly once');
  assert.strictEqual(findClass(mount, 'media-import-source').disabled, false,
    'terminal marker failure must unlock film selection');
  assert.strictEqual(findClass(mount, 'media-import-reload').disabled, false,
    'terminal marker failure must unlock scanning');
  assert.strictEqual(findClass(mount, 'media-import-search').disabled, false,
    'terminal marker failure must unlock searching');
  assert.strictEqual(findClass(mount, 'media-import-filter').disabled, false,
    'terminal marker failure must unlock filtering');
  assert(findClass(mount, 'media-import-completed-notice').textContent.includes(
    'Dubletten-Markierung fehlgeschlagen'));
  const afterFailedMarker = findClass(mount, 'media-import-source');
  afterFailedMarker.value = SOURCE;
  afterFailedMarker.listeners.change();
  await settle();
  assert.strictEqual(submits, failedDuplicatePosts + 1,
    'same source is not automatically re-marked after terminal failure');
  const deleteAlreadyImported = findClass(mount, 'media-import-delete-duplicate');
  assert(deleteAlreadyImported && !deleteAlreadyImported.hidden &&
    !deleteAlreadyImported.disabled,
    'unmarked source with duplicate preview offers explicit cleanup');
  confirmLocal = false;
  await deleteAlreadyImported.listeners.click();
  assert.strictEqual(submits, failedDuplicatePosts + 1,
    'cancelled source deletion never submits');
  confirmLocal = true;
  state = 'succeeded';
  await deleteAlreadyImported.listeners.click();
  await settle();
  assert.strictEqual(submits, failedDuplicatePosts + 2,
    'one explicit cleanup operation after duplicate marker failure');
  assert.strictEqual(JSON.parse(requests.filter(x =>
    x.options.method === 'POST').slice(-1)[0].options.body).videoDecision,
    'delete_duplicate');
  assert(!batchSources.includes(SOURCE),
    'canonical cleanup removes unmarked import source, not the VDR recording');
  previewInspectionStatus = 'ready';

  // MT.META4: only verified paths and explicit SxxEyy / Staffel patterns
  // establish presentation groups. Selection still posts opaque IDs only.
  context.resetMediaImportVisibility(true);
  previewMode = 'ready';
  previewInspectionStatus = 'ready';
  submitMode = 'accepted';
  batchSources = [SOURCE, SOURCE_B, SOURCE_C, SOURCE_D];
  markedSources = [MARK_PC];
  sourceFolders[SOURCE] = 'Serien/Andor/Staffel 1';
  sourceFolders[SOURCE_B] = 'Serien/Andor/Staffel 1';
  sourceFolders[SOURCE_C] = 'Serien/Andor/Staffel 2';
  sourceFolders[SOURCE_D] = 'Serien/Zweite Serie/Staffel 1';
  sourceFolders[MARK_PC] = 'Serien/Andor/Staffel 1';
  sourceDisplayOverride = 'S01E01 Pilot';
  sourceDisplayLabels[SOURCE_B] = 'Staffel 1 Folge 2';
  sourceDisplayLabels[SOURCE_C] = 'S02E01 Rückkehr';
  sourceDisplayLabels[SOURCE_D] = 'S01E01 Pilot';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const seriesGroups = findClass(mount, 'media-import-series-groups');
  const seasonPicker = findClass(mount, 'media-import-season-filter');
  const mobileGroups = findClass(mount, 'media-import-mobile-nav');
  const seriesPosts = submits;
  const andor = seriesGroups.children.find(function (button) {
    return button.dataset.folder === 'Serien/Andor';
  });
  const secondSeries = seriesGroups.children.find(function (button) {
    return button.dataset.folder === 'Serien/Zweite Serie';
  });
  assert(andor && secondSeries, 'same episode name in other source folder is independent');
  andor.listeners.click();
  assert.strictEqual(findClass(mount, 'media-import-panel').dataset.mobilePane, 'media');
  assert(!findClass(mount, 'media-import-season-label').hidden);
  assert(seasonPicker.children.some(function (option) {
    return option.value === 's:1' && option.textContent === 'Staffel 1';
  }));
  assert(seasonPicker.children.some(function (option) { return option.value === 's:2'; }));
  assert(findClass(mount, 'media-import-visible-count').textContent.includes('3 unmarkierte'));
  seasonPicker.value = 's:2';
  seasonPicker.listeners.change();
  let seasonIds = findClass(mount, 'media-import-selection').children;
  assert(seasonIds.some(function (row) { return row.dataset.sourceId === SOURCE_C; }));
  assert(!seasonIds.some(function (row) { return row.dataset.sourceId === SOURCE; }));
  assert(!seasonIds.some(function (row) { return row.dataset.sourceId === MARK_PC; }),
    'parked PC source is never an ordinary candidate');
  seasonPicker.value = 's:1';
  seasonPicker.listeners.change();
  seasonIds = findClass(mount, 'media-import-selection').children;
  assert(seasonIds.some(function (row) { return row.dataset.sourceId === SOURCE; }));
  assert(seasonIds.some(function (row) { return row.dataset.sourceId === SOURCE_B; }));
  assert(seasonIds.some(function (row) {
    return row.dataset.sourceId === MARK_PC &&
      row.className.includes('media-import-marked-item');
  }), '.pc_encode remains marked and visible in its actual season');
  const episodeRow = seasonIds.find(function (row) {
    return row.dataset.sourceId === SOURCE_B;
  });
  assert(findClass(episodeRow, 'media-import-episode-meta').textContent.includes('S01E02'),
    'explicit episode number appears beside original label');
  findClass(episodeRow, 'media-import-episode-open').listeners.click();
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-source').value, SOURCE_B);
  assert.strictEqual(findClass(mount, 'media-import-panel').dataset.mobilePane, 'details');
  assert.strictEqual(findClass(mount, 'media-import-online-kind').value, 'series',
    'long-form Staffel/Folge coordinates must select series metadata search');
  assert.strictEqual(findClass(mount, 'media-import-online-query').value, 'Andor',
    'series metadata query must use the actual series folder, not episode filename');
  assert(findClass(mount, 'media-import-online').children.some(function (child) {
    return (child.className || '').includes('media-import-online-context') &&
      child.textContent.includes('Staffel 1 · Folge 2');
  }), 'recognized episode coordinates stay visible during automatic TMDb lookup');
  await findClass(mount, 'media-import-mobile-back').listeners.click();
  assert.strictEqual(findClass(mount, 'media-import-panel').dataset.mobilePane, 'media');
  secondSeries.listeners.click();
  seasonIds = findClass(mount, 'media-import-selection').children;
  assert.strictEqual(seasonIds.length, 1);
  assert.strictEqual(seasonIds[0].dataset.sourceId, SOURCE_D);
  assert.strictEqual(submits, seriesPosts, 'group navigation does not create a Core operation');

  // MT.META5: selecting a recognized series episode automatically follows
  // the existing authenticated TMDb search -> season -> episode lookup.
  // Scanning alone still never starts an online lookup or import.
  andor.listeners.click();
  const onlineSelect = findClass(mount, 'media-import-source');
  const beforeAutoRequests = onlineRequests.length;
  const beforeAutoImports = submits;
  onlineSelect.value = SOURCE;
  onlineSelect.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-local-metadata'),
    'local NFO is preserved during automatic TMDb lookup');
  const autoRequests = onlineRequests.slice(beforeAutoRequests);
  assert.strictEqual(autoRequests.length, 3,
    'one selected series episode performs search, season and episode lookup');
  assert(autoRequests[0].route.endsWith('/search') &&
    autoRequests[1].route.endsWith('/seasons') &&
    autoRequests[2].route.endsWith('/episodes'));
  assert.strictEqual(JSON.parse(autoRequests[2].options.body).seasonNumber, 1);
  const automaticOnline = findClass(mount, 'media-import-online-confirmed');
  assert(automaticOnline && automaticOnline.children[0].textContent.includes('Pilot'),
    'exact S01E01 is automatically identified from provider results');
  assert(automaticOnline.children[0].textContent.includes('Automatisch erkannte'),
    'automatic attribution is labelled separately from manual selection');
  // Previously completed technical checks are visible across episode changes
  // and are not repeated within this source scan.
  const checksFor = function (sourceId) {
    return requests.filter(function (r) {
      return r.path.endsWith('/media-import-sources/' + sourceId + '/preview');
    }).length;
  };
  const checkedA = checksFor(SOURCE);
  onlineSelect.value = SOURCE_B;
  onlineSelect.listeners.change();
  await settle();
  assert(findClass(mount, 'media-import-start').disabled === false);
  const beforeReturning = checksFor(SOURCE);
  onlineSelect.value = SOURCE;
  onlineSelect.listeners.change();
  await settle();
  assert.strictEqual(beforeReturning, checkedA);
  assert.strictEqual(checksFor(SOURCE), checkedA,
    'reselecting a checked episode never repeats its technical Agent probe');
  assert(findClass(mount, 'media-import-preview').textContent.includes(
    'VDR-Zielordner:'), 'reused technical details appear immediately');
  assert(findClass(mount, 'media-import-selection').children.some(function (row) {
    return row.dataset.sourceId === SOURCE &&
      findClass(row, 'media-import-episode-meta').textContent.includes('Technisch geprüft');
  }), 'episode row stays visibly marked as technically checked');
  assert(!findClass(mount, 'media-import-preview-retry').hidden,
    'the user can explicitly request a fresh technical check');
  const beforeManualRecheck = checksFor(SOURCE);
  await findClass(mount, 'media-import-preview-retry').listeners.click();
  await settle();
  assert.strictEqual(checksFor(SOURCE), beforeManualRecheck + 1,
    'the explicit recheck does exactly one fresh technical request');
  // Local source artwork stays authoritative, while the episode still has
  // its own secondary slot and cannot replace the series cover.
  const localAutoPoster = findClass(findClass(mount, 'media-import-artwork-poster'),
    'media-import-artwork-image');
  assert(localAutoPoster && localAutoPoster.src === 'data:image/jpeg;base64,/9j/2Q==',
    'existing local poster stays visible after automatic TMDb matching');
  const localEpisodeStill = findClass(findClass(mount, 'media-import-artwork-episode'),
    'media-import-artwork-image');
  assert(localEpisodeStill && localEpisodeStill.src.includes('externalNamespace=tv-episode'),
    'identified episode still is separate from local poster');
  assert(findClass(mount, 'media-import-artwork').children.length === 3,
    'poster, episode still and fanart have independent slots');
  // When no local poster exists, use the actual series cover as the main
  // image; the matching S01E01 photo is separate, with fanart unaffected.
  previewArtworkMode = 'none';
  await findClass(mount, 'media-import-preview-retry').listeners.click();
  await settle();
  const autoPoster = findClass(findClass(mount, 'media-import-artwork-poster'),
    'media-import-artwork-image');
  const autoStill = findClass(findClass(mount, 'media-import-artwork-episode'),
    'media-import-artwork-image');
  assert(autoPoster && autoPoster.src.includes('externalNamespace=tv') &&
    !autoPoster.src.includes('externalNamespace=tv-episode'),
    'identified Andor series supplies the main cover');
  assert(autoStill && autoStill.src.includes('externalNamespace=tv-episode'),
    'episode photo appears only in a separate slot');
  assert(findClass(findClass(mount, 'media-import-artwork-fanart'),
    'media-import-artwork-empty').textContent === 'Keine lokale Fanart',
    'missing fanart stays independent of the episode image');
  previewArtworkMode = 'poster';
  await findClass(mount, 'media-import-preview-retry').listeners.click();
  await settle();
  assert.strictEqual(submits, beforeAutoImports,
    'automatic metadata lookup never imports, marks or changes VDR media');
  // Multiple equally plausible provider IDs must not be silently chosen.
  onlineMode = 'ambiguous';
  const beforeAmbiguousRequests = onlineRequests.length;
  onlineSelect.listeners.change();
  await settle();
  assert.strictEqual(onlineRequests.length - beforeAmbiguousRequests, 1,
    'ambiguous title never triggers season or episode lookup');
  assert.strictEqual(findClass(mount, 'media-import-online-confirmed'), null,
    'ambiguous series identity is never accepted automatically');
  assert(findClass(mount, 'media-import-online-status').textContent.includes('Mehrdeutige Serie'),
    'ambiguous provider identity is clearly explained to the user');
  // An automatic lookup still in flight belongs only to the source that
  // initiated it. Switching to another episode discards the late result.
  onlineMode = 'defer';
  onlineSelect.listeners.change();
  await settle();
  const resolveOldAuto = resolveOnlineReply;
  assert.strictEqual(typeof resolveOldAuto, 'function');
  onlineMode = 'offline';
  onlineSelect.value = SOURCE_B;
  onlineSelect.listeners.change();
  await settle();
  resolveOldAuto(onlinePage([onlineCandidate('series', '1399', 'Andor')]));
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-online-confirmed'), null,
    'late automatic provider reply never identifies a different selected episode');
  onlineMode = 'ready';
  onlineSelect.value = SOURCE;
  onlineSelect.listeners.change();
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-online-kind').value, 'series',
    'S01E01 is a series episode, not a movie');
  assert.strictEqual(findClass(mount, 'media-import-online-query').value, 'Andor',
    'S01E01 search starts with the series name, not the raw filename');
  assert(findClass(mount, 'media-import-online').children.some(function (child) {
    return (child.className || '').includes('media-import-online-context') &&
      child.textContent.includes('Staffel 1 · Folge 1');
  }), 'the selected episode coordinates remain visible alongside provider context');
  const importedBeforeOnline = submits;
  const movieChoice = findClass(mount, 'media-import-online-kind');
  movieChoice.value = 'movie';
  movieChoice.listeners.change();
  let onlineQuery = findClass(mount, 'media-import-online-query');
  onlineQuery.value = 'Film mit belegter TMDb-ID';
  onlineQuery.listeners.input();
  await findClass(mount, 'media-import-online-search').listeners.click();
  await settle();
  let onlineResults = findClass(mount, 'media-import-online-results');
  assert.strictEqual(onlineResults.children.length, 2,
    'wrong-provider candidates are not presented as confirmed TMDb results');
  assert.strictEqual(findClass(mount, 'media-import-online-confirmed'), null,
    'multiple candidates never select themselves, even if titles match');
  await findClass(onlineResults.children[0], 'media-import-online-choose').listeners.click();
  let confirmedOnline = findClass(mount, 'media-import-online-confirmed');
  assert(confirmedOnline && confirmedOnline.children[0].textContent.includes('Film mit belegter TMDb-ID'));
  const onlinePoster = findClass(confirmedOnline, 'media-import-online-poster');
  assert(onlinePoster && onlinePoster.src.includes('/settings/series-artwork/candidate-image?') &&
    onlinePoster.src.includes('externalNamespace=movie'),
    'poster URL is served by the existing authenticated Suite image route');
  onlinePoster.listeners.error();
  assert(confirmedOnline.children[0].textContent.includes('Film mit belegter TMDb-ID') &&
    confirmedOnline.children.some(function (child) {
      return child.textContent.includes('Online-Poster nicht verfügbar');
    }), 'missing TMDb artwork leaves the selected title and plot intact');
  assert(findClass(mount, 'media-import-local-metadata') &&
    findClass(mount, 'media-import-nfo-plot'),
    'confirmed online metadata must not replace or suppress NFO text');
  assert.strictEqual(submits, importedBeforeOnline,
    'online selection must never submit a media import or mutation');

  // Real series -> season -> episode search, with local S01E01 conflict check.
  let onlineKind = findClass(mount, 'media-import-online-kind');
  onlineKind.value = 'series';
  onlineKind.listeners.change();
  onlineQuery = findClass(mount, 'media-import-online-query');
  onlineQuery.value = 'Andor';
  onlineQuery.listeners.input();
  await findClass(mount, 'media-import-online-search').listeners.click();
  await settle();
  onlineResults = findClass(mount, 'media-import-online-results');
  assert.strictEqual(onlineResults.children.length, 1);
  findClass(onlineResults.children[0], 'media-import-online-choose').listeners.click();
  assert.strictEqual(findClass(mount, 'media-import-online-confirmed'), null,
    'an episode must not silently accept a series-level identity');
  await findClass(findClass(mount, 'media-import-online-results').children[0],
    'media-import-online-next').listeners.click();
  await settle();
  assert(findClass(mount, 'media-import-online-context').textContent.includes('Andor'));
  await findClass(findClass(mount, 'media-import-online-results').children[0],
    'media-import-online-next').listeners.click();
  await settle();
  onlineResults = findClass(mount, 'media-import-online-results');
  assert.strictEqual(onlineResults.children.length, 2);
  findClass(onlineResults.children[0], 'media-import-online-choose').listeners.click();
  assert.strictEqual(findClass(mount, 'media-import-online-confirmed'), null,
    'S01E02 cannot become S01E01 by a fuzzy title match');
  onlineResults = findClass(mount, 'media-import-online-results');
  findClass(onlineResults.children[1], 'media-import-online-choose').listeners.click();
  confirmedOnline = findClass(mount, 'media-import-online-confirmed');
  assert(confirmedOnline && confirmedOnline.children[0].textContent.includes('Pilot'));
  assert(findClass(confirmedOnline, 'media-import-online-poster').src.includes('externalNamespace=tv-episode'));
  const manualCover = findClass(findClass(mount, 'media-import-artwork-poster'),
    'media-import-artwork-image');
  const manualStill = findClass(findClass(mount, 'media-import-artwork-episode'),
    'media-import-artwork-image');
  assert(manualCover && manualCover.src === 'data:image/jpeg;base64,/9j/2Q==' &&
    manualStill && manualStill.src.includes('externalNamespace=tv-episode'),
    'manual episode selection preserves local poster and separates episode image');
  assert(onlineRequests.some(x => x.route.endsWith('/seasons')) &&
    onlineRequests.some(x => x.route.endsWith('/episodes')));

  // Online outage preserves the original NFO and local image without a
  // misleading selected identity, and does not block import preview.
  onlineMode = 'offline';
  onlineKind = findClass(mount, 'media-import-online-kind');
  onlineKind.value = 'movie';
  onlineKind.listeners.change();
  onlineQuery = findClass(mount, 'media-import-online-query');
  onlineQuery.value = 'No network';
  onlineQuery.listeners.input();
  await findClass(mount, 'media-import-online-search').listeners.click();
  await settle();
  assert(findClass(mount, 'media-import-online-status').textContent.includes('lokale Daten'));
  assert.strictEqual(findClass(mount, 'media-import-online-confirmed'), null);
  assert(findClass(mount, 'media-import-local-metadata') &&
    findClass(mount, 'media-import-artwork'),
    'offline fallback retains local information and images');
  assert.strictEqual(submits, importedBeforeOnline);

  // Late metadata replies belong to the exact source, owner and backend.
  onlineMode = 'defer';
  onlineQuery = findClass(mount, 'media-import-online-query');
  onlineQuery.value = 'Late';
  onlineQuery.listeners.input();
  const pendingOnline = findClass(mount, 'media-import-online-search').listeners.click();
  await settle();
  assert.strictEqual(typeof resolveOnlineReply, 'function');
  // The new episode auto-search must not replace the deliberately delayed
  // manual reply used to verify stale-response isolation.
  onlineMode = 'offline';
  onlineSelect.value = SOURCE_B;
  onlineSelect.listeners.change();
  await settle();
  resolveOnlineReply(onlinePage([onlineCandidate('movie','13','STALE MOVIE')]));
  await pendingOnline;
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-online-confirmed'), null);
  assert(!findClass(mount, 'media-import-online-results').children.length,
    'switching source discards stale provider candidates');
  // Switching backend/owner while a lookup is pending also discards results.
  onlineMode = 'defer';
  const anotherQuery = findClass(mount, 'media-import-online-query');
  anotherQuery.value = 'Backend changed';
  anotherQuery.listeners.input();
  const pendingBackend = findClass(mount, 'media-import-online-search').listeners.click();
  await settle();
  const resolvePreviousBackend = resolveOnlineReply;
  context.VdrSuiteMediaImport.deactivate();
  await context.VdrSuiteMediaImport.activate('backend-b', mount);
  resolvePreviousBackend(onlinePage([onlineCandidate('movie','13','OLD BACKEND')]));
  await pendingBackend;
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-online-confirmed'), null,
    'an old backend never populates the current detail');
  assert.strictEqual(submits, importedBeforeOnline,
    'search and offline fallback have no Core import side effects');

  // Confirmed duplicate cleanup must require a deliberate click and operate
  // only on the marked import-source identity, not on any VDR recording.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE];
  markedSources = [MARK_DUP, MARK_SKIP];
  state = 'succeeded';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const markerRows = findClass(mount, 'media-import-selection').children;
  const markedDuplicateRow = markerRows.find(function (row) {
    return row.dataset.sourceId === MARK_DUP;
  });
  assert(markedDuplicateRow);
  const inlineDelete = findClass(markedDuplicateRow, 'media-import-delete-duplicate-inline');
  assert(inlineDelete && !inlineDelete.hidden && !inlineDelete.disabled,
    'confirmed duplicate has a directly reachable delete action in its list row');
  findClass(markedDuplicateRow, 'media-import-episode-open').listeners.click();
  const deleteAction = findClass(mount, 'media-import-delete-duplicate');
  assert(deleteAction && !deleteAction.hidden && !deleteAction.disabled);
  const beforeDelete = submits;
  confirmLocal = false;
  await inlineDelete.listeners.click();
  assert.strictEqual(submits, beforeDelete, 'cancelled inline delete creates no operation');
  confirmLocal = true;
  await inlineDelete.listeners.click();
  await settle();
  assert.strictEqual(submits, beforeDelete + 1,
    'explicit delete uses one existing canonical media import operation');
  assert.strictEqual(JSON.parse(requests.filter(function (r) {
    return r.options.method === 'POST';
  }).slice(-1)[0].options.body).videoDecision, 'delete_duplicate');
  assert(!markedSources.includes(MARK_DUP), 'the marked source was removed');
  assert(findClass(mount, 'status').textContent.includes('gelöscht'),
    'readback distinguishes source deletion from a new VDR recording');
  const skipRow = findClass(mount, 'media-import-selection').children.find(
    function (row) { return row.dataset.sourceId === MARK_SKIP; });
  assert(skipRow && !findClass(skipRow, 'media-import-delete-duplicate'),
    'skipped rows never receive a destructive inline action');
  findClass(skipRow, 'media-import-episode-open').listeners.click();
  const skippedDeleteAction = findClass(mount, 'media-import-delete-duplicate');
  assert(!skippedDeleteAction || skippedDeleteAction.hidden ||
    skippedDeleteAction.disabled,
    'skipped originals must never offer duplicate deletion');
  markedSources = [];

  // MT.REC6: Agent inventory status survives a browser reset; selecting the
  // imported episode must not trigger another ffprobe/preview POST or marker.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE];
  recordingStatuses[SOURCE] = 'recording_present';
  state = 'succeeded';
  submitMode = 'accepted';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const importedRow = findClass(mount, 'media-import-selection').children.find(
    function (row) { return row.dataset.sourceId === SOURCE; });
  assert(importedRow && importedRow.children[2].textContent.includes('Bereits importiert'),
    'fresh login gets recording presence from the backend, not browser memory');
  assert(findClass(importedRow, 'media-import-episode-check').disabled,
    'already imported source cannot be bulk-selected for another import');
  assert(findClass(mount, 'media-import-batch').disabled,
    'bulk import does not include previously imported recordings');
  const importedFilter = findClass(mount, 'media-import-filter');
  importedFilter.value = 'imported';
  importedFilter.listeners.change();
  const filteredImportedRow = findClass(mount, 'media-import-selection').children.find(
    function (row) { return row.dataset.sourceId === SOURCE; });
  assert(filteredImportedRow,
    'imported status filter retains confirmed recording');
  const readOnlyPreviewCount = requests.filter(function (req) {
    return req.path.includes('/preview');
  }).length;
  const importedPostCount = submits;
  findClass(filteredImportedRow, 'media-import-episode-open').listeners.click();
  await settle();
  assert.strictEqual(requests.filter(function (req) {
    return req.path.includes('/preview');
  }).length, readOnlyPreviewCount,
  'opening already imported recording must reuse inventory instead of a new preview');
  assert.strictEqual(submits, importedPostCount,
    'opening an imported episode cannot automatically mark or import');
  const importedRemove = findClass(filteredImportedRow, 'media-import-delete-duplicate-inline');
  assert(importedRemove && !importedRemove.disabled,
    'unmarked confirmed recording has a directly reachable delete action');
  confirmLocal = false;
  await importedRemove.listeners.click();
  assert.strictEqual(submits, importedPostCount,
    'cancelled unmarked duplicate cleanup must not submit');
  confirmLocal = true;
  await importedRemove.listeners.click();
  await settle();
  assert.strictEqual(submits, importedPostCount + 1,
    'explicit cleanup submits exactly one existing Core delete_duplicate operation');
  assert.strictEqual(JSON.parse(requests.filter(function (req) {
    return req.options.method === 'POST';
  }).slice(-1)[0].options.body).videoDecision, 'delete_duplicate');
  assert(!findClass(mount, 'media-import-selection').children.some(function (row) {
    return row.dataset.sourceId === SOURCE;
  }), 'confirmed cleanup refreshes inventory and removes the deleted source');
  delete recordingStatuses[SOURCE];

  // MT.REC6 regression: an uncertain S01E01 delete must NOT disable
  // S01E06's independently validated import. The old operation stays
  // source-pinned and can be queried using GET without a second delete POST.
  context.resetMediaImportVisibility(true);
  batchSources = [SOURCE, SOURCE_B, SOURCE_C];
  markedSources = [];
  recordingStatuses[SOURCE] = 'recording_present';
  recordingStatuses[SOURCE_C] = 'recording_present';
  state = 'outcome_unknown';
  submitMode = 'accepted';
  previewMode = 'ready';
  previewInspectionStatus = 'ready';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const sourceRow = findClass(mount, 'media-import-selection').children.find(
    function (row) { return row.dataset.sourceId === SOURCE; });
  const uncertainDelete = findClass(sourceRow, 'media-import-delete-duplicate-inline');
  assert(uncertainDelete && !uncertainDelete.disabled,
    'S01E01 has a confirmed VDR recording and a reachable cleanup operation');
  const beforeUncertainDelete = submits;
  await uncertainDelete.listeners.click();
  await settle();
  assert.strictEqual(submits, beforeUncertainDelete + 1,
    'exactly one confirmed S01E01 delete request is sent');
  assert(batchSources.includes(SOURCE),
    'uncertain cleanup must never be mocked as a successful source deletion');
  assert(uncertainDelete.disabled, 'S01E01 deletion must stay locked to its unknown result');
  const blockedReason = findClass(sourceRow, 'media-import-delete-blocked');
  assert(blockedReason && !blockedReason.hidden &&
    blockedReason.textContent.includes('ungeklärt'),
    'the exact blocked S01E01 row must explain why deletion is unavailable');
  const independentDuplicateRow = findClass(mount, 'media-import-selection').children.find(
    function (row) { return row.dataset.sourceId === SOURCE_C; });
  const independentDelete = findClass(independentDuplicateRow,
    'media-import-delete-duplicate-inline');
  assert(independentDelete && !independentDelete.disabled,
    'unresolved S01E01 must not disable the confirmed S01E03 delete button');
  const beforeIndependentConfirm = confirmCalls;
  confirmLocal = false;
  await independentDelete.listeners.click();
  await settle();
  assert.strictEqual(confirmCalls, beforeIndependentConfirm + 1,
    'direct click on S01E03 must reach its own ordinary confirmation dialog');
  assert.strictEqual(submits, beforeUncertainDelete + 1,
    'cancelled S01E03 cleanup must never submit or replay S01E01');
  confirmLocal = true;
  const independentSource = findClass(mount, 'media-import-source');
  independentSource.value = SOURCE_B;
  independentSource.listeners.change();
  await settle();
  assert.strictEqual(findClass(mount, 'media-import-preview-state').textContent,
    'Direktimport möglich', 'S01E06 remains independently ready');
  assert.strictEqual(findClass(mount, 'media-import-start').disabled, false,
    'an unrelated unknown deletion must not disable S01E06 import');
  const unresolved = findClass(mount, 'media-import-unresolved-operations');
  assert(unresolved && unresolved.children.length === 1,
    'old S01E01 operation remains displayed after switching episodes');
  assert(unresolved.children[0].textContent === '' ||
    findClass(unresolved, 'media-import-unresolved-label').textContent.includes(OP),
    'the old exact Core operation identity remains visible');
  const beforeUnknownCheck = submits;
  await findClass(unresolved, 'media-import-unresolved-check').listeners.click();
  await settle();
  assert.strictEqual(submits, beforeUnknownCheck,
    'checking old S01E01 via GET cannot submit another deletion');
  const pinnedDiagnostics = findClass(mount, 'media-import-operation-diagnostics');
  assert(pinnedDiagnostics.textContent.includes('Core-Status: outcome_unknown') &&
    pinnedDiagnostics.textContent.includes('Operation: ' + OP),
    'old source remains diagnosable after selecting the independent episode');
  assert(pinnedDiagnostics.textContent.includes('Letzte erfolgreiche Core-Abfrage'),
    'manual read visibly timestamps a real public Operation response');

  independentSource.value = SOURCE;
  independentSource.listeners.change();
  await settle();
  const pinnedRow = findClass(mount, 'media-import-selection').children.find(
    function (row) { return row.dataset.sourceId === SOURCE; });
  const pinnedDelete = findClass(pinnedRow, 'media-import-delete-duplicate-inline');
  assert(pinnedDelete && pinnedDelete.disabled,
    'the unresolved S01E01 source remains blocked against replay');
  independentSource.value = SOURCE_B;
  independentSource.listeners.change();
  await settle();
  const beforeIndependentImport = submits;
  assert(!findClass(mount, 'media-import-start').disabled);
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, beforeIndependentImport + 1,
    'S01E06 creates one independent new import, not a S01E01 retry');
  const independentPost = requests.filter(function (entry) {
    return entry.path.endsWith('/media-imports') && entry.options.method === 'POST';
  }).slice(-1)[0];
  assert.strictEqual(JSON.parse(independentPost.options.body).sourceId, SOURCE_B);
  assert(findClass(mount, 'media-import-unresolved-operations').children.length === 1,
    'the pinned S01E01 operation survives a separate S01E06 admission');
  delete recordingStatuses[SOURCE];
  delete recordingStatuses[SOURCE_C];

  // MT.META6: reuse the ALREADY automatically identified episode. Exactly
  // one normal import POST carries the frozen native-info metadata; neither
  // the click nor the admission path does a second TMDb search.
  // SOURCE still has the deliberately unresolved S01E01 cleanup above.
  // Test the frozen handoff on an independent, exact S01E01 source: never
  // replay the source-pinned unknown Core mutation just to satisfy a test.
  context.resetMediaImportVisibility(true);
  onlineMode = 'ready';
  sourceFolders[SOURCE_D] = 'Serien/Andor/Staffel 1';
  sourceDisplayLabels[SOURCE_D] = 'S01E01 Pilot';
  batchSources = [SOURCE_D, SOURCE_C];
  markedSources = [];
  recordingStatuses[SOURCE_C] = 'recording_present';
  state = 'succeeded';
  submitMode = 'accepted';
  previewMode = 'ready';
  previewInspectionStatus = 'ready';
  assert.strictEqual(await context.refreshMediaImportVisibility('backend-a'), true);
  context.selectModule('media-import');
  await settle();
  const frozenSource = findClass(mount, 'media-import-source');
  frozenSource.value = SOURCE_D;
  frozenSource.listeners.change();
  await settle();
  const autoConfirmation = findClass(mount, 'media-import-online-confirmed');
  assert(autoConfirmation && autoConfirmation.children.some(function (child) {
    return child.textContent.includes('Pilot');
  }), 'the same exact episode is already identified before the import click');
  const beforeSnapshotPost = submits;
  const beforeSnapshotAssignment = metadataAssignments.length;
  assert(!findClass(mount, 'media-import-start').disabled,
    'metadata availability adds no gate to the video import');
  await findClass(mount, 'media-import-start').listeners.click();
  await settle();
  assert.strictEqual(submits, beforeSnapshotPost + 1,
    'one recognized episode creates exactly one normal import operation');
  const frozenPost = requests.filter(function (entry) {
    return entry.path.endsWith('/media-imports') &&
      entry.options.method === 'POST';
  }).slice(-1)[0];
  const frozenBody = JSON.parse(frozenPost.options.body);
  assert.strictEqual(frozenBody.sourceId, SOURCE_D);
  assert(typeof frozenBody.metadata === 'string' &&
    frozenBody.metadata.startsWith('v2|episode|') &&
    frozenBody.metadata.includes('|Pilot|') &&
    frozenBody.metadata.includes('|1|1|tv-episode|'),
    'the exact pre-found episode is frozen into the one import request');
  const decodedFrozen = frozenBody.metadata.split('|');
  assert.strictEqual(decodeURIComponent(decodedFrozen[2]), 'Andor');
  assert.strictEqual(decodeURIComponent(decodedFrozen[3]), 'Pilot');
  assert.strictEqual(metadataAssignments.length, beforeSnapshotAssignment,
    'Core-owned v2 snapshot must not trigger a competing browser metadata assignment POST');
  assert.strictEqual(decodedFrozen[7], 'tv-episode',
    'previously selected TMDb kind is carried as metadata identity');
  assert(/^\d+$/.test(decodedFrozen[8]),
    'previously selected TMDb episode id is frozen with the operation');
  assert.strictEqual(decodeURIComponent(decodedFrozen[9]), '/preview.jpg',
    'the exact recognized series poster travels in the same persisted import snapshot');

  // MT.META6: after a successful S01E06 import, unrelated S01E02-S01E05
  // duplicates must immediately be actionable. Core readback's GET must not
  // leave every button stuck at "previous operation being read".
  assert(findClass(mount, 'media-import-import-title').textContent.includes(
    'Import abgeschlossen'), 'Core confirmed the independent import first');
  const afterImportRow = findClass(mount, 'media-import-selection').children.find(
    function (row) { return row.dataset.sourceId === SOURCE_C; });
  const afterImportDelete = afterImportRow &&
    findClass(afterImportRow, 'media-import-delete-duplicate-inline');
  const afterImportReason = afterImportRow &&
    findClass(afterImportRow, 'media-import-delete-blocked');
  assert(afterImportDelete && !afterImportDelete.disabled &&
    (!afterImportReason || afterImportReason.hidden),
    'confirmed S01E06 must not block independent S01E03 cleanup after status readback');
  const afterImportSubmits = submits;
  confirmLocal = false;
  await afterImportDelete.listeners.click();
  assert.strictEqual(submits, afterImportSubmits,
    'independent delete must reach confirmation without submitting when cancelled');
  confirmLocal = true;

  console.log('A7.5 real web composition and media import lifecycle: PASS');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
