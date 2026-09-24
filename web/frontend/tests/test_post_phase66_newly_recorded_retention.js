'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(
  path.join(frontendRoot, 'home-recording-discovery.js'),
  'utf8'
);

assert(source.includes("const HOME_RESUME_EVENT = 'vdr-suite:home-resume';"));
assert(source.includes('Promise.allSettled(loads)'));
assert(source.includes('parallelHomeResume'));
assert(source.includes('state.seriesAvailable === true'));
assert(source.includes("loadSeries(client, backendId, generation, [{id: 'series'}]"));
assert(source.includes('retainVisible: true'));
assert(!source.includes('refreshRecordingPresentationDependents'));

let selectedModule = 'overview';
let mode = 'initial';
let recordingFetches = 0;
let genreFetches = 0;
let seriesFetches = 0;
let folderFetches = 0;
const listeners = new Map();
const styleNodes = new Map();

function pending() {
  return new Promise(() => {});
}

function element(tagName) {
  return {
    tagName: String(tagName || '').toUpperCase(),
    id: '',
    className: '',
    textContent: '',
    dataset: Object.create(null),
    style: Object.create(null),
    children: [],
    childNodes: [],
    parentNode: null,
    appendChild(child) {
      if (child) child.parentNode = this;
      this.children.push(child);
      this.childNodes.push(child);
      return child;
    },
    append() {
      Array.from(arguments).forEach(child => this.appendChild(child));
    },
    addEventListener() {},
    setAttribute(name, value) {
      if (name === 'data-home-discovery-rail') {
        this.dataset.homeDiscoveryRail = String(value);
      }
    },
    replaceChildren() {
      this.children = [];
      this.childNodes = [];
      Array.from(arguments).forEach(child => this.appendChild(child));
    },
    querySelector() { return null; },
    querySelectorAll() { return []; },
    remove() {}
  };
}

const host = {
  children: [],
  querySelector() { return null; },
  appendChild(child) {
    this.children.push(child);
    child.parentNode = this;
    return child;
  },
  insertBefore(child) {
    return this.appendChild(child);
  }
};

const document = {
  readyState: 'loading',
  hidden: false,
  head: {
    appendChild(node) {
      if (node && node.id) styleNodes.set(node.id, node);
    }
  },
  createElement: element,
  getElementById(id) {
    return styleNodes.get(id) || null;
  },
  querySelector(selector) {
    if (selector === '[data-home-zone="additional-sections"]') return host;
    if (selector === '.module-tab.active[data-module="overview"]') {
      return selectedModule === 'overview' ? {dataset: {module: 'overview'}} : null;
    }
    return null;
  },
  addEventListener(type, listener) {
    const values = listeners.get(type) || [];
    values.push(listener);
    listeners.set(type, values);
  },
  dispatchEvent(event) {
    (listeners.get(event && event.type) || []).forEach(listener => listener(event));
    return true;
  }
};

const client = {
  fetchClientRecordings() {
    recordingFetches += 1;
    return mode === 'initial'
      ? Promise.resolve({recordings: []})
      : pending();
  },
  fetchClientGenres() {
    genreFetches += 1;
    return mode === 'initial'
      ? Promise.resolve({genres: [{id: 'series', label: 'Serien', count: 1}]})
      : pending();
  },
  fetchClientGenreRecordings() {
    seriesFetches += 1;
    return mode === 'initial'
      ? Promise.resolve({recordings: [], totalCount: 0, hasMore: false})
      : pending();
  },
  fetchClientRecordingFolder() {
    folderFetches += 1;
    return mode === 'initial'
      ? Promise.resolve({folders: [], recordings: []})
      : pending();
  },
  createClientLiveUpdateSource() {
    return {
      onopen: null,
      onerror: null,
      addEventListener() {},
      close() {}
    };
  }
};

function IntersectionObserver() {
  this.observe = function () {};
  this.disconnect = function () {};
}

const window = {
  document,
  console,
  IntersectionObserver,
  Promise,
  setTimeout() { return 1; },
  clearTimeout() {},
  CustomEvent: function CustomEvent(type, options) {
    this.type = type;
    this.detail = options && options.detail;
  },
  VdrSuitePlatform: {
    getSelectedModule() { return selectedModule; },
    getSelectedBackendId() { return 'default'; },
    getClientApi() { return client; }
  }
};
window.window = window;

const context = vm.createContext({
  window,
  document,
  console,
  Promise,
  Map,
  Set,
  Math,
  Date,
  Number,
  String,
  Boolean,
  Object,
  Array
});

vm.runInContext(source, context, {
  filename: 'web/frontend/home-recording-discovery.js'
});

const api = window.VdrSuiteHomeRecordingDiscovery;
assert(api);
assert.strictEqual(api.install(), true);

(async function () {
  assert.strictEqual(recordingFetches, 0, 'lazy install must not fetch Newly Recorded eagerly');
  assert.strictEqual(genreFetches, 0, 'lazy install must not fetch Genres eagerly');
  assert.strictEqual(seriesFetches, 0, 'lazy install must not fetch Series eagerly');
  assert.strictEqual(folderFetches, 0, 'lazy install must not fetch folders eagerly');

  // Cold Home first establishes canonical genre availability. Series may depend
  // on that first genre result, but all later Home returns must not.
  assert.strictEqual(await api.refresh(), true);

  recordingFetches = 0;
  genreFetches = 0;
  seriesFetches = 0;
  folderFetches = 0;
  mode = 'pending';

  const homeResumeListeners = listeners.get('vdr-suite:home-resume') || [];
  assert.strictEqual(homeResumeListeners.length, 1,
    'Recording Discovery must have one canonical Home-resume listener');

  document.dispatchEvent(new window.CustomEvent('vdr-suite:home-resume', {
    detail: {backendId: 'default', previousModule: 'recordings2'}
  }));

  assert.strictEqual(recordingFetches, 1,
    'Home resume must start Newly Recorded immediately');
  assert.strictEqual(genreFetches, 1,
    'Home resume must start Genres immediately');
  assert.strictEqual(seriesFetches, 1,
    'known canonical Series must start immediately without waiting for the unresolved Genres revalidation');
  assert.strictEqual(folderFetches, 1,
    'Home resume must start Recording folders immediately');

  console.log('post-Phase-66 parallel Home Recording revalidation contract ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
