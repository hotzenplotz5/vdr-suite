'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const restartSeekSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-fallback-restart-seek.js'),
  'utf8'
);
const fallbackSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-fallback-controls.js'),
  'utf8'
);

function descendants(root) {
  const result = [];
  (function walk(value) {
    if (!value) return;
    result.push(value);
    (value.children || []).forEach(walk);
  }(root));
  return result;
}

function element(tagName) {
  const listeners = {};
  let disabled = false;
  const node = {
    tagName: String(tagName || '').toUpperCase(),
    children: [], style: {}, dataset: {}, className: '', textContent: '',
    hidden: false, focused: false, type: '', title: '', value: '', min: '', max: '', step: '',
    currentTime: 0, paused: true, controls: true, firstChild: null, parentNode: null,
    classList: {toggle() {}},
    appendChild(child) {
      child.parentNode = this;
      this.children.push(child);
      this.firstChild = this.children[0] || null;
      return child;
    },
    replaceChildren() {
      this.children = [];
      Array.from(arguments).forEach(child => this.appendChild(child));
      this.firstChild = this.children[0] || null;
    },
    removeChild(child) {
      this.children = this.children.filter(value => value !== child);
      this.firstChild = this.children[0] || null;
    },
    setAttribute(name, value) { this[name] = String(value); },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    dispatch(name, event) {
      const payload = event || {target: this};
      (listeners[name] || []).forEach(callback => callback(payload));
    },
    click() { this.dispatch('click', {target: this}); },
    focus() { if (!disabled) this.focused = true; },
    play() { this.paused = false; this.dispatch('play', {target: this}); return Promise.resolve(); },
    pause() { this.paused = true; this.dispatch('pause', {target: this}); },
    querySelector(selector) {
      const all = descendants(this);
      if (selector === 'video') return all.find(value => value.tagName === 'VIDEO') || null;
      if (selector === 'button.recordings2-primary') {
        return all.find(value => value.tagName === 'BUTTON' && value.className === 'recordings2-primary') || null;
      }
      if (selector.charAt(0) === '.') {
        const className = selector.slice(1);
        return all.find(value => String(value.className || '').split(/\s+/).includes(className)) || null;
      }
      const aria = selector.match(/^(button|input)\[aria-label="([^"]+)"\]$/);
      if (aria) {
        return all.find(value =>
          value.tagName === aria[1].toUpperCase() && value['aria-label'] === aria[2]
        ) || null;
      }
      return null;
    }
  };
  Object.defineProperty(node, 'disabled', {
    enumerable: true,
    configurable: true,
    get() { return disabled; },
    set(value) {
      disabled = Boolean(value);
      if (disabled) node.focused = false;
    }
  });
  return node;
}

function flush() {
  let chain = Promise.resolve();
  for (let index = 0; index < 12; index += 1) chain = chain.then(() => Promise.resolve());
  return chain;
}

const requests = [];
let createSequence = 0;
const document = {createElement: element};
const window = {
  document, console, Object, Number, String, Math, Promise, Error, Array,
  VdrSuiteBrowserSession: {csrfHeaders() { return {'X-CSRF-Token': 'csrf-test'}; }},
  VdrSuiteClientApi: {
    requestJson(requestPath, options) {
      const body = JSON.parse(options.body);
      requests.push({path: requestPath, body});
      if (body.operation === 'playback-status') {
        return Promise.resolve({mediaSession: {
          id: body.sessionId,
          state: 'ready',
          playback: {
            positionSeconds: 0,
            durationSeconds: 7134,
            seek: {supported: false, preparing: false},
            resume: {supported: true, preparing: false}
          }
        }});
      }
      createSequence += 1;
      const start = Number(body.startPositionSeconds) || 0;
      return Promise.resolve({mediaSession: {
        id: 'hls-session-' + createSequence,
        state: 'ready',
        presentationProfileId: 'hls-fmp4',
        mediaPath: '/api/media/sessions/hls-session-' + createSequence + '/hls/master.m3u8',
        playback: {
          positionSeconds: start,
          durationSeconds: 7134,
          seek: {supported: false, preparing: false},
          resume: {supported: true, preparing: false}
        }
      }});
    }
  },
  setTimeout() { return 1; },
  clearTimeout() {}
};
window.window = window;

let currentPlayback = {};
Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
  configurable: true, enumerable: true,
  get() { return currentPlayback; },
  set(value) { currentPlayback = value; }
});

const context = vm.createContext({window, document, console, Object, Number, String, Math, Promise, Error, Array});
vm.runInContext(restartSeekSource, context, {filename: 'recording-fallback-restart-seek.js'});
vm.runInContext(fallbackSource, context, {filename: 'recording-fallback-controls.js'});

assert.strictEqual(window.__vdrSuiteRecordingFallbackRestartSeekBound, true);
assert.strictEqual(window.VdrSuiteRecordingFallbackRestartSeek.__test.parseTime('01:02:03'), 3723);

let creates = 0;
let starts = 0;
let destroys = 0;
window.VdrSuiteRecordings2Playback = {
  createPanel(recording, backendId, options) {
    creates += 1;
    assert.ok(options && typeof options.createSession === 'function');
    const panel = element('section');
    const startButton = element('button');
    startButton.className = 'recordings2-primary';
    panel.appendChild(startButton);
    const video = element('video');
    panel.appendChild(video);
    let sessionId = '';
    return {
      element: panel,
      start() {
        starts += 1;
        return Promise.resolve(options.createSession()).then(session => {
          sessionId = session.mediaSession.id;
          video.paused = false;
          video.dispatch('play', {target: video});
          return sessionId;
        });
      },
      destroy() { destroys += 1; sessionId = ''; video.paused = true; },
      sessionId() { return sessionId; }
    };
  }
};

(async function () {
  const factory = window.VdrSuiteRecordings2Playback;
  assert.strictEqual(factory.__vdrSuiteFallbackRestartSeekDecorated, true);
  const playback = factory.createPanel({id: 'recording-42'}, 'default');
  assert.ok(playback && playback.element);
  assert.strictEqual(typeof playback.seekAbsolute, 'function');
  assert.strictEqual(typeof playback.seekRelative, 'function');

  const back10Button = playback.element.querySelector('button[aria-label="10 Sekunden zurück"]');
  const forward60Button = playback.element.querySelector('button[aria-label="60 Sekunden vor"]');
  const timeline = playback.element.querySelector('input[aria-label="Wiedergabeposition"]');
  const directTime = playback.element.querySelector('input[aria-label="Direkte Wiedergabezeit"]');
  const directButton = playback.element.querySelector('button[aria-label="Zur eingegebenen Wiedergabezeit springen"]');
  const positionLabel = playback.element.querySelector('.recordings2-playback-position');
  assert.ok(back10Button && forward60Button && timeline && directTime && directButton && positionLabel);
  assert.strictEqual(timeline.disabled, true, 'restart-seek stays closed before playback is active');
  assert.strictEqual(
    timeline.style.minHeight,
    '2.75rem',
    'compatibility timeline must expose the same mobile-sized touch target as the accepted volume range'
  );
  assert.strictEqual(
    timeline.style.touchAction,
    'pan-y',
    'compatibility timeline must reserve horizontal drag for the native range control on touch browsers'
  );

  assert.strictEqual(await playback.start(), 'hls-session-1');
  await flush();
  assert.strictEqual(playback.canResume(), true);
  assert.strictEqual(playback.duration(), 7134);
  assert.strictEqual(timeline.disabled, false, 'indexed HLS restart-seek must enable the timeline');
  assert.strictEqual(forward60Button.disabled, false);
  assert.strictEqual(directButton.disabled, false);
  assert.strictEqual(timeline.max, '7133');

  let video = playback.element.querySelector('video');
  directTime.focus();
  assert.strictEqual(directTime.focused, true, 'direct seek input must accept focus when restart-seek is available');
  video.currentTime = 120;
  video.dispatch('timeupdate', {target: video});
  assert.strictEqual(playback.position(), 120);
  assert.strictEqual(directTime.disabled, false);
  assert.strictEqual(
    directTime.focused,
    true,
    'fallback timeupdate must not transiently disable and blur the restart-seek input'
  );

  assert.strictEqual(await playback.seekRelative(60), true);
  assert.strictEqual(creates, 2, 'HLS seek must create a fresh transport owner');
  assert.strictEqual(destroys, 1, 'HLS seek must release the previous transport owner');
  let createRequests = requests.filter(entry => !entry.body.operation);
  assert.strictEqual(createRequests[1].body.startPositionSeconds, 180);
  assert.strictEqual(playback.position(), 180);

  video = playback.element.querySelector('video');
  video.currentTime = 5;
  video.dispatch('timeupdate', {target: video});
  back10Button.click();
  await flush();
  createRequests = requests.filter(entry => !entry.body.operation);
  assert.strictEqual(createRequests[2].body.startPositionSeconds, 175, '−10 must use absolute Recording time');

  directTime.value = '00:10:00';
  directButton.click();
  await flush();
  createRequests = requests.filter(entry => !entry.body.operation);
  assert.strictEqual(createRequests[3].body.startPositionSeconds, 600, 'direct time seek must restart at requested time');

  // Real Android regression: while the user drags the range input, playback
  // continues to emit timeupdate. That event must not snap the thumb back to
  // the current playback position before the range control emits change.
  timeline.value = '1200';
  timeline.dispatch('input', {target: timeline});
  assert.strictEqual(positionLabel.textContent, '00:20:00 / 01:58:54');
  video = playback.element.querySelector('video');
  video.currentTime = 12;
  video.dispatch('timeupdate', {target: video});
  assert.strictEqual(
    timeline.value,
    '1200',
    'timeupdate must not overwrite the user-selected timeline value while dragging'
  );
  assert.strictEqual(
    positionLabel.textContent,
    '00:20:00 / 01:58:54',
    'timeupdate must preserve the timeline preview while dragging'
  );
  timeline.dispatch('change', {target: timeline});
  await flush();
  createRequests = requests.filter(entry => !entry.body.operation);
  assert.strictEqual(createRequests[4].body.startPositionSeconds, 1200, 'timeline change must restart HLS at absolute time');
  assert.strictEqual(playback.position(), 1200);
  assert.strictEqual(starts, 5);

  console.log('phase65d2 HLS fallback restart-seek controls and focus ownership ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
