'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const vm = require('node:vm');
const { spawnSync } = require('node:child_process');
const frontend = path.resolve(__dirname, '../web/frontend');
const read = name => fs.readFileSync(path.join(frontend, name), 'utf8');
const run = (name, env) => vm.runInNewContext(read(name), env, { filename: name });
const { createProbe, summarizeResources, startupTiming } = require('../web/frontend/browser-artwork-probe');

async function main() {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'vdr-suite-diagnostics-'));
  try {
    const output = path.join(tmp, 'browser-performance-home.html');
    const build = spawnSync(process.execPath, [path.join(__dirname, 'build_browser_performance_diagnostics.js'), output], { encoding: 'utf8' });
    assert.equal(build.status, 0, build.stderr);
    const html = fs.readFileSync(output, 'utf8');
    assert.ok(html.includes('browser-artwork-probe.js'));
    assert.ok(html.includes('browser-performance-bridge.js'));
    assert.ok(html.indexOf('browser-performance-bridge.js') < html.indexOf('platform/bootstrap.js'));
    assert.ok(html.includes('../frontend/app.js'));
    assert.ok(html.includes('../frontend/platform/public-url.js'));
    assert.equal((html.match(/browser-performance-bridge.js/g) || []).length, 1);
    assert.ok(!html.includes('src="../frontend/browser-performance-bridge.js"'));
    assert.ok(!html.includes('vdr-suite-diagnostics=1'));
    assert.ok(!read('index.html').includes('browser-performance-bridge.js'));
    const page = read('browser-performance-diagnostics.html');
    for (const id of ['home', 'cold', 'clean', 'start', 'stop', 'headers', 'export', 'status', 'metrics', 'history', 'headers-result']) assert.ok(page.includes('id="' + id + '"'));

    const listeners = {};
    const sent = [];
    const frame = { attributes: {}, hasAttribute(name) { return name in this.attributes; }, getAttribute(name) { return this.attributes[name]; } };
    frame.attributes['data-vdr-suite-diagnostics'] = 'test-token';
    const parent = { postMessage(message, origin) { sent.push({ message, origin }); } };
    const calls = [];
    const probe = {
      start(label, options) { calls.push(['start', label, options]); },
      stop() { calls.push(['stop']); return { label: 'initial-home', metrics: { resourceRequests: 2 } }; },
      inspectHeaders() { calls.push(['headers']); return Promise.resolve({ status: 200, headers: {} }); }
    };
    const child = { frameElement: frame, parent, VdrSuiteArtworkProbe: probe, location: { origin: 'https://example.test' }, addEventListener(type, fn) { listeners[type] = fn; }, Promise };
    run('browser-performance-bridge.js', { window: child, Promise });
    assert.equal(calls[0][1], 'initial-home');
    assert.equal(calls[0][2].buffered, true);
    assert.equal(sent[0].message.type, 'started');
    const message = (type, extra) => listeners.message({ source: parent, origin: 'https://example.test', data: Object.assign({ channel: 'vdr-suite-browser-diagnostics', token: 'test-token', type }, extra) });
    listeners.message({ source: {}, origin: 'https://example.test', data: { channel: 'vdr-suite-browser-diagnostics', token: 'test-token', type: 'stop' } });
    assert.equal(calls.length, 1);
    message('stop');
    assert.equal(sent.at(-1).message.type, 'result');
    message('start', { label: 'navigation' });
    assert.equal(calls.at(-1)[2].buffered, false);
    message('stop');
    message('headers');
    await new Promise(resolve => setImmediate(resolve));
    assert.equal(sent.at(-1).message.type, 'headers');
    assert.equal(calls.at(-1)[0], 'headers');
    child.navigator = {serviceWorker:{controller:{postMessage(data, ports) {
      assert.equal(data.type,'vdr-suite-artwork-measurement');
      ports[0].send({schema:1,capacity:1024,maxPreviewBytes:262144,counters:{localHits:7,imageHttpRequests:2}});
    }}}};
    child.MessageChannel = class {
      constructor() {
        this.port1 = {close() {}};
        this.port2 = {send: value => this.port1.onmessage({data:value})};
      }
    };
    child.setTimeout = setTimeout;
    child.clearTimeout = clearTimeout;
    message('start',{label:'measurement'});
    message('stop');
    await new Promise(resolve=>setImmediate(resolve));
    assert.equal(sent.at(-1).message.type,'result');
    assert.equal(sent.at(-1).message.value.metrics.artworkWorker.counters.localHits,7);
    assert.equal(sent.at(-1).message.value.metrics.artworkWorker.capacity,1024);

    const handlers = {};
    const elements = {};
    function element(id) {
      return elements[id] = { id, disabled: false, textContent: '', attributes: {}, children: [], addEventListener(type, fn) { handlers[id + ':' + type] = fn; }, setAttribute(name, value) { this.attributes[name] = value; }, replaceChildren() { this.children = []; }, append(...nodes) { this.children.push(...nodes); }, click() { this.clicked = true; } };
    }
    for (const id of ['home', 'cold', 'clean', 'start', 'stop', 'headers', 'export', 'status', 'metrics', 'history', 'headers-result']) element(id);
    const outgoing = [];
    const childWindow = { postMessage(data, origin) { outgoing.push({ data, origin }); }, location: { pathname: '/vdr-suite/frontend/browser-performance-home.html' } };
    elements.home.contentWindow = childWindow;
    let navigations = 0;
    Object.defineProperty(elements.home, 'src', {
      get() { return this.currentSrc; },
      set(value) { this.currentSrc = value; navigations++; }
    });
    const document = { getElementById(id) { return elements[id]; }, createElement() { return { textContent: '', children: [], append(...nodes) { this.children.push(...nodes); }, click() { this.clicked = true; } }; } };
    const root = { document, location: { origin: 'https://example.test' }, addEventListener(type, fn) { handlers['root:' + type] = fn; }, URL: { createObjectURL() { return 'blob:test'; }, revokeObjectURL() {} }, Blob, setTimeout(fn) { fn(); }, Math, Date };
    run('browser-performance-diagnostics.js', { window: root, Blob, Math, Date, JSON, Object, String });
    assert.equal(elements.home.src, 'browser-performance-home.html');
    assert.equal(navigations, 1, 'ordinary diagnostic startup is a warm/unchanged-cache measurement');
    const token = elements.home.attributes['data-vdr-suite-diagnostics'];
    function incoming(type, value, override) {
      handlers['root:message']({ source: childWindow, origin: 'https://example.test', data: Object.assign({ channel: 'vdr-suite-browser-diagnostics', token, type, value }, override) });
    }
    incoming('started', 'initial-home');
    assert.equal(elements.stop.disabled, false);
    handlers['stop:click']();
    assert.equal(outgoing.at(-1).data.type, 'stop');
    incoming('result', { label: 'initial-home', metrics: { resourceRequests: 2 } });
    assert.equal(elements.start.disabled, false);
    assert.equal(elements.export.disabled, false);
    incoming('result', { label: 'forged', metrics: {} }, { token: 'wrong' });
    assert.ok(!elements.history.textContent.includes('forged'));
    handlers['headers:click']();
    assert.equal(outgoing.at(-1).data.type, 'headers');
    incoming('headers', { status: 200, headers: { 'cache-control': 'private' } });
    assert.equal(elements.headers.disabled, false);
    handlers['cold:click']();
    assert.equal(elements.home.src, 'browser-performance-home.html');
    assert.equal(navigations, 2, 'reload never clears artwork implicitly');
    assert.notEqual(elements.home.attributes['data-vdr-suite-diagnostics'], token);
    incoming('result', { label: 'stale', metrics: {} });
    assert.ok(!elements.history.textContent.includes('stale'));
    const warmToken = elements.home.attributes['data-vdr-suite-diagnostics'];
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:warmToken,type:'started',
        value:'initial-home'}});

    // Explicit PWA-artwork preparation must finish before the embedded
    // Home document starts. No browser HTTP cache or unrelated storage is
    // cleared, and the cache state must be exported on the capture.
    const workerMessages = [];
    root.navigator = {serviceWorker:{controller:{postMessage(data, ports) {
      workerMessages.push({data, port:ports[0]});
    }}}};
    root.MessageChannel = class {
      constructor() {
        this.port1 = {close() {this.closed=true;}};
        this.port2 = {send:value=>this.port1.onmessage({data:value})};
      }
    };
    root.setTimeout = setTimeout;
    root.clearTimeout = clearTimeout;
    handlers['clean:click']();
    assert.equal(elements.home.src, 'about:blank',
      'previous Home must stop requests while clearing artwork');
    assert.equal(navigations, 3);
    assert.equal(workerMessages.length, 1);
    assert.equal(workerMessages[0].data.type, 'vdr-suite-artwork-clear');
    assert.equal(elements.clean.disabled, true);
    workerMessages[0].port.send({schema:1,cleared:true});
    await new Promise(resolve=>setImmediate(resolve));
    assert.equal(navigations, 4, 'the next measured Home loads only after worker acknowledgment');
    assert.equal(elements.home.src, 'browser-performance-home.html');
    const cleanToken = elements.home.attributes['data-vdr-suite-diagnostics'];
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:cleanToken,type:'started',value:'initial-home'}});
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:cleanToken,type:'result',
        value:{label:'initial-home',metrics:{resourceRequests:2}}}});
    assert.ok(elements.history.textContent.includes('"pwaArtwork": "cleared"'));
    assert.ok(elements.history.textContent.includes('"httpCache": "not-cleared"'));
    let exported;
    root.URL.createObjectURL = blob => {exported=blob;return 'blob:test';};
    handlers['export:click']();
    const exportReport = JSON.parse(await exported.text());
    assert.equal(exportReport.captures.at(-1).cachePreparation.pwaArtwork,'cleared');
    assert.equal(exportReport.captures.at(-1).cachePreparation.httpCache,'not-cleared');

    // A missing Service Worker does not masquerade as a cold-cache run.
    root.navigator = {};
    handlers['clean:click']();
    await new Promise(resolve=>setImmediate(resolve));
    assert.equal(elements.home.src,'browser-performance-home.html');
    const unsupportedToken = elements.home.attributes['data-vdr-suite-diagnostics'];
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:unsupportedToken,type:'started',
        value:'initial-home'}});
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:unsupportedToken,type:'result',
        value:{label:'initial-home',metrics:{}}}});
    assert.ok(elements.history.textContent.includes('"pwaArtwork": "unavailable"'));

    // Explicit deletion failure and lack of acknowledgment must be visible
    // in the export; neither may be presented as a confirmed cold cache.
    root.navigator = {serviceWorker:{controller:{postMessage(data, ports) {
      workerMessages.push({data, port:ports[0]});
    }}}};
    handlers['clean:click']();
    workerMessages.at(-1).port.send({schema:1,cleared:false});
    await new Promise(resolve=>setImmediate(resolve));
    const failedToken = elements.home.attributes['data-vdr-suite-diagnostics'];
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:failedToken,type:'started',
        value:'initial-home'}});
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:failedToken,type:'result',
        value:{label:'initial-home',metrics:{}}}});
    assert.ok(elements.history.textContent.includes('"pwaArtwork": "failed"'));

    let timeoutCallback;
    root.setTimeout = callback => { timeoutCallback = callback; return 91; };
    root.clearTimeout = () => {};
    handlers['clean:click']();
    assert.equal(elements.home.src,'about:blank');
    assert.equal(typeof timeoutCallback,'function');
    timeoutCallback();
    await new Promise(resolve=>setImmediate(resolve));
    const timeoutToken = elements.home.attributes['data-vdr-suite-diagnostics'];
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:timeoutToken,type:'result',
        value:{label:'initial-home',metrics:{}}}});
    assert.ok(elements.history.textContent.includes('"pwaArtwork": "timeout"'));

    // Real SW lifecycle: this open diagnostic tab may still be controlled by
    // the previous worker (no clients.claim), even after the updated worker
    // has activated. Send the clear to registration.active, not controller.
    root.setTimeout = setTimeout;
    root.clearTimeout = clearTimeout;
    root.location.href = 'https://example.test/vdr-suite/frontend/browser-performance-diagnostics.html';
    const previousWorker = {postMessage() {throw Error('old controller should not be used');}};
    const registeredMessages = [];
    const updatedWorker = {postMessage(data, ports) {
      registeredMessages.push({data, port:ports[0]});
    }};
    let lookups = 0, updates = 0;
    const registration = {active:updatedWorker,update() {
      updates += 1;
      return Promise.resolve(this);
    }};
    root.navigator = {serviceWorker:{controller:previousWorker,
      getRegistration(url) {
        lookups += 1;
        assert.equal(url, root.location.href);
        return Promise.resolve(registration);
      }}};
    // The previous capture already finished; trigger an unrelated new
    // diagnostic preparation without a second page or manual cache clear.
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:timeoutToken,
        type:'started',value:'initial-home'}});
    const beforeActive = navigations;
    handlers['clean:click']();
    assert.equal(elements.home.src,'about:blank');
    await new Promise(resolve=>setImmediate(resolve));
    assert.equal(lookups,1);
    assert.equal(updates,1);
    assert.equal(registeredMessages.length,1);
    assert.equal(registeredMessages[0].data.type,'vdr-suite-artwork-clear');
    assert.equal(navigations,beforeActive+1,
      'Home must not start while current worker clears artwork');
    registeredMessages[0].port.send({schema:1,cleared:true});
    await new Promise(resolve=>setImmediate(resolve));
    assert.equal(navigations,beforeActive+2);
    const activeToken = elements.home.attributes['data-vdr-suite-diagnostics'];
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:activeToken,
        type:'started',value:'initial-home'}});
    handlers['root:message']({source:childWindow, origin:'https://example.test',
      data:{channel:'vdr-suite-browser-diagnostics',token:activeToken,
        type:'result',value:{label:'initial-home',metrics:{}}}});
    assert.ok(elements.history.textContent.includes('"pwaArtwork": "cleared"'));

    // If a new release is still installing, avoid sending to the outdated
    // active worker until the activation event arrives.
    let activationListener, activationRemovals = 0;
    const installing = {state:'installing',
      addEventListener(type,fn) {
        assert.equal(type,'statechange'); activationListener=fn;
      },
      removeEventListener(type,fn) {
        assert.equal(type,'statechange'); assert.equal(fn,activationListener);
        activationRemovals++;
      }};
    const queuedRegistration = {active:previousWorker, installing,
      update() {return Promise.resolve(this);}};
    root.navigator.serviceWorker.getRegistration = () => Promise.resolve(queuedRegistration);
    const beforeActivation = navigations;
    handlers['clean:click']();
    await new Promise(resolve=>setImmediate(resolve));
    assert.equal(navigations,beforeActivation+1);
    assert.equal(registeredMessages.length,1,
      'do not message the old active worker while the new one installs');
    queuedRegistration.active = updatedWorker;
    installing.state='activated';
    activationListener();
    assert.equal(registeredMessages.length,2);
    registeredMessages[1].port.send({schema:1,cleared:true});
    await new Promise(resolve=>setImmediate(resolve));
    assert.equal(navigations,beforeActivation+2);
    assert.equal(activationRemovals,1);

    let now = 0;
    const observers = [];
    class Observer {
      constructor(callback) { this.callback = callback; this.pending = []; observers.push(this); }
      observe(options) { this.options = options; }
      takeRecords() { const records = this.pending; this.pending = []; return records; }
      disconnect() { this.disconnected = true; }
    }
    class MutationObserverMock extends Observer {
      observe(target, options) {
        assert.equal(target, env.document.documentElement);
        assert.equal(arguments.length, 2);
        assert.equal(options.subtree, true);
        assert.equal(options.childList, true);
        this.options = options;
      }
    }
    const cover = { currentSrc: 'https://example.test/api/vdr/recordings/metadata/image?id=secret', src: '', complete: true, naturalWidth: 100, loading: 'lazy', getBoundingClientRect() { return {width:100,height:120,left:10,right:110,top:10,bottom:130}; } };
    const imageEvents = {};
    const env = { document: { documentElement: {}, addEventListener(type, fn) { imageEvents[type] = fn; }, removeEventListener(type, fn) { assert.equal(imageEvents[type], fn); delete imageEvents[type]; }, querySelectorAll(selector) { return selector === 'img' ? [cover] : selector === '.media-home-discovery-rail' ? [{ scrollLeft: 30 }] : selector.includes(' img') ? [cover] : []; } }, performance: { now() { return now; } }, PerformanceObserver: Observer, MutationObserver: MutationObserverMock, location: { href: 'https://example.test/frontend/', origin: 'https://example.test' }, URL, fetch: async () => { throw new Error('Unexpected network request'); } };
    env.innerWidth = 400;
    env.innerHeight = 300;
    const measurement = createProbe(env);
    measurement.start('initial-home', { buffered: true });
    assert.equal(observers[0].options.buffered, true);
    assert.equal(observers[1].options.buffered, true);
    assert.equal(observers[2].options.subtree, true);
    observers[0].pending.push({ entryType: 'resource', name: cover.currentSrc, initiatorType: 'img', transferSize: 120 });
    observers[1].pending.push({ entryType: 'longtask', duration: 70 });
    observers[2].pending.push({});
    now = 35;
    imageEvents.load({target:cover});
    now = 80;
    const result = measurement.stop();
    assert.equal(result.visibleArtwork.visibleAtStop, 1);
    assert.equal(result.visibleArtwork.loadedAtStop, 1);
    assert.equal(result.visibleArtwork.pendingAtStop, 0);
    assert.equal(result.visibleArtwork.firstLoadEventMs, 35);
    assert.equal(result.visibleArtwork.lastLoadEventMs, 35);
    assert.equal(result.visibleArtwork.observedLoadEvents, 1);
    assert.equal(result.initialViewportArtwork.initialVisible, 1);
    assert.equal(result.initialViewportArtwork.alreadyLoadedAtFirstObservation, 1);
    assert.equal(result.initialViewportArtwork.readyAtStop, 1);
    assert.equal(result.initialViewportArtwork.allReadyObservedMs, 0);
    assert.equal(imageEvents.load, undefined);

    // First-viewport cohort is fixed before scrolling. A later stop with
    // zero visible posters must not lose first-screen readiness timing.
    let cohortNow = 0, posterLeft = 150;
    const cohortFrames = [], cohortListeners = {}, cohortObservers = [];
    const railClip = {getBoundingClientRect() {
      return {left:100,right:420,top:0,bottom:350};
    }};
    const makeCover = (id, x) => ({
      currentSrc: 'https://example.test/api/vdr/recordings/metadata/image?id=' + id,
      complete:false, naturalWidth:0, loading:'lazy',
      closest() { return railClip; },
      getBoundingClientRect() {
        const left = id === 'main' ? posterLeft : x;
        return {left,right:left+120,top:40,bottom:200,width:120,height:160};
      }
    });
    const firstPoster = makeCover('main',150);
    const clippedPoster = makeCover('clipped',700);
    class CohortObserver {
      constructor(callback) {this.callback=callback;this.pending=[];cohortObservers.push(this);}
      observe() {}
      takeRecords() {return this.pending.splice(0);}
      disconnect() {}
    }
    const cohortEnv = {
      innerWidth:800, innerHeight:600,
      location:{href:'https://example.test/frontend/',origin:'https://example.test'},
      performance:{now(){return cohortNow;}},
      requestAnimationFrame(fn){cohortFrames.push(fn);},
      PerformanceObserver:CohortObserver, MutationObserver:CohortObserver,
      document:{
        documentElement:{},
        addEventListener(type,fn){cohortListeners[type]=fn;},
        removeEventListener(type){delete cohortListeners[type];},
        querySelectorAll(selector) {
          return selector === 'img' ? [firstPoster,clippedPoster] : [];
        }
      }
    };
    const cohortProbe = createProbe(cohortEnv);
    cohortProbe.start('first-viewport', {buffered:false});
    assert.equal(cohortFrames.length,1);
    cohortNow=120;cohortFrames.shift()();
    // A rail-clipped poster must not join the initial visible cohort.
    firstPoster.complete=true;firstPoster.naturalWidth=120;
    cohortNow=430;cohortListeners.load({target:firstPoster});
    cohortFrames.shift()();
    // The user scrolls away before stopping; first-screen timing persists.
    posterLeft=950;
    cohortNow=550;
    const cohortResult=cohortProbe.stop();
    assert.equal(cohortResult.visibleArtwork.visibleAtStop,0);
    assert.equal(cohortResult.initialViewportArtwork.firstVisibleAtMs,120);
    assert.equal(cohortResult.initialViewportArtwork.initialVisible,1);
    assert.equal(cohortResult.initialViewportArtwork.alreadyLoadedAtFirstObservation,0);
    assert.equal(cohortResult.initialViewportArtwork.readyAtStop,1);
    assert.equal(cohortResult.initialViewportArtwork.pendingAtStop,0);
    assert.equal(cohortResult.initialViewportArtwork.firstReadyObservedMs,430);
    assert.equal(cohortResult.initialViewportArtwork.allReadyObservedMs,430);
    assert(!JSON.stringify(cohortResult.initialViewportArtwork).includes('main'));
    assert(!JSON.stringify(cohortResult.initialViewportArtwork).includes('clipped'));
    assert.equal(result.metrics.resourceRequests, 1);
    assert.equal(result.metrics.longTasks, 1);
    assert.equal(result.metrics.mutations, 1);
    assert.equal(result.observerSupport.mutation, true);
    assert.equal(result.bufferedStartup, true);
    assert.equal(result.startupTiming.homeControlsReadyMs, null);
    const timeEntries = {
      'first-paint': 11, 'first-contentful-paint': 25,
      'vdr-suite:startup:session-ready': 50,
      'vdr-suite:startup:backend-selected': 70,
      'vdr-suite:startup:snapshot-requested': 71,
      'vdr-suite:startup:home-navigation-ready': 73,
      'vdr-suite:startup:snapshot-ready': 100,
      'vdr-suite:startup:home-controls-ready': 120,
      'vdr-suite:startup:home-frame': 145,
      'vdr-suite:startup:recording-count-settled': 500
    };
    const timing = startupTiming({getEntriesByName(name) {
      return Object.hasOwn(timeEntries, name) ? [{startTime: timeEntries[name]}] : [];
    }}, [
      {name:'https://example.test/vdr-suite/api/backends/private-backend/snapshot',
        startTime:72, requestStart:75, responseStart:92, responseEnd:97},
      {name:'https://other.test/api/backends/private-backend/snapshot',
        startTime:72, requestStart:75, responseStart:92, responseEnd:97}
    ], 'https://example.test/vdr-suite/frontend/');
    assert.equal(timing.homeNavigationReadyMs, 73);
    assert.deepStrictEqual(timing.snapshotResource, {
      dispatchToFetchMs:1, fetchToRequestMs:3, requestToFirstByteMs:17, bodyMs:5, responseToReadyMs:3
    });
    assert.ok(!JSON.stringify(timing).includes('private-backend'));
    assert.equal(timing.firstContentfulPaintMs, 25);
    assert.equal(timing.homeControlsReadyMs, 120);
    assert.equal(timing.homeFrameMs, 145);
    assert.equal(timing.recordingCountSettledMs, 500);
    assert.equal(timing.homeControlsAfterSessionMs, 70);
    assert.equal(result.metrics.resourceDiagnostics.observedEntries, 1);
    assert.ok(observers.every(observer => observer.disconnected));
    assert.ok(!JSON.stringify(result).includes('secret'));
    const requests = [];
    env.fetch = async function (url, options) { requests.push({ url, options }); return { status: 304, type: 'basic', headers: { get(name) { return name === 'etag' ? '"abc"' : null; } }, body: { cancel: async function () {} } }; };
    const headers = await measurement.inspectHeaders();
    assert.equal(requests.length, 1);
    assert.equal(requests[0].url, cover.currentSrc);
    assert.equal(requests[0].options.cache, 'no-cache');
    assert.equal(headers.status, 304);
    assert.ok(!JSON.stringify(headers).includes('secret'));
    assert.ok(!JSON.stringify(headers).includes('url:'));

    // The production probe classifies every observed resource, not only img
    // initiators or URLs still present in the DOM. No raw identifier escapes.
    const baseUrl = 'https://example.test/vdr-suite/frontend/';
    const sample = [
      { name: 'https://example.test/vdr-suite/api/vdr/recordings/metadata/image?id=private-recording', initiatorType: 'img', responseStatus: 404, transferSize: 350, encodedBodySize: 0 },
      { name: 'https://example.test/vdr-suite/api/vdr/recordings/metadata/image?id=another-recording', initiatorType: 'img', responseStatus: 404, transferSize: 250, encodedBodySize: 16384 },
      { name: 'https://example.test/vdr-suite/api/epg/cache/metadata/image?id=private-event', initiatorType: 'fetch', responseStatus: 200, transferSize: 300000, encodedBodySize: 262144, decodedBodySize: 262144 },
      { name: 'https://example.test/vdr-suite/channel-logos/private-channel.svg', initiatorType: 'img', responseStatus: 200, transferSize: 0, encodedBodySize: 100 },
      { name: 'https://example.test/vdr-suite/api/epg/cache/now-next?token=private-token', initiatorType: 'fetch', responseStatus: 204, transferSize: 100 },
      { name: 'https://example.test/vdr-suite/api/epg/cache/changes?token=private-token', initiatorType: 'fetch', responseStatus: 0, transferSize: 0 },
      { name: 'https://outside.test/private-path?token=private-token', initiatorType: 'script', responseStatus: 0, transferSize: 0 },
      { name: 'https://example.test/vdr-suite/recording-artwork/private-poster.webp', initiatorType: 'img', responseStatus: 200, transferSize: 2000000, encodedBodySize: 1048577, duration: 90, requestStart: 10, responseStart: 65, responseEnd: 90, serverTiming: [{name:'artwork-read',duration:17},{name:'artwork-queue',duration:51},{name:'artwork-preview',duration:23}] }
    ];
    const detail = summarizeResources(sample, baseUrl);
    assert.equal(detail.observedEntries, sample.length);
    assert.equal(detail.rows.reduce((n, row) => n + row.count, 0), sample.length);
    const missing = detail.rows.find(row => row.category === 'recording-metadata-image' && row.status === '404');
    assert.equal(missing.count, 2);
    assert.equal(missing.transferBytes, 600);
    assert.equal(missing.sizeDistribution.zero, 1);
    assert.equal(missing.sizeDistribution['1-16KiB'], 1);
    assert.equal(detail.rows.find(row => row.category === 'epg-cache-api' && row.status === '204').count, 1);
    assert.equal(detail.rows.find(row => row.category === 'epg-cache-api' && row.status === 'unavailable').count, 1);
    assert.equal(detail.rows.find(row => row.category === 'cross-origin').count, 1);
    assert.equal(detail.rows.find(row => row.category === 'recording-artwork').sizeDistribution['over-1MiB'], 1);
    const artworkTiming = detail.artworkVariantDetails.find(row => row.category === 'recording-artwork').timingMs;
    assert.deepEqual(artworkTiming.resourceDuration, {samples:1,total:90,max:90});
    assert.deepEqual(artworkTiming.requestToFirstByte, {samples:1,total:55,max:55});
    assert.deepEqual(artworkTiming.responseBody, {samples:1,total:25,max:25});
    assert.deepEqual(artworkTiming.serverArtworkRead, {samples:1,total:17,max:17});
    assert.deepEqual(artworkTiming.serverArtworkQueue, {samples:1,total:51,max:51});
    assert.deepEqual(artworkTiming.serverArtworkPreview, {samples:1,total:23,max:23});
    const serialized = JSON.stringify(detail);
    for (const secret of ['private-recording', 'another-recording', 'private-event', 'private-channel', 'private-token', 'private-path', 'private-poster', 'outside.test']) assert.ok(!serialized.includes(secret));
    assert.ok(!serialized.includes('https://'));
    assert.ok(!serialized.includes('?'));
    const legacy = require('../web/frontend/browser-artwork-probe').summarize(sample, [], 0, [], baseUrl);
    assert.equal(legacy.resourceRequests, 4);
    assert.equal(legacy.resourceDiagnostics.observedEntries, sample.length);
    assert.equal(legacy.resourceDiagnostics.rows.reduce((n, row) => n + row.count, 0), sample.length);

    // Execute the installed production composition's own probe, bridge and
    // controller. The existing stop/export path must carry the new data.
    const productionObservers = [];
    class ProductionObserver {
      constructor(callback) { this.callback = callback; this.pending = []; productionObservers.push(this); }
      observe() {}
      takeRecords() { const pending = this.pending; this.pending = []; return pending; }
      disconnect() { this.disconnected = true; }
    }
    const productionWindow = {
      document: { documentElement: {}, querySelectorAll() { return []; } },
      performance: { now() { return 10; } },
      PerformanceObserver: ProductionObserver,
      MutationObserver: ProductionObserver,
      location: { href: baseUrl, origin: 'https://example.test' },
      URL, fetch: async () => { throw new Error('Unexpected request'); }
    };
    run('browser-artwork-probe.js', { window: productionWindow, URL, Map, Set, Number, Array, Object, String, Math, JSON });
    productionWindow.VdrSuiteArtworkProbe.start('initial-home', { buffered: true });
    productionObservers[0].pending.push(...sample.map(entry => Object.assign({ entryType: 'resource' }, entry)));
    const productionResult = productionWindow.VdrSuiteArtworkProbe.stop();
    assert.equal(productionResult.metrics.resourceDiagnostics.observedEntries, sample.length);
    assert.ok(!JSON.stringify(productionResult).includes('private-recording'));
    assert.ok(productionObservers.every(observer => observer.disconnected));
    // PWA.15.11: a horizontally clipped image with an unassigned deferred
    // URL must count as not ready upon entry, then record its visible delay.
    // A completed prefetch already in view counts as ready, without IDs in JSON.
    let scrollNow = 0, scrollOffset = 0;
    const scrollFrames = [], scrollHandlers = {};
    const scrollRail = {scrollLeft:0,
      getBoundingClientRect(){return {left:100,right:500,top:0,bottom:350};}};
    const pendingScrollPoster = {
      src:'',__vdrSuiteDeferredArtworkUrl:'https://example.test/api/vdr/recordings/metadata/image?id=private-pending',
      complete:false,naturalWidth:0,loading:'lazy',closest(){return scrollRail;},
      getBoundingClientRect(){const left=620-scrollOffset;return {left,right:left+100,top:20,bottom:170,width:100,height:150};}};
    const preparedScrollPoster = {
      src:'https://example.test/api/vdr/recordings/metadata/image?id=private-ready',
      complete:true,naturalWidth:100,__vdrSuiteArtworkPrefetchStartedAt:20,
      __vdrSuiteArtworkPrefetchDirection:'forward',loading:'eager',closest(){return scrollRail;},
      getBoundingClientRect(){const left=650-scrollOffset;return {left,right:left+100,top:20,bottom:170,width:100,height:150};}};
    const scrollEnv = {
      innerWidth:800,innerHeight:600,URL,
      location:{href:'https://example.test/frontend/',origin:'https://example.test'},
      performance:{now(){return scrollNow;}},
      requestAnimationFrame(fn){scrollFrames.push(fn);},
      document:{documentElement:{},
        querySelectorAll(selector){
          if(selector==='img')return [pendingScrollPoster,preparedScrollPoster];
          if(selector.includes('.media-home-discovery-rail'))return [scrollRail];
          return [];
        },
        addEventListener(name,fn){scrollHandlers[name]=fn;},
        removeEventListener(name){delete scrollHandlers[name];}}
    };
    const scrollProbe = createProbe(scrollEnv);
    scrollProbe.start('scroll-prefetch');
    scrollFrames.shift()(); // both covers remain horizontally clipped
    scrollNow=50;scrollOffset=220;scrollRail.scrollLeft=220;
    scrollHandlers.scroll();scrollFrames.shift()();
    assert.equal(pendingScrollPoster.src,'');
    scrollNow=120;pendingScrollPoster.src=pendingScrollPoster.__vdrSuiteDeferredArtworkUrl;
    pendingScrollPoster.complete=true;pendingScrollPoster.naturalWidth=100;
    scrollHandlers.load({target:pendingScrollPoster});scrollFrames.shift()();
    const scrollReport = scrollProbe.stop();
    assert.equal(scrollReport.railArtworkPrefetch.enteredVisible,2);
    assert.equal(scrollReport.railArtworkPrefetch.readyBeforeEntry,1);
    assert.equal(scrollReport.railArtworkPrefetch.notReadyOnEntry,1);
    assert.equal(scrollReport.railArtworkPrefetch.becameReadyAfterEntry,1);
    assert.equal(scrollReport.railArtworkPrefetch.meanReadyDelayMs,70);
    assert.equal(scrollReport.railArtworkPrefetch.prefetchHitRatePercent,50);
    assert.equal(scrollReport.railArtworkPrefetch.rightScrollFrames,1);
    assert.equal(scrollReport.railArtworkPrefetch.forwardEntries,1);
    assert.ok(!JSON.stringify(scrollReport).includes('private-pending'));
    assert.ok(!JSON.stringify(scrollReport).includes('private-ready'));
    assert.equal(scrollHandlers.scroll,undefined);

    console.log('browser diagnostics composition, lifecycle, mutation observer, buffered observers, resource classification and privacy ok');
  } finally { fs.rmSync(tmp, { recursive: true, force: true }); }
}
main().catch(error => { console.error(error); process.exitCode = 1; });
