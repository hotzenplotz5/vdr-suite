'use strict';

const assert = require('assert').strict;
const fs = require('fs');
const vm = require('vm');

const viewSource = fs.readFileSync('web/frontend/recordings2-browser-view.js', 'utf8');
const heroSource = fs.readFileSync('web/frontend/recordings2-hero-detail.js', 'utf8');
const prewarmSource = fs.readFileSync('web/frontend/recordings2-hero-prewarm.js', 'utf8');

function element(tagName) {
  const listeners = {};
  return {
    tagName: String(tagName).toUpperCase(), dataset: {}, children: [],
    className: '', textContent: '', hidden: false, isConnected: true,
    classList: {add() {}, remove() {}},
    style: {setProperty() {}},
    appendChild(child) { this.children.push(child); child.parentNode = this; return child; },
    replaceChildren() {
      this.children.forEach(child => { child.isConnected = false; });
      this.children = [];
      Array.from(arguments).forEach(child => this.appendChild(child));
    },
    querySelector() { return null; },
    querySelectorAll() { return []; },
    setAttribute() {}, scrollTo() {}, scrollIntoView() {},
    addEventListener(type, callback) { listeners[type] = callback; },
    click() { if (listeners.click) listeners.click(); },
    remove() { this.isConnected = false; }
  };
}

const mount = element('div');
const scheduled = [];
const owners = [];
let stoppedCount = 0;
const stopSequence = [];
const document = {
  createElement: element,
  getElementById() { return null; },
  head: {appendChild() {}}
};
const shared = {
  mountTarget() { return mount; },
  installStyles() {},
  node(tag, className, text) {
    const result = element(tag);
    result.className = className || '';
    result.textContent = text == null ? '' : String(text);
    return result;
  },
  createButton(title, callback) {
    const result = element('button');
    result.textContent = title;
    result.addEventListener('click', callback);
    return result;
  },
  createPoster() { return element('img'); },
  first(obj, keys, fallback) {
    for (const key of keys) if (obj && obj[key] != null) return obj[key];
    return fallback;
  },
  selectedBackendId() { return 'default'; },
  text(value) { return value == null ? '' : String(value); },
  recordingTitle(recording) { return recording.title || ''; },
  recordingSubtitle() { return ''; },
  recordingSummary() { return ''; },
  formatStart() { return ''; },
  formatDuration() { return ''; },
  formatSize() { return ''; },
  provider() { return {}; },
  clientApi() { return null; },
  number(value) { return Number(value) || 0; }
};

const window = {
  document, VdrSuiteRecordings2Shared: shared,
  VdrSuiteRecordingPlaybackRestartChoice: {install() {}},
  VdrSuiteRecordings2Playback: {
    createPanel() {
      const player = {
        element: element('div'), playing: true, destroyed: false, starts: 0,
        stop() {
          stopSequence.push('stop');
          this.playing = false;
          return Promise.resolve(true);
        },
        destroy() {
          stopSequence.push('destroy');
          this.destroyed = true; this.playing = false; stoppedCount += 1;
        },
        startAtAbsolute() { this.starts += 1; return Promise.resolve('playing'); },
        play() { this.starts += 1; return Promise.resolve(true); }
      };
      owners.push(player);
      return player;
    }
  },
  setTimeout(callback) { scheduled.push(callback); return scheduled.length; }
};
window.window = window;
const context = vm.createContext({window, document, console, Promise, Number, String, Math, Object});
vm.runInContext(viewSource, context, {filename: 'recordings2-browser-view.js'});
vm.runInContext(heroSource, context, {filename: 'recordings2-hero-detail.js'});
vm.runInContext(prewarmSource, context, {filename: 'recordings2-hero-prewarm.js'});

const state = {
  selectedRecording: {id: 'medicus2', title: 'Der Medicus II', backendId: 'default'},
  backendId: 'default', path: ''
};
const view = window.VdrSuiteRecordings2BrowserView.create({
  getState() { return state; }
});
const hero = window.VdrSuiteRecordings2HeroDetail.__test;
view.renderDetail();
assert.equal(owners.length, 1);
const initialRoot = mount.children[0];
assert.equal(initialRoot.__vdrSuiteRecordingPlaybackOwner, owners[0]);
assert.equal(initialRoot.dataset.recordings2HeroMode, 'detail',
  'the actual browser view must show the Hero before the metadata addon loads');
assert(initialRoot.children.some(child =>
  child.className.includes('recordings2-hero-recording-panel')),
  'the first frame must have the final Hero recording panel');

hero.showMode(initialRoot, 'playback');
assert.equal(initialRoot.dataset.recordings2HeroMode, 'playback');
const backButton = initialRoot.children.find(child =>
  child.className === 'recordings2-hero-mode-back');
assert(backButton, 'the real Details button must be present');
backButton.click();
assert.equal(stoppedCount, 1, 'Back to Details must destroy active media owner');
assert.deepEqual(stopSequence, ['stop', 'destroy'],
  'Details must commit a canonical Stop checkpoint BEFORE tearing down the media owner');
assert.equal(owners[0].playing, false, 'hidden playback may not continue playing');
assert.equal(owners.length, 2, 'returning to Details must prepare a fresh usable owner');
assert.notEqual(mount.children[0], initialRoot);
assert.equal(mount.children[0].__vdrSuiteRecordingPlaybackOwner, owners[1]);
assert.equal(owners[1].destroyed, false);
assert.equal(owners[1].starts, 0, 'reopening Details must not autoplay');

const nextRoot = mount.children[0];
hero.showMode(nextRoot, 'playback');
const oldTimeouts = scheduled.length;
hero.playPrepared(nextRoot, {resumePositionSeconds: 120});
assert(scheduled.length > oldTimeouts, 'prepared playback schedules its promotion');
hero.showMode(nextRoot, 'detail');
assert.equal(stoppedCount, 2);
assert.deepEqual(stopSequence, ['stop', 'destroy', 'stop', 'destroy']);
assert.equal(owners.length, 3);
assert.equal(nextRoot.isConnected, false);
while (scheduled.length) scheduled.shift()();
Promise.resolve().then(function () {
  assert.equal(owners[1].starts, 0,
    'stale prewarm callback must not restart the destroyed hidden owner');
  assert.equal(owners[2].destroyed, false);
  console.log('Recording Hero return-to-Details stops playback and cancels stale prewarm PASS');
}).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
