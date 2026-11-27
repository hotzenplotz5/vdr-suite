'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

// A DOM/clock double executes the real mount and event handlers. CSS rendering,
// CEF key mapping and actual image decoding are deliberately not claimed.
class Element {
  constructor(doc,tag) {
    this.ownerDocument=doc;this.tagName=tag;this.children=[];this.parentNode=null;
    this.dataset={};this.style={};this.attributes={};this.listeners={};
    this.className='';this.textContent='';this.hidden=false;this.clientWidth=1200;
    this.classList={toggle:(name,enabled)=>{
      const classes=new Set(this.className.split(' ').filter(Boolean));
      if(enabled) classes.add(name);else classes.delete(name);
      this.className=Array.from(classes).join(' ');
    }};
  }
  setAttribute(key,value){this.attributes[key]=String(value);}
  addEventListener(key,fn){(this.listeners[key] ||= []).push(fn);}
  emit(key,event={}){for(const fn of this.listeners[key] || [])fn({target:this,...event});}
  appendChild(el){return this.insertBefore(el,null);}
  append(...els){els.forEach(el=>this.appendChild(el));}
  insertBefore(el,before){
    if(el.parentNode)el.remove();
    const i=before ? this.children.indexOf(before) : this.children.length;
    assert(i>=0);this.children.splice(i,0,el);el.parentNode=this;return el;
  }
  remove(){if(this.parentNode){const siblings=this.parentNode.children;siblings.splice(siblings.indexOf(this),1);this.parentNode=null;}}
  replaceChildren(...els){for(const child of [...this.children])child.remove();this.append(...els);}
  contains(el){return this===el || this.children.some(child=>child.contains(el));}
  find(fn){if(fn(this))return this;for(const child of this.children){const found=child.find(fn);if(found)return found;}return null;}
  querySelector(tag){return this.find(el=>el.tagName===tag);}
  focus(){this.ownerDocument.activeElement=this;this.emit('focus');}
}
function documentFixture(){
  const doc={listeners:{},createElement(tag){return new Element(this,tag);},
    getElementById(id){return this.head.find(el=>el.id===id)||this.body.find(el=>el.id===id);},
    addEventListener(key,fn){this.listeners[key]=fn;},
    removeEventListener(key,fn){assert.equal(this.listeners[key],fn);delete this.listeners[key];}};
  doc.head=doc.createElement('head');doc.body=doc.createElement('body');return doc;
}
const flush=()=>new Promise(resolve=>setImmediate(resolve));
async function run(){
  const doc=documentFixture(),target=doc.createElement('main');doc.body.appendChild(target);
  const storage=new Map([['vdr-suite.image-gallery.view','grid']]);
  const timers=new Map();let timerId=0;
  const window={localStorage:{getItem:k=>storage.get(k),setItem:(k,v)=>storage.set(k,v)},
    setInterval(fn,ms){timers.set(++timerId,{fn,ms});return timerId;},
    clearInterval(id){timers.delete(id);}};
  vm.runInNewContext(fs.readFileSync('web/frontend/modules/image-gallery.js','utf8'),{window});
  const photos=Array.from({length:1000},(_,i)=>({name:'Photo '+i,relativePath:'Photo '+i+'.jpg',cover:'Photo '+i+'.jpg'}));
  const requests=[];
  const mounted=window.VdrSuiteImageGallery.mount(target,{
    listSources:async()=>[{id:'pictures',title:'Pictures'}],
    browse:async(id,path)=>{requests.push([id,path]);return photos;},
    previewUrl:(id,path)=>'/preview/'+id+'/'+path,
    originalUrl:(id,path)=>'/photo/'+id+'/'+path
  },{locale:'en',intervalMs:5000,repeat:false});
  const root=()=>target.children[0];
  const byClass=(parent,name)=>parent.find(el=>el.className.split(' ').includes(name));
  const byText=(parent,txt)=>parent.find(el=>el.tagName==='button'&&el.textContent===txt);
  const gallery=()=>byClass(root(),'image-gallery-grid')||byClass(root(),'image-gallery-carousel');
  const selected=()=>Number(byClass(gallery(),'is-selected').dataset.index);
  const key=(k)=>doc.listeners.keydown({key:k,target:doc.activeElement,preventDefault(){},stopPropagation(){}});
  await flush();
  assert.equal(gallery().children.length,1);
  gallery().children[0].emit('click');
  await flush();
  assert.deepEqual(requests,[['pictures','']]);
  assert.equal(gallery().children.length,80);
  const first=gallery().children[0];
  key('ArrowRight');
  assert.equal(selected(),1);
  assert.equal(gallery().children[0],first,'moving focus must keep existing image tiles');
  byText(root(),'More pictures').emit('click');
  assert.equal(selected(),81);
  assert.equal(gallery().children.length,80);
  assert.equal(gallery().children[0].dataset.index,'80');
  key('ArrowLeft');key('ArrowLeft');
  assert.equal(selected(),79);
  assert.equal(gallery().children.length,80);
  key('ArrowRight');
  assert.equal(selected(),80);
  assert.equal(gallery().children.length,80);
  key('1');
  assert.equal(storage.get('vdr-suite.image-gallery.view'),'carousel');
  assert.equal(selected(),80);
  assert.equal(gallery().children.length,7);
  const centered=byClass(gallery(),'is-selected');
  key('ArrowRight');
  assert.equal(selected(),81);
  assert(gallery().children.includes(centered),'Cover Flow must retain neighboring elements');
  byClass(gallery(),'is-selected').emit('click');
  const overlay=byClass(doc.body,'image-gallery-full');
  assert(overlay);
  const image=overlay.querySelector('img');
  assert.equal(image.src,'/photo/pictures/Photo 81.jpg');
  key('ColorF1Green');
  assert.equal(timers.size,1);
  const clock=Array.from(timers.values())[0];
  assert.equal(clock.ms,5000);
  clock.fn();
  assert.equal(image.src,'/photo/pictures/Photo 82.jpg');
  key('Escape');
  assert.equal(timers.size,0);
  assert.equal(byClass(doc.body,'image-gallery-full'),null);
  assert.equal(selected(),82);
  key('1');
  assert.equal(selected(),82);
  assert.equal(gallery().children.length,80);
  key('Escape');
  assert.equal(gallery().children.length,1);
  mounted.dispose();
  assert.equal(target.children.length,0);
  assert.equal(doc.listeners.keydown,undefined);
  assert.equal(timers.size,0);
  assert.equal(requests.length,1,'navigation and slideshow must not poll the backend');
  console.log('IMAGE_GALLERY_MOUNT_LARGE_COLLECTION=PASS');
  const home=doc.createElement('section');doc.body.appendChild(home);
  const sample=Array.from({length:12},(_,i)=>({sourceId:'pictures',name:'Sample '+i,relativePath:'Urlaub/Sample '+i+'.jpg'}));
  let reads=0,opened=[];
  const railAdapter={sample:async()=>{++reads;return sample;},previewUrl:(id,path)=>'/preview/'+id+'/'+path};
  const rail=window.VdrSuiteImageGallery.mountHome(home,railAdapter,{locale:'en',open:photo=>opened.push(photo)});
  await flush();
  const cards=home.children[1].children;
  assert.equal(cards.length,10);assert.equal(reads,1);
  const camera=cards[0].children[0];
  assert.equal(camera.tagName,'img');
  assert.equal(camera.alt,'');
  assert.equal(camera.src,'data:image/webp;base64,'+
    fs.readFileSync('modules/image/assets/gallery-camera.webp').toString('base64'),
    'shortcut must render the exact approved camera artwork');
  const styles=doc.getElementById('image-gallery-style').textContent;
  assert(styles.includes('#detail:has(.module-tab.active[data-module="overview"]) > [data-home-zone="image-gallery"]{order:100;'),
    'Gallery must follow existing flex-ordered Home zones even when loaded late');
  const homeStyles=fs.readFileSync('web/frontend/home-continue-watching.js','utf8');
  assert(homeStyles.includes('[data-home-zone="additional-sections"]{order:40}') &&
    homeStyles.includes('[data-home-zone="primary-rail"]{order:110}') &&
    homeStyles.includes('#detail-data{order:120}'),
    'Gallery follows media rails but precedes the inventory heading and cards');
  cards[0].emit('click');cards[3].emit('click');
  assert.equal(opened[0],null);assert.equal(opened[1],sample[2]);
  assert.equal(cards[3].children[0].src,'/preview/pictures/Urlaub/Sample 2.jpg');
  rail.dispose();assert(home.hidden);assert.equal(home.children.length,0);
  const direct=window.VdrSuiteImageGallery.mount(target,{
    listSources:async()=>[{id:'pictures',title:'Pictures'}],
    browse:async(id,path)=>{assert.equal(path,'Urlaub');return sample;},
    previewUrl:(id,path)=>'/preview/'+id+'/'+path,
    originalUrl:(id,path)=>'/photo/'+id+'/'+path
  },{locale:'en',initialPhoto:sample[2]});
  await flush();
  assert.equal(byClass(doc.body,'image-gallery-full').querySelector('img').src,'/photo/pictures/Urlaub/Sample 2.jpg');
  direct.dispose();
  let resolveSample;
  const stale=window.VdrSuiteImageGallery.mountHome(home,{sample:()=>new Promise(resolve=>{resolveSample=resolve;})},{locale:'en',open(){}});
  await flush();stale.dispose();resolveSample(sample);await flush();
  assert.equal(home.children.length,0);
  // Exercise the actual application owner, including sample lifetime and
  // selecting the existing gallery module with a photo from the rail.
  const app=fs.readFileSync('web/frontend/app.js','utf8');
  const extract=(from,to)=>{
    const start=app.indexOf(from),end=app.indexOf(to,start);
    assert(start>=0 && end>start);return app.slice(start,end);
  };
  const appWindow=window;
  let selectedModuleSeen=null, sampleReads=0;
  const tab={hidden:false,disabled:false};
  const nav={children:[],querySelector(){return this.search;},insertBefore(child,before){
    this.children.splice(this.children.indexOf(child),1);
    this.children.splice(before ? this.children.indexOf(before) : this.children.length,0,child);
  }};
  const search={parentNode:nav};
  Object.defineProperty(search,'nextElementSibling',{get(){return nav.children[nav.children.indexOf(search)+1]||null;}});
  tab.parentNode=nav;nav.search=search;nav.children=[tab,search];
  const galleryApi=appWindow.VdrSuiteImageGallery;
  appWindow.VdrSuiteImageGallery={...galleryApi,adapterFor:()=>({
    sample:async()=>{++sampleReads;return sample;},
    previewUrl:railAdapter.previewUrl
  })};
  const ctx={
    window:appWindow,document:{documentElement:{lang:'en'},querySelector:()=>home,
      getElementById:()=>nav,querySelectorAll:()=>[]},
    imageGalleryTab:tab,imageGalleryHome:null,imageGalleryInitialPhoto:null,imageGalleryBackend:'default',
    selectedModule:'overview',selectedBackendId:'default',browserSessionFrontendActive:true,
    selectedBackend:{id:'default'},currentSnapshot:{},placeMediaImportTab(){},
    closeHomeHevcStream(){},renderMediaToolsHomeShortcut(){},shouldPublishHomeResume(){return false;},
    renderSelectedModule(){selectedModuleSeen=ctx.selectedModule;if(selectedModuleSeen==='overview')ctx.renderImageGalleryHome();},
    disposeImageGallery(){},mediaImportModule(){return null;}
  };
  vm.createContext(ctx);
  vm.runInContext(extract('function clearImageGalleryHome()', 'function resetImageGalleryVisibility()')+
    extract('function placeImageGalleryTab()', 'placeImageGalleryTab();')+
    extract('function selectModule(moduleName)', 'function markSelected(backendId)'),ctx);
  ctx.placeImageGalleryTab();assert.equal(nav.children[1],tab);
  ctx.renderImageGalleryHome();await flush();
  ctx.renderImageGalleryHome();assert.equal(sampleReads,1);
  home.children[1].children[2].emit('click');
  assert.equal(selectedModuleSeen,'image-gallery');assert.equal(ctx.imageGalleryInitialPhoto,sample[1]);assert(home.hidden);
  ctx.selectModule('overview');await flush();assert.equal(sampleReads,2);
  ctx.imageGalleryBackend='';ctx.renderImageGalleryHome();assert(home.hidden);
  console.log('IMAGE_GALLERY_HOME_RAIL=PASS');
}
run().catch(error=>{console.error(error);process.exitCode=1;});
