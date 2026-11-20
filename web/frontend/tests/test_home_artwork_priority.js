'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const frames = [];
const window = {innerWidth: 800, innerHeight: 600, requestAnimationFrame: cb => frames.push(cb)};
vm.runInNewContext(fs.readFileSync(path.join(__dirname, '..', 'home-recording-discovery.js'), 'utf8'), {window});
const prioritize = window.VdrSuiteHomeRecordingDiscovery.prioritizeVisibleArtwork;
const image = (left, top, connected = true) => ({isConnected:connected, complete:false, loading:'lazy', getBoundingClientRect:()=>({left,top,right:left+100,bottom:top+150,width:100,height:150})});
const visible = image(15,20), offscreen = image(820,20), detached = image(15,20,false);
const extras = Array.from({length:15}, () => image(15,20));
for (const i of [visible,offscreen,detached,...extras]) prioritize(i);
assert.equal(frames.length,1);
frames.shift()();
assert.equal(visible.fetchPriority,'high');
assert.equal(visible.loading,'eager');
assert.equal(offscreen.loading,'lazy');
assert.equal(detached.loading,'lazy');
assert.equal(extras.filter(i=>i.fetchPriority==='high').length,11);
console.log('PWA.3 visible-first artwork contract PASS');

const prioritizeRail = window.VdrSuiteHomeRecordingDiscovery.prioritizeInlineRailArtwork;
const visibleRailImage = image(120, 40);
const next = [image(510,40), image(620,40), image(730,40), image(840,40), image(900,40)];
const far = image(1300,40);
const hidden = image(200,610);
const rail = {isConnected:true, getBoundingClientRect:()=>({left:100,top:0,right:500,bottom:300}),
  querySelectorAll:()=>[visibleRailImage,...next,far,hidden]};
prioritizeRail(rail); prioritizeRail(rail);
assert.equal(frames.length,1);
frames.shift()();
assert.equal(visibleRailImage.fetchPriority,'high');
assert.equal(next.filter(i=>i.fetchPriority==='low').length,0,
  'PWA.15.10: do not start speculative rail loads while a visible cover is pending');
assert(next.every(i=>i.loading==='lazy'),
  'clipped rail cards remain native lazy until current viewport settles');
visibleRailImage.complete=true;
prioritizeRail(rail); frames.shift()();
assert.equal(next.filter(i=>i.fetchPriority==='low').length,4,
  'adjacent warmup still runs on later rail demand after visible cover completes');
assert.equal(next[4].loading,'lazy');
assert.equal(far.loading,'lazy');
assert.equal(hidden.loading,'lazy');
// A formerly speculative image becomes visible on scrolling: promote it.
next[0].getBoundingClientRect=()=>({left:120,top:20,right:220,bottom:170,width:100,height:150});
window.navigator={connection:{saveData:true}};
prioritizeRail(rail); frames.shift()();
assert.equal(next[0].fetchPriority,'high');
assert.equal(next[4].loading,'lazy');
const removed = image(120,20);
prioritizeRail({isConnected:false,querySelectorAll:()=>[removed]}); frames.shift()();
assert.equal(removed.loading,'lazy');
console.log('PWA.14 inline rail visible and bounded lookahead artwork contract PASS');


// Cold Home: do not start image network traffic for horizontally or vertically
// clipped cards. The real scroll ancestor intersection fires on demand.
const deferred = [];
let observerInstance;
class FakeArtworkIntersectionObserver {
  constructor(callback, options) {
    this.callback = callback; this.options = options; this.targets = new Set();
    observerInstance = this; deferred.push(this);
  }
  observe(target) { this.targets.add(target); }
  unobserve(target) { this.targets.delete(target); }
  notify(target, isIntersecting) {
    this.callback([{target, isIntersecting}]);
  }
}
const deferredWindow = {IntersectionObserver: FakeArtworkIntersectionObserver};
vm.runInNewContext(fs.readFileSync(path.join(__dirname, '..', 'home-recording-discovery.js'), 'utf8'), {window:deferredWindow});
const assignArtwork = deferredWindow.VdrSuiteHomeRecordingDiscovery.deferHomeArtworkImage;
const deferredFirst = {src:'',loading:'lazy',getBoundingClientRect(){ return {left:0,right:100,top:0,bottom:100}; }};
const deferredSecond = {src:'',loading:'lazy',getBoundingClientRect(){ return {left:100,right:200,top:0,bottom:100}; }};
assignArtwork(deferredFirst, '/api/cover-one?variant=home');
assignArtwork(deferredSecond, '/api/cover-two?variant=home');
assert.equal(deferred.length, 1, 'one shared IntersectionObserver per Home document');
assert.equal(observerInstance.options.rootMargin, '120px 0px');
assert.equal(deferredFirst.src, '', 'offscreen posters have no request URL yet');
assert.equal(deferredSecond.src, '', 'offscreen posters stay unrequested');
observerInstance.notify(deferredFirst, false);
assert.equal(deferredFirst.src, '', 'nonintersecting cards stay deferred');
observerInstance.notify(deferredFirst, true);
assert.equal(deferredFirst.src, '/api/cover-one?variant=home');
assert.equal(deferredSecond.src, '');
assert.equal(observerInstance.targets.has(deferredFirst), false);
observerInstance.notify(deferredSecond, true);
assert.equal(deferredSecond.src, '/api/cover-two?variant=home');
assert.equal(observerInstance.targets.size, 0);
const withoutGeometry = {src:''};
assignArtwork(withoutGeometry, '/cover-no-geometry');
assert.equal(withoutGeometry.src, '/cover-no-geometry',
  'legacy/render-test image nodes without geometry use the ordinary image path');
const withoutObserver={};
vm.runInNewContext(fs.readFileSync(path.join(__dirname, '..', 'home-recording-discovery.js'), 'utf8'), {window:withoutObserver});
const fallbackImage={src:''};
withoutObserver.VdrSuiteHomeRecordingDiscovery.deferHomeArtworkImage(fallbackImage, '/cover-fallback');
assert.equal(fallbackImage.src, '/cover-fallback', 'unsupported browsers retain the ordinary native artwork path');
const sourceFiles=['home-recording-discovery.js','home-recording-discovery-bootstrap.js','home-recently-watched.js'];
for(const sourceFile of sourceFiles){
  const code = fs.readFileSync(path.join(__dirname, '..', sourceFile), 'utf8');
  assert(code.includes('deferHomeArtworkImage('), sourceFile+' must defer its canonical Home artwork requests');
}
console.log('PWA.15 cold Home artwork demand-first contract PASS');
// PWA.15.8: priority is fixed at the moment src starts the native request.
// The IO lookahead can report near images before visible ones; clipped
// images must not compete at high priority with the first visible posters.
const started = [];
let demandObserver;
class DemandObserver {
  constructor(callback, options) { this.callback = callback; this.options = options; }
  observe() {}
  unobserve() {}
  notify(entries) { this.callback(entries); }
}
const priorityWindow = {
  innerWidth: 800, innerHeight: 600,
  requestAnimationFrame() {},
  IntersectionObserver: class extends DemandObserver {
    constructor(callback, options) {
      super(callback, options); demandObserver = this;
    }
  }
};
vm.runInNewContext(fs.readFileSync(path.join(__dirname, '..', 'home-recording-discovery.js'), 'utf8'),
  {window: priorityWindow});
const requestPoster = priorityWindow.VdrSuiteHomeRecordingDiscovery.deferHomeArtworkImage;
const clippedRail = {
  getBoundingClientRect() { return {left:100,top:0,right:500,bottom:400}; }
};
function pendingPoster(id, left, top, rail) {
  const node = {
    isConnected: true, complete: false, loading:'lazy',
    closest() { return rail || null; },
    getBoundingClientRect() {
      return {left,top,right:left+100,bottom:top+150,width:100,height:150};
    }
  };
  let src = '';
  Object.defineProperty(node, 'src', {
    get() { return src; },
    set(value) {
      src = value;
      started.push({id,priority:this.fetchPriority,loading:this.loading});
    }
  });
  requestPoster(node, '/art/' + id + '?variant=home');
  return node;
}
const nearFirst = pendingPoster('near', 515, 10, clippedRail);
const firstVisible = pendingPoster('visible', 150, 10, clippedRail);
const belowViewport = pendingPoster('below', 180, 625, null);
const notIntersecting = pendingPoster('offscreen', 1200, 10, clippedRail);
assert.equal(started.length, 0, 'no speculative artwork network requests before IO demand');
demandObserver.notify([
  {target:nearFirst,isIntersecting:true},
  {target:belowViewport,isIntersecting:true},
  {target:notIntersecting,isIntersecting:false},
  {target:firstVisible,isIntersecting:true}
]);
assert.deepEqual(started.map(entry => entry.id), ['visible','near','below'],
  'real viewport request must be initiated before horizontal and vertical lookahead');
assert.deepEqual(started.map(entry => entry.priority), ['high','low','low'],
  'clipped and below-viewport artwork must not use high request priority');
assert.equal(started[0].loading, 'eager',
  'first visible img must be eager before the browser sees its src');
assert.equal(started[1].loading, 'lazy', 'lookahead retains normal lazy loading');
assert.equal(notIntersecting.src, '', 'nonintersecting artwork remains unrequested');
assert.equal(firstVisible.src, '/art/visible?variant=home');
assert.equal(demandObserver.options.rootMargin, '120px 0px');
console.log('PWA.15.8 cold Home request-time artwork priority contract PASS');

