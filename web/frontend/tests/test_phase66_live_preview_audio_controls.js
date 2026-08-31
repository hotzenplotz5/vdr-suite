'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const volumeSource = fs.readFileSync(path.join(frontendRoot, 'api', 'playback-volume-controls.js'), 'utf8');
const previewSource = fs.readFileSync(path.join(frontendRoot, 'home-live-preview.js'), 'utf8');
const liveViewSource = fs.readFileSync(path.join(frontendRoot, 'live-tv-view.js'), 'utf8');
const heroSource = fs.readFileSync(path.join(frontendRoot, 'home-live-hero.js'), 'utf8');

// Home keeps one browse-safe muted preview intent. It must not acquire the
// client-local full-player audio controls that belong to explicit playback.
assert(previewSource.includes("ownerIntent: 'preview'"));
assert(previewSource.includes('video.muted = true'));
assert(previewSource.includes('video.controls = false'));
assert(previewSource.includes('video.autoplay = true'));
assert(previewSource.includes('video.playsInline = true'));
assert(previewSource.includes("video.setAttribute('muted', '')"));
assert(previewSource.includes('video.muted = false'));
assert(previewSource.includes('video.controls = true'));
assert(previewSource.includes("video.removeAttribute('muted')"));
assert(heroSource.includes('VdrSuiteLiveTvView'));
assert(heroSource.includes('startChannel(channel)'));
assert(heroSource.includes("watch.setAttribute('data-home-live-action', 'watch')"));
assert(heroSource.includes("epg.setAttribute('data-home-live-action', 'epg')"));

// Entering the real Live-TV view reprojects an already-active shell session
// through the ordinary Live panel factory with no preview intent. Therefore the
// existing Phase-65.D Volume/Mute owner remains available after Watch Live.
assert(liveViewSource.includes("playback.createLivePanel(channel, snapshot.backendId || selectedBackend(), {});"));
assert(liveViewSource.includes("playback.createLivePanel(channel, state.backendId || selectedBackend(), {replacesSessionId: replacesSessionId || ''});"));

function createRuntime() {
  const observers = [];
  let assignedFacade = {};
  let mediaElementsCreated = 0;

  function matches(node, selector) {
    if (!node) return false;
    if (selector === 'video') return node.tagName === 'VIDEO';
    if (selector.charAt(0) === '.') {
      return String(node.className || '').split(/\s+/).includes(selector.slice(1));
    }
    return false;
  }

  function node(tagName) {
    const listeners = Object.create(null);
    const value = {
      tagName: String(tagName || '').toUpperCase(),
      children: [],
      className: '',
      parentNode: null,
      hidden: false,
      disabled: false,
      style: {},
      value: '',
      setAttribute() {},
      appendChild(child) {
        if (child.parentNode && typeof child.parentNode.removeChild === 'function') child.parentNode.removeChild(child);
        child.parentNode = this;
        this.children.push(child);
        return child;
      },
      removeChild(child) {
        const index = this.children.indexOf(child);
        if (index >= 0) this.children.splice(index, 1);
        child.parentNode = null;
        return child;
      },
      querySelector(selector) {
        let result = null;
        (function walk(current) {
          if (!current || result) return;
          if (matches(current, selector)) {
            result = current;
            return;
          }
          (current.children || []).forEach(walk);
        }(this));
        return result;
      },
      addEventListener(name, callback) {
        (listeners[name] ||= []).push(callback);
      },
      removeEventListener(name, callback) {
        if (!listeners[name]) return;
        listeners[name] = listeners[name].filter(item => item !== callback);
      },
      listenerCount(name) { return (listeners[name] || []).length; }
    };
    if (value.tagName === 'VIDEO') {
      mediaElementsCreated += 1;
      value.volume = 1;
      value.muted = false;
    }
    return value;
  }

  function descendants(root) {
    const values = [];
    (function walk(current) {
      if (!current) return;
      values.push(current);
      (current.children || []).forEach(walk);
    }(root));
    return values;
  }

  class MutationObserver {
    constructor(callback) {
      this.callback = callback;
      this.connected = false;
      observers.push(this);
    }
    observe(target, options) {
      this.target = target;
      this.options = options;
      this.connected = true;
    }
    disconnect() { this.connected = false; }
  }

  const head = node('head');
  const document = {
    head,
    createElement: node,
    createTextNode(text) {
      const value = node('#text');
      value.textContent = String(text);
      return value;
    },
    getElementById(id) {
      return descendants(head).find(item => item.id === id) || null;
    }
  };
  const window = {document, MutationObserver, console, Promise};
  window.window = window;

  Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
    configurable: true,
    enumerable: true,
    get() { return assignedFacade; },
    set(value) { assignedFacade = value; }
  });

  vm.runInContext(volumeSource, vm.createContext({
    window,
    document,
    MutationObserver,
    console,
    Promise,
    Object,
    String,
    Number,
    Boolean,
    RegExp,
    Error,
    Math
  }), {filename: 'playback-volume-controls.js'});

  function basePanel(kind) {
    const element = node('section');
    element.className = kind + '-panel';
    const video = node('video');
    element.appendChild(video);
    return Object.freeze({
      kind,
      element,
      start() { return Promise.resolve(kind + '-session'); },
      destroy() {},
      relinquishForReplacement() { return Promise.resolve(kind + '-session'); }
    });
  }

  window.VdrSuiteRecordings2Playback = Object.freeze({
    createPanel() { return basePanel('recording'); },
    createLivePanel() { return basePanel('live'); }
  });

  function find(root, className) {
    return descendants(root).find(item => String(item.className || '').split(/\s+/).includes(className));
  }

  return {window, observers, find, descendants, mediaElementsCreated: () => mediaElementsCreated};
}

const runtime = createRuntime();
const facade = runtime.window.VdrSuiteRecordings2Playback;

const preview = facade.createLivePanel({id: 'C1'}, 'backend-a', {ownerIntent: 'preview'});
const previewVideo = preview.element.querySelector('video');
assert(previewVideo, 'preview must keep the canonical Live media element');
assert.strictEqual(preview.__vdrSuiteVolumeControlsDecorated, undefined, 'preview must not get full-player audio decoration');
assert.strictEqual(runtime.find(preview.element, 'recordings2-volume-owner-shell'), undefined, 'preview must not reserve a full-player audio shell');
assert.strictEqual(runtime.find(preview.element, 'recordings2-volume-controls'), undefined, 'preview must contain no Volume/Mute control area');
assert.strictEqual(runtime.find(preview.element, 'recordings2-volume-mute'), undefined, 'preview must contain no mute button');
assert.strictEqual(runtime.find(preview.element, 'recordings2-volume-range'), undefined, 'preview must contain no volume slider');
assert.strictEqual(previewVideo.listenerCount('volumechange'), 0, 'preview must not bind full-player Volume/Mute state listeners');
assert.strictEqual(runtime.observers.length, 0, 'preview must not install the full-player audio observer');

const fullLive = facade.createLivePanel({id: 'C1'}, 'backend-a', {});
assert.strictEqual(fullLive.__vdrSuiteVolumeControlsDecorated, true, 'explicit Live-TV must retain normal audio controls');
assert(runtime.find(fullLive.element, 'recordings2-volume-controls'));
assert(runtime.find(fullLive.element, 'recordings2-volume-mute'));
assert(runtime.find(fullLive.element, 'recordings2-volume-range'));
assert.strictEqual(fullLive.element.querySelector('video').listenerCount('volumechange'), 1, 'explicit Live-TV must retain client-local audio state binding');
assert.strictEqual(runtime.observers.length, 1, 'explicit Live-TV must retain its stable audio owner observer');

const recording = facade.createPanel({id: 'R1'}, 'backend-a');
assert.strictEqual(recording.__vdrSuiteVolumeControlsDecorated, true, 'Recording playback audio controls must remain unchanged');
assert(runtime.find(recording.element, 'recordings2-volume-controls'));
assert.strictEqual(runtime.mediaElementsCreated(), 3, 'the follow-up must not create a second media element');

console.log('phase66 Home Live preview audio-control boundary ok');
