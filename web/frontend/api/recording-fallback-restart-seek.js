// Indexed restart-seek adapter for the Recording HLS compatibility owner.
// The rolling HLS transport itself is not random-seekable. Once the Recording
// index is ready, a seek is implemented truthfully by stopping that owner and
// opening a fresh HLS MediaSession at the requested absolute Recording time.
(function (global) {
  'use strict';

  const marker = '__vdrSuiteRecordingFallbackRestartSeekBound';
  const CAPABILITY_POLL_MS = 500;
  if (!global || !global.document || global[marker] === true) return;

  const descriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
  if (!descriptor || typeof descriptor.get !== 'function' || typeof descriptor.set !== 'function') return;

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function formatTime(value) {
    const seconds = Math.max(0, Math.floor(Number(value) || 0));
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const remaining = seconds % 60;
    return String(hours).padStart(2, '0') + ':' +
      String(minutes).padStart(2, '0') + ':' +
      String(remaining).padStart(2, '0');
  }

  function parseTime(value) {
    const raw = text(value).trim();
    if (!raw) return null;
    if (/^\d+$/.test(raw)) return Number(raw);
    const parts = raw.split(':');
    if (parts.length < 2 || parts.length > 3 ||
        parts.some(function (part) { return !/^\d+$/.test(part); })) return null;
    const numbers = parts.map(Number);
    const hours = parts.length === 3 ? numbers[0] : 0;
    const minutes = parts.length === 3 ? numbers[1] : numbers[0];
    const seconds = parts.length === 3 ? numbers[2] : numbers[1];
    if (minutes >= 60 || seconds >= 60) return null;
    return hours * 3600 + minutes * 60 + seconds;
  }

  function find(root, selector) {
    return root && typeof root.querySelector === 'function'
      ? root.querySelector(selector)
      : null;
  }

  function decoratePanel(playback) {
    if (!playback || !playback.element ||
        typeof playback.start !== 'function' ||
        typeof playback.stop !== 'function' ||
        typeof playback.resume !== 'function' ||
        typeof playback.canResume !== 'function' ||
        typeof playback.position !== 'function' ||
        typeof playback.duration !== 'function' ||
        typeof playback.state !== 'function') return playback;
    if (playback.__vdrSuiteFallbackRestartSeekDecorated === true) return playback;

    const panel = playback.element;
    const back60Button = find(panel, 'button[aria-label="60 Sekunden zurück"]');
    const back10Button = find(panel, 'button[aria-label="10 Sekunden zurück"]');
    const forward10Button = find(panel, 'button[aria-label="10 Sekunden vor"]');
    const forward60Button = find(panel, 'button[aria-label="60 Sekunden vor"]');
    const timeline = find(panel, 'input[aria-label="Wiedergabeposition"]');
    const directTime = find(panel, 'input[aria-label="Direkte Wiedergabezeit"]');
    const directButton = find(panel, 'button[aria-label="Zur eingegebenen Wiedergabezeit springen"]');
    const positionLabel = find(panel, '.recordings2-playback-position');
    const notice = find(panel, '.recordings2-fallback-contract-status');
    if (!back60Button || !back10Button || !forward10Button || !forward60Button ||
        !timeline || !directTime || !directButton || !positionLabel) return playback;

    // Once restart-seek is active, it owns the seek-control disabled state.
    // Briefly disabling a focused input makes desktop browsers drop its focus.
    panel.__vdrSuiteFallbackRestartSeekControlsOwned = true;

    let seekInFlight = false;
    let timelineDragging = false;
    let capabilityTimer = null;
    let observedVideo = null;
    let observer = null;

    function state() {
      return text(playback.state());
    }

    function duration() {
      const value = Number(playback.duration());
      return Number.isFinite(value) && value > 0 ? Math.floor(value) : 0;
    }

    function position() {
      const value = Number(playback.position());
      return Number.isFinite(value) && value >= 0 ? Math.floor(value) : 0;
    }

    function active() {
      const value = state();
      return value === 'playing' || value === 'paused';
    }

    function canSeek() {
      if (!active() || seekInFlight || duration() <= 0) return false;
      try { return playback.canResume() === true; } catch (error) { return false; }
    }

    function maximum() {
      return Math.max(0, duration() - 1);
    }

    function clampTarget(value) {
      const target = Math.floor(Number(value));
      if (!Number.isFinite(target) || duration() <= 0) return null;
      return Math.max(0, Math.min(maximum(), target));
    }

    function setNotice(message, error) {
      if (!notice) return;
      if (notice.textContent !== message) notice.textContent = message;
      if (notice.classList && typeof notice.classList.toggle === 'function') {
        notice.classList.toggle('error', Boolean(error));
      }
    }

    function syncControls(previewPosition) {
      const enabled = canSeek();
      [back60Button, back10Button, forward10Button, forward60Button, timeline, directTime, directButton]
        .forEach(function (control) { control.disabled = !enabled; });
      timeline.min = '0';
      timeline.max = String(maximum());
      timeline.step = '1';
      if (enabled && previewPosition === undefined && !timelineDragging) {
        timeline.value = String(Math.max(0, Math.min(maximum(), position())));
      }
      if (previewPosition !== undefined) {
        const preview = clampTarget(previewPosition);
        if (preview !== null) {
          positionLabel.textContent = formatTime(preview) + ' / ' + formatTime(duration());
        }
      }
      if (enabled && !seekInFlight && notice && active()) {
        setNotice('Kompatibilitätsmodus · Zeit-Sprung verfügbar · Stream wird beim Springen neu aufgebaut.', false);
      }
    }

    function clearCapabilityPoll() {
      if (capabilityTimer !== null && typeof global.clearTimeout === 'function') {
        global.clearTimeout(capabilityTimer);
      }
      capabilityTimer = null;
    }

    function scheduleCapabilityPoll() {
      if (!active() || canSeek() || capabilityTimer !== null ||
          typeof global.setTimeout !== 'function') return;
      capabilityTimer = global.setTimeout(function () {
        capabilityTimer = null;
        bindVideo();
        syncControls();
        if (active() && !canSeek()) scheduleCapabilityPoll();
      }, CAPABILITY_POLL_MS);
    }

    function handlePlaybackEvent() {
      syncControls();
      scheduleCapabilityPoll();
    }

    function bindVideo() {
      const video = find(panel, 'video');
      if (!video || video === observedVideo || typeof video.addEventListener !== 'function') return;
      observedVideo = video;
      ['play', 'pause', 'playing', 'timeupdate', 'loadedmetadata']
        .forEach(function (name) { video.addEventListener(name, handlePlaybackEvent); });
    }

    function restartAt(value) {
      const target = clampTarget(value);
      if (target === null || !canSeek()) {
        return Promise.reject(new Error('Zeit-Sprung ist für diese Aufnahme noch nicht verfügbar.'));
      }
      const wasPaused = state() === 'paused';
      timelineDragging = false;
      seekInFlight = true;
      clearCapabilityPoll();
      syncControls();
      setNotice('Springe zu ' + formatTime(target) + ' · HLS-Stream wird neu aufgebaut …', false);

      return Promise.resolve(playback.stop()).then(function (stopped) {
        if (stopped === false || state() !== 'stopped') {
          throw new Error('Der bisherige HLS-Stream konnte nicht sauber gestoppt werden.');
        }
        return target > 0 ? playback.resume(target) : playback.start();
      }).then(function (sessionId) {
        if (!sessionId) throw new Error('Die neue HLS-MediaSession wurde nicht gestartet.');
        if (wasPaused && typeof playback.pause === 'function') playback.pause();
        seekInFlight = false;
        bindVideo();
        syncControls();
        scheduleCapabilityPoll();
        setNotice(
          wasPaused
            ? 'Sprung abgeschlossen · Wiedergabe bleibt pausiert.'
            : 'Sprung abgeschlossen · Wiedergabe wird fortgesetzt.',
          false
        );
        return true;
      }).catch(function (error) {
        seekInFlight = false;
        timelineDragging = false;
        syncControls();
        setNotice(
          error && error.message ? 'Zeit-Sprung fehlgeschlagen: ' + error.message : 'Zeit-Sprung fehlgeschlagen.',
          true
        );
        throw error;
      });
    }

    function seekAbsolute(value) {
      return restartAt(value);
    }

    function seekRelative(deltaSeconds) {
      const delta = Number(deltaSeconds);
      if (!Number.isFinite(delta)) {
        return Promise.reject(new Error('Ungültiger relativer Zeitsprung.'));
      }
      return restartAt(position() + delta);
    }

    back60Button.addEventListener('click', function () { seekRelative(-60).catch(function () {}); });
    back10Button.addEventListener('click', function () { seekRelative(-10).catch(function () {}); });
    forward10Button.addEventListener('click', function () { seekRelative(10).catch(function () {}); });
    forward60Button.addEventListener('click', function () { seekRelative(60).catch(function () {}); });
    timeline.addEventListener('input', function () {
      if (timeline.disabled) return;
      timelineDragging = true;
      syncControls(Number(timeline.value));
    });
    timeline.addEventListener('change', function () {
      const target = Number(timeline.value);
      timelineDragging = false;
      if (timeline.disabled) {
        syncControls();
        return;
      }
      seekAbsolute(target).catch(function () { syncControls(); });
    });
    timeline.addEventListener('pointercancel', function () {
      timelineDragging = false;
      syncControls();
    });
    directButton.addEventListener('click', function () {
      const target = parseTime(directTime.value);
      if (target === null) {
        setNotice('Ungültige Zeit. Erwartet werden Sekunden, MM:SS oder HH:MM:SS.', true);
        return;
      }
      seekAbsolute(target).then(function () {
        directTime.value = formatTime(clampTarget(target));
      }).catch(function () {});
    });
    directTime.addEventListener('keydown', function (event) {
      if (event && event.key === 'Enter' && typeof directButton.click === 'function') directButton.click();
    });

    if (typeof global.MutationObserver === 'function') {
      observer = new global.MutationObserver(function () {
        bindVideo();
        syncControls();
        scheduleCapabilityPoll();
      });
      observer.observe(panel, {childList: true, subtree: true});
    }

    function start() {
      timelineDragging = false;
      return Promise.resolve(playback.start()).then(function (sessionId) {
        bindVideo();
        syncControls();
        scheduleCapabilityPoll();
        return sessionId;
      });
    }

    function resume(value) {
      timelineDragging = false;
      return Promise.resolve(playback.resume(value)).then(function (sessionId) {
        bindVideo();
        syncControls();
        scheduleCapabilityPoll();
        return sessionId;
      });
    }

    function destroy() {
      timelineDragging = false;
      clearCapabilityPoll();
      if (observer && typeof observer.disconnect === 'function') observer.disconnect();
      observer = null;
      if (typeof playback.destroy === 'function') playback.destroy();
    }

    bindVideo();
    syncControls();
    scheduleCapabilityPoll();

    const wrapped = {};
    Object.keys(playback).forEach(function (key) { wrapped[key] = playback[key]; });
    wrapped.start = start;
    wrapped.resume = resume;
    wrapped.destroy = destroy;
    wrapped.seekAbsolute = seekAbsolute;
    wrapped.seekRelative = seekRelative;
    wrapped.__vdrSuiteFallbackRestartSeekDecorated = true;
    return Object.freeze(wrapped);
  }

  function decoratePlayback(value) {
    const source = value && typeof value === 'object' ? value : {};
    if (typeof source.createPanel !== 'function' || source.__vdrSuiteFallbackRestartSeekDecorated === true) return source;
    const decorated = {};
    Object.keys(source).forEach(function (key) { decorated[key] = source[key]; });
    const factory = source.createPanel;
    decorated.createPanel = function () {
      return decoratePanel(factory.apply(source, arguments));
    };
    decorated.__vdrSuiteFallbackRestartSeekDecorated = true;
    return Object.freeze(decorated);
  }

  Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
    configurable: descriptor.configurable !== false,
    enumerable: descriptor.enumerable !== false,
    get: function () { return descriptor.get.call(global); },
    set: function (value) { descriptor.set.call(global, decoratePlayback(value)); }
  });

  global[marker] = true;
  global.VdrSuiteRecordingFallbackRestartSeek = Object.freeze({
    __test: Object.freeze({formatTime: formatTime, parseTime: parseTime})
  });
}(window));
