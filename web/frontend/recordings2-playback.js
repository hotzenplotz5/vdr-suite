// Browser playback owner for Phase-65 Recording MediaSessions.
(function (global) {
  'use strict';

  const POLL_INTERVAL_MS = 1000;
  const BUFFER_HISTORY_SECONDS = 90;
  const STARTUP_BUFFER_SECONDS = 12;
  const REBUFFER_RESUME_SECONDS = 12;
  const BUFFER_EPSILON_SECONDS = 0.25;

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
      credentials: 'same-origin'
    });
  }

  // Page teardown cannot rely on VdrSuiteClientApi.requestJson because its
  // generic request options intentionally do not guarantee Fetch keepalive.
  // Use a direct same-origin Fetch for the small stop request so closing or
  // navigating away from a browser tab can still release the server worker.
  function stopSessionKeepalive(backendId, sessionId) {
    const id = safeSessionId(sessionId);
    if (!id || typeof global.fetch !== 'function') return Promise.resolve(null);

    return global.fetch('/api/media/sessions', {
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
    }).catch(function () {
      // Teardown is best-effort; daemon shutdown recovery remains the final
      // ownership fence if the browser disappears before the request lands.
      return null;
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
    const segmentDurations = [];
    let pendingDuration = 0;
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
      if (line.indexOf('#EXTINF:') === 0) {
        const comma = line.indexOf(',');
        const rawDuration = line.slice(8, comma === -1 ? line.length : comma);
        const duration = Number(rawDuration);
        pendingDuration = Number.isFinite(duration) && duration > 0 ? duration : 0;
        return;
      }
      if (line.charAt(0) === '#') return;
      const segment = safeArtifactName(line);
      if (segment) {
        segments.push(segment);
        segmentDurations.push(pendingDuration);
      }
      pendingDuration = 0;
    });

    return {
      initSegment: initSegment,
      segments: segments,
      segmentDurations: segmentDurations,
      ended: ended
    };
  }

  function startupBatch(manifest, targetSeconds) {
    const segments = manifest && Array.isArray(manifest.segments)
      ? manifest.segments
      : [];
    const durations = manifest && Array.isArray(manifest.segmentDurations)
      ? manifest.segmentDurations
      : [];
    const target = Math.max(0, Number(targetSeconds) || 0);
    const selected = [];
    let duration = 0;

    for (let index = 0; index < segments.length; index += 1) {
      selected.push(segments[index]);
      const segmentDuration = Number(durations[index] || 0);
      if (Number.isFinite(segmentDuration) && segmentDuration > 0) {
        duration += segmentDuration;
      }
      if (duration + BUFFER_EPSILON_SECONDS >= target) {
        return {ready: true, segments: selected, duration: duration};
      }
    }

    return {
      ready: Boolean(manifest && manifest.ended && selected.length > 0),
      segments: selected,
      duration: duration
    };
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

  function bytesView(value) {
    if (!value || typeof value.byteLength !== 'number') return null;
    try {
      if (value.buffer && typeof value.byteOffset === 'number') {
        return new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
      }
      return new Uint8Array(value);
    } catch (error) {
      return null;
    }
  }

  function concatArrayBuffers(values) {
    const views = [];
    let total = 0;
    (values || []).forEach(function (value) {
      const view = bytesView(value);
      if (!view) throw new Error('Media-Segment enthält keine gültigen Binärdaten.');
      views.push(view);
      total += view.byteLength;
    });
    const combined = new Uint8Array(total);
    let offset = 0;
    views.forEach(function (view) {
      combined.set(view, offset);
      offset += view.byteLength;
    });
    return combined.buffer;
  }

  function hexByte(value) {
    return Number(value).toString(16).padStart(2, '0');
  }

  function boxTypeAt(bytes, offset, type) {
    if (offset < 0 || offset + 4 > bytes.length || type.length !== 4) return false;
    for (let index = 0; index < 4; index += 1) {
      if (bytes[offset + index] !== type.charCodeAt(index)) return false;
    }
    return true;
  }

  function uint32At(bytes, offset) {
    if (offset < 0 || offset + 4 > bytes.length) return 0;
    return (((bytes[offset] << 24) >>> 0) +
      (bytes[offset + 1] << 16) +
      (bytes[offset + 2] << 8) +
      bytes[offset + 3]) >>> 0;
  }

  function findBox(bytes, type) {
    for (let typeOffset = 4; typeOffset + 4 <= bytes.length; typeOffset += 1) {
      if (!boxTypeAt(bytes, typeOffset, type)) continue;
      const start = typeOffset - 4;
      const size = uint32At(bytes, start);
      if (size < 8 || start + size > bytes.length) continue;
      return {payloadStart: start + 8, end: start + size};
    }
    return null;
  }

  function avcCodecFromInitSegment(value) {
    const bytes = bytesView(value);
    if (!bytes) return '';
    const box = findBox(bytes, 'avcC');
    if (!box || box.payloadStart + 4 > box.end || bytes[box.payloadStart] !== 1) return '';
    return 'avc1.' +
      hexByte(bytes[box.payloadStart + 1]) +
      hexByte(bytes[box.payloadStart + 2]) +
      hexByte(bytes[box.payloadStart + 3]);
  }

  function descriptorLength(bytes, offset, end) {
    let length = 0;
    for (let count = 0; count < 4 && offset < end; count += 1, offset += 1) {
      const value = bytes[offset];
      length = (length << 7) | (value & 0x7f);
      if ((value & 0x80) === 0) return {length: length, next: offset + 1};
    }
    return null;
  }

  function audioObjectType(bytes, offset, end) {
    if (offset >= end) return 0;
    let type = bytes[offset] >> 3;
    if (type === 31) {
      if (offset + 1 >= end) return 0;
      type = 32 + (((bytes[offset] & 0x07) << 3) | (bytes[offset + 1] >> 5));
    }
    return type;
  }

  function aacCodecFromInitSegment(value) {
    const bytes = bytesView(value);
    if (!bytes) return '';
    const box = findBox(bytes, 'esds');
    if (!box) return '';

    // esds is a FullBox, so the first four payload bytes are version/flags.
    // DecoderSpecificInfo (tag 0x05) contains the MPEG-4 AudioSpecificConfig.
    for (let offset = box.payloadStart + 4; offset < box.end; offset += 1) {
      if (bytes[offset] !== 0x05) continue;
      const descriptor = descriptorLength(bytes, offset + 1, box.end);
      if (!descriptor || descriptor.length < 2 || descriptor.next + descriptor.length > box.end) continue;
      const type = audioObjectType(bytes, descriptor.next, descriptor.next + descriptor.length);
      if (type > 0 && type <= 63) return 'mp4a.40.' + String(type);
    }
    return '';
  }

  function mimeTypeFromInitSegment(value) {
    const videoCodec = avcCodecFromInitSegment(value);
    const audioCodec = aacCodecFromInitSegment(value);
    if (!videoCodec && !audioCodec) return '';
    const codecs = [];
    if (videoCodec) codecs.push(videoCodec);
    if (audioCodec) codecs.push(audioCodec);
    return (videoCodec ? 'video/mp4' : 'audio/mp4') + '; codecs="' + codecs.join(',') + '"';
  }

  function supportedMimeType(initBytes) {
    const MediaSource = global.MediaSource;
    if (!MediaSource || typeof MediaSource.isTypeSupported !== 'function') return '';
    const mimeType = mimeTypeFromInitSegment(initBytes);
    if (!mimeType || !MediaSource.isTypeSupported(mimeType)) return '';
    return mimeType;
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

  function bufferedAheadSeconds(sourceBuffer, video) {
    const buffered = sourceBuffer && sourceBuffer.buffered;
    if (!buffered || buffered.length === 0) return 0;
    const currentTime = Math.max(0, Number(video && video.currentTime) || 0);

    for (let index = 0; index < buffered.length; index += 1) {
      const start = Number(buffered.start(index));
      const end = Number(buffered.end(index));
      if (!Number.isFinite(start) || !Number.isFinite(end) || !(end > start)) continue;
      if (currentTime + BUFFER_EPSILON_SECONDS >= start && currentTime <= end) {
        return Math.max(0, end - Math.max(currentTime, start));
      }
    }

    const firstStart = Number(buffered.start(0));
    const firstEnd = Number(buffered.end(0));
    if (Number.isFinite(firstStart) && Number.isFinite(firstEnd) && currentTime < firstStart) {
      return Math.max(0, firstEnd - firstStart);
    }
    return 0;
  }

  function bufferReady(sourceBuffer, video, targetSeconds, ended) {
    const ahead = bufferedAheadSeconds(sourceBuffer, video);
    const target = Math.max(0, Number(targetSeconds) || 0);
    return ahead + BUFFER_EPSILON_SECONDS >= target || (Boolean(ended) && ahead > 0);
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
    let playbackStarted = false;
    let rebuffering = false;
    let playbackFailed = false;
    let stopIssued = false;
    let pageHidden = false;
    let activeSessionId = '';
    let pollTimer = null;
    let mediaSource = null;
    let objectUrl = '';
    let abortController = null;
    let activeSourceBuffer = null;
    const appendedSegments = new Set();
    let initAppended = false;

    function setStatus(message, error) {
      status.textContent = message;
      status.classList.toggle('error', Boolean(error));
    }

    function bufferText(sourceBuffer) {
      return bufferedAheadSeconds(sourceBuffer, video).toFixed(1) + ' s';
    }

    function stopActiveSession(useKeepalive) {
      if (stopIssued || !activeSessionId) return;
      stopIssued = true;
      const request = useKeepalive
        ? stopSessionKeepalive(backendId, activeSessionId)
        : stopSession(backendId, activeSessionId);
      if (request && typeof request.catch === 'function') {
        request.catch(function () {
          // Browser teardown is best-effort. Daemon shutdown recovery remains
          // the final fence for an ungraceful browser/process loss.
        });
      }
    }

    function handlePageHide() {
      if (destroyed) return;
      pageHidden = true;
      stopActiveSession(true);
    }

    function handleEnded() {
      if (destroyed) return;
      stopActiveSession(false);
    }

    function schedulePump(callback) {
      if (destroyed) return;
      pollTimer = global.setTimeout(callback, POLL_INTERVAL_MS);
    }

    function resumeAfterRebuffer(sourceBuffer, ended) {
      if (!rebuffering || destroyed) return;
      if (!bufferReady(sourceBuffer, video, REBUFFER_RESUME_SECONDS, ended)) {
        setStatus(
          'Puffer wird nachgeladen · ' + bufferText(sourceBuffer) +
            ' / ' + REBUFFER_RESUME_SECONDS + ' s',
          false
        );
        return;
      }

      rebuffering = false;
      setStatus('Puffer wieder aufgebaut · ' + bufferText(sourceBuffer), false);
      const request = video.play();
      if (request && typeof request.catch === 'function') {
        request.catch(function () {
          // Playback was already user-authorized by the initial start action.
          // Browser controls remain available if a platform still requires a tap.
        });
      }
    }

    function prepareSourceBuffer(mediaPath) {
      return fetchText(mediaPath, abortController.signal)
        .then(function (manifestText) {
          const manifest = parsePlaylist(manifestText);
          if (!manifest.initSegment) {
            throw new Error('HLS-Manifest enthält kein fMP4-Init-Segment.');
          }
          return fetchBytes(
            artifactUrl(mediaPath, manifest.initSegment),
            abortController.signal
          );
        })
        .then(function (initBytes) {
          if (destroyed) return null;
          const declaredMimeType = mimeTypeFromInitSegment(initBytes);
          if (!declaredMimeType) {
            throw new Error('fMP4-Init-Segment enthält keine unterstützte H.264/AAC-Codec-Konfiguration.');
          }
          const mimeType = supportedMimeType(initBytes);
          if (!mimeType) {
            throw new Error('Browser unterstützt die tatsächliche fMP4-Codec-Konfiguration nicht (' + declaredMimeType + ').');
          }
          const sourceBuffer = mediaSource.addSourceBuffer(mimeType);
          sourceBuffer.mode = 'segments';
          activeSourceBuffer = sourceBuffer;
          return appendBytes(sourceBuffer, initBytes).then(function () {
            initAppended = true;
            return {sourceBuffer: sourceBuffer, mimeType: mimeType};
          });
        });
    }

    function fetchStartupBatch(mediaPath, batch, sourceBuffer) {
      setStatus(
        'Startpuffer bereit · ' + batch.duration.toFixed(1) + ' s werden geladen.',
        false
      );
      return Promise.all(batch.segments.map(function (segmentName) {
        return fetchBytes(artifactUrl(mediaPath, segmentName), abortController.signal);
      })).then(function (buffers) {
        if (destroyed) return;
        return appendBytes(sourceBuffer, concatArrayBuffers(buffers)).then(function () {
          batch.segments.forEach(function (segmentName) {
            appendedSegments.add(segmentName);
          });
          setStatus(
            'Startpuffer aufgebaut · ' + bufferText(sourceBuffer) +
              ' · Wiedergabe startet.',
            false
          );
        });
      });
    }

    function pump(mediaPath, sourceBuffer) {
      if (destroyed) return Promise.resolve();
      return fetchText(mediaPath, abortController.signal)
        .then(function (manifestText) {
          if (destroyed) return null;
          const manifest = parsePlaylist(manifestText);
          if (!manifest.initSegment || !initAppended) {
            throw new Error('HLS-Stream besitzt kein initialisiertes fMP4-Segment.');
          }

          if (appendedSegments.size === 0) {
            const batch = startupBatch(manifest, STARTUP_BUFFER_SECONDS);
            if (!batch.ready) {
              setStatus(
                'Startpuffer wird aufgebaut · ' + batch.duration.toFixed(1) +
                  ' / ' + STARTUP_BUFFER_SECONDS + ' s',
                false
              );
              schedulePump(function () {
                pump(mediaPath, sourceBuffer).catch(handlePlaybackError);
              });
              return null;
            }
            return fetchStartupBatch(mediaPath, batch, sourceBuffer).then(function () {
              if (destroyed) return;
              if (manifest.ended) {
                if (mediaSource.readyState === 'open' && !sourceBuffer.updating) {
                  mediaSource.endOfStream();
                }
                return;
              }
              schedulePump(function () {
                pump(mediaPath, sourceBuffer).catch(handlePlaybackError);
              });
            });
          }

          let chain = Promise.resolve();
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
                  if (rebuffering) {
                    resumeAfterRebuffer(sourceBuffer, manifest.ended);
                  }
                  else if (playbackStarted) {
                    setStatus(
                      'Wiedergabe läuft · Puffer ' + bufferText(sourceBuffer) +
                        ' · ' + appendedSegments.size + ' Segment(e) empfangen.',
                      false
                    );
                  }
                });
              });
          });

          return chain.then(function () {
            if (destroyed) return;
            if (manifest.ended) {
              if (mediaSource.readyState === 'open' && !sourceBuffer.updating) {
                mediaSource.endOfStream();
              }
              resumeAfterRebuffer(sourceBuffer, true);
              setStatus('Wiedergabe läuft · Stream vollständig erzeugt.', false);
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
      stopActiveSession(false);
      setStatus(error && error.message ? error.message : String(error || 'Wiedergabefehler'), true);
      startButton.disabled = false;
    }

    function handlePlaying() {
      if (destroyed) return;
      playbackStarted = true;
      rebuffering = false;
      if (activeSourceBuffer) {
        setStatus('Wiedergabe läuft · Puffer ' + bufferText(activeSourceBuffer), false);
      }
    }

    function handleWaiting() {
      if (destroyed || playbackFailed || !playbackStarted || rebuffering) return;
      rebuffering = true;
      try { video.pause(); } catch (error) {}
      if (activeSourceBuffer) {
        setStatus(
          'Puffer wird nachgeladen · ' + bufferText(activeSourceBuffer) +
            ' / ' + REBUFFER_RESUME_SECONDS + ' s',
          false
        );
      }
    }

    function start() {
      if (started || destroyed) return;
      started = true;
      startButton.disabled = true;
      setStatus('MediaSession wird vorbereitet …', false);

      const MediaSource = global.MediaSource;
      if (!MediaSource || typeof MediaSource.isTypeSupported !== 'function') {
        started = false;
        startButton.disabled = false;
        setStatus('Dieser Browser unterstützt den benötigten MediaSource-Pfad nicht.', true);
        return;
      }

      abortController = new global.AbortController();
      mediaSource = new global.MediaSource();
      objectUrl = global.URL.createObjectURL(mediaSource);
      video.src = objectUrl;
      video.hidden = false;

      // Keep the user-initiated play request pending while the HLS worker builds
      // the startup buffer. The first media append is deliberately a >=12 s
      // batch, so playback cannot consume the first 4 s segment immediately.
      const playRequest = video.play();
      if (playRequest && typeof playRequest.catch === 'function') playRequest.catch(function () {});

      const sessionPromise = createSession(backendId, recording).then(function (session) {
        const mediaSession = session && session.mediaSession;
        activeSessionId = safeSessionId(mediaSession && mediaSession.id);
        if (pageHidden) stopActiveSession(true);
        else if (destroyed || playbackFailed) stopActiveSession(false);
        return session;
      });

      Promise.all([
        sessionPromise,
        waitForSourceOpen(mediaSource)
      ]).then(function (results) {
        if (destroyed || pageHidden) return null;
        const session = results[0];
        const mediaSession = session && session.mediaSession;
        const mediaPath = mediaSession && text(mediaSession.mediaPath);
        if (!activeSessionId || !mediaSession || mediaSession.state !== 'ready' || !mediaPath) {
          throw new Error('MediaSession wurde nicht wiedergabebereit bereitgestellt.');
        }
        return prepareSourceBuffer(mediaPath).then(function (prepared) {
          if (destroyed || pageHidden || !prepared) return;
          startButton.hidden = true;
          setStatus(
            'Streaming vorbereitet · ' + text(mediaSession.presentationProfileId || 'hls-fmp4') +
              ' · Startpuffer ' + STARTUP_BUFFER_SECONDS + ' s',
            false
          );
          return pump(mediaPath, prepared.sourceBuffer);
        });
      }).catch(handlePlaybackError);
    }

    function destroy() {
      if (destroyed) return;
      destroyed = true;
      if (typeof global.removeEventListener === 'function') {
        global.removeEventListener('pagehide', handlePageHide);
      }
      stopActiveSession(false);
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

    if (typeof global.addEventListener === 'function') {
      global.addEventListener('pagehide', handlePageHide);
    }
    video.addEventListener('playing', handlePlaying);
    video.addEventListener('waiting', handleWaiting);
    video.addEventListener('ended', handleEnded);
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
      stopSessionKeepalive: stopSessionKeepalive,
      safeArtifactName: safeArtifactName,
      parsePlaylist: parsePlaylist,
      startupBatch: startupBatch,
      concatArrayBuffers: concatArrayBuffers,
      artifactUrl: artifactUrl,
      avcCodecFromInitSegment: avcCodecFromInitSegment,
      aacCodecFromInitSegment: aacCodecFromInitSegment,
      mimeTypeFromInitSegment: mimeTypeFromInitSegment,
      supportedMimeType: supportedMimeType,
      bufferedAheadSeconds: bufferedAheadSeconds,
      bufferReady: bufferReady,
      startupBufferSeconds: STARTUP_BUFFER_SECONDS,
      rebufferResumeSeconds: REBUFFER_RESUME_SECONDS
    })
  });
}(window));
