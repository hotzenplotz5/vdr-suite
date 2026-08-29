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
  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }
  function post(body) {
    const fetcher = global.fetch || fetch;
    return fetcher('/api/media/continue-watching', {
      method: 'POST',
      credentials: 'same-origin',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify(body)
    }).then(function (response) {
      if (!response || !response.ok) throw new Error('continue watching sync failed');
      return true;
    });
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

  const recordingsDescriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2') || {
    configurable: true,
    enumerable: true,
    value: global.VdrSuiteRecordings2
  };
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
    if (!owner) return Promise.reject(new Error('Canonical Recording owner is unavailable.'));
    if (target === 0) {
      if (typeof owner.start !== 'function') return Promise.reject(new Error('Canonical Recording start is unavailable.'));
      return Promise.resolve(owner.start());
    }
    if (typeof owner.startAtAbsolute === 'function') {
      return Promise.resolve(owner.startAtAbsolute(target));
    }
    if (typeof owner.start === 'function' && typeof owner.seekAbsolute === 'function') {
      return Promise.resolve(owner.start()).then(function () {
        return owner.seekAbsolute(target);
      });
    }
    return Promise.reject(new Error('Canonical Recording absolute start is unavailable.'));
  }

  function decorateOwner(owner, recording, backendId) {
    if (!owner || owner.__vdrSuiteContinueWatchingOwner) return owner;
    let timer = null;
    let lastPosition = -1;
    let destroyed = false;
    let mediaObserver = null;
    const boundMedia = [];

    function canResume() {
      return typeof owner.canResume !== 'function' || owner.canResume() === true;
    }
    function flush(forceClear) {
      if (destroyed && !forceClear) return;
      const position = typeof owner.position === 'function'
        ? Math.max(0, Math.floor(Number(owner.position()) || 0))
        : 0;
      const duration = typeof owner.duration === 'function'
        ? Math.max(0, Math.floor(Number(owner.duration()) || 0))
        : 0;
      const complete = forceClear || (duration > 0 && position >= duration);
      if (!complete && (!position || !canResume() || position === lastPosition)) return;
      lastPosition = position;
      post(complete ? {
        operation: 'clear',
        backendId,
        recordingId: recordingId(recording),
        operationId: nextOperationId('clear')
      } : {
        operation: 'progress',
        backendId,
        recordingId: recordingId(recording),
        positionSeconds: position,
        resumeSupported: true,
        operationId: nextOperationId('progress')
      }).catch(function () {});
    }
    function poll() {
      if (destroyed) return;
      flush(false);
      timer = global.setTimeout(poll, 5000);
    }
    function ensurePolling() {
      if (timer === null && !destroyed) timer = global.setTimeout(poll, 5000);
    }
    function bindEnded() {
      const root = owner.element;
      if (!root || typeof root.querySelectorAll !== 'function') return;
      const media = root.querySelectorAll('video, audio');
      Array.prototype.forEach.call(media || [], function (element) {
        if (!element || boundMedia.indexOf(element) >= 0 || typeof element.addEventListener !== 'function') return;
        boundMedia.push(element);
        element.addEventListener('ended', function () { flush(true); });
      });
      if (mediaObserver || typeof global.MutationObserver !== 'function') return;
      mediaObserver = new global.MutationObserver(bindEnded);
      mediaObserver.observe(root, {childList: true, subtree: true});
    }
    function cleanupTracking() {
      destroyed = true;
      if (timer !== null) global.clearTimeout(timer);
      timer = null;
      if (mediaObserver && typeof mediaObserver.disconnect === 'function') mediaObserver.disconnect();
      mediaObserver = null;
    }

    const decorated = {};
    Object.keys(owner).forEach(function (key) { decorated[key] = owner[key]; });
    decorated.startAtAbsolute = function (positionSeconds) {
      bindEnded();
      ensurePolling();
      return startAtAbsolute(owner, positionSeconds).then(function (result) {
        bindEnded();
        return result;
      });
    };
    if (typeof owner.start === 'function') {
      decorated.start = function () {
        bindEnded();
        ensurePolling();
        const result = owner.start();
        Promise.resolve(result).then(bindEnded).catch(function () {});
        return result;
      };
    }
    if (typeof owner.resume === 'function') {
      decorated.resume = function (positionSeconds) {
        bindEnded();
        ensurePolling();
        const result = owner.resume(positionSeconds);
        Promise.resolve(result).then(bindEnded).catch(function () {});
        return result;
      };
    }
    if (typeof owner.stop === 'function') {
      decorated.stop = function () {
        flush(false);
        return owner.stop();
      };
    }
    if (typeof owner.destroy === 'function') {
      decorated.destroy = function () {
        flush(false);
        cleanupTracking();
        return owner.destroy();
      };
    }
    if (typeof owner.relinquishForReplacement === 'function') {
      decorated.relinquishForReplacement = function () {
        flush(false);
        cleanupTracking();
        return owner.relinquishForReplacement();
      };
    }
    decorated.__vdrSuiteContinueWatchingOwner = true;
    const result = Object.freeze(decorated);
    bindEnded();

    if (pending.autoStart && pending.backendId === backendId && pending.recordingId === recordingId(recording)) {
      const position = pending.positionSeconds;
      pending.autoStart = false;
      global.setTimeout(function () {
        result.startAtAbsolute(position).catch(function () {});
      }, 0);
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
    __test: Object.freeze({startAtAbsolute, rememberOpen, decorateOwner, post})
  });
}(window));
