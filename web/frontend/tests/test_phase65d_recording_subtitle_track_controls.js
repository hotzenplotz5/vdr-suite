'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-track-controls.js'),
  'utf8'
);

function descendants(root) {
  const values = [];
  (function walk(item) {
    if (!item) return;
    values.push(item);
    (item.children || []).forEach(walk);
  }(root));
  return values;
}

function node(tagName) {
  const listeners = {};
  const upper = String(tagName || '').toUpperCase();
  const value = {
    tagName: upper,
    children: [],
    className: '',
    classList: {toggle(name, enabled) { this[name] = Boolean(enabled); }},
    style: {},
    textContent: '',
    hidden: false,
    disabled: false,
    value: '',
    selected: false,
    parentNode: null,
    currentTime: upper === 'VIDEO' ? 0 : undefined,
    track: upper === 'TRACK' ? {mode: 'disabled'} : undefined,
    appendChild(child) {
      if (child.parentNode && typeof child.parentNode.removeChild === 'function') {
        child.parentNode.removeChild(child);
      }
      child.parentNode = this;
      this.children.push(child);
      if (this.tagName === 'SELECT' && child.selected) this.value = child.value;
      return child;
    },
    replaceChildren() {
      this.children.forEach(child => { child.parentNode = null; });
      this.children = [];
      if (this.tagName === 'SELECT') this.value = '';
    },
    removeChild(child) {
      const index = this.children.indexOf(child);
      if (index >= 0) this.children.splice(index, 1);
      child.parentNode = null;
      return child;
    },
    replaceWith(next) {
      if (!this.parentNode) return;
      const parent = this.parentNode;
      const index = parent.children.indexOf(this);
      if (index < 0) return;
      if (next.parentNode && typeof next.parentNode.removeChild === 'function') {
        next.parentNode.removeChild(next);
      }
      parent.children[index] = next;
      next.parentNode = parent;
      this.parentNode = null;
    },
    querySelector(selector) {
      if (selector === 'video') {
        return descendants(this).find(item => item !== this && item.tagName === 'VIDEO') || null;
      }
      if (selector === '.recordings2-recording-fallback-shell') {
        return descendants(this).find(item =>
          item !== this &&
          String(item.className || '').split(/\s+/).includes('recordings2-recording-fallback-shell')
        ) || null;
      }
      return null;
    },
    setAttribute(name, val) { this[name] = String(val); },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    dispatch(name) {
      (listeners[name] || []).forEach(callback => callback({target: this}));
    }
  };
  Object.defineProperty(value, 'firstChild', {
    get() { return this.children.length ? this.children[0] : null; }
  });
  return value;
}

function find(root, predicate) {
  return descendants(root).find(predicate);
}

function flush(count = 12) {
  let promise = Promise.resolve();
  for (let index = 0; index < count; index += 1) promise = promise.then(() => Promise.resolve());
  return promise;
}

function trackContract(audioTrackId, subtitleTrackId) {
  return {
    audio: {
      selectionSupported: true,
      selectionReason: null,
      selectedTrackId: audioTrackId,
      defaultTrackId: 'audio-1',
      availableTracks: [
        {id: 'audio-1', language: 'ger', codec: 'aac', channels: 2, layout: 'stereo', roles: [], default: true},
        {id: 'audio-2', language: 'eng', codec: 'aac', channels: 2, layout: 'stereo', roles: [], default: false}
      ]
    },
    subtitles: {
      selectionSupported: true,
      selectionReason: null,
      offSupported: true,
      offSelected: !subtitleTrackId,
      selectedTrackId: subtitleTrackId || null,
      defaultTrackId: null,
      availableTracks: [
        {
          id: 'subtitle-1', language: 'ger', label: 'Deutsch', format: 'subrip',
          roles: [], default: false, forced: false, selectable: true, deliveryFormat: 'webvtt'
        },
        {
          id: 'subtitle-2', language: 'ger', label: 'DVB', format: 'dvb-subtitle',
          roles: [], default: false, forced: false, selectable: false
        },
        {
          id: 'subtitle-3', language: 'ger', label: 'Teletext', format: 'teletext',
          roles: [], default: false, forced: false, selectable: false
        }
      ]
    }
  };
}

(async function () {
  const timers = [];
  function setTimeoutFake(callback) {
    timers.push(callback);
    return timers.length;
  }
  function clearTimeoutFake() {}
  async function runNextTimer(message) {
    assert.ok(timers.length > 0, message || 'expected lifetime watcher timer');
    const callback = timers.shift();
    callback();
    await flush();
  }

  const blobUrls = [];
  const revokedUrls = [];
  class BlobFake {
    constructor(parts, options) {
      this.parts = parts;
      this.options = options;
    }
  }
  const URLFake = {
    createObjectURL(blob) {
      const url = 'blob:subtitle-' + String(blobUrls.length + 1);
      blobUrls.push({url, blob});
      return url;
    },
    revokeObjectURL(url) { revokedUrls.push(url); }
  };

  let activeSessionId = '';
  let activeProfileId = 'progressive-fmp4';
  let activeAudioTrackId = 'audio-1';
  let activeSubtitleTrackId = '';
  let outerPosition = 0;
  let fallbackAbsolutePosition = 0;
  let fallbackSelections = 0;
  let inFallback = false;
  const statusRequests = [];
  const subtitleRequests = [];

  const fastElement = node('section');
  fastElement.className = 'recordings2-fast-player';
  const internalStartButton = node('button');
  internalStartButton.className = 'recordings2-primary';
  const progressiveVideo = node('video');
  fastElement.appendChild(internalStartButton);
  fastElement.appendChild(progressiveVideo);
  internalStartButton.addEventListener('click', function () {
    activeSessionId = 'progressive-session-1';
    activeProfileId = 'progressive-fmp4';
    activeAudioTrackId = 'audio-1';
    activeSubtitleTrackId = '';
    outerPosition = 0;
    fallbackAbsolutePosition = 0;
    progressiveVideo.currentTime = 0;
    inFallback = false;
  });

  const fallbackElement = node('section');
  fallbackElement.className = 'recordings2-recording-fallback-shell';
  let hlsVideo = node('video');
  fallbackElement.appendChild(hlsVideo);

  const fallbackOwner = Object.freeze({
    selectAudioTrack(trackId) {
      fallbackSelections += 1;
      activeAudioTrackId = trackId;
      activeSessionId = 'hls-session-' + String(fallbackSelections + 1);
      const replacementVideo = node('video');
      replacementVideo.currentTime = 0;
      hlsVideo.replaceWith(replacementVideo);
      hlsVideo = replacementVideo;
      return Promise.resolve(activeSessionId);
    },
    position() { return fallbackAbsolutePosition; },
    state() { return 'playing'; },
    stop() { return Promise.resolve(true); }
  });
  fallbackElement.__vdrSuiteRecordingFallbackOwner = fallbackOwner;

  const basePanel = Object.freeze({
    element: fastElement,
    start() {
      throw new Error('production-style subtitle lifecycle test must not use wrapped start()');
    },
    sessionId() { return activeSessionId; },
    position() { return outerPosition; },
    state() { return inFallback ? 'fallback' : (activeSessionId ? 'playing' : 'idle'); },
    seekAbsolute() { return Promise.resolve(true); },
    play() { return Promise.resolve(true); },
    pause() { return true; },
    stop() {
      activeSessionId = '';
      return Promise.resolve(true);
    },
    destroy() {},
    relinquishForReplacement() { return Promise.resolve(activeSessionId); }
  });

  let assigned = {};
  const document = {createElement: node};
  const window = {
    document,
    console,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake,
    Blob: BlobFake,
    URL: URLFake,
    VdrSuiteBrowserSession: {csrfHeaders() { return {'X-CSRF-Token': 'csrf-subtitle-owner'}; }},
    VdrSuiteClientApi: {
      requestJson(requestPath, options) {
        const body = JSON.parse(options.body);
        if (body.operation === 'track-status') {
          statusRequests.push(body);
          return Promise.resolve({mediaSession: {
            id: body.sessionId,
            state: 'ready',
            presentationProfileId: activeProfileId,
            tracks: trackContract(activeAudioTrackId, activeSubtitleTrackId)
          }});
        }
        throw new Error('unexpected JSON operation ' + body.operation);
      }
    },
    fetch(requestPath, options) {
      assert.strictEqual(requestPath, '/api/media/sessions');
      const body = JSON.parse(options.body);
      assert.strictEqual(body.operation, 'select-subtitle-track');
      subtitleRequests.push(body);
      if (body.subtitleTrackId === 'off') {
        activeSubtitleTrackId = '';
        return Promise.resolve({
          ok: true,
          headers: {get() { return 'application/json'; }},
          json() { return Promise.resolve({mediaSession: {id: body.sessionId}}); }
        });
      }
      assert.strictEqual(body.subtitleTrackId, 'subtitle-1');
      activeSubtitleTrackId = 'subtitle-1';
      return Promise.resolve({
        ok: true,
        headers: {get(name) { return String(name).toLowerCase() === 'content-type' ? 'text/vtt; charset=utf-8' : null; }},
        text() {
          return Promise.resolve('WEBVTT\n\n00:00:00.000 --> 00:00:02.000\nTest subtitle\n');
        }
      });
    }
  };
  window.window = window;
  Object.defineProperty(window, 'VdrSuiteRecordings2Playback', {
    configurable: true,
    enumerable: true,
    get() { return assigned; },
    set(value) { assigned = value; }
  });

  const context = vm.createContext({
    window,
    document,
    console,
    Object,
    String,
    Number,
    Array,
    Boolean,
    Promise,
    RegExp,
    Error,
    JSON,
    Math,
    Date,
    Blob: BlobFake,
    URL: URLFake,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake
  });
  vm.runInContext(source, context, {filename: 'recording-track-controls.js'});

  window.VdrSuiteRecordings2Playback = Object.freeze({createPanel() { return basePanel; }});
  const playback = window.VdrSuiteRecordings2Playback.createPanel({id: 'subtitle-recording'}, 'default');
  const ownerShell = playback.element;
  assert.strictEqual(ownerShell.className, 'recordings2-track-owner-shell');
  assert.strictEqual(ownerShell.children[0], fastElement);
  assert.strictEqual(statusRequests.length, 0, 'idle owner must not poll track status');

  // Production's visible Start button calls the canonical owner's internal
  // closure, not the decorator's exported start(). The lifetime watcher must
  // discover the first MediaSession from owner state.
  internalStartButton.dispatch('click');
  await runNextTimer('subtitle owner must observe an internally started session');
  assert.ok(
    statusRequests.some(body => body.sessionId === 'progressive-session-1'),
    'internal Start must trigger track-status for the first active session'
  );

  const audioSelect = find(
    ownerShell,
    item => item.tagName === 'SELECT' && item['aria-label'] === 'Tonspur auswählen'
  );
  const subtitleSelect = find(
    ownerShell,
    item => item.tagName === 'SELECT' && item['aria-label'] === 'Untertitel auswählen'
  );
  const subtitleRow = find(
    ownerShell,
    item => item.className === 'recordings2-subtitle-track-control'
  );
  assert.ok(subtitleSelect, 'subtitle selector must live in the stable Recording track owner');
  assert.strictEqual(subtitleRow.hidden, false);
  assert.strictEqual(subtitleSelect.disabled, false);
  assert.strictEqual(subtitleSelect.value, 'off');
  assert.deepStrictEqual(
    subtitleSelect.children.map(option => option.value),
    ['off', 'subtitle-1'],
    'DVB and Teletext metadata must not become selectable browser text tracks'
  );
  assert.strictEqual(audioSelect.value, 'audio-1');

  subtitleSelect.value = 'subtitle-1';
  subtitleSelect.dispatch('change');
  await flush(20);
  assert.strictEqual(subtitleRequests.length, 1);
  assert.strictEqual(subtitleRequests[0].sessionId, 'progressive-session-1');
  assert.strictEqual(subtitleRequests[0].subtitleTrackId, 'subtitle-1');
  assert.strictEqual(subtitleRequests[0].streamBasePositionSeconds, 0);
  let mountedTrack = find(progressiveVideo, item => item.tagName === 'TRACK');
  assert.ok(mountedTrack, 'selected WebVTT must be mounted on the active video');
  assert.strictEqual(mountedTrack.kind, 'subtitles');
  assert.strictEqual(mountedTrack.track.mode, 'showing');
  const firstSubtitleUrl = mountedTrack.src;
  assert.strictEqual(firstSubtitleUrl, 'blob:subtitle-1');

  subtitleSelect.value = 'off';
  subtitleSelect.dispatch('change');
  await flush(20);
  assert.strictEqual(subtitleRequests.length, 2);
  assert.strictEqual(subtitleRequests[1].subtitleTrackId, 'off');
  assert.strictEqual(find(progressiveVideo, item => item.tagName === 'TRACK'), undefined);
  assert.ok(revokedUrls.includes(firstSubtitleUrl), 'OFF must revoke the active WebVTT Blob URL');

  // Re-enable the preference so same-session reposition and later transport
  // replacement must rebind it without creating another playback owner.
  subtitleSelect.value = 'subtitle-1';
  subtitleSelect.dispatch('change');
  await flush(20);
  mountedTrack = find(progressiveVideo, item => item.tagName === 'TRACK');
  assert.ok(mountedTrack);
  const beforeSeekUrl = mountedTrack.src;

  outerPosition = 120;
  progressiveVideo.currentTime = 0;
  await runNextTimer('same-session stream-base change must be observed');
  await flush(20);
  const seekRequest = subtitleRequests.find(request =>
    request.sessionId === 'progressive-session-1' &&
    request.subtitleTrackId === 'subtitle-1' &&
    request.streamBasePositionSeconds === 120
  );
  assert.ok(seekRequest, 'same-session seek/restart must request WebVTT from the new Recording base');
  mountedTrack = find(progressiveVideo, item => item.tagName === 'TRACK');
  assert.ok(mountedTrack);
  assert.notStrictEqual(mountedTrack.src, beforeSeekUrl);
  assert.ok(revokedUrls.includes(beforeSeekUrl), 'same-session rebind must revoke the stale Blob URL');

  const beforeHlsUrl = mountedTrack.src;
  activeSessionId = 'hls-session-1';
  activeProfileId = 'hls-fmp4';
  fallbackAbsolutePosition = 300;
  hlsVideo.currentTime = 0;
  inFallback = true;
  fastElement.replaceWith(fallbackElement);
  assert.strictEqual(outerPosition, 120, 'outer fast-owner position intentionally stays stale after fallback');
  assert.strictEqual(ownerShell.children[0], fallbackElement, 'HLS replaces only the inner transport');
  assert.ok(
    find(ownerShell, item => item.className === 'recordings2-track-controls'),
    'stable subtitle/audio owner must survive transport replacement'
  );
  await runNextTimer('lifetime watcher must observe HLS replacement session');
  await flush(20);
  assert.ok(statusRequests.some(body => body.sessionId === 'hls-session-1'));
  const hlsSubtitleRequest = subtitleRequests.find(request =>
    request.sessionId === 'hls-session-1' &&
    request.subtitleTrackId === 'subtitle-1' &&
    request.streamBasePositionSeconds === 300
  );
  assert.ok(hlsSubtitleRequest, 'subtitle base must come from the active HLS owner, not stale outer position');
  mountedTrack = find(hlsVideo, item => item.tagName === 'TRACK');
  assert.ok(mountedTrack, 'replacement video must receive a fresh WebVTT track');
  assert.strictEqual(mountedTrack.track.mode, 'showing');
  assert.ok(revokedUrls.includes(beforeHlsUrl), 'transport replacement must revoke the previous Blob URL');
  assert.strictEqual(subtitleSelect.value, 'subtitle-1', 'stable owner must preserve subtitle preference');
  assert.strictEqual(audioSelect.value, 'audio-1', 'subtitle rebinding must not change audio selection');

  // Advancing within the same HLS stream changes absolute and local position
  // equally, so the stream base remains 300 and no WebVTT re-extraction occurs.
  const requestsBeforeHlsProgress = subtitleRequests.length;
  fallbackAbsolutePosition = 337;
  hlsVideo.currentTime = 37;
  await runNextTimer('same HLS stream progress must keep its stream base stable');
  await flush(20);
  assert.strictEqual(
    subtitleRequests.length,
    requestsBeforeHlsProgress,
    'normal HLS playback progress must not continuously regenerate WebVTT'
  );

  // The accepted HLS audio path remains the owner of audio replacement. A
  // subtitle preference may follow that replacement, but must not duplicate it.
  const beforeAudioReplacementUrl = mountedTrack.src;
  audioSelect.value = 'audio-2';
  audioSelect.dispatch('change');
  await flush(30);
  assert.strictEqual(fallbackSelections, 1, 'audio switch must still delegate to the existing HLS owner');
  assert.strictEqual(activeAudioTrackId, 'audio-2');
  assert.ok(statusRequests.some(body => body.sessionId === 'hls-session-2'));
  const audioReplacementSubtitleRequest = subtitleRequests.find(request =>
    request.sessionId === 'hls-session-2' &&
    request.subtitleTrackId === 'subtitle-1' &&
    request.streamBasePositionSeconds === 337
  );
  assert.ok(audioReplacementSubtitleRequest, 'subtitle preference must follow the audio replacement session/base');
  mountedTrack = find(hlsVideo, item => item.tagName === 'TRACK');
  assert.ok(mountedTrack);
  assert.ok(revokedUrls.includes(beforeAudioReplacementUrl));
  assert.strictEqual(subtitleSelect.value, 'subtitle-1');
  assert.strictEqual(audioSelect.value, 'audio-2');

  const finalUrl = mountedTrack.src;
  playback.destroy();
  assert.ok(revokedUrls.includes(finalUrl), 'owner destruction must revoke the final subtitle Blob');
  assert.strictEqual(find(hlsVideo, item => item.tagName === 'TRACK'), undefined);

  console.log('phase65d production-style Recording subtitle WebVTT owner lifecycle covered');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
