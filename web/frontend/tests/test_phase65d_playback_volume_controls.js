'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'playback-volume-controls.js'),
  'utf8'
);

function createRuntime() {
  const observers = [];
  let mediaElementsCreated = 0;
  let startCalls = 0;
  let destroyCalls = 0;
  let assignedFacade = {};

  function matches(node, selector) {
    if (!node) return false;
    if (selector === 'video') return node.tagName === 'VIDEO';
    if (selector.charAt(0) === '.') {
      return String(node.className || '').split(/\s+/).includes(selector.slice(1));
    }
    return false;
  }

  function node(tagName) {
    const listeners = {};
    const value = {
      tagName: String(tagName || '').toUpperCase(),
      children: [],
      className: '',
      style: {},
      textContent: '',
      hidden: false,
      disabled: false,
      value: '',
      min: '',
      max: '',
      step: '',
      type: '',
      title: '',
      parentNode: null,
      attributes: {},
      appendChild(child) {
        if (child.parentNode && typeof child.parentNode.removeChild === 'function') {
          child.parentNode.removeChild(child);
        }
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
      replaceChild(next, previous) {
        const index = this.children.indexOf(previous);
        if (index < 0) throw new Error('previous child missing');
        previous.parentNode = null;
        next.parentNode = this;
        this.children[index] = next;
        observers.forEach(observer => observer.notify());
        return previous;
      },
      replaceWith(next) {
        if (!this.parentNode) return;
        this.parentNode.replaceChild(next, this);
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
      setAttribute(name, val) {
        this.attributes[name] = String(val);
      },
      getAttribute(name) {
        return Object.prototype.hasOwnProperty.call(this.attributes, name)
          ? this.attributes[name]
          : null;
      },
      addEventListener(name, callback) {
        if (!listeners[name]) listeners[name] = [];
        listeners[name].push(callback);
      },
      removeEventListener(name, callback) {
        if (!listeners[name]) return;
        listeners[name] = listeners[name].filter(item => item !== callback);
      },
      dispatch(name) {
        (listeners[name] || []).slice().forEach(callback => callback({target: this}));
      }
    };

    if (value.tagName === 'VIDEO') {
      mediaElementsCreated += 1;
      let volume = 1;
      let muted = false;
      value.ignoreVolumeWrites = false;
      value.ignoreMutedWrites = false;
      Object.defineProperty(value, 'volume', {
        configurable: true,
        get() { return volume; },
        set(next) {
          if (value.ignoreVolumeWrites) return;
          volume = Number(next);
          value.dispatch('volumechange');
        }
      });
      Object.defineProperty(value, 'muted', {
        configurable: true,
        get() { return muted; },
        set(next) {
          if (value.ignoreMutedWrites) return;
          muted = Boolean(next);
          value.dispatch('volumechange');
        }
      });
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
    observe() { this.connected = true; }
    disconnect() { this.connected = false; }
    notify() {
      if (this.connected) this.callback([{type: 'childList'}]);
    }
  }

  const head = node('head');
  const body = node('body');
  const document = {
    head,
    body,
    createElement(tagName) { return node(tagName); },
    createTextNode(text) {
      const value = node('#text');
      value.textContent = String(text);
      return value;
    },
    getElementById(id) {
      return descendants(head).concat(descendants(body))
        .find(item => item.id === id) || null;
    }
  };

  const window = {
    document,
    MutationObserver,
    console,
    Promise
  };
  window.window = window;

  Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
    configurable: true,
    enumerable: true,
    get() { return assignedFacade; },
    set(value) { assignedFacade = value; }
  });

  const context = vm.createContext({
    window,
    document,
    MutationObserver,
    console,
    Object,
    String,
    Number,
    Boolean,
    Promise,
    RegExp,
    Error,
    Math
  });
  vm.runInContext(source, context, {filename: 'playback-volume-controls.js'});

  function basePanel(kind) {
    const element = node('section');
    element.className = kind === 'live' ? 'live-panel' : 'recording-panel';
    const video = node('video');
    element.appendChild(video);
    return Object.freeze({
      element,
      start() { startCalls += 1; return Promise.resolve(kind + '-session'); },
      destroy() { destroyCalls += 1; },
      relinquishForReplacement() { return Promise.resolve(kind + '-session'); },
      sessionId() { return kind + '-session'; },
      state() { return 'playing'; }
    });
  }

  window.VdrSuiteRecordings2Playback = Object.freeze({
    createPanel() { return basePanel('recording'); },
    createLivePanel() { return basePanel('live'); }
  });

  function find(root, className) {
    return descendants(root).find(item => String(item.className || '').split(/\s+/).includes(className));
  }

  return {
    window,
    document,
    observers,
    descendants,
    find,
    node,
    metrics: {
      mediaElementsCreated: () => mediaElementsCreated,
      startCalls: () => startCalls,
      destroyCalls: () => destroyCalls
    }
  };
}

(async function () {
  const runtime = createRuntime();
  const api = runtime.window.VdrSuitePlaybackVolumeControls.__test;

  assert.strictEqual(api.volumeFromPercent(0), 0);
  assert.strictEqual(api.volumeFromPercent(25), 0.25);
  assert.strictEqual(api.volumeFromPercent(100), 1);
  assert.strictEqual(api.volumeFromPercent(-20), 0);
  assert.strictEqual(api.volumeFromPercent(140), 1);
  assert.strictEqual(api.volumeToPercent(0), 0);
  assert.strictEqual(api.volumeToPercent(0.375), 38);
  assert.strictEqual(api.volumeToPercent(1), 100);

  const facade = runtime.window.VdrSuiteRecordings2Playback;
  const recording = facade.createPanel({id: 'recording-1'}, 'default');
  const live = facade.createLivePanel({id: 'S19.2E-1-1011-11100'}, 'default');

  assert.strictEqual(recording.__vdrSuiteVolumeControlsDecorated, true);
  assert.strictEqual(live.__vdrSuiteVolumeControlsDecorated, true);
  assert(runtime.find(recording.element, 'recordings2-volume-controls'));
  assert(runtime.find(live.element, 'recordings2-volume-controls'));
  assert.strictEqual(
    runtime.descendants(recording.element).filter(item => item.tagName === 'VIDEO').length,
    1,
    'volume decorator must not create a second media element'
  );

  const recordingVideo = recording.element.querySelector('video');
  const recordingRange = runtime.find(recording.element, 'recordings2-volume-range');
  const recordingMute = runtime.find(recording.element, 'recordings2-volume-mute');
  const recordingOutput = runtime.find(recording.element, 'recordings2-volume-output');

  recordingRange.value = '35';
  recordingRange.dispatch('input');
  assert.strictEqual(recordingVideo.volume, 0.35);
  assert.strictEqual(recordingOutput.textContent, '35 %');
  assert.strictEqual(runtime.metrics.startCalls(), 0, 'volume change must not restart playback');

  recordingMute.dispatch('click');
  assert.strictEqual(recordingVideo.muted, true);
  assert.strictEqual(recordingMute.textContent, 'Ton an');
  assert.strictEqual(recordingMute.getAttribute('aria-pressed'), 'true');
  assert.strictEqual(runtime.metrics.startCalls(), 0, 'mute must not start or restart playback');

  recordingMute.dispatch('click');
  assert.strictEqual(recordingVideo.muted, false);
  assert.strictEqual(recordingMute.textContent, 'Stumm');

  recordingVideo.volume = 0.62;
  assert.strictEqual(recordingRange.value, '62');
  assert.strictEqual(recordingOutput.textContent, '62 %');

  // HLS/restart replacement: the stable outer volume owner remains while its
  // replaceable transport supplies a fresh HTMLMediaElement.
  recordingMute.dispatch('click');
  assert.strictEqual(recordingVideo.muted, true);
  const oldPresentation = recording.element.children[0];
  const replacement = runtime.node('section');
  replacement.className = 'recordings2-recording-fallback-shell';
  const replacementVideo = runtime.node('video');
  replacement.appendChild(replacementVideo);
  recording.element.replaceChild(replacement, oldPresentation);
  assert.strictEqual(replacementVideo.volume, 0.62, 'replacement video must inherit confirmed owner volume');
  assert.strictEqual(replacementVideo.muted, true, 'replacement video must inherit confirmed owner mute state');
  assert.strictEqual(recordingRange.value, '62');
  assert.strictEqual(recordingMute.getAttribute('aria-pressed'), 'true');

  // Persistent presentation switch: reparenting the owner shell keeps exactly
  // the same media element and therefore cannot reset local volume state.
  const fullView = runtime.node('div');
  const miniPlayer = runtime.node('div');
  fullView.appendChild(live.element);
  const liveVideo = live.element.querySelector('video');
  const liveRange = runtime.find(live.element, 'recordings2-volume-range');
  liveRange.value = '47';
  liveRange.dispatch('input');
  miniPlayer.appendChild(live.element);
  assert.strictEqual(live.element.querySelector('video'), liveVideo);
  assert.strictEqual(liveVideo.volume, 0.47);
  assert.strictEqual(runtime.descendants(live.element).filter(item => item.tagName === 'VIDEO').length, 1);

  // Capability-based fail-safe: if a platform refuses a volume write, the UI
  // reads back the actual element state and disables only that unavailable
  // control. No UA route and no playback/API mutation is involved.
  replacementVideo.ignoreVolumeWrites = true;
  recordingRange.value = '20';
  recordingRange.dispatch('input');
  assert.strictEqual(replacementVideo.volume, 0.62);
  assert.strictEqual(recordingRange.value, '62');
  assert.strictEqual(recordingRange.disabled, true);
  const status = runtime.find(recording.element, 'recordings2-volume-status');
  assert.strictEqual(status.hidden, false);
  assert(status.textContent.includes('Systemlautstärke'));

  assert.strictEqual(runtime.metrics.startCalls(), 0);
  assert.strictEqual(runtime.metrics.mediaElementsCreated(), 3, 'only base/replacement owners may create videos');

  recording.destroy();
  live.destroy();
  assert.strictEqual(runtime.metrics.destroyCalls(), 2);

  console.log('phase65d playback volume controls ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
