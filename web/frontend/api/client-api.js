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

  function requestOptions(options) {
    const normalized = normalizeOptions(options);

    return {
      method: normalized.method || 'GET',
      headers: Object.assign({}, DEFAULT_HEADERS, normalized.headers || {}),
      body: normalized.body
    };
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

  function fetchClientChannels(options) {
    return requestJson('/api/vdr/channels/live', options);
  }

  function fetchClientEpgWindow(options) {
    return requestJson('/api/vdr/events/live', options);
  }

  function fetchClientRecordings(options) {
    return requestJson('/api/vdr/recordings/live', options);
  }

  function fetchClientSearchTimers(options) {
    return requestJson('/api/vdr/searchtimers/live', options);
  }

  window.VdrSuiteClientApi = Object.freeze({
    requestJson: requestJson,
    fetchClientTimers: fetchClientTimers,
    fetchClientTimerConflicts: fetchClientTimerConflicts,
    fetchClientChannels: fetchClientChannels,
    fetchClientEpgWindow: fetchClientEpgWindow,
    fetchClientRecordings: fetchClientRecordings,
    fetchClientSearchTimers: fetchClientSearchTimers
  });
}());
