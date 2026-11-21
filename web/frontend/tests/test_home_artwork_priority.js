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
