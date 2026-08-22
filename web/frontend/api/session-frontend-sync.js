(function (global) {
  'use strict';

  let bound = false;
  let initialized = false;
  let previousAuthenticated = false;
  let reloadScheduled = false;

  function pageIsLeaving() {
    return Boolean(
      global.document &&
      global.document.visibilityState === 'hidden'
    );
  }

  function refreshBackendSelection() {
    if (typeof global.loadBackendSelection === 'function') {
      global.loadBackendSelection();
    }
  }

  function reloadFrontend() {
    if (reloadScheduled || pageIsLeaving()) {
      return;
    }

    reloadScheduled = true;

    if (global.location && typeof global.location.reload === 'function') {
      global.location.reload();
    }
  }

  function sessionChanged(state) {
    const authenticated = Boolean(state && state.authenticated);

    if (!initialized) {
      initialized = true;
      previousAuthenticated = authenticated;
      return;
    }

    const wasAuthenticated = previousAuthenticated;
    previousAuthenticated = authenticated;

    if (!wasAuthenticated && authenticated) {
      refreshBackendSelection();
      return;
    }

    if (wasAuthenticated && !authenticated) {
      reloadFrontend();
    }
  }

  function bindSession() {
    if (bound) {
      return true;
    }

    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.subscribe !== 'function') {
      return false;
    }

    bound = true;
    session.subscribe(sessionChanged);
    return true;
  }

  function start() {
    if (bindSession()) {
      return;
    }

    let attempts = 0;

    function retry() {
      attempts += 1;

      if (bindSession() || attempts >= 20 || typeof global.setTimeout !== 'function') {
        return;
      }

      global.setTimeout(retry, 50);
    }

    retry();
  }

  if (global.document && global.document.readyState === 'loading') {
    global.document.addEventListener('DOMContentLoaded', start, {once: true});
  } else {
    start();
  }
}(window));

// Phase-65.B Live-TV and the first Phase-65.C completed-Recording startup
// vertical use Suite-owned continuous fMP4 streams. The original Recording
// HLS/MSE player remains available as a compatibility fallback for sources or
// transformations that cannot use the low-latency continuous path.
(function (global) {
  'use strict';

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function safeSessionId(value) {
    const id = text(value).trim();
    return id && id.length <= 128 && /^[A-Za-z0-9._:-]+$/.test(id) ? id : '';
  }

  function channelId(channel) {
    if (!channel || typeof channel !== 'object') return '';
    return text(channel.channelId || channel.id || channel.nativeId).trim();
  }

  function recordingId(recording) {
    if (!recording || typeof recording !== 'object') return '';
    return text(recording.recordingId || recording.id).trim();
  }

  function safeLiveMediaPath(value) {
    const path = text(value).trim();
    if (!path || path.indexOf('?') !== -1 || path.indexOf('#') !== -1) return '';
    return /^\/api\/media\/sessions\/[A-Za-z0-9._:-]+\/live\/stream\.mp4$/.test(path)
      ? path
      : '';
  }

  function safeRecordingMediaPath(value) {
    const path = text(value).trim();
    if (!path || path.indexOf('?') !== -1 || path.indexOf('#') !== -1) return '';
    return /^\/api\/media\/sessions\/[A-Za-z0-9._:-]+\/recording\/stream\.mp4$/.test(path)
      ? path
      : '';
  }

  function publicMediaPath(canonical) {
    if (!canonical) return '';
    const publicUrl = global.VdrSuitePublicUrl;
    if (!publicUrl || typeof publicUrl.resolvePath !== 'function') return canonical;

    try {
      return text(publicUrl.resolvePath(canonical)).trim();
    } catch (error) {
      return '';
    }
  }

  function publicLiveMediaPath(value) {
    return publicMediaPath(safeLiveMediaPath(value));
  }

  function publicRecordingMediaPath(value) {
    return publicMediaPath(safeRecordingMediaPath(value));
  }

  function browserProgressiveCapabilities() {
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

  function liveCapabilities() {
    return browserProgressiveCapabilities();
  }

  function recordingCapabilities() {
    return browserProgressiveCapabilities();
  }

  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function createLiveSession(backendId, channel, replacesSessionId) {
    const api = global.VdrSuiteClientApi;
    const id = channelId(channel);
    const replacement = replacesSessionId ? safeSessionId(replacesSessionId) : '';
    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('Client API für Live-TV ist nicht verfügbar.'));
    }
    if (!id) return Promise.reject(new Error('Der Kanal besitzt keine öffentliche Channel-ID.'));
    if (replacesSessionId && !replacement) {
      return Promise.reject(new Error('Die zu ersetzende Live-Session-ID ist ungültig.'));
    }

    const body = {
      resourceKind: 'live-channel',
      backendId: text(backendId || 'default'),
      channelId: id,
      capabilities: liveCapabilities()
    };
    if (replacement) body.replacesSessionId = replacement;

    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify(body),
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function createRecordingSession(backendId, recording) {
    const api = global.VdrSuiteClientApi;
    const id = recordingId(recording);
    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('Client API für Aufnahme-Wiedergabe ist nicht verfügbar.'));
    }
    if (!id) {
      return Promise.reject(new Error('Die Aufnahme besitzt keine öffentliche Recording-ID.'));
    }

    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify({
        backendId: text(backendId || 'default'),
        recordingId: id,
        capabilities: recordingCapabilities()
      }),
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function stopLiveSession(backendId, sessionId, keepalive) {
    const id = safeSessionId(sessionId);
    if (!id) return Promise.resolve(null);
    const body = JSON.stringify({
      resourceKind: 'live-channel',
      backendId: text(backendId || 'default'),
      sessionId: id,
      operation: 'stop'
    });

    if (keepalive && typeof global.fetch === 'function') {
      return global.fetch('/api/media/sessions', {
        method: 'POST',
        headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
        body: body,
        cache: 'no-store',
        credentials: 'same-origin',
        keepalive: true
      }).catch(function () { return null; });
    }

    const api = global.VdrSuiteClientApi;
    if (!api || typeof api.requestJson !== 'function') return Promise.resolve(null);
    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: body,
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function stopRecordingSession(backendId, sessionId, keepalive) {
    const id = safeSessionId(sessionId);
    if (!id) return Promise.resolve(null);
    const body = JSON.stringify({
      backendId: text(backendId || 'default'),
      sessionId: id,
      operation: 'stop'
    });

    if (keepalive && typeof global.fetch === 'function') {
      return global.fetch('/api/media/sessions', {
        method: 'POST',
        headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
        body: body,
        cache: 'no-store',
        credentials: 'same-origin',
        keepalive: true
      }).catch(function () { return null; });
    }

    const api = global.VdrSuiteClientApi;
    if (!api || typeof api.requestJson !== 'function') return Promise.resolve(null);
    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: body,
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function createLivePanel(channel, backendId, options) {
    const settings = options && typeof options === 'object' ? options : {};
    const replacement = settings.replacesSessionId
      ? safeSessionId(settings.replacesSessionId)
      : '';

    const panel = global.document.createElement('section');
    panel.className = 'recordings2-playback recordings2-live-playback';
    panel.setAttribute('aria-label', 'Live-TV wiedergeben');

    const heading = global.document.createElement('div');
    heading.className = 'recordings2-section-title';
    const title = global.document.createElement('h4');
    title.textContent = 'Live-TV · ' + text(
      channel && (channel.name || channel.channelName || channel.title || channelId(channel))
    );
    heading.appendChild(title);
    panel.appendChild(heading);

    const status = global.document.createElement('p');
    status.className = 'recordings2-playback-status';
    status.setAttribute('role', 'status');
    status.textContent = 'Live-TV wird vorbereitet …';
    panel.appendChild(status);

    const video = global.document.createElement('video');
    video.controls = true;
    video.autoplay = true;
    video.playsInline = true;
    video.preload = 'auto';
    video.style.width = '100%';
    video.style.maxHeight = '70vh';
    video.style.background = '#000';
    video.setAttribute('aria-label', 'VDR Live-TV');
    panel.appendChild(video);

    let destroyed = false;
    let started = false;
    let stopIssued = false;
    let activeSessionId = '';
    let sessionCreationPromise = Promise.resolve('');

    function setStatus(message, error) {
      status.textContent = message;
      status.classList.toggle('error', Boolean(error));
    }

    function stopActive(keepalive) {
      if (stopIssued || !activeSessionId) return;
      stopIssued = true;
      const request = stopLiveSession(backendId, activeSessionId, keepalive);
      if (request && typeof request.catch === 'function') request.catch(function () {});
    }

    function releaseVideo() {
      try { video.pause(); } catch (error) {}
      try {
        video.removeAttribute('src');
        if (typeof video.load === 'function') video.load();
      } catch (error) {}
    }

    function pageHide() {
      if (!destroyed) stopActive(true);
    }

    function startPlayback() {
      if (started || destroyed) return sessionCreationPromise;
      started = true;
      setStatus('Live-Receiver wird geöffnet …', false);

      const promise = createLiveSession(backendId, channel, replacement).then(function (session) {
        if (destroyed) return '';
        const mediaSession = session && session.mediaSession;
        const id = safeSessionId(mediaSession && mediaSession.id);
        const mediaPath = publicLiveMediaPath(mediaSession && mediaSession.mediaPath);
        if (!id || !mediaPath || !mediaSession || mediaSession.state !== 'ready') {
          throw new Error('Live-MediaSession wurde nicht direkt wiedergabebereit bereitgestellt.');
        }
        activeSessionId = id;
        video.src = mediaPath;
        video.hidden = false;
        if (typeof video.load === 'function') video.load();
        setStatus('Direktstream verbunden · warte auf ersten decodierbaren Frame …', false);
        const playRequest = video.play();
        if (playRequest && typeof playRequest.catch === 'function') {
          playRequest.catch(function () {
            setStatus('Direktstream bereit · Wiedergabe über Player starten.', false);
          });
        }
        return id;
      }).catch(function (error) {
        if (!destroyed) {
          setStatus(error && error.message ? error.message : String(error || 'Live-TV konnte nicht gestartet werden.'), true);
          stopActive(false);
        }
        return '';
      });

      sessionCreationPromise = promise;
      return promise;
    }

    function destroy() {
      if (destroyed) return;
      destroyed = true;
      if (typeof global.removeEventListener === 'function') {
        global.removeEventListener('pagehide', pageHide);
      }
      stopActive(false);
      releaseVideo();
    }

    function relinquishForReplacement() {
      if (!started || destroyed) {
        destroy();
        return Promise.resolve('');
      }
      return sessionCreationPromise.catch(function () { return ''; }).then(function (sessionId) {
        const id = safeSessionId(sessionId);
        if (!id) {
          destroy();
          return '';
        }
        stopIssued = true;
        destroyed = true;
        if (typeof global.removeEventListener === 'function') {
          global.removeEventListener('pagehide', pageHide);
        }
        releaseVideo();
        return id;
      });
    }

    video.addEventListener('playing', function () {
      if (!destroyed) setStatus('Live-TV läuft.', false);
    });
    video.addEventListener('waiting', function () {
      if (!destroyed) setStatus('Live-TV wartet auf Daten …', false);
    });
    video.addEventListener('error', function () {
      if (!destroyed) setStatus('Browser konnte den Live-Direktstream nicht wiedergeben.', true);
    });
    if (typeof global.addEventListener === 'function') {
      global.addEventListener('pagehide', pageHide);
    }

    return Object.freeze({
      element: panel,
      destroy: destroy,
      start: startPlayback,
      sessionId: function () { return activeSessionId; },
      relinquishForReplacement: relinquishForReplacement
    });
  }

  function nowMilliseconds() {
    if (global.performance && typeof global.performance.now === 'function') {
      return Number(global.performance.now()) || 0;
    }
    if (global.Date && typeof global.Date.now === 'function') {
      return Number(global.Date.now()) || 0;
    }
    return 0;
  }

  function createRecordingPanel(recording, backendId, legacyFactory) {
    const panel = global.document.createElement('section');
    panel.className = 'recordings2-playback recordings2-recording-fast-playback';
    panel.setAttribute('aria-label', 'Aufnahme wiedergeben');

    const heading = global.document.createElement('div');
    heading.className = 'recordings2-section-title';
    const title = global.document.createElement('h4');
    title.textContent = 'Wiedergabe';
    heading.appendChild(title);
    panel.appendChild(heading);

    const status = global.document.createElement('p');
    status.className = 'recordings2-playback-status';
    status.setAttribute('role', 'status');
    status.textContent = 'Bereit zum Starten.';
    panel.appendChild(status);

    const startButton = global.document.createElement('button');
    startButton.type = 'button';
    startButton.className = 'recordings2-primary';
    startButton.textContent = '▶ Aufnahme abspielen';
    panel.appendChild(startButton);

    const video = global.document.createElement('video');
    video.controls = true;
    video.playsInline = true;
    video.preload = 'auto';
    video.hidden = true;
    video.style.width = '100%';
    video.style.maxHeight = '70vh';
    video.style.background = '#000';
    video.setAttribute('aria-label', 'VDR-Aufnahme');
    panel.appendChild(video);

    let destroyed = false;
    let started = false;
    let stopIssued = false;
    let activeSessionId = '';
    let startupStartedAt = 0;
    let firstMediaReported = false;
    let fallbackPanel = null;
    let fallbackActivation = null;
    let sessionCreationPromise = Promise.resolve('');

    function setStatus(message, error) {
      status.textContent = message;
      status.classList.toggle('error', Boolean(error));
    }

    function releaseVideo() {
      try { video.pause(); } catch (error) {}
      try {
        video.removeAttribute('src');
        if (typeof video.load === 'function') video.load();
      } catch (error) {}
    }

    function stopActive(keepalive) {
      if (stopIssued || !activeSessionId) return;
      stopIssued = true;
      const request = stopRecordingSession(backendId, activeSessionId, keepalive);
      if (request && typeof request.catch === 'function') request.catch(function () {});
    }

    function replaceWithFallback() {
      if (fallbackPanel || typeof legacyFactory !== 'function') return fallbackPanel;
      fallbackPanel = legacyFactory(recording, backendId);
      if (!fallbackPanel || !fallbackPanel.element) {
        fallbackPanel = null;
        return null;
      }
      if (typeof panel.replaceWith === 'function') {
        panel.replaceWith(fallbackPanel.element);
      }
      else if (panel.parentNode && typeof panel.parentNode.replaceChild === 'function') {
        panel.parentNode.replaceChild(fallbackPanel.element, panel);
      }
      return fallbackPanel;
    }

    function activateFallback(error) {
      if (destroyed) return Promise.resolve('');
      if (fallbackActivation) return fallbackActivation;

      stopActive(false);
      releaseVideo();
      const legacy = replaceWithFallback();
      if (!legacy || typeof legacy.start !== 'function') {
        setStatus(
          error && error.message
            ? error.message
            : 'Schneller Aufnahme-Pfad ist nicht verfügbar.',
          true
        );
        return Promise.resolve('');
      }

      fallbackActivation = Promise.resolve(legacy.start()).catch(function () {
        return '';
      });
      return fallbackActivation;
    }

    function pageHide() {
      if (destroyed) return;
      if (fallbackPanel && typeof fallbackPanel.destroy === 'function') {
        fallbackPanel.destroy();
        return;
      }
      stopActive(true);
    }

    function startPlayback() {
      if (started || destroyed) return sessionCreationPromise;
      started = true;
      startButton.disabled = true;
      startupStartedAt = nowMilliseconds();
      setStatus('MediaSession wird vorbereitet …', false);

      const promise = createRecordingSession(backendId, recording).then(function (session) {
        if (destroyed) return '';
        const mediaSession = session && session.mediaSession;
        const id = safeSessionId(mediaSession && mediaSession.id);
        if (!id || !mediaSession || mediaSession.state !== 'ready') {
          throw new Error('Schnelle Recording-MediaSession wurde nicht bereitgestellt.');
        }
        // Own any successfully issued session before validating the fast-path
        // presentation. If the server legitimately selected HLS instead, the
        // fallback must STOP this provisional session before opening its own.
        activeSessionId = id;
        const mediaPath = publicRecordingMediaPath(mediaSession.mediaPath);
        if (!mediaPath || mediaSession.presentationProfileId !== 'progressive-fmp4') {
          throw new Error('Schnelle Recording-MediaSession wurde nicht bereitgestellt.');
        }

        video.src = mediaPath;
        video.hidden = false;
        startButton.hidden = true;
        if (typeof video.load === 'function') video.load();
        setStatus('Direktstream verbunden · warte auf ersten decodierbaren Frame …', false);
        const playRequest = video.play();
        if (playRequest && typeof playRequest.catch === 'function') {
          playRequest.catch(function () {
            activateFallback(new Error('Browser hat den schnellen Recording-Start abgelehnt.'));
          });
        }
        return id;
      }).catch(function (error) {
        return activateFallback(error);
      });

      sessionCreationPromise = promise;
      return promise;
    }

    function destroy() {
      if (destroyed) return;
      destroyed = true;
      if (typeof global.removeEventListener === 'function') {
        global.removeEventListener('pagehide', pageHide);
      }
      if (fallbackPanel && typeof fallbackPanel.destroy === 'function') {
        fallbackPanel.destroy();
      }
      else {
        stopActive(false);
        releaseVideo();
      }
    }

    video.addEventListener('playing', function () {
      if (destroyed || fallbackPanel || firstMediaReported) return;
      firstMediaReported = true;
      const elapsed = Math.max(0, nowMilliseconds() - startupStartedAt);
      setStatus('Aufnahme läuft · Start ' + (elapsed / 1000).toFixed(2) + ' s', false);
      if (global.console && typeof global.console.info === 'function') {
        global.console.info(
          'recording playback first-media',
          {sessionId: activeSessionId, profile: 'progressive-fmp4', startupMs: Math.round(elapsed)}
        );
      }
    });
    video.addEventListener('waiting', function () {
      if (!destroyed && !fallbackPanel && firstMediaReported) {
        setStatus('Aufnahme wartet auf Daten …', false);
      }
    });
    video.addEventListener('stalled', function () {
      if (!destroyed && !fallbackPanel && !firstMediaReported) {
        activateFallback(new Error('Schneller Recording-Stream ist vor dem ersten Frame stehen geblieben.'));
      }
    });
    video.addEventListener('ended', function () {
      if (!destroyed && !fallbackPanel) stopActive(false);
    });
    video.addEventListener('error', function () {
      if (!destroyed && !fallbackPanel) {
        activateFallback(new Error('Browser konnte den schnellen Recording-Stream nicht wiedergeben.'));
      }
    });
    startButton.addEventListener('click', startPlayback);
    if (typeof global.addEventListener === 'function') {
      global.addEventListener('pagehide', pageHide);
    }

    return Object.freeze({
      element: panel,
      destroy: destroy,
      start: startPlayback,
      sessionId: function () {
        if (fallbackPanel && typeof fallbackPanel.sessionId === 'function') {
          return fallbackPanel.sessionId();
        }
        return activeSessionId;
      },
      relinquishForReplacement: function () {
        destroy();
        return Promise.resolve('');
      }
    });
  }

  const liveOnlyFacade = Object.freeze({createLivePanel: createLivePanel});
  let currentPlayback = global.VdrSuiteRecordings2Playback || liveOnlyFacade;

  function wrapPlayback(value) {
    const source = value && typeof value === 'object' ? value : {};
    const wrapped = {};
    Object.keys(source).forEach(function (key) { wrapped[key] = source[key]; });
    if (typeof source.createPanel === 'function') {
      const legacyFactory = source.createPanel;
      wrapped.createPanel = function (recording, backendId) {
        return createRecordingPanel(recording, backendId, legacyFactory);
      };
    }
    wrapped.createLivePanel = createLivePanel;
    return Object.freeze(wrapped);
  }

  currentPlayback = wrapPlayback(currentPlayback);
  try {
    Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
      configurable: true,
      enumerable: true,
      get: function () { return currentPlayback; },
      set: function (value) { currentPlayback = wrapPlayback(value); }
    });
  } catch (error) {
    global.VdrSuiteRecordings2Playback = currentPlayback;
  }

  global.VdrSuiteLivePlayback = Object.freeze({
    createLivePanel: createLivePanel,
    __test: Object.freeze({
      liveCapabilities: liveCapabilities,
      safeLiveMediaPath: safeLiveMediaPath,
      publicLiveMediaPath: publicLiveMediaPath,
      safeSessionId: safeSessionId,
      channelId: channelId
    })
  });

  global.VdrSuiteRecordingFastPlayback = Object.freeze({
    __test: Object.freeze({
      recordingCapabilities: recordingCapabilities,
      safeRecordingMediaPath: safeRecordingMediaPath,
      publicRecordingMediaPath: publicRecordingMediaPath,
      recordingId: recordingId
    })
  });
}(window));
