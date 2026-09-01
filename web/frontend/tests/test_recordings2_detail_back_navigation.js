'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const scrollCalls = [];

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
  createElement:createElement
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
  createPoster() { return createElement('figure'); },
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

const view = window.VdrSuiteRecordings2BrowserView.create({
  getState() { return currentState; },
  closeDetail() { closeCount += 1; },
  reload() {},
  openFolder() {},
  selectRecording() {},
  loadMore() {}
});

view.renderDetail();
assert.strictEqual(scrollCalls.length, 1, 'opening a Recording detail must reveal its start');
assert.strictEqual(scrollCalls[0].element, target.children[0]);
assert.deepStrictEqual(scrollCalls[0].options, {block:'start', behavior:'auto'});

let root = target.children[0];
let header = root.children[0];
let toolbar = header.children[1];
let backButton = toolbar.children[0];
assert.strictEqual(backButton.textContent, '← Zum Ordner');
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
  detailReturnLabel:'← Zurück zu Home'
});
view.renderDetail();
assert.strictEqual(scrollCalls.length, 3, 'folder -> detail must reveal the detail start again');
root = target.children[0];
header = root.children[0];
toolbar = header.children[1];
backButton = toolbar.children[0];
assert.strictEqual(backButton.textContent, '← Zurück zu Home');

const source = fs.readFileSync('web/frontend/recordings2-browser-view.js', 'utf8');
assert(!source.includes('history.pushState'));
assert(!source.includes('history.replaceState'));
assert(!source.includes('popstate'));

console.log('recordings2 detail back navigation ok');
