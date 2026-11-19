'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const read = name => fs.readFileSync(path.join(__dirname, '..', name), 'utf8');
const manifest = JSON.parse(read('manifest.webmanifest'));

async function run(prefix) {
  const scope = 'https://suite.test' + prefix + '/frontend/';
  const listeners = {};
  const stores = new Map();
  const own = 'vdr-suite-pwa-shell:' + encodeURIComponent(scope) + ':';
  stores.set(own + 'v0', new Map());
  stores.set('unrelated-cache', new Map());
  stores.set(own + 'other-installation:v0', new Map()); // not a production cache name
  const foreign = 'vdr-suite-pwa-shell:' + encodeURIComponent('https://suite.test/other/frontend/') + ':v0';
  stores.set(foreign, new Map());
  const oldArtwork = 'vdr-suite-pwa-artwork:' + encodeURIComponent(scope) + ':v1';
  const foreignArtwork = 'vdr-suite-pwa-artwork:' + encodeURIComponent('https://suite.test/other/frontend/') + ':v1';
  stores.set(oldArtwork, new Map());
  stores.set(foreignArtwork, new Map());
  let failStorage = false;
  let network = async () => new Response('private online data');
  const caches = {
    async open(name) {
      if (failStorage) throw Error('quota');
      if (!stores.has(name)) stores.set(name, new Map());
      const store = stores.get(name);
      return { async put(key, value) { store.set(key, value.clone()); }, async match(key) { return store.get(key)?.clone(); } };
    },
    async keys() { return [...stores.keys()]; },
    async delete(name) { return stores.delete(name); }
  };
  vm.runInNewContext(read('service-worker.js'), {
    self: { registration: { scope }, addEventListener: (name, handler) => { listeners[name] = handler; } },
    URL, Response, caches, fetch: request => network(request)
  });
  const lifecycle = name => new Promise(resolve => listeners[name]({ waitUntil: resolve }));
  const request = (url, mode = 'navigate', method = 'GET') => {
    let response;
    listeners.fetch({ request: { url, mode, method }, respondWith(value) { response = value; } });
    return response;
  };
  await lifecycle('install');
  assert.equal(stores.get(own + 'v2').size, 1);
  await lifecycle('activate');
  assert(!stores.has(own + 'v0'));
  assert(stores.has(foreign));
  assert(stores.has('unrelated-cache'));
  assert(!stores.has(oldArtwork));
  assert(stores.has(foreignArtwork));
  assert.equal(await (await request(scope)).text(), 'private online data');
  network = async () => new Response('unauthorized', { status: 401 });
  assert.equal((await request(scope)).status, 401);
  network = async () => { throw Error('offline'); };
  const shell = await (await request(scope)).text();
  assert.match(shell, /Server unavailable/);
  assert.match(shell, /addEventListener\("online"/);
  assert.match(await (await request(scope + 'index.html')).text(), /Protected content/);
  for (const url of [scope + '?token=x', scope + 'app.js', scope + 'style.css', scope + 'browser-performance-diagnostics.html', 'https://suite.test' + prefix + '/api/v1/recordings', 'https://suite.test' + prefix + '/recording-artwork/a', 'https://other.test/frontend/']) assert.equal(request(url), undefined);
  assert.equal(request(scope, 'cors'), undefined);
  assert.equal(request(scope, 'navigate', 'POST'), undefined);
  assert.equal(stores.get(own + 'v2').size, 1);
  failStorage = true;
  await lifecycle('install');
  assert.match(await (await request(scope)).text(), /Server unavailable/);
  assert.equal(new URL(manifest.start_url, scope + 'manifest.webmanifest').href, scope);
  assert.equal(new URL(manifest.scope, scope + 'manifest.webmanifest').href, scope);

  let call;
  const win = { isSecureContext: true, navigator: { serviceWorker: { register(...args) { call = args; return Promise.reject(Error('blocked')); } } }, VdrSuitePublicUrl: { resolvePath: value => prefix + value } };
  vm.runInNewContext(read('pwa-register.js'), { window: win });
  assert.equal(call[0], prefix + '/frontend/service-worker.js');
  assert.equal(call[1].scope, prefix + '/frontend/');
  assert.equal(call[1].updateViaCache, 'none');
  for (const window of [{ isSecureContext: false, navigator: {} }, { isSecureContext: true, navigator: {} }]) vm.runInNewContext(read('pwa-register.js'), { window });
}
(async () => {
  await run('');
  await run('/vdr-suite');
  console.log('PWA.1 shell contract PASS');
})().catch(error => { console.error(error); process.exitCode = 1; });
