'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const window = {};
vm.runInNewContext(fs.readFileSync('web/frontend/modules/image-gallery.js','utf8'),{window});
const storage = new Map();
const adapter = {getItem:k=>storage.get(k),setItem:(k,v)=>storage.set(k,v)};
const nav = window.VdrSuiteImageGallery.createNavigation(adapter);
assert.equal(nav.state.mode,'carousel');
nav.setEntries([{directory:true,name:'Folder'},{name:'Photo 1'},{name:'Photo 2'}]);
assert.equal(nav.open(),false);
nav.move(1);
nav.toggleMode();
assert.equal(nav.state.mode,'grid');
assert.equal(nav.state.selected,1);
assert.equal(window.VdrSuiteImageGallery.createNavigation(adapter).state.mode,'grid');
assert.equal(nav.open(),true);
nav.rotate();
nav.zoom();
assert.equal(nav.state.rotation,90);
assert.equal(nav.state.zoom,1.5);
assert.equal(nav.stepPhoto(1,false),true);
assert.equal(nav.state.selected,2);
assert.equal(nav.state.rotation,0);
assert.equal(nav.stepPhoto(1,false),false);
assert.equal(nav.stepPhoto(1,true),true);
assert.equal(nav.state.selected,1);
assert.equal(nav.stepPhoto(-1,false),false);
nav.close();
assert.equal(nav.state.full,false);
assert.equal(nav.state.selected,1);
nav.setEntries([],100);
assert.equal(nav.state.selected,0);
assert.equal(nav.open(),false);
console.log('IMAGE_GALLERY_NAVIGATION=PASS');

async function testPublicGalleryClient() {
  const path = require('node:path');
  const canonical = path.resolve(__dirname,'../../../clients/reference-js/public-v1-client.js');
  assert.equal(fs.realpathSync(path.resolve(__dirname,'../api/public-v1-client.js')),canonical);
  const api = require(canonical);
  const requests = [];
  const client = api.createClient({
    baseUrl:'https://suite.example',credentials:'same-origin',
    fetch(url,options) {
      requests.push({url,options});
      return Promise.resolve({ok:true,status:200,headers:{get(){return null;}},
        text(){return Promise.resolve(JSON.stringify({entries:[],enabled:true,sources:[]}));}});
    }
  });
  const prefix='https://suite.example/api/v1/addons/image/';
  const options={backendId:'default',sourceId:'pictures',path:'Urlaub & Meer/Bäume #1.jpg'};
  await client.getImageGalleryLibrary({backendId:'default',cache:'no-store'});
  await client.getImageGalleryFolder(options);
  const query='?backendId=default&sourceId=pictures&path='+encodeURIComponent(options.path);
  assert.equal(requests[0].url,prefix+'library?backendId=default');
  assert.equal(requests[0].options.credentials,'same-origin');
  assert.equal(requests[0].options.cache,'no-store');
  assert.equal(requests[1].url,prefix+'browse'+query);
  assert.equal(client.imageGalleryPreviewUrl(options),prefix+'preview'+query);
  assert.equal(client.imageGalleryPhotoUrl(options),prefix+'photo'+query);
  assert.equal(requests.length,2,'constructing image URLs must not trigger extra requests');
  await client.getImageGalleryLibrary({backendId:'default',sample:true,cache:'no-store'});
  assert.equal(requests[2].url,prefix+'library?backendId=default&sample=1');
  assert.throws(()=>client.getImageGalleryLibrary({backendId:'../other'}));
  assert.throws(()=>client.getImageGalleryFolder({backendId:'default',path:''}));
  console.log('IMAGE_GALLERY_PUBLIC_CLIENT=PASS');
}
testPublicGalleryClient().catch(error=>{console.error(error);process.exitCode=1;});
