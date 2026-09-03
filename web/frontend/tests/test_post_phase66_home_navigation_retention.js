'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

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

console.log('post-Phase-66 canonical Home navigation retention contract ok');
