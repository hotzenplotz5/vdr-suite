'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-track-controls.js'),
  'utf8'
);

function node(tagName) {
  const listeners = {};
  const value = {
    tagName: String(tagName || '').toUpperCase(),
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
    appendChild(child) {
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
    setAttribute(name, val) { this[name] = String(val); },
    addEventListener(name, callback) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push(callback);
    },
    dispatch(name, event) {
      (listeners[name] || []).forEach(callback => callback(event || {target: this}));
    }
  };
  Object.defineProperty(value, 'firstChild', {
    get() { return this.children.length ? this.children[0] : null; }
  });
  return value;
}

function descendants(root) {
  const values = [];
  (function walk(item) {
    if (!item) return;
    values.push(item);
    (item.children || []).forEach(walk);
  }(root));
  return values;
}

function find(root, predicate) {
  return descendants(root).find(predicate);
}

function flush(count = 6) {
  let promise = Promise.resolve();
  for (let index = 0; index < count; index += 1) {
    promise = promise.then(() => Promise.resolve());
  }
  return promise;
}

function tracks(selectedTrackId, supported = true, reason = null) {
  return {
    audio: {
      selectionSupported: supported,
      selectionReason: reason,
      selectedTrackId,
      defaultTrackId: 'audio-1',
      availableTracks: [
        {
          id: 'audio-1', language: 'deu', label: 'Deutsch Dolby Digital',
          codec: 'ac3', channels: 6, layout: '5.1(side)', roles: [], default: true,
          selected: selectedTrackId === 'audio-1'
        },
        {
          id: 'audio-2', language: 'eng', label: '',
          codec: 'aac', channels: 2, layout: 'stereo', roles: ['original'], default: false,
          selected: selectedTrackId === 'audio-2'
        }
      ]
    },
    subtitles: {
      selectionSupported: false,
      selectionReason: 'profile_does_not_deliver_selectable_subtitles',
      offSupported: true,
      offSelected: true,
      availableTracks: [
        {id: 'subtitle-1', language: 'deu', format: 'dvb-subtitle', roles: ['forced'], forced: true}
      ],
      selectedTrackId: null,
      defaultTrackId: null
    }
  };
}

function createRuntime(options) {
  const settings = Object.assign({
    initialSelected: 'audio-1',
    selectionSupported: true,
    selectionReason: null,
    selectionFails: false,
    reconnectFails: false,
    initialState: 'playing',
    position: 125
  }, options || {});

  const requests = [];
  const seekCalls = [];
  let stopCalls = 0;
  let playCalls = 0;
  let pauseCalls = 0;
  let state = settings.initialState;
  let activeTrack = settings.initialSelected;
  const sessionId = 'mediasess_track_1';
  let assignedFacade = {};

  const document = {
    createElement(tagName) { return node(tagName); }
  };

  const root = node('section');
  const basePanel = Object.freeze({
    element: root,
    start() { return Promise.resolve(sessionId); },
    sessionId() { return sessionId; },
    position() { return settings.position; },
    state() { return state; },
    seekAbsolute(position) {
      seekCalls.push(position);
      if (settings.reconnectFails) return Promise.reject(new Error('reconnect failed'));
      return Promise.resolve(true);
    },
    play() { playCalls += 1; state = 'playing'; return Promise.resolve(true); },
    pause() { pauseCalls += 1; state = 'paused'; return true; },
    stop() { stopCalls += 1; state = 'stopped'; return Promise.resolve(true); },
    destroy() { state = 'destroyed'; },
    relinquishForReplacement() { return Promise.resolve(sessionId); }
  });

  const window = {
    document,
    console,
    setTimeout,
    clearTimeout,
    VdrSuiteBrowserSession: {
      csrfHeaders() { return {'X-CSRF-Token': 'csrf-track'}; }
    },
    VdrSuiteClientApi: {
      requestJson(requestPath, requestOptions) {
        const body = JSON.parse(requestOptions.body);
        requests.push({path: requestPath, body, options: requestOptions});
        if (body.operation === 'track-status') {
          return Promise.resolve({
            mediaSession: {
              id: sessionId,
              state: 'ready',
              presentationProfileId: 'progressive-fmp4',
              tracks: tracks(activeTrack, settings.selectionSupported, settings.selectionReason)
            }
          });
        }
        if (body.operation === 'select-audio-track') {
          if (settings.selectionFails) return Promise.reject(new Error('server rejected track'));
          activeTrack = body.audioTrackId;
          return Promise.resolve({
            mediaSession: {
              id: sessionId,
              state: 'ready',
              presentationProfileId: 'progressive-fmp4',
              mediaPath: '/api/media/sessions/' + sessionId + '/recording/stream.mp4',
              playback: {
                positionSeconds: body.positionSeconds,
                durationSeconds: 5400,
                seek: {supported: true, preparing: false, window: {startSeconds: 0, endSeconds: 5400}}
              },
              tracks: tracks(activeTrack, true, null)
            }
          });
        }
        throw new Error('unexpected operation ' + body.operation);
      }
    }
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
    setTimeout,
    clearTimeout
  });
  vm.runInContext(source, context, {filename: 'recording-track-controls.js'});

  window.VdrSuiteRecordings2Playback = Object.freeze({
    createPanel() { return basePanel; }
  });

  const playback = window.VdrSuiteRecordings2Playback.createPanel(
    {id: 'recording-42'},
    'living-room'
  );

  return {
    window,
    playback,
    requests,
    seekCalls,
    root,
    metrics: {
      stopCalls: () => stopCalls,
      playCalls: () => playCalls,
      pauseCalls: () => pauseCalls,
      activeTrack: () => activeTrack,
      state: () => state
    }
  };
}

(async function () {
  const runtime = createRuntime();
  const playback = runtime.playback;
  const id = await playback.start();
  assert.strictEqual(id, 'mediasess_track_1');
  await flush();

  const select = find(
    runtime.root,
    item => item.tagName === 'SELECT' && item['aria-label'] === 'Tonspur auswählen'
  );
  assert.ok(select, 'audio selection must decorate the existing Recording panel');
  const audioRow = find(
    runtime.root,
    item => item.className === 'recordings2-audio-track-control'
  );
  assert.strictEqual(audioRow.hidden, false, 'audio selector must be visible for multiple selectable tracks');
  assert.strictEqual(select.children.length, 2);
  assert.strictEqual(select.value, 'audio-1');
  assert.ok(select.children[0].textContent.includes('Deutsch'));
  assert.ok(select.children[0].textContent.includes('AC3'));
  assert.ok(select.children[0].textContent.includes('5.1(side)'));
  assert.ok(select.children[1].textContent.includes('Englisch'));
  assert.ok(select.children[1].textContent.includes('Original'));

  const subtitle = find(
    runtime.root,
    item => item.className === 'recordings2-subtitle-track-status'
  );
  assert.strictEqual(subtitle.hidden, false, 'known suppressed subtitles must be reported truthfully');
  assert.ok(subtitle.textContent.includes('Untertitel: Aus'));
  assert.ok(subtitle.textContent.includes('nicht verfügbar'));

  const statusRequests = runtime.requests.filter(entry => entry.body.operation === 'track-status');
  assert.strictEqual(statusRequests.length, 1);
  assert.strictEqual(statusRequests[0].body.sessionId, id);

  select.value = 'audio-2';
  select.dispatch('change');
  await flush(12);

  const selections = runtime.requests.filter(entry => entry.body.operation === 'select-audio-track');
  assert.strictEqual(selections.length, 1);
  assert.strictEqual(selections[0].body.sessionId, id);
  assert.strictEqual(selections[0].body.recordingId, 'recording-42');
  assert.strictEqual(selections[0].body.audioTrackId, 'audio-2');
  assert.strictEqual(selections[0].body.positionSeconds, 125);
  assert.deepStrictEqual(selections[0].body.capabilities.protocols, ['progressive']);
  assert.deepStrictEqual(selections[0].body.capabilities.containers, ['fmp4']);
  assert.deepStrictEqual(selections[0].body.capabilities.audioCodecs, ['aac']);
  assert.strictEqual(runtime.seekCalls.length, 1, 'existing D.2 owner must perform the reconnect');
  assert.strictEqual(runtime.seekCalls[0], 125, 'audio reconnect must preserve absolute Recording position');
  assert.strictEqual(playback.sessionId(), id, 'audio selection must preserve MediaSession identity');
  assert.strictEqual(select.value, 'audio-2');
  assert.strictEqual(runtime.metrics.pauseCalls(), 1, 'playing audio switch must pause the old transport before server restart');
  assert.strictEqual(runtime.metrics.playCalls(), 1, 'playing audio switch must resume only after reconnect');
  assert.strictEqual(runtime.metrics.state(), 'playing', 'playing state must remain playing');

  const paused = createRuntime({initialState: 'paused'});
  await paused.playback.start();
  await flush();
  const pausedSelect = find(paused.root, item => item.tagName === 'SELECT');
  pausedSelect.value = 'audio-2';
  pausedSelect.dispatch('change');
  await flush(12);
  assert.deepStrictEqual(paused.seekCalls, [125]);
  assert.strictEqual(paused.metrics.state(), 'paused', 'paused state must remain paused');
  assert.strictEqual(paused.metrics.playCalls(), 0, 'paused audio switch must not autoplay');

  const unsupported = createRuntime({
    selectionSupported: false,
    selectionReason: 'recording_audio_track_selection_timeline_unavailable'
  });
  await unsupported.playback.start();
  await flush();
  const unsupportedRow = find(
    unsupported.root,
    item => item.className === 'recordings2-audio-track-control'
  );
  const unsupportedSelect = find(unsupported.root, item => item.tagName === 'SELECT');
  assert.strictEqual(unsupportedRow.hidden, true, 'unsupported audio selection must not expose a fake control');
  assert.strictEqual(unsupportedSelect.disabled, true);
  assert.strictEqual(
    unsupported.requests.filter(entry => entry.body.operation === 'select-audio-track').length,
    0
  );

  const rejected = createRuntime({selectionFails: true});
  await rejected.playback.start();
  await flush();
  const rejectedSelect = find(rejected.root, item => item.tagName === 'SELECT');
  rejectedSelect.value = 'audio-2';
  rejectedSelect.dispatch('change');
  await flush(12);
  assert.strictEqual(rejected.seekCalls.length, 0, 'failed server selection must not reconnect');
  assert.strictEqual(rejectedSelect.value, 'audio-1', 'failed selection must restore selected track');
  const rejectedNote = find(rejected.root, item => item.className === 'recordings2-track-status');
  assert.ok(rejectedNote.textContent.includes('Tonspurwechsel fehlgeschlagen'));
  assert.ok(rejectedNote.textContent.includes('server rejected track'));
  assert.strictEqual(rejected.metrics.stopCalls(), 0, 'server rejection keeps the current playback owner alive');
  assert.strictEqual(rejected.metrics.pauseCalls(), 1, 'playing selection must be stabilized before request');
  assert.strictEqual(rejected.metrics.playCalls(), 1, 'rejected selection must resume the old playback');
  assert.strictEqual(rejected.metrics.state(), 'playing');

  const reconnectFailure = createRuntime({reconnectFails: true});
  await reconnectFailure.playback.start();
  await flush();
  const reconnectSelect = find(reconnectFailure.root, item => item.tagName === 'SELECT');
  reconnectSelect.value = 'audio-2';
  reconnectSelect.dispatch('change');
  await flush(12);
  assert.strictEqual(reconnectFailure.seekCalls.length, 1);
  assert.strictEqual(
    reconnectFailure.metrics.stopCalls(),
    1,
    'post-selection reconnect failure must fail closed and stop the owned session'
  );
  const reconnectNote = find(
    reconnectFailure.root,
    item => item.className === 'recordings2-track-status'
  );
  assert.ok(reconnectNote.textContent.includes('serverseitig gewechselt'));
  assert.ok(reconnectNote.textContent.includes('Wiedergabe wurde gestoppt'));

  const testApi = runtime.window.VdrSuiteRecordingTrackControls.__test;
  assert.strictEqual(testApi.safeAudioTrackId('audio-2'), 'audio-2');
  assert.strictEqual(testApi.safeAudioTrackId('pid-123'), '');
  assert.deepStrictEqual(Array.from(testApi.recordingCapabilities().videoCodecs), ['h264']);

  console.log('phase65d recording track controls, normalized audio selection and truthful subtitle status ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
