'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

function deferred() {
  let resolve;
  let reject;
  const promise = new Promise((ok, fail) => {
    resolve = ok;
    reject = fail;
  });
  return {promise, resolve, reject};
}

const source = fs.readFileSync(
  path.join(__dirname, '..', 'modules', 'global-search.js'),
  'utf8'
);

const document = {
  readyState: 'loading',
  addEventListener: function () {},
  getElementById: function () { return null; },
  querySelector: function () { return null; },
  head: {appendChild: function () {}},
  body: {appendChild: function () {}},
  createElement: function () { throw new Error('DOM creation is not expected in this contract test'); }
};

const selectedModules = [];
const scrollCalls = [];
const window = {
  AbortController,
  Promise,
  Date,
  Intl,
  setTimeout,
  clearTimeout,
  selectModule(moduleName) {
    selectedModules.push(moduleName);
  },
  scrollTo(options) {
    scrollCalls.push(options);
  }
};
window.window = window;

vm.runInNewContext(source, {
  window,
  document,
  console,
  Promise,
  Date,
  Intl,
  AbortController,
  setTimeout,
  clearTimeout
}, {filename: 'global-search.js'});

const testApi = window.VdrSuiteGlobalSearch.__test;
assert.strictEqual(testApi.minimumQueryLength, 2);
assert.strictEqual(testApi.debounceMs, 280);
assert.strictEqual(testApi.requestTimeoutMs, 12000);
assert(source.includes("homeLabel: '⌂ Home'"));
assert(source.includes('focusNavigation: true'));
assert(source.includes('onHome: returnHome'));
testApi.returnHome();
assert.deepStrictEqual(selectedModules, ['overview']);
assert.strictEqual(scrollCalls.length, 1);
assert.strictEqual(scrollCalls[0].top, 0);
assert.strictEqual(scrollCalls[0].left, 0);
assert.strictEqual(scrollCalls[0].behavior, 'auto');

const event = {backendId:'living-room',channelId:'S19.2E-1-2',eventId:'42',
  artwork:{available:true,url:'/api/epg/cache/artwork?backend=living-room&channelId=S19.2E-1-2&eventId=42'}};
const artworkSources = testApi.epgArtworkSources(event);
assert.strictEqual(artworkSources.length,2);
assert.strictEqual(artworkSources[0],'/api/v1/epg/artwork?backendId=living-room&channelId=S19.2E-1-2&eventId=42&variant=home');
assert.strictEqual(artworkSources[1],event.artwork.url);
assert.strictEqual(testApi.epgArtworkSources({artwork:{available:false}}).length,0);
assert.strictEqual(testApi.epgArtworkSources({artwork:event.artwork})[0],event.artwork.url);
let errorHandler = null;
let missing = 0;
const image = {src:'',addEventListener(name, handler, options) {
  assert.strictEqual(name,'error'); assert.strictEqual(options.once,true);
  errorHandler = handler;
}};
testApi.setEpgArtworkImage(image,artworkSources,()=>{missing+=1;});
assert.strictEqual(image.src,artworkSources[0]);
errorHandler();
assert.strictEqual(image.src,artworkSources[1]);
assert.strictEqual(missing,0);
errorHandler();
assert.strictEqual(missing,1);

(async function run() {
  const first = deferred();
  const second = deferred();
  let calls = 0;
  const coordinator = testApi.createRequestCoordinator(function () {
    calls += 1;
    return calls === 1 ? first.promise : second.promise;
  });

  const oldRequest = coordinator.run('John');
  const newRequest = coordinator.run('John Travolta');
  second.resolve({query: 'John Travolta'});
  const newResult = await newRequest;
  assert.strictEqual(newResult.current, true);
  assert.strictEqual(newResult.payload.query, 'John Travolta');

  first.resolve({query: 'John'});
  const oldResult = await oldRequest;
  assert.strictEqual(oldResult.current, false);
  assert.strictEqual(oldResult.payload.query, 'John');

  const pending = deferred();
  const invalidated = testApi.createRequestCoordinator(function () { return pending.promise; });
  const invalidatedRequest = invalidated.run('Pulp Fiction');
  invalidated.invalidate();
  pending.resolve({query: 'Pulp Fiction'});
  const invalidatedResult = await invalidatedRequest;
  assert.strictEqual(invalidatedResult.current, false);


  const timedOut = testApi.createRequestCoordinator(function (_value, signal) {
    return new Promise(function (_resolve, reject) {
      signal.addEventListener('abort', function () {
        const error = new Error('aborted');
        error.name = 'AbortError';
        reject(error);
      }, {once: true});
    });
  }, 5);
  const timeoutResult = await timedOut.run('Langsame Suche');
  assert.strictEqual(timeoutResult.current, true);
  assert(timeoutResult.error);
  assert.match(timeoutResult.error.message, /dauerte zu lange/);

  console.log('global search frontend runtime tests passed');
})().catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
