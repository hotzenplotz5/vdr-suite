from .common import replace_once

# ---------------------------------------------------------------------------
# Frontend: a pending response is never a permanent cache hit.
# ---------------------------------------------------------------------------

replace_once(
    "web/frontend/epg-metadata-detail.js",
    '  const requestCache = new Map();\n',
    '  const requestCache = new Map();\n'
    '  const pendingRetryDelaysMs = Object.freeze([250, 500, 750, 1000, 1500, 2000]);\n'
)

old_fetch = r'''  function fetchMetadata(backendId, channelId, nativeEventId) {
    const key = backendId + '\n' + channelId + '\n' + nativeEventId;
    if (requestCache.has(key)) return requestCache.get(key);

    const client = global.VdrSuiteClientApi;
    if (!client || typeof client.requestJson !== 'function') {
      return Promise.reject(new Error('EPG-Metadaten-Client ist nicht verfügbar.'));
    }

    const request = client.requestJson('/api/epg/cache/metadata', {
      query: {backend: backendId, channelId: channelId, eventId: nativeEventId},
      cache: 'no-store',
      credentials: 'same-origin'
    }).catch(function (error) {
      requestCache.delete(key);
      throw error;
    });

    requestCache.set(key, request);
    return request;
  }
'''

new_fetch = r'''  function fetchMetadata(backendId, channelId, nativeEventId, attempt) {
    const key = backendId + '\n' + channelId + '\n' + nativeEventId;
    const retryAttempt = Number.isInteger(attempt) && attempt >= 0 ? attempt : 0;
    if (requestCache.has(key)) return requestCache.get(key);

    const client = global.VdrSuiteClientApi;
    if (!client || typeof client.requestJson !== 'function') {
      return Promise.reject(new Error('EPG-Metadaten-Client ist nicht verfügbar.'));
    }

    const request = client.requestJson('/api/epg/cache/metadata', {
      query: {backend: backendId, channelId: channelId, eventId: nativeEventId},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function (metadata) {
      requestCache.delete(key);
      if (metadata && metadata.available === true) {
        const ready = Promise.resolve(metadata);
        requestCache.set(key, ready);
        return metadata;
      }
      if (metadata && metadata.status === 'pending' &&
          retryAttempt < pendingRetryDelaysMs.length &&
          typeof global.setTimeout === 'function') {
        return new Promise(function (resolve) {
          global.setTimeout(resolve, pendingRetryDelaysMs[retryAttempt]);
        }).then(function () {
          return fetchMetadata(
            backendId,
            channelId,
            nativeEventId,
            retryAttempt + 1
          );
        });
      }
      return metadata;
    }).catch(function (error) {
      requestCache.delete(key);
      throw error;
    });

    requestCache.set(key, request);
    return request;
  }
'''

replace_once(
    "web/frontend/epg-metadata-detail.js",
    old_fetch,
    new_fetch
)

replace_once(
    "web/frontend/epg-metadata-detail.js",
    '''      if (!metadata || metadata.available !== true) {
        status.textContent = 'Für diese Sendung sind keine erweiterten TVScraper-Daten verfügbar.';
        return;
      }
''',
    '''      if (!metadata || metadata.available !== true) {
        if (metadata && metadata.status === 'pending') {
          status.textContent = 'TVScraper-Metadaten sind noch nicht materialisiert. Bitte die Sendung erneut öffnen.';
        } else if (metadata && metadata.status === 'stale-event') {
          status.textContent = 'Diese EPG-Version wurde inzwischen durch eine aktuelle Sendungs-ID ersetzt.';
        } else {
          status.textContent = 'Für diese Sendung sind keine erweiterten TVScraper-Daten verfügbar.';
        }
        return;
      }
'''
)

# ---------------------------------------------------------------------------
# Daemon warmup: explicit authoritative epoch window.
# ---------------------------------------------------------------------------

replace_once(
    "core/daemon/src/DaemonRuntimeEpgCache.cpp",
    '''    VdrEventQuery query;
    query.from = -1;
    query.timespan = static_cast<int>(GenreWindowSeconds);
    query.channelEventLimit = 160;
''',
    '''    constexpr std::int64_t PastOverlapSeconds = 3 * 60 * 60;
    const std::int64_t currentEpoch = epgGenreEpochSeconds();

    VdrEventQuery query;
    query.from = currentEpoch - PastOverlapSeconds;
    query.timespan = static_cast<int>(
        GenreWindowSeconds + PastOverlapSeconds);
    query.channelEventLimit = 160;
'''
)
