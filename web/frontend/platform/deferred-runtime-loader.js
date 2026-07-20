'use strict';

function installSearchTimerPreviewCacheWarmup() {
  if (typeof window === 'undefined' || typeof window.fetch !== 'function') return;
  if (window.__vdrSuiteSearchTimerPreviewFetchWrapped === true) return;

  const originalFetch = window.fetch.bind(window);
  const previewPaths = [
    '/api/vdr/searchtimers/preview',
    '/api/searchtimers/preview'
  ];
  const refreshPaths = [
    '/api/vdr/searchtimers/preview/cache/refresh',
    '/api/searchtimers/preview/cache/refresh'
  ];

  function requestUrl(input) {
    if (typeof input === 'string') return input;
    if (input && typeof input.url === 'string') return input.url;
    return '';
  }

  function requestMethod(input, init) {
    if (init && init.method) return String(init.method).toUpperCase();
    if (input && typeof input.method === 'string') return String(input.method).toUpperCase();
    return 'GET';
  }

  function previewRequest(url, method) {
    if (method !== 'GET') return false;
    return previewPaths.some(path => url.indexOf(path) !== -1) &&
      url.indexOf('/preview/cache/refresh') === -1;
  }

  function backendFromPreviewUrl(url) {
    try {
      const parsed = new URL(url, window.location.href);
      return parsed.searchParams.get('backend') ||
        parsed.searchParams.get('backendId') ||
        'default';
    } catch (error) {
      return 'default';
    }
  }

  function refreshUrl(path, backendId) {
    const params = new URLSearchParams({
      backend: backendId,
      from: '-1',
      timespan: String(14 * 24 * 60 * 60),
      limit: '0',
      channelEventLimit: '96',
      _: String(Date.now())
    });
    return path + '?' + params.toString();
  }

  function refreshCache(backendId) {
    function attempt(index) {
      if (index >= refreshPaths.length) {
        return Promise.reject(new Error('SearchTimer-Preview-Cache konnte nicht aktualisiert werden.'));
      }

      return originalFetch(refreshUrl(refreshPaths[index], backendId), {
        method: 'POST',
        cache: 'no-store',
        credentials: 'same-origin',
        headers: {Accept: 'application/json'}
      }).then(response => {
        if (!response.ok) {
          return attempt(index + 1);
        }
        return response.json().then(result => {
          const ready = result && String(result.status || '') === 'ready';
          const available = result && result.available === true;
          const eventCount = Number(result && result.eventCount || 0);
          if (!ready || !available || eventCount <= 0) {
            throw new Error('SearchTimer-Preview-Cache ist nicht bereit oder leer.');
          }
          return result;
        });
      }).catch(error => {
        if (index + 1 < refreshPaths.length) return attempt(index + 1);
        throw error;
      });
    }

    return attempt(0);
  }

  window.fetch = function (input, init) {
    const url = requestUrl(input);
    const method = requestMethod(input, init);
    if (!previewRequest(url, method)) {
      return originalFetch(input, init);
    }

    return refreshCache(backendFromPreviewUrl(url))
      .then(() => originalFetch(input, init));
  };

  window.__vdrSuiteSearchTimerPreviewFetchWrapped = true;
}

function loadVdrSuiteDeferredRuntime(id, src, readyCheck) {
  if (typeof readyCheck === 'function' && readyCheck()) {
    return Promise.resolve();
  }

  const existing = document.getElementById(id);

  if (existing) {
    return new Promise((resolve, reject) => {
      if (
        existing.dataset.loaded === 'true' ||
        (typeof readyCheck === 'function' && readyCheck())
      ) {
        resolve();
        return;
      }

      existing.addEventListener('load', resolve, {once: true});
      existing.addEventListener('error', reject, {once: true});
    });
  }

  return new Promise((resolve, reject) => {
    const script = document.createElement('script');
    script.id = id;
    script.src = src;
    script.async = false;

    script.addEventListener(
      'load',
      () => {
        script.dataset.loaded = 'true';
        resolve();
      },
      {once: true}
    );

    script.addEventListener('error', reject, {once: true});
    document.head.appendChild(script);
  });
}

function startVdrSuiteDeferredFrontendRuntimes() {
  const epgMetadataRuntime = loadVdrSuiteDeferredRuntime(
    'vdr-suite-epg-metadata-detail-runtime',
    '/frontend/epg-metadata-detail.js',
    () => Boolean(window.VdrSuiteEpgMetadataDetail)
  ).catch(error => {
    console.error('VDR-Suite EPG metadata detail runtime failed', error);
  });

  const epgSearchTimerRuntime = epgMetadataRuntime.then(() =>
    loadVdrSuiteDeferredRuntime(
      'vdr-suite-epg-searchtimer-actions-runtime',
      '/frontend/epg-searchtimer-actions.js',
      () => Boolean(window.VdrSuiteEpgSearchTimerActions)
    )
  ).catch(error => {
    console.error('VDR-Suite EPG SearchTimer runtime failed', error);
  });

  epgSearchTimerRuntime.then(() =>
    loadVdrSuiteDeferredRuntime(
      'vdr-suite-epg-metadata-detail-hook-runtime',
      '/frontend/epg-metadata-detail-hook.js',
      () => Boolean(window.VdrSuiteEpgMetadataDetailHook)
    )
  ).catch(error => {
    console.error('VDR-Suite EPG metadata detail hook failed', error);
  });

  loadVdrSuiteDeferredRuntime(
    'vdr-suite-recording-trash-ux-runtime',
    '/frontend/recording-trash-ux.js',
    () => Boolean(window.VdrSuiteRecordingTrashUx)
  ).catch(error => {
    console.error('VDR-Suite deferred frontend runtime failed', error);
  });
}

if (typeof window !== 'undefined') {
  installSearchTimerPreviewCacheWarmup();

  window.VdrSuiteDeferredFrontendRuntimes = Object.freeze({
    start: startVdrSuiteDeferredFrontendRuntimes
  });

  if (document.readyState === 'loading') {
    document.addEventListener(
      'DOMContentLoaded',
      startVdrSuiteDeferredFrontendRuntimes,
      {once: true}
    );
  } else {
    startVdrSuiteDeferredFrontendRuntimes();
  }
}
