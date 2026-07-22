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
      if (existing.dataset.failed === 'true') {
        reject(new Error('Frontend-Runtime konnte nicht geladen werden: ' + existing.src));
        return;
      }

      existing.addEventListener('load', resolve, {once: true});
      existing.addEventListener(
        'error',
        () => reject(new Error('Frontend-Runtime konnte nicht geladen werden: ' + src)),
        {once: true}
      );
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

    script.addEventListener(
      'error',
      () => {
        script.dataset.failed = 'true';
        reject(new Error('Frontend-Runtime konnte nicht geladen werden: ' + src));
      },
      {once: true}
    );

    document.head.appendChild(script);
  });
}

function loadVdrSuiteEpgDetailRuntime() {
  return loadVdrSuiteDeferredRuntime(
    'vdr-suite-epg-searchtimer-actions-runtime',
    '/frontend/epg-searchtimer-actions.js',
    () => Boolean(
      window.VdrSuiteEpgSearchTimerActions &&
      window.VdrSuiteEpgMetadataDetail &&
      window.VdrSuiteEpgMetadataDetailHook &&
      window.VdrSuiteEpgDetailDesktopFocus
    )
  );
}

function loadVdrSuiteRecordings2Runtime() {
  const sharedRuntime = loadVdrSuiteDeferredRuntime(
    'vdr-suite-recordings2-shared-runtime',
    '/frontend/recordings2-shared.js',
    () => Boolean(window.VdrSuiteRecordings2Shared)
  );

  const folderArtworkRuntime = sharedRuntime.then(() => loadVdrSuiteDeferredRuntime(
    'vdr-suite-recordings2-folder-artwork-runtime',
    '/frontend/recordings2-folder-artwork.js',
    () => Boolean(window.VdrSuiteRecordings2FolderArtwork)
  ));

  const actionsRuntime = sharedRuntime.then(() => loadVdrSuiteDeferredRuntime(
    'vdr-suite-recordings2-actions-runtime',
    '/frontend/recordings2-actions.js',
    () => Boolean(window.VdrSuiteRecordings2Actions)
  ));

  const browserViewRuntime = Promise.all([
    folderArtworkRuntime,
    actionsRuntime
  ]).then(() => loadVdrSuiteDeferredRuntime(
    'vdr-suite-recordings2-browser-view-runtime',
    '/frontend/recordings2-browser-view.js',
    () => Boolean(window.VdrSuiteRecordings2BrowserView)
  ));

  const recordings2Runtime = browserViewRuntime.then(() => loadVdrSuiteDeferredRuntime(
    'vdr-suite-recordings2-runtime',
    '/frontend/recordings2.js',
    () => Boolean(window.VdrSuiteRecordings2)
  ));

  const metadataDetailRuntime = sharedRuntime
    .then(() => loadVdrSuiteDeferredRuntime(
      'vdr-suite-recordings2-person-search-view-runtime',
      '/frontend/recordings2-person-search-view.js',
      () => Boolean(window.VdrSuiteRecordings2PersonSearchView)
    ))
    .then(() => loadVdrSuiteDeferredRuntime(
      'vdr-suite-recordings2-metadata-view-runtime',
      '/frontend/recordings2-metadata-view.js',
      () => Boolean(window.VdrSuiteRecordings2MetadataView)
    ))
    .then(() => loadVdrSuiteDeferredRuntime(
      'vdr-suite-recordings2-metadata-detail-runtime',
      '/frontend/recordings2-metadata-detail.js',
      () => Boolean(window.VdrSuiteRecordings2MetadataDetail)
    ))
    .catch(error => {
      console.error('VDR-Suite Recordings 2 metadata detail runtime failed', error);
      return null;
    });

  return Promise.all([
    recordings2Runtime,
    metadataDetailRuntime
  ]).then(() => undefined);
}

function startVdrSuiteDeferredFrontendRuntimes() {
  loadVdrSuiteEpgDetailRuntime().catch(error => {
    console.error('VDR-Suite combined EPG detail runtime failed', error);
  });

  loadVdrSuiteRecordings2Runtime().catch(error => {
    console.error('VDR-Suite Recordings 2 runtime failed', error);
  });
}

if (typeof window !== 'undefined') {
  installSearchTimerPreviewCacheWarmup();

  window.VdrSuiteDeferredFrontendRuntimes = Object.freeze({
    start: startVdrSuiteDeferredFrontendRuntimes,
    loadEpgDetail: loadVdrSuiteEpgDetailRuntime,
    loadRecordings2: loadVdrSuiteRecordings2Runtime
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
