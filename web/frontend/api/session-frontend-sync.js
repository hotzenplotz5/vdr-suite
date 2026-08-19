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

// Phase-65.B Live-TV owns a separate low-latency browser path. Recording
// playback may still lazy-load recordings2-playback.js; this facade preserves
// its recording API while always keeping createLivePanel on the direct stream.
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

  function safeLiveMediaPath(value) {
    const path = text(value).trim();
    if (!path || path.indexOf('?') !== -1 || path.indexOf('#') !== -1) return '';
    return /^\/api\/media\/sessions\/[A-Za-z0-9._:-]+\/live\/stream\.mp4$/.test(path)
      ? path
      : '';
  }

  function publicLiveMediaPath(value) {
    const canonical = safeLiveMediaPath(value);
    if (!canonical) return '';

    const publicUrl = global.VdrSuitePublicUrl;
    if (!publicUrl || typeof publicUrl.resolvePath !== 'function') return canonical;

    try {
      return text(publicUrl.resolvePath(canonical)).trim();
    } catch (error) {
      return '';
    }
  }

  function liveCapabilities() {
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

  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function createSession(backendId, channel, replacesSessionId) {
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

  function stopSession(backendId, sessionId, keepalive) {
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
      const request = stopSession(backendId, activeSessionId, keepalive);
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

      const promise = createSession(backendId, channel, replacement).then(function (session) {
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
        // Preserve strict daemon ordering: the browser tears down only its
        // decoder/socket. STOP A -> OPEN B is owned by replacesSessionId.
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

  const liveOnlyFacade = Object.freeze({createLivePanel: createLivePanel});
  let currentPlayback = global.VdrSuiteRecordings2Playback || liveOnlyFacade;

  function wrapPlayback(value) {
    const source = value && typeof value === 'object' ? value : {};
    const wrapped = {};
    Object.keys(source).forEach(function (key) { wrapped[key] = source[key]; });
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
}(window));
