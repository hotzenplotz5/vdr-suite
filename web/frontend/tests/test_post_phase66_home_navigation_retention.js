'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const frontendRoot = path.join(__dirname, '..');
const discovery = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8');
const remote = fs.readFileSync(path.join(frontendRoot, 'modules', 'remote.js'), 'utf8');

assert(
  discovery.includes("homeReadyBackendId: ''"),
  'Recording Discovery must track the same-backend Home projection that is already settled'
);
assert(
  discovery.includes('if (state.homeReadyBackendId === backendId) return Promise.resolve(true);'),
  'normal Home return must reuse the settled same-backend Recording Discovery projection'
);
assert(
  discovery.includes("state.homeReadyBackendId = backendId;"),
  'a settled Recording Discovery refresh must certify the displayed same-backend Home state'
);
assert(
  discovery.includes("state.homeReadyBackendId = '';"),
  'explicit refresh/backend invalidation must be able to clear retained Home readiness'
);
assert(
  discovery.includes('if (state.homeReadyBackendId === backendId) return true;'),
  'Home scheduling must not re-arm lazy Recording Discovery for an already settled backend'
);

assert(
  remote.includes(".module-tab.active[data-module=\\\"overview\\\"]") ||
    remote.includes(".module-tab.active[data-module=\"overview\"]"),
  'remote Home navigation must detect an already active canonical Home tab'
);
assert(
  remote.includes("if(!active&&tab&&typeof tab.click==='function')tab.click()"),
  'remote Home hotspot must not synthesize another Home click while Home is already active'
);

console.log('post-Phase-66 Home navigation retention contract ok');
