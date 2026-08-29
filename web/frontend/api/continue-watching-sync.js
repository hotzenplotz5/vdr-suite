(function (global) {
  'use strict';

  const descriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback') || {
    configurable: true,
    enumerable: true,
    value: global.VdrSuiteRecordings2Playback
  };
  let currentValue = descriptor.get ? descriptor.get.call(global) : descriptor.value;
  const pending = {backendId: '', recordingId: '', positionSeconds: 0, autoStart: false};
  let operationCounter = 0;

  function text(value) { return value == null ? '' : String(value); }
  function recordingId(recording) { return text(recording && (recording.id || recording.recordingId)); }
  function nextOperationId(prefix) {
    operationCounter += 1;
    return 'cw-' + prefix + '-' + Date.now().toString(36) + '-' + operationCounter.toString(36);
  }
  function post(body) {
    const fetcher = global.fetch || fetch;
    return fetcher('/api/media/continue-watching', {
      method: 'POST', credentials: 'same-origin', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(body)
    }).then(function (response) { if (!response || !response.ok) throw new Error('continue watching sync failed'); return true; });
  }
  function rememberOpen(recording, options) {
    const settings = options && typeof options === 'object' ? options : {};
    if (settings.continueWatching !== true || settings.autoStartPlayback !== true) return;
    pending.backendId = text(recording && recording.backendId) || 'default';
    pending.recordingId = recordingId(recording);
    pending.positionSeconds = Math.max(0, Math.floor(Number(settings.playbackStartPositionSeconds) || 0));
    pending.autoStart = Boolean(pending.recordingId);
  }
  function decorateRecordings2(value) {
    if (!value || typeof value.openRecording !== 'function' || value.__vdrSuiteContinueWatchingOpenDecorated) return value;
    const copy = {};
    Object.keys(value).forEach(function (key) { copy[key] = value[key]; });
    const open = value.openRecording;
    copy.openRecording = function (recording, options) {
      rememberOpen(recording, options);
      return open.call(value, recording, options);
    };
    copy.__vdrSuiteContinueWatchingOpenDecorated = true;
    return Object.freeze(copy);
  }

  const recordingsDescriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2') || {configurable: true, enumerable: true, value: global.VdrSuiteRecordings2};
  let recordingsValue = recordingsDescriptor.get ? recordingsDescriptor.get.call(global) : recordingsDescriptor.value;
  Object.defineProperty(global, 'VdrSuiteRecordings2', {
    configurable: recordingsDescriptor.configurable !== false,
    enumerable: recordingsDescriptor.enumerable !== false,
    get: function () { return recordingsDescriptor.get ? recordingsDescriptor.get.call(global) : recordingsValue; },
    set: function (value) {
      const decorated = decorateRecordings2(value);
      recordingsValue = decorated;
      if (recordingsDescriptor.set) recordingsDescriptor.set.call(global, decorated);
    }
  });
  if (recordingsValue) global.VdrSuiteRecordings2 = recordingsValue;

  function startAtAbsolute(owner, positionSeconds) {
    const target = Math.max(0, Math.floor(Number(positionSeconds) || 0));
    if (!owner) return Promise.resolve('');
    if (target > 0 && typeof owner.resume === 'function') return Promise.resolve(owner.resume(target));
    if (target === 0 && typeof owner.start === 'function') return Promise.resolve(owner.start());
    return Promise.reject(new Error('Canonical Recording resume is unavailable.'));
  }
  function decorateOwner(owner, recording, backendId) {
    if (!owner || owner.__vdrSuiteContinueWatchingOwner) return owner;
    let timer = null;
    let lastPosition = -1;
    let ended = false;
    function canResume() { return typeof owner.canResume !== 'function' || owner.canResume() === true; }
    function flush(forceClear) {
      const position = typeof owner.position === 'function' ? Math.max(0, Math.floor(Number(owner.position()) || 0)) : 0;
      const duration = typeof owner.duration === 'function' ? Math.max(0, Math.floor(Number(owner.duration()) || 0)) : 0;
      const complete = forceClear || (duration > 0 && position >= duration);
      if (!complete && (!position || !canResume() || position === lastPosition)) return;
      lastPosition = position;
      post(complete ? {
        operation: 'clear', backendId, recordingId: recordingId(recording), operationId: nextOperationId('clear')
      } : {
        operation: 'progress', backendId, recordingId: recordingId(recording), positionSeconds: position,
        resumeSupported: true, operationId: nextOperationId('progress')
      }).catch(function () {});
    }
    function poll() {
      if (ended) return;
      flush(false);
      timer = global.setTimeout(poll, 5000);
    }
    const decorated = {};
    Object.keys(owner).forEach(function (key) { decorated[key] = owner[key]; });
    decorated.startAtAbsolute = function (positionSeconds) { return startAtAbsolute(owner, positionSeconds); };
    if (typeof owner.start === 'function') decorated.start = function () { const result = owner.start(); if (timer === null) timer = global.setTimeout(poll, 5000); return result; };
    if (typeof owner.resume === 'function') decorated.resume = function (positionSeconds) { const result = owner.resume(positionSeconds); if (timer === null) timer = global.setTimeout(poll, 5000); return result; };
    if (typeof owner.stop === 'function') decorated.stop = function () { flush(false); return owner.stop(); };
    if (typeof owner.destroy === 'function') decorated.destroy = function () { flush(false); ended = true; if (timer !== null) global.clearTimeout(timer); return owner.destroy(); };
    decorated.__vdrSuiteContinueWatchingOwner = true;
    const result = Object.freeze(decorated);
    if (pending.autoStart && pending.backendId === backendId && pending.recordingId === recordingId(recording)) {
      const position = pending.positionSeconds;
      pending.autoStart = false;
      global.setTimeout(function () { result.startAtAbsolute(position).catch(function () {}); }, 0);
    }
    return result;
  }
  function decoratePlayback(value) {
    const source = value && typeof value === 'object' ? value : {};
    if (typeof source.createPanel !== 'function' || source.__vdrSuiteContinueWatchingDecorated) return source;
    const decorated = {};
    Object.keys(source).forEach(function (key) { decorated[key] = source[key]; });
    const factory = source.createPanel;
    decorated.createPanel = function (recording, backendId) {
      return decorateOwner(factory.apply(source, arguments), recording, backendId);
    };
    decorated.__vdrSuiteContinueWatchingDecorated = true;
    return Object.freeze(decorated);
  }
  Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
    configurable: descriptor.configurable !== false,
    enumerable: descriptor.enumerable !== false,
    get: function () { return descriptor.get ? descriptor.get.call(global) : currentValue; },
    set: function (value) {
      const decorated = decoratePlayback(value);
      currentValue = decorated;
      if (descriptor.set) descriptor.set.call(global, decorated);
    }
  });
  if (currentValue) global.VdrSuiteRecordings2Playback = currentValue;

  global.VdrSuiteContinueWatchingSync = Object.freeze({
    __test: Object.freeze({startAtAbsolute, rememberOpen, decorateOwner})
  });
}(window));
