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

async function runLogos(prefix) {
  const scope = 'https://suite.test' + prefix + '/frontend/';
  const base = 'https://suite.test' + prefix + '/channel-logos/';
  const own = 'vdr-suite-pwa-logos:' + encodeURIComponent(scope) + ':';
  const listeners = {}, stores = new Map();
  let clock = 1700000000000, reads = 0, failStorage = false;
  let status = 200, type = 'image/png', policy = 'public, max-age=3600', body = 'logo-one', age = '0', vary = '';
  const caches = {
    async open(name) {
      if (failStorage) throw Error('quota');
      if (!stores.has(name)) stores.set(name, new Map());
      const store = stores.get(name);
      return {
        async put(key, value) { store.set(key, value.clone()); },
        async match(key) { return store.get(key)?.clone(); },
        async keys() { return [...store.keys()].map(url => ({url})); },
        async delete(key) { return store.delete(key); }
      };
    },
    async keys() { return [...stores.keys()]; },
    async delete(name) { return stores.delete(name); }
  };
  function start() {
    vm.runInNewContext(read('service-worker.js'), {
      self: {registration: {scope}, addEventListener: (name, fn) => {listeners[name] = fn;}},
      URL, Response, Request, Headers, caches, Date: {now: () => clock},
      fetch: async request => {
        reads++;
        assert.equal(request.cache, 'no-store');
        return new Response(status === 204 ? null : body, {status, headers: {
          'Content-Type':type, 'Cache-Control':policy, Age:age, Vary:vary
        }});
      }
    });
  }
  async function load(url, options = {}, destination = 'image') {
    const request = new Request(url, options);
    Object.defineProperty(request, 'destination', {value: destination});
    const pending = [];
    let result;
    listeners.fetch({request, waitUntil:p=>pending.push(p), respondWith:p=>{result=p;}});
    if (!result) return undefined;
    const response = await result;
    await Promise.all(pending);
    return response;
  }
  start();
  const positive = base + 'das%20erste.png';
  assert.equal(await (await load(positive)).text(),'logo-one');
  assert.equal(reads,1);
  start(); // simulate a new worker with unchanged persistent Cache Storage
  assert.equal(await (await load(positive)).text(),'logo-one');
  assert.equal(reads,1);
  assert.equal((await load(positive)).headers.get('X-VDR-Suite-Logo-Expires-At'),null);
  clock += 3600001; body = 'logo-two';
  assert.equal(await (await load(positive)).text(),'logo-two');
  assert.equal(reads,2);
  const missing = base + 'missing.png';
  status=204; policy='public, max-age=300';
  assert.equal((await load(missing)).status,204);
  assert.equal((await load(missing)).status,204);
  assert.equal(reads,3);
  clock += 300001; status=200; policy='public, max-age=3600'; body='added';
  assert.equal(await (await load(missing)).text(),'added');
  assert.equal(reads,4);
  type='image/svg+xml';
  const sprite=base+'vdr-suite-brand/recording-genre-sprite.svg';
  assert.equal(await (await load(sprite,{},'')).text(),'added');
  assert.equal(await (await load(sprite,{},'')).text(),'added');
  assert.equal(reads,5);
  for (const url of [base+'a.png?token=x',base+'a.jpg',base+'nested%2f..%2fsecret.png', 'https://other.test/channel-logos/a.png']) {
    assert.equal(await load(url),undefined);
  }
  for (const options of [{headers:{Authorization:'Basic private'}},{headers:{Range:'bytes=0-20'}},{cache:'reload'},{method:'POST'}]) {
    assert.equal(await load(positive,options),undefined);
  }
  type='image/png'; policy='no-cache';
  await load(base+'old.png'); await load(base+'old.png'); assert.equal(reads,7);
  policy='public, max-age=3600'; vary='Cookie';
  await load(base+'private-variant.png'); await load(base+'private-variant.png'); assert.equal(reads,9);
  vary=''; status=404;
  await load(base+'broken.png'); await load(base+'broken.png'); assert.equal(reads,11);
  status=200; age='3600'; await load(base+'aged.png'); await load(base+'aged.png'); assert.equal(reads,13);
  age='0'; failStorage=true; assert.equal(await (await load(base+'quota.png')).text(),'added'); failStorage=false;
  assert.equal(reads,14);
  // Keep shared logo storage independent from private session cache purges.
  assert(stores.get(own+'v1').has(positive));
  assert.equal(stores.get(own+'v1').size,3);
  const foreign='vdr-suite-pwa-logos:'+encodeURIComponent('https://suite.test/other/frontend/')+':v0';
  stores.set(own+'v0',new Map()); stores.set(foreign,new Map());
  await new Promise(resolve=>listeners.activate({waitUntil:resolve}));
  assert(!stores.has(own+'v0')); assert(stores.has(foreign));
}
(async () => {
  await runLogos('');
  await runLogos('/vdr-suite');
  console.log('PWA.13.1 public positive/negative logo cache lifecycle PASS');
  await run('');
  await run('/vdr-suite');
  console.log('PWA.1 shell contract PASS');
})().catch(error => { console.error(error); process.exitCode = 1; });
