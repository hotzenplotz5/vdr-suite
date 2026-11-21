'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');
const appSource = fs.readFileSync(path.join(frontendRoot, 'app.js'), 'utf8');
const bootstrapSource = fs.readFileSync(
  path.join(frontendRoot, 'home-recording-discovery-bootstrap.js'),
  'utf8'
);

const deferredIndex = indexSource.indexOf('../frontend/platform/deferred-runtime-loader.js');
const clientIndex = indexSource.indexOf('../frontend/api/client-api.js');
const discoveryIndex = indexSource.indexOf('../frontend/home-recording-discovery.js');
const heroIndex = indexSource.indexOf('../frontend/home-live-hero.js');
const appIndex = indexSource.indexOf('../frontend/app.js');
assert(deferredIndex >= 0 && clientIndex > deferredIndex);
assert(discoveryIndex > clientIndex);
assert(heroIndex > discoveryIndex);
assert(appIndex > heroIndex);

const markStart = appSource.indexOf('function markSelected(backendId)');
const markEnd = appSource.indexOf('\nfunction loadBackendDetails(', markStart);
const loadStart = markEnd + 1;
const loadEnd = appSource.indexOf('\nfunction renderBackend(', loadStart);
assert(markStart >= 0 && markEnd > markStart);
assert(loadStart > markEnd && loadEnd > loadStart);
const markSelectedSource = appSource.slice(markStart, markEnd);
const loadBackendDetailsSource = appSource.slice(loadStart, loadEnd);
assert(loadBackendDetailsSource.includes('markSelected(backendId);'));
assert(loadBackendDetailsSource.includes("cache: 'no-store', priority: 'high'"),
  'the real Home bootstrap assigns high priority to its existing small scope readback');
assert(
  loadBackendDetailsSource.indexOf('markSelected(backendId);') <
  loadBackendDetailsSource.indexOf("selectModule('overview');"),
  'Home resume must publish only after the canonical backend is selected'
);

function classList(initial) {
  const values = new Set(String(initial || '').split(/\s+/).filter(Boolean));
  return {
    contains(name) { return values.has(name); },
    add(name) { values.add(name); },
    remove(name) { values.delete(name); },
    toggle(name, enabled) {
      if (enabled === undefined) enabled = !values.has(name);
      if (enabled) values.add(name); else values.delete(name);
      return enabled;
    }
  };
}

class FakeElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.dataset = Object.create(null);
    this.className = '';
    this.classList = classList();
    this.parentNode = null;
    this.textContent = '';
  }

  appendChild(child) {
    if (!child) return child;
    child.parentNode = this;
    this.children.push(child);
    return child;
  }

  addEventListener() {}
}

function flush() {
  return new Promise(resolve => setImmediate(resolve));
}

async function main() {
  const backendHost = new FakeElement('section');
  const backendCard = new FakeElement('article');
  backendCard.dataset.backendId = 'backend-a';
  backendCard.classList = classList('backend-card');
  backendHost.appendChild(backendCard);

  const backendObservers = [];
  const deferredLoads = [];
  const documentListeners = Object.create(null);

  const document = {
    head: {appendChild() {}},
    createElement(tagName) { return new FakeElement(tagName); },
    getElementById(id) {
      if (id === 'backends') return backendHost;
      return null;
    },
    querySelector(selector) {
      if (selector === '#backends .backend-card.selected, #backends [aria-selected="true"]') {
        return backendCard.classList.contains('selected') ? backendCard : null;
      }
      return null;
    },
    querySelectorAll(selector) {
      if (selector === '.backend-card') return [backendCard];
      return [];
    },
    addEventListener(type, listener) {
      (documentListeners[type] ||= []).push(listener);
    }
  };

  const context = {
    window: {},
    document,
    console,
    setImmediate
  };
  context.window.window = context.window;
  context.window.document = document;
  context.window.console = console;
  context.window.loadVdrSuiteDeferredRuntime = function(id, src, readyCheck) {
    deferredLoads.push({id, src});
    context.window.VdrSuiteHomeRecordingDiscovery = {install() {}};
    assert.strictEqual(typeof readyCheck, 'function');
    assert.strictEqual(readyCheck(), true);
    return Promise.resolve();
  };
  context.window.MutationObserver = function(callback) {
    this.callback = callback;
    this.observe = (target, options) => {
      this.target = target;
      this.options = options;
    };
    this.disconnect = () => { this.disconnected = true; };
    backendObservers.push(this);
  };

  vm.createContext(context);
  vm.runInContext(
    `var selectedBackendId = '';\n` +
    `var selectedModule = 'overview';\n` +
    `window.VdrSuitePlatform = {\n` +
    `  getSelectedBackendId: function () { return selectedBackendId; },\n` +
    `  getSelectedModule: function () { return selectedModule; }\n` +
    `};\n` +
    markSelectedSource,
    context
  );

  vm.runInContext(bootstrapSource, context, {
    filename: 'web/frontend/home-recording-discovery-bootstrap.js'
  });

  assert.strictEqual(
    deferredLoads.length,
    0,
    'Discovery runtime must not load before app.js has selected a canonical backend'
  );
  assert.strictEqual(backendObservers.length, 1);
  assert.strictEqual(backendObservers[0].target, backendHost);
  assert.strictEqual(backendObservers[0].options.childList, true);
  assert.strictEqual(backendObservers[0].options.subtree, true);
  assert.strictEqual(backendObservers[0].options.attributes, true);
  assert(backendObservers[0].options.attributeFilter.includes('class'));

  vm.runInContext("markSelected('backend-a');", context);
  assert.strictEqual(backendCard.classList.contains('selected'), true);

  backendObservers[0].callback([{type: 'attributes', target: backendCard}]);
  await flush();
  await flush();

  assert.strictEqual(backendObservers[0].disconnected, true);
  assert.deepStrictEqual(deferredLoads, [{
    id: 'vdr-suite-home-recording-discovery-runtime',
    src: '/frontend/home-recording-discovery.js'
  }]);
  assert(context.window.VdrSuiteHomeRecordingDiscovery);
  assert.strictEqual(
    typeof context.window.VdrSuiteHomeRecordingDiscoveryBootstrap.loadWhenBackendReady,
    'function'
  );

  await context.window.VdrSuiteHomeRecordingDiscoveryBootstrap.loadWhenBackendReady();
  assert.strictEqual(
    deferredLoads.length,
    1,
    'Already-ready Discovery runtime must not be injected a second time'
  );

  // PWA.15.22: the first existing Home snapshot is the authorization
  // readback needed for local artwork. It must be dispatched before the
  // recording-count request, and with a browser network priority hint.
  // No duplicate snapshot or separate authorization request is allowed.
  const clientSource = fs.readFileSync(path.join(frontendRoot, 'api/client-api.js'), 'utf8');
  const snapshotRequests = [];
  const clientWindow = {};
  vm.runInNewContext(clientSource, {
    window:clientWindow, Promise, URLSearchParams,
    fetch(url, options) {
      snapshotRequests.push({url, options});
      const payload = url.indexOf('/snapshot') >= 0
        ? {recordingCount:1} : {totalCount:12};
      return Promise.resolve({
        ok:true, status:200, redirected:false,
        text() { return Promise.resolve(JSON.stringify(payload)); }
      });
    }
  }, {filename:'web/frontend/api/client-api.js'});
  let earlySnapshot;
  const snapshotResult = await clientWindow.VdrSuiteClientApi.fetchClientBackendSnapshot(
    'backend-a', {cache:'no-store',priority:'high'}, value => {earlySnapshot = value; });
  assert.strictEqual(snapshotRequests.length,2,
    'the normal snapshot plus recording-count read; no extra HTTP readback');
  assert.strictEqual(snapshotRequests[0].url,'/api/backends/backend-a/snapshot');
  assert.strictEqual(snapshotRequests[0].options.priority,'high',
    'first small scope-bearing snapshot wins priority over speculative images');
  assert.strictEqual(snapshotRequests[0].options.cache,'no-store');
  assert.strictEqual(snapshotRequests[0].options.credentials,undefined,
    'no new auth mode or request credentials');
  assert.strictEqual(snapshotRequests[1].options.priority,undefined,
    'large/independent recording count read remains normal priority');
  assert.strictEqual(earlySnapshot.recordingCount,1,
    'early snapshot callback still fires before count composition');
  assert.strictEqual(snapshotResult.recordingCount,12,
    'normal combined snapshot result still uses the recording-count read');


  // PWA.15.24: controllerchange must reuse the existing API owner's scope
  // readback, but never launch the independent recording-count request.
  const proofOnlyBefore = snapshotRequests.length;
  const proofOnly = await clientWindow.VdrSuiteClientApi.fetchClientBackendSnapshot(
    'backend-a', {cache:'no-store',priority:'high',artworkProofReadbackOnly:true});
  assert.strictEqual(snapshotRequests.length,proofOnlyBefore+1,
    'one scope-only snapshot must issue exactly one GET');
  assert.strictEqual(snapshotRequests.at(-1).url,'/api/backends/backend-a/snapshot');
  assert.strictEqual(snapshotRequests.at(-1).options.priority,'high');
  assert.strictEqual(snapshotRequests.at(-1).options.cache,'no-store');
  assert.strictEqual(proofOnly.recordingCount,1,
    'scope-only readback returns the original snapshot without count composition');

  // Execute the actual production Home and browser-session owner sources in
  // the same VM. A simulated authentication restore triggers loadBackendDetails,
  // and a real-style SW controllerchange triggers the one-off scope readback.
  const authStart = appSource.indexOf('let browserSessionFrontendActive = false;');
  const authEnd = appSource.indexOf('// Phase 58.90b:',authStart);
  assert(authStart>=0 && authEnd>authStart);
  const authSource = appSource.slice(authStart,authEnd);
  assert(authSource.includes("addEventListener('controllerchange'"),
    'the canonical authenticated Home owner must register controller recovery');
  const callbacks = {};
  const sw = {controller:null,addEventListener(type,fn){callbacks[type]=fn;}};
  let publishSession;
  let ownerContext;
  const ownerSession = {
    subscribe(fn){publishSession=fn;},
    restore(){publishSession({authenticated:true});return Promise.resolve(true);}
  };
  const ownerWindow = {navigator:{serviceWorker:sw},VdrSuiteBrowserSession:ownerSession};
  const ownerDocument = {
    visibilityState:'visible',
    querySelector(){return null;},
    createElement(){return {className:'',textContent:''};},
    getElementById(){return null;}
  };
  const ownerBackends = {replaceChildren(){}}, ownerDetail = {replaceChildren(){},appendChild(){},getClientRects(){return [];}};
  const owner = {
    window:ownerWindow, document:ownerDocument, Promise, console,
    selectedBackend:null,selectedBackendId:'',selectedModule:'overview',
    currentSnapshot:null,backendDetailSequence:0,
    backendsElement:ownerBackends,detailDataElement:ownerDetail,
    detailMetaElement:{},detailDataElementOwner:ownerDetail,
    refreshDetailButton:{},statusElement:{},
    resetMediaImportVisibility(){},refreshMediaImportVisibility(){},
    closeHomeHevcStream(){},renderSnapshotMetrics(){},
    markHomeStartup(){},reportConnectionUnavailable(){},
    frontendPlatformClientApi(){return clientWindow.VdrSuiteClientApi;},
    markSelected(id){owner.selectedBackendId=id;},
    selectModule(name){owner.selectedModule=name;},
    loadBackendSelection(){
      return vm.runInContext("loadBackendDetails({frontendSelector:{id:'backend-a',label:'Backend A'}})", ownerContext);
    }
  };
  ownerContext=vm.createContext(owner);
  const startRequests = snapshotRequests.length;
  vm.runInContext(loadBackendDetailsSource+'\n'+authSource,ownerContext,
    {filename:'web/frontend/app.js:Home-owner'});
  assert.strictEqual(typeof callbacks.controllerchange,'function');
  await flush();
  assert.strictEqual(snapshotRequests.length,startRequests+2,
    'authenticated initial Home retains original snapshot and count requests');
  assert.strictEqual(snapshotRequests[startRequests].options.priority,'high');
  assert.strictEqual(snapshotRequests[startRequests+1].options.priority,undefined);

  // No controller -> no new request. A new controller triggers one snapshot.
  callbacks.controllerchange();
  assert.strictEqual(snapshotRequests.length,startRequests+2);
  sw.controller={worker:1};
  callbacks.controllerchange();
  assert.strictEqual(snapshotRequests.length,startRequests+3);
  assert.strictEqual(snapshotRequests.at(-1).url,'/api/backends/backend-a/snapshot');
  assert.strictEqual(snapshotRequests.at(-1).options.priority,'high');
  assert.strictEqual(snapshotRequests.at(-1).options.cache,'no-store');
  callbacks.controllerchange();
  assert.strictEqual(snapshotRequests.length,startRequests+3,
    'duplicate controllerchange without a new controller must do nothing');

  owner.selectedModule='recordings';
  sw.controller={worker:2};
  callbacks.controllerchange();
  assert.strictEqual(snapshotRequests.length,startRequests+3,
    'leaving Home must not launch extra snapshot traffic');
  owner.selectedModule='overview';
  sw.controller={worker:3};
  callbacks.controllerchange();
  assert.strictEqual(snapshotRequests.length,startRequests+4,
    'returning to Home requires a new actual controller transition');

  publishSession({authenticated:false});
  sw.controller={worker:4};
  callbacks.controllerchange();
  assert.strictEqual(snapshotRequests.length,startRequests+4,
    'signed-out Home must never request a recovery proof');
  assert.strictEqual(owner.selectedBackendId,'');
  assert.strictEqual(owner.selectedBackend,null);
  publishSession({authenticated:true});
  await flush();
  assert.strictEqual(snapshotRequests.length,startRequests+6,
    'the normal authenticated bootstrap still fetches snapshot plus count');
  sw.controller={worker:5};
  callbacks.controllerchange();
  assert.strictEqual(snapshotRequests.length,startRequests+7,
    're-authenticated Home can recover once after a genuine controller change');
  assert.strictEqual(snapshotRequests.at(-1).url,'/api/backends/backend-a/snapshot');
  assert.strictEqual(snapshotRequests.at(-1).options.credentials,undefined,
    'recovery does not change the existing credential mode');
  console.log('PWA.15.24 authenticated Home controller proof recovery PASS');

  console.log('phase66 home initial bootstrap backend readiness ok');
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
