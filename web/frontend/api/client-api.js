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
      (path !== '/api/metadata/genres' || query.scope === 'recordings') &&
      (path !== '/api/v1/recordings' || query.view === 'movies')
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
    const input = normalizeOptions(options);
    // Compatibility for older callers without a backend scope. All
    // production browser calls supply the selected backend explicitly.
    if (!input.backendId) return requestJson('/api/vdr/timers/live', options);
    return publicNativeTimerClient().getLiveNativeTimers({
      backendId: input.backendId, credentials: 'same-origin',
      cache: 'no-store', signal: input.signal
    });
  }

  function fetchClientTimerConflicts(options) {
    const input = normalizeOptions(options);
    if (!input.backendId) return requestJson('/api/vdr/timers/conflicts/live', options);
    return publicNativeTimerClient().getNativeTimerConflicts({
      backendId: input.backendId, credentials: 'same-origin',
      cache: 'no-store', signal: input.signal
    });
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

  // F5.6.2: raw Aux is a separate unconditional native VDR read.
  // Unlike timer detail it has no Binding ETag and cannot return 304.
  function fetchPublicNativeTimerAux(options) {
    const input = normalizeOptions(options);
    return publicNativeTimerClient().getNativeTimerAux({
      backendId: input.backendId,
      timerId: input.timerId,
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

  // Slice 3: direct external/unbound native actions retain the single
  // established VDR writer and return a synchronous result, not HTTP 202.
  function fetchPublicNativeTimerDirectCreate(options) {
    const input = normalizeOptions(options);
    return publicNativeTimerClient().createNativeTimer({
      backendId: input.payload && input.payload.backendId, payload: input.payload,
      headers: Object.assign({}, copyHeaders(input.headers), activeSessionCsrfHeaders()),
      credentials: 'same-origin', signal: input.signal
    }).then(result => result.data);
  }

  function fetchPublicNativeTimerDirectUpdate(options) {
    const input = normalizeOptions(options);
    return publicNativeTimerClient().updateExternalNativeTimer({
      backendId: input.backendId, timerId: input.timerId,
      payload: input.payload,
      headers: Object.assign({}, copyHeaders(input.headers), activeSessionCsrfHeaders()),
      credentials: 'same-origin', signal: input.signal
    }).then(result => result.data);
  }

  function fetchPublicNativeTimerDirectDelete(options) {
    const input = normalizeOptions(options);
    return publicNativeTimerClient().deleteExternalNativeTimer({
      backendId: input.backendId, timerId: input.timerId,
      headers: Object.assign({}, copyHeaders(input.headers), activeSessionCsrfHeaders()),
      credentials: 'same-origin', signal: input.signal
    }).then(result => result.data);
  }

  // F5.6: manual one-shot Timer CREATE uses the same Public-v1 client,
  // current Browser session/CSRF and caller's stable Idempotency-Key.
  // A 202 is only an Operation; no legacy mutation or automatic retry.
  function fetchPublicManualTimerCreate(options) {
    const input = normalizeOptions(options);
    return publicNativeTimerClient().submitManualTimerIntent({
      payload: input.payload,
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

  // Timer editors consume only the public Channel identity, number, name,
  // group, radio and enabled fields. Other browser surfaces still require
  // their richer legacy projection. Keep the existing authenticated browser
  // read for users whose editor access does not include channels.view.
  async function fetchEditorChannelCatalog(input) {
    const backendId = input.query && input.query.backend;
    const factory = window.VdrSuitePublicV1Client;
    if (!backendId || !factory || typeof factory.createClient !== 'function')
      return requestJson('/api/vdr/channels', input);
    const client = publicNativeTimerClient();
    const items = [];
    const seen = new Set();
    const cursors = new Set();
    let cursor;
    try {
      do {
        const page = await client.getChannels({
          query: {backendIds: [backendId], limit: 100, cursor,
            sort: 'backendId,channelNumber,channelId', order: 'asc'},
          cache: input.cache, credentials: input.credentials || 'same-origin',
          signal: input.signal
        });
        if (!page || !Array.isArray(page.items) || !page.page ||
            !page.meta || page.meta.partial !== false)
          return requestJson('/api/vdr/channels', input);
        for (const channel of page.items) {
          if (!channel || channel.backendId !== backendId ||
              typeof channel.channelId !== 'string' || !channel.channelId)
            return requestJson('/api/vdr/channels', input);
          if (!seen.has(channel.channelId)) {
            seen.add(channel.channelId);
            items.push(channel);
          }
        }
        if (!page.page.hasMore) break;
        cursor = page.page.nextCursor;
        if (typeof cursor !== 'string' || !cursor || cursors.has(cursor))
          return requestJson('/api/vdr/channels', input);
        cursors.add(cursor);
      } while (cursor);
    } catch (error) {
      // Browser-only compatibility: a current editor user can have Timer
      // rights without the separate public Channel grant. Do not alter grants
      // or retry an unknown mutation (this is a read-only catalog).
      if (error && error.status === 403 &&
          (error.code === 'permission_denied' || error.code === 'backend_scope_denied'))
        return requestJson('/api/vdr/channels', input);
      throw error;
    }
    return {channels: items};
  }

  function fetchClientChannels(options) {
    const input = normalizeOptions(options);
    if (input.editorCatalog === true) return fetchEditorChannelCatalog(input);
    return requestJson('/api/vdr/channels', input);
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

  function globalSearchArtworkUrl(backendId, recordingId, artworkId) {
    return /^[A-Za-z0-9._-]+$/.test(String(backendId || '')) &&
      /^rec_[0-9a-f]{32}$/.test(String(recordingId || '')) &&
      /^[0-9a-f]{32}$/.test(String(artworkId || ''))
      ? '/api/v1/recordings/' + recordingId + '/artwork/' + artworkId +
        '?backendId=' + encodeURIComponent(backendId) : '';
  }

  // Adapt the existing Public-v1 page contracts to the search dialog's
  // existing card owners. Group only by the server's canonical personKey;
  // do not introduce another text matcher or re-rank results.
  function globalSearchBrowserView(payload, backendId, query) {
    const recordingPage = payload && payload.recordings;
    const epgPage = payload && payload.epg;
    if (!recordingPage || !epgPage ||
        !Array.isArray(recordingPage.items) || !Array.isArray(epgPage.items) ||
        !recordingPage.page || !epgPage.page)
      throw new Error('Ungültige Public-v1-Suchantwort');
    const people = new Map();
    function person(item, recording) {
      if (!item.personKey || !item.matchedPerson) return;
      const key = item.personKey;
      const entry = people.get(key) || {name:'',role:'',recordingCount:0,epgCount:0,
        image:{available:false,url:''}};
      entry.name = item.matchedPerson;
      entry.role = item.matchedRole || '';
      if (recording) entry.recordingCount += 1;
      else entry.epgCount += 1;
      const ref = item.personPortrait;
      const url = ref && globalSearchArtworkUrl(backendId, ref.recordingId, ref.artworkId);
      if (url && !entry.image.available) entry.image = {available:true,url};
      people.set(key, entry);
    }
    const recordings = recordingPage.items.map(item => {
      if (item.kind !== 'recording' || item.backendId !== backendId ||
          !/^rec_[0-9a-f]{32}$/.test(String(item.recordingId || '')))
        throw new Error('Ungültige Aufnahmeidentität in der Suchantwort');
      person(item, true);
      return Object.assign({}, item, {
        recordedAt: String(item.startTime || ''),
        metadata: {subtitle: item.subtitle || ''}
      });
    });
    const epg = epgPage.items.map(item => {
      if (item.kind !== 'epg' || item.backendId !== backendId ||
          !item.channelId || !item.eventId)
        throw new Error('Ungültige EPG-Identität in der Suchantwort');
      person(item, false);
      const url = item.artworkAvailable ? '/api/v1/epg/artwork?backendId=' +
        encodeURIComponent(backendId) + '&channelId=' +
        encodeURIComponent(item.channelId) + '&eventId=' +
        encodeURIComponent(item.eventId) + '&variant=home' : '';
      return Object.assign({}, item, {id:item.eventId, artwork:{available:!!url,url}});
    });
    return {
      backendId, query, status:'ready', recordings, epg,
      recordingTotal:recordingPage.page.totalCount,
      epgTotal:epgPage.page.totalCount,
      recordingHasMore:recordingPage.page.hasMore,
      epgHasMore:epgPage.page.hasMore,
      hasMore:recordingPage.page.hasMore || epgPage.page.hasMore,
      people:Array.from(people.keys()).sort().map(key => people.get(key))
    };
  }

  function fetchClientGlobalSearch(options) {
    const input = normalizeOptions(options);
    const query = copyQuery(input.query);
    const legacy = () => requestJson('/api/search', backendQueryOptions(options));
    const backendId = input.backendId || query.backend || 'default';
    const searchText = query.query || query.text || '';
    const limit = query.limit === undefined ? 20 : Number(query.limit);
    const offset = query.offset === undefined ? 0 : Number(query.offset);
    // Keep the established Browser route for its existing wider query
    // options; the dialog uses the common 24-hit/default-window contract.
    if (typeof searchText !== 'string' || searchText.trim().length < 2 ||
        searchText.length > 240 || !Number.isInteger(limit) || limit < 1 ||
        limit > 30 || !Number.isInteger(offset) || offset < 0 || offset > 1000000 ||
        Object.keys(query).some(key => !['backend','query','text','limit','offset','_'].includes(key)))
      return legacy();
    return requestJson('/api/v1/search', {
      query:{backendId,query:searchText,limit,offset},
      cache:input.cache || 'no-store', credentials:input.credentials || 'same-origin',
      signal:input.signal
    }).then(payload => globalSearchBrowserView(payload, backendId, searchText))
      .catch(error => {
        // Independently authenticated Browser compatibility for older daemons
        // or pre-existing browser-only access. Never retry authentication loss,
        // transport failures or invalid Public-v1 data through another route.
        if (error && (error.status === 404 || error.status === 405 ||
            (error.status === 403 && ['permission_denied','backend_scope_denied'].includes(error.code))))
          return legacy();
        throw error;
      });
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
      '/api/v1/backends/' + encodeURIComponent(backendId) + '/recordings/metadata/trailer',
      Object.assign({}, normalized, {backendId: undefined})
    );
  }

  function fetchClientRecordings(options) {
    return requestJson('/api/vdr/recordings/query', options);
  }

  // Adapt the canonical public Home movie pages without inventing native
  // recording paths. Retain the established large expanded-rail read by
  // traversing the public offset pages (maximum 100 per HTTP response).
  async function fetchPublicHomeMovies(options) {
    const input = normalizeOptions(options);
    const query = input.query || {};
    const backendId = query.backend;
    const start = Number(query.offset || 0);
    const count = Number(query.limit);
    const rows = [], pages = [], ids = new Set();
    let total;
    let offset = start;
    do {
      const limit = Math.min(100, count - rows.length);
      let payload;
      try {
        payload = await requestJson('/api/v1/recordings', {
          query: {backendId, view: 'movies', limit, offset,
            movieReleaseYearFrom: query.movieReleaseYearFrom,
            movieReleaseYearTo: query.movieReleaseYearTo},
          credentials: input.credentials || 'same-origin',
          cache: input.cache || 'no-store', signal: input.signal
        });
      } catch (error) {
        // Only explicit older-route absence permits compatibility. Never
        // fall back on an authorization or provider failure.
        if (!pages.length && (error.status === 404 || error.status === 501))
          return fetchClientRecordings(input);
        throw error;
      }
      const page = payload && payload.page;
      const items = payload && payload.items;
      if (!page || !Array.isArray(items) || !payload.meta || payload.meta.partial !== false ||
          page.offset !== offset || page.limit !== limit ||
          !Number.isSafeInteger(page.totalCount) || page.totalCount < 0 ||
          items.length > limit ||
          (offset > page.totalCount ? items.length !== 0 : offset + items.length > page.totalCount) ||
          page.hasMore !== (offset + items.length < page.totalCount) ||
          (page.hasMore && !items.length) ||
          (total !== undefined && total !== page.totalCount))
        throw new Error('Unvollständige Filmseite.');
      // Older versions of this same endpoint lack the effective release date.
      // A whole first page lacking that additive field identifies the old
      // presentation contract. Never mix proven public and legacy pages.
      if (!pages.length && items.length && items.every(item =>
          item && item.releaseDate === undefined)) return fetchClientRecordings(input);
      for (const item of items) {
        if (!item || item.kind !== 'recording' || item.backendId !== backendId ||
            !/^rec_[0-9a-f]{32}$/.test(item.recordingId) ||
            typeof item.recordedAt !== 'string' || typeof item.releaseDate !== 'string' ||
            ids.has(item.recordingId)) throw new Error('Uneinheitliche Filmseite.');
        ids.add(item.recordingId);
        rows.push(item);
      }
      pages.push(payload);
      total = page.totalCount;
      offset += items.length;
      if (!page.hasMore) break;
    } while (rows.length < count);
    if (typeof input.onPage === 'function') pages.forEach(input.onPage);
    return {recordings: rows, totalCount: total};
  }

  function fetchClientRecordingCacheStatus(options) {
    return requestJson('/api/vdr/recordings/cache/status', backendQueryOptions(options));
  }

  function fetchClientRecordingFolder(options) {
    return requestJson('/api/vdr/recordings/folder', backendQueryOptions(options));
  }

  // Keep the exact provider preflight ETag with the public action response.
  // Browser CSRF comes from the same existing session transport.
  function fetchPublicRecordingAction(options) {
    const input = normalizeOptions(options);
    const path = '/api/v1/recordings/actions/' +
      (input.execute === true ? 'execute' : 'validate');
    return fetch(path, requestOptions(queryMutationOptions(jsonPostOptions(input))))
      .then(function (response) {
        return parseJsonResponse(path, response).then(function (data) {
          if (!response.ok) throw createClientError(path, response.status, data);
          return {data: data, etag: response.headers &&
            typeof response.headers.get === 'function' ? response.headers.get('ETag') || '' : ''};
        });
      });
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

  function fetchPublicSearchTimerDraftPreview(options) {
    const normalized = normalizeOptions(options);
    const payload = normalized.payload || {};
    const query = {backend: normalized.backend, query: payload.query,
      mode: payload.mode, limit: normalized.limit || 30};
    // Only existing EPG matcher options belong to the read-only preview.
    for (const key of ['compareTitle','compareSubtitle','compareSummary',
      'useDuration','useTime','useDayOfWeek']) query[key] = payload[key] ? 1 : 0;
    for (const key of ['useChannel','channels','channelMin','channelMax',
      'durationMinMinutes','durationMaxMinutes','startTime','stopTime','dayOfWeek'])
      query[key] = payload[key];
    if (payload.mode === 5) query.tolerance = payload.tolerance;
    return requestJson('/api/v1/search-timers/draft-preview', {
      query, cache: 'no-store', credentials: 'same-origin', signal: normalized.signal
    });
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
    fetchPublicNativeTimerAux: fetchPublicNativeTimerAux,
    fetchPublicNativeTimerModify: fetchPublicNativeTimerModify,
    fetchPublicNativeTimerDelete: fetchPublicNativeTimerDelete,
    fetchPublicNativeTimerDirectCreate: fetchPublicNativeTimerDirectCreate,
    fetchPublicNativeTimerDirectUpdate: fetchPublicNativeTimerDirectUpdate,
    fetchPublicNativeTimerDirectDelete: fetchPublicNativeTimerDirectDelete,
    fetchPublicNativeTimerOperation: fetchPublicNativeTimerOperation,
    fetchPublicManualTimerCreate: fetchPublicManualTimerCreate,
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
    fetchPublicHomeMovies: fetchPublicHomeMovies,
    fetchClientRecordingCacheStatus: fetchClientRecordingCacheStatus,
    fetchClientRecordingFolder: fetchClientRecordingFolder,
    fetchPublicRecordingAction: fetchPublicRecordingAction,
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
    fetchPublicSearchTimerDraftPreview: fetchPublicSearchTimerDraftPreview,
    fetchPublicSearchTimerDetail: fetchPublicSearchTimerDetail,
    fetchPublicSearchTimerVersion: fetchPublicSearchTimerVersion,
    mutatePublicSearchTimer: mutatePublicSearchTimer,
    fetchClientSearchTimerUpdateAction: fetchClientSearchTimerUpdateAction,
    fetchClientSearchTimerDeleteAction: fetchClientSearchTimerDeleteAction
  });
}());