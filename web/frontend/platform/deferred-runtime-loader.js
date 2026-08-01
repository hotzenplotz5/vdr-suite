'use strict';

function installSearchTimerPreviewCacheWarmup() {
  if (typeof window === 'undefined' || typeof window.fetch !== 'function') return;
  if (window.__vdrSuiteSearchTimerPreviewFetchWrapped === true) return;

  const originalFetch = window.fetch.bind(window);
  const previewPaths = [
    '/api/vdr/searchtimers/preview',
    '/api/searchtimers/preview'
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

  function refreshCache(backendId) {
    const client = window.VdrSuiteClientApi;
    if (!client ||
        typeof client.fetchClientSearchTimerPreviewCacheRefresh !== 'function') {
      return Promise.reject(new Error(
        'SearchTimer-Preview-Cache konnte nicht aktualisiert werden: Client API fehlt.'
      ));
    }

    return client.fetchClientSearchTimerPreviewCacheRefresh({
      backendId: backendId,
      query: {
        from: -1,
        timespan: 14 * 24 * 60 * 60,
        limit: 0,
        channelEventLimit: 96,
        _: Date.now()
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(result => {
      const ready = result && String(result.status || '') === 'ready';
      const available = result && result.available === true;
      const eventCount = Number(result && result.eventCount || 0);
      if (!ready || !available || eventCount <= 0) {
        throw new Error('SearchTimer-Preview-Cache ist nicht bereit oder leer.');
      }
      return result;
    });
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

function installSecurityRoleErrorMessages() {
  if (typeof window === 'undefined' || typeof window.fetch !== 'function') return;
  if (window.__vdrSuiteSecurityRoleErrorMessagesWrapped === true) return;

  const originalFetch = window.fetch.bind(window);

  function readOnlyMessage() {
    const language = document && document.documentElement
      ? String(document.documentElement.lang || '').toLowerCase()
      : '';
    return language.startsWith('en')
      ? 'This account has read-only access to this backend.'
      : 'Dieses Konto hat für dieses Backend nur Lesezugriff.';
  }

  window.fetch = function (input, init) {
    return originalFetch(input, init).then(response => {
      if (!response || response.status !== 403 ||
          typeof response.clone !== 'function' ||
          typeof window.Response !== 'function') {
        return response;
      }

      let inspectionResponse;
      try {
        inspectionResponse = response.clone();
      } catch (error) {
        return response;
      }

      return inspectionResponse.text().then(text => {
        let payload;
        try {
          payload = JSON.parse(text);
        } catch (error) {
          return response;
        }

        if (!payload || !payload.error || typeof payload.error !== 'object' ||
            String(payload.error.code || '') !== 'role_read_only') {
          return response;
        }

        payload.error.message = readOnlyMessage();
        const headers = typeof window.Headers === 'function'
          ? new window.Headers(response.headers)
          : response.headers;
        return new window.Response(JSON.stringify(payload), {
          status: response.status,
          statusText: response.statusText,
          headers: headers
        });
      }).catch(() => response);
    });
  };

  window.__vdrSuiteSecurityRoleErrorMessagesWrapped = true;
}

function installMutationCsrfForPaths(wrapperMarker, mutationPaths) {
  if (typeof window === 'undefined' || typeof window.fetch !== 'function') return;
  if (window[wrapperMarker] === true) return;

  const originalFetch = window.fetch.bind(window);
  const protectedMutationPaths = Object.freeze(mutationPaths.slice());

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

  function requestPath(url) {
    try {
      return new URL(url, window.location.href).pathname;
    } catch (error) {
      const query = url.indexOf('?');
      return query === -1 ? url : url.slice(0, query);
    }
  }

  function isProtectedMutation(input, init) {
    return requestMethod(input, init) === 'POST' &&
      protectedMutationPaths.includes(requestPath(requestUrl(input)));
  }

  function csrfHeaders() {
    const session = window.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function mergedInit(input, init) {
    const securityHeaders = csrfHeaders();
    if (!Object.prototype.hasOwnProperty.call(securityHeaders, 'X-CSRF-Token')) {
      return init;
    }

    const next = Object.assign({}, init || {});
    const requestHeaders = input && typeof input !== 'string' && input.headers
      ? input.headers
      : {};

    if (typeof window.Headers === 'function') {
      const headers = new window.Headers(requestHeaders);
      new window.Headers(next.headers || {}).forEach((value, name) => {
        headers.set(name, value);
      });
      Object.keys(securityHeaders).forEach(name => {
        headers.set(name, securityHeaders[name]);
      });
      next.headers = headers;
      return next;
    }

    next.headers = Object.assign(
      {},
      requestHeaders,
      next.headers || {},
      securityHeaders
    );
    return next;
  }

  window.fetch = function (input, init) {
    return isProtectedMutation(input, init)
      ? originalFetch(input, mergedInit(input, init))
      : originalFetch(input, init);
  };

  window[wrapperMarker] = true;
}

function installTimerMutationCsrf() {
  installMutationCsrfForPaths(
    '__vdrSuiteTimerMutationCsrfWrapped',
    [
      '/api/vdr/timers/actions/create',
      '/api/vdr/timers/actions/update',
      '/api/vdr/timers/actions/delete'
    ]
  );
}

function installChannelMoveMutationCsrf() {
  installMutationCsrfForPaths(
    '__vdrSuiteChannelMoveMutationCsrfWrapped',
    [
      '/api/vdr/channels/move',
      '/api/vdr/channels/actions/move'
    ]
  );
}

function installRecordingExecutionMutationCsrf() {
  installMutationCsrfForPaths(
    '__vdrSuiteRecordingExecutionMutationCsrfWrapped',
    [
      '/api/recordings/actions/execute',
      '/api/vdr/recordings/actions/execute'
    ]
  );
}

function installSearchTimerCreateMutationCsrf() {
  installMutationCsrfForPaths(
    '__vdrSuiteSearchTimerCreateMutationCsrfWrapped',
    [
      '/api/searchtimers',
      '/api/vdr/searchtimers'
    ]
  );
}

function installSearchTimerMaintenanceMutationCsrf() {
  installMutationCsrfForPaths(
    '__vdrSuiteSearchTimerMaintenanceMutationCsrfWrapped',
    [
      '/api/searchtimers/update',
      '/api/vdr/searchtimers/update',
      '/api/searchtimers/delete',
      '/api/vdr/searchtimers/delete'
    ]
  );
}

function installSearchTimerExecutionMutationCsrf() {
  installMutationCsrfForPaths(
    '__vdrSuiteSearchTimerExecutionMutationCsrfWrapped',
    [
      '/api/searchtimers/execute',
      '/api/vdr/searchtimers/execute',
      '/api/searchtimers/real-test',
      '/api/vdr/searchtimers/real-test'
    ]
  );
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

function loadVdrSuiteGenresRuntime() {
  return Promise.all([
    loadVdrSuiteEpgDetailRuntime(),
    loadVdrSuiteRecordings2Runtime()
  ])
    .then(() => loadVdrSuiteDeferredRuntime(
      'vdr-suite-epg-detail-owner-runtime',
      '/frontend/epg-detail-owner.js',
      () => Boolean(window.VdrSuiteEpgDetailOwner)
    ))
    .then(() => loadVdrSuiteDeferredRuntime(
      'vdr-suite-genres-runtime',
      '/frontend/modules/genres.js',
      () => Boolean(window.VdrSuiteGenres)
    ));
}

function startVdrSuiteDeferredFrontendRuntimes() {
  loadVdrSuiteEpgDetailRuntime().catch(error => {
    console.error('VDR-Suite combined EPG detail runtime failed', error);
  });

  loadVdrSuiteRecordings2Runtime().catch(error => {
    console.error('VDR-Suite Recordings 2 runtime failed', error);
  });

  loadVdrSuiteGenresRuntime().catch(error => {
    console.error('VDR-Suite genres runtime failed', error);
  });
}

if (typeof window !== 'undefined') {
  installSecurityRoleErrorMessages();
  installTimerMutationCsrf();
  installChannelMoveMutationCsrf();
  installRecordingExecutionMutationCsrf();
  installSearchTimerCreateMutationCsrf();
  installSearchTimerMaintenanceMutationCsrf();
  installSearchTimerExecutionMutationCsrf();
  installSearchTimerPreviewCacheWarmup();

  window.VdrSuiteDeferredFrontendRuntimes = Object.freeze({
    start: startVdrSuiteDeferredFrontendRuntimes,
    loadEpgDetail: loadVdrSuiteEpgDetailRuntime,
    loadRecordings2: loadVdrSuiteRecordings2Runtime,
    loadGenres: loadVdrSuiteGenresRuntime
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
