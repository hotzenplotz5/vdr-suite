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
// vertical use Suite-owned continuous fMP4 streams. Browsers that expose MSE
// plus streaming Fetch consume that exact stream through SourceBuffer so the
// platform does not have to treat an open-ended fMP4 HTTP response as a native
// progressive file. This is capability based (not UA based) and keeps exactly
// one MediaSession / Gateway stream owner. The existing Recording HLS/MSE
// player remains the compatibility fallback when even the continuous MSE path
// cannot consume the selected stream.
(function (global) {
  'use strict';

  const CONTINUOUS_INIT_LIMIT_BYTES = 1024 * 1024;
  const CONTINUOUS_PENDING_LIMIT_BYTES = 8 * 1024 * 1024;
  const CONTINUOUS_BUFFER_HISTORY_SECONDS = 90;

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

  function supportsContinuousFmp4Mse() {
    const MediaSource = global.MediaSource;
    return Boolean(
      MediaSource &&
      typeof MediaSource.isTypeSupported === 'function' &&
      typeof global.fetch === 'function' &&
      typeof global.AbortController === 'function' &&
      typeof global.ReadableStream === 'function' &&
      global.URL &&
      typeof global.URL.createObjectURL === 'function' &&
      typeof global.URL.revokeObjectURL === 'function'
    );
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

  function concatBytes(left, right) {
    const first = bytesView(left);
    const second = bytesView(right);
    if (!first || !second) throw new Error('Ungültige fMP4-Bytedaten.');
    const combined = new Uint8Array(first.byteLength + second.byteLength);
    combined.set(first, 0);
    combined.set(second, first.byteLength);
    return combined;
  }

  function uint32At(bytes, offset) {
    if (offset < 0 || offset + 4 > bytes.length) return 0;
    return (((bytes[offset] << 24) >>> 0) +
      (bytes[offset + 1] << 16) +
      (bytes[offset + 2] << 8) +
      bytes[offset + 3]) >>> 0;
  }

  function boxTypeAt(bytes, offset) {
    if (offset < 0 || offset + 4 > bytes.length) return '';
    return String.fromCharCode(
      bytes[offset],
      bytes[offset + 1],
      bytes[offset + 2],
      bytes[offset + 3]
    );
  }

  function findCompleteInitEnd(value) {
    const bytes = bytesView(value);
    if (!bytes) return 0;
    let offset = 0;
    while (offset + 8 <= bytes.length) {
      const size = uint32At(bytes, offset);
      const type = boxTypeAt(bytes, offset + 4);
      if (size === 1) {
        throw new Error('64-Bit-fMP4-Boxen sind im Browser-Startpfad nicht unterstützt.');
      }
      if (size === 0) return 0;
      if (size < 8) throw new Error('Ungültige fMP4-Boxgröße.');
      if (offset + size > bytes.length) return 0;
      offset += size;
      if (type === 'moov') return offset;
    }
    return 0;
  }

  function completeMediaPrefix(value) {
    const bytes = bytesView(value);
    if (!bytes) return 0;
    let offset = 0;
    let lastMdatEnd = 0;
    while (offset + 8 <= bytes.length) {
      const size = uint32At(bytes, offset);
      const type = boxTypeAt(bytes, offset + 4);
      if (size === 1) {
        throw new Error('64-Bit-fMP4-Boxen sind im Browser-Stream nicht unterstützt.');
      }
      if (size === 0) break;
      if (size < 8) throw new Error('Ungültige fMP4-Boxgröße.');
      if (offset + size > bytes.length) break;
      offset += size;
      if (type === 'mdat') lastMdatEnd = offset;
    }
    return lastMdatEnd;
  }

  function hexByte(value) {
    return Number(value).toString(16).padStart(2, '0');
  }

  function findBox(bytes, type) {
    for (let typeOffset = 4; typeOffset + 4 <= bytes.length; typeOffset += 1) {
      if (boxTypeAt(bytes, typeOffset) !== type) continue;
      const start = typeOffset - 4;
      const size = uint32At(bytes, start);
      if (size < 8 || start + size > bytes.length) continue;
      return {payloadStart: start + 8, end: start + size};
    }
    return null;
  }

  function avcCodecFromInit(value) {
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

  function aacCodecFromInit(value) {
    const bytes = bytesView(value);
    if (!bytes) return '';
    const box = findBox(bytes, 'esds');
    if (!box) return '';
    for (let offset = box.payloadStart + 4; offset < box.end; offset += 1) {
      if (bytes[offset] !== 0x05) continue;
      const descriptor = descriptorLength(bytes, offset + 1, box.end);
      if (!descriptor || descriptor.length < 2 || descriptor.next + descriptor.length > box.end) continue;
      const type = audioObjectType(bytes, descriptor.next, descriptor.next + descriptor.length);
      if (type > 0 && type <= 63) return 'mp4a.40.' + String(type);
    }
    return '';
  }

  function mimeTypeFromContinuousInit(value) {
    const bytes = bytesView(value);
    if (!bytes || !findBox(bytes, 'moov')) return '';
    const videoCodec = avcCodecFromInit(bytes);
    const audioCodec = aacCodecFromInit(bytes);
    if (!videoCodec && !audioCodec) return '';
    const codecs = [];
    if (videoCodec) codecs.push(videoCodec);
    if (audioCodec) codecs.push(audioCodec);
    return (videoCodec ? 'video/mp4' : 'audio/mp4') + '; codecs="' + codecs.join(',') + '"';
  }

  function waitForSourceOpen(mediaSource) {
    if (mediaSource.readyState === 'open') return Promise.resolve();
    return new Promise(function (resolve, reject) {
      function opened() {
        cleanup();
        resolve();
      }
      function closed() {
        cleanup();
        reject(new Error('MediaSource wurde vor dem Öffnen geschlossen.'));
      }
      function cleanup() {
        mediaSource.removeEventListener('sourceopen', opened);
        mediaSource.removeEventListener('sourceclose', closed);
      }
      mediaSource.addEventListener('sourceopen', opened);
      mediaSource.addEventListener('sourceclose', closed);
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
        reject(new Error('Browser konnte kontinuierliche fMP4-Daten nicht verarbeiten.'));
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

  function appendBytes(sourceBuffer, value) {
    const bytes = bytesView(value);
    if (!bytes || bytes.byteLength === 0) return Promise.resolve();
    return sourceBufferOperation(sourceBuffer, function () {
      sourceBuffer.appendBuffer(bytes);
    });
  }

  function trimContinuousHistory(sourceBuffer, video) {
    const currentTime = Math.max(0, Number(video && video.currentTime) || 0);
    const removeUntil = currentTime - CONTINUOUS_BUFFER_HISTORY_SECONDS;
    if (removeUntil <= 0 || !sourceBuffer.buffered || sourceBuffer.buffered.length === 0) {
      return Promise.resolve();
    }
    const start = Number(sourceBuffer.buffered.start(0));
    const end = Math.min(removeUntil, Number(sourceBuffer.buffered.end(0)));
    if (!Number.isFinite(start) || !Number.isFinite(end) || !(end > start)) {
      return Promise.resolve();
    }
    return sourceBufferOperation(sourceBuffer, function () {
      sourceBuffer.remove(start, end);
    });
  }

  function createContinuousFmp4Mse(video, mediaPath, onFailure) {
    if (!supportsContinuousFmp4Mse()) return null;

    const MediaSource = global.MediaSource;
    const mediaSource = new MediaSource();
    const abortController = new global.AbortController();
    const objectUrl = global.URL.createObjectURL(mediaSource);
    let reader = null;
    let sourceBuffer = null;
    let pending = new Uint8Array(0);
    let destroyed = false;
    let initAppended = false;

    function destroy() {
      if (destroyed) return;
      destroyed = true;
      try { abortController.abort(); } catch (error) {}
      if (reader && typeof reader.cancel === 'function') {
        try {
          const cancellation = reader.cancel();
          if (cancellation && typeof cancellation.catch === 'function') {
            cancellation.catch(function () {});
          }
        } catch (error) {}
      }
      try { global.URL.revokeObjectURL(objectUrl); } catch (error) {}
    }

    function fail(error) {
      if (destroyed || (error && error.name === 'AbortError')) return;
      if (typeof onFailure === 'function') onFailure(error);
    }

    function appendAvailableMedia() {
      if (!sourceBuffer || pending.byteLength === 0) return Promise.resolve();
      const prefixLength = completeMediaPrefix(pending);
      if (prefixLength <= 0) {
        if (pending.byteLength > CONTINUOUS_PENDING_LIMIT_BYTES) {
          return Promise.reject(new Error('Kontinuierlicher fMP4-Puffer enthält kein vollständiges Medienfragment.'));
        }
        return Promise.resolve();
      }
      const ready = pending.slice(0, prefixLength);
      pending = pending.slice(prefixLength);
      return trimContinuousHistory(sourceBuffer, video).then(function () {
        return appendBytes(sourceBuffer, ready);
      });
    }

    function initializeSourceBuffer() {
      if (initAppended) return Promise.resolve();
      const initEnd = findCompleteInitEnd(pending);
      if (initEnd <= 0) {
        if (pending.byteLength > CONTINUOUS_INIT_LIMIT_BYTES) {
          return Promise.reject(new Error('Kontinuierlicher fMP4-Stream liefert kein begrenztes Init-Segment.'));
        }
        return Promise.resolve();
      }

      const initBytes = pending.slice(0, initEnd);
      const mimeType = mimeTypeFromContinuousInit(initBytes);
      if (!mimeType || !MediaSource.isTypeSupported(mimeType)) {
        return Promise.reject(new Error(
          'Browser-MSE unterstützt die tatsächliche kontinuierliche fMP4-Codec-Konfiguration nicht' +
          (mimeType ? ' (' + mimeType + ')' : '') + '.'
        ));
      }

      sourceBuffer = mediaSource.addSourceBuffer(mimeType);
      sourceBuffer.mode = 'segments';
      pending = pending.slice(initEnd);
      return appendBytes(sourceBuffer, initBytes).then(function () {
        initAppended = true;
        return appendAvailableMedia();
      });
    }

    function pump() {
      if (destroyed || !reader) return Promise.resolve();
      return reader.read().then(function (result) {
        if (destroyed) return;
        if (!result || result.done) {
          return appendAvailableMedia().then(function () {
            if (mediaSource.readyState === 'open' && sourceBuffer && !sourceBuffer.updating) {
              try { mediaSource.endOfStream(); } catch (error) {}
            }
          });
        }

        const chunk = bytesView(result.value);
        if (!chunk || chunk.byteLength === 0) return pump();
        pending = concatBytes(pending, chunk);
        return initializeSourceBuffer().then(function () {
          if (!initAppended) return pump();
          return appendAvailableMedia().then(pump);
        });
      });
    }

    video.src = objectUrl;
    video.hidden = false;
    if (typeof video.load === 'function') video.load();
    const playRequest = video.play();

    Promise.all([
      waitForSourceOpen(mediaSource),
      global.fetch(mediaPath, {
        method: 'GET',
        credentials: 'same-origin',
        cache: 'no-store',
        signal: abortController.signal
      }).then(function (response) {
        if (!response || !response.ok) {
          throw new Error('Kontinuierlicher fMP4-Stream konnte nicht geladen werden (' +
            (response ? response.status : 'network') + ').');
        }
        if (!response.body || typeof response.body.getReader !== 'function') {
          throw new Error('Browser stellt für den kontinuierlichen fMP4-Stream keinen Streaming-Reader bereit.');
        }
        return response.body.getReader();
      })
    ]).then(function (results) {
      if (destroyed) return;
      reader = results[1];
      return pump();
    }).catch(fail);

    return Object.freeze({
      playRequest: playRequest,
      destroy: destroy,
      objectUrl: objectUrl
    });
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
    let playbackFailed = false;
    let activeSessionId = '';
    let continuousTransport = null;
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
      if (continuousTransport) {
        continuousTransport.destroy();
        continuousTransport = null;
      }
      try { video.pause(); } catch (error) {}
      try {
        video.removeAttribute('src');
        if (typeof video.load === 'function') video.load();
      } catch (error) {}
    }

    function failPlayback(error) {
      if (destroyed || playbackFailed) return;
      playbackFailed = true;
      if (continuousTransport) {
        continuousTransport.destroy();
        continuousTransport = null;
      }
      stopActive(false);
      setStatus(
        error && error.message
          ? error.message
          : 'Browser konnte den Live-Stream nicht wiedergeben.',
        true
      );
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

        continuousTransport = createContinuousFmp4Mse(video, mediaPath, function (error) {
          failPlayback(error);
        });

        let playRequest = null;
        if (continuousTransport) {
          playRequest = continuousTransport.playRequest;
          setStatus('Direktstream verbunden · Browser-MSE wartet auf ersten Frame …', false);
        }
        else {
          video.src = mediaPath;
          video.hidden = false;
          if (typeof video.load === 'function') video.load();
          setStatus('Direktstream verbunden · warte auf ersten decodierbaren Frame …', false);
          playRequest = video.play();
        }

        if (playRequest && typeof playRequest.catch === 'function') {
          playRequest.catch(function (error) {
            if (error && error.name === 'NotSupportedError') {
              failPlayback(error);
              return;
            }
            if (!destroyed && !playbackFailed) {
              setStatus('Direktstream bereit · Wiedergabe über Player starten.', false);
            }
          });
        }
        return id;
      }).catch(function (error) {
        if (!destroyed) failPlayback(error);
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
      if (!destroyed && !playbackFailed) setStatus('Live-TV läuft.', false);
    });
    video.addEventListener('waiting', function () {
      if (!destroyed && !playbackFailed) setStatus('Live-TV wartet auf Daten …', false);
    });
    video.addEventListener('error', function () {
      if (!destroyed && !playbackFailed) {
        const mediaError = video.error;
        const detail = mediaError && mediaError.message ? ': ' + mediaError.message : '';
        failPlayback(new Error('Browser konnte den Live-Stream nicht wiedergeben' + detail));
      }
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
    let continuousTransport = null;
    let sessionCreationPromise = Promise.resolve('');

    function setStatus(message, error) {
      status.textContent = message;
      status.classList.toggle('error', Boolean(error));
    }

    function releaseVideo() {
      if (continuousTransport) {
        continuousTransport.destroy();
        continuousTransport = null;
      }
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

        video.hidden = false;
        startButton.hidden = true;
        continuousTransport = createContinuousFmp4Mse(video, mediaPath, function (error) {
          activateFallback(error);
        });

        let playRequest = null;
        if (continuousTransport) {
          setStatus('Direktstream verbunden · Browser-MSE wartet auf ersten Frame …', false);
          playRequest = continuousTransport.playRequest;
        }
        else {
          video.src = mediaPath;
          if (typeof video.load === 'function') video.load();
          setStatus('Direktstream verbunden · warte auf ersten decodierbaren Frame …', false);
          playRequest = video.play();
        }

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
      if (!destroyed && !fallbackPanel && !firstMediaReported && !continuousTransport) {
        activateFallback(new Error('Schneller Recording-Stream ist vor dem ersten Frame stehen geblieben.'));
      }
    });
    video.addEventListener('ended', function () {
      if (!destroyed && !fallbackPanel) stopActive(false);
    });
    video.addEventListener('error', function () {
      if (!destroyed && !fallbackPanel) {
        const mediaError = video.error;
        const detail = mediaError && mediaError.message ? ': ' + mediaError.message : '';
        activateFallback(new Error('Browser konnte den schnellen Recording-Stream nicht wiedergeben' + detail));
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
      channelId: channelId,
      supportsContinuousFmp4Mse: supportsContinuousFmp4Mse,
      findCompleteInitEnd: findCompleteInitEnd,
      completeMediaPrefix: completeMediaPrefix,
      mimeTypeFromContinuousInit: mimeTypeFromContinuousInit
    })
  });

  global.VdrSuiteRecordingFastPlayback = Object.freeze({
    __test: Object.freeze({
      recordingCapabilities: recordingCapabilities,
      safeRecordingMediaPath: safeRecordingMediaPath,
      publicRecordingMediaPath: publicRecordingMediaPath,
      recordingId: recordingId,
      supportsContinuousFmp4Mse: supportsContinuousFmp4Mse
    })
  });
}(window));
