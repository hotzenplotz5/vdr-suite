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

    return fetch(url, requestOptions(normalized)).then(function (response) {
      return parseJsonResponse(path, response).then(function (payload) {
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

      if (response.status === 204) {
        return {
          status: 204,
          revision: revision,
          width: width,
          height: height,
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

  function fetchClientBackendSnapshot(backendId, options) {
    const id = backendId ? String(backendId) : 'default';
    const snapshotRequest = requestJson(
      '/api/backends/' + encodeURIComponent(id) + '/snapshot',
      options
    );
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

  function fetchClientSearchTimerCreateAction(options) {
    return requestJson(
      '/api/vdr/searchtimers',
      jsonPostOptions(options)
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
    fetchClientTimerCreateAction: fetchClientTimerCreateAction,
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
    fetchClientSearchTimerUpdateAction: fetchClientSearchTimerUpdateAction,
    fetchClientSearchTimerDeleteAction: fetchClientSearchTimerDeleteAction
  });
}());