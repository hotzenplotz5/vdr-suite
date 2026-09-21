'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-recently-watched.js'), 'utf8');

const requests = [];
const observers = [];
let selectedModule = 'recordings2';

const overviewTab = {
  dataset: {module: 'overview'},
  classList: {
    contains(name) {
      return name === 'active' && selectedModule === 'overview';
    }
  }
};

const moduleNav = {};
const additionalSections = {
  lastElementChild: null,
  querySelector() { return null; },
  appendChild(node) { this.lastElementChild = node; }
};

class FakeMutationObserver {
  constructor(callback) {
    this.callback = callback;
    this.target = null;
    this.options = null;
    observers.push(this);
  }

  observe(target, options) {
    this.target = target;
    this.options = options;
  }

  disconnect() {}
}

const document = {
  readyState: 'loading',
  addEventListener() {},
  querySelector(selector) {
    if (selector === '#module-nav') return moduleNav;
    if (selector === '[data-home-zone="additional-sections"]') return additionalSections;
    return null;
  }
};

const window = {
  document,
  MutationObserver: FakeMutationObserver,
  setTimeout(callback) {
    callback();
    return 1;
  },
  clearTimeout() {},
  fetch: async (requestPath, options) => {
    requests.push({path: requestPath, options});
    return {ok: true, json: async () => ({items: []})};
  },
  VdrSuitePlatform: {
    getSelectedModule() { return selectedModule; },
    getSelectedBackendId() { return 'default'; }
  },
  VdrSuiteBrowserSession: {
    csrfHeaders() { return {'X-VDR-Suite-CSRF': 'phase66-history-navigation'}; }
  }
};
window.window = window;

const context = {
  window,
  document,
  fetch: window.fetch,
  console,
  setTimeout: window.setTimeout,
  clearTimeout: window.clearTimeout,
  MutationObserver: FakeMutationObserver
};

vm.createContext(context);
vm.runInContext(source, context);

const history = window.VdrSuiteHomeRecentlyWatched;
assert(history && history._test);
assert.strictEqual(typeof history._test.installModuleObserver, 'function');

history._test.installModuleObserver();

const moduleObserver = observers.find(observer => observer.target === moduleNav);
assert(moduleObserver, 'History must observe the canonical module navigation state');
assert.strictEqual(moduleObserver.options.attributes, true);
assert.strictEqual(moduleObserver.options.subtree, true);
assert.deepStrictEqual(Array.from(moduleObserver.options.attributeFilter), ['class']);

requests.length = 0;
selectedModule = 'overview';
moduleObserver.callback([{target: overviewTab, attributeName: 'class'}]);

assert.strictEqual(requests.length, 1,
  'Programmatic return to the canonical Home module must refresh Recently Watched');
assert.strictEqual(requests[0].path, '/api/media/recently-watched');
assert.strictEqual(requests[0].options.credentials, 'same-origin');
assert.strictEqual(requests[0].options.headers['X-VDR-Suite-CSRF'], 'phase66-history-navigation');
assert.deepStrictEqual(JSON.parse(requests[0].options.body), {
  operation: 'list',
  backendId: 'default'
});

requests.length = 0;
selectedModule = 'recordings2';
moduleObserver.callback([{target: overviewTab, attributeName: 'class'}]);
assert.strictEqual(requests.length, 0,
  'Leaving Home must not trigger a Recently Watched refresh');

console.log('phase66 recently watched canonical Home lifecycle refresh ok');
