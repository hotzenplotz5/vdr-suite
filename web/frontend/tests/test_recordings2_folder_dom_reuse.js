'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

// Exercise the production Shared -> Cards -> BrowserView composition, not
// an isolated poster cache helper.
let createdImages = 0;
function element(tag) {
  const listeners = {};
  const node = {
    tagName: tag.toUpperCase(), className: '', textContent: '', children: [],
    parentNode: null, dataset: {}, attributes: {}, scrollTop: 0, style: {},
    disabled: false, hidden: false,
    setAttribute(name, value) { this.attributes[name] = String(value); },
    addEventListener(name, callback) { listeners[name] = callback; },
    click() { if (listeners.click) listeners.click({target: this}); },
    appendChild(child) {
      if (child.parentNode) {
        const siblings = child.parentNode.children;
        siblings.splice(siblings.indexOf(child), 1);
      }
      child.parentNode = this;
      this.children.push(child);
      return child;
    },
    append(...children) { children.forEach(child => this.appendChild(child)); },
    replaceChildren(...children) {
      this.children.forEach(child => { child.parentNode = null; });
      this.children = [];
      this.append(...children);
    },
    contains(child) {
      return this === child || this.children.some(node => node.contains(child));
    },
    querySelector(selector) {
      for (const child of this.children) {
        if (selector === 'img' && child.tagName === 'IMG') return child;
        if (selector.startsWith('.') && child.className.split(/\s+/).includes(selector.slice(1))) return child;
        const match = child.querySelector(selector);
        if (match) return match;
      }
      return null;
    },
    scrollIntoView() {}
  };
  node.classList = {
    add(className) { if (!node.className.split(/\s+/).includes(className)) node.className += ' ' + className; },
    remove(className) { node.className = node.className.split(/\s+/).filter(name => name !== className).join(' '); }
  };
  if (tag === 'img') createdImages += 1;
  return node;
}
const target = element('div');
const head = element('head');
const document = {
  head, hidden: false,
  createElement: element,
  getElementById(id) {
    if (id === 'detail-data') return target;
    return head.children.find(child => child.id === id) || null;
  }
};
let selectedBackend = 'default';
const scrollCalls = [];
let sessionListener = null;
const window = {
  VdrSuiteBrowserSession: {
    subscribe(listener) { sessionListener = listener; listener({authenticated: true}); }
  },
  scrollY: 0, scrollX: 0,
  scrollTo(options) { scrollCalls.push(options); this.scrollY = options.top; },
  VdrSuitePlatform: {
    getMountTarget() { return target; },
    getSelectedBackendId() { return selectedBackend; }
  }
};
const context = vm.createContext({window, document, console, Promise, Map});
for (const file of [
  'web/frontend/recordings2-shared.js',
  'web/frontend/recordings2-browser-cards.js',
  'web/frontend/recordings2-browser-view.js'
]) vm.runInContext(fs.readFileSync(file, 'utf8'), context, {filename: file});

function recording(id, url, title) {
  return {
    backendId: selectedBackend, recordingId: id,
    backendNativeId: '/srv/vdr/video/' + id + '.rec',
    path: '/srv/vdr/video/' + id + '.rec', title: title,
    metadata: {presentation: {posterUrl: url, summary: 'Beschreibung'}}
  };
}
function folder(path, recordings) {
  return {
    backendId: selectedBackend, path: path,
    parentPath: '', recordings: recordings, selectedRecording: null,
    data: {recordingFolder: true, path: path, folders: [], recordings: recordings,
      recordingCount: recordings.length, returnedCount: recordings.length,
      totalCount: recordings.length},
    loadingMore: false
  };
}
let current = folder('Komödie', [recording('ace', '/art/ace.jpg', 'Ace Ventura')]);
let selected = null;
const view = window.VdrSuiteRecordings2BrowserView.create({
  getState() { return current; },
  openFolder() {}, selectRecording(entry) { selected = entry; }, closeDetail() {},
  reload() {}, loadMore() {}
});
view.renderFolder();
const comedyRoot = target.children[0];
const acePoster = comedyRoot.querySelector('img');
assert.ok(acePoster, 'the real shared poster owner must create the image');
const imageCount = createdImages;

current.loading = true;
view.renderLoading();
assert.strictEqual(target.children[0], comedyRoot, 'same-folder reload must retain painted DOM');
current.loading = false;
view.renderFolder();
assert.strictEqual(target.children[0], comedyRoot);

window.scrollY = 312;
current = folder('Horror', [recording('horror', '/art/horror.jpg', 'Horror')]);
view.renderLoading();
view.renderFolder();
assert.notStrictEqual(target.children[0], comedyRoot);

current = folder('Komödie', [recording('ace', '/art/ace.jpg', 'Ace Ventura')]);
view.renderLoading();
view.renderFolder();
assert.strictEqual(target.children[0], comedyRoot, 'Komödie -> Horror -> Komödie must reuse the whole folder DOM');
assert.strictEqual(target.children[0].querySelector('img'), acePoster, 'Ace Ventura poster identity must survive');
assert.strictEqual(createdImages, imageCount + 1, 'no redundant img for the revisited folder');
assert.ok(scrollCalls.some(call => call.top === 312), 'returning to the folder restores its scroll position');

const neighbor = recording('neighbor', '/art/neighbor.jpg', 'Nachbar');
current = folder('Komödie', [recording('ace', '/art/ace.jpg', 'Ace Ventura (neu)'), neighbor]);
view.renderFolder();
assert.strictEqual(target.children[0].querySelector('img'), acePoster,
  'new records and changed titles must not reset existing artwork');
assert.strictEqual(createdImages, imageCount + 2, 'only the new Recording needs another img');
target.children[0].querySelector('.recordings2-recording-list').children[0].children[0].click();
assert.strictEqual(selected.title, 'Ace Ventura (neu)',
  'reconciled cards must open the current Recording object');
const neighborPoster = target.children[0].querySelector('.recordings2-recording-list')
  .children[1].querySelector('img');
assert.ok(neighborPoster);

current = folder('Komödie', [recording('ace', '/art/ace-v2.jpg', 'Ace Ventura (neu)'), neighbor]);
view.renderFolder();
const list = target.children[0].querySelector('.recordings2-recording-list');
assert.notStrictEqual(list.children[0].querySelector('img'), acePoster,
  'changed artwork URL must replace only the affected image');
assert.strictEqual(list.children[1].querySelector('img'), neighborPoster,
  'unchanged neighbor poster must survive the same refresh');

view.destroy();
view.renderFolder();
assert.notStrictEqual(target.children[0].querySelector('img'),
  list.children[0].querySelector('img'), 'destroy must drop retained images');

const beforeBackend = target.children[0].querySelector('img');
selectedBackend = 'other';
current = folder('Komödie', [recording('ace', '/art/ace-v2.jpg', 'Ace Ventura')]);
view.renderFolder();
assert.notStrictEqual(target.children[0].querySelector('img'), beforeBackend,
  'backend change must not reuse the former backend DOM');
selectedBackend = 'default';
current = folder('Komödie', [recording('ace', '/art/ace-v2.jpg', 'Ace Ventura')]);
view.renderFolder();
assert.notStrictEqual(target.children[0].querySelector('img'), beforeBackend,
  'returning to a backend after a switch must not recover an old DOM');

const beforeLogout = target.children[0].querySelector('img');
sessionListener({authenticated: false});
view.renderFolder();
assert.notStrictEqual(target.children[0].querySelector('img'), beforeLogout,
  'logout must invalidate retained private DOM');

const visible = target.children[0];
current.error = new Error('permission denied');
view.renderError();
assert.notStrictEqual(target.children[0], visible,
  'a rejected read must show error, not keep displaying a cached folder');
current.error = null;
view.renderFolder();
assert.notStrictEqual(target.children[0].querySelector('img'), beforeLogout,
  'a rejected read must not recover earlier cached artwork');

let oldest = null;
for (let i = 0; i < 5; i += 1) {
  current = folder('Folder-' + i, [recording('movie-' + i, '/art/' + i + '.jpg', 'Film')]);
  view.renderLoading();
  view.renderFolder();
  if (i === 0) oldest = target.children[0].querySelector('img');
}
current = folder('Folder-0', [recording('movie-0', '/art/0.jpg', 'Film')]);
view.renderLoading();
view.renderFolder();
assert.notStrictEqual(target.children[0].querySelector('img'), oldest,
  'the bounded detached-view cache must evict the oldest folder');

console.log('PWA.15.7 recordings folder DOM reuse, changes, isolation and scroll ok');
