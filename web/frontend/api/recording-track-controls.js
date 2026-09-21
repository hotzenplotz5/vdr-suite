// Phase 65.D Recording audio/subtitle track controls.
//
// This adapter decorates the existing persistent Recording playback owner. It
// never creates a second player or restart architecture. Audio selection reuses
// the accepted D.2 owner paths. Browser-text subtitles are delivered as
// session-bound WebVTT and mounted on the currently active video element.
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

  function safeSubtitleTrackId(value) {
    const id = text(value).trim();
    if (id === 'off') return 'off';
    return /^subtitle-[1-9][0-9]{0,9}$/.test(id) && id.length <= 20 ? id : '';
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

  function selectSubtitleTrack(backendId, sessionId, trackId, streamBasePositionSeconds) {
    const id = safeSessionId(sessionId);
    const subtitleTrackId = safeSubtitleTrackId(trackId);
    const streamBase = Math.max(0, Math.floor(Number(streamBasePositionSeconds) || 0));
    if (!id || !subtitleTrackId || typeof global.fetch !== 'function') {
      return Promise.reject(new Error('Recording-Untertitelauswahl ist nicht verfügbar.'));
    }
    return global.fetch('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify({
        operation: 'select-subtitle-track',
        backendId: text(backendId || 'default'),
        sessionId: id,
        subtitleTrackId: subtitleTrackId,
        streamBasePositionSeconds: streamBase
      }),
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function (response) {
      if (!response || response.ok !== true) {
        return Promise.resolve(response && typeof response.json === 'function' ? response.json() : null)
          .catch(function () { return null; })
          .then(function (payload) {
            const code = payload && payload.error && payload.error.code;
            throw new Error(code || 'Recording-Untertitelauswahl fehlgeschlagen.');
          });
      }
      if (subtitleTrackId === 'off') {
        return Promise.resolve(typeof response.json === 'function' ? response.json() : {})
          .then(function (payload) { return {off: true, payload: payload}; });
      }
      const contentType = response.headers && typeof response.headers.get === 'function'
        ? text(response.headers.get('Content-Type')).toLowerCase()
        : '';
      if (contentType.indexOf('text/vtt') < 0 || typeof response.text !== 'function') {
        throw new Error('Server hat keine WebVTT-Untertitelspur geliefert.');
      }
      return response.text().then(function (webVtt) {
        if (text(webVtt).indexOf('WEBVTT') !== 0) {
          throw new Error('WebVTT-Untertitelspur ist ungültig.');
        }
        return {off: false, webVtt: webVtt};
      });
    });
  }

  function roleLabel(role) {
    switch (text(role)) {
    case 'original': return 'Original';
    case 'commentary': return 'Kommentar';
    case 'descriptive': return 'Audiodeskription';
    case 'hearing-impaired': return 'Hörgeschädigt';
    case 'forced': return 'Erzwungen';
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

  function languageCode(language) {
    const normalized = text(language).trim().toLowerCase();
    if (normalized === 'deu' || normalized === 'ger' || normalized === 'de') return 'de';
    if (normalized === 'eng' || normalized === 'en') return 'en';
    return /^[a-z]{2}$/.test(normalized) ? normalized : '';
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
    const roles = Array.isArray(source.roles) ? source.roles.map(roleLabel).filter(Boolean) : [];
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

  function subtitleTrackLabel(track, index) {
    const source = track && typeof track === 'object' ? track : {};
    const parts = [];
    const language = text(source.language).trim();
    const displayLanguage = languageLabel(language);
    const label = text(source.label).trim();
    const roles = Array.isArray(source.roles) ? source.roles.map(roleLabel).filter(Boolean) : [];
    if (displayLanguage) parts.push(displayLanguage);
    if (label && label.toLowerCase() !== language.toLowerCase() &&
        label.toLowerCase() !== displayLanguage.toLowerCase()) parts.push(label);
    roles.forEach(function (role) { parts.push(role); });
    if (source.default === true) parts.push('Standard');
    return parts.length ? parts.join(' · ') : 'Untertitel ' + String(index + 1);
  }

  function clearChildren(node) {
    if (!node) return;
    if (typeof node.replaceChildren === 'function') {
      node.replaceChildren();
      return;
    }
    while (node.firstChild && typeof node.removeChild === 'function') node.removeChild(node.firstChild);
    if (node.options && typeof node.options.length === 'number') node.options.length = 0;
  }

  function hlsProfile(profileId) {
    return profileId === 'hls-fmp4' || profileId === 'hls-ts';
  }

  function fallbackOwner(root) {
    if (!root) return null;
    if (root.__vdrSuiteRecordingFallbackOwner) return root.__vdrSuiteRecordingFallbackOwner;
    if (typeof root.querySelector !== 'function') return null;
    const fallback = root.querySelector('.recordings2-recording-fallback-shell');
    return fallback && fallback.__vdrSuiteRecordingFallbackOwner
      ? fallback.__vdrSuiteRecordingFallbackOwner
      : null;
  }

  function decoratePanel(panel, recording, backendId) {
    if (!panel || !panel.element || typeof panel.start !== 'function' ||
        typeof panel.sessionId !== 'function' || typeof panel.position !== 'function' ||
        typeof panel.state !== 'function' || typeof panel.seekAbsolute !== 'function') return panel;

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

    const subtitleRow = global.document.createElement('label');
    subtitleRow.className = 'recordings2-subtitle-track-control';
    subtitleRow.hidden = true;
    subtitleRow.textContent = 'Untertitel ';
    const subtitleSelect = global.document.createElement('select');
    subtitleSelect.setAttribute('aria-label', 'Untertitel auswählen');
    subtitleSelect.disabled = true;
    subtitleRow.appendChild(subtitleSelect);
    host.appendChild(subtitleRow);

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
    let ownerLifecycleUnsubscribe = null;
    let usingCanonicalLifecycle = false;

    let subtitleSelectionInFlight = false;
    let subtitlePreferenceTrackId = 'off';
    let selectedSubtitleTrackId = '';
    let subtitleTracks = [];
    let subtitleSelectionSupported = false;
    let managedSubtitleElement = null;
    let managedSubtitleUrl = '';
    let appliedSubtitleSessionId = '';
    let appliedSubtitleBase = -1;

    function activeFallbackOwner() {
      return fallbackOwner(shell);
    }

    function currentVideo() {
      return typeof shell.querySelector === 'function' ? shell.querySelector('video') : null;
    }

    function streamBasePosition() {
      const video = currentVideo();
      const hlsOwner = activeFallbackOwner();
      const positionOwner = hlsOwner && typeof hlsOwner.position === 'function'
        ? hlsOwner
        : panel;
      const absolute = Math.max(0, Math.floor(Number(positionOwner.position()) || 0));
      const local = Number(video && video.currentTime);
      const localSeconds = Number.isFinite(local) && local > 0 ? Math.floor(local) : 0;
      return Math.max(0, absolute - localSeconds);
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

    function clearOwnerLifecycle() {
      if (typeof ownerLifecycleUnsubscribe === 'function') {
        try { ownerLifecycleUnsubscribe(); } catch (error) {}
      }
      ownerLifecycleUnsubscribe = null;
      usingCanonicalLifecycle = false;
    }

    function revokeManagedSubtitleUrl() {
      if (managedSubtitleUrl && global.URL && typeof global.URL.revokeObjectURL === 'function') {
        try { global.URL.revokeObjectURL(managedSubtitleUrl); } catch (error) {}
      }
      managedSubtitleUrl = '';
    }

    function detachManagedSubtitle() {
      if (managedSubtitleElement) {
        try {
          if (managedSubtitleElement.track) managedSubtitleElement.track.mode = 'disabled';
          if (managedSubtitleElement.parentNode &&
              typeof managedSubtitleElement.parentNode.removeChild === 'function') {
            managedSubtitleElement.parentNode.removeChild(managedSubtitleElement);
          }
        } catch (error) {}
      }
      managedSubtitleElement = null;
      revokeManagedSubtitleUrl();
      appliedSubtitleSessionId = '';
      appliedSubtitleBase = -1;
    }

    function resetTrackPresentation() {
      clearPoll();
      detachManagedSubtitle();
      pollFailures = 0;
      activeSessionId = '';
      activeProfileId = '';
      selectedTrackId = '';
      selectionInFlight = false;
      selectedSubtitleTrackId = '';
      subtitleTracks = [];
      subtitleSelectionSupported = false;
      subtitleSelectionInFlight = false;
      clearChildren(audioSelect);
      clearChildren(subtitleSelect);
      audioRow.hidden = true;
      audioSelect.disabled = true;
      subtitleRow.hidden = true;
      subtitleSelect.disabled = true;
      subtitleInfo.hidden = true;
      subtitleInfo.textContent = '';
      note.hidden = true;
      note.textContent = '';
      host.hidden = true;
    }

    function findSubtitle(trackId) {
      return subtitleTracks.find(function (track) {
        return safeSubtitleTrackId(track && track.id) === trackId;
      }) || null;
    }

    function mountWebVtt(track, webVtt, sessionId, basePosition) {
      const video = currentVideo();
      if (!video || !global.Blob || !global.URL || typeof global.URL.createObjectURL !== 'function') {
        throw new Error('Browser kann die WebVTT-Untertitelspur nicht einbinden.');
      }
      detachManagedSubtitle();
      const blob = new global.Blob([webVtt], {type: 'text/vtt'});
      const url = global.URL.createObjectURL(blob);
      const element = global.document.createElement('track');
      element.kind = 'subtitles';
      element.label = subtitleTrackLabel(track, 0);
      const code = languageCode(track && track.language);
      if (code) element.srclang = code;
      element.src = url;
      element.default = true;
      video.appendChild(element);
      if (element.track) element.track.mode = 'showing';
      managedSubtitleElement = element;
      managedSubtitleUrl = url;
      appliedSubtitleSessionId = sessionId;
      appliedSubtitleBase = basePosition;
    }

    function setNote(message, error) {
      note.textContent = text(message);
      note.hidden = !note.textContent;
      if (note.classList && typeof note.classList.toggle === 'function') {
        note.classList.toggle('error', Boolean(error));
      }
    }

    function applySubtitlePreference(userInitiated) {
      if (disposed || subtitleSelectionInFlight) return Promise.resolve(false);
      const id = safeSessionId(panel.sessionId()) || activeSessionId;
      const targetTrackId = safeSubtitleTrackId(subtitlePreferenceTrackId);
      if (!id || !targetTrackId || !subtitleSelectionSupported) return Promise.resolve(false);
      const base = streamBasePosition();
      if (targetTrackId !== 'off' && appliedSubtitleSessionId === id &&
          appliedSubtitleBase === base && managedSubtitleElement) return Promise.resolve(true);

      const track = targetTrackId === 'off' ? null : findSubtitle(targetTrackId);
      if (targetTrackId !== 'off' && (!track || track.selectable !== true ||
          text(track.deliveryFormat) !== 'webvtt')) return Promise.resolve(false);

      subtitleSelectionInFlight = true;
      subtitleSelect.disabled = true;
      if (userInitiated) setNote(
        targetTrackId === 'off' ? 'Untertitel werden ausgeschaltet …' : 'Untertitel werden geladen …',
        false
      );

      return selectSubtitleTrack(backendId, id, targetTrackId, base).then(function (result) {
        if (disposed) return false;
        const currentId = safeSessionId(panel.sessionId());
        const currentBase = streamBasePosition();
        if (currentId !== id || currentBase !== base) {
          detachManagedSubtitle();
          return false;
        }
        if (targetTrackId === 'off' || result.off === true) {
          detachManagedSubtitle();
          selectedSubtitleTrackId = '';
          subtitlePreferenceTrackId = 'off';
          subtitleSelect.value = 'off';
          if (userInitiated) setNote('Untertitel ausgeschaltet.', false);
        }
        else {
          mountWebVtt(track, result.webVtt, id, base);
          selectedSubtitleTrackId = targetTrackId;
          subtitleSelect.value = targetTrackId;
          if (userInitiated) setNote('Untertitel gewechselt.', false);
        }
        return true;
      }).catch(function (error) {
        if (userInitiated) {
          setNote(error && error.message
            ? 'Untertitelwechsel fehlgeschlagen: ' + error.message
            : 'Untertitelwechsel fehlgeschlagen.', true);
        }
        throw error;
      }).finally(function () {
        subtitleSelectionInFlight = false;
        subtitleSelect.disabled = subtitleRow.hidden;
        host.hidden = audioRow.hidden && subtitleRow.hidden && subtitleInfo.hidden && note.hidden;
      });
    }

    function handleOwnerLifecycle(snapshot) {
      if (disposed || !snapshot) return;
      const currentId = safeSessionId(snapshot.sessionId);
      const ownerState = text(snapshot.state);
      const transition = text(snapshot.transition);
      const sessionDetached = !currentId && (
        transition === 'transport-replaced' ||
        ownerState === 'idle' || ownerState === 'stopped' ||
        ownerState === 'destroyed' || ownerState === 'stopping' || ownerState === 'replacing'
      );

      if (sessionDetached) {
        if (activeSessionId) resetTrackPresentation();
        return;
      }

      if (currentId && currentId !== activeSessionId) {
        detachManagedSubtitle();
        activeSessionId = currentId;
        pollFailures = 0;
        refreshTracks();
        return;
      }

      if (currentId && subtitlePreferenceTrackId !== 'off' &&
          subtitleSelectionSupported && !subtitleSelectionInFlight &&
          (transition === 'seek-completed' || transition === 'transport-replaced' ||
           transition === 'session-replaced')) {
        const currentBase = streamBasePosition();
        if (appliedSubtitleSessionId !== currentId || appliedSubtitleBase !== currentBase) {
          detachManagedSubtitle();
          applySubtitlePreference(false).catch(function () {});
        }
      }
    }

    function bindOwnerLifecycle() {
      if (typeof panel.subscribe !== 'function') return false;
      clearSessionWatch();
      ownerLifecycleUnsubscribe = panel.subscribe(handleOwnerLifecycle);
      usingCanonicalLifecycle = true;
      return true;
    }

    // Compatibility only for an older/test owner that does not yet publish the
    // canonical ADR-0056 lifecycle. Production Recording owners use subscribe().
    function scheduleSessionWatch() {
      if (usingCanonicalLifecycle || disposed || sessionWatchTimer !== null ||
          typeof global.setTimeout !== 'function') return;
      const handle = global.setTimeout(function () {
        sessionWatchTimer = null;
        if (disposed || usingCanonicalLifecycle) return;
        const currentId = safeSessionId(panel.sessionId());
        if (!currentId && activeSessionId) {
          resetTrackPresentation();
        }
        else if (currentId && currentId !== activeSessionId) {
          detachManagedSubtitle();
          activeSessionId = currentId;
          pollFailures = 0;
          refreshTracks();
        }
        else if (currentId && subtitlePreferenceTrackId !== 'off' &&
                 subtitleSelectionSupported && !subtitleSelectionInFlight) {
          const currentBase = streamBasePosition();
          if (appliedSubtitleSessionId !== currentId || appliedSubtitleBase !== currentBase) {
            detachManagedSubtitle();
            applySubtitlePreference(false).catch(function () {});
          }
        }
        scheduleSessionWatch();
      }, TRACK_STATUS_POLL_MS);
      sessionWatchTimer = handle;
      if (handle && typeof handle.unref === 'function') handle.unref();
    }

    function scheduleRefresh() {
      if (disposed || pollTimer !== null || typeof global.setTimeout !== 'function') return;
      pollTimer = global.setTimeout(function () {
        pollTimer = null;
        refreshTracks();
      }, TRACK_STATUS_POLL_MS);
      if (pollTimer && typeof pollTimer.unref === 'function') pollTimer.unref();
    }

    function renderTracks(mediaSession) {
      activeProfileId = text(mediaSession && mediaSession.presentationProfileId).trim();
      const tracks = mediaSession && mediaSession.tracks;
      const audio = tracks && tracks.audio;
      const available = audio && Array.isArray(audio.availableTracks) ? audio.availableTracks : [];
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
      const allSubtitleTracks = subtitle && Array.isArray(subtitle.availableTracks)
        ? subtitle.availableTracks : [];
      subtitleTracks = allSubtitleTracks.filter(function (track) {
        return Boolean(safeSubtitleTrackId(track && track.id)) &&
          track.selectable === true && text(track.deliveryFormat) === 'webvtt';
      });
      subtitleSelectionSupported = Boolean(
        subtitle && subtitle.selectionSupported === true && subtitle.offSupported === true &&
        subtitleTracks.length > 0
      );
      selectedSubtitleTrackId = safeSubtitleTrackId(subtitle && subtitle.selectedTrackId);
      if (selectedSubtitleTrackId === 'off') selectedSubtitleTrackId = '';

      clearChildren(subtitleSelect);
      const offOption = global.document.createElement('option');
      offOption.value = 'off';
      offOption.textContent = 'Aus';
      subtitleSelect.appendChild(offOption);
      subtitleTracks.forEach(function (track, index) {
        const option = global.document.createElement('option');
        option.value = safeSubtitleTrackId(track.id);
        option.textContent = subtitleTrackLabel(track, index);
        subtitleSelect.appendChild(option);
      });
      if (subtitlePreferenceTrackId !== 'off' && findSubtitle(subtitlePreferenceTrackId)) {
        subtitleSelect.value = subtitlePreferenceTrackId;
      }
      else if (selectedSubtitleTrackId && findSubtitle(selectedSubtitleTrackId)) {
        subtitlePreferenceTrackId = selectedSubtitleTrackId;
        subtitleSelect.value = selectedSubtitleTrackId;
      }
      else {
        subtitleSelect.value = 'off';
      }
      subtitleRow.hidden = !subtitleSelectionSupported;
      subtitleSelect.disabled = !subtitleSelectionSupported || subtitleSelectionInFlight;

      const unsupportedPresent = allSubtitleTracks.length > 0 && subtitleTracks.length === 0;
      const truthfulOff = Boolean(subtitle && subtitle.offSelected === true);
      subtitleInfo.hidden = !(unsupportedPresent && truthfulOff);
      subtitleInfo.textContent = subtitleInfo.hidden
        ? ''
        : 'Untertitel: Aus · Auswahl ist in diesem Wiedergabepfad nicht verfügbar.';

      host.hidden = audioRow.hidden && subtitleRow.hidden && subtitleInfo.hidden && note.hidden;
      const reason = text(audio && audio.selectionReason);
      if (!audioSelectable && reason === 'recording_audio_track_selection_preparing') scheduleRefresh();
      else clearPoll();

      if (subtitleSelectionSupported && subtitlePreferenceTrackId !== 'off') {
        Promise.resolve().then(function () {
          return applySubtitlePreference(false);
        }).catch(function () {});
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
      }).catch(function () {
        if (disposed) return false;
        pollFailures += 1;
        if (pollFailures < TRACK_STATUS_MAX_FAILURES) scheduleRefresh();
        else {
          clearPoll();
          setNote('Track-Informationen konnten nicht geladen werden.', true);
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
        detachManagedSubtitle();
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
          setNote('Tonspur-Replacement konnte nicht verifiziert werden. Wiedergabe wurde gestoppt.', true);
        }
        else {
          selectedTrackId = previousTrackId;
          audioSelect.value = previousTrackId;
          setNote(error && error.message
            ? 'Tonspurwechsel fehlgeschlagen: ' + error.message
            : 'Tonspurwechsel fehlgeschlagen.', true);
        }
        host.hidden = false;
        throw error;
      });
    }

    function performProgressiveSelection(targetTrackId, previousTrackId, id, playbackState, position) {
      let serverSelected = false;
      let pausedForSelection = false;
      if (playbackState === 'playing') {
        if (typeof panel.pause !== 'function' || panel.pause() !== true) {
          audioSelect.value = previousTrackId;
          return Promise.reject(new Error('Wiedergabe konnte für den Tonspurwechsel nicht stabil pausiert werden.'));
        }
        pausedForSelection = true;
      }

      return selectAudioTrack(backendId, recording, id, targetTrackId, position).then(function (session) {
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
        detachManagedSubtitle();
        renderTracks(mediaSession);
        if (selectedTrackId !== targetTrackId) throw new Error('Server hat eine andere Tonspur bestätigt.');
        return Promise.resolve(panel.seekAbsolute(position)).then(function () {
          if (disposed) return false;
          if (safeSessionId(panel.sessionId()) !== id)
            throw new Error('MediaSession-ID hat sich beim Tonspur-Reconnect geändert.');
          const stateAfterReconnect = text(panel.state());
          if (playbackState === 'paused' && stateAfterReconnect !== 'paused') {
            if (typeof panel.pause !== 'function' || panel.pause() !== true)
              throw new Error('Pause-Zustand konnte nach dem Tonspurwechsel nicht erhalten werden.');
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
          setNote('Tonspur wurde serverseitig gewechselt, aber der Player konnte nicht neu verbunden werden. Wiedergabe wurde gestoppt.', true);
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
          setNote(error && error.message
            ? 'Tonspurwechsel fehlgeschlagen: ' + error.message
            : 'Tonspurwechsel fehlgeschlagen.', true);
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
      const hlsOwner = hlsProfile(activeProfileId) ? activeFallbackOwner() : null;
      const usingHlsOwner = Boolean(
        hlsOwner && typeof hlsOwner.selectAudioTrack === 'function' && typeof hlsOwner.state === 'function');
      const playbackState = usingHlsOwner ? text(hlsOwner.state()) : text(panel.state());
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
      const operation = usingHlsOwner
        ? performHlsSelection(targetTrackId, previousTrackId, id, playbackState)
        : performProgressiveSelection(targetTrackId, previousTrackId, id, playbackState, position);
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

    subtitleSelect.addEventListener('change', function () {
      const target = safeSubtitleTrackId(subtitleSelect.value);
      if (!target) return;
      const previousPreference = subtitlePreferenceTrackId;
      subtitlePreferenceTrackId = target;
      applySubtitlePreference(true).catch(function () {
        subtitlePreferenceTrackId = previousPreference;
        subtitleSelect.value = previousPreference;
      });
    });

    const wrapped = {};
    Object.keys(panel).forEach(function (key) { wrapped[key] = panel[key]; });
    wrapped.element = shell;

    wrapped.start = function () {
      const result = panel.start.apply(panel, arguments);
      if (usingCanonicalLifecycle) return result;
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
        if (!usingCanonicalLifecycle) {
          resetTrackPresentation();
          scheduleSessionWatch();
        }
        return panel.stop.apply(panel, arguments);
      };
    }

    if (typeof panel.relinquishForReplacement === 'function') {
      wrapped.relinquishForReplacement = function () {
        clearPoll();
        clearSessionWatch();
        clearOwnerLifecycle();
        detachManagedSubtitle();
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
        clearOwnerLifecycle();
        detachManagedSubtitle();
        subtitlePreferenceTrackId = 'off';
        panel.destroy.apply(panel, arguments);
      };
    }

    if (!bindOwnerLifecycle()) scheduleSessionWatch();
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
      safeSubtitleTrackId: safeSubtitleTrackId,
      recordingCapabilities: recordingCapabilities,
      audioTrackLabel: audioTrackLabel,
      subtitleTrackLabel: subtitleTrackLabel,
      languageLabel: languageLabel,
      languageCode: languageCode,
      fallbackOwner: fallbackOwner,
      trackStatus: trackStatus,
      selectAudioTrack: selectAudioTrack,
      selectSubtitleTrack: selectSubtitleTrack
    })
  });
}(window));
