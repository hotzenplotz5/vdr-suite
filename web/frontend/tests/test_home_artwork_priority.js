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
assert.equal(next.filter(i=>i.fetchPriority==='low').length,4);
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
const deferredFirst = {src:'',loading:'lazy'};
const deferredSecond = {src:'',loading:'lazy'};
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
