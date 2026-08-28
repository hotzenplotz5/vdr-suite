// ADR-0057 bounded Recording network interruption recovery policy.
//
// This decorator observes the canonical Recording playback owner and delegates
// every recovery command back to that same owner. Browser online/offline and
// media-element events are only transport-local evidence. They never become a
// second MediaSession, lifecycle or provider authority.
(function (global) {
  'use strict';

  const marker = '__vdrSuiteRecordingNetworkRecoveryBound';
  const RECOVERY_MEDIA_TIMEOUT_MS = 20000;
  if (!global || !global.document || global[marker] === true) return;

  const descriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
  if (!descriptor || typeof descriptor.get !== 'function' || typeof descriptor.set !== 'function') return;

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function safePosition(value) {
    const seconds = Number(value);
    return Number.isFinite(seconds) && seconds >= 0 ? Math.floor(seconds) : 0;
  }

  function browserOnline() {
    return !(global.navigator && global.navigator.onLine === false);
  }

  function find(root, selector) {
    return root && typeof root.querySelector === 'function'
      ? root.querySelector(selector)
      : null;
  }

  function networkFailure(snapshot) {
    const source = snapshot && typeof snapshot === 'object' ? snapshot : {};
    const failure = source.failure && typeof source.failure === 'object'
      ? source.failure
      : null;
    if (source.state !== 'stopped' || source.transport !== 'progressive-fmp4' || !failure) {
      return false;
    }
    if (failure.category !== 'transport' || failure.terminal !== true) return false;
    if (failure.origin !== 'client-transport' && failure.origin !== 'platform-player') return false;
    return failure.reasonCode === 'client_transport_failed' ||
      failure.reasonCode === 'client_stream_fetch_failed' ||
      failure.reasonCode === 'client_media_network_error';
  }

  function guardPlayback(value) {
    const guard = global.VdrSuiteRecordingNetworkRecoveryGuard;
    return guard && typeof guard.guardPlayback === 'function'
      ? guard.guardPlayback(value)
      : value;
  }

  function decoratePanel(panel, recording, backendId) {
    if (!panel || !panel.element ||
        typeof panel.start !== 'function' ||
        typeof panel.play !== 'function' ||
        typeof panel.pause !== 'function' ||
        typeof panel.stop !== 'function' ||
        typeof panel.seekAbsolute !== 'function' ||
        typeof panel.position !== 'function' ||
        typeof panel.snapshot !== 'function' ||
        typeof panel.subscribe !== 'function') {
      return panel;
    }
    if (panel.__vdrSuiteRecordingNetworkRecoveryDecorated === true) return panel;

    const shell = panel.element;
    let disposed = false;
    let offlineEvidence = !browserOnline();
    let networkEpoch = offlineEvidence ? 1 : 0;
    let attemptedEpoch = -1;
    let armed = false;
    let recoveryInFlight = false;
    let awaitingRecoveredMedia = false;
    let interruptedPosition = 0;
    let lastPosition = safePosition(panel.position());
    let boundVideo = null;
    let mediaWaitResolve = null;
    let mediaWaitReject = null;
    let mediaWaitTimer = null;
    let observer = null;
    let unsubscribe = null;

    function statusNode() {
      return find(shell, '.recordings2-playback-status');
    }

    function setStatus(message, error) {
      const status = statusNode();
      if (!status) return;
      status.textContent = message;
      if (status.classList && typeof status.classList.toggle === 'function') {
        status.classList.toggle('error', Boolean(error));
      }
    }

    function clearMediaWait() {
      if (mediaWaitTimer !== null && typeof global.clearTimeout === 'function') {
        try { global.clearTimeout(mediaWaitTimer); } catch (error) {}
      }
      mediaWaitTimer = null;
      mediaWaitResolve = null;
      mediaWaitReject = null;
    }

    function resolveMediaWait() {
      const resolve = mediaWaitResolve;
      clearMediaWait();
      if (resolve) resolve(true);
    }

    function rejectMediaWait(error) {
      const reject = mediaWaitReject;
      clearMediaWait();
      if (reject) reject(error || new Error('Wiedergabe konnte nicht fortgesetzt werden.'));
    }

    function waitForRecoveredMedia() {
      return new Promise(function (resolve, reject) {
        clearMediaWait();
        mediaWaitResolve = resolve;
        mediaWaitReject = reject;
        if (typeof global.setTimeout === 'function') {
          mediaWaitTimer = global.setTimeout(function () {
            rejectMediaWait(new Error('Zeitüberschreitung beim Wiederherstellen der Wiedergabe.'));
          }, RECOVERY_MEDIA_TIMEOUT_MS);
        }
      });
    }

    function ownerState() {
      try {
        const snapshot = panel.snapshot();
        return snapshot && snapshot.state ? snapshot.state : '';
      } catch (error) {
        return '';
      }
    }

    function trackPosition() {
      if (disposed) return;
      const state = ownerState();
      if (state !== 'playing' && state !== 'paused' && state !== 'seeking') return;
      const position = safePosition(panel.position());
      if (position >= 0) lastPosition = position;
      if (browserOnline() && !armed && !recoveryInFlight) {
        offlineEvidence = false;
      }
    }

    function handlePlaying() {
      trackPosition();
      if (!recoveryInFlight || !awaitingRecoveredMedia || !browserOnline()) return;
      setStatus('Verbindung wiederhergestellt · Aufnahme läuft weiter.', false);
      resolveMediaWait();
    }

    function bindVideo() {
      const video = find(shell, 'video');
      if (video === boundVideo) return false;
      if (boundVideo && typeof boundVideo.removeEventListener === 'function') {
        try { boundVideo.removeEventListener('timeupdate', trackPosition); } catch (error) {}
        try { boundVideo.removeEventListener('playing', handlePlaying); } catch (error) {}
      }
      boundVideo = video || null;
      if (!video || typeof video.addEventListener !== 'function') return true;
      video.addEventListener('timeupdate', trackPosition);
      video.addEventListener('playing', handlePlaying);
      return true;
    }

    function stopRecoverySession() {
      const state = ownerState();
      if (state === 'stopped' || state === 'destroyed' || state === 'fallback' || state === 'idle') {
        return Promise.resolve(false);
      }
      try {
        return Promise.resolve(panel.stop()).catch(function () { return false; });
      } catch (error) {
        return Promise.resolve(false);
      }
    }

    function recoverySequence() {
      let startedSessionId = '';
      return Promise.resolve(panel.start()).then(function (sessionId) {
        startedSessionId = text(sessionId).trim();
        if (!startedSessionId) {
          throw new Error('Neue Recording-MediaSession wurde nicht bereitgestellt.');
        }
        if (!browserOnline()) {
          throw new Error('Netzwerk wurde während der Wiederherstellung erneut unterbrochen.');
        }
        if (panel.pause() !== true) {
          throw new Error('Neue Recording-MediaSession konnte vor dem Resume nicht pausiert werden.');
        }
        if (interruptedPosition <= 0) return true;
        return panel.seekAbsolute(interruptedPosition);
      }).then(function () {
        if (!startedSessionId || !browserOnline()) {
          throw new Error('Netzwerk wurde während der Wiederherstellung erneut unterbrochen.');
        }
        awaitingRecoveredMedia = true;
        const mediaReady = waitForRecoveredMedia();
        return Promise.resolve(panel.play()).then(function () {
          return mediaReady;
        }, function (error) {
          rejectMediaWait(error);
          throw error;
        });
      }).catch(function (error) {
        rejectMediaWait(error);
        return stopRecoverySession().then(function () {
          throw error;
        });
      });
    }

    function finishRecoverySuccess() {
      armed = false;
      recoveryInFlight = false;
      awaitingRecoveredMedia = false;
      offlineEvidence = false;
      interruptedPosition = 0;
      attemptedEpoch = networkEpoch;
      trackPosition();
    }

    function finishRecoveryFailure(error) {
      recoveryInFlight = false;
      awaitingRecoveredMedia = false;
      if (!browserOnline()) {
        armed = true;
        offlineEvidence = true;
        setStatus(
          'Verbindung unterbrochen · Wiedergabe wird fortgesetzt, sobald das Netzwerk wieder verfügbar ist.',
          false
        );
        return;
      }
      armed = false;
      offlineEvidence = false;
      const detail = error && error.message ? ': ' + error.message : '';
      setStatus(
        'Aufnahme-Wiedergabe konnte nach der Netzwerkunterbrechung nicht automatisch fortgesetzt werden' + detail,
        true
      );
    }

    function recover() {
      if (disposed || !armed || recoveryInFlight || !browserOnline()) return Promise.resolve(false);
      if (attemptedEpoch === networkEpoch) return Promise.resolve(false);

      const guard = global.VdrSuiteRecordingNetworkRecoveryGuard;
      if (!guard || typeof guard.withoutCompatibilityFallback !== 'function') {
        armed = false;
        setStatus('Automatische Wiederherstellung ist nicht verfügbar.', true);
        return Promise.resolve(false);
      }

      attemptedEpoch = networkEpoch;
      offlineEvidence = false;
      recoveryInFlight = true;
      awaitingRecoveredMedia = false;
      setStatus('Verbindung wiederhergestellt · Wiedergabe wird fortgesetzt …', false);

      return guard.withoutCompatibilityFallback(recording, backendId, recoverySequence).then(function () {
        if (disposed) return false;
        finishRecoverySuccess();
        return true;
      }).catch(function (error) {
        if (!disposed) finishRecoveryFailure(error);
        return false;
      });
    }

    function armRecovery() {
      interruptedPosition = lastPosition;
      armed = true;
      recoveryInFlight = false;
      awaitingRecoveredMedia = false;
      setStatus(
        browserOnline()
          ? 'Netzwerkunterbrechung erkannt · Wiedergabe wird fortgesetzt …'
          : 'Verbindung unterbrochen · Wiedergabe wird fortgesetzt, sobald das Netzwerk wieder verfügbar ist.',
        false
      );
      if (browserOnline()) recover();
    }

    function ownerChanged(snapshot) {
      if (disposed) return;
      if (networkFailure(snapshot) && offlineEvidence) {
        armRecovery();
        return;
      }
      if (recoveryInFlight && awaitingRecoveredMedia && snapshot && snapshot.state === 'stopped') {
        rejectMediaWait(new Error('Wiederhergestellte Wiedergabe wurde vor dem ersten Frame gestoppt.'));
      }
    }

    function wentOffline() {
      if (disposed) return;
      if (browserOnline()) return;
      if (!offlineEvidence) networkEpoch += 1;
      offlineEvidence = true;
      if (recoveryInFlight) {
        setStatus(
          'Verbindung erneut unterbrochen · Wiedergabe wartet auf das Netzwerk.',
          false
        );
      }
    }

    function cameOnline() {
      if (disposed || !browserOnline()) return;
      if (armed && !recoveryInFlight) recover();
    }

    bindVideo();
    unsubscribe = panel.subscribe(ownerChanged);
    if (typeof global.addEventListener === 'function') {
      global.addEventListener('offline', wentOffline);
      global.addEventListener('online', cameOnline);
    }
    if (typeof global.MutationObserver === 'function') {
      observer = new global.MutationObserver(function () { bindVideo(); });
      observer.observe(shell, {childList: true, subtree: true});
    }

    function destroy() {
      if (disposed) return;
      disposed = true;
      armed = false;
      recoveryInFlight = false;
      awaitingRecoveredMedia = false;
      rejectMediaWait(new Error('Playback owner destroyed.'));
      if (typeof unsubscribe === 'function') unsubscribe();
      unsubscribe = null;
      if (observer && typeof observer.disconnect === 'function') observer.disconnect();
      observer = null;
      if (typeof global.removeEventListener === 'function') {
        global.removeEventListener('offline', wentOffline);
        global.removeEventListener('online', cameOnline);
      }
      if (boundVideo && typeof boundVideo.removeEventListener === 'function') {
        try { boundVideo.removeEventListener('timeupdate', trackPosition); } catch (error) {}
        try { boundVideo.removeEventListener('playing', handlePlaying); } catch (error) {}
      }
      boundVideo = null;
      if (typeof panel.destroy === 'function') return panel.destroy();
    }

    const wrapped = {};
    Object.keys(panel).forEach(function (key) { wrapped[key] = panel[key]; });
    wrapped.destroy = destroy;
    wrapped.__vdrSuiteRecordingNetworkRecoveryDecorated = true;
    return Object.freeze(wrapped);
  }

  let cachedSource = null;
  let cachedDecorated = null;

  function decoratePlayback(value) {
    const source = value && typeof value === 'object' ? value : {};
    if (source === cachedSource && cachedDecorated) return cachedDecorated;
    if (typeof source.createPanel !== 'function' ||
        source.__vdrSuiteRecordingNetworkRecoveryDecorated === true) {
      cachedSource = source;
      cachedDecorated = source;
      return source;
    }

    const decorated = {};
    Object.keys(source).forEach(function (key) { decorated[key] = source[key]; });
    const factory = source.createPanel;
    decorated.createPanel = function (recording, backendId) {
      return decoratePanel(factory.apply(source, arguments), recording, backendId);
    };
    decorated.__vdrSuiteRecordingNetworkRecoveryDecorated = true;
    cachedSource = source;
    cachedDecorated = Object.freeze(decorated);
    return cachedDecorated;
  }

  Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
    configurable: descriptor.configurable !== false,
    enumerable: descriptor.enumerable !== false,
    get: function () { return decoratePlayback(descriptor.get.call(global)); },
    set: function (value) {
      cachedSource = null;
      cachedDecorated = null;
      descriptor.set.call(global, guardPlayback(value));
    }
  });

  global[marker] = true;
  global.VdrSuiteRecordingNetworkRecovery = Object.freeze({
    networkFailure: networkFailure
  });
}(window));
