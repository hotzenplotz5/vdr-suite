// Phase 65.D Recording audio-track controls.
//
// This adapter deliberately wraps the existing persistent Recording playback
// owner instead of creating another player architecture. Progressive-fMP4 keeps
// the same MediaSession and uses the established D.2 seek/reconnect operation.
// HLS uses the established D.2 stop/resume replacement owner at the same
// absolute Recording position. Track IDs remain normalized Suite session IDs.
(function (global) {
  'use strict';

  const marker = '__vdrSuiteRecordingTrackControlsBound';
  const TRACK_STATUS_POLL_MS = 750;
  const TRACK_STATUS_MAX_FAILURES = 5;
  if (!global || !global.document || global[marker] === true) return;

  const descriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
  if (!descriptor || typeof descriptor.get !== 'function' || typeof descriptor.set !== 'function') return;

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function safeSessionId(value) {
    const id = text(value).trim();
    return id && id.length <= 128 && /^[A-Za-z0-9._:-]+$/.test(id) ? id : '';
  }

  function safeAudioTrackId(value) {
    const id = text(value).trim();
    return /^audio-[1-9][0-9]{0,9}$/.test(id) && id.length <= 16 ? id : '';
  }

  function recordingId(recording) {
    if (!recording || typeof recording !== 'object') return '';
    return text(recording.recordingId || recording.id).trim();
  }

  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function recordingCapabilities() {
    return {
      protocols: ['progressive'],
      containers: ['fmp4'],
      videoCodecs: ['h264'],
      audioCodecs: ['aac'],
      supportsByteRanges: false,
      maxVideoWidth: 1920,
      maxVideoHeight: 1080,
      maxAudioChannels: 2
    };
  }

  function request(body) {
    const api = global.VdrSuiteClientApi;
    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('Client API für Aufnahme-Wiedergabe ist nicht verfügbar.'));
    }
    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify(body),
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function trackStatus(backendId, sessionId) {
    const id = safeSessionId(sessionId);
    if (!id) return Promise.reject(new Error('Ungültige Recording-MediaSession-ID.'));
    return request({
      operation: 'track-status',
      backendId: text(backendId || 'default'),
      sessionId: id
    });
  }

  function selectAudioTrack(backendId, recording, sessionId, trackId, positionSeconds) {
    const id = safeSessionId(sessionId);
    const audioTrackId = safeAudioTrackId(trackId);
    const sourceRecordingId = recordingId(recording);
    const position = Math.max(0, Math.floor(Number(positionSeconds) || 0));
    if (!id || !audioTrackId || !sourceRecordingId) {
      return Promise.reject(new Error('Ungültige Recording-Tonspurauswahl.'));
    }
    return request({
      operation: 'select-audio-track',
      backendId: text(backendId || 'default'),
      sessionId: id,
      recordingId: sourceRecordingId,
      audioTrackId: audioTrackId,
      positionSeconds: position,
      capabilities: recordingCapabilities()
    });
  }

  function roleLabel(role) {
    switch (text(role)) {
    case 'original': return 'Original';
    case 'commentary': return 'Kommentar';
    case 'descriptive': return 'Audiodeskription';
    case 'hearing-impaired': return 'Hörgeschädigt';
    default: return '';
    }
  }

  function languageLabel(language) {
    const value = text(language).trim();
    const normalized = value.toLowerCase();
    if (normalized === 'deu' || normalized === 'ger' || normalized === 'de') return 'Deutsch';
    if (normalized === 'eng' || normalized === 'en') return 'Englisch';
    if (normalized === 'und' || normalized === 'unknown') return '';
    return value ? value.toUpperCase() : '';
  }

  function audioTrackLabel(track, index) {
    const source = track && typeof track === 'object' ? track : {};
    const parts = [];
    const language = text(source.language).trim();
    const displayLanguage = languageLabel(language);
    const label = text(source.label).trim();
    const codec = text(source.codec).trim();
    const layout = text(source.layout).trim();
    const channels = Number(source.channels);
    const roles = Array.isArray(source.roles)
      ? source.roles.map(roleLabel).filter(Boolean)
      : [];

    if (displayLanguage) parts.push(displayLanguage);
    if (label && label.toLowerCase() !== language.toLowerCase() &&
        label.toLowerCase() !== displayLanguage.toLowerCase()) parts.push(label);
    if (codec && codec !== 'unknown' && codec !== 'none') parts.push(codec.toUpperCase());
    if (layout) parts.push(layout);
    else if (Number.isFinite(channels) && channels > 0) parts.push(String(channels) + ' Kanäle');
    roles.forEach(function (role) { parts.push(role); });
    if (source.default === true) parts.push('Standard');
    return parts.length ? parts.join(' · ') : 'Tonspur ' + String(index + 1);
  }

  function clearChildren(node) {
    if (!node) return;
    if (typeof node.replaceChildren === 'function') {
      node.replaceChildren();
      return;
    }
    while (node.firstChild && typeof node.removeChild === 'function') {
      node.removeChild(node.firstChild);
    }
    if (node.options && typeof node.options.length === 'number') {
      node.options.length = 0;
    }
  }

  function hlsProfile(profileId) {
    return profileId === 'hls-fmp4' || profileId === 'hls-ts';
  }

  function fallbackOwner(root) {
    if (!root) return null;
    if (root.__vdrSuiteRecordingFallbackOwner) {
      return root.__vdrSuiteRecordingFallbackOwner;
    }
    if (typeof root.querySelector !== 'function') return null;
    const fallback = root.querySelector('.recordings2-recording-fallback-shell');
    return fallback && fallback.__vdrSuiteRecordingFallbackOwner
      ? fallback.__vdrSuiteRecordingFallbackOwner
      : null;
  }

  function decoratePanel(panel, recording, backendId) {
    if (!panel || !panel.element || typeof panel.start !== 'function' ||
        typeof panel.sessionId !== 'function' || typeof panel.position !== 'function' ||
        typeof panel.state !== 'function' || typeof panel.seekAbsolute !== 'function') {
      return panel;
    }

    // The fast progressive panel replaces its own DOM node when it falls back
    // to HLS. Keep track controls in a stable owner shell around that replaceable
    // transport so they survive the transition instead of disappearing with it.
    const shell = global.document.createElement('div');
    shell.className = 'recordings2-track-owner-shell';
    shell.appendChild(panel.element);

    const host = global.document.createElement('div');
    host.className = 'recordings2-track-controls';
    host.hidden = true;
    host.style.display = 'grid';
    host.style.gap = '0.45rem';
    host.style.marginTop = '0.75rem';

    const audioRow = global.document.createElement('label');
    audioRow.className = 'recordings2-audio-track-control';
    audioRow.hidden = true;
    audioRow.textContent = 'Tonspur ';
    const audioSelect = global.document.createElement('select');
    audioSelect.setAttribute('aria-label', 'Tonspur auswählen');
    audioSelect.disabled = true;
    audioRow.appendChild(audioSelect);
    host.appendChild(audioRow);

    const note = global.document.createElement('p');
    note.className = 'recordings2-track-status';
    note.setAttribute('role', 'status');
    note.hidden = true;
    host.appendChild(note);

    const subtitleInfo = global.document.createElement('p');
    subtitleInfo.className = 'recordings2-subtitle-track-status';
    subtitleInfo.hidden = true;
    host.appendChild(subtitleInfo);

    shell.appendChild(host);

    let disposed = false;
    let selectionInFlight = false;
    let selectedTrackId = '';
    let pollTimer = null;
    let pollFailures = 0;
    let activeSessionId = '';
    let activeProfileId = '';
    let sessionWatchTimer = null;

    function activeFallbackOwner() {
      return fallbackOwner(shell);
    }

    function clearPoll() {
      if (pollTimer !== null && typeof global.clearTimeout === 'function') {
        try { global.clearTimeout(pollTimer); } catch (error) {}
      }
      pollTimer = null;
    }

    function clearSessionWatch() {
      if (sessionWatchTimer !== null && typeof global.clearTimeout === 'function') {
        try { global.clearTimeout(sessionWatchTimer); } catch (error) {}
      }
      sessionWatchTimer = null;
    }

    function scheduleSessionWatch() {
      if (disposed || sessionWatchTimer !== null || typeof global.setTimeout !== 'function') return;
      sessionWatchTimer = global.setTimeout(function () {
        sessionWatchTimer = null;
        if (disposed) return;
        const currentId = safeSessionId(panel.sessionId());
        if (currentId && activeSessionId && currentId !== activeSessionId) {
          activeSessionId = currentId;
          pollFailures = 0;
          refreshTracks();
        }
        scheduleSessionWatch();
      }, TRACK_STATUS_POLL_MS);
    }

    function setNote(message, error) {
      note.textContent = text(message);
      note.hidden = !note.textContent;
      if (note.classList && typeof note.classList.toggle === 'function') {
        note.classList.toggle('error', Boolean(error));
      }
    }

    function scheduleRefresh() {
      if (disposed || pollTimer !== null || typeof global.setTimeout !== 'function') return;
      pollTimer = global.setTimeout(function () {
        pollTimer = null;
        refreshTracks();
      }, TRACK_STATUS_POLL_MS);
    }

    function renderTracks(mediaSession) {
      activeProfileId = text(mediaSession && mediaSession.presentationProfileId).trim();
      const tracks = mediaSession && mediaSession.tracks;
      const audio = tracks && tracks.audio;
      const available = audio && Array.isArray(audio.availableTracks)
        ? audio.availableTracks
        : [];
      const nextSelected = safeAudioTrackId(audio && audio.selectedTrackId);

      clearChildren(audioSelect);
      available.forEach(function (track, index) {
        const trackId = safeAudioTrackId(track && track.id);
        if (!trackId) return;
        const option = global.document.createElement('option');
        option.value = trackId;
        option.textContent = audioTrackLabel(track, index);
        option.selected = trackId === nextSelected;
        audioSelect.appendChild(option);
      });

      selectedTrackId = nextSelected;
      if (selectedTrackId) audioSelect.value = selectedTrackId;
      const hlsOwner = activeFallbackOwner();
      const transportCanSelect = activeProfileId === 'progressive-fmp4' ||
        (hlsProfile(activeProfileId) && hlsOwner && typeof hlsOwner.selectAudioTrack === 'function');
      const audioSelectable = Boolean(
        transportCanSelect && audio && audio.selectionSupported === true &&
        available.filter(function (track) { return Boolean(safeAudioTrackId(track && track.id)); }).length > 1
      );
      audioRow.hidden = !audioSelectable;
      audioSelect.disabled = !audioSelectable || selectionInFlight;

      const subtitle = tracks && tracks.subtitles;
      const subtitleTracks = subtitle && Array.isArray(subtitle.availableTracks)
        ? subtitle.availableTracks
        : [];
      const truthfulOff = Boolean(subtitle && subtitle.offSelected === true);
      subtitleInfo.hidden = !(subtitleTracks.length > 0 && truthfulOff);
      subtitleInfo.textContent = subtitleInfo.hidden
        ? ''
        : 'Untertitel: Aus · Auswahl ist in diesem Wiedergabepfad nicht verfügbar.';

      host.hidden = audioRow.hidden && subtitleInfo.hidden && note.hidden;
      const reason = text(audio && audio.selectionReason);
      if (!audioSelectable && reason === 'recording_audio_track_selection_preparing') {
        scheduleRefresh();
      }
      else {
        clearPoll();
      }
    }

    function refreshTracks() {
      if (disposed) return Promise.resolve(false);
      const id = safeSessionId(panel.sessionId()) || activeSessionId;
      if (!id) return Promise.resolve(false);
      activeSessionId = id;
      return trackStatus(backendId, id).then(function (session) {
        if (disposed) return false;
        const mediaSession = session && session.mediaSession;
        if (safeSessionId(mediaSession && mediaSession.id) !== id ||
            !mediaSession || mediaSession.state !== 'ready') {
          throw new Error('Track-Status gehört nicht zur aktiven Recording-MediaSession.');
        }
        const currentId = safeSessionId(panel.sessionId());
        if (currentId && currentId !== id) {
          activeSessionId = currentId;
          scheduleRefresh();
          return false;
        }
        pollFailures = 0;
        renderTracks(mediaSession);
        return mediaSession;
      }).catch(function (error) {
        if (disposed) return false;
        pollFailures += 1;
        if (pollFailures < TRACK_STATUS_MAX_FAILURES) {
          scheduleRefresh();
        }
        else {
          clearPoll();
          setNote('Tonspurinformationen konnten nicht geladen werden.', true);
          host.hidden = false;
        }
        return false;
      });
    }

    function performHlsSelection(targetTrackId, previousTrackId, id, playbackState) {
      const hlsOwner = activeFallbackOwner();
      if (!hlsOwner || typeof hlsOwner.selectAudioTrack !== 'function') {
        return Promise.reject(new Error('Aktiver HLS-Playback-Owner ist für Tonspurwechsel nicht verfügbar.'));
      }
      let replacementStarted = false;
      return Promise.resolve(hlsOwner.selectAudioTrack(targetTrackId)).then(function (newSessionId) {
        const replacementId = safeSessionId(newSessionId) || safeSessionId(panel.sessionId());
        if (!replacementId || replacementId === id) {
          throw new Error('HLS-Tonspurwechsel hat keine neue MediaSession erzeugt.');
        }
        replacementStarted = true;
        activeSessionId = replacementId;
        return trackStatus(backendId, replacementId);
      }).then(function (session) {
        if (disposed) return false;
        const mediaSession = session && session.mediaSession;
        if (!mediaSession || mediaSession.state !== 'ready' ||
            safeSessionId(mediaSession.id) !== activeSessionId ||
            !hlsProfile(text(mediaSession.presentationProfileId))) {
          throw new Error('HLS-Replacement-Trackstatus ist ungültig.');
        }
        renderTracks(mediaSession);
        if (selectedTrackId !== targetTrackId) {
          throw new Error('Server hat für den HLS-Replacement-Stream eine andere Tonspur bestätigt.');
        }
        const stateAfterReplacement = text(hlsOwner.state());
        if (stateAfterReplacement !== playbackState) {
          throw new Error('Play/Pause-Zustand wurde beim HLS-Tonspurwechsel nicht erhalten.');
        }
        return true;
      }).catch(function (error) {
        if (replacementStarted) {
          audioRow.hidden = true;
          audioSelect.disabled = true;
          if (typeof hlsOwner.stop === 'function') {
            try {
              const stopped = hlsOwner.stop();
              if (stopped && typeof stopped.catch === 'function') stopped.catch(function () {});
            } catch (stopError) {}
          }
          setNote(
            'Tonspur-Replacement konnte nicht verifiziert werden. Wiedergabe wurde gestoppt.',
            true
          );
        }
        else {
          selectedTrackId = previousTrackId;
          audioSelect.value = previousTrackId;
          setNote(
            error && error.message
              ? 'Tonspurwechsel fehlgeschlagen: ' + error.message
              : 'Tonspurwechsel fehlgeschlagen.',
            true
          );
        }
        host.hidden = false;
        throw error;
      });
    }

    function performProgressiveSelection(
      targetTrackId,
      previousTrackId,
      id,
      playbackState,
      position
    ) {
      let serverSelected = false;
      let pausedForSelection = false;

      if (playbackState === 'playing') {
        if (typeof panel.pause !== 'function' || panel.pause() !== true) {
          audioSelect.value = previousTrackId;
          return Promise.reject(new Error('Wiedergabe konnte für den Tonspurwechsel nicht stabil pausiert werden.'));
        }
        pausedForSelection = true;
      }

      return selectAudioTrack(
        backendId,
        recording,
        id,
        targetTrackId,
        position
      ).then(function (session) {
        if (disposed) return false;
        const mediaSession = session && session.mediaSession;
        const returnedId = safeSessionId(mediaSession && mediaSession.id);
        const playback = mediaSession && mediaSession.playback;
        const confirmedPosition = Number(playback && playback.positionSeconds);
        if (!mediaSession || mediaSession.state !== 'ready' || returnedId !== id ||
            mediaSession.presentationProfileId !== 'progressive-fmp4' ||
            !Number.isFinite(confirmedPosition) || Math.floor(confirmedPosition) !== position) {
          throw new Error('Tonspurwechsel hat die Recording-Playback-Identität nicht erhalten.');
        }

        serverSelected = true;
        renderTracks(mediaSession);
        if (selectedTrackId !== targetTrackId) {
          throw new Error('Server hat eine andere Tonspur bestätigt.');
        }

        return Promise.resolve(panel.seekAbsolute(position)).then(function () {
          if (disposed) return false;
          if (safeSessionId(panel.sessionId()) !== id) {
            throw new Error('MediaSession-ID hat sich beim Tonspur-Reconnect geändert.');
          }
          const stateAfterReconnect = text(panel.state());
          if (playbackState === 'paused' && stateAfterReconnect !== 'paused') {
            if (typeof panel.pause !== 'function' || panel.pause() !== true) {
              throw new Error('Pause-Zustand konnte nach dem Tonspurwechsel nicht erhalten werden.');
            }
          }
          else if (playbackState === 'playing' && stateAfterReconnect === 'paused' &&
                   typeof panel.play === 'function') {
            return Promise.resolve(panel.play()).then(function () { return true; });
          }
          return true;
        });
      }).catch(function (error) {
        if (serverSelected) {
          audioRow.hidden = true;
          audioSelect.disabled = true;
          setNote(
            'Tonspur wurde serverseitig gewechselt, aber der Player konnte nicht neu verbunden werden. Wiedergabe wurde gestoppt.',
            true
          );
          if (typeof panel.stop === 'function') {
            try {
              const stopped = panel.stop();
              if (stopped && typeof stopped.catch === 'function') stopped.catch(function () {});
            } catch (stopError) {}
          }
        }
        else {
          selectedTrackId = previousTrackId;
          audioSelect.value = previousTrackId;
          if (pausedForSelection && typeof panel.play === 'function') {
            try {
              const resumed = panel.play();
              if (resumed && typeof resumed.catch === 'function') resumed.catch(function () {});
            } catch (resumeError) {}
          }
          setNote(
            error && error.message
              ? 'Tonspurwechsel fehlgeschlagen: ' + error.message
              : 'Tonspurwechsel fehlgeschlagen.',
            true
          );
          host.hidden = false;
        }
        throw error;
      });
    }

    function performSelection() {
      if (disposed || selectionInFlight) return Promise.resolve(false);
      const targetTrackId = safeAudioTrackId(audioSelect.value);
      const previousTrackId = selectedTrackId;
      if (!targetTrackId || targetTrackId === previousTrackId) return Promise.resolve(false);

      const id = safeSessionId(panel.sessionId()) || activeSessionId;
      const playbackState = text(panel.state());
      const position = Math.max(0, Math.floor(Number(panel.position()) || 0));
      if (!id || (playbackState !== 'playing' && playbackState !== 'paused')) {
        audioSelect.value = previousTrackId;
        return Promise.reject(new Error('Tonspur kann nur während aktiver Recording-Wiedergabe gewechselt werden.'));
      }

      activeSessionId = id;
      selectionInFlight = true;
      audioSelect.disabled = true;
      setNote('Tonspur wird gewechselt …', false);
      host.hidden = false;

      const operation = hlsProfile(activeProfileId) && activeFallbackOwner()
        ? performHlsSelection(targetTrackId, previousTrackId, id, playbackState)
        : performProgressiveSelection(
            targetTrackId,
            previousTrackId,
            id,
            playbackState,
            position
          );

      return Promise.resolve(operation).then(function (result) {
        if (disposed || result === false) return result;
        selectionInFlight = false;
        audioSelect.disabled = audioRow.hidden;
        setNote('Tonspur gewechselt.', false);
        host.hidden = false;
        return true;
      }).catch(function (error) {
        selectionInFlight = false;
        audioSelect.disabled = audioRow.hidden;
        throw error;
      });
    }

    audioSelect.addEventListener('change', function () {
      performSelection().catch(function () {});
    });

    const wrapped = {};
    Object.keys(panel).forEach(function (key) { wrapped[key] = panel[key]; });
    wrapped.element = shell;

    wrapped.start = function () {
      const result = panel.start.apply(panel, arguments);
      return Promise.resolve(result).then(function (id) {
        activeSessionId = safeSessionId(id) || safeSessionId(panel.sessionId());
        pollFailures = 0;
        refreshTracks();
        scheduleSessionWatch();
        return id;
      });
    };

    if (typeof panel.stop === 'function') {
      wrapped.stop = function () {
        clearPoll();
        clearSessionWatch();
        audioRow.hidden = true;
        audioSelect.disabled = true;
        subtitleInfo.hidden = true;
        note.hidden = true;
        host.hidden = true;
        activeSessionId = '';
        activeProfileId = '';
        return panel.stop.apply(panel, arguments);
      };
    }

    if (typeof panel.relinquishForReplacement === 'function') {
      wrapped.relinquishForReplacement = function () {
        clearPoll();
        clearSessionWatch();
        activeSessionId = '';
        activeProfileId = '';
        return panel.relinquishForReplacement.apply(panel, arguments);
      };
    }

    if (typeof panel.destroy === 'function') {
      wrapped.destroy = function () {
        if (disposed) return;
        disposed = true;
        clearPoll();
        clearSessionWatch();
        panel.destroy.apply(panel, arguments);
      };
    }

    return Object.freeze(wrapped);
  }

  let cachedSource = null;
  let cachedDecorated = null;

  function decoratePlayback(source) {
    const value = source && typeof source === 'object' ? source : {};
    if (value === cachedSource && cachedDecorated) return cachedDecorated;
    if (typeof value.createPanel !== 'function') return value;

    const decorated = {};
    Object.keys(value).forEach(function (key) { decorated[key] = value[key]; });
    const factory = value.createPanel;
    decorated.createPanel = function (recording, backendId) {
      return decoratePanel(factory(recording, backendId), recording, backendId);
    };
    decorated.__vdrSuiteRecordingTrackControlsDecorated = true;
    cachedSource = value;
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
      descriptor.set.call(global, value);
    }
  });

  global[marker] = true;
  global.VdrSuiteRecordingTrackControls = Object.freeze({
    __test: Object.freeze({
      safeAudioTrackId: safeAudioTrackId,
      recordingCapabilities: recordingCapabilities,
      audioTrackLabel: audioTrackLabel,
      languageLabel: languageLabel,
      fallbackOwner: fallbackOwner,
      trackStatus: trackStatus,
      selectAudioTrack: selectAudioTrack
    })
  });
}(window));