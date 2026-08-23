// Preserve Recording playback controls when the fast continuous-fMP4 path
// falls back to the legacy HLS/MSE transport.
(function (global) {
  'use strict';

  const marker = '__vdrSuiteRecordingFallbackControlsBound';
  if (!global || !global.document || global[marker] === true) return;

  const descriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
  if (!descriptor || typeof descriptor.get !== 'function' || typeof descriptor.set !== 'function') {
    return;
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

  function find(root, selector) {
    return root && typeof root.querySelector === 'function'
      ? root.querySelector(selector)
      : null;
  }

  function createButton(label, title) {
    const button = global.document.createElement('button');
    button.type = 'button';
    button.textContent = label;
    button.title = title;
    button.setAttribute('aria-label', title);
    return button;
  }

  function decoratePanel(factory, recording, backendId) {
    const host = global.document.createElement('div');
    host.className = 'recordings2-recording-fallback-shell';

    const transportHost = global.document.createElement('div');
    transportHost.className = 'recordings2-recording-fallback-transport';
    host.appendChild(transportHost);

    const notice = global.document.createElement('p');
    notice.className = 'recordings2-playback-status';
    notice.setAttribute('role', 'status');
    notice.textContent = 'Kompatibilitätsmodus · Zeit-Sprung ist für diesen Wiedergabepfad nicht verfügbar.';
    host.appendChild(notice);

    const controls = global.document.createElement('div');
    controls.className = 'recordings2-playback-controls';
    controls.style.display = 'grid';
    controls.style.gap = '0.65rem';
    controls.style.marginTop = '0.75rem';

    const transportRow = global.document.createElement('div');
    transportRow.className = 'recordings2-playback-transport';
    transportRow.style.display = 'flex';
    transportRow.style.flexWrap = 'wrap';
    transportRow.style.gap = '0.5rem';

    const back60Button = createButton('−60', '60 Sekunden zurück');
    const back10Button = createButton('−10', '10 Sekunden zurück');
    const playPauseButton = createButton('Pause', 'Wiedergabe pausieren oder fortsetzen');
    const forward10Button = createButton('+10', '10 Sekunden vor');
    const forward60Button = createButton('+60', '60 Sekunden vor');
    const stopButton = createButton('Stop', 'Wiedergabe stoppen');
    [back60Button, back10Button, playPauseButton, forward10Button, forward60Button, stopButton]
      .forEach(function (button) { transportRow.appendChild(button); });
    controls.appendChild(transportRow);

    const positionLabel = global.document.createElement('div');
    positionLabel.className = 'recordings2-playback-position';
    positionLabel.setAttribute('aria-live', 'polite');
    positionLabel.textContent = '00:00:00 / --:--:--';
    controls.appendChild(positionLabel);

    const timeline = global.document.createElement('input');
    timeline.type = 'range';
    timeline.min = '0';
    timeline.max = '0';
    timeline.step = '1';
    timeline.value = '0';
    timeline.disabled = true;
    timeline.setAttribute('aria-label', 'Wiedergabeposition');
    timeline.style.width = '100%';
    controls.appendChild(timeline);

    const directRow = global.document.createElement('div');
    directRow.className = 'recordings2-playback-direct-seek';
    directRow.style.display = 'flex';
    directRow.style.flexWrap = 'wrap';
    directRow.style.gap = '0.5rem';
    const directTime = global.document.createElement('input');
    directTime.type = 'text';
    directTime.inputMode = 'numeric';
    directTime.placeholder = 'HH:MM:SS';
    directTime.disabled = true;
    directTime.setAttribute('aria-label', 'Direkte Wiedergabezeit');
    const directButton = createButton('Springen', 'Zur eingegebenen Wiedergabezeit springen');
    directButton.disabled = true;
    directRow.appendChild(directTime);
    directRow.appendChild(directButton);
    controls.appendChild(directRow);

    const restartButton = createButton('↺ Wiedergabe von vorn', 'Wiedergabe von vorn starten');
    restartButton.hidden = true;
    controls.appendChild(restartButton);
    host.appendChild(controls);

    let inner = null;
    let destroyed = false;
    let started = false;
    let stopped = false;
    let stoppedPosition = 0;

    function currentVideo() {
      return inner && inner.element ? find(inner.element, 'video') : null;
    }

    function nativeDuration() {
      const video = currentVideo();
      const value = Number(video && video.duration);
      return Number.isFinite(value) && value > 0 ? Math.floor(value) : 0;
    }

    function position() {
      if (stopped) return stoppedPosition;
      const video = currentVideo();
      const value = Number(video && video.currentTime);
      return Number.isFinite(value) && value > 0 ? Math.floor(value) : 0;
    }

    function updatePosition() {
      const duration = nativeDuration();
      positionLabel.textContent = formatTime(position()) + ' / ' +
        (duration > 0 ? formatTime(duration) : '--:--:--');
    }

    function updateControls() {
      const video = currentVideo();
      const active = started && !stopped && !destroyed && Boolean(video);
      back60Button.disabled = true;
      back10Button.disabled = true;
      forward10Button.disabled = true;
      forward60Button.disabled = true;
      timeline.disabled = true;
      directTime.disabled = true;
      directButton.disabled = true;
      playPauseButton.disabled = !active;
      stopButton.disabled = !active;
      playPauseButton.textContent = video && video.paused ? 'Play' : 'Pause';
      restartButton.hidden = !stopped;
      restartButton.disabled = !stopped || destroyed;
      updatePosition();
    }

    function bindVideo() {
      const video = currentVideo();
      if (!video || video.__vdrSuiteFallbackControlsBound === true) return;
      video.__vdrSuiteFallbackControlsBound = true;
      video.controls = false;
      ['play', 'pause', 'playing', 'timeupdate', 'durationchange', 'loadedmetadata']
        .forEach(function (name) {
          video.addEventListener(name, updateControls);
        });
      video.addEventListener('ended', function () {
        if (destroyed) return;
        stoppedPosition = position();
        started = false;
        stopped = true;
        notice.textContent = 'Wiedergabe beendet · Wiedergabe von vorn möglich.';
        updateControls();
      });
    }

    function mountInner() {
      const next = factory(recording, backendId);
      if (!next || !next.element || typeof next.start !== 'function') return null;
      inner = next;
      if (typeof transportHost.replaceChildren === 'function') {
        transportHost.replaceChildren(inner.element);
      }
      else {
        while (transportHost.firstChild && typeof transportHost.removeChild === 'function') {
          transportHost.removeChild(transportHost.firstChild);
        }
        transportHost.appendChild(inner.element);
      }
      const innerStart = find(inner.element, 'button.recordings2-primary');
      if (innerStart) innerStart.hidden = true;
      bindVideo();
      return inner;
    }

    function start() {
      if (destroyed) return Promise.resolve('');
      if (!inner || stopped) {
        if (!mountInner()) return Promise.resolve('');
      }
      started = true;
      stopped = false;
      stoppedPosition = 0;
      restartButton.hidden = true;
      notice.textContent = 'Kompatibilitätsmodus · Zeit-Sprung ist für diesen Wiedergabepfad nicht verfügbar.';
      bindVideo();
      updateControls();
      return Promise.resolve(inner.start()).then(function (sessionId) {
        bindVideo();
        updateControls();
        return sessionId;
      });
    }

    function play() {
      const video = currentVideo();
      if (!video || destroyed || stopped) return Promise.resolve(false);
      const request = video.play();
      updateControls();
      return request && typeof request.then === 'function'
        ? request.then(function () { return true; })
        : Promise.resolve(true);
    }

    function pause() {
      const video = currentVideo();
      if (!video || destroyed || stopped) return false;
      try { video.pause(); } catch (error) { return false; }
      updateControls();
      return true;
    }

    function stop() {
      if (destroyed || stopped || !inner) return Promise.resolve(false);
      stoppedPosition = position();
      if (typeof inner.destroy === 'function') inner.destroy();
      started = false;
      stopped = true;
      notice.textContent = 'Wiedergabe gestoppt · Wiedergabe von vorn möglich.';
      updateControls();
      return Promise.resolve(true);
    }

    function unsupportedSeek() {
      return Promise.reject(new Error('Zeit-Sprung ist im Recording-Kompatibilitätspfad nicht verfügbar.'));
    }

    function destroy() {
      if (destroyed) return;
      destroyed = true;
      if (inner && typeof inner.destroy === 'function') inner.destroy();
      started = false;
      updateControls();
    }

    function state() {
      if (destroyed) return 'destroyed';
      if (stopped) return 'stopped';
      if (!started) return 'idle';
      const video = currentVideo();
      return video && video.paused ? 'paused' : 'playing';
    }

    playPauseButton.addEventListener('click', function () {
      const video = currentVideo();
      if (!video || stopped) return;
      if (video.paused) play().catch(function () {});
      else pause();
    });
    stopButton.addEventListener('click', function () { stop().catch(function () {}); });
    restartButton.addEventListener('click', function () { start().catch(function () {}); });

    if (!mountInner()) return null;
    updateControls();

    return Object.freeze({
      element: host,
      start: start,
      play: play,
      pause: pause,
      stop: stop,
      destroy: destroy,
      position: position,
      duration: function () {
        const duration = nativeDuration();
        return duration > 0 ? duration : null;
      },
      state: state,
      seekAbsolute: unsupportedSeek,
      seekRelative: unsupportedSeek,
      sessionId: function () {
        return inner && typeof inner.sessionId === 'function' ? inner.sessionId() : '';
      },
      relinquishForReplacement: function () {
        if (inner && typeof inner.relinquishForReplacement === 'function') {
          return inner.relinquishForReplacement();
        }
        destroy();
        return Promise.resolve('');
      }
    });
  }

  function decoratePlayback(value) {
    const source = value && typeof value === 'object' ? value : {};
    if (typeof source.createPanel !== 'function' || source.__vdrSuiteFallbackControlsDecorated === true) {
      return source;
    }
    const decorated = {};
    Object.keys(source).forEach(function (key) { decorated[key] = source[key]; });
    const factory = source.createPanel;
    decorated.createPanel = function (recording, backendId) {
      return decoratePanel(factory, recording, backendId);
    };
    decorated.__vdrSuiteFallbackControlsDecorated = true;
    return Object.freeze(decorated);
  }

  Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
    configurable: descriptor.configurable !== false,
    enumerable: descriptor.enumerable !== false,
    get: function () { return descriptor.get.call(global); },
    set: function (value) { descriptor.set.call(global, decoratePlayback(value)); }
  });

  global[marker] = true;
  global.VdrSuiteRecordingFallbackControls = Object.freeze({
    __test: Object.freeze({formatTime: formatTime})
  });
}(window));
