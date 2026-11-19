'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const window = {navigator: {}, isSecureContext:false};
vm.runInNewContext(fs.readFileSync(path.join(__dirname,'../pwa-register.js'),'utf8'), {window});
const create = window.VdrSuiteConnectionRuntime.create;
let online = true, checks = 0, navigations = 0, suspends = 0, verify = () => Promise.resolve(true);
const states = [];
const connection = create({online:()=>online, verify:()=>{checks++; return verify();},
  navigate:()=>navigations++, suspend:()=>suspends++, render:state=>states.push(state)});
(async()=>{
  assert.equal(connection.state(),'ready');
  connection.live('error'); assert.equal(connection.state(),'reconnecting');
  connection.live('open'); assert.equal(connection.state(),'ready');
  assert.equal(checks,0); assert.equal(navigations,0);
  online=false; connection.offline(); assert.equal(connection.state(),'offline');
  assert.equal(await connection.retry(),false); assert.equal(checks,0);
  online=true; verify=()=>Promise.resolve(false);
  assert.equal(await connection.retry(),false); assert.equal(connection.state(),'unavailable');
  assert.equal(navigations,0);
  let finish;
  verify=()=>new Promise(resolve=>{finish=resolve;});
  const first=connection.retry(); const second=connection.retry();
  assert.equal(first,second); await Promise.resolve();
  online=false; connection.offline(); finish(true);
  assert.equal(await first,false); assert.equal(navigations,0);
  online=true; verify=()=>Promise.resolve(true);
  connection.live('open'); await connection.retry();
  assert.equal(connection.state(),'recovering'); assert.equal(navigations,1);
  assert.equal(await connection.retry(),false); assert.equal(navigations,1);
  assert(suspends>=2);
  const fresh=create({online:()=>false, verify:()=>{throw Error('must not verify offline');},
    navigate:()=>{throw Error('must not navigate offline');}, suspend:()=>{}, render:()=>{}});
  assert.equal(fresh.state(),'offline');
  assert.equal(await fresh.retry(),false);
  console.log('PWA.6 connection state contract PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
