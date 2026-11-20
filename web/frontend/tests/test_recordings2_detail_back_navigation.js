'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const scrollCalls = [];
let posterCreations = 0;

function createElement(tagName) {
  const listeners = Object.create(null);
  const element = {
    tagName:String(tagName || '').toUpperCase(),
    type:'',
    className:'',
    textContent:'',
    hidden:false,
    disabled:false,
    children:[],
    dataset:{},
    attributes:Object.create(null),
    classList:{
      add(name) {
        const names = element.className.split(/\s+/).filter(Boolean);
        if (!names.includes(name)) names.push(name);
        element.className = names.join(' ');
      },
      remove(name) {
        element.className = element.className
          .split(/\s+/)
          .filter(Boolean)
          .filter(value => value !== name)
          .join(' ');
      },
      contains(name) {
        return element.className.split(/\s+/).filter(Boolean).includes(name);
      }
    },
    setAttribute(name, value) {
      this.attributes[name] = String(value);
    },
    appendChild(child) {
      this.children.push(child);
      return child;
    },
    append() {
      this.children.push(...arguments);
    },
    replaceChildren() {
      this.children = Array.from(arguments);
    },
    addEventListener(name, callback) {
      listeners[name] = callback;
    },
    click() {
      if (listeners.click) listeners.click({target:this});
    },
    scrollIntoView(options) {
      scrollCalls.push({element:this, options:options});
    }
  };
  return element;
}

const target = createElement('div');
const document = {
  createElement:createElement,
  // The real browser always provides document.head. The DOM fixture must
  // support the stylesheet installed when recording detail is opened.
  head: { appendChild() {} }
};

function first(object, names, fallback) {
  const source = object && typeof object === 'object' ? object : {};
  for (const name of names) {
    if (source[name] !== undefined && source[name] !== null && source[name] !== '') {
      return source[name];
    }
  }
  return fallback;
}

const shared = {
  mountTarget() { return target; },
  installStyles() {},
  selectedBackendId() { return 'default'; },
  normalizePath(value) { return String(value || '').replace(/^\/+|\/+$/g, ''); },
  decodeDisplayText(value) { return String(value || ''); },
  text(value) { return value === undefined || value === null ? '' : String(value); },
  first:first,
  number(value, fallback) {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : Number(fallback || 0);
  },
  node(tagName, className, textContent) {
    const node = createElement(tagName);
    node.className = className || '';
    node.textContent = textContent === undefined || textContent === null
      ? ''
      : String(textContent);
    return node;
  },
  createButton(label, callback, className) {
    const button = createElement('button');
    button.type = 'button';
    button.className = className || '';
    button.textContent = String(label || '');
    button.addEventListener('click', callback);
    return button;
  },
  createPoster() { posterCreations += 1; return createElement('figure'); },
  folderList(data) { return data && Array.isArray(data.folders) ? data.folders : []; },
  recordingList(data) { return data && Array.isArray(data.recordings) ? data.recordings : []; },
  recordingTitle(recording) { return String(recording && recording.title || 'Aufnahme'); },
  recordingSubtitle(recording) { return String(recording && recording.subtitle || ''); },
  recordingSummary(recording) { return String(recording && recording.summary || ''); },
  formatStart(value) { return String(value || ''); },
  formatDuration(value) { return String(value || 0) + ' s'; },
  formatSize(value) { return String(value || 0) + ' MB'; },
  provider(recording) {
    return recording && recording.metadata && recording.metadata.provider
      ? recording.metadata.provider
      : {};
  }
};

const window = {
  VdrSuiteRecordings2Shared:shared
};
const context = vm.createContext({
  window,
  document,
  console,
  Object,
  String,
  Number,
  Promise,
  Array
});

vm.runInContext(
  fs.readFileSync('web/frontend/recordings2-browser-cards.js', 'utf8'),
  context,
  {filename:'web/frontend/recordings2-browser-cards.js'}
);

vm.runInContext(
  fs.readFileSync('web/frontend/recordings2-browser-view.js', 'utf8'),
  context,
  {filename:'web/frontend/recordings2-browser-view.js'}
);

const recording = {
  title:'s04e04 Eidwahrer',
  path:'Serien/Game of Thrones/s04e04',
  startTime:'24.12.2015, 23:15',
  durationSeconds:3600,
  sizeMb:2048,
  summary:'Beschreibung',
  metadata:{provider:{source:'VDR'}}
};

let currentState = {
  selectedRecording:recording,
  detailReturnLabel:'← Zum Ordner',
  backendId:'default',
  path:'Serien/Game of Thrones',
  parentPath:'Serien',
  recordings:[],
  data:{folders:[], recordings:[], recordingCount:0, returnedCount:0},
  loadingMore:false,
  error:null
};
let closeCount = 0;
let homeCount = 0;

const view = window.VdrSuiteRecordings2BrowserView.create({
  getState() { return currentState; },
  closeDetail() { closeCount += 1; },
  goHome() { homeCount += 1; },
  reload() {},
  openFolder() {},
  selectRecording() {},
  loadMore() {}
});

view.renderDetail();
assert.strictEqual(scrollCalls.length, 1, 'opening a Recording detail must reveal its start');
assert.strictEqual(scrollCalls[0].element, target.children[0]);
assert.strictEqual(scrollCalls[0].options.block, 'start');
assert.strictEqual(scrollCalls[0].options.behavior, 'auto');

let root = target.children[0];
let header = root.children[0];
let toolbar = header.children[1];
let backButton = toolbar.children[0];
let homeButton = toolbar.children[1];
assert.strictEqual(backButton.textContent, '← Zum Ordner');
assert.strictEqual(homeButton.textContent, '⌂ Home');
assert(homeButton.classList.contains('recordings2-home'));
homeButton.click();
assert.strictEqual(homeCount, 1,
  'every Recording detail must expose the canonical Home exit');
backButton.click();
assert.strictEqual(closeCount, 1, 'detail Back must use the canonical closeDetail owner');

view.renderDetail();
assert.strictEqual(
  scrollCalls.length,
  1,
  'a detail rerender must not pull the user back to the top after they started reading'
);

view.destroy();
view.renderDetail();
assert.strictEqual(
  scrollCalls.length,
  2,
  'reopening the same Recording after leaving detail must reveal Back again'
);

currentState = Object.assign({}, currentState, {
  selectedRecording:null,
  recordings:[],
  data:{folders:[], recordings:[], recordingCount:0, returnedCount:0}
});
view.renderFolder();
assert.strictEqual(scrollCalls.length, 2);

currentState = Object.assign({}, currentState, {
  selectedRecording:recording,
  detailReturnLabel:'← Zurück zur Suche',
  detailHome:function () {},
  detailHomeLabel:'⌂ Home'
});
view.renderDetail();
assert.strictEqual(scrollCalls.length, 3, 'folder -> detail must reveal the detail start again');
root = target.children[0];
header = root.children[0];
toolbar = header.children[1];
backButton = toolbar.children[0];
homeButton = toolbar.children[1];
assert.strictEqual(backButton.textContent, '← Zurück zur Suche');
assert.strictEqual(homeButton.textContent, '⌂ Home');
homeButton.click();
assert.strictEqual(homeCount, 2,
  'external Recording detail Home must use the canonical Home exit owner');

// PWA.15.7: switching Komödie -> Horror -> Komödie must retain the *same*
 // poster DOM nodes after a fresh authorized folder response.
const ace = Object.assign({}, recording, {title:'Ace Ventura'});
const scary = Object.assign({}, recording, {title:'Horror'});
const comedyPayload = {
  recordingFolder:true, path:'Komödie', folders:[], recordings:[ace],
  totalCount:1, recordingCount:1, folderCount:0, returnedCount:1
};
const horrorPayload = {
  recordingFolder:true, path:'Horror', folders:[], recordings:[scary],
  totalCount:1, recordingCount:1, folderCount:0, returnedCount:1
};
function selectFolder(path, payload, recordings) {
  currentState = Object.assign({}, currentState, {
    selectedRecording:null, backendId:'default', path:path, parentPath:'',
    data:payload, recordings:recordings, error:null, loadingMore:false
  });
  view.renderLoading();
  view.renderFolder();
  return target.children[0];
}
const comedyRoot = selectFolder('Komödie', comedyPayload, [ace]);
const createdBeforeReturn = posterCreations + 1;
selectFolder('Horror', horrorPayload, [scary]);
assert.strictEqual(posterCreations, createdBeforeReturn);
const comedyAgain = selectFolder('Komödie', comedyPayload, [ace]);
assert.strictEqual(comedyAgain, comedyRoot,
  'returning to an unchanged authorized folder must reattach the same DOM tree');
assert.strictEqual(posterCreations, createdBeforeReturn,
  'a cached folder return must not create or assign new poster images');

// A changed response must not silently keep stale metadata or actions.
const revisedAce = Object.assign({}, ace, {title:'Ace Ventura – aktualisiert'});
const revisedPayload = Object.assign({}, comedyPayload, {recordings:[revisedAce]});
currentState = Object.assign({}, currentState, {
  data:revisedPayload, recordings:[revisedAce]
});
view.renderFolder();
assert.notStrictEqual(target.children[0], comedyRoot,
  'a changed server recording invalidates the cached presentation');
assert.strictEqual(posterCreations, createdBeforeReturn + 1);

// A failed authorization clears the retained views; never show stale posters.
selectFolder('Horror', horrorPayload, [scary]);
currentState = Object.assign({}, currentState, {error:new Error('401 Unauthorized')});
view.renderError();
const afterDenied = selectFolder('Komödie', comedyPayload, [ace]);
assert.notStrictEqual(afterDenied, comedyRoot,
  'an authorization error must not revive a previously retained view');

// Backend isolation: another backend may not reuse the first backend's DOM.
const isolatedRoot = afterDenied;
currentState = Object.assign({}, currentState, {backendId:'another'});
view.renderLoading();
view.renderFolder();
assert.notStrictEqual(target.children[0], isolatedRoot,
  'switching backend must not retain another backend\'s poster DOM');

view.destroy();
const source = fs.readFileSync('web/frontend/recordings2-browser-view.js', 'utf8');
const runtimeSource = fs.readFileSync('web/frontend/recordings2.js', 'utf8');
assert(runtimeSource.includes("if (typeof state.detailHome === 'function')"));
assert(runtimeSource.includes("global.selectModule('overview')"));
assert(runtimeSource.includes("config.focusNavigation === true"));
assert(runtimeSource.includes("target.querySelector('.recordings2-toolbar button')"));
assert(!source.includes('history.pushState'));
assert(!source.includes('history.replaceState'));
assert(!source.includes('popstate'));

console.log('recordings2 detail back navigation ok');
