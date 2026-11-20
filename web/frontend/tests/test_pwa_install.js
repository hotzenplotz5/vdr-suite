'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const read = name => fs.readFileSync(path.join(__dirname, '..', name));
const manifest = JSON.parse(read('manifest.webmanifest').toString());
const source = read('pwa-register.js').toString();

for (const prefix of ['', '/vdr-suite']) {
  const address = 'https://suite.test' + prefix + '/frontend/manifest.webmanifest';
  const scope = 'https://suite.test' + prefix + '/frontend/';
  for (const key of ['id', 'start_url', 'scope']) assert.equal(new URL(manifest[key], address).href, scope);
  assert.equal(manifest.display, 'standalone');
  assert.notEqual(manifest.prefer_related_applications, true);
  for (const size of [192,512]) {
    const entry = manifest.icons.find(icon => icon.sizes === size+'x'+size);
    assert(entry && entry.type === 'image/png');
    assert(entry.purpose.split(' ').includes('any'));
    assert(entry.purpose.split(' ').includes('maskable'));
    assert.equal(new URL(entry.src,address).href,scope+'pwa-icon-'+size+'.png');
    const png = read(entry.src);
    assert.equal(png.subarray(0,8).toString('hex'),'89504e470d0a1a0a');
    assert.equal(png.subarray(12,16).toString(),'IHDR');
    assert.equal(png.readUInt32BE(16),size);
    assert.equal(png.readUInt32BE(20),size);
  }
}

function fixture({secure=true, standalone=false, legacy=false}={}) {
  const events={}, mediaEvents={}, actions={}, classes=new Set();
  const row={hidden:true}, button={addEventListener:(name,fn)=>{actions[name]=fn;}};
  const mq={matches:standalone,addEventListener:(name,fn)=>{mediaEvents[name]=fn;}};
  const window={
    isSecureContext:secure, navigator:{standalone:legacy},
    document:{body:{classList:{toggle:(name,flag)=>{if(flag)classes.add(name);else classes.delete(name);}}},
      getElementById:id=>id==='vdr-suite-pwa-install'?row:id==='vdr-suite-pwa-install-button'?button:null},
    addEventListener:(name,fn)=>{events[name]=fn;},
    matchMedia:q=>{assert.equal(q,'(display-mode: standalone)');return mq;}
  };
  vm.runInNewContext(source,{window});
  let count=0, prevented=0;
  function offer(outcome='dismissed', error=false) {
    const event={preventDefault(){prevented++;},prompt(){count++;return error?Promise.reject(Error('not available')):Promise.resolve({outcome});}};
    events.beforeinstallprompt(event);
    return event;
  }
  return {row,events,classes,offer,click:()=>actions.click(),count:()=>count,prevented:()=>prevented,
    standalone:value=>{mq.matches=value;mediaEvents.change();}};
}
(async()=>{
  const a=fixture();
  assert(a.row.hidden,'unsupported until real offer');
  a.offer();
  assert(!a.row.hidden,'browser offer reveals button');
  assert.equal(a.count(),0,'no automatic prompts');
  assert.equal(a.prevented(),1);
  const clicked=a.click();
  assert.equal(a.count(),1,'prompt invoked synchronously from click');
  assert(a.row.hidden);
  await clicked;
  await a.click();
  assert.equal(a.count(),1,'dismissed offer is consumed');
  a.offer('accepted',true);
  await a.click();
  assert(a.row.hidden,'prompt failure remains optional');
  a.offer('accepted');
  await a.click();
  a.events.appinstalled();
  a.offer('accepted');
  assert(a.row.hidden,'installed app does not offer again');
  assert(!a.classes.has('vss-pwa-standalone'),'installing does not change current display mode');
  a.standalone(true);
  assert(a.classes.has('vss-pwa-standalone'));
  const b=fixture({standalone:true});
  b.offer(); assert(b.row.hidden); assert.equal(b.prevented(),0);
  b.standalone(false); b.offer(); assert(!b.row.hidden);
  const c=fixture({secure:false});
  c.offer(); await c.click(); assert(c.row.hidden); assert.equal(c.count(),0);assert.equal(c.prevented(),0);
  const d=fixture({legacy:true});
  d.offer(); assert(d.row.hidden);assert(d.classes.has('vss-pwa-standalone'));
  const e=fixture();
  e.events.beforeinstallprompt({});assert(e.row.hidden);
  e.offer(); const first=e.click();const second=e.click();await Promise.all([first,second]);assert.equal(e.count(),1);
  console.log('PWA.8 manifest/icon/install contract PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
