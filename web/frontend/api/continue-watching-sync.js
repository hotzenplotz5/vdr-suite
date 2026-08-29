(function (global) {
  'use strict';

  const playbackDescriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
  if (!playbackDescriptor || typeof playbackDescriptor.get !== 'function' || typeof playbackDescriptor.set !== 'function') {
    return;
  }

  const pending = {backendId: '', recordingId: '', positionSeconds: 0, autoStart: false};
  let operationCounter = 0;
  let mutationQueue = Promise.resolve();

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
  function enqueue(body) {
    const operation = mutationQueue.catch(function () {}).then(function () {
      return post(body);
    });
    mutationQueue = operation.catch(function () {});
    return operation;
  }
  function rememberOpen(recording, options) {
    const settings = options && typeof options === 'object' ? options : {};
    if (settings.continueWatching !== true || settings.autoStartPlayback !== true) return;
    pending.backendId = text(recording && recording.backendId) || 'default';
    pending.recordingId = recordingId(recording);
    pending.positionSeconds = Math.max(0, Math.floor(Number(settings.playbackStartPositionSeconds) || 0));
    pending.autoStart = Boolean(pending.recordingId);
  }

  let cachedRecordingsSource = null;
  let cachedRecordingsDecorated = null;
  function decorateRecordings2(value) {
    if (!value || typeof value.openRecording !== 'function' || value.__vdrSuiteContinueWatchingOpenDecorated) return value;
    if (value === cachedRecordingsSource && cachedRecordingsDecorated) return cachedRecordingsDecorated;
    const copy = {};
    Object.keys(value).forEach(function (key) { copy[key] = value[key]; });
    const open = value.openRecording;
    copy.openRecording = function (recording, options) {
      rememberOpen(recording, options);
      return open.call(value, recording, options);
    };
    copy.__vdrSuiteContinueWatchingOpenDecorated = true;
    cachedRecordingsSource = value;
    cachedRecordingsDecorated = Object.freeze(copy);
    return cachedRecordingsDecorated;
  }

  const recordingsDescriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2') || {
    configurable: true,
    enumerable: true,
    value: global.VdrSuiteRecordings2
  };
  let recordingsValue = recordingsDescriptor.get ? null : recordingsDescriptor.value;
  Object.defineProperty(global, 'VdrSuiteRecordings2', {
    configurable: recordingsDescriptor.configurable !== false,
    enumerable: recordingsDescriptor.enumerable !== false,
    get: function () {
      const value = recordingsDescriptor.get
        ? recordingsDescriptor.get.call(global)
        : recordingsValue;
      return decorateRecordings2(value);
    },
    set: function (value) {
      cachedRecordingsSource = null;
      cachedRecordingsDecorated = null;
      if (recordingsDescriptor.set) recordingsDescriptor.set.call(global, value);
      else recordingsValue = value;
    }
  });

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
    if (typeof owner.snapshot !== 'function' || typeof owner.subscribe !== 'function') return owner;

    let sampleTimer = null;
    let disposed = false;
    let latestSnapshot = owner.snapshot();
    let lastPosition = -1;
    let unsubscribeLifecycle = null;
    let mediaObserver = null;
    const boundMedia = [];

    function snapshotActive(snapshot) {
      const state = snapshot && text(snapshot.state);
      return Boolean(snapshot && snapshot.sessionId) &&
        state !== 'idle' &&
        state !== 'stopped' &&
        state !== 'destroyed' &&
        state !== 'relinquished';
    }
    function canResume() {
      return typeof owner.canResume === 'function' && owner.canResume() === true;
    }
    function readPosition() {
      return typeof owner.position === 'function'
        ? Math.max(0, Math.floor(Number(owner.position()) || 0))
        : 0;
    }
    function readDuration() {
      return typeof owner.duration === 'function'
        ? Math.max(0, Math.floor(Number(owner.duration()) || 0))
        : 0;
    }
    function syncProgress(forceClear, allowInactive) {
      if (disposed && !forceClear) return Promise.resolve(false);
      if (!forceClear && !allowInactive && !snapshotActive(latestSnapshot)) return Promise.resolve(false);
      const position = readPosition();
      const duration = readDuration();
      const complete = forceClear || (duration > 0 && position >= duration);
      if (!complete && (!position || !canResume() || position === lastPosition)) return Promise.resolve(false);
      lastPosition = position;
      return enqueue(complete ? {
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
      }).then(function () { return true; }, function () { return false; });
    }
    function stopSampling() {
      if (sampleTimer !== null && typeof global.clearTimeout === 'function') {
        global.clearTimeout(sampleTimer);
      }
      sampleTimer = null;
    }
    function sample() {
      sampleTimer = null;
      if (disposed || !snapshotActive(latestSnapshot)) return;
      syncProgress(false, false).then(function () {
        if (!disposed && snapshotActive(latestSnapshot) && typeof global.setTimeout === 'function') {
          sampleTimer = global.setTimeout(sample, 5000);
        }
      });
    }
    function ensureSampling() {
      if (sampleTimer === null && !disposed && snapshotActive(latestSnapshot) && typeof global.setTimeout === 'function') {
        sampleTimer = global.setTimeout(sample, 5000);
      }
    }
    function bindEnded() {
      const root = owner.element;
      if (!root || typeof root.querySelectorAll !== 'function') return;
      const media = root.querySelectorAll('video, audio');
      Array.prototype.forEach.call(media || [], function (element) {
        if (!element || boundMedia.indexOf(element) >= 0 || typeof element.addEventListener !== 'function') return;
        boundMedia.push(element);
        element.addEventListener('ended', function () {
          stopSampling();
          syncProgress(true, true);
        });
      });
      if (mediaObserver || typeof global.MutationObserver !== 'function') return;
      mediaObserver = new global.MutationObserver(bindEnded);
      mediaObserver.observe(root, {childList: true, subtree: true});
    }
    function disposeTracking() {
      if (disposed) return;
      disposed = true;
      stopSampling();
      if (mediaObserver && typeof mediaObserver.disconnect === 'function') mediaObserver.disconnect();
      mediaObserver = null;
      if (unsubscribeLifecycle) unsubscribeLifecycle();
      unsubscribeLifecycle = null;
    }
    function lifecycleChanged(snapshot) {
      const previous = latestSnapshot;
      const wasActive = snapshotActive(previous);
      latestSnapshot = snapshot || {};
      const active = snapshotActive(latestSnapshot);
      bindEnded();

      if (wasActive && !active) {
        syncProgress(false, true);
      }
      if (active) ensureSampling();
      else stopSampling();

      const transition = text(latestSnapshot.transition);
      if (transition === 'destroyed' || transition === 'relinquished') {
        disposeTracking();
      }
    }

    const decorated = {};
    Object.keys(owner).forEach(function (key) { decorated[key] = owner[key]; });
    decorated.startAtAbsolute = function (positionSeconds) {
      return startAtAbsolute(owner, positionSeconds);
    };
    decorated.__vdrSuiteContinueWatchingOwner = true;
    const result = Object.freeze(decorated);

    bindEnded();
    unsubscribeLifecycle = owner.subscribe(lifecycleChanged);

    if (pending.autoStart && pending.backendId === backendId && pending.recordingId === recordingId(recording)) {
      const position = pending.positionSeconds;
      pending.autoStart = false;
      if (typeof global.setTimeout === 'function') {
        global.setTimeout(function () {
          result.startAtAbsolute(position).catch(function () {});
        }, 0);
      }
    }
    return result;
  }

  let cachedPlaybackSource = null;
  let cachedPlaybackDecorated = null;
  function decoratePlayback(value) {
    const source = value && typeof value === 'object' ? value : {};
    if (source === cachedPlaybackSource && cachedPlaybackDecorated) return cachedPlaybackDecorated;
    if (typeof source.createPanel !== 'function' || source.__vdrSuiteContinueWatchingDecorated) {
      cachedPlaybackSource = source;
      cachedPlaybackDecorated = source;
      return source;
    }
    const decorated = {};
    Object.keys(source).forEach(function (key) { decorated[key] = source[key]; });
    const factory = source.createPanel;
    decorated.createPanel = function (recording, backendId) {
      return decorateOwner(factory.apply(source, arguments), recording, backendId);
    };
    decorated.__vdrSuiteContinueWatchingDecorated = true;
    cachedPlaybackSource = source;
    cachedPlaybackDecorated = Object.freeze(decorated);
    return cachedPlaybackDecorated;
  }

  Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
    configurable: playbackDescriptor.configurable !== false,
    enumerable: playbackDescriptor.enumerable !== false,
    get: function () {
      return decoratePlayback(playbackDescriptor.get.call(global));
    },
    set: function (value) {
      cachedPlaybackSource = null;
      cachedPlaybackDecorated = null;
      playbackDescriptor.set.call(global, value);
    }
  });

  global.VdrSuiteContinueWatchingSync = Object.freeze({
    __test: Object.freeze({
      startAtAbsolute,
      rememberOpen,
      decorateOwner,
      post,
      enqueue
    })
  });
}(window));
