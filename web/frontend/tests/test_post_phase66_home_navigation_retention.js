'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const repoRoot = path.join(frontendRoot, '..', '..');
const remote = fs.readFileSync(path.join(frontendRoot, 'modules', 'remote.js'), 'utf8');
const serverPaths = fs.readFileSync(path.join(repoRoot, 'core/http/src/TestHttpServerPaths.inc'), 'utf8');
const serverAssets = fs.readFileSync(path.join(repoRoot, 'core/http/src/TestHttpServerAssets.inc'), 'utf8');

assert(
  serverPaths.includes('{"/frontend/app.js", "app.js", "application/javascript; charset=utf-8", "modules/remote.js"}'),
  'production app.js must continue to compose the existing remote addon'
);
assert(serverAssets.includes('content += "\\n\\n";'));
assert(serverAssets.includes('content += addon;'));

assert(
  remote.includes('function installHomeNavigationRetention()'),
  'the existing app addon must fence Home navigation before data-owner click listeners'
);
assert(
  remote.includes(".module-tab[data-module=\\\"overview\\\"], [data-brand-module=\\\"overview\\\"]") ||
    remote.includes(".module-tab[data-module=\"overview\"], [data-brand-module=\"overview\"]"),
  'both canonical Home launchers must use the same navigation fence'
);
assert(
  remote.includes("g.selectModule('overview')"),
  'the fence must delegate navigation to the canonical app.js owner'
);
assert(
  remote.includes("if(typeof v.stopPropagation==='function')v.stopPropagation()"),
  'Home navigation must not bubble into independent Home data refresh listeners'
);
assert(
  remote.includes("document.addEventListener('click',homeNavigationClick,true)"),
  'the fence must run in capture phase before the existing bubble listeners'
);
assert(
  remote.includes('installHomeNavigationRetention();styles();preloadRemoteImage();'),
  'the production remote/app addon setup must install the Home navigation fence eagerly'
);

const navigationStart = remote.indexOf('function homeNavigationTarget(');
const navigationEnd = remote.indexOf('\nfunction stop()', navigationStart);
assert(navigationStart >= 0 && navigationEnd > navigationStart);
const navigationSource = remote.slice(navigationStart, navigationEnd);

let captureListener = null;
let selectCount = 0;
let prevented = 0;
let stopped = 0;
let scrolled = 0;
const detail = {
  scrollIntoView(options) {
    scrolled += 1;
    assert.deepStrictEqual(options, {behavior: 'smooth', block: 'start'});
  }
};
const document = {
  __vdrSuiteHomeNavigationRetentionBound: false,
  addEventListener(type, listener, capture) {
    assert.strictEqual(type, 'click');
    assert.strictEqual(capture, true);
    captureListener = listener;
  },
  getElementById(id) {
    return id === 'detail-data' ? detail : null;
  }
};
const g = {
  selectModule(moduleName) {
    selectCount += 1;
    assert.strictEqual(moduleName, 'overview');
  }
};

vm.runInNewContext(
  navigationSource + '\ninstallHomeNavigationRetention();',
  {document, g}
);
assert.strictEqual(typeof captureListener, 'function');

const homeTab = {
  dataset: {module: 'overview'},
  closest(selector) {
    assert(selector.includes('.module-tab[data-module="overview"]'));
    return this;
  }
};
captureListener({
  target: homeTab,
  preventDefault() { prevented += 1; },
  stopPropagation() { stopped += 1; }
});
assert.strictEqual(selectCount, 1, 'bottom Home must delegate exactly once to app.js');
assert.strictEqual(prevented, 1);
assert.strictEqual(stopped, 1, 'bottom Home must not reach refresh listeners');
assert.strictEqual(scrolled, 0);

const brandHome = {
  dataset: {brandModule: 'overview'},
  closest() { return this; }
};
captureListener({
  target: brandHome,
  preventDefault() { prevented += 1; },
  stopPropagation() { stopped += 1; }
});
assert.strictEqual(selectCount, 2, 'top Home launcher must delegate exactly once to app.js');
assert.strictEqual(prevented, 2);
assert.strictEqual(stopped, 2, 'top Home launcher must not reach refresh listeners');
assert.strictEqual(scrolled, 1, 'top Home launcher keeps its existing scroll affordance');

const otherTarget = {closest() { return null; }};
captureListener({target: otherTarget});
assert.strictEqual(selectCount, 2, 'non-Home clicks must remain untouched');
assert.strictEqual(stopped, 2);

console.log('post-Phase-66 canonical Home navigation retention contract ok');
