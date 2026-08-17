// Browser playback owner for Phase-65 Recording MediaSessions.
(function (global) {
  'use strict';

  const POLL_INTERVAL_MS = 1000;
  const BUFFER_HISTORY_SECONDS = 90;
  const MIME_CANDIDATES = Object.freeze([
    'video/mp4; codecs="avc1.640028,mp4a.40.2"',
    'video/mp4; codecs="avc1.64001f,mp4a.40.2"',
    'video/mp4; codecs="avc1.4d401f,mp4a.40.2"',
    'video/mp4; codecs="avc1.42e01e,mp4a.40.2"'
  ]);

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function recordingId(recording) {
    if (!recording || typeof recording !== 'object') return '';
    return text(recording.recordingId || recording.id).trim();
  }

  function safeSessionId(value) {
    const id = text(value).trim();
    if (!id || id.length > 128) return '';
    return /^[A-Za-z0-9._:-]+$/.test(id) ? id : '';
  }

  function capabilities() {
    return {
      protocols: ['hls'],
      containers: ['fmp4'],
      videoCodecs: ['h264'],
      audioCodecs: ['aac'],
      supportsByteRanges: false,
      maxVideoWidth: 1920,
      maxVideoHeight: 1080
    };
  }

  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function createSession(backendId, recording) {
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
        capabilities: capabilities()
      }),
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function stopSession(backendId, sessionId) {
    const api = global.VdrSuiteClientApi;
    const id = safeSessionId(sessionId);
    if (!api || typeof api.requestJson !== 'function') return Promise.resolve(null);
    if (!id) return Promise.resolve(null);

    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify({
        operation: 'stop',
        backendId: text(backendId || 'default'),
        sessionId: id
      }),
      cache: 'no-store',
      credentials: 'same-origin',
      keepalive: true
    });
  }

  function safeArtifactName(value) {
    const name = text(value).trim();
    if (!name || name.length > 160) return '';
    if (!/^[A-Za-z0-9._-]+$/.test(name)) return '';
    if (name === '.' || name === '..' || name.indexOf('..') !== -1) return '';
    if (!/\.(?:mp4|m4s)$/i.test(name)) return '';
    return name;
  }

  function parsePlaylist(source) {
    const lines = text(source).split(/\r?\n/).map(line => line.trim()).filter(Boolean);
    let initSegment = '';
    const segments = [];
    let ended = false;

    lines.forEach(function (line) {
      if (line === '#EXT-X-ENDLIST') {
        ended = true;
        return;
      }
      if (line.indexOf('#EXT-X-MAP:') === 0) {
        const match = line.match(/URI="([^"]+)"/);
        if (match) initSegment = safeArtifactName(match[1]);
        return;
      }
      if (line.charAt(0) === '#') return;
      const segment = safeArtifactName(line);
      if (segment) segments.push(segment);
    });

    return {initSegment: initSegment, segments: segments, ended: ended};
  }

  function artifactUrl(mediaPath, artifactName) {
    const name = safeArtifactName(artifactName);
    const manifest = text(mediaPath);
    const slash = manifest.lastIndexOf('/');
    if (!name || slash <= 0 || manifest.indexOf('?') !== -1 || manifest.indexOf('#') !== -1) {
      throw new Error('Ungültiger MediaSession-Artefaktpfad.');
    }
    const base = manifest.slice(0, slash + 1);
    if (!/^\/api\/media\/sessions\/[A-Za-z0-9._:-]+\/hls\/$/.test(base)) {
      throw new Error('MediaSession-Pfad liegt außerhalb des HLS-Gateways.');
    }
    return base + name;
  }

  function fetchText(url, signal) {
    return global.fetch(url, {
      method: 'GET',
      credentials: 'same-origin',
      cache: 'no-store',
      signal: signal
    }).then(function (response) {
      if (!response || !response.ok) {
        throw new Error('HLS-Manifest konnte nicht geladen werden (' + (response ? response.status : 'network') + ').');
      }
      return response.text();
    });
  }

  function fetchBytes(url, signal) {
    return global.fetch(url, {
      method: 'GET',
      credentials: 'same-origin',
      cache: 'no-store',
      signal: signal
    }).then(function (response) {
      if (!response || !response.ok) {
        throw new Error('Media-Segment konnte nicht geladen werden (' + (response ? response.status : 'network') + ').');
      }
      return response.arrayBuffer();
    });
  }

  function supportedMimeType() {
    const MediaSource = global.MediaSource;
    if (!MediaSource || typeof MediaSource.isTypeSupported !== 'function') return '';
    for (let index = 0; index < MIME_CANDIDATES.length; index += 1) {
      if (MediaSource.isTypeSupported(MIME_CANDIDATES[index])) return MIME_CANDIDATES[index];
    }
    return '';
  }

  function waitForSourceOpen(mediaSource) {
    if (mediaSource.readyState === 'open') return Promise.resolve();
    return new Promise(function (resolve, reject) {
      function opened() {
        cleanup();
        resolve();
      }
      function failed() {
        cleanup();
        reject(new Error('MediaSource konnte nicht geöffnet werden.'));
      }
      function cleanup() {
        mediaSource.removeEventListener('sourceopen', opened);
        mediaSource.removeEventListener('sourceclose', failed);
      }
      mediaSource.addEventListener('sourceopen', opened);
      mediaSource.addEventListener('sourceclose', failed);
    });
  }

  function sourceBufferOperation(sourceBuffer, action) {
    return new Promise(function (resolve, reject) {
      function done() {
        cleanup();
        resolve();
      }
      function failed() {
        cleanup();
        reject(new Error('Browser konnte das fMP4-Segment nicht verarbeiten.'));
      }
      function cleanup() {
        sourceBuffer.removeEventListener('updateend', done);
        sourceBuffer.removeEventListener('error', failed);
      }
      sourceBuffer.addEventListener('updateend', done);
      sourceBuffer.addEventListener('error', failed);
      try {
        action();
      } catch (error) {
        cleanup();
        reject(error);
      }
    });
  }

  function appendBytes(sourceBuffer, bytes) {
    return sourceBufferOperation(sourceBuffer, function () {
      sourceBuffer.appendBuffer(bytes);
    });
  }

  function trimHistory(sourceBuffer, video) {
    const currentTime = Number(video.currentTime || 0);
    const trimUntil = currentTime - BUFFER_HISTORY_SECONDS;
    if (trimUntil <= 0 || !sourceBuffer.buffered || sourceBuffer.buffered.length === 0) {
      return Promise.resolve();
    }
    const start = sourceBuffer.buffered.start(0);
    const end = Math.min(trimUntil, sourceBuffer.buffered.end(0));
    if (!(end > start)) return Promise.resolve();
    return sourceBufferOperation(sourceBuffer, function () {
      sourceBuffer.remove(start, end);
    });
  }

  function createPanel(recording, backendId) {
    const panel = document.createElement('section');
    panel.className = 'recordings2-playback';
    panel.setAttribute('aria-label', 'Aufnahme wiedergeben');

    const heading = document.createElement('div');
    heading.className = 'recordings2-section-title';
    const title = document.createElement('h4');
    title.textContent = 'Wiedergabe';
    heading.appendChild(title);
    panel.appendChild(heading);

    const status = document.createElement('p');
    status.className = 'recordings2-playback-status';
    status.setAttribute('role', 'status');
    status.textContent = 'Bereit zum Starten.';
    panel.appendChild(status);

    const startButton = document.createElement('button');
    startButton.type = 'button';
    startButton.className = 'recordings2-primary';
    startButton.textContent = '▶ Aufnahme abspielen';
    panel.appendChild(startButton);

    const video = document.createElement('video');
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
    let playbackFailed = false;
    let stopIssued = false;
    let activeSessionId = '';
    let pollTimer = null;
    let mediaSource = null;
    let objectUrl = '';
    let abortController = null;
    const appendedSegments = new Set();
    let initAppended = false;

    function setStatus(message, error) {
      status.textContent = message;
      status.classList.toggle('error', Boolean(error));
    }

    function stopActiveSession() {
      if (stopIssued || !activeSessionId) return;
      stopIssued = true;
      stopSession(backendId, activeSessionId).catch(function () {
        // Browser teardown is best-effort. The server still owns shutdown recovery.
      });
    }

    function schedulePump(callback) {
      if (destroyed) return;
      pollTimer = global.setTimeout(callback, POLL_INTERVAL_MS);
    }

    function pump(mediaPath, sourceBuffer) {
      if (destroyed) return Promise.resolve();
      return fetchText(mediaPath, abortController.signal)
        .then(function (manifestText) {
          if (destroyed) return null;
          const manifest = parsePlaylist(manifestText);
          if (!manifest.initSegment) {
            throw new Error('HLS-Manifest enthält kein fMP4-Init-Segment.');
          }

          let chain = Promise.resolve();
          if (!initAppended) {
            chain = chain
              .then(function () {
                return fetchBytes(artifactUrl(mediaPath, manifest.initSegment), abortController.signal);
              })
              .then(function (bytes) {
                if (destroyed) return;
                return appendBytes(sourceBuffer, bytes).then(function () {
                  initAppended = true;
                });
              });
          }

          manifest.segments.forEach(function (segmentName) {
            if (appendedSegments.has(segmentName)) return;
            chain = chain
              .then(function () {
                if (destroyed) return null;
                return trimHistory(sourceBuffer, video).then(function () {
                  return fetchBytes(artifactUrl(mediaPath, segmentName), abortController.signal);
                });
              })
              .then(function (bytes) {
                if (destroyed || !bytes) return;
                return appendBytes(sourceBuffer, bytes).then(function () {
                  appendedSegments.add(segmentName);
                  setStatus('Wiedergabe läuft · ' + appendedSegments.size + ' Segment(e) empfangen.', false);
                });
              });
          });

          return chain.then(function () {
            if (destroyed) return;
            if (manifest.ended) {
              if (mediaSource.readyState === 'open' && !sourceBuffer.updating) {
                mediaSource.endOfStream();
              }
              setStatus('Aufnahme vollständig gepuffert.', false);
              return;
            }
            schedulePump(function () {
              pump(mediaPath, sourceBuffer).catch(handlePlaybackError);
            });
          });
        });
    }

    function handlePlaybackError(error) {
      if (destroyed || (error && error.name === 'AbortError')) return;
      playbackFailed = true;
      stopActiveSession();
      setStatus(error && error.message ? error.message : String(error || 'Wiedergabefehler'), true);
      startButton.disabled = false;
    }

    function start() {
      if (started || destroyed) return;
      started = true;
      startButton.disabled = true;
      setStatus('MediaSession wird vorbereitet …', false);

      const mimeType = supportedMimeType();
      if (!mimeType) {
        started = false;
        startButton.disabled = false;
        setStatus('Dieser Browser unterstützt den benötigten fMP4-MediaSource-Pfad nicht.', true);
        return;
      }

      abortController = new global.AbortController();
      mediaSource = new global.MediaSource();
      objectUrl = global.URL.createObjectURL(mediaSource);
      video.src = objectUrl;
      video.hidden = false;
      const playRequest = video.play();
      if (playRequest && typeof playRequest.catch === 'function') playRequest.catch(function () {});

      const sessionPromise = createSession(backendId, recording).then(function (session) {
        const mediaSession = session && session.mediaSession;
        activeSessionId = safeSessionId(mediaSession && mediaSession.id);
        if (destroyed || playbackFailed) stopActiveSession();
        return session;
      });

      Promise.all([
        sessionPromise,
        waitForSourceOpen(mediaSource)
      ]).then(function (results) {
        if (destroyed) return;
        const session = results[0];
        const mediaSession = session && session.mediaSession;
        const mediaPath = mediaSession && text(mediaSession.mediaPath);
        if (!activeSessionId || !mediaSession || mediaSession.state !== 'ready' || !mediaPath) {
          throw new Error('MediaSession wurde nicht wiedergabebereit bereitgestellt.');
        }
        const sourceBuffer = mediaSource.addSourceBuffer(mimeType);
        sourceBuffer.mode = 'segments';
        startButton.hidden = true;
        setStatus('Streaming gestartet · ' + text(mediaSession.presentationProfileId || 'hls-fmp4'), false);
        return pump(mediaPath, sourceBuffer);
      }).catch(handlePlaybackError);
    }

    function destroy() {
      if (destroyed) return;
      destroyed = true;
      stopActiveSession();
      if (pollTimer !== null) {
        global.clearTimeout(pollTimer);
        pollTimer = null;
      }
      if (abortController) abortController.abort();
      try { video.pause(); } catch (error) {}
      try {
        video.removeAttribute('src');
        if (typeof video.load === 'function') video.load();
      } catch (error) {}
      if (objectUrl) {
        try { global.URL.revokeObjectURL(objectUrl); } catch (error) {}
        objectUrl = '';
      }
    }

    startButton.addEventListener('click', start);

    return Object.freeze({
      element: panel,
      destroy: destroy
    });
  }

  global.VdrSuiteRecordings2Playback = Object.freeze({
    createPanel: createPanel,
    __test: Object.freeze({
      recordingId: recordingId,
      safeSessionId: safeSessionId,
      capabilities: capabilities,
      createSession: createSession,
      stopSession: stopSession,
      safeArtifactName: safeArtifactName,
      parsePlaylist: parsePlaylist,
      artifactUrl: artifactUrl,
      supportedMimeType: supportedMimeType
    })
  });
}(window));