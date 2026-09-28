'use strict';

(function (root, factory) {
  const exported = factory(root);

  if (typeof module === 'object' && module.exports) {
    module.exports = exported;
  }

  if (root) {
    root.VdrSuitePublicV1Client = exported;
  }
})(typeof globalThis !== 'undefined' ? globalThis : this, function (root) {
  class VdrSuitePublicClientError extends Error {
    constructor(path, status, payload, response) {
      const detail = payload && typeof payload === 'object'
        ? (payload.detail || payload.title || payload.message || payload.code)
        : null;
      super(detail ? String(detail) : ('HTTP ' + String(status) + ' from ' + path));
      this.name = 'VdrSuitePublicClientError';
      this.path = path;
      this.status = Number(status) || 0;
      this.code = payload && payload.code ? String(payload.code) : 'http_error';
      this.requestId = payload && payload.requestId
        ? String(payload.requestId)
        : headerValue(response, 'X-Request-ID');
      this.correlationId = payload && payload.correlationId
        ? String(payload.correlationId)
        : headerValue(response, 'X-Correlation-ID');
      this.payload = payload && typeof payload === 'object' ? payload : null;
    }
  }

  function headerValue(response, name) {
    if (!response || !response.headers || typeof response.headers.get !== 'function') {
      return null;
    }
    const value = response.headers.get(name);
    return value === null || value === undefined || value === '' ? null : String(value);
  }

  function copyHeaders(source) {
    const headers = {};
    if (!source) return headers;

    if (typeof source.forEach === 'function') {
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

  function normalizeBaseUrl(value) {
    if (value === undefined || value === null || value === '') return '';
    const text = String(value);
    if (text.indexOf('?') >= 0 || text.indexOf('#') >= 0) {
      throw new Error('baseUrl must not contain query or fragment');
    }
    return text.replace(/\/+$/, '');
  }

  function buildUrl(baseUrl, path) {
    return baseUrl + path;
  }

  function parseJsonBody(response) {
    return response.text().then(function (text) {
      if (text === '') return null;
      try {
        return JSON.parse(text);
      } catch (error) {
        throw new Error('Invalid JSON response');
      }
    });
  }

  function isPublicClientError(error) {
    return error instanceof VdrSuitePublicClientError
      || Boolean(error && error.name === 'VdrSuitePublicClientError');
  }

  function backendQuery(query) {
    if (query === undefined || query === null) return '';
    if (typeof query !== 'object' || Array.isArray(query)) {
      throw new Error('backend query must be an object');
    }

    const allowed = new Set(['limit', 'cursor', 'sort', 'order']);
    Object.keys(query).forEach(function (key) {
      if (!allowed.has(key)) {
        throw new Error('unsupported backend query field: ' + key);
      }
    });

    const params = new URLSearchParams();

    if (query.limit !== undefined) {
      if (!Number.isInteger(query.limit) || query.limit < 1 || query.limit > 100) {
        throw new Error('backend query limit must be an integer from 1 to 100');
      }
      params.set('limit', String(query.limit));
    }

    if (query.cursor !== undefined) {
      if (typeof query.cursor !== 'string' || query.cursor === '') {
        throw new Error('backend query cursor must be a non-empty string');
      }
      params.set('cursor', query.cursor);
    }

    if (query.sort !== undefined) {
      if (query.sort !== 'backendId') {
        throw new Error('backend query sort must be backendId');
      }
      params.set('sort', query.sort);
    }

    if (query.order !== undefined) {
      if (query.order !== 'asc') {
        throw new Error('backend query order must be asc');
      }
      params.set('order', query.order);
    }

    const encoded = params.toString();
    return encoded === '' ? '' : ('?' + encoded);
  }



  function channelQuery(query) {
    if (!query || typeof query !== 'object' || Array.isArray(query)) {
      throw new Error('channel query must be an object');
    }

    const allowed = new Set(['backendIds', 'limit', 'cursor', 'sort', 'order']);
    Object.keys(query).forEach(function (key) {
      if (!allowed.has(key)) {
        throw new Error('unsupported channel query field: ' + key);
      }
    });

    if (!Array.isArray(query.backendIds)
        || query.backendIds.length < 1
        || query.backendIds.length > 16) {
      throw new Error('channel query backendIds must contain 1 to 16 backends');
    }

    const parts = [];
    query.backendIds.forEach(function (backendId) {
      if (typeof backendId !== 'string' || backendId === '') {
        throw new Error('channel query backendIds must contain non-empty strings');
      }
      parts.push('backendId=' + encodeURIComponent(backendId));
    });

    if (query.limit !== undefined) {
      if (!Number.isInteger(query.limit) || query.limit < 1 || query.limit > 100) {
        throw new Error('channel query limit must be an integer from 1 to 100');
      }
      parts.push('limit=' + String(query.limit));
    }

    if (query.cursor !== undefined) {
      if (typeof query.cursor !== 'string' || query.cursor === '') {
        throw new Error('channel query cursor must be a non-empty string');
      }
      parts.push('cursor=' + encodeURIComponent(query.cursor));
    }

    if (query.sort !== undefined) {
      if (query.sort !== 'backendId,channelId') {
        throw new Error('channel query sort must be backendId,channelId');
      }
      parts.push('sort=backendId,channelId');
    }

    if (query.order !== undefined) {
      if (query.order !== 'asc') {
        throw new Error('channel query order must be asc');
      }
      parts.push('order=asc');
    }

    return '?' + parts.join('&');
  }



  function timerAssignmentCollectionQuery(query) {
    if (!query || typeof query !== 'object' || Array.isArray(query)) {
      throw new Error('TimerAssignment collection query must be an object');
    }

    const allowed = new Set(['backendId', 'limit', 'cursor', 'sort', 'order']);
    Object.keys(query).forEach(function (key) {
      if (!allowed.has(key)) {
        throw new Error('unsupported TimerAssignment collection query field: ' + key);
      }
    });

    if (typeof query.backendId !== 'string' || query.backendId === '') {
      throw new Error('TimerAssignment collection backendId must be a non-empty string');
    }

    const params = new URLSearchParams();
    params.set('backend', query.backendId);

    if (query.limit !== undefined) {
      if (!Number.isInteger(query.limit) || query.limit < 1 || query.limit > 100) {
        throw new Error('TimerAssignment collection limit must be an integer from 1 to 100');
      }
      params.set('limit', String(query.limit));
    }

    if (query.cursor !== undefined) {
      if (typeof query.cursor !== 'string' || query.cursor === '') {
        throw new Error('TimerAssignment collection cursor must be a non-empty string');
      }
      params.set('cursor', query.cursor);
    }

    if (query.sort !== undefined) {
      if (query.sort !== 'timerAssignmentId') {
        throw new Error('TimerAssignment collection sort must be timerAssignmentId');
      }
      params.set('sort', query.sort);
    }

    if (query.order !== undefined) {
      if (query.order !== 'asc') {
        throw new Error('TimerAssignment collection order must be asc');
      }
      params.set('order', query.order);
    }

    return '?' + params.toString();
  }



  function timerAssignmentItemPath(options) {
    if (!options || typeof options !== 'object' || Array.isArray(options)) {
      throw new Error('TimerAssignment item options must be an object');
    }
    if (typeof options.timerAssignmentId !== 'string' || options.timerAssignmentId === '') {
      throw new Error('TimerAssignment item timerAssignmentId must be a non-empty string');
    }
    if (/[\/?#]/.test(options.timerAssignmentId)) {
      throw new Error('TimerAssignment item timerAssignmentId contains a path delimiter');
    }
    if (typeof options.backendId !== 'string' || options.backendId === '') {
      throw new Error('TimerAssignment item backendId must be a non-empty string');
    }
    return '/api/v1/timer-assignments/' + options.timerAssignmentId
      + '?backend=' + encodeURIComponent(options.backendId);
  }

  function createClient(config) {
    const normalized = config && typeof config === 'object' ? config : {};
    const baseUrl = normalizeBaseUrl(normalized.baseUrl);
    const fetchImpl = normalized.fetch
      || (root && typeof root.fetch === 'function' ? root.fetch.bind(root) : null);
    const defaultHeaders = copyHeaders(normalized.headers);

    if (typeof fetchImpl !== 'function') {
      throw new Error('A fetch implementation is required');
    }

    function request(path, options) {
      const requestOptions = options && typeof options === 'object' ? options : {};
      const headers = Object.assign(
        {Accept: 'application/json'},
        defaultHeaders,
        copyHeaders(requestOptions.headers)
      );

      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'GET',
        headers: headers,
        credentials: requestOptions.credentials !== undefined
          ? requestOptions.credentials
          : normalized.credentials,
        cache: requestOptions.cache,
        signal: requestOptions.signal
      }).then(function (response) {
        return parseJsonBody(response).then(function (payload) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(path, response.status, payload, response);
          }
          return payload;
        });
      });
    }


    function requestRevisioned(path, options) {
      const requestOptions = options && typeof options === 'object' ? options : {};
      if (requestOptions.ifNoneMatch !== undefined
          && (typeof requestOptions.ifNoneMatch !== 'string'
              || requestOptions.ifNoneMatch === '')) {
        throw new Error('ifNoneMatch must be a non-empty string');
      }

      const headers = Object.assign(
        {Accept: 'application/json'},
        defaultHeaders,
        copyHeaders(requestOptions.headers)
      );
      if (requestOptions.ifNoneMatch !== undefined) {
        headers['If-None-Match'] = requestOptions.ifNoneMatch;
      }

      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'GET',
        headers: headers,
        credentials: requestOptions.credentials !== undefined
          ? requestOptions.credentials
          : normalized.credentials,
        cache: requestOptions.cache,
        signal: requestOptions.signal
      }).then(function (response) {
        const entityTag = headerValue(response, 'ETag');
        if (response.status === 304) {
          return {
            status: 304,
            etag: entityTag,
            data: null
          };
        }

        return parseJsonBody(response).then(function (payload) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(path, response.status, payload, response);
          }
          return {
            status: response.status,
            etag: entityTag,
            data: payload
          };
        });
      });
    }

    return Object.freeze({
      getApiRoot(options) {
        return request('/api/v1', options);
      },
      getCapabilities(options) {
        return request('/api/v1/capabilities', options);
      },
      getBackends(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request('/api/v1/backends' + backendQuery(normalizedOptions.query), normalizedOptions);
      },
      getChannels(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request('/api/v1/channels' + channelQuery(normalizedOptions.query), normalizedOptions);
      },
      getTimerAssignments(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request(
          '/api/v1/timer-assignments' + timerAssignmentCollectionQuery(normalizedOptions.query),
          normalizedOptions
        );
      },
      getTimerAssignment(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestRevisioned(
          timerAssignmentItemPath(normalizedOptions),
          normalizedOptions
        );
      }
    });
  }

  return Object.freeze({
    createClient: createClient,
    PublicClientError: VdrSuitePublicClientError,
    isClientError: isPublicClientError
  });
});
