'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const sessionSource = fs.readFileSync(path.join(frontendRoot, 'api', 'session-frontend-sync.js'), 'utf8');
const fallbackSource = fs.readFileSync(path.join(frontendRoot, 'api', 'recording-fallback-controls.js'), 'utf8');
const continueSource = fs.readFileSync(path.join(frontendRoot, 'api', 'continue-watching-sync.js'), 'utf8');

const allElements = [];

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
  const node = {
    tagName: String(tagName || '').toUpperCase(),
    children: [],
    style: {},
    dataset: {},
    className: '',
    textContent: '',
    hidden: false,
    disabled: false,
    type: '',
    title: '',
    value: '',
    min: '',
    max: '',
    step: '',
    currentTime: 0,
    duration: 0,
    paused: true,
    controls: true,
    parentNode: null,
    firstChild: null,
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
    replaceWith(replacement) {
      this.replacement = replacement;
      if (this.parentNode && typeof this.parentNode.replaceChild === 'function') {
        this.parentNode.replaceChild(replacement, this);
      }
    },
    replaceChild(replacement, current) {
      const index = this.children.indexOf(current);
      if (index >= 0) {
        replacement.parentNode = this;
        current.parentNode = null;
        this.children[index] = replacement;
        this.firstChild = this.children[0] || null;
      }
    },
    setAttribute(name, value) { this[name] = String(value); },
    removeAttribute(name) { delete this[name]; },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    removeEventListener(name, callback) {
      if (!listeners[name]) return;
      listeners[name] = listeners[name].filter(value => value !== callback);
    },
    dispatch(name, event) {
      (listeners[name] || []).slice().forEach(callback => callback(event || {target: this}));
    },
    click() { this.dispatch('click', {target: this}); },
    play() {
      this.paused = false;
      this.dispatch('play', {target: this});
      this.dispatch('playing', {target: this});
      return Promise.resolve();
    },
    pause() {
      this.paused = true;
      this.dispatch('pause', {target: this});
    },
    load() {},
    querySelector(selector) {
      const all = descendants(this);
      if (selector === 'video') return all.find(value => value.tagName === 'VIDEO') || null;
      if (selector === 'button.recordings2-primary') {
        return all.find(value => value.tagName === 'BUTTON' && value.className === 'recordings2-primary') || null;
      }
      return null;
    },
    querySelectorAll(selector) {
      const all = descendants(this);
      if (selector === 'video, audio') {
        return all.filter(value => value.tagName === 'VIDEO' || value.tagName === 'AUDIO');
      }
      return [];
    }
  };
  allElements.push(node);
  return node;
}

function createLifecycle(initial) {
  let revision = 0;
  let current = Object.assign({
    lifecycleVersion: 1,
    lifecycleRevision: 0,
    transition: 'snapshot'
  }, initial || {});
  const listeners = [];
  return {
    snapshot() { return Object.assign({}, current); },
    publish(change) {
      revision += 1;
      current = Object.assign({}, current, change || {}, {lifecycleRevision: revision});
      listeners.slice().forEach(listener => listener(Object.assign({}, current)));
      return Object.assign({}, current);
    },
    subscribe(listener) {
      listeners.push(listener);
      listener(Object.assign({}, current, {transition: 'snapshot'}));
      return function () {
        const index = listeners.indexOf(listener);
        if (index >= 0) listeners.splice(index, 1);
      };
    },
    clear() { listeners.length = 0; }
  };
}

function flush(count = 20) {
  let promise = Promise.resolve();
  for (let index = 0; index < count; index += 1) {
    promise = promise.then(() => Promise.resolve());
  }
  return promise;
}

const suiteWrites = [];
const mediaRequests = [];
let hlsSequence = 0;
const document = {
  readyState: 'complete',
  visibilityState: 'visible',
  createElement: element,
  addEventListener() {},
  querySelector() { return null; }
};

const window = {
  window: null,
  document,
  console,
  Object,
  Number,
  String,
  Math,
  Promise,
  Error,
  Array,
  Date,
  performance: {now() { return 0; }},
  VdrSuiteBrowserSession: {
    subscribe() { return function () {}; },
    csrfHeaders() { return {'X-VDR-Suite-CSRF': 'phase66-fallback-stop'}; }
  },
  VdrSuitePlaybackOwnerLifecycle: {create: createLifecycle},
  addEventListener() {},
  removeEventListener() {},
  setTimeout(callback, delay) {
    if (Number(delay) === 0) Promise.resolve().then(callback);
    return 1;
  },
  clearTimeout() {},
  MutationObserver: class {
    observe() {}
    disconnect() {}
  },
  fetch(requestPath, options) {
    if (requestPath === '/api/media/continue-watching' || requestPath === '/api/media/recently-watched') {
      suiteWrites.push({path: requestPath, body: JSON.parse(options.body)});
      return Promise.resolve({ok: true});
    }
    return Promise.resolve({ok: true});
  },
  VdrSuiteClientApi: {
    requestJson(requestPath, options) {
      const body = JSON.parse(options.body);
      mediaRequests.push({path: requestPath, body});
      if (body.operation === 'stop') return Promise.resolve({ok: true});
      if (body.operation === 'playback-status') {
        return Promise.resolve({mediaSession: {
          id: body.sessionId,
          state: 'ready',
          presentationProfileId: 'hls-fmp4',
          mediaPath: '/api/media/sessions/' + body.sessionId + '/hls/master.m3u8',
          playback: {
            positionSeconds: 0,
            durationSeconds: 7200,
            seek: {supported: false, preparing: false},
            resume: {supported: true, preparing: false}
          }
        }});
      }

      const protocols = body.capabilities && body.capabilities.protocols || [];
      if (protocols.indexOf('progressive') >= 0) {
        if (body.recordingId === '1917') {
          return Promise.resolve({mediaSession: {
            id: 'fast-1917',
            state: 'ready',
            presentationProfileId: 'progressive-fmp4',
            mediaPath: '/api/media/sessions/fast-1917/recording/stream.mp4',
            playback: {
              positionSeconds: 0,
              durationSeconds: 7140,
              seek: {supported: true, preparing: false, window: {startSeconds: 0, endSeconds: 7140}},
              resume: {supported: true, preparing: false}
            }
          }});
        }
        // This is the Being-John-Malkovich-style transport mismatch: the fast
        // owner receives a valid session, but not the progressive profile/path
        // it can own, so production must replace it with the HLS child owner.
        return Promise.resolve({mediaSession: {
          id: 'fast-provisional-malkovich',
          state: 'ready',
          presentationProfileId: 'hls-fmp4',
          mediaPath: '/api/media/sessions/fast-provisional-malkovich/hls/master.m3u8',
          playback: {
            positionSeconds: 0,
            durationSeconds: 7200,
            seek: {supported: false, preparing: false},
            resume: {supported: true, preparing: false}
          }
        }});
      }

      hlsSequence += 1;
      const hlsId = 'hls-malkovich-' + hlsSequence;
      return Promise.resolve({mediaSession: {
        id: hlsId,
        state: 'ready',
        presentationProfileId: 'hls-fmp4',
        mediaPath: '/api/media/sessions/' + hlsId + '/hls/master.m3u8',
        playback: {
          positionSeconds: Number(body.startPositionSeconds) || 0,
          durationSeconds: 7200,
          seek: {supported: false, preparing: false},
          resume: {supported: true, preparing: false}
        }
      }});
    }
  }
};
window.window = window;

const context = vm.createContext({
  window,
  document,
  console,
  Object,
  Number,
  String,
  Math,
  Promise,
  Error,
  Array,
  Date,
  setTimeout: window.setTimeout,
  clearTimeout: window.clearTimeout,
  MutationObserver: window.MutationObserver,
  fetch: window.fetch
});

// Production composition order: persistent Fast owner first, HLS fallback
// controls decorate the later legacy factory assignment, Continue Watching then
// observes the final canonical owner exposed by the same descriptor chain.
vm.runInContext(sessionSource, context, {filename: 'session-frontend-sync.js'});
vm.runInContext(fallbackSource, context, {filename: 'recording-fallback-controls.js'});
vm.runInContext(continueSource, context, {filename: 'continue-watching-sync.js'});

window.VdrSuiteRecordings2Playback = {
  createPanel(recording, backendId, options) {
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
        return Promise.resolve(options.createSession()).then(session => {
          sessionId = session.mediaSession.id;
          video.paused = false;
          video.dispatch('play');
          video.dispatch('playing');
          return sessionId;
        });
      },
      destroy() {
        sessionId = '';
        video.paused = true;
      },
      sessionId() { return sessionId; }
    };
  }
};

function findLatest(predicate) {
  for (let index = allElements.length - 1; index >= 0; index -= 1) {
    if (predicate(allElements[index])) return allElements[index];
  }
  return null;
}

function progressWrite(recordingId) {
  return suiteWrites.filter(entry =>
    entry.path === '/api/media/continue-watching' &&
    entry.body.operation === 'progress' &&
    entry.body.recordingId === recordingId
  ).pop() || null;
}

(async function () {
  const factory = window.VdrSuiteRecordings2Playback;
  const malkovich = factory.createPanel({id: 'being-john-malkovich'}, 'default');
  assert.ok(malkovich && typeof malkovich.subscribe === 'function');
  assert.strictEqual(await malkovich.start(), 'hls-malkovich-1');
  await flush();
  assert.strictEqual(malkovich.snapshot().transport, 'hls-compatibility');

  const hlsVideo = findLatest(value => value.tagName === 'VIDEO');
  assert.ok(hlsVideo, 'HLS compatibility video must exist after transport replacement');
  hlsVideo.currentTime = 125;
  hlsVideo.dispatch('timeupdate');
  assert.strictEqual(malkovich.position(), 125, 'outer owner must project HLS absolute position');
  assert.strictEqual(malkovich.duration(), 7200, 'outer owner must project HLS duration');
  assert.strictEqual(malkovich.canResume(), true, 'outer owner must project HLS resume truth');

  const stopButton = findLatest(value => value.tagName === 'BUTTON' && value.textContent === 'Stop');
  assert.ok(stopButton, 'production HLS shell must expose the Stop action');
  stopButton.click();
  await flush(40);

  const stoppedWrite = progressWrite('being-john-malkovich');
  assert.ok(stoppedWrite, 'partially watched HLS fallback recording must persist Continue Watching on Stop');
  assert.strictEqual(stoppedWrite.body.positionSeconds, 125);
  assert.strictEqual(stoppedWrite.body.resumeSupported, true);
  assert.strictEqual(malkovich.position(), 125, 'stopped HLS truth must stay projected after transport destruction');
  assert.strictEqual(malkovich.canResume(), true);

  const historyWrites = suiteWrites.filter(entry =>
    entry.path === '/api/media/recently-watched' &&
    entry.body.recordingId === 'being-john-malkovich'
  );
  assert.ok(historyWrites.length >= 1, 'History remains independently persisted');
  const stoppedHistory = historyWrites[historyWrites.length - 1].body;
  assert.strictEqual(stoppedHistory.positionSeconds, 125);
  assert.strictEqual(stoppedHistory.resumeSupported, true);
  assert.strictEqual(stoppedHistory.ended, false);

  // Countercase for an already-working progressive recording such as 1917.
  const progressive = factory.createPanel({id: '1917'}, 'default');
  assert.strictEqual(await progressive.start(), 'fast-1917');
  await flush();
  const progressiveVideo = findLatest(value =>
    value.tagName === 'VIDEO' && value['aria-label'] === 'VDR-Aufnahme'
  );
  assert.ok(progressiveVideo);
  progressiveVideo.currentTime = 94;
  progressiveVideo.dispatch('timeupdate');
  await progressive.stop();
  await flush(40);
  const progressiveWrite = progressWrite('1917');
  assert.ok(progressiveWrite, 'existing progressive Continue Watching path must remain intact');
  assert.strictEqual(progressiveWrite.body.positionSeconds, 94);
  assert.strictEqual(progressiveWrite.body.resumeSupported, true);

  const provisionalStop = mediaRequests.find(entry =>
    entry.body.operation === 'stop' && entry.body.sessionId === 'fast-provisional-malkovich'
  );
  assert.ok(provisionalStop, 'transport replacement must still clean up the provisional fast session');

  console.log('phase66 fallback Stop persists Continue Watching and preserves progressive path');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
