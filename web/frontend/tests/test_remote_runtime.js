'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const clientSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'live-remote-client-api.js'),
  'utf8'
);
const source = fs.readFileSync(
  path.join(__dirname, '..', 'modules', 'remote.js'),
  'utf8'
);
const appSource = fs.readFileSync(path.join(__dirname, '..', 'app.js'), 'utf8');
const deSource = fs.readFileSync(path.join(__dirname, '..', 'locales', 'de.js'), 'utf8');
const enSource = fs.readFileSync(path.join(__dirname, '..', 'locales', 'en.js'), 'utf8');
const deferredSource = fs.readFileSync(
  path.join(__dirname, '..', 'platform', 'deferred-runtime-loader.js'),
  'utf8'
);

const remotePng = fs.readFileSync(
  path.join(__dirname, '..', 'assets', 'vdr-remote-photorealistic.png')
);
assert.deepStrictEqual(
  Array.from(remotePng.subarray(0, 8)),
  [137, 80, 78, 71, 13, 10, 26, 10]
);
assert.strictEqual(remotePng.readUInt32BE(16), 360);
assert.strictEqual(remotePng.readUInt32BE(20), 1220);
assert.strictEqual(remotePng[24], 8);
assert.strictEqual(remotePng[25], 6);

assert(!source.includes('/remote/seq'));
assert(!source.includes('/remote/kbd'));
assert(!source.includes('/osd'));
assert(!source.includes('restfulapi'));
assert(!source.includes('/api/vdr/remote/actions'));
assert(!source.includes('/api/vdr/live/overlay'));
assert(clientSource.includes("'/api/vdr/remote/actions'"));
assert(clientSource.includes("'/api/vdr/live/overlay'"));
assert(clientSource.includes("'/api/vdr/live'"));
assert(clientSource.includes('if (!sessionApi.isAuthenticated())'));
assert(deferredSource.includes("String(payload.error.code || '') !== 'role_read_only'"));
assert(deferredSource.includes('Dieses Konto hat für dieses Backend nur Lesezugriff.'));
assert(deferredSource.includes('This account has read-only access to this backend.'));
assert(deferredSource.includes('installSecurityRoleErrorMessages();'));

assert(source.includes("P='/channel-logos/vdr-suite-brand/vdr-remote-photorealistic.png'"));
assert(source.includes("el('section','rst')"));
assert(source.includes("el('img','rpi')"));
assert(source.includes("el('button','rpk')"));
assert(source.includes("s.setAttribute('aria-label','Fotorealistische VDR-Fernbedienung')"));
assert(source.includes("z.textContent='Fotorealistisch · scrollbar'"));
assert(source.includes("let d=null,hd=null,n=0,e=null,q=0,busy=false,helpZoom=100,returnToRemote=false,rootOverflow=''"));
assert(source.includes("x.classList.add('down')"));
assert(source.includes("x.classList.remove('down')"));
assert(source.includes("x.classList.add('send')"));
assert(source.includes("x.classList.remove('send')"));
assert(source.includes("x.getAttribute('aria-disabled')==='true'||busy"));
assert(source.includes('function signedIn()'));
assert(source.includes('function signInMessage()'));
assert(source.includes('function refreshControl()'));
assert(source.includes("if(!signedIn())return Promise.reject(Error(signInMessage()))"));
assert(source.includes("if(!signedIn()){enabled(false);status(signInMessage(),'warning');release(x);return}"));
assert(source.includes("if(signedIn()){subscribe();refresh()}else"));
assert(source.includes("auth.subscribe(v=>{if(!d||!d.open)return;refreshControl()"));
assert(!source.includes('lockControls(true)'));
assert(!source.includes('button.disabled = locked'));
assert(source.includes('.rpk.down::before'));
assert(source.includes('.rpk:not(.off):active::before'));
assert(source.includes('transform:translateY(2px) scale(.985)'));
assert(source.includes('width:min(19rem,72vw)'));
assert(source.includes('overflow-y:auto'));
assert(source.includes('position:sticky'));
assert(!source.includes('.rpk:hover'));
assert(!source.includes('.brand-feature-remote:hover'));
assert(source.includes('hotspotCount:H.length'));
assert(source.includes("x.dataset.nav='overview'"));
assert(source.includes('VDR-SUITE-Übersicht öffnen'));
assert(source.includes('[data-action],[data-nav]'));
assert(source.includes('module-tab[data-module="overview"]'));
assert(source.includes('function preloadRemoteImage()'));
assert(source.includes("l.rel='preload'"));
assert(source.includes("l.as='image'"));
assert(source.includes('l.href=P'));
assert(source.includes('preloadRemoteImage();const auth=session();'));
assert(source.includes("i.fetchPriority='high'"));
assert(source.includes("i.decoding='async'"));
assert(source.includes('function epgTimelineKey()'));
assert(source.includes("x.dataset.nav='epg'"));
assert(source.includes('EPG-Zeitleiste öffnen'));
assert(source.includes('s.append(i,overviewKey(),epgTimelineKey())'));
assert(source.includes('module-tab[data-module="epg"]'));

assert(source.includes("x.classList.add('brand-feature-remote')"));
assert(source.includes("x.setAttribute('aria-label','VDR - Fernbedienung öffnen')"));
assert(source.includes("el('h2','','VDR - Fernbedienung')"));
assert(source.includes("v.setAttribute('aria-label','VDR - Fernbedienung')"));
assert(source.includes("t.textContent='VDR - Fernbedienung'"));

assert(source.includes('function recordAction(x)'));
assert(source.includes('fetchClientTimers'));
assert(source.includes('fetchClientTimerDeleteAction'));
assert(source.includes('Mehrere laufende Aufnahmen auf diesem Kanal gefunden.'));
assert(source.includes('Aufnahme gestoppt und Timer gelöscht.'));
assert(source.includes("x.dataset.action==='record'"));
assert(!source.includes('/api/vdr/timers/actions/delete'));

assert(source.includes('function buildHelp()'));
assert(source.includes('function openHelp(fromRemote)'));
assert(source.includes("b=el('button','rhc','?')"));
assert(source.includes('b.onclick=()=>openHelp(true)'));
assert(source.includes('h.append(t,b,c)'));
assert(source.includes('UNASSIGNED_HELP_KEYS'));
assert(source.includes("document.documentElement.style.overflow='hidden'"));
assert(source.includes('data-help-image'));
assert(source.includes('Aktuell belegte Tasten'));
assert(source.includes('Noch nicht belegte sichtbare Tasten'));
assert(appSource.includes('settings-remote-card'));
assert(appSource.includes('settings.remoteHelpOpen'));
assert(appSource.includes('remote.openHelp(false)'));
assert(deSource.includes('"settings.remoteControl": "Fernbedienung"'));
assert(enSource.includes('"settings.remoteControl": "Remote control"'));

const serverPaths = fs.readFileSync(
  path.join(__dirname, '..', '..', '..', 'core', 'http', 'src', 'TestHttpServerPaths.inc'),
  'utf8'
);
const liveRemoteMake = fs.readFileSync(
  path.join(__dirname, '..', '..', '..', 'mk', 'live-remote.mk'),
  'utf8'
);
assert(serverPaths.includes('{"/frontend/app.js", "app.js", "application/javascript; charset=utf-8", "modules/remote.js"}'));
assert(serverPaths.includes('{"/frontend/api/client-api.js", "api/client-api.js", "application/javascript; charset=utf-8", "api/live-remote-client-api.js"}'));
assert(liveRemoteMake.includes('web/frontend/modules/remote.js'));
assert(liveRemoteMake.includes('web/frontend/api/live-remote-client-api.js'));
assert(liveRemoteMake.includes('web/frontend/assets/vdr-remote-photorealistic.png'));
assert(liveRemoteMake.includes('$(RM) $(DESTDIR)$(CACHEDIR)/channel-logos/vdr-suite-brand/vdr-remote-photorealistic.svg'));
assert(!source.includes('vdr-remote-photorealistic.svg'));
assert(source.includes('["volumeUp",68.889,36.311,16.111,4.754,"50%"]'));
assert(source.includes('["volumeDown",15.278,36.311,15.833,4.754,"50%"]'));
assert(source.includes('["menu",43.611,36.311,12.778,4.754,"50%"]'));
assert(source.includes('["channelUp",67.5,69.59,18.333,5.738,"34%"]'));
assert(source.includes('["channelDown",67.5,75.246,18.333,5.574,"34%"]'));

function response(status, payload) {
  const text = payload === null || payload === undefined
    ? ''
    : JSON.stringify(payload);
  function build() {
    return {
      status,
      ok: status >= 200 && status < 300,
      text() { return Promise.resolve(text); },
      clone() { return build(); }
    };
  }
  return build();
}

const document = {
  head: {appendChild() {}},
  body: {append() {}},
  createElement(tag) {
    return {
      tagName: tag.toUpperCase(),
      dataset: {},
      style: {},
      classList: {add() {}, remove() {}, toggle() {}},
      setAttribute() {},
      getAttribute() { return null; },
      removeAttribute() {},
      appendChild() {},
      append() {},
      addEventListener() {},
      querySelector() { return null; },
      querySelectorAll() { return []; }
    };
  },
  querySelector() { return null; },
  querySelectorAll() { return []; },
  getElementById() { return null; },
  addEventListener() {}
};

const requests = [];
const baseApi = Object.freeze({
  requestJson(url, options) {
    requests.push({url, options});
    return Promise.resolve({success: true});
  },
  fetchClientBackends() {
    return Promise.resolve({backends: []});
  }
});
const context = {
  window: null,
  document,
  console,
  Date,
  JSON,
  Object,
  Promise,
  Set,
  URLSearchParams,
  setTimeout,
  clearTimeout,
  fetch() {
    return Promise.resolve(response(401, {
      error: {
        code: 'authentication_required',
        message: 'Authentication is required'
      }
    }));
  },
  VdrSuiteClientApi: baseApi
};
context.window = context;
vm.createContext(context);
vm.runInContext(clientSource, context);
vm.runInContext(source, context);

assert(context.VdrSuiteRemote);
const runningTimers = Array.from(context.VdrSuiteRemote.runningTimersForChannel({timers: [
  {id: '17', channelId: 'C-1', title: 'Direktaufnahme', flags: 9},
  {id: '18', channelId: 'C-2', title: 'Andere Aufnahme', recording: true},
  {id: '19', channelId: 'C-1', title: 'Nicht aktiv', flags: 1}
]}, 'C-1'));
assert.strictEqual(runningTimers.length, 1);
assert.strictEqual(runningTimers[0].timerId, '17');
assert.strictEqual(runningTimers[0].title, 'Direktaufnahme');
assert.strictEqual(
  context.VdrSuiteRemote.remoteImagePath,
  '/channel-logos/vdr-suite-brand/vdr-remote-photorealistic.png'
);
assert.strictEqual(context.VdrSuiteRemote.hotspotCount, 36);
assert.strictEqual(context.VdrSuiteRemote.helpKeyCount, 55);
assert.strictEqual(typeof context.VdrSuiteRemote.openHelp, 'function');
assert.deepStrictEqual(Array.from(context.VdrSuiteRemote.actions), [
  'up', 'down', 'left', 'right', 'ok', 'back', 'menu', 'info',
  'red', 'green', 'yellow', 'blue', 'zero', 'one', 'two', 'three',
  'four', 'five', 'six', 'seven', 'eight', 'nine', 'channelUp',
  'channelDown', 'volumeUp', 'volumeDown', 'mute', 'play', 'pause',
  'stop', 'record', 'fastForward', 'rewind', 'next', 'previous',
  'switchChannel'
]);
assert.strictEqual(typeof context.VdrSuiteClientApi.fetchClientRemoteAction, 'function');
assert.strictEqual(typeof context.VdrSuiteClientApi.fetchClientLiveOverlay, 'function');
assert.strictEqual(
  context.VdrSuiteRemote.canControlBackend({
    enabled: true,
    online: true,
    frontendSelector: {canWrite: false},
    capabilities: {remoteControl: true}
  }),
  false
);
assert.strictEqual(
  context.VdrSuiteRemote.canControlBackend({
    enabled: true,
    online: true,
    frontendSelector: {canWrite: true},
    capabilities: {remoteControl: true}
  }),
  true
);

context.VdrSuiteClientApi.fetchClientLiveOverlay({backendId: 'default'});
assert.strictEqual(requests.length, 1);
assert.strictEqual(requests[0].url, '/api/vdr/live/overlay');

console.log('remote frontend contract ok');
async function exerciseRemoteOverlay(clientSource, source, assert, load, log) {
 const sockets=[], reads=[]; let dialog, selected='default', token='a'.repeat(40);
 function node(tag) {
  const listeners={};
  const x={tagName:tag.toUpperCase(),dataset:{},style:{},attrs:{},children:[],
   classList:{add(){},remove(){},toggle(){}},
   setAttribute(k,v){this.attrs[k]=v},getAttribute(k){return this.attrs[k]||null},removeAttribute(k){delete this.attrs[k]},
   append(...children){this.children.push(...children)},appendChild(child){this.append(child)},replaceChildren(...children){this.children=children},
   addEventListener(k,fn){(listeners[k]||=[]).push(fn)},
   querySelector(s){return this.querySelectorAll(s)[0]||null},
   querySelectorAll(s){const out=[];for(const c of this.children){if(s==='[data-overlay]'&&c.dataset.overlay||s==='[data-status]'&&c.dataset.status||s==='[data-action]'&&c.dataset.action)out.push(c);out.push(...c.querySelectorAll(s))}return out},
   showModal(){this.open=true},close(){this.open=false;(listeners.close||[]).forEach(fn=>fn())}
  };
  if(tag==='dialog')dialog=x;
  return x;
 }
 const document={head:node('head'),body:node('body'),documentElement:{lang:'de',style:{}},createElement:node,querySelector(){return null},querySelectorAll(){return[]},getElementById(){return null},addEventListener(){}};
 class EventSource {
  constructor(){this.listeners={};sockets.push(this)}
  addEventListener(k,fn){(this.listeners[k]||=[]).push(fn)}
  emit(k,data){const event={data:JSON.stringify(data||{})};(this.listeners[k]||[]).forEach(fn=>fn(event));if(k==='open'&&this.onopen)this.onopen(event);if(k==='error'&&this.onerror)this.onerror(event)}
  close(){this.closed=true}
 }
 const window={document,EventSource,console:log,setTimeout(){return 1},clearTimeout(){},fetch:async()=>({ok:true,status:200,text:async()=>JSON.stringify({csrfToken:token,expiresAt:'2099-01-01T00:00:00Z'})}),
  VdrSuiteClientApi:{requestJson(url,opts){assert.strictEqual(url,'/api/vdr/live/overlay');return new Promise((resolve,reject)=>reads.push({backend:opts.query.backend,resolve,reject}))},
   fetchClientBackends(){return Promise.resolve({backends:[{backendId:selected,canRemoteControl:true}]})}},
  VdrSuitePlatform:{getSelectedBackendId(){return selected},getClientApi(){return window.VdrSuiteClientApi}}
 };
 window.window=window;load(clientSource,window);load(source,window);
 const flush=async()=>{for(let i=0;i<12;i++)await Promise.resolve()};
 const payload=name=>({channel:{available:true,id:name,number:1,name},present:{available:true,title:'Programm'}});
 const overlay=()=>dialog.querySelector('[data-overlay]').children.map(x=>x.textContent).join(' | ');
 await window.VdrSuiteBrowserSession.restore();
 window.VdrSuiteRemote.open();await flush();
 assert.strictEqual(reads.length,1);assert.strictEqual(reads[0].backend,'default');
 reads[0].resolve({channel:{available:false},message:'RESTfulAPI info response contains no live channel'});await flush();
 assert(overlay().includes('RESTfulAPI info response contains no live channel'));
 const s=sockets[0];s.emit('open');assert.strictEqual(reads.length,1);
 s.emit('update',{sequenceNumber:1,backendId:'other',changedDomains:['liveOverlay']});
 s.emit('update',{sequenceNumber:2,backendId:'default',changedDomains:['recordings']});assert.strictEqual(reads.length,1);
 s.emit('update',{sequenceNumber:3,backendId:'default',changedDomains:['liveOverlay']});
 s.emit('update',{sequenceNumber:3,backendId:'default',changedDomains:['liveOverlay']});assert.strictEqual(reads.length,2);
 reads[1].resolve(payload('ZDF HD'));await flush();assert(overlay().includes('ZDF HD'));
 s.emit('error');s.emit('open');assert.strictEqual(reads.length,3);
 reads[2].reject(new Error('HTTP 503'));await flush();assert(overlay().includes('ZDF HD'));assert(overlay().includes('HTTP 503'));
 s.emit('resync');assert.strictEqual(reads.length,4);
 s.emit('update',{sequenceNumber:1,backendId:'default',changedDomains:['liveOverlay']});assert.strictEqual(reads.length,5);
 reads[4].resolve(payload('Das Erste HD'));reads[3].resolve(payload('veraltet'));await flush();assert(overlay().includes('Das Erste HD'));assert(!overlay().includes('veraltet'));
 selected='secondary';s.emit('update',{sequenceNumber:2,backendId:'secondary',changedDomains:['liveOverlay']});
 assert.strictEqual(reads[5].backend,'secondary');assert(!overlay().includes('Das Erste HD'));
 reads[5].resolve(payload('WDR'));await flush();assert(overlay().includes('WDR'));
 s.emit('update',{sequenceNumber:3,backendId:'secondary',changedDomains:['liveOverlay']});
 window.VdrSuiteBrowserSession.clear('session_revoked');reads[6].resolve(payload('Geheim'));await flush();assert(!overlay().includes('Geheim'));assert(!overlay().includes('WDR'));
 await window.VdrSuiteBrowserSession.restore();await flush();assert.strictEqual(reads.length,8);
 dialog.close();reads[7].resolve(payload('geschlossen'));await flush();assert(!overlay().includes('geschlossen'));assert(sockets[sockets.length-1].closed);
 window.VdrSuiteRemote.open();await flush();assert.strictEqual(reads.length,9);
 reads[8].resolve(payload('Arte'));await flush();assert(overlay().includes('Arte'));dialog.close();
}

exerciseRemoteOverlay(clientSource, source, assert, (code, window) => vm.runInNewContext(code, {window, document:window.document, console, Date, Promise, JSON, Object, Set, URLSearchParams, setTimeout, clearTimeout}),console).then(()=>console.log('remote overlay SSE lifecycle passed')).catch(error=>{console.error(error);process.exitCode=1});
