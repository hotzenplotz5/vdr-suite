// Preserve Recording playback controls when the fast continuous-fMP4 path
// falls back to the legacy HLS/MSE transport. The Recording index remains
// source-owned, so this shell also consumes the server playback/resume contract
// without pretending that the rolling HLS transport supports arbitrary seek.
(function (global) {
  'use strict';

  const marker = '__vdrSuiteRecordingFallbackControlsBound';
  const INDEX_POLL_MS = 750;
  const INDEX_STATUS_MAX_FAILURES = 5;
  if (!global || !global.document || global[marker] === true) return;

  const descriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
  if (!descriptor || typeof descriptor.get !== 'function' || typeof descriptor.set !== 'function') return;

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function recordingId(recording) {
    if (!recording || typeof recording !== 'object') return '';
    return text(recording.recordingId || recording.id).trim();
  }

  function safeSessionId(value) {
    const id = text(value).trim();
    return id && id.length <= 128 && /^[A-Za-z0-9._:-]+$/.test(id) ? id : '';
  }

  function safeAudioTrackId(value) {
    const id = text(value).trim();
    return /^audio-[1-9][0-9]{0,9}$/.test(id) && id.length <= 16 ? id : '';
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

  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function capabilities() {
    return {
      protocols: ['hls'],
      containers: ['fmp4'],
      videoCodecs: ['h264'],
      audioCodecs: ['aac'],
      supportsByteRanges: false,
      maxVideoWidth: 1920,
      maxVideoHeight: 1080,
      maxAudioChannels: 2
    };
  }

  function createSession(backendId, recording, startPositionSeconds, audioTrackId) {
    const api = global.VdrSuiteClientApi;
    const id = recordingId(recording);
    const start = Math.max(0, Math.floor(Number(startPositionSeconds) || 0));
    const selectedAudioTrackId = safeAudioTrackId(audioTrackId);
    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('Client API für Aufnahme-Wiedergabe ist nicht verfügbar.'));
    }
    if (!id) return Promise.reject(new Error('Die Aufnahme besitzt keine öffentliche Recording-ID.'));
    if (audioTrackId && !selectedAudioTrackId) {
      return Promise.reject(new Error('Ungültige normalisierte Tonspur-ID.'));
    }

    const body = {
      backendId: text(backendId || 'default'),
      recordingId: id,
      capabilities: capabilities()
    };
    if (start > 0) body.startPositionSeconds = start;
    if (selectedAudioTrackId) body.audioTrackId = selectedAudioTrackId;
    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify(body),
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function playbackStatus(backendId, sessionId) {
    const api = global.VdrSuiteClientApi;
    const id = safeSessionId(sessionId);
    if (!api || typeof api.requestJson !== 'function' || !id) {
      return Promise.reject(new Error('Recording-Indexstatus ist nicht verfügbar.'));
    }
    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify({
        operation: 'playback-status',
        backendId: text(backendId || 'default'),
        sessionId: id
      }),
      cache: 'no-store',
      credentials: 'same-origin'
    });
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

  function bindRestartChoice(playback) {
    if (!playback || !playback.element) return;

    function installReadyHelper() {
      const helper = global.VdrSuiteRecordingPlaybackRestartChoice;
      if (!helper || typeof helper.install !== 'function') return false;
      helper.install(playback);
      return true;
    }

    if (installReadyHelper()) return;
    if (typeof global.loadVdrSuiteDeferredRuntime !== 'function') return;

    Promise.resolve(global.loadVdrSuiteDeferredRuntime(
      'vdr-suite-recording-playback-restart-choice-runtime',
      '/frontend/recording-playback-restart-choice.js',
      function () {
        return Boolean(
          global.VdrSuiteRecordingPlaybackRestartChoice &&
          typeof global.VdrSuiteRecordingPlaybackRestartChoice.install === 'function'
        );
      }
    )).then(function () {
      installReadyHelper();
    }).catch(function (error) {
      if (global.console && typeof global.console.error === 'function') {
        global.console.error('VDR-Suite Recording fallback restart choice binding failed', error);
      }
    });
  }

  function decoratePanel(factory, recording, backendId) {
    const host = global.document.createElement('div');
    host.className = 'recordings2-recording-fallback-shell';

    // A direct primary button gives the shared Stop/restart-choice owner a
    // stable anchor even though the real HLS start button lives inside the
    // replaceable legacy transport panel.
    const restartAnchor = createButton('▶ Aufnahme abspielen', 'Aufnahme abspielen');
    restartAnchor.className = 'recordings2-primary';
    restartAnchor.hidden = true;
    host.appendChild(restartAnchor);

    const transportHost = global.document.createElement('div');
    transportHost.className = 'recordings2-recording-fallback-transport';
    host.appendChild(transportHost);

    const notice = global.document.createElement('p');
    notice.className = 'recordings2-playback-status recordings2-fallback-contract-status';
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
    host.appendChild(controls);

    let inner = null;
    let destroyed = false;
    let started = false;
    let stopped = false;
    let activeSessionId = '';
    let playbackBaseSeconds = 0;
    let durationSeconds = 0;
    let resumeSupported = false;
    let resumePreparing = false;
    let stoppedPosition = 0;
    let stoppedDuration = 0;
    let stoppedResumeSupported = false;
    let indexTimer = null;
    let indexFailures = 0;
    let preferredAudioTrackId = '';
    let audioSelectionInFlight = false;

    function currentVideo() {
      return inner && inner.element ? find(inner.element, 'video') : null;
    }

    function position() {
      if (stopped) return stoppedPosition;
      const video = currentVideo();
      const local = Number(video && video.currentTime);
      return playbackBaseSeconds + (Number.isFinite(local) && local > 0 ? Math.floor(local) : 0);
    }

    function duration() {
      const value = stopped ? stoppedDuration : durationSeconds;
      return value > 0 ? value : null;
    }

    function updatePosition() {
      positionLabel.textContent = formatTime(position()) + ' / ' +
        (duration() ? formatTime(duration()) : '--:--:--');
    }

    function setContractNotice() {
      if (stopped) return;
      if (resumePreparing) {
        notice.textContent = 'Kompatibilitätsmodus · Aufnahmeindex wird erstellt …';
      }
      else if (resumeSupported) {
        notice.textContent = 'Kompatibilitätsmodus · Fortsetzen verfügbar · freier Zeit-Sprung bleibt deaktiviert.';
      }
      else {
        notice.textContent = 'Kompatibilitätsmodus · Zeit-Sprung ist für diesen Wiedergabepfad nicht verfügbar.';
      }
    }

    function updateControls() {
      const video = currentVideo();
      const active = started && !stopped && !destroyed && Boolean(video);
      if (host.__vdrSuiteFallbackRestartSeekControlsOwned !== true) {
        [back60Button, back10Button, forward10Button, forward60Button, timeline, directTime, directButton]
          .forEach(function (control) { control.disabled = true; });
      }
      playPauseButton.disabled = !active;
      stopButton.disabled = !active;
      playPauseButton.textContent = video && video.paused ? 'Play' : 'Pause';
      updatePosition();
    }

    function clearIndexPoll() {
      if (indexTimer !== null && typeof global.clearTimeout === 'function') {
        global.clearTimeout(indexTimer);
      }
      indexTimer = null;
    }

    function scheduleIndexPoll() {
      if (!resumePreparing || stopped || destroyed || !activeSessionId ||
          indexTimer !== null || typeof global.setTimeout !== 'function') return;
      indexTimer = global.setTimeout(function () {
        indexTimer = null;
        playbackStatus(backendId, activeSessionId).then(function (session) {
          if (stopped || destroyed) return;
          const mediaSession = session && session.mediaSession;
          if (safeSessionId(mediaSession && mediaSession.id) !== activeSessionId) {
            throw new Error('Recording-Indexstatus gehört zu einer anderen MediaSession.');
          }
          applyPlaybackContract(mediaSession, false);
          indexFailures = 0;
        }).catch(function () {
          indexFailures += 1;
          if (indexFailures >= INDEX_STATUS_MAX_FAILURES) {
            resumePreparing = false;
            setContractNotice();
            updateControls();
            return;
          }
          scheduleIndexPoll();
        });
      }, INDEX_POLL_MS);
    }

    function applyPlaybackContract(mediaSession, updateBase) {
      const id = safeSessionId(mediaSession && mediaSession.id);
      if (id) activeSessionId = id;
      const playback = mediaSession && mediaSession.playback;
      const serverDuration = Number(playback && playback.durationSeconds);
      durationSeconds = Number.isFinite(serverDuration) && serverDuration > 0
        ? Math.floor(serverDuration)
        : 0;
      const resume = playback && playback.resume;
      resumeSupported = Boolean(resume && resume.supported === true && durationSeconds > 0);
      resumePreparing = Boolean(resume && resume.preparing === true && !resumeSupported);
      if (updateBase) {
        const serverPosition = Number(playback && playback.positionSeconds);
        playbackBaseSeconds = Number.isFinite(serverPosition) && serverPosition >= 0
          ? Math.floor(serverPosition)
          : 0;
      }
      setContractNotice();
      updateControls();
      if (resumePreparing) scheduleIndexPoll();
      else clearIndexPoll();
    }

    function bindVideo() {
      const video = currentVideo();
      if (!video || video.__vdrSuiteFallbackControlsBound === true) return;
      video.__vdrSuiteFallbackControlsBound = true;
      video.controls = false;
      ['play', 'pause', 'playing', 'timeupdate', 'loadedmetadata']
        .forEach(function (name) { video.addEventListener(name, updateControls); });
      video.addEventListener('ended', function () {
        if (destroyed) return;
        stoppedPosition = position();
        stoppedDuration = durationSeconds;
        stoppedResumeSupported = false;
        started = false;
        stopped = true;
        clearIndexPoll();
        notice.textContent = 'Wiedergabe beendet · Wiedergabe von vorn möglich.';
        updateControls();
      });
    }

    function mountInner(startPositionSeconds) {
      const target = Math.max(0, Math.floor(Number(startPositionSeconds) || 0));
      const next = factory(recording, backendId, {
        createSession: function () {
          return createSession(
            backendId,
            recording,
            target,
            preferredAudioTrackId
          ).then(function (session) {
            applyPlaybackContract(session && session.mediaSession, true);
            return session;
          });
        }
      });
      if (!next || !next.element || typeof next.start !== 'function') return null;
      inner = next;
      if (typeof transportHost.replaceChildren === 'function') transportHost.replaceChildren(inner.element);
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

    function startAt(startPositionSeconds) {
      if (destroyed) return Promise.resolve('');
      const target = Math.max(0, Math.floor(Number(startPositionSeconds) || 0));
      if (!inner || stopped) {
        if (!mountInner(target)) return Promise.resolve('');
      }
      started = true;
      stopped = false;
      activeSessionId = '';
      playbackBaseSeconds = target;
      durationSeconds = stoppedDuration || durationSeconds;
      resumeSupported = false;
      resumePreparing = false;
      indexFailures = 0;
      stoppedPosition = 0;
      stoppedDuration = 0;
      stoppedResumeSupported = false;
      setContractNotice();
      bindVideo();
      updateControls();
      return Promise.resolve(inner.start()).then(function (sessionId) {
        bindVideo();
        updateControls();
        return sessionId;
      }).catch(function (error) {
        started = false;
        stopped = true;
        stoppedPosition = target;
        stoppedDuration = durationSeconds;
        notice.textContent = error && error.message
          ? 'Fortsetzen fehlgeschlagen: ' + error.message
          : 'Fortsetzen fehlgeschlagen.';
        updateControls();
        throw error;
      });
    }

    function start() {
      return startAt(0);
    }

    function resume(positionSeconds) {
      const target = Math.max(0, Math.floor(Number(positionSeconds) || 0));
      if (!stopped || !stoppedResumeSupported || target <= 0) {
        return Promise.reject(new Error('Fortsetzen ist im Recording-Kompatibilitätspfad nicht verfügbar.'));
      }
      return startAt(target);
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
      stoppedDuration = durationSeconds;
      stoppedResumeSupported = Boolean(resumeSupported && stoppedPosition > 0);
      clearIndexPoll();
      if (typeof inner.destroy === 'function') inner.destroy();
      started = false;
      stopped = true;
      activeSessionId = '';
      notice.textContent = stoppedResumeSupported
        ? 'Wiedergabe gestoppt · ab ' + formatTime(stoppedPosition) + ' fortsetzen oder von vorn.'
        : 'Wiedergabe gestoppt · Wiedergabe von vorn möglich.';
      updateControls();
      return Promise.resolve(true);
    }

    function selectAudioTrack(audioTrackId) {
      const targetTrackId = safeAudioTrackId(audioTrackId);
      if (!targetTrackId) {
        return Promise.reject(new Error('Ungültige normalisierte Tonspur-ID.'));
      }
      if (destroyed || stopped || !started || audioSelectionInFlight || !resumeSupported) {
        return Promise.reject(new Error('Tonspurwechsel ist für diesen HLS-Wiedergabestand nicht verfügbar.'));
      }
      if (targetTrackId === preferredAudioTrackId) {
        return Promise.resolve(activeSessionId);
      }

      const currentState = state();
      const targetPosition = position();
      const wasPaused = currentState === 'paused';
      if ((currentState !== 'playing' && currentState !== 'paused') || targetPosition < 0) {
        return Promise.reject(new Error('Tonspurwechsel benötigt aktive Recording-Wiedergabe.'));
      }

      const previousTrackId = preferredAudioTrackId;
      audioSelectionInFlight = true;
      return Promise.resolve(stop()).then(function (stoppedCleanly) {
        if (stoppedCleanly === false || state() !== 'stopped') {
          throw new Error('Der bisherige HLS-Stream konnte nicht sauber gestoppt werden.');
        }
        preferredAudioTrackId = targetTrackId;
        if (targetPosition > 0) return resume(targetPosition);
        return startAt(0);
      }).then(function (sessionId) {
        if (!safeSessionId(sessionId)) {
          throw new Error('Die HLS-Replacement-MediaSession wurde nicht gestartet.');
        }
        if (wasPaused && !pause()) {
          throw new Error('Pause-Zustand konnte nach dem Tonspurwechsel nicht erhalten werden.');
        }
        audioSelectionInFlight = false;
        return sessionId;
      }).catch(function (error) {
        preferredAudioTrackId = previousTrackId;
        audioSelectionInFlight = false;
        throw error;
      });
    }

    function unsupportedSeek() {
      return Promise.reject(new Error('Zeit-Sprung ist im Recording-Kompatibilitätspfad nicht verfügbar.'));
    }

    function destroy() {
      if (destroyed) return;
      destroyed = true;
      clearIndexPoll();
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

    if (!mountInner(0)) return null;
    updateControls();

    const playback = Object.freeze({
      element: host,
      start: start,
      resume: resume,
      canResume: function () { return stopped ? stoppedResumeSupported : resumeSupported; },
      play: play,
      pause: pause,
      stop: stop,
      destroy: destroy,
      position: position,
      duration: duration,
      state: state,
      selectAudioTrack: selectAudioTrack,
      canSelectAudioTrack: function () {
        return !destroyed && !stopped && started && !audioSelectionInFlight && resumeSupported;
      },
      seekAbsolute: unsupportedSeek,
      seekRelative: unsupportedSeek,
      sessionId: function () {
        return inner && typeof inner.sessionId === 'function' ? inner.sessionId() : activeSessionId;
      },
      relinquishForReplacement: function () {
        clearIndexPoll();
        if (inner && typeof inner.relinquishForReplacement === 'function') {
          return inner.relinquishForReplacement();
        }
        destroy();
        return Promise.resolve('');
      }
    });
    host.__vdrSuiteRecordingFallbackOwner = playback;
    return playback;
  }

  function decoratePlayback(value) {
    const source = value && typeof value === 'object' ? value : {};
    if (typeof source.createPanel !== 'function' || source.__vdrSuiteFallbackControlsDecorated === true) return source;
    const decorated = {};
    Object.keys(source).forEach(function (key) { decorated[key] = source[key]; });
    const factory = source.createPanel;
    decorated.createPanel = function (recording, backendId) {
      const playback = decoratePanel(factory, recording, backendId);
      bindRestartChoice(playback);
      return playback;
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
    __test: Object.freeze({formatTime: formatTime, safeAudioTrackId: safeAudioTrackId})
  });
}(window));