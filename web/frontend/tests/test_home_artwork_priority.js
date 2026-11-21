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
const deferredWindow = {innerWidth:800,innerHeight:600,IntersectionObserver: FakeArtworkIntersectionObserver};
vm.runInNewContext(fs.readFileSync(path.join(__dirname, '..', 'home-recording-discovery.js'), 'utf8'), {window:deferredWindow});
const assignArtwork = deferredWindow.VdrSuiteHomeRecordingDiscovery.deferHomeArtworkImage;
const deferredFirst = {isConnected:true,src:'',loading:'lazy',getBoundingClientRect(){ return {left:0,right:100,top:0,bottom:100}; }};
const deferredSecond = {isConnected:true,src:'',loading:'lazy',getBoundingClientRect(){ return {left:100,right:200,top:0,bottom:100}; }};
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
const priorityFrames = [];
let demandObserver;
let actualVisibleObserver;
class DemandObserver {
  constructor(callback, options) { this.callback = callback; this.options = options; }
  observe() {}
  unobserve() {}
  notify(entries) { this.callback(entries); }
}
const priorityWindow = {
  innerWidth: 800, innerHeight: 600,
  requestAnimationFrame(fn) {priorityFrames.push(fn);},
  IntersectionObserver: class extends DemandObserver {
    constructor(callback, options) {
      super(callback, options);
      if (options.rootMargin === '120px 0px') demandObserver = this;
      else if (options.rootMargin === '0px') actualVisibleObserver = this;
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
  const callbacks = {};
  const node = {
    isConnected: true, complete: false, loading:'lazy',
    addEventListener(type, callback) {callbacks[type] = callback;},
    finish() {this.complete = true; if (callbacks.load) callbacks.load();},
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
assert.deepEqual(started.map(entry => entry.id), ['visible'],
  'cold Home must start only the actual visible poster until it finishes');
assert.equal(started[0].priority, 'high',
  'the actual visible poster must start with high priority');
assert.equal(started[0].loading, 'eager',
  'first visible img must be eager before the browser sees its src');
assert.equal(nearFirst.src, '');
assert.equal(belowViewport.src, '',
  'IO margin must not bypass the bounded speculative queue');
while (priorityFrames.length) priorityFrames.shift()();
assert.equal(started.length, 1,
  'an unfinished visible cover prevents new speculative starts on cold Home');
firstVisible.finish();
while (priorityFrames.length) priorityFrames.shift()();
assert.deepEqual(started.map(entry => entry.id), ['visible','near','below'],
  'after visible completion the shared coordinator starts adjacent covers');
assert.deepEqual(started.map(entry => entry.priority), ['high','low','low'],
  'nearby posters remain low priority even after queued warmup');
assert(started.slice(1).every(entry => entry.loading === 'eager'),
  'bounded coordinator starts deferred nearby posters only when allowed');
assert.equal(notIntersecting.src, '', 'nonintersecting artwork remains unrequested');
assert.equal(firstVisible.src, '/art/visible?variant=home');
assert.equal(demandObserver.options.rootMargin, '120px 0px');
console.log('PWA.15.8 cold Home request-time artwork priority contract PASS');

// PWA.15.18: twenty near-rail and below-viewport IO reports must not
// start twenty simultaneous HTTP requests; visible demand stays immediate.
nearFirst.finish();
belowViewport.finish();
while (priorityFrames.length) priorityFrames.shift()();
const burst = Array.from({length:20}, (_, index) =>
  pendingPoster('cold-near-'+index, 515, 10, clippedRail));
demandObserver.notify(burst.map(target => ({target, isIntersecting:true})));
assert.equal(started.filter(entry => entry.id.startsWith('cold-near-')).length, 0,
  'IO must defer nearby requests to shared scheduling rather than start at notification');
while (priorityFrames.length) priorityFrames.shift()();
assert.equal(started.filter(entry => entry.id.startsWith('cold-near-')).length, 4,
  'no more than four IO speculative starts per animation frame');
priorityWindow.VdrSuiteHomeRecordingDiscovery.prioritizeVisibleArtwork(firstVisible);
while (priorityFrames.length) priorityFrames.shift()();
assert.equal(started.filter(entry => entry.id.startsWith('cold-near-')).length, 6,
  'six total pending IO speculative loads are the same global rail budget');
// An earlier root-margin event will not fire again merely because an image
// crosses the actual viewport edge. A separate zero-margin event must
// promote a previously deferred request even with no spare warmup slot.
assert(actualVisibleObserver && actualVisibleObserver.options.rootMargin === '0px',
  'cold lookahead tracks later viewport entry without polling');
const promotedNear = burst[8];
promotedNear.getBoundingClientRect = () =>
  ({left:200,right:300,top:10,bottom:160,width:100,height:150});
actualVisibleObserver.notify([{target:promotedNear,isIntersecting:true}]);
assert.equal(started.at(-1).id, 'cold-near-8',
  'pending near-rail poster starts on actual visibility without new scroll events');
assert.equal(started.at(-1).priority, 'high',
  'actual demand keeps high HTTP priority even when speculative slots are full');
const newlyVisible = pendingPoster('cold-visible', 180, 10, clippedRail);
demandObserver.notify([{target:newlyVisible, isIntersecting:true}]);
assert.equal(started.at(-1).id, 'cold-visible',
  'a newly visible cold poster must start immediately, even with all speculative slots full');
assert.equal(started.at(-1).priority, 'high');
burst[0].finish();
while (priorityFrames.length) priorityFrames.shift()();
assert.equal(started.filter(entry => entry.id.startsWith('cold-near-')).length, 7,
  'a new visible cold image still takes precedence after a speculative slot frees');
newlyVisible.finish();
promotedNear.finish();
while (priorityFrames.length) priorityFrames.shift()();
assert.equal(started.filter(entry => entry.id.startsWith('cold-near-')).length, 8,
  'one completed low-priority load releases one speculative slot after visible demand settles');
assert.equal(new Set(started.map(entry => entry.id)).size, started.length,
  'IO and rail coordinator must not assign image src twice');
console.log('PWA.15.18 bounded IO lookahead and visible-demand priority PASS');

// PWA.15.19: a stale 120px IO entry can arrive after a rail is detached
// or hidden. Its unmeasurable geometry must never initiate an HTTP image.
const detachedPoster = pendingPoster('stale-detached', 170, 10, clippedRail);
detachedPoster.isConnected = false;
const hiddenRail = {getBoundingClientRect() {
  return {left:100,right:100,top:0,bottom:400};
}};
const hiddenPoster = pendingPoster('stale-hidden', 170, 10, hiddenRail);
const beforeStale = started.length;
demandObserver.notify([
  {target:detachedPoster,isIntersecting:true},
  {target:hiddenPoster,isIntersecting:true}
]);
assert.equal(started.length,beforeStale,
  'stale IO intersection must not start detached or hidden artwork requests');
assert.equal(detachedPoster.src,'');
assert.equal(hiddenPoster.src,'');
detachedPoster.isConnected = true;
hiddenPoster.closest = () => clippedRail;
demandObserver.notify([
  {target:detachedPoster,isIntersecting:true},
  {target:hiddenPoster,isIntersecting:true}
]);
assert.equal(started.at(-2).id,'stale-detached');
assert.equal(started.at(-1).id,'stale-hidden');
assert(started.slice(-2).every(entry => entry.priority === 'high'),
  'reconnected, genuinely visible covers retain normal high-priority demand');
assert.equal(new Set(started.map(entry => entry.id)).size,started.length,
  'recovered IO artwork must not reassign earlier URLs');
console.log('PWA.15.19 stale IO geometry and recovery contract PASS');

// PWA.15.10.1: deferred near-rail artwork has no src. When visible covers
// complete, the existing rAF rail owner must actually start low-priority
// image requests ahead of a horizontal swipe, not merely set loading=eager.
const aheadFrames = [];
let aheadObserver;
const aheadWindow = {innerWidth:800,innerHeight:600,
  requestAnimationFrame: fn => aheadFrames.push(fn),
  IntersectionObserver:class {
    constructor(callback,options){this.callback=callback;this.options=options;this.targets=new Set();aheadObserver=this;}
    observe(node){this.targets.add(node);}
    unobserve(node){this.targets.delete(node);}
    notify(entries){this.callback(entries);}
  }};
vm.runInNewContext(fs.readFileSync(path.join(__dirname,'..','home-recording-discovery.js'),'utf8'),{window:aheadWindow});
const aheadDiscovery=aheadWindow.VdrSuiteHomeRecordingDiscovery;
const aheadStarts=[];
const aheadRail={isConnected:true,
  getBoundingClientRect(){return {left:100,right:500,top:0,bottom:300};},
  querySelectorAll(){return [aheadVisible,aheadNearOne,aheadNearTwo,aheadFar];}};
function aheadImage(id,left){
  const callbacks={};let src='';
  const node={isConnected:true,complete:false,naturalWidth:0,loading:'lazy',
    closest(){return aheadRail;},
    addEventListener(type,callback){callbacks[type]=callback;},
    getBoundingClientRect(){return {left,right:left+100,top:20,bottom:170,width:100,height:150};},
    triggerLoad(){if(callbacks.load)callbacks.load();}};
  Object.defineProperty(node,'src',{get(){return src;},set(value){
    src=value; aheadStarts.push({id,priority:this.fetchPriority,loading:this.loading});
  }});
  aheadDiscovery.deferHomeArtworkImage(node,'/art/'+id+'?variant=home');
  return node;
}
const aheadVisible=aheadImage('visible',140);
const aheadNearOne=aheadImage('near-1',510);
const aheadNearTwo=aheadImage('near-2',650);
const aheadFar=aheadImage('far',1400);
aheadObserver.notify([{target:aheadVisible,isIntersecting:true}]);
while(aheadFrames.length)aheadFrames.shift()();
assert.deepEqual(aheadStarts.map(x=>x.id),['visible']);
aheadDiscovery.prioritizeInlineRailArtwork(aheadRail);
aheadFrames.shift()();
assert.equal(aheadNearOne.src,'',
  'not-yet-loaded viewport poster must prevent extra speculative requests');
aheadVisible.complete=true;aheadVisible.naturalWidth=100;
aheadVisible.triggerLoad(); // no scroll event needed for initial preload
assert.equal(aheadFrames.length,1);
aheadFrames.shift()();
assert.deepEqual(aheadStarts.map(x=>x.id),['visible','near-1','near-2'],
  'two near-rail deferred src values are assigned after visible load');
assert.deepEqual(aheadStarts.slice(1).map(x=>x.priority),['low','low']);
assert(aheadStarts.slice(1).every(x=>x.loading==='eager'),
  'warmup really requests adjacent posters, rather than only setting priority');
assert.equal(aheadFar.src,'','far unseen poster is still deferred');
assert.equal(aheadObserver.targets.has(aheadNearOne),false,
  'prefetched node is unobserved, preventing duplicate src assignment');
aheadObserver.notify([{target:aheadNearOne,isIntersecting:true}]);
assert.equal(aheadStarts.filter(x=>x.id==='near-1').length,1);
console.log('PWA.15.10.1 bounded near-rail src prewarm after visible load contract PASS');




// PWA.15.11 directional lookahead: exercise the same exported production
// coordinator and actual scroll geometry, including pending deferred src.
function scrollScene(initial){
  const frames=[],starts=[];
  let clock=50;
  const rail={isConnected:true,scrollLeft:initial,clientWidth:400,
    getBoundingClientRect(){return {left:100,right:500,top:0,bottom:350};},
    querySelectorAll(){return posters;}};
  const env={innerWidth:800,innerHeight:600,performance:{now(){return clock;}},
    requestAnimationFrame(fn){frames.push(fn);},
    IntersectionObserver:class{observe(){} unobserve(){}}};
  vm.runInNewContext(fs.readFileSync(path.join(__dirname,'..','home-recording-discovery.js'),'utf8'),{window:env});
  const discovery=env.VdrSuiteHomeRecordingDiscovery;
  function make(index){
    const callbacks={};let src='';
    const img={isConnected:true,complete:false,naturalWidth:0,loading:'lazy',
      closest(){return rail;},addEventListener(t,cb){callbacks[t]=cb;},
      getBoundingClientRect(){
        const left=160+110*index-rail.scrollLeft;
        return {left,right:left+100,top:20,bottom:170,width:100,height:150};
      },
      finish(){this.complete=true;this.naturalWidth=100;if(callbacks.load)callbacks.load();}};
    Object.defineProperty(img,'src',{get(){return src;},set(url){
      assert.equal(src,'','duplicate src start for '+index);
      src=url;starts.push({index,priority:this.fetchPriority,loading:this.loading});
    }});
    discovery.deferHomeArtworkImage(img,'/art/'+index+'?variant=home');
    return img;
  }
  const posters=Array.from({length:32},(_,i)=>make(i));
  const drain=()=>{while(frames.length)frames.shift()();};
  const completeOutstanding=()=>{
    starts.filter(x=>!posters[x.index].complete).forEach(x=>posters[x.index].finish());
    drain();
  };
  const schedule=()=>{discovery.prioritizeInlineRailArtwork(rail);drain();};
  return {rail,starts,posters,schedule,drain,completeOutstanding,advance(ms){clock+=ms;}};
}
const rightScene=scrollScene(0);
rightScene.schedule();
assert(rightScene.starts.length>=3);
assert(rightScene.starts.every(x=>x.priority==='high'),
  'initial visible requests precede speculation');
assert.equal(rightScene.posters[4].src,'','cold viewport remains protected');
rightScene.completeOutstanding();
assert(rightScene.starts.some(x=>x.index===4 && x.priority==='low'));
assert.equal(rightScene.posters[20].src,'','no full-collection initial loading');
rightScene.completeOutstanding();
const beforeRight=rightScene.starts.length;
rightScene.rail.scrollLeft=500;rightScene.schedule();
assert(rightScene.starts.some(x=>x.index>=9),
  'fast right scroll anticipates upcoming posters');
assert(rightScene.starts.slice(beforeRight).every(x=>x.index<18),
  'right lookahead remains within two visible widths');
assert.equal(rightScene.posters[25].src,'','far image remains deferred');
rightScene.schedule();
assert.equal(new Set(rightScene.starts.map(x=>x.index)).size,rightScene.starts.length,
  'repeat scheduling never reassigns src');
assert(rightScene.posters[4].complete && rightScene.posters[4].src,
  'loaded covers survive scrolling');
const leftScene=scrollScene(900);
leftScene.schedule();leftScene.completeOutstanding();leftScene.completeOutstanding();
leftScene.rail.scrollLeft=500;leftScene.schedule();
assert(leftScene.starts.some(x=>x.index===4 && x.priority==='high'),
  'newly visible images are started at high priority on reversal');
assert(leftScene.starts.some(x=>x.index===3 && x.priority==='low'),
  'reverse motion prefetches earlier left-hand cards');
assert.equal(leftScene.posters[25].src,'','left lookahead stays bounded');
assert.equal(new Set(leftScene.starts.map(x=>x.index)).size,leftScene.starts.length);
console.log('PWA.15.11 directional, rapid scroll, request reuse and bounded prefetch PASS');

// PWA.15.14: pending lookahead images that become visible must no longer
// occupy speculative slots while the user scrolls toward the next covers.
const demandScene = scrollScene(0);
demandScene.schedule();
demandScene.completeOutstanding(); // first viewport finishes; adjacent warmup begins
demandScene.schedule(); // fill another speculative slot, leave requests pending
const beforeDemandScroll = demandScene.starts.length;
assert(demandScene.starts.some(x => x.index === 4 && x.priority === 'low'));
assert.equal(demandScene.posters[4].complete, false);
demandScene.rail.scrollLeft = 500;
demandScene.schedule();
const demandAhead = demandScene.starts.slice(beforeDemandScroll)
  .filter(x => x.index >= 9 && x.priority === 'low');
assert(demandAhead.length >= 3,
  'visible pending prefetch must release lookahead slots for the next low-priority posters');
assert.equal(new Set(demandScene.starts.map(x => x.index)).size, demandScene.starts.length,
  'promoting a prefetched image must never assign the same src again');
console.log('PWA.15.14 scroll-demand prefetch budget release PASS');

// PWA.15.15: a smooth swipe has small per-frame distances but high
// velocity. Prewarm farther down its direction, using the same bounded
// coordinator and never loading the entire collection.
function settleInitial(scene) {
  scene.schedule();
  scene.completeOutstanding(); // initial viewport
  scene.completeOutstanding(); // first near-rail prefetch batch
  scene.completeOutstanding(); // remaining cold 1.5-width lookahead
}
const smoothScene=scrollScene(0);
settleInitial(smoothScene);
assert.equal(smoothScene.posters[12].src,'',
  'an untouched cold Home must not prefetch three screens ahead');
const beforeSmooth=smoothScene.starts.length;
smoothScene.advance(16);
smoothScene.rail.scrollLeft=18; // 1125px/s, but < half a rail width per frame
smoothScene.schedule();
const smoothStarts=smoothScene.starts.slice(beforeSmooth);
assert(smoothStarts.some(x=>x.index>=12 && x.priority==='low'),
  'a fast smooth swipe must begin covers beyond the previous 2-width bound');
assert(smoothStarts.length<=4,'no more than four new speculative starts per frame');
assert(smoothScene.starts.filter(x=>!smoothScene.posters[x.index].complete).length<=6,
  'no more than six outstanding speculative image requests');
assert.equal(smoothScene.posters[25].src,'',
  'even a fast swipe does not request distant posters');
const slowScene=scrollScene(0);
settleInitial(slowScene);
const beforeSlow=slowScene.starts.length;
slowScene.advance(1000);
slowScene.rail.scrollLeft=18;
slowScene.schedule();
const slowStarts=slowScene.starts.slice(beforeSlow);
assert(slowStarts.some(x=>x.index===9 && x.priority==='low'),
  'ordinary scrolling should start the next 2-width window');
assert.equal(slowScene.posters[12].src,'',
  'ordinary scrolling retains the smaller lookahead window');
assert.equal(new Set(smoothScene.starts.map(x=>x.index)).size,smoothScene.starts.length,
  'smooth scroll must never restart an assigned src');
console.log('PWA.15.15 velocity-aware, bounded directional prewarm PASS');


// PWA.15.21: nearby vertical-rail artwork must receive the *existing*
// speculative slots before farther directional candidates. A load event
// triggers the same coordinator, with no extra HTTP start owner.
const nearFrames=[], nearStarts=[];
let nearObserver;
const nearWindow={innerWidth:800,innerHeight:600,
  requestAnimationFrame(fn){nearFrames.push(fn);},
  IntersectionObserver:class{
    constructor(callback,options){this.callback=callback;this.options=options;
      if(options.rootMargin==='120px 0px')nearObserver=this;}
    observe(){}unobserve(){}
    notify(entries){this.callback(entries);}
  }};
vm.runInNewContext(fs.readFileSync(path.join(__dirname,'..','home-recording-discovery.js'),'utf8'),{window:nearWindow});
const nearDiscovery=nearWindow.VdrSuiteHomeRecordingDiscovery;
const nearRail={isConnected:true,scrollLeft:0,clientWidth:400,
  getBoundingClientRect(){return {left:100,right:500,top:0,bottom:350};},
  querySelectorAll(){return [nearVisible,...nearFar];}};
function nearImage(id,left,top,rail){
  let src='';const callbacks={};
  const img={isConnected:true,complete:false,naturalWidth:0,loading:'lazy',
    closest(){return rail||null;},
    addEventListener(type,fn){callbacks[type]=fn;},
    getBoundingClientRect(){return {left,right:left+100,top,bottom:top+150,width:100,height:150};},
    finish(){this.complete=true;this.naturalWidth=100;if(callbacks.load)callbacks.load();}};
  Object.defineProperty(img,'src',{get(){return src;},set(url){
    assert.equal(src,'','never reassign near-rail src');
    src=url;nearStarts.push({id,priority:this.fetchPriority});
  }});
  nearDiscovery.deferHomeArtworkImage(img,'/art/'+id+'?variant=home');
  return img;
}
const nearVisible=nearImage('visible',140,20,nearRail);
const nearFar=Array.from({length:5},(_,i)=>nearImage('far-'+i,710+100*i,20,nearRail));
const nearBelow=nearImage('near-below',180,620,null);
nearObserver.notify([{target:nearVisible,isIntersecting:true},
  {target:nearBelow,isIntersecting:true}]);
while(nearFrames.length)nearFrames.shift()();
assert.deepEqual(nearStarts.map(x=>x.id),['visible'],
  'cold Home still isolates first visible image from all speculative starts');
nearVisible.finish();
while(nearFrames.length)nearFrames.shift()();
assert.equal(nearStarts[1].id,'near-below',
  '20px below viewport must outrank >200px forward rail art within the same budget');
assert.equal(nearStarts[1].priority,'low',
  'nearby speculation remains low priority');
assert(nearStarts.length<=5,'only four speculative starts plus visible demand');
assert.equal(new Set(nearStarts.map(x=>x.id)).size,nearStarts.length,
  'nearby-first sorting must not start duplicate requests');
console.log('PWA.15.21 near-demand priority within unchanged 4/6 budget PASS');

// A fast swipe must stop using its three-width prewarm window after
// movement has ceased. Completion events must not keep it active forever.
const pausedScene=scrollScene(0);
settleInitial(pausedScene);
pausedScene.advance(16);
pausedScene.rail.scrollLeft=18;
pausedScene.schedule();
const beforePause=pausedScene.starts.length;
assert(pausedScene.starts.slice(beforePause-4).some(x=>x.index>=12),
  'rapid swipe still uses its original predictive lookahead');
pausedScene.advance(300);
pausedScene.completeOutstanding();
pausedScene.schedule();
assert(pausedScene.starts.slice(beforePause).every(x=>x.index<12),
  'after 300ms idle, no further three-width speculative starts');
assert.equal(new Set(pausedScene.starts.map(x=>x.index)).size,pausedScene.starts.length);
console.log('PWA.15.21 stopped-swipe lookahead decay PASS');


// PWA.15.25: use the actual four Home poster renderers, not only a
// source-level string assertion. A missing URL must never start HTTP; an
// image error changes the visual fallback only, without negative caching.
function artworkRenderer(sourceName, startName, stopName, bindingFactory, from) {
  const src=fs.readFileSync(path.join(__dirname,'..',sourceName),'utf8');
  const start=src.indexOf('  function '+startName+'(',from||0);
  const stop=src.indexOf('\n  function '+stopName+'(',start);
  assert(start>=0&&stop>start,sourceName+' renderer source exists');
  const harness=makeArtworkDoc();
  const bindings=bindingFactory(harness.doc);
  const fn=vm.runInNewContext(src.slice(start,stop)+'\n'+startName,
    bindings,{filename:sourceName+':'+startName});
  return {fn,harness};
}
function makeArtworkDoc(){
  const images=[];
  const doc={createElement(tag){
    const node={tagName:tag.toUpperCase(),className:'',textContent:'',children:[],
      listeners:{},parentNode:null,
      appendChild(child){child.parentNode=this;this.children.push(child);return child;},
      addEventListener(name,callback){this.listeners[name]=callback;},
      remove(){if(this.parentNode){
        this.parentNode.children=this.parentNode.children.filter(x=>x!==this);
        this.parentNode=null;
      }},
      fail(){if(this.listeners.error)this.listeners.error();}};
    if(tag==='img')images.push(node);
    return node;
  }};
  return {doc,images};
}
function assertNoArtwork(renderer,label){
  const {fn,harness}=renderer;
  const missing=fn({title:'Doku ohne Cover',posterUrl:''});
  assert.equal(missing.className,'media-home-discovery-artwork no-artwork',label+' no URL class');
  assert.equal(missing.textContent,'No Artwork',label+' missing artwork label');
  assert.equal(missing.children.length,0,label+' no image request for empty URL');
  const present=fn({title:'Doku mit Cover',posterUrl:'/image'});
  assert.equal(present.className,'media-home-discovery-artwork',label+' ordinary poster');
  assert.equal(present.children.length,1,label+' poster still uses native img');
  present.children[0].fail();
  assert.equal(present.className,'media-home-discovery-artwork no-artwork',label+' failed poster');
  assert.equal(present.textContent,'No Artwork',label+' failed poster text');
  assert.equal(present.children.length,0,label+' broken image removed');
  const retry=fn({title:'Doku mit Cover',posterUrl:'/image'});
  assert.equal(retry.children.length,1,label+' later render retries image, no negative cache');
  assert.equal(retry.className,'media-home-discovery-artwork');
  assert.equal(harness.images.length,2,label+' exactly two image attempts; none on missing URL');
}
const normalRenderer=artworkRenderer('home-recording-discovery.js',
  'createPosterArtwork','createArtwork',doc=>({
    doc,text:value=>String(value||''),publicPath:x=>x,homePreview:x=>x,
    deferHomeArtworkImage:(image,url)=>{image.src=url;},prioritizeVisibleArtwork:()=>{}
  }));
const normalWrap={fn:value=>normalRenderer.fn(value.title,value.posterUrl,value.title),
  harness:normalRenderer.harness};
assertNoArtwork(normalWrap,'discovery');
const bootstrapRenderer=artworkRenderer('home-recording-discovery-bootstrap.js',
  'createInlineArtwork','ensureRecordings2',doc=>({
    doc,global:{},recordingTitle:r=>r.title,recordingPosterUrl:r=>r.posterUrl,
    publicPath:x=>x
  }));
assertNoArtwork(bootstrapRenderer,'bootstrap inline');
const recentRenderer=artworkRenderer('home-recently-watched.js',
  'createArtwork','ensureRecordings2',doc=>({
    doc,global:{},title:item=>item.title,posterUrl:item=>item.posterUrl,
    publicPath:x=>x,homePreview:x=>x
  }));
assertNoArtwork(recentRenderer,'recently watched');
const movieSrc=fs.readFileSync(path.join(__dirname,'..','home-recently-watched.js'),'utf8');
const movieOffset=movieSrc.indexOf('  function posterUrl(recording)');
const movieRenderer=artworkRenderer('home-recently-watched.js',
  'createArtwork','createMovieCard',doc=>({
    doc,global:{},recordingTitle:r=>r.title,posterUrl:r=>r.posterUrl,
    publicPath:x=>x,homePreview:x=>x
  }),movieOffset);
assertNoArtwork(movieRenderer,'recent movies');
const css=fs.readFileSync(path.join(__dirname,'..','style.css'),'utf8');
assert(css.includes('.media-home-discovery-artwork.no-artwork {') &&
  css.includes('background: #050505;') && css.includes('letter-spacing: .09em;'),
  'no-artwork class must show a styled black tile in the installed main stylesheet');
console.log('PWA.15.25 no-artwork fallback in all Home poster renderers PASS');


// PWA.15.26: a settled negative native read must not turn the cached
// synthetic preferred URL into an HTTP 404. Unknown reads retain fallback.
const artworkSource = fs.readFileSync(
  path.join(__dirname, '..', 'home-recording-discovery.js'), 'utf8');
const posterStart = artworkSource.indexOf('  function recordingPosterUrl(recording) {');
const posterEnd = artworkSource.indexOf(
  '\n  function recordingMetadataPosterUrl(', posterStart);
assert(posterStart >= 0 && posterEnd > posterStart,
  'the real Home poster selector must be present');
const posterFromRecording = vm.runInNewContext(
  artworkSource.slice(posterStart, posterEnd) + '\nrecordingPosterUrl',
  {
    metadata: recording => recording.metadata || {},
    presentation: recording => recording.metadata.presentation || {},
    text: value => String(value || '').trim(),
    recordingMetadataPosterUrl: value => {
      const portrait = (value.images || []).find(entry =>
        entry.orientation === 'portrait' && entry.image &&
        entry.image.available === true && entry.image.url);
      return (portrait && portrait.image.url) ||
        (value.preferredArtwork && value.preferredArtwork.available === true
          ? value.preferredArtwork.url : '') || '';
    }
  });
const speculativePoster = '/api/vdr/recordings/metadata/image?kind=preferred&index=0';
const unverifiedRecording = {
  metadata: {
    artwork: {available: false, preferredUrl: speculativePoster},
    presentation: {posterUrl: speculativePoster}
  }
};
assert.equal(posterFromRecording(unverifiedRecording), speculativePoster,
  'unknown native status keeps the existing legacy fallback');
const settledMiss = {
  ...unverifiedRecording,
  nativeMetadata: {available: false, status: 'not-found', settled: true}
};
assert.equal(posterFromRecording(settledMiss), '',
  'settled native not-found must not request an invented preferred image');
assert.equal(posterFromRecording({
  ...unverifiedRecording,
  nativeCardMetadata: {available: false, settled: true}
}), '', 'the same settled read applies to cached Home cards');
assert.equal(posterFromRecording({
  ...unverifiedRecording,
  nativeMetadata: {available: false, settled: false}
}), speculativePoster, 'an unsettled native read is not a negative artwork proof');
assert.equal(posterFromRecording({
  ...unverifiedRecording,
  nativeMetadata: {available: true, settled: true}
}), speculativePoster, 'positive native metadata retains its image route');
const cachedPoster = '/recording-artwork/default/cached-poster';
assert.equal(posterFromRecording({
  metadata: {
    artwork: {available: true, preferredUrl: cachedPoster},
    presentation: {posterUrl: cachedPoster}
  },
  nativeMetadata: {available: false, settled: true}
}), cachedPoster, 'known cached legacy artwork takes precedence over a native miss');
const confirmedMissingImage = {
  ...unverifiedRecording,
  nativeMetadata: {
    available: true, status: 'ready',
    preferredArtwork: {available: false}, images: []
  }
};
assert.equal(posterFromRecording(confirmedMissingImage), '',
  'found native metadata without preferred/gallery artwork must not emit an image GET');
assert.equal(posterFromRecording(unverifiedRecording,
  confirmedMissingImage.nativeMetadata), '',
  'a later rich native read without image suppresses a speculative URL');
const confirmedGallery = '/api/vdr/recordings/metadata/image?kind=gallery&index=2';
const galleryMetadata = {
  available: true, preferredArtwork: {available: false},
  images: [{orientation: 'portrait', image: {available: true, url: confirmedGallery}}]
};
assert.equal(posterFromRecording(unverifiedRecording, galleryMetadata),
  confirmedGallery, 'real gallery poster replaces a guessed preferred URL');
assert.equal(posterFromRecording({
  ...unverifiedRecording,
  nativeCardMetadata: galleryMetadata
}), confirmedGallery, 'embedded native gallery works before rich metadata GET');
const revisionPoster = speculativePoster + '&assignmentRevision=12';
assert.equal(posterFromRecording({
  metadata: {
    artwork: {available: false, preferredUrl: revisionPoster},
    presentation: {posterUrl: revisionPoster}
  },
  nativeMetadata: {available: false, settled: true}
}), revisionPoster, 'selected manual poster revisions are never suppressed');
assert.equal(posterFromRecording({
  metadata: {
    artwork: {available: false, preferredUrl: '/external-legacy.jpg'},
    presentation: {posterUrl: '/external-legacy.jpg'}
  },
  nativeMetadata: {available: false, settled: true}
}), '/external-legacy.jpg', 'non-synthetic existing URLs are not suppressed');
console.log('PWA.15.26 settled native misses and legacy poster fallback PASS');
