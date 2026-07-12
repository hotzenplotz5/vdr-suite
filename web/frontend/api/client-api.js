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
      credentials: normalized.credentials
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
        return String(payload.error);
      }

      if (payload.message) {
        return String(payload.message);
      }
    }

    return 'Request failed for ' + path + ' with status ' + status;
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
          throw new Error(errorMessage(path, response.status, payload));
        }

        return payload;
      });
    });
  }

  function requestJsonWithFallback(path, fallbackPath, options) {
    return requestJson(path, options).catch(function (error) {
      if (!fallbackPath) {
        throw error;
      }

      return requestJson(fallbackPath, options);
    });
  }

  function requestJsonWithFallbacks(paths, options) {
    const candidates = Array.isArray(paths) ? paths.slice() : [];

    function tryNext(index) {
      if (index >= candidates.length) {
        return Promise.reject(new Error('No fallback path available'));
      }

      return requestJson(candidates[index], options).catch(function (error) {
        if (index >= candidates.length - 1) {
          throw error;
        }

        return tryNext(index + 1);
      });
    }

    return tryNext(0);
  }

  function fetchClientTimers(options) {
    return requestJsonWithFallback(
      '/api/vdr/timers/live',
      '/api/vdr/timers',
      options
    );
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

  function fetchClientChannelMoveAction(options) {
    return requestJson('/api/vdr/channels/move', jsonPostOptions(options));
  }

  function fetchClientCapabilities(options) {
    return requestJson('/api/vdr/capabilities', options);
  }

  function fetchClientVdrOverview(options) {
    return requestJsonWithFallback('/api/vdr/overview', '/api/vdr', options);
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
    return requestJson('/api/backends/' + encodeURIComponent(id) + '/snapshot', options);
  }

  function fetchClientEpgWindow(options) {
    return requestJson('/api/vdr/events/live', options);
  }

  function fetchClientEpgSearch(options) {
    return requestJson('/api/epg/search', backendQueryOptions(options));
  }

  function fetchClientEpgCacheStatus(options) {
    return requestJson('/api/epg/cache/status', options);
  }

  function fetchClientEpgCacheWindow(options) {
    return requestJson('/api/epg/cache/window', options);
  }

  function fetchClientEpgCacheRefresh(options) {
    const normalized = normalizeOptions(options);

    return requestJson('/api/epg/cache/refresh', Object.assign({}, normalized, {
      method: normalized.method || 'POST'
    }));
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
    return requestJsonWithFallback(
      '/api/vdr/persons',
      '/api/persons',
      options
    );
  }

  function fetchClientRecordingPersons(options) {
    return requestJsonWithFallback(
      '/api/vdr/recordings/persons/search',
      '/api/recordings/persons/search',
      backendQueryOptions(options)
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
    return requestJsonWithFallback(
      '/api/vdr/recordings/actions/validate',
      '/api/recordings/actions/validate',
      jsonPostOptions(options)
    );
  }

  function recordingListFromResponse(data) {
    if (Array.isArray(data)) {
      return data;
    }

    if (data && Array.isArray(data.recordings)) {
      return data.recordings;
    }

    if (data && Array.isArray(data.items)) {
      return data.items;
    }

    return [];
  }

  function recordingTitle(recording) {
    if (!recording || typeof recording !== 'object') {
      return '';
    }

    return String(
      recording.title ||
      recording.name ||
      recording.displayName ||
      ''
    ).trim();
  }

  function normalizedRecordingActionPayload(options) {
    const normalized = normalizeOptions(options);

    if (normalized.payload && typeof normalized.payload === 'object') {
      return normalized.payload;
    }

    if (normalized.body && typeof normalized.body === 'object') {
      return normalized.body;
    }

    return {};
  }

  function delay(milliseconds) {
    return new Promise(function (resolve) {
      window.setTimeout(resolve, milliseconds);
    });
  }

  function refreshRecordingView() {
    if (typeof window.loadRecordings === 'function') {
      window.loadRecordings();
      return;
    }

    const refreshButton = document.getElementById('refresh-detail');
    if (refreshButton && !refreshButton.disabled) {
      refreshButton.click();
    }
  }

  function renameVisibleInCache(data, expectedName) {
    const normalizedExpected = String(expectedName || '').trim();

    if (normalizedExpected === '') {
      return true;
    }

    return recordingListFromResponse(data).some(function (recording) {
      const title = recordingTitle(recording);
      return title === normalizedExpected ||
        title.endsWith('/' + normalizedExpected);
    });
  }

  function waitForRenameCache(payload, attempt) {
    const expectedName = String(payload.newName || '').trim();
    const backendId = String(payload.backendId || 'default').trim() || 'default';

    if (expectedName === '') {
      return Promise.resolve(false);
    }

    return requestJson('/api/vdr/recordings/query', {
      query: {
        backend: backendId,
        title: expectedName,
        limit: 50,
        offset: 0,
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    })
      .then(function (data) {
        if (renameVisibleInCache(data, expectedName)) {
          return true;
        }

        if (attempt >= 59) {
          return false;
        }

        return delay(1000).then(function () {
          return waitForRenameCache(payload, attempt + 1);
        });
      })
      .catch(function () {
        if (attempt >= 59) {
          return false;
        }

        return delay(1000).then(function () {
          return waitForRenameCache(payload, attempt + 1);
        });
      });
  }

  function synchronizeRecordingView(options, result) {
    if (result && Object.prototype.hasOwnProperty.call(result, 'success') && result.success === false) {
      return Promise.resolve(result);
    }

    const payload = normalizedRecordingActionPayload(options);
    const action = String(payload.action || '').toUpperCase();

    if (action !== 'RENAME') {
      refreshRecordingView();
      return Promise.resolve(result);
    }

    return waitForRenameCache(payload, 0).then(function () {
      refreshRecordingView();
      return result;
    });
  }

  function fetchClientRecordingActionExecution(options) {
    const normalized = normalizeOptions(options);

    return requestJsonWithFallback(
      '/api/vdr/recordings/actions/execute',
      '/api/recordings/actions/execute',
      jsonPostOptions(normalized)
    ).then(function (result) {
      return synchronizeRecordingView(normalized, result);
    });
  }

  function fetchClientSearchTimers(options) {
    return requestJsonWithFallbacks(
      [
        '/api/vdr/searchtimers/live',
        '/api/vdr/searchtimers',
        '/api/searchtimers'
      ],
      options
    );
  }

  function fetchClientSearchTimerDiscovery(options) {
    return requestJsonWithFallback(
      '/api/vdr/searchtimers/discovery',
      '/api/searchtimers/discovery',
      backendQueryOptions(options)
    );
  }

  function fetchClientSearchTimerPreview(options) {
    return requestJsonWithFallback(
      '/api/vdr/searchtimers/preview',
      '/api/searchtimers/preview',
      backendQueryOptions(options)
    );
  }

  function fetchClientSearchTimerPlan(options) {
    return requestJsonWithFallback(
      '/api/vdr/searchtimers/plan',
      '/api/searchtimers/plan',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimerValidate(options) {
    return requestJsonWithFallback(
      '/api/vdr/searchtimers/validate',
      '/api/searchtimers/validate',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimerExecute(options) {
    return requestJsonWithFallback(
      '/api/vdr/searchtimers/execute',
      '/api/searchtimers/execute',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimerRealTest(options) {
    return requestJsonWithFallback(
      '/api/vdr/searchtimers/real-test',
      '/api/searchtimers/real-test',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimerCreateAction(options) {
    return requestJsonWithFallback(
      '/api/vdr/searchtimers',
      '/api/searchtimers',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimerUpdateAction(options) {
    return requestJsonWithFallback(
      '/api/vdr/searchtimers/update',
      '/api/searchtimers/update',
      jsonPostOptions(options)
    );
  }

  function fetchClientSearchTimerDeleteAction(options) {
    return requestJsonWithFallback(
      '/api/vdr/searchtimers/delete',
      '/api/searchtimers/delete',
      jsonPostOptions(options)
    );
  }

  window.VdrSuiteClientApi = Object.freeze({
    requestJson: requestJson,
    fetchClientTimers: fetchClientTimers,
    fetchClientTimerConflicts: fetchClientTimerConflicts,
    fetchClientTimerCreateAction: fetchClientTimerCreateAction,
    fetchClientTimerUpdateAction: fetchClientTimerUpdateAction,
    fetchClientTimerDeleteAction: fetchClientTimerDeleteAction,
    fetchClientChannels: fetchClientChannels,
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
    fetchClientEpgSearch: fetchClientEpgSearch,
    fetchClientEpgCacheStatus: fetchClientEpgCacheStatus,
    fetchClientEpgCacheWindow: fetchClientEpgCacheWindow,
    fetchClientEpgCacheRefresh: fetchClientEpgCacheRefresh,
    fetchClientEpgNowNext: fetchClientEpgNowNext,
    fetchClientEpgTimeWindow: fetchClientEpgTimeWindow,
    fetchClientEpgChannelWindow: fetchClientEpgChannelWindow,
    fetchClientMetadata: fetchClientMetadata,
    fetchClientPersons: fetchClientPersons,
    fetchClientRecordingPersons: fetchClientRecordingPersons,
    fetchClientRecordings: fetchClientRecordings,
    fetchClientRecordingCacheStatus: fetchClientRecordingCacheStatus,
    fetchClientRecordingFolder: fetchClientRecordingFolder,
    fetchClientRecordingActionValidation: fetchClientRecordingActionValidation,
    fetchClientRecordingActionExecution: fetchClientRecordingActionExecution,
    fetchClientSearchTimers: fetchClientSearchTimers,
    fetchClientSearchTimerDiscovery: fetchClientSearchTimerDiscovery,
    fetchClientSearchTimerPreview: fetchClientSearchTimerPreview,
    fetchClientSearchTimerPlan: fetchClientSearchTimerPlan,
    fetchClientSearchTimerValidate: fetchClientSearchTimerValidate,
    fetchClientSearchTimerExecute: fetchClientSearchTimerExecute,
    fetchClientSearchTimerRealTest: fetchClientSearchTimerRealTest,
    fetchClientSearchTimerCreateAction: fetchClientSearchTimerCreateAction,
    fetchClientSearchTimerUpdateAction: fetchClientSearchTimerUpdateAction,
    fetchClientSearchTimerDeleteAction: fetchClientSearchTimerDeleteAction
  });
}());
