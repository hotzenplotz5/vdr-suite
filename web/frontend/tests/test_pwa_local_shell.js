'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const vm = require('node:vm');
const {webcrypto} = require('node:crypto');
const {generate} = require('../../../tools/build_pwa_shell.js');
const helper = fs.readFileSync(path.join(__dirname, '..', 'pwa-local-shell.js'), 'utf8');
const base = `const scope = new URL(self.registration.scope); const offlineShell = () => new Response('offline');
self.addEventListener('fetch', event => handleLocalShell(event));`;
const routes = `{"/frontend/app.js", "app.js", "application/javascript; charset=utf-8", "addon.js"},
{"/frontend/style.css", "style.css", "text/css; charset=utf-8", nullptr},
{"/frontend/browser-performance-diagnostics.js", "diagnostics.js", "application/javascript; charset=utf-8", nullptr}`;
const folder = fs.mkdtempSync(path.join(os.tmpdir(), 'pwa-shell-test-'));
function make() {return generate(folder, routes, base, helper);}
function fixture(prefix, code, fail = '') {
  const scope = 'https://suite.test' + prefix + '/frontend/';
  const stores = new Map(), listeners = {}, calls = [];
  let clientUrl = scope, offline = false;
  const caches = {
    async open(name) {
      if (fail === 'storage') throw Error('quota');
      if (!stores.has(name)) stores.set(name, new Map());
      const store = stores.get(name);
      return {async keys() {return [...store.keys()].map(url => ({url}));},
        async delete(key) {return store.delete(key);},
        async put(key, response) {store.set(key, response.clone());},
        async match(key) {return store.get(key)?.clone();}};
    }, async keys() {return [...stores.keys()];}, async delete(name) {return stores.delete(name);}
  };
  const context = {URL, Request, Response, Uint8Array, crypto: webcrypto, caches,
    self: {registration: {scope}, clients: {get: async () => ({url: clientUrl})},
      addEventListener(name, fn) {listeners[name] = fn;}},
    fetch: async (request, init = {}) => {
      calls.push({url: request.url, credentials: request.credentials,
        cache: init.cache || request.cache, redirect: request.redirect});
      if (offline) throw Error('offline');
      if (request.credentials !== 'omit') return new Response('network');
      const name = request.url.slice(scope.length);
      if (fail === '401' && name === 'app.js') return new Response('unauthorized', {status: 401});
      const bytes = name === 'app.js' ? 'app\n\naddon' : name === 'style.css' ? 'css' : 'html';
      return new Response(fail === 'changed' && name === 'app.js' ? 'different' : bytes, {headers: {
        'Content-Type': name === 'app.js' ? 'application/javascript; charset=utf-8' : name === 'style.css' ? 'text/css' : 'text/html',
        'Cache-Control': fail === 'private' ? 'private' : 'no-cache'}});
    }};
  vm.runInNewContext(code, context);
  const lifecycle = name => new Promise(resolve => listeners[name]({waitUntil: resolve}));
  function request(url, mode = 'navigate', destination = '', method = 'GET') {
    let result;
    listeners.fetch({request: {url, mode, method, destination, credentials: 'same-origin'}, clientId: 'client', respondWith(value) {result = value;}});
    return result;
  }
  return {scope, stores, calls, lifecycle, request, client: value => {clientUrl = value;}, offline: () => {offline = true;}};
}
async function run() {
  try {
    for (const [name, bytes] of Object.entries({'index.html':'html','app.js':'app','addon.js':'addon','style.css':'css','diagnostics.js':'diagnostic'})) fs.writeFileSync(path.join(folder, name), bytes);
    const code = make();
    fs.writeFileSync(path.join(folder, 'addon.js'), 'changed');
    assert.notEqual(make(), code, 'composed addon changes worker release identity');
    fs.writeFileSync(path.join(folder, 'addon.js'), 'addon');
    assert.equal(make(), code);

    // The served HTML differs from its raw staged bytes since the daemon
    // injects the exact SearchTimer script SHA into the index response.
    const crypto = require('node:crypto');
    const marker = 'src="../frontend/modules/searchtimers.js"';
    const index = '<!doctype html><script ' + marker + '></script>';
    fs.mkdirSync(path.join(folder, 'modules'));
    fs.writeFileSync(path.join(folder, 'modules/searchtimers.js'), 'search-v2');
    fs.writeFileSync(path.join(folder, 'index.html'), index);
    const manifestText = make();
    const match = manifestText.match(/const localShellRelease = (\{[^\n]+\});/);
    assert(match, 'generated worker contains static manifest');
    const manifest = JSON.parse(match[1]).assets;
    const version = crypto.createHash('sha256').update('search-v2').digest('hex').slice(0, 16);
    const servedIndex = index.replace(marker,
      'src="../frontend/modules/searchtimers.js?v=' + version + '"');
    assert.equal(manifest['index.html'].sha256,
      crypto.createHash('sha256').update(servedIndex).digest('hex'),
      'manifest must hash the actually served versioned HTML');
    fs.rmSync(path.join(folder, 'modules'), {recursive: true});
    fs.writeFileSync(path.join(folder, 'index.html'), 'html');

    for (const prefix of ['', '/vdr-suite']) {
      const f = fixture(prefix, code);
      await f.lifecycle('install');
      assert.equal(f.calls.length, 3);
      assert(f.calls.every(request => request.credentials === 'omit' && request.cache === 'no-store' && request.redirect === 'error'));
      const own = 'vdr-suite-pwa-static:' + encodeURIComponent(f.scope) + ':';
      f.stores.set(own + 'old', new Map());
      f.stores.set('other-installation', new Map());
      await f.lifecycle('activate');
      assert(!f.stores.has(own + 'old'));
      assert(f.stores.has('other-installation'));
      // Warm online sessions must never use the stale precached HTML/JS.
      assert.equal(await (await f.request(f.scope)).text(), 'network');
      assert.equal(await (await f.request(f.scope + 'index.html')).text(), 'network');
      assert.equal(await (await f.request(f.scope + 'app.js', 'cors', 'script')).text(), 'network');
      assert.equal(await (await f.request(f.scope + 'style.css', 'cors', 'style')).text(), 'network');
      assert.equal(f.calls.length, 7, 'online HTML and scripts must revalidate via network');
      assert(f.calls.slice(3).every(call => call.cache === 'no-store'),
        'ordinary F5 must bypass stale HTTP cache through the worker');

      // Complete, verified static shell remains available if offline.
      f.offline();
      assert.equal(await (await f.request(f.scope)).text(), 'html');
      assert.equal(await (await f.request(f.scope + 'app.js', 'cors', 'script')).text(), 'app\n\naddon');
      const version = require('node:crypto').createHash('sha256').update('app\n\naddon').digest('hex').slice(0, 16);
      assert.equal(await (await f.request(f.scope + 'app.js?v=' + version, 'cors', 'script')).text(),
        'app\n\naddon', 'versioned script resolves to exact cached release when offline');
      assert.equal(f.request(f.scope + 'app.js?v=0000000000000000', 'cors', 'script'), undefined,
        'mismatched version must not serve stale script');
      for (const url of [f.scope + '?x=1', f.scope + 'browser-performance-diagnostics.html', 'https://suite.test' + prefix + '/api/private']) assert.equal(f.request(url), undefined);
      assert.equal(f.request(f.scope, 'navigate', '', 'POST'), undefined);
      f.client(f.scope + 'browser-performance-home.html');
      await assert.rejects(f.request(f.scope + 'app.js', 'cors', 'script'),
        /offline/, 'unrelated pages must not receive the cached shell');
      for (const fail of ['storage', '401', 'private', 'changed']) {
        const bad = fixture(prefix, code, fail);
        await bad.lifecycle('install');
        assert.equal(await (await bad.request(bad.scope)).text(), 'network');
        bad.offline();
        assert.equal(await (await bad.request(bad.scope)).text(), 'offline');
      }
    }
    console.log('PWA.10 versioned local shell contract PASS');
  } finally {fs.rmSync(folder, {recursive:true, force:true});}
}
run().catch(error => {console.error(error); process.exitCode = 1;});
