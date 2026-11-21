'use strict';

const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'continue-watching-sync.js'), 'utf8');

let storedPosition = 400;
let revision = 2;
let submitted = [];
const window = {
  VdrSuiteBrowserSession: {
    csrfHeaders() {return {'X-VDR-Suite-CSRF':'valid-session-proof'};}
  }
};
let playback = {};
Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
  configurable:true, get(){return playback;},set(value){playback=value;}
});
window.fetch = async (url, init) => {
  assert.equal(url, '/api/media/continue-watching');
  assert.equal(init.credentials,'same-origin');
  assert.equal(init.headers['X-VDR-Suite-CSRF'],'valid-session-proof');
  const body=JSON.parse(init.body);
  if (body.operation==='revision')
    return {ok:true,status:200,json:async()=>({revision})};
  submitted.push(body);
  if(body.expectedRevision!==revision)
    return {ok:false,status:409,json:async()=>({currentRevision:revision})};
  storedPosition=body.operation==='clear'?0:body.positionSeconds;
  revision++;
  return {ok:true,status:200,json:async()=>({ok:true,revision})};
};
window.window=window;
const context={window,fetch:window.fetch,console,Promise,Number,String,Math,Object,Date,Error};
vm.createContext(context);
vm.runInContext(source,context);
const sync=window.VdrSuiteContinueWatchingSync.__test;
assert(sync && sync.enqueueVersioned && sync.loadRevision);
const item=(pos,id)=>({
  operation:'progress',backendId:'default',recordingId:'r1',
  resumeSupported:true,positionSeconds:pos,operationId:id
});
(async()=>{
  // Browser starts at revision 2 and commits an actually observed checkpoint.
  const browser={revision:null,loading:null,blocked:false};
  assert.equal(await sync.loadRevision(browser,'default','r1'),2);
  assert.equal(await sync.enqueueVersioned(item(450,'browser-1'),browser),true);
  assert.equal(storedPosition,450);
  assert.equal(browser.revision,3);

  // Another linked authorized Device commits a newer position. The browser
  // still has its old revision and may not overwrite that device.
  storedPosition=1600; revision=4;
  assert.equal(await sync.enqueueVersioned(item(500,'late-browser'),browser),false);
  assert.equal(storedPosition,1600);
  assert.equal(browser.blocked,true);
  const count=submitted.length;
  assert.equal(await sync.enqueueVersioned(item(530,'blocked-browser'),browser),false);
  assert.equal(submitted.length,count);

  // A fresh, intentionally created playback owner may observe the new
  // version and proceed, but an old owner must not silently rebase.
  const second={revision:null,loading:null,blocked:false};
  assert.equal(await sync.enqueueVersioned(item(1650,'new-session'),second),true);
  assert.equal(second.revision,5);
  assert.equal(storedPosition,1650);

  // Explicit reset retains the revision fence and cannot resurrect an
  // earlier progress update after it was confirmed.
  const clear={revision:null,loading:null,blocked:false};
  assert.equal(await sync.enqueueVersioned({
    operation:'clear',backendId:'default',recordingId:'r1',
    operationId:'explicit-clear'
  },clear),true);
  assert.equal(storedPosition,0);
  assert.equal(revision,6);
  assert.equal(await sync.enqueueVersioned(item(1700,'late-new-session'),second),false);
  assert.equal(storedPosition,0);
  assert(submitted.every(x=>Number.isSafeInteger(x.expectedRevision)));
  console.log('P1 Browser/Device CAS checkpoint and reset contract PASS');
})().catch(e=>{console.error(e);process.exitCode=1;});
