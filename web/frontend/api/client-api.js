(function () {
  'use strict';

  const DEFAULT_HEADERS = Object.freeze({
    Accept: 'application/json'
  });

  function normalizeOptions(options) {
    return options && typeof options === 'object' ? options : {};
  }

  function copyQuery(source) {
    const query = {};

    if (!source) {
      return query;
    }

    Object.keys(source).forEach(function (key) {
      const value = source[key];

      if (value === undefined || value === null || value === '') {
        return;
      }

      query[key] = value;
    });

    return query;
  }

  function copyHeaders(source) {
    const headers = {};

    if (!source) {
      return headers;
    }

    if (typeof Headers === 'function' && source instanceof Headers) {
      source.forEach(function (value, name) {
        headers[name] = value;
      });
      return headers;
    }

    Object.keys(source).forEach(function (name) {
      headers[name] = source[name];
    });
    return headers;
  }

  function activeSessionCsrfHeaders() {
    const session = window.VdrSuiteBrowserSession;

    if (!session || typeof session.csrfHeaders !== 'function') {
      return {};
    }

    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object'
      ? copyHeaders(headers)
      : {};
  }

  function queryMutationOptions(options) {
    const normalized = normalizeOptions(options);

    return Object.assign({}, normalized, {
      method: normalized.method || 'POST',
      headers: Object.assign(
        {},
        copyHeaders(normalized.headers),
        activeSessionCsrfHeaders()
      )
    });
  }

  function buildQueryString(query) {
    const params = new URLSearchParams();

    Object.keys(query || {}).forEach(function (key) {
      const value = query[key];

      if (Array.isArray(value)) {
        value.forEach(function (entry) {
          if (entry !== undefined && entry !== null && entry !== '') {
            params.append(key, String(entry));
          }
        });
        return;
      }

      if (value !== undefined && value !== null && value !== '') {
        params.set(key, String(value));
      }
    });

    const encoded = params.toString();

    if (!encoded) {
      return '';
    }

    return '?' + encoded;
  }

  function queryOptions(options) {
    const normalized = normalizeOptions(options);
    const query = copyQuery(normalized.query);

    if (normalized.backendId) {
      query.backendId = normalized.backendId;
    }

    if (normalized.channelId) {
      query.channelId = normalized.channelId;
    }

    if (normalized.start) {
      query.start = normalized.start;
    }

    if (normalized.end) {
      query.end = normalized.end;
    }

    return query;
  }

  function backendQueryOptions(options) {
    const normalized = normalizeOptions(options);
    const query = copyQuery(normalized.query);

    if (normalized.backendId && !query.backend) {
      query.backend = normalized.backendId;
    }

    return Object.assign({}, normalized, {
      backendId: undefined,
      query: query
    });
  }

  function requestOptions(options) {
    const normalized = normalizeOptions(options);

    return {
      method: normalized.method || 'GET',
      headers: Object.assign({}, DEFAULT_HEADERS, normalized.headers || {}),
      body: normalized.body,
      cache: normalized.cache,
      credentials: normalized.credentials,
      signal: normalized.signal
    };
  }

  function jsonPostOptions(options) {
    const normalized = normalizeOptions(options);
    const bodySource = normalized.body !== undefined
      ? normalized.body
      : normalized.payload;

    return Object.assign({}, normalized, {
      method: normalized.method || 'POST',
      headers: Object.assign(
        { 'Content-Type': 'application/json' },
        normalized.headers || {}
      ),
      body: bodySource && typeof bodySource === 'object'
        ? JSON.stringify(bodySource)
        : bodySource
    });
  }

  function errorMessage(path, status, payload) {
    if (payload && typeof payload === 'object') {
      if (payload.error) {
        if (typeof payload.error === 'object') {
          if (payload.error.message) {
            return String(payload.error.message);
          }

          if (payload.error.code) {
            return String(payload.error.code);
          }
        } else {
          return String(payload.error);
        }
      }

      if (payload.message) {
        return String(payload.message);
      }

      if (payload.detail) {
        return String(payload.detail);
      }

      if (payload.title) {
        return String(payload.title);
      }

      if (payload.code) {
        return String(payload.code);
      }
    }

    return 'Request failed for ' + path + ' with status ' + status;
  }

  function errorField(payload, name) {
    if (!payload || typeof payload !== 'object') {
      return null;
    }

    if (payload[name] !== undefined && payload[name] !== null &&
        payload[name] !== '') {
      return String(payload[name]);
    }

    if (payload.error && typeof payload.error === 'object' &&
        payload.error[name] !== undefined &&
        payload.error[name] !== null &&
        payload.error[name] !== '') {
      return String(payload.error[name]);
    }

    return null;
  }

  function createClientError(path, status, payload) {
    const failure = new Error(errorMessage(path, status, payload));
    failure.name = 'VdrSuiteClientError';
    failure.path = path;
    failure.status = Number(status) || 0;
    failure.code = errorField(payload, 'code');
    failure.requestId = errorField(payload, 'requestId');
    failure.correlationId = errorField(payload, 'correlationId');
    failure.payload = payload && typeof payload === 'object' ? payload : null;
    return failure;
  }

  function isClientError(error) {
    return Boolean(
      error &&
      error.name === 'VdrSuiteClientError' &&
      typeof error.status === 'number'
    );
  }

  function parseJsonResponse(path, response) {
    return response.text().then(function (text) {
      if (!text) {
        return null;
      }

      try {
        return JSON.parse(text);
      } catch (error) {
        throw new Error('Invalid JSON response from ' + path);
      }
    });
  }

  function requestJson(path, options) {
    const normalized = normalizeOptions(options);
    const query = queryOptions(normalized);
    const url = path + buildQueryString(query);

    const homeCache = window.VdrSuiteHomeCache;
    const snapshot = path.match(/^\/api\/backends\/([A-Za-z0-9_.-]+)\/snapshot$/);
    const stamp = homeCache && (normalized.method || 'GET') === 'GET' &&
      (path !== '/api/metadata/genres' || query.scope === 'recordings')
        ? homeCache.requestStamp(path, snapshot ? snapshot[1] : query.backend || query.backendId)
        : null;
    return fetch(url, requestOptions(normalized)).then(function (response) {
      if (stamp) homeCache.observe(stamp, response, null);
      return parseJsonResponse(path, response).then(function (payload) {
        if (stamp) homeCache.observe(stamp, response, payload);
        if (!response.ok) {
          throw createClientError(path, response.status, payload);
        }

        return payload;
      });
    });
  }

  function requestBinary(path, options) {
    const normalized = normalizeOptions(options);
    const query = queryOptions(normalized);
    const url = path + buildQueryString(query);

    return fetch(url, requestOptions(normalized)).then(function (response) {
      const revision = Number(
        response.headers && typeof response.headers.get === 'function'
          ? response.headers.get('X-Vdr-Suite-Hbbtv-Revision')
          : 0
      ) || 0;
      const width = Number(
        response.headers && typeof response.headers.get === 'function'
          ? response.headers.get('X-Vdr-Suite-Hbbtv-Width')
          : 0
      ) || 0;
      const height = Number(
        response.headers && typeof response.headers.get === 'function'
          ? response.headers.get('X-Vdr-Suite-Hbbtv-Height')
          : 0
      ) || 0;
      const frameKind = response.headers &&
        typeof response.headers.get === 'function'
          ? String(response.headers.get('X-Vdr-Suite-Hbbtv-Frame') || '')
              .toLowerCase()
          : '';
      const baseRevision = Number(
        response.headers && typeof response.headers.get === 'function'
          ? response.headers.get('X-Vdr-Suite-Hbbtv-Base-Revision')
          : 0
      ) || 0;
      const patchX = Number(
        response.headers && typeof response.headers.get === 'function'
          ? response.headers.get('X-Vdr-Suite-Hbbtv-Patch-X')
          : 0
      ) || 0;
      const patchY = Number(
        response.headers && typeof response.headers.get === 'function'
          ? response.headers.get('X-Vdr-Suite-Hbbtv-Patch-Y')
          : 0
      ) || 0;
      const patchWidth = Number(
        response.headers && typeof response.headers.get === 'function'
          ? response.headers.get('X-Vdr-Suite-Hbbtv-Patch-Width')
          : 0
      ) || 0;
      const patchHeight = Number(
        response.headers && typeof response.headers.get === 'function'
          ? response.headers.get('X-Vdr-Suite-Hbbtv-Patch-Height')
          : 0
      ) || 0;

      if (response.status === 204) {
        return {
          status: 204,
          revision: revision,
          width: width,
          height: height,
          frameKind: frameKind,
          baseRevision: baseRevision,
          patchX: patchX,
          patchY: patchY,
          patchWidth: patchWidth,
          patchHeight: patchHeight,
          bytes: new Uint8Array(0)
        };
      }

      if (!response.ok) {
        return response.text().then(function (body) {
          let payload = null;
          if (body) {
            try { payload = JSON.parse(body); } catch (error) { payload = null; }
          }
          throw createClientError(path, response.status, payload);
        });
      }

      const contentType = response.headers &&
        typeof response.headers.get === 'function'
        ? String(response.headers.get('Content-Type') || '').toLowerCase()
        : '';
      if (contentType.indexOf('image/qoi') !== 0) {
        throw new Error('Invalid binary response type from ' + path);
      }

      return response.arrayBuffer().then(function (buffer) {
        if (!buffer || buffer.byteLength === 0 ||
            buffer.byteLength > 16 * 1024 * 1024) {
          throw new Error('Invalid binary response size from ' + path);
        }

        return {
          status: response.status,
          revision: revision,
          width: width,
          height: height,
          frameKind: frameKind,
          baseRevision: baseRevision,
          patchX: patchX,
          patchY: patchY,
          patchWidth: patchWidth,
          patchHeight: patchHeight,
          bytes: new Uint8Array(buffer)
        };
      });
    });
  }

  function fetchClientTimers(options) {
    return requestJson('/api/vdr/timers/live', options);
  }

  function fetchClientTimerConflicts(options) {
    return requestJson('/api/vdr/timers/conflicts/live', options);
  }

  // F5.3: reuse the Public-v1 reference client's scoped native Timer reads.
  // The legacy editor needs more fields and different write semantics.
  function publicNativeTimerClient() {
    const factory = window.VdrSuitePublicV1Client;
    if (!factory || typeof factory.createClient !== 'function') {
      throw new Error('Public API v1 Client ist nicht verfügbar.');
    }
    return factory.createClient({
      baseUrl: window.location && window.location.origin ? window.location.origin : '',
      credentials: 'same-origin'
    });
  }

  function fetchPublicNativeTimerPage(options) {
    const input = normalizeOptions(options);
    return publicNativeTimerClient().getNativeTimers({
      query: {backendId: input.backendId, offset: input.offset, limit: input.limit},
      credentials: 'same-origin',
      signal: input.signal
    });
  }

  function fetchPublicNativeTimerDetail(options) {
    const input = normalizeOptions(options);
    return publicNativeTimerClient().getNativeTimer({
      backendId: input.backendId,
      timerId: input.timerId,
      ifNoneMatch: input.ifNoneMatch,
      credentials: 'same-origin',
      signal: input.signal
    });
  }

  // F5.5: Explicit Public-v1 native mutation client, separate from the
  // synchronous legacy editor. The caller supplies the current binding ETag,
  // one idempotency key and, for UPDATE, the complete native specification.
  // The reference client sends exactly once and returns a durable Operation,
  // never the legacy {success:true} result.
  function fetchPublicNativeTimerModify(options) {
    const input = normalizeOptions(options);
    return publicNativeTimerClient().modifyNativeTimer({
      backendId: input.backendId,
      timerId: input.timerId,
      ifMatch: input.ifMatch,
      idempotencyKey: input.idempotencyKey,
      payload: input.payload,
      headers: Object.assign({}, copyHeaders(input.headers), activeSessionCsrfHeaders()),
      credentials: 'same-origin',
      signal: input.signal
    });
  }

  // The public write returns HTTP 202, not a completed Timer mutation.
  // Read the existing durable Operation once; SSE is the frontend's
  // preferred completion signal when the edit workflow is migrated.
  function fetchPublicNativeTimerOperation(options) {
    const input = normalizeOptions(options);
    return publicNativeTimerClient().getOperation({
      operationId: input.operationId,
      ifNoneMatch: input.ifNoneMatch,
      credentials: 'same-origin',
      signal: input.signal
    });
  }

  function fetchPublicNativeTimerDelete(options) {
    const input = normalizeOptions(options);
    return publicNativeTimerClient().deleteNativeTimer({
      backendId: input.backendId,
      timerId: input.timerId,
      ifMatch: input.ifMatch,
      idempotencyKey: input.idempotencyKey,
      headers: Object.assign({}, copyHeaders(input.headers), activeSessionCsrfHeaders()),
      credentials: 'same-origin',
      signal: input.signal
    });
  }

  // One public EPG Timer CREATE path for all browser EPG surfaces. The
  // server owns the native timer, VDR margins and conflict decision.
  // Never synthesize If-Match from a cached EPG card or retry a POST.
  function fetchClientEpgTimerIntentCreate(options) {
    const input = normalizeOptions(options);
    const backendId = String(input.backendId || '').trim();
    const channelId = String(input.channelId || '').trim();
    const eventId = String(input.eventId || '').trim();
    if (!backendId || !channelId || !eventId) {
      return Promise.reject(new Error('Backend, Kanal und native EPG-Ereignis-ID sind erforderlich.'));
    }
    const random = window.crypto;
    if (!random || typeof random.getRandomValues !== 'function') {
      return Promise.reject(new Error('Sichere Auftrags-ID ist nicht verfügbar.'));
    }
    const bytes = new Uint8Array(16);
    random.getRandomValues(bytes);
    const idempotencyKey = 'epg-web-' +
      Array.from(bytes, byte => byte.toString(16).padStart(2, '0')).join('');
    const eventPath = '/api/v1/epg/event';
    const url = eventPath + buildQueryString({backendId, channelId, eventId});

    return fetch(url, requestOptions({cache: 'no-store', credentials: 'same-origin'}))
      .then(function (response) {
        return parseJsonResponse(eventPath, response).then(function (event) {
          if (!response.ok) {
            throw createClientError(eventPath, response.status, event);
          }
          const etag = response.headers && typeof response.headers.get === 'function'
            ? String(response.headers.get('ETag') || '') : '';
          if (!/^"[^"]+"$/.test(etag) ||
              !event || String(event.backendId || '') !== backendId ||
              String(event.channelId || '') !== channelId ||
              String(event.eventId || '') !== eventId ||
              !(Number(event.startTime) > 0) ||
              !(Number(event.endTime) > Number(event.startTime))) {
            throw new Error('EPG-Ereignis oder revisionssicherer ETag fehlt. Kein Timer gesendet.');
          }
          return requestJson('/api/v1/epg/timer-intents',
            queryMutationOptions(jsonPostOptions({
              payload: {backendId, channelId, eventId},
              headers: {
                'If-Match': etag,
                'Idempotency-Key': idempotencyKey
              },
              cache: 'no-store',
              credentials: 'same-origin'
            })));
        });
      });
  }

  function fetchClientTimerCreateAction(options) {
    return requestJson('/api/vdr/timers/actions/create', jsonPostOptions(options));
  }

  function fetchClientTimerUpdateAction(options) {
    return requestJson('/api/vdr/timers/actions/update', jsonPostOptions(options));
  }

  function fetchClientTimerDeleteAction(options) {
    return requestJson('/api/vdr/timers/actions/delete', jsonPostOptions(options));
  }

  function fetchClientChannels(options) {
    return requestJson('/api/vdr/channels', options);
  }

  function fetchClientTeletextService(options) {
    return requestJson('/api/vdr/broadcast/teletext/service', options);
  }

  function fetchClientTeletextPage(options) {
    return requestJson('/api/vdr/broadcast/teletext/page', options);
  }

  function fetchClientHbbtvApplications(options) {
    return requestJson('/api/vdr/broadcast/hbbtv/applications', options);
  }

  function hbbtvMutationOptions(options) {
    return jsonPostOptions(queryMutationOptions(options));
  }

  function fetchClientHbbtvSessionLaunch(options) {
    return requestJson(
      '/api/vdr/broadcast/hbbtv/sessions',
      hbbtvMutationOptions(options)
    );
  }

  function fetchClientHbbtvSessionStatus(options) {
    return requestJson(
      '/api/vdr/broadcast/hbbtv/sessions/status',
      hbbtvMutationOptions(options)
    );
  }

  function fetchClientHbbtvSessionInput(options) {
    return requestJson(
      '/api/vdr/broadcast/hbbtv/sessions/input',
      hbbtvMutationOptions(options)
    );
  }

  function fetchClientHbbtvSessionClose(options) {
    return requestJson(
      '/api/vdr/broadcast/hbbtv/sessions/close',
      hbbtvMutationOptions(options)
    );
  }

  function fetchClientHbbtvMedia(options) {
    return requestJson(
      '/api/vdr/broadcast/hbbtv/sessions/media',
      options
    );
  }

  function mutateClientHbbtvMedia(options) {
    return requestJson(
      '/api/vdr/broadcast/hbbtv/sessions/media',
      hbbtvMutationOptions(options)
    );
  }

  function fetchClientHbbtvPresentation(options) {
    const normalized = normalizeOptions(options);
    return requestBinary(
      '/api/vdr/broadcast/hbbtv/sessions/presentation',
      Object.assign({}, normalized, {
        headers: Object.assign(
          {},
          copyHeaders(normalized.headers),
          {Accept: 'image/qoi'}
        )
      })
    );
  }

  function fetchClientChannelMoveAction(options) {
    return requestJson('/api/vdr/channels/move', jsonPostOptions(options));
  }

  function fetchClientCapabilities(options) {
    return requestJson('/api/vdr/capabilities', options);
  }

  function fetchClientVdrOverview(options) {
    return requestJson('/api/vdr/overview', options);
  }

  function fetchClientVdrStatus(options) {
    return requestJson('/api/vdr/status', options);
  }

  function fetchClientVdrHealth(options) {
    return requestJson('/api/vdr/health', options);
  }

  function fetchClientVdrSnapshotSummary(options) {
    return requestJson('/api/vdr/snapshot', options);
  }

  function fetchClientVdrSnapshots(options) {
    return requestJson('/api/vdr/snapshots', options);
  }

  function fetchClientBackends(options) {
    return requestJson('/api/backends', options);
  }

  function fetchClientDefaultBackend(options) {
    return requestJson('/api/backends/default', options);
  }

  function fetchClientBackendSnapshot(backendId, options, onSnapshot) {
    const id = backendId ? String(backendId) : 'default';
    const snapshotRequest = requestJson(
      '/api/backends/' + encodeURIComponent(id) + '/snapshot',
      options
    ).then(function (snapshot) {
      // Optional early delivery; the returned Promise retains count composition.
      if (typeof onSnapshot === 'function') onSnapshot(snapshot);
      return snapshot;
    });
    const recordingStatusRequest = fetchClientRecordingCacheStatus(
      Object.assign({}, normalizeOptions(options), {backendId: id})
    ).catch(function () {
      return null;
    });

    return Promise.all([snapshotRequest, recordingStatusRequest])
      .then(function (results) {
        const snapshot = results[0];
        const recordingStatus = results[1];

        if (!snapshot || typeof snapshot !== 'object' ||
            !recordingStatus || typeof recordingStatus !== 'object') {
          return snapshot;
        }

        const recordingCount = Number(recordingStatus.totalCount);
        if (!Number.isFinite(recordingCount) || recordingCount < 0) {
          return snapshot;
        }

        return Object.assign({}, snapshot, {
          recordingCount: recordingCount
        });
      });
  }

  function fetchClientEpgWindow(options) {
    return requestJson('/api/vdr/events/live', options);
  }

  function fetchClientGlobalSearch(options) {
    return requestJson('/api/search', backendQueryOptions(options));
  }

  function fetchClientEpgSearch(options) {
    return requestJson('/api/epg/search', backendQueryOptions(options));
  }

  function fetchClientEpgCacheStatus(options) {
    return requestJson('/api/epg/cache/status', options);
  }

  function fetchClientEpgCacheNowNext(options) {
    return requestJson('/api/epg/cache/now-next', options);
  }

  function fetchClientEpgCacheNowNextArtwork(options) {
    return requestJson('/api/epg/cache/now-next-artwork', options);
  }

  function fetchClientEpgCacheWindow(options) {
    return requestJson('/api/epg/cache/window', options);
  }

  function fetchClientEpgCacheRefresh(options) {
    return requestJson('/api/epg/cache/refresh',
      backendQueryOptions(queryMutationOptions(options))
    );
  }

  function fetchClientEpgNowNext(options) {
    return requestJson('/api/epg/now-next', options);
  }

  function fetchClientEpgTimeWindow(options) {
    return requestJson('/api/epg/time-window', options);
  }

  function fetchClientEpgChannelWindow(options) {
    return requestJson('/api/epg/channel-window', options);
  }

  function fetchClientMetadata(options) {
    return requestJson('/api/metadata', options);
  }

  function fetchClientPersons(options) {
    return requestJson('/api/vdr/persons', options);
  }

  function fetchClientRecordingPersons(options) {
    return requestJson(
      '/api/vdr/recordings/persons/search',
      backendQueryOptions(options)
    );
  }

  function fetchClientRecordingTrailer(options) {
    const normalized = normalizeOptions(options);
    const backendId = normalized.backendId ? String(normalized.backendId) : 'default';
    return requestJson(
      '/api/backends/' + encodeURIComponent(backendId) + '/recordings/metadata/trailer',
      Object.assign({}, normalized, {backendId: undefined})
    );
  }

  function fetchClientRecordings(options) {
    return requestJson('/api/vdr/recordings/query', options);
  }

  function fetchClientRecordingCacheStatus(options) {
    return requestJson('/api/vdr/recordings/cache/status', backendQueryOptions(options));
  }

  function fetchClientRecordingFolder(options) {
    return requestJson('/api/vdr/recordings/folder', backendQueryOptions(options));
  }

  function fetchClientRecordingActionValidation(options) {
    return requestJson(
      '/api/vdr/recordings/actions/validate',
      jsonPostOptions(options)
    );
  }

  function fetchClientRecordingActionExecution(options) {
    return requestJson(
      '/api/vdr/recordings/actions/execute',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimers(options) {
    return requestJson('/api/vdr/searchtimers', options);
  }

  function fetchClientSearchTimerDiscovery(options) {
    return requestJson(
      '/api/vdr/searchtimers/discovery',
      backendQueryOptions(options)
    );
  }

  function fetchClientSearchTimerPreview(options) {
    return requestJson(
      '/api/vdr/searchtimers/preview',
      backendQueryOptions(options)
    );
  }

  function fetchClientSearchTimerPreviewCacheRefresh(options) {
    return requestJson(
      '/api/vdr/searchtimers/preview/cache/refresh',
      backendQueryOptions(queryMutationOptions(options))
    );
  }

  function fetchClientSearchTimerPlan(options) {
    return requestJson(
      '/api/vdr/searchtimers/plan',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimerValidate(options) {
    return requestJson(
      '/api/vdr/searchtimers/validate',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimerExecute(options) {
    return requestJson(
      '/api/vdr/searchtimers/execute',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimerRealTest(options) {
    return requestJson(
      '/api/vdr/searchtimers/real-test',
      jsonPostOptions(options)
    );
  }

  // Both Browser and Device clients use the same Public-v1 domain CREATE.
  // Browser cookie/CSRF is handled by queryMutationOptions; operation
  // identity and idempotency are owned by the Public-v1 backend.
  function fetchClientSearchTimerCreateStaged(options) {
    const normalized = normalizeOptions(options);
    return requestJson(
      '/api/v1/search-timers',
      queryMutationOptions(jsonPostOptions({
        query: { backend: normalized.backend },
        payload: normalized.payload,
        headers: { 'Idempotency-Key': normalized.idempotencyKey },
        cache: 'no-store',
        credentials: 'same-origin'
      }))
    );
  }

  function fetchClientSearchTimerCreateAction(options) {
    return fetchClientSearchTimerCreateStaged(options);
  }

  function fetchClientSearchTimerOperation(options) {
    const normalized = normalizeOptions(options);
    const id = String(normalized.operationId || '');
    if (!/^op_[0-9a-f]{32}$/.test(id)) {
      return Promise.reject(new Error('Ungültige Suchtimer-Vorgangs-ID.'));
    }
    return requestJson('/api/v1/operations/' + encodeURIComponent(id), {
      cache: 'no-store', credentials: 'same-origin'
    });
  }

  // Public-v1 reads and native compare-and-swap mutations use opaque IDs.
  function publicSearchTimerPath(options, suffix) {
    const id = String(options.searchTimerId || '');
    if (!/^st_[0-9a-f]{32}$/.test(id)) {
      throw new Error('Ungültige öffentliche Suchtimer-ID.');
    }
    return '/api/v1/search-timers/' + encodeURIComponent(id) + (suffix || '');
  }

  function fetchPublicSearchTimerPage(options) {
    const normalized = normalizeOptions(options);
    return requestJson('/api/v1/search-timers', {
      query: { backend: normalized.backend, cursor: normalized.cursor, limit: normalized.limit },
      cache: 'no-store', credentials: 'same-origin', signal: normalized.signal
    });
  }

  async function fetchPublicSearchTimers(options) {
    const normalized = normalizeOptions(options);
    const items = [];
    const cursors = new Set();
    const ids = new Set();
    let cursor = '';
    do {
      const page = await fetchPublicSearchTimerPage({
        backend: normalized.backend, cursor, limit: 30, signal: normalized.signal
      });
      if (!page || !Array.isArray(page.items) || !page.page ||
          !page.meta || page.meta.partial !== false) {
        throw new Error('Unvollständige Suchtimer-Liste.');
      }
      for (const item of page.items) {
        if (!item || item.backendId !== normalized.backend ||
            !/^st_[0-9a-f]{32}$/.test(item.searchTimerId) || ids.has(item.searchTimerId)) {
          throw new Error('Ungültige Suchtimer-Identität in der Liste.');
        }
        ids.add(item.searchTimerId);
        items.push(item);
      }
      cursor = page.page.hasMore ? page.page.nextCursor : '';
      if (page.page.hasMore &&
          (typeof cursor !== 'string' || !cursor || cursors.has(cursor))) {
        throw new Error('Ungültige Suchtimer-Folgeseite.');
      }
      if (cursor) cursors.add(cursor);
    } while (cursor);
    return {items, totalCount: items.length, meta: {partial: false}};
  }

  async function fetchPublicSearchTimerPreview(options) {
    const normalized = normalizeOptions(options);
    return requestJson(publicSearchTimerPath(normalized, '/preview'), {
      query: { backend: normalized.backend, limit: normalized.limit || 30 },
      cache: 'no-store', credentials: 'same-origin', signal: normalized.signal
    });
  }

  async function fetchPublicSearchTimerDetail(options) {
    const normalized = normalizeOptions(options);
    return requestJson(publicSearchTimerPath(normalized), {
      query: { backend: normalized.backend },
      cache: 'no-store', credentials: 'same-origin', signal: normalized.signal
    });
  }

  async function fetchPublicSearchTimerVersion(options) {
    const normalized = normalizeOptions(options);
    return requestJson(publicSearchTimerPath(normalized, '/version'), {
      query: { backend: normalized.backend },
      cache: 'no-store', credentials: 'same-origin', signal: normalized.signal
    });
  }

  async function mutatePublicSearchTimer(options) {
    const normalized = normalizeOptions(options);
    return requestJson(publicSearchTimerPath(normalized, '/mutations'),
      queryMutationOptions(jsonPostOptions({
        query: { backend: normalized.backend },
        payload: normalized.payload,
        headers: {
          'Idempotency-Key': normalized.idempotencyKey,
          'If-Match': '"' + normalized.revision + '"'
        },
        cache: 'no-store', credentials: 'same-origin', signal: normalized.signal
      }))
    );
  }

  function fetchClientSearchTimerUpdateAction(options) {
    return requestJson(
      '/api/vdr/searchtimers/update',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimerDeleteAction(options) {
    return requestJson(
      '/api/vdr/searchtimers/delete',
      jsonPostOptions(options)
    );
  }

  window.VdrSuiteClientApi = Object.freeze({
    requestJson: requestJson,
    requestBinary: requestBinary,
    isClientError: isClientError,
    fetchClientTimers: fetchClientTimers,
    fetchClientTimerConflicts: fetchClientTimerConflicts,
    fetchPublicNativeTimerPage: fetchPublicNativeTimerPage,
    fetchPublicNativeTimerDetail: fetchPublicNativeTimerDetail,
    fetchPublicNativeTimerModify: fetchPublicNativeTimerModify,
    fetchPublicNativeTimerDelete: fetchPublicNativeTimerDelete,
    fetchPublicNativeTimerOperation: fetchPublicNativeTimerOperation,
    fetchClientTimerCreateAction: fetchClientTimerCreateAction,
    fetchClientEpgTimerIntentCreate: fetchClientEpgTimerIntentCreate,
    fetchClientTimerUpdateAction: fetchClientTimerUpdateAction,
    fetchClientTimerDeleteAction: fetchClientTimerDeleteAction,
    fetchClientChannels: fetchClientChannels,
    fetchClientTeletextService: fetchClientTeletextService,
    fetchClientTeletextPage: fetchClientTeletextPage,
    fetchClientHbbtvApplications: fetchClientHbbtvApplications,
    fetchClientHbbtvSessionLaunch: fetchClientHbbtvSessionLaunch,
    fetchClientHbbtvSessionStatus: fetchClientHbbtvSessionStatus,
    fetchClientHbbtvSessionInput: fetchClientHbbtvSessionInput,
    fetchClientHbbtvSessionClose: fetchClientHbbtvSessionClose,
    fetchClientHbbtvMedia: fetchClientHbbtvMedia,
    mutateClientHbbtvMedia: mutateClientHbbtvMedia,
    fetchClientHbbtvPresentation: fetchClientHbbtvPresentation,
    fetchClientChannelMoveAction: fetchClientChannelMoveAction,
    fetchClientCapabilities: fetchClientCapabilities,
    fetchClientVdrOverview: fetchClientVdrOverview,
    fetchClientVdrStatus: fetchClientVdrStatus,
    fetchClientVdrHealth: fetchClientVdrHealth,
    fetchClientVdrSnapshotSummary: fetchClientVdrSnapshotSummary,
    fetchClientVdrSnapshots: fetchClientVdrSnapshots,
    fetchClientBackends: fetchClientBackends,
    fetchClientDefaultBackend: fetchClientDefaultBackend,
    fetchClientBackendSnapshot: fetchClientBackendSnapshot,
    fetchClientEpgWindow: fetchClientEpgWindow,
    fetchClientGlobalSearch: fetchClientGlobalSearch,
    fetchClientEpgSearch: fetchClientEpgSearch,
    fetchClientEpgCacheStatus: fetchClientEpgCacheStatus,
    fetchClientEpgCacheNowNext: fetchClientEpgCacheNowNext,
    fetchClientEpgCacheNowNextArtwork: fetchClientEpgCacheNowNextArtwork,
    fetchClientEpgCacheWindow: fetchClientEpgCacheWindow,
    fetchClientEpgCacheRefresh: fetchClientEpgCacheRefresh,
    fetchClientEpgNowNext: fetchClientEpgNowNext,
    fetchClientEpgTimeWindow: fetchClientEpgTimeWindow,
    fetchClientEpgChannelWindow: fetchClientEpgChannelWindow,
    fetchClientMetadata: fetchClientMetadata,
    fetchClientPersons: fetchClientPersons,
    fetchClientRecordingPersons: fetchClientRecordingPersons,
    fetchClientRecordingTrailer: fetchClientRecordingTrailer,
    fetchClientRecordings: fetchClientRecordings,
    fetchClientRecordingCacheStatus: fetchClientRecordingCacheStatus,
    fetchClientRecordingFolder: fetchClientRecordingFolder,
    fetchClientRecordingActionValidation: fetchClientRecordingActionValidation,
    fetchClientRecordingActionExecution: fetchClientRecordingActionExecution,
    fetchClientSearchTimers: fetchClientSearchTimers,
    fetchClientSearchTimerDiscovery: fetchClientSearchTimerDiscovery,
    fetchClientSearchTimerPreview: fetchClientSearchTimerPreview,
    fetchClientSearchTimerPreviewCacheRefresh: fetchClientSearchTimerPreviewCacheRefresh,
    fetchClientSearchTimerPlan: fetchClientSearchTimerPlan,
    fetchClientSearchTimerValidate: fetchClientSearchTimerValidate,
    fetchClientSearchTimerExecute: fetchClientSearchTimerExecute,
    fetchClientSearchTimerRealTest: fetchClientSearchTimerRealTest,
    fetchClientSearchTimerCreateAction: fetchClientSearchTimerCreateAction,
    fetchClientSearchTimerCreateStaged: fetchClientSearchTimerCreateStaged,
    fetchClientSearchTimerOperation: fetchClientSearchTimerOperation,
    fetchPublicSearchTimerPage: fetchPublicSearchTimerPage,
    fetchPublicSearchTimers: fetchPublicSearchTimers,
    fetchPublicSearchTimerPreview: fetchPublicSearchTimerPreview,
    fetchPublicSearchTimerDetail: fetchPublicSearchTimerDetail,
    fetchPublicSearchTimerVersion: fetchPublicSearchTimerVersion,
    mutatePublicSearchTimer: mutatePublicSearchTimer,
    fetchClientSearchTimerUpdateAction: fetchClientSearchTimerUpdateAction,
    fetchClientSearchTimerDeleteAction: fetchClientSearchTimerDeleteAction
  });
}());