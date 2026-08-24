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
  const RECORDING_INDEX_STATUS_POLL_MS = 750;
  const RECORDING_INDEX_STATUS_MAX_FAILURES = 5;

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

  function createContinuousFmp4Mse(video, mediaPath, onFailure, autoPlay) {
    if (!supportsContinuousFmp4Mse()) return null;

    const shouldAutoPlay = autoPlay !== false;
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
    const playRequest = shouldAutoPlay ? video.play() : null;

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

  function seekRecordingSession(backendId, sessionId, positionSeconds) {
    const api = global.VdrSuiteClientApi;
    const id = safeSessionId(sessionId);
    const position = Number(positionSeconds);
    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('Client API für Aufnahme-Wiedergabe ist nicht verfügbar.'));
    }
    if (!id || !Number.isFinite(position) || position < 0 || Math.floor(position) !== position) {
      return Promise.reject(new Error('Ungültige Recording-Seek-Anforderung.'));
    }

    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify({
        backendId: text(backendId || 'default'),
        sessionId: id,
        operation: 'seek',
        positionSeconds: position
      }),
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function recordingPlaybackStatus(backendId, sessionId) {
    const api = global.VdrSuiteClientApi;
    const id = safeSessionId(sessionId);
    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('Client API für Aufnahme-Wiedergabe ist nicht verfügbar.'));
    }
    if (!id) {
      return Promise.reject(new Error('Ungültige Recording-MediaSession-ID.'));
    }
    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify({
        backendId: text(backendId || 'default'),
        sessionId: id,
        operation: 'playback-status'
      }),
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

    const controls = global.document.createElement('div');
    controls.className = 'recordings2-playback-controls';
    controls.hidden = true;
    controls.style.display = 'grid';
    controls.style.gap = '0.65rem';
    controls.style.marginTop = '0.75rem';

    const transportRow = global.document.createElement('div');
    transportRow.className = 'recordings2-playback-transport';
    transportRow.style.display = 'flex';
    transportRow.style.flexWrap = 'wrap';
    transportRow.style.gap = '0.5rem';

    function transportButton(label, titleText) {
      const button = global.document.createElement('button');
      button.type = 'button';
      button.textContent = label;
      button.title = titleText;
      button.setAttribute('aria-label', titleText);
      return button;
    }

    const back60Button = transportButton('−60', '60 Sekunden zurück');
    const back10Button = transportButton('−10', '10 Sekunden zurück');
    const playPauseButton = transportButton('Pause', 'Wiedergabe pausieren oder fortsetzen');
    const forward10Button = transportButton('+10', '10 Sekunden vor');
    const forward60Button = transportButton('+60', '60 Sekunden vor');
    const stopButton = transportButton('Stop', 'Wiedergabe stoppen');
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
    directTime.setAttribute('aria-label', 'Direkte Wiedergabezeit');
    const directButton = transportButton('Springen', 'Zur eingegebenen Wiedergabezeit springen');
    directRow.appendChild(directTime);
    directRow.appendChild(directButton);
    controls.appendChild(directRow);
    panel.appendChild(controls);

    let destroyed = false;
    let started = false;
    let stopIssued = false;
    let stopped = false;
    let activeSessionId = '';
    let activeMediaPath = '';
    let startupStartedAt = 0;
    let firstMediaReported = false;
    let fallbackPanel = null;
    let fallbackActivation = null;
    let continuousTransport = null;
    let sessionCreationPromise = Promise.resolve('');
    let durationSeconds = 0;
    let seekSupported = false;
    let seekPreparing = false;
    let seekStartSeconds = 0;
    let seekEndSeconds = 0;
    let seekBaseSeconds = 0;
    let seekInFlight = false;
    let indexStatusTimer = null;
    let indexStatusInFlight = false;
    let indexStatusFailures = 0;

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
          parts.some(function (part) { return !/^\d+$/.test(part); })) {
        return null;
      }
      const numbers = parts.map(Number);
      const hours = parts.length === 3 ? numbers[0] : 0;
      const minutes = parts.length === 3 ? numbers[1] : numbers[0];
      const seconds = parts.length === 3 ? numbers[2] : numbers[1];
      if (minutes >= 60 || seconds >= 60) return null;
      return hours * 3600 + minutes * 60 + seconds;
    }

    function localPositionSeconds() {
      const value = Number(video.currentTime);
      return Number.isFinite(value) && value > 0 ? value : 0;
    }

    function positionSeconds() {
      if (!started || stopped) return seekBaseSeconds;
      return seekBaseSeconds + localPositionSeconds();
    }

    function seekMaximum() {
      if (!seekSupported || seekEndSeconds <= seekStartSeconds) return 0;
      return Math.max(seekStartSeconds, Math.ceil(seekEndSeconds) - 1);
    }

    function clampSeekTarget(value) {
      const position = Math.floor(Number(value));
      if (!Number.isFinite(position) || !seekSupported) return null;
      const maximum = seekMaximum();
      if (position < seekStartSeconds || position > maximum) return null;
      return position;
    }

    function updatePositionDisplay(previewPosition) {
      const current = previewPosition === undefined
        ? Math.floor(positionSeconds())
        : Math.floor(Number(previewPosition) || 0);
      positionLabel.textContent = formatTime(current) + ' / ' +
        (durationSeconds > 0 ? formatTime(durationSeconds) : '--:--:--');
      if (!timeline.disabled && previewPosition === undefined) {
        timeline.value = String(Math.max(seekStartSeconds, Math.min(seekMaximum(), current)));
      }
    }

    function updateControls() {
      const active = started && !destroyed && !fallbackPanel && !stopped;
      const canSeek = active && seekSupported && !seekInFlight;
      back60Button.disabled = !canSeek;
      back10Button.disabled = !canSeek;
      forward10Button.disabled = !canSeek;
      forward60Button.disabled = !canSeek;
      timeline.disabled = !canSeek;
      directTime.disabled = !canSeek;
      directButton.disabled = !canSeek;
      playPauseButton.disabled = !active || seekInFlight;
      stopButton.disabled = !active;
      playPauseButton.textContent = video.paused ? 'Play' : 'Pause';
      timeline.min = String(seekStartSeconds);
      timeline.max = String(seekMaximum());
      updatePositionDisplay();
    }

    function applyPlaybackContract(mediaSession) {
      const playback = mediaSession && mediaSession.playback;
      const duration = Number(playback && playback.durationSeconds);
      durationSeconds = Number.isFinite(duration) && duration > 0 ? Math.floor(duration) : 0;
      const seek = playback && playback.seek;
      const windowValue = seek && seek.window;
      const start = Number(windowValue && windowValue.startSeconds);
      const end = Number(windowValue && windowValue.endSeconds);
      seekSupported = Boolean(
        seek && seek.supported === true && durationSeconds > 0 &&
        Number.isFinite(start) && Number.isFinite(end) && start >= 0 && end > start
      );
      seekPreparing = Boolean(seek && seek.preparing === true && !seekSupported);
      seekStartSeconds = seekSupported ? Math.floor(start) : 0;
      seekEndSeconds = seekSupported ? Math.min(durationSeconds, Math.floor(end)) : 0;
      if (seekSupported && seekEndSeconds <= seekStartSeconds) {
        seekSupported = false;
        seekStartSeconds = 0;
        seekEndSeconds = 0;
      }
      if (seekSupported) seekPreparing = false;
      const position = Number(playback && playback.positionSeconds);
      seekBaseSeconds = Number.isFinite(position) && position >= 0 ? Math.floor(position) : 0;
      updateControls();
    }

    function setStatus(message, error) {
      status.textContent = message;
      status.classList.toggle('error', Boolean(error));
    }

    function clearIndexStatusPoll() {
      if (indexStatusTimer !== null && typeof global.clearTimeout === 'function') {
        try { global.clearTimeout(indexStatusTimer); } catch (error) {}
      }
      indexStatusTimer = null;
      indexStatusInFlight = false;
    }

    function scheduleIndexStatusPoll() {
      if (!seekPreparing || destroyed || stopped || fallbackPanel || !activeSessionId ||
          typeof global.setTimeout !== 'function' || indexStatusTimer !== null || indexStatusInFlight) {
        return;
      }
      indexStatusTimer = global.setTimeout(function () {
        indexStatusTimer = null;
        pollIndexStatus();
      }, RECORDING_INDEX_STATUS_POLL_MS);
    }

    function pollIndexStatus() {
      if (!seekPreparing || destroyed || stopped || fallbackPanel || !activeSessionId || indexStatusInFlight) {
        return Promise.resolve(false);
      }
      indexStatusInFlight = true;
      return recordingPlaybackStatus(backendId, activeSessionId).then(function (session) {
        indexStatusInFlight = false;
        if (destroyed || stopped || fallbackPanel) return false;
        const mediaSession = session && session.mediaSession;
        const returnedId = safeSessionId(mediaSession && mediaSession.id);
        const returnedPath = publicRecordingMediaPath(mediaSession && mediaSession.mediaPath);
        if (!mediaSession || mediaSession.state !== 'ready' ||
            returnedId !== activeSessionId ||
            mediaSession.presentationProfileId !== 'progressive-fmp4' ||
            !returnedPath || returnedPath !== activeMediaPath) {
          throw new Error('MediaSession hat während der Index-Erstellung ihre Playback-Identität geändert.');
        }

        applyPlaybackContract(mediaSession);
        indexStatusFailures = 0;
        if (seekSupported) {
          clearIndexStatusPoll();
          setStatus('Aufnahme läuft · Index erstellt · Seek bereit.', false);
          updateControls();
          return true;
        }
        if (seekPreparing) {
          setStatus('Aufnahme läuft · Index wird erstellt …', false);
          scheduleIndexStatusPoll();
          return false;
        }

        clearIndexStatusPoll();
        setStatus('Aufnahme läuft · Index konnte nicht für Seek bereitgestellt werden.', true);
        updateControls();
        return false;
      }).catch(function () {
        indexStatusInFlight = false;
        if (destroyed || stopped || fallbackPanel || !seekPreparing) return false;
        indexStatusFailures += 1;
        if (indexStatusFailures >= RECORDING_INDEX_STATUS_MAX_FAILURES) {
          seekPreparing = false;
          clearIndexStatusPoll();
          setStatus('Aufnahme läuft · Indexstatus konnte nicht ermittelt werden.', true);
          updateControls();
          return false;
        }
        setStatus('Aufnahme läuft · Index wird erstellt …', false);
        scheduleIndexStatusPoll();
        return false;
      });
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
        video.currentTime = 0;
      } catch (error) {}
    }

    function stopActive(keepalive) {
      if (stopIssued || !activeSessionId) return Promise.resolve(null);
      stopIssued = true;
      const request = stopRecordingSession(backendId, activeSessionId, keepalive);
      if (!request || typeof request.then !== 'function') return Promise.resolve(request || null);
      if (keepalive) return request.catch(function () { return null; });
      return request;
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

      seekPreparing = false;
      clearIndexStatusPoll();
      stopActive(false).catch(function () {});
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

    function failRepositionedPlayback(error) {
      if (destroyed || stopped || fallbackPanel) return;
      stopped = true;
      seekInFlight = false;
      seekPreparing = false;
      clearIndexStatusPoll();
      stopActive(false).catch(function () {});
      releaseVideo();
      setStatus(
        error && error.message
          ? 'Seek wurde serverseitig ausgeführt, aber der neue Stream konnte nicht wiedergegeben werden: ' + error.message
          : 'Seek wurde serverseitig ausgeführt, aber der neue Stream konnte nicht wiedergegeben werden.',
        true
      );
      updateControls();
    }

    function connectRecordingStream(autoPlay, initialConnection) {
      const shouldPlay = autoPlay !== false;
      continuousTransport = createContinuousFmp4Mse(
        video,
        activeMediaPath,
        function (error) {
          if (initialConnection) activateFallback(error);
          else failRepositionedPlayback(error);
        },
        shouldPlay
      );

      if (continuousTransport) return continuousTransport.playRequest;

      video.src = activeMediaPath;
      video.hidden = false;
      if (typeof video.load === 'function') video.load();
      return shouldPlay ? video.play() : null;
    }

    function pageHide() {
      if (destroyed) return;
      seekPreparing = false;
      clearIndexStatusPoll();
      if (fallbackPanel && typeof fallbackPanel.destroy === 'function') {
        fallbackPanel.destroy();
        return;
      }
      stopActive(true);
    }

    function startPlayback() {
      if ((started && !stopped) || destroyed) return sessionCreationPromise;
      started = true;
      stopped = false;
      stopIssued = false;
      firstMediaReported = false;
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
        activeMediaPath = publicRecordingMediaPath(mediaSession.mediaPath);
        if (!activeMediaPath || mediaSession.presentationProfileId !== 'progressive-fmp4') {
          throw new Error('Schnelle Recording-MediaSession wurde nicht bereitgestellt.');
        }

        applyPlaybackContract(mediaSession);
        video.hidden = false;
        startButton.hidden = true;
        controls.hidden = false;
        updateControls();
        const playRequest = connectRecordingStream(true, true);

        if (seekPreparing) {
          setStatus('Direktstream verbunden · Index wird im Hintergrund erstellt …', false);
          scheduleIndexStatusPoll();
        }
        else if (continuousTransport) {
          setStatus('Direktstream verbunden · Browser-MSE wartet auf ersten Frame …', false);
        }
        else {
          setStatus('Direktstream verbunden · warte auf ersten decodierbaren Frame …', false);
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

    function playPlayback() {
      if (!started) return startPlayback();
      if (destroyed || stopped || fallbackPanel || seekInFlight) return Promise.resolve(false);
      const request = video.play();
      updateControls();
      return request && typeof request.then === 'function'
        ? request.then(function () { return true; })
        : Promise.resolve(true);
    }

    function pausePlayback() {
      if (!started || destroyed || stopped || fallbackPanel || seekInFlight) return false;
      try { video.pause(); } catch (error) { return false; }
      setStatus(seekPreparing ? 'Aufnahme pausiert · Index wird erstellt …' : 'Aufnahme pausiert.', false);
      updateControls();
      return true;
    }

    function stopPlayback() {
      if (!started || destroyed || fallbackPanel || stopped) return Promise.resolve(false);
      stopped = true;
      seekInFlight = false;
      seekPreparing = false;
      clearIndexStatusPoll();
      const stopRequest = stopActive(false);
      releaseVideo();
      startButton.textContent = '▶ Wiedergabe erneut starten';
      startButton.hidden = false;
      startButton.disabled = true;
      controls.hidden = true;
      video.hidden = true;
      setStatus('Wiedergabe wird gestoppt …', false);
      updateControls();
      return Promise.resolve(stopRequest).then(function () {
        activeSessionId = '';
        activeMediaPath = '';
        startButton.disabled = false;
        setStatus('Wiedergabe gestoppt · erneut starten möglich.', false);
        return true;
      }).catch(function (error) {
        startButton.disabled = true;
        setStatus(
          error && error.message
            ? 'Wiedergabe lokal gestoppt · Server-Cleanup fehlgeschlagen: ' + error.message
            : 'Wiedergabe lokal gestoppt · Server-Cleanup fehlgeschlagen.',
          true
        );
        throw error;
      });
    }

    function seekAbsolute(position) {
      const target = clampSeekTarget(position);
      if (target === null || destroyed || stopped || fallbackPanel || !activeSessionId) {
        return Promise.reject(new Error('Seek ist für diese Wiedergabe oder Position nicht verfügbar.'));
      }
      if (seekInFlight) {
        return Promise.reject(new Error('Ein Seek läuft bereits.'));
      }

      const shouldResume = !video.paused;
      seekInFlight = true;
      updateControls();
      setStatus('Springe zu ' + formatTime(target) + ' …', false);

      return seekRecordingSession(backendId, activeSessionId, target).then(function (session) {
        if (destroyed || stopped) return false;
        const mediaSession = session && session.mediaSession;
        const returnedId = safeSessionId(mediaSession && mediaSession.id);
        const returnedPath = publicRecordingMediaPath(mediaSession && mediaSession.mediaPath);
        if (!mediaSession || mediaSession.state !== 'ready' ||
            returnedId !== activeSessionId ||
            mediaSession.presentationProfileId !== 'progressive-fmp4' ||
            !returnedPath || returnedPath !== activeMediaPath) {
          throw new Error('MediaSession hat nach dem Seek ihre Playback-Identität geändert.');
        }

        releaseVideo();
        applyPlaybackContract(mediaSession);
        if (!seekSupported || seekBaseSeconds !== target) {
          throw new Error('MediaSession hat den angeforderten Seek nicht bestätigt.');
        }

        const playRequest = connectRecordingStream(shouldResume, false);
        seekInFlight = false;
        setStatus(
          shouldResume
            ? 'Seek abgeschlossen · Wiedergabe wird fortgesetzt.'
            : 'Seek abgeschlossen · Wiedergabe bleibt pausiert.',
          false
        );
        updateControls();
        if (playRequest && typeof playRequest.catch === 'function') {
          playRequest.catch(function () {
            setStatus('Seek abgeschlossen · Wiedergabe über Play fortsetzen.', false);
            updateControls();
          });
        }
        return true;
      }).catch(function (error) {
        seekInFlight = false;
        setStatus(
          error && error.message ? 'Seek fehlgeschlagen: ' + error.message : 'Seek fehlgeschlagen.',
          true
        );
        updateControls();
        throw error;
      });
    }

    function seekRelative(deltaSeconds) {
      const delta = Number(deltaSeconds);
      if (!Number.isFinite(delta)) {
        return Promise.reject(new Error('Ungültiger relativer Zeitsprung.'));
      }
      const maximum = seekMaximum();
      const target = Math.max(
        seekStartSeconds,
        Math.min(maximum, Math.floor(positionSeconds() + delta))
      );
      return seekAbsolute(target);
    }

    function playbackState() {
      if (destroyed) return 'destroyed';
      if (fallbackPanel) return 'fallback';
      if (stopped) return 'stopped';
      if (seekInFlight) return 'seeking';
      if (!started) return 'idle';
      return video.paused ? 'paused' : 'playing';
    }

    function destroy() {
      if (destroyed) return;
      destroyed = true;
      seekPreparing = false;
      clearIndexStatusPoll();
      if (typeof global.removeEventListener === 'function') {
        global.removeEventListener('pagehide', pageHide);
      }
      if (fallbackPanel && typeof fallbackPanel.destroy === 'function') {
        fallbackPanel.destroy();
      }
      else {
        stopActive(false).catch(function () {});
        releaseVideo();
      }
    }

    video.addEventListener('playing', function () {
      if (destroyed || fallbackPanel || stopped) return;
      if (!firstMediaReported) {
        firstMediaReported = true;
        const elapsed = Math.max(0, nowMilliseconds() - startupStartedAt);
        setStatus(
          seekPreparing
            ? 'Aufnahme läuft · Start ' + (elapsed / 1000).toFixed(2) + ' s · Index wird erstellt …'
            : 'Aufnahme läuft · Start ' + (elapsed / 1000).toFixed(2) + ' s',
          false
        );
        if (global.console && typeof global.console.info === 'function') {
          global.console.info(
            'recording playback first-media',
            {sessionId: activeSessionId, profile: 'progressive-fmp4', startupMs: Math.round(elapsed)}
          );
        }
      }
      else {
        setStatus(seekPreparing ? 'Aufnahme läuft · Index wird erstellt …' : 'Aufnahme läuft.', false);
      }
      updateControls();
    });
    video.addEventListener('play', updateControls);
    video.addEventListener('pause', function () {
      if (!destroyed && !fallbackPanel && !stopped && !seekInFlight) {
        updateControls();
      }
    });
    video.addEventListener('timeupdate', function () {
      if (!destroyed && !fallbackPanel && !stopped) updatePositionDisplay();
    });
    video.addEventListener('waiting', function () {
      if (!destroyed && !fallbackPanel && !stopped && firstMediaReported) {
        setStatus(seekPreparing ? 'Aufnahme wartet auf Daten · Index wird erstellt …' : 'Aufnahme wartet auf Daten …', false);
      }
    });
    video.addEventListener('stalled', function () {
      if (!destroyed && !fallbackPanel && !stopped && !firstMediaReported && !continuousTransport) {
        activateFallback(new Error('Schneller Recording-Stream ist vor dem ersten Frame stehen geblieben.'));
      }
    });
    video.addEventListener('ended', function () {
      if (!destroyed && !fallbackPanel && !stopped) {
        stopped = true;
        seekPreparing = false;
        clearIndexStatusPoll();
        stopActive(false).catch(function () {});
        updateControls();
      }
    });
    video.addEventListener('error', function () {
      if (!destroyed && !fallbackPanel && !stopped) {
        const mediaError = video.error;
        const detail = mediaError && mediaError.message ? ': ' + mediaError.message : '';
        if (firstMediaReported || seekBaseSeconds > 0) {
          failRepositionedPlayback(new Error('Browser konnte den Recording-Stream nicht wiedergeben' + detail));
        }
        else {
          activateFallback(new Error('Browser konnte den schnellen Recording-Stream nicht wiedergeben' + detail));
        }
      }
    });

    startButton.addEventListener('click', startPlayback);
    playPauseButton.addEventListener('click', function () {
      if (video.paused) {
        playPlayback().catch(function () {});
      }
      else {
        pausePlayback();
      }
    });
    stopButton.addEventListener('click', function () {
      stopPlayback().catch(function () {});
    });
    back60Button.addEventListener('click', function () { seekRelative(-60).catch(function () {}); });
    back10Button.addEventListener('click', function () { seekRelative(-10).catch(function () {}); });
    forward10Button.addEventListener('click', function () { seekRelative(10).catch(function () {}); });
    forward60Button.addEventListener('click', function () { seekRelative(60).catch(function () {}); });
    timeline.addEventListener('input', function () {
      if (!timeline.disabled) updatePositionDisplay(Number(timeline.value));
    });
    timeline.addEventListener('change', function () {
      if (timeline.disabled) return;
      seekAbsolute(Number(timeline.value)).catch(function () { updatePositionDisplay(); });
    });
    directButton.addEventListener('click', function () {
      const target = parseTime(directTime.value);
      if (target === null) {
        setStatus('Ungültige Zeit. Erwartet werden Sekunden, MM:SS oder HH:MM:SS.', true);
        return;
      }
      seekAbsolute(target).then(function () {
        directTime.value = formatTime(target);
      }).catch(function () {});
    });
    directTime.addEventListener('keydown', function (event) {
      if (event && event.key === 'Enter' && typeof directButton.click === 'function') {
        directButton.click();
      }
    });
    if (typeof global.addEventListener === 'function') {
      global.addEventListener('pagehide', pageHide);
    }

    updateControls();

    return Object.freeze({
      element: panel,
      destroy: destroy,
      start: startPlayback,
      play: playPlayback,
      pause: pausePlayback,
      stop: stopPlayback,
      position: function () { return Math.floor(positionSeconds()); },
      duration: function () { return durationSeconds > 0 ? durationSeconds : null; },
      state: playbackState,
      seekAbsolute: seekAbsolute,
      seekRelative: seekRelative,
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
      recordingPlaybackStatus: recordingPlaybackStatus,
      supportsContinuousFmp4Mse: supportsContinuousFmp4Mse
    })
  });
}(window));
