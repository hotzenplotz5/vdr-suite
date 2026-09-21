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
const appIndex = indexSource.indexOf('../frontend/app.js');
assert(deferredIndex >= 0 && clientIndex > deferredIndex && appIndex > clientIndex);

const markStart = appSource.indexOf('function markSelected(backendId)');
const markEnd = appSource.indexOf('\nfunction loadBackendDetails(', markStart);
const loadStart = markEnd + 1;
const loadEnd = appSource.indexOf('\nfunction renderBackend(', loadStart);
assert(markStart >= 0 && markEnd > markStart);
assert(loadStart > markEnd && loadEnd > loadStart);
const markSelectedSource = appSource.slice(markStart, markEnd);
const loadBackendDetailsSource = appSource.slice(loadStart, loadEnd);
assert(loadBackendDetailsSource.includes('markSelected(backendId);'));
assert(
  loadBackendDetailsSource.indexOf("selectModule('overview');") <
  loadBackendDetailsSource.indexOf('markSelected(backendId);')
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

  console.log('phase66 home initial bootstrap backend readiness ok');
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
