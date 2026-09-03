'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const discoverySource = fs.readFileSync(
  path.join(frontendRoot, 'home-recording-discovery.js'),
  'utf8'
);
const remoteSource = fs.readFileSync(
  path.join(frontendRoot, 'modules', 'remote.js'),
  'utf8'
);

const navigationStart = remoteSource.indexOf('function homeNavigationTarget(');
const navigationEnd = remoteSource.indexOf('\nfunction stop()', navigationStart);
assert(navigationStart >= 0 && navigationEnd > navigationStart);
const navigationSource = remoteSource.slice(navigationStart, navigationEnd);

let selectedModule = 'overview';
let captureListener = null;
const bubbleListeners = [];
let scheduledRefreshes = 0;
let recordingFetches = 0;
let genreFetches = 0;
let genreRecordingFetches = 0;
let folderFetches = 0;
let selectCount = 0;
const styleNodes = new Map();

const discoveryHost = {
  querySelector() { return null; },
  insertBefore() {},
  appendChild() {}
};

const document = {
  __vdrSuiteHomeNavigationRetentionBound: false,
  head: {
    appendChild(node) {
      if (node && node.id) styleNodes.set(node.id, node);
    }
  },
  createElement(tagName) {
    return {
      tagName: String(tagName || '').toUpperCase(),
      id: '',
      className: '',
      textContent: '',
      dataset: Object.create(null),
      style: Object.create(null),
      appendChild() {},
      append() {},
      addEventListener() {},
      setAttribute() {},
      replaceChildren() {},
      querySelector() { return null; },
      querySelectorAll() { return []; }
    };
  },
  getElementById(id) {
    return styleNodes.get(id) || null;
  },
  querySelector(selector) {
    if (selector === '[data-home-zone="additional-sections"]') return discoveryHost;
    if (selector === '.module-tab.active[data-module="overview"]') {
      return selectedModule === 'overview' ? {dataset: {module: 'overview'}} : null;
    }
    return null;
  },
  addEventListener(type, listener, capture) {
    if (type !== 'click') return;
    if (capture === true) captureListener = listener;
    else bubbleListeners.push(listener);
  }
};

const client = {
  fetchClientRecordings() {
    recordingFetches += 1;
    return Promise.resolve({recordings: []});
  },
  fetchClientGenres() {
    genreFetches += 1;
    return Promise.resolve({genres: []});
  },
  fetchClientGenreRecordings() {
    genreRecordingFetches += 1;
    return Promise.resolve({recordings: []});
  },
  fetchClientRecordingFolder() {
    folderFetches += 1;
    return Promise.resolve({folders: [], recordings: []});
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
  setTimeout() {
    scheduledRefreshes += 1;
    return scheduledRefreshes;
  },
  VdrSuitePlatform: {
    getSelectedModule() { return selectedModule; },
    getSelectedBackendId() { return 'default'; },
    getClientApi() { return client; }
  },
  selectModule(moduleName) {
    selectCount += 1;
    selectedModule = moduleName;
  }
};
window.window = window;

const context = {
  window,
  document,
  console,
  g: window,
  setTimeout: window.setTimeout
};
vm.createContext(context);
vm.runInContext(discoverySource, context, {
  filename: 'web/frontend/home-recording-discovery.js'
});

assert.strictEqual(window.VdrSuiteHomeRecordingDiscovery.install(), true);
assert.strictEqual(recordingFetches, 0, 'lazy install must not fetch Newly Recorded eagerly');
assert.strictEqual(genreFetches, 0, 'lazy install must not fetch Genres eagerly');
assert.strictEqual(genreRecordingFetches, 0, 'lazy install must not fetch Genre recordings eagerly');
assert.strictEqual(folderFetches, 0, 'lazy install must not fetch Recording folders eagerly');
assert(
  bubbleListeners.length >= 1,
  'Recording Discovery must retain its Home navigation bubble listener'
);

const homeTab = {
  dataset: {module: 'overview'},
  closest(selector) {
    if (selector === '.module-tab[data-module], [data-brand-module]') return this;
    if (selector.includes('.module-tab[data-module="overview"]')) return this;
    return null;
  }
};

bubbleListeners.forEach(listener => listener({target: homeTab}));
assert(
  scheduledRefreshes >= 1,
  'without the navigation fence an existing Home listener would schedule Discovery refresh'
);
scheduledRefreshes = 0;

vm.runInContext(
  navigationSource + '\ninstallHomeNavigationRetention();',
  context
);
assert.strictEqual(typeof captureListener, 'function');

function assertNoDiscoveryRefetch(prefix) {
  assert.strictEqual(scheduledRefreshes, 0, prefix + ' must not schedule Recording Discovery refresh');
  assert.strictEqual(recordingFetches, 0, prefix + ' must not refetch Newly Recorded');
  assert.strictEqual(genreFetches, 0, prefix + ' must not refetch Genres');
  assert.strictEqual(genreRecordingFetches, 0, prefix + ' must not refetch Genre recordings');
  assert.strictEqual(folderFetches, 0, prefix + ' must not refetch Recording folders');
}

function dispatchHome(target) {
  let stopped = false;
  const event = {
    target,
    preventDefault() {},
    stopPropagation() { stopped = true; }
  };
  captureListener(event);
  if (!stopped) bubbleListeners.forEach(listener => listener(event));
  return stopped;
}

selectedModule = 'recordings2';
assert.strictEqual(dispatchHome(homeTab), true);
assert.strictEqual(selectCount, 1, 'lower Home must delegate exactly once to canonical app navigation');
assert.strictEqual(selectedModule, 'overview');
assertNoDiscoveryRefetch('lower Home return');

const brandHome = {
  dataset: {brandModule: 'overview'},
  closest(selector) {
    if (selector.includes('[data-brand-module="overview"]')) return this;
    return null;
  }
};
selectedModule = 'epg';
assert.strictEqual(dispatchHome(brandHome), true);
assert.strictEqual(selectCount, 2, 'upper Home launcher must delegate exactly once to canonical app navigation');
assert.strictEqual(selectedModule, 'overview');
assertNoDiscoveryRefetch('upper Home return');

assert(
  discoverySource.includes('loadNewly(client, backendId, generation)'),
  'explicit Recording Discovery refresh must continue to own Newly Recorded loading'
);
assert(
  discoverySource.includes('loadGenres(client, backendId, generation'),
  'explicit Recording Discovery refresh must continue to own Genre loading'
);
assert(
  discoverySource.includes('loadFolders(client, backendId, generation)'),
  'explicit Recording Discovery refresh must continue to own Recording-folder loading'
);

console.log('post-Phase-66 Recording Discovery Home retention contract ok');
