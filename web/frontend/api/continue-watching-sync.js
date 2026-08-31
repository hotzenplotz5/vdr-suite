(function (global) {
  'use strict';

  const playbackDescriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
  if (!playbackDescriptor || typeof playbackDescriptor.get !== 'function' || typeof playbackDescriptor.set !== 'function') {
    return;
  }

  const RESUME_READY_POLL_MS = 100;
  const RESUME_READY_MAX_ATTEMPTS = 300;
  const CONTINUE_ENDPOINT = '/api/media/continue-watching';
  const HISTORY_ENDPOINT = '/api/media/recently-watched';
  const pending = {backendId: '', recordingId: '', positionSeconds: 0, autoStart: false};
  let operationCounter = 0;
  let historyOperationCounter = 0;
  let mutationQueue = Promise.resolve();
  let historyMutationQueue = Promise.resolve();

  function text(value) { return value == null ? '' : String(value); }
  function recordingId(recording) { return text(recording && (recording.id || recording.recordingId)); }
  function nextOperationId(prefix) {
    operationCounter += 1;
    return 'cw-' + prefix + '-' + Date.now().toString(36) + '-' + operationCounter.toString(36);
  }
  function nextHistoryOperationId(prefix) {
    historyOperationCounter += 1;
    return 'rw-' + prefix + '-' + Date.now().toString(36) + '-' + historyOperationCounter.toString(36);
  }
  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }
  function postTo(endpoint, body, errorText) {
    const fetcher = global.fetch || fetch;
    return fetcher(endpoint, {
      method: 'POST',
      credentials: 'same-origin',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify(body)
    }).then(function (response) {
      if (!response || !response.ok) throw new Error(errorText);
      return true;
    });
  }
  function post(body) {
    return postTo(CONTINUE_ENDPOINT, body, 'continue watching sync failed');
  }
  function postHistory(body) {
    return postTo(HISTORY_ENDPOINT, body, 'recently watched sync failed');
  }
  function enqueue(body) {
    const operation = mutationQueue.catch(function () {}).then(function () { return post(body); });
    mutationQueue = operation.catch(function () {});
    return operation;
  }
  function enqueueHistory(body) {
    const operation = historyMutationQueue.catch(function () {}).then(function () { return postHistory(body); });
    historyMutationQueue = operation.catch(function () {});
    return operation;
  }
  function clearCurrent(backendId, currentRecordingId) {
    const id = text(currentRecordingId);
    if (!id) return Promise.resolve(false);
    return enqueue({
      operation: 'clear',
      backendId: text(backendId) || 'default',
      recordingId: id,
      operationId: nextOperationId('clear')
    }).then(function () { return true; }, function () { return false; });
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
      const value = recordingsDescriptor.get ? recordingsDescriptor.get.call(global) : recordingsValue;
      return decorateRecordings2(value);
    },
    set: function (value) {
      cachedRecordingsSource = null;
      cachedRecordingsDecorated = null;
      if (recordingsDescriptor.set) recordingsDescriptor.set.call(global, value);
      else recordingsValue = value;
    }
  });

  function ownerState(owner) {
    if (!owner || typeof owner.state !== 'function') return '';
    try { return text(owner.state()); } catch (error) { return ''; }
  }

  function waitForResumeReady(owner, attempt) {
    if (!owner || typeof owner.canResume !== 'function') return Promise.resolve(true);
    try {
      if (owner.canResume() === true) return Promise.resolve(true);
    } catch (error) {}

    const state = ownerState(owner);
    if (state === 'stopped' || state === 'destroyed' || state === 'relinquished') {
      return Promise.reject(new Error('Canonical Recording owner stopped before resume became ready.'));
    }

    const currentAttempt = Math.max(0, Math.floor(Number(attempt) || 0));
    if (currentAttempt >= RESUME_READY_MAX_ATTEMPTS || typeof global.setTimeout !== 'function') {
      return Promise.reject(new Error('Canonical Recording resume did not become ready in time.'));
    }

    return new Promise(function (resolve) {
      global.setTimeout(resolve, RESUME_READY_POLL_MS);
    }).then(function () {
      return waitForResumeReady(owner, currentAttempt + 1);
    });
  }

  function startAtAbsolute(owner, positionSeconds) {
    const target = Math.max(0, Math.floor(Number(positionSeconds) || 0));
    if (!owner) return Promise.reject(new Error('Canonical Recording owner is unavailable.'));
    if (target === 0) {
      if (typeof owner.start !== 'function') return Promise.reject(new Error('Canonical Recording start is unavailable.'));
      return Promise.resolve(owner.start());
    }
    if (typeof owner.startAtAbsolute === 'function') return Promise.resolve(owner.startAtAbsolute(target));
    if (typeof owner.start === 'function' && typeof owner.seekAbsolute === 'function') {
      return Promise.resolve(owner.start()).then(function (sessionId) {
        if (!sessionId) throw new Error('Canonical Recording session did not start.');
        return waitForResumeReady(owner, 0);
      }).then(function () { return owner.seekAbsolute(target); });
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
    let lastHistoryPosition = -1;
    let historyObserved = false;
    let historyStartTimer = null;
    let unsubscribeLifecycle = null;
    let mediaObserver = null;
    const boundMedia = [];

    function snapshotActive(snapshot) {
      const state = snapshot && text(snapshot.state);
      return Boolean(snapshot && snapshot.sessionId) &&
        state !== 'idle' && state !== 'stopped' && state !== 'destroyed' && state !== 'relinquished';
    }
    function canResume() {
      return typeof owner.canResume === 'function' && owner.canResume() === true;
    }
    function readPosition() {
      return typeof owner.position === 'function'
        ? Math.max(0, Math.floor(Number(owner.position()) || 0)) : 0;
    }
    function readDuration() {
      return typeof owner.duration === 'function'
        ? Math.max(0, Math.floor(Number(owner.duration()) || 0)) : 0;
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
        operation: 'clear', backendId, recordingId: recordingId(recording), operationId: nextOperationId('clear')
      } : {
        operation: 'progress', backendId, recordingId: recordingId(recording), positionSeconds: position,
        resumeSupported: true, operationId: nextOperationId('progress')
      }).then(function () { return true; }, function () { return false; });
    }
    function syncHistory(ended, allowInactive) {
      if (disposed && !ended) return Promise.resolve(false);
      if (!ended && !allowInactive && !snapshotActive(latestSnapshot)) return Promise.resolve(false);
      const id = recordingId(recording);
      if (!id) return Promise.resolve(false);
      const positionKnown = typeof owner.position === 'function';
      const position = readPosition();
      if (!ended && positionKnown && historyObserved && position === lastHistoryPosition) return Promise.resolve(false);
      if (!ended && !positionKnown && historyObserved) return Promise.resolve(false);
      historyObserved = true;
      lastHistoryPosition = position;
      const resumeSupportKnown = typeof owner.canResume === 'function';
      let resumeSupported = false;
      if (resumeSupportKnown) {
        try { resumeSupported = owner.canResume() === true; } catch (error) {}
      }
      const body = {
        operation: 'activity', backendId, recordingId: id,
        resumeSupportKnown, resumeSupported, ended: ended === true,
        operationId: nextHistoryOperationId(ended ? 'ended' : 'activity')
      };
      if (positionKnown) body.positionSeconds = position;
      return enqueueHistory(body).then(function () { return true; }, function () { return false; });
    }
    function stopSampling() {
      if (sampleTimer !== null && typeof global.clearTimeout === 'function') global.clearTimeout(sampleTimer);
      sampleTimer = null;
    }
    function stopHistoryStart() {
      if (historyStartTimer !== null && typeof global.clearTimeout === 'function') global.clearTimeout(historyStartTimer);
      historyStartTimer = null;
    }
    function sample() {
      sampleTimer = null;
      if (disposed || !snapshotActive(latestSnapshot)) return;
      Promise.all([syncProgress(false, false), syncHistory(false, false)]).then(function () {
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
    function ensureInitialHistory() {
      if (historyObserved || historyStartTimer !== null || disposed || !snapshotActive(latestSnapshot) ||
          typeof global.setTimeout !== 'function') return;
      historyStartTimer = global.setTimeout(function () {
        historyStartTimer = null;
        syncHistory(false, false);
      }, 0);
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
          stopHistoryStart();
          syncProgress(true, true);
          syncHistory(true, true);
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
      stopHistoryStart();
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
        stopHistoryStart();
        syncProgress(false, true);
        if (historyObserved) syncHistory(false, true);
      }
      if (active) {
        ensureInitialHistory();
        ensureSampling();
      } else stopSampling();

      const transition = text(latestSnapshot.transition);
      if (transition === 'destroyed' || transition === 'relinquished') disposeTracking();
    }

    const decorated = {};
    Object.keys(owner).forEach(function (key) { decorated[key] = owner[key]; });
    decorated.startAtAbsolute = function (positionSeconds) { return startAtAbsolute(owner, positionSeconds); };
    decorated.__vdrSuiteContinueWatchingOwner = true;
    const result = Object.freeze(decorated);

    bindEnded();
    unsubscribeLifecycle = owner.subscribe(lifecycleChanged);

    if (pending.autoStart && pending.backendId === backendId && pending.recordingId === recordingId(recording)) {
      const position = pending.positionSeconds;
      pending.autoStart = false;
      if (typeof global.setTimeout === 'function') {
        global.setTimeout(function () { result.startAtAbsolute(position).catch(function () {}); }, 0);
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
    get: function () { return decoratePlayback(playbackDescriptor.get.call(global)); },
    set: function (value) {
      cachedPlaybackSource = null;
      cachedPlaybackDecorated = null;
      playbackDescriptor.set.call(global, value);
    }
  });

  global.VdrSuiteContinueWatchingSync = Object.freeze({
    clear: clearCurrent,
    __test: Object.freeze({
      startAtAbsolute,
      waitForResumeReady,
      rememberOpen,
      decorateOwner,
      post,
      postHistory,
      enqueue,
      enqueueHistory,
      clearCurrent
    })
  });
}(window));
