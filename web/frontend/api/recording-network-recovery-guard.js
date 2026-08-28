// ADR-0057 Recording network interruption recovery guard.
//
// The established fast Recording owner owns startup compatibility fallback.
// Automatic network recovery must reuse that same owner, but a failed recovery
// attempt after already-established playback must not silently switch to HLS.
// This helper therefore provides one narrow, recording-scoped suppression
// around the existing compatibility factory. It never creates a MediaSession,
// owns a transport or starts recovery by itself.
(function (global) {
  'use strict';

  const marker = '__vdrSuiteRecordingNetworkRecoveryGuardBound';
  if (!global || global[marker] === true) return;

  const scopes = [];

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function recordingId(recording) {
    if (!recording || typeof recording !== 'object') return '';
    return text(recording.recordingId || recording.id).trim();
  }

  function backendId(value) {
    return text(value || 'default').trim() || 'default';
  }

  function scopeMatches(scope, recording, backend) {
    if (!scope) return false;
    return scope.recordingId === recordingId(recording) &&
      scope.backendId === backendId(backend);
  }

  function suppressed(recording, backend) {
    for (let index = scopes.length - 1; index >= 0; index -= 1) {
      if (scopeMatches(scopes[index], recording, backend)) return true;
    }
    return false;
  }

  function guardPlayback(value) {
    const source = value && typeof value === 'object' ? value : null;
    if (!source || source.__vdrSuiteRecordingNetworkRecoveryGuarded === true ||
        typeof source.createPanel !== 'function') {
      return source;
    }

    const decorated = {};
    Object.keys(source).forEach(function (key) { decorated[key] = source[key]; });
    const factory = source.createPanel;
    decorated.createPanel = function (recording, backend) {
      if (suppressed(recording, backend)) return null;
      return factory.apply(source, arguments);
    };
    decorated.__vdrSuiteRecordingNetworkRecoveryGuarded = true;
    return Object.freeze(decorated);
  }

  let currentPlayback = guardPlayback(global.VdrSuiteRecordings2Playback);

  try {
    Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
      configurable: true,
      enumerable: true,
      get: function () { return currentPlayback; },
      set: function (value) { currentPlayback = guardPlayback(value); }
    });
  } catch (error) {
    if (global.VdrSuiteRecordings2Playback) {
      global.VdrSuiteRecordings2Playback = currentPlayback;
    }
  }

  function withoutCompatibilityFallback(recording, backend, callback) {
    if (typeof callback !== 'function') {
      return Promise.reject(new Error('Recovery callback is required.'));
    }

    const scope = {
      recordingId: recordingId(recording),
      backendId: backendId(backend)
    };
    if (!scope.recordingId) {
      return Promise.reject(new Error('Recording identity is required for recovery.'));
    }

    scopes.push(scope);
    let result;
    try {
      result = callback();
    } catch (error) {
      scopes.pop();
      return Promise.reject(error);
    }

    return Promise.resolve(result).then(function (value) {
      scopes.pop();
      return value;
    }, function (error) {
      scopes.pop();
      throw error;
    });
  }

  global[marker] = true;
  global.VdrSuiteRecordingNetworkRecoveryGuard = Object.freeze({
    guardPlayback: guardPlayback,
    withoutCompatibilityFallback: withoutCompatibilityFallback,
    isCompatibilityFallbackSuppressed: suppressed
  });
}(window));
