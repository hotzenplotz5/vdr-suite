'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');

const root = path.resolve(__dirname, '..');
const read = name => fs.readFileSync(path.join(root, name), 'utf8');
const paths = read('core/http/src/TestHttpServerPaths.inc');
const routes = read('core/http/src/TestHttpServerRoutes.inc');
const server = read('core/http/src/TestHttpServer.cpp');
const assets = read('core/http/src/TestHttpServerAssets.inc');
const install = read('mk/browser-performance-diagnostics.mk');
const ordinaryIndex = read('web/frontend/index.html');
const names = [
  'browser-artwork-probe.js',
  'browser-performance-bridge.js',
  'browser-performance-diagnostics.js',
  'browser-performance-diagnostics.html',
  'browser-performance-home.html'
];

// The daemon uses an explicit allowlist, not a generic filesystem route.
const entries = [...paths.matchAll(/\{"(\/frontend\/[^"\n]+)", "([^"\n]+)", "([^"\n]+)", (nullptr|"[^"\n]+")\}/g)];
const byPath = new Map();
for (const entry of entries) {
  assert.ok(!byPath.has(entry[1]), 'duplicate frontend route: ' + entry[1]);
  byPath.set(entry[1], { file: entry[2], type: entry[3], addon: entry[4] });
}
for (const name of names) {
  const route = '/frontend/' + name;
  const entry = byPath.get(route);
  assert.ok(entry, 'missing production HTTP route: ' + route);
  assert.equal(entry.file, name);
  assert.equal(entry.type, name.endsWith('.html') ? 'text/html; charset=utf-8' : 'application/javascript; charset=utf-8');
  assert.equal(entry.addon, 'nullptr', 'diagnostics must not be bundled into ordinary assets');
  assert.ok(install.includes('web/frontend/' + name) || name === 'browser-performance-home.html' && install.includes('build_browser_performance_diagnostics.js'));
}
assert.ok(routes.includes('return isIndexPath(path) || findFrontendAsset(path) != nullptr;'));
assert.ok(routes.includes('return makeFrontendAssetResponse('));
assert.ok(server.includes('request.method == "GET" &&'));
assert.ok(server.includes('isFrontendPath(request.path)'));
assert.ok(server.includes('return serveFrontendPath(request.path);'));
assert.ok(assets.includes('response.headers["Cache-Control"] = "no-cache";'));
assert.ok(assets.includes('return makeStaticNotFoundResponse();'));
assert.ok(!ordinaryIndex.includes('browser-performance-bridge.js'));
assert.ok(!ordinaryIndex.includes('browser-artwork-probe.js'));
console.log('production browser diagnostics route registration and asset policy ok');
