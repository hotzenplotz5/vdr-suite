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



  function deviceCollectionQuery(query) {
    if (query === undefined || query === null) return '';
    if (typeof query !== 'object' || Array.isArray(query)) {
      throw new Error('Device collection query must be an object');
    }
    const allowed = new Set(['limit', 'cursor']);
    Object.keys(query).forEach(function(key) {
      if (!allowed.has(key)) throw new Error('unsupported Device query: ' + key);
    });
    const params = new URLSearchParams();
    if (query.limit !== undefined) {
      if (!Number.isInteger(query.limit) || query.limit < 1 || query.limit > 100)
        throw new Error('Device limit must be 1..100');
      params.set('limit', String(query.limit));
    }
    if (query.cursor !== undefined) {
      if (typeof query.cursor !== 'string' ||
          !/^dv1_[A-Za-z0-9_.-]{1,128}$/.test(query.cursor))
        throw new Error('Device cursor is invalid');
      params.set('cursor', query.cursor);
    }
    const result = params.toString();
    return result ? '?' + result : '';
  }

  function accountQuery(query) {
    if (query === undefined || query === null) return '';
    if (typeof query !== 'object' || Array.isArray(query)) {
      throw new Error('account query must be an object');
    }

    const allowed = new Set(['limit', 'cursor', 'sort', 'order']);
    Object.keys(query).forEach(function (key) {
      if (!allowed.has(key)) {
        throw new Error('unsupported account query field: ' + key);
      }
    });

    const params = new URLSearchParams();

    if (query.limit !== undefined) {
      if (!Number.isInteger(query.limit) || query.limit < 1 || query.limit > 100) {
        throw new Error('account query limit must be an integer from 1 to 100');
      }
      params.set('limit', String(query.limit));
    }

    if (query.cursor !== undefined) {
      if (typeof query.cursor !== 'string' || query.cursor === '') {
        throw new Error('account query cursor must be a non-empty string');
      }
      params.set('cursor', query.cursor);
    }

    if (query.sort !== undefined) {
      if (query.sort !== 'accountId') {
        throw new Error('account query sort must be accountId');
      }
      params.set('sort', query.sort);
    }

    if (query.order !== undefined) {
      if (query.order !== 'asc') {
        throw new Error('account query order must be asc');
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
      if (query.sort !== 'backendId,channelId' &&
          query.sort !== 'backendId,channelNumber,channelId') {
        throw new Error('unsupported Channel collection ordering');
      }
      parts.push('sort=' + query.sort);
    }

    if (query.order !== undefined) {
      if (query.order !== 'asc') {
        throw new Error('channel query order must be asc');
      }
      parts.push('order=asc');
    }

    return '?' + parts.join('&');
  }




  // F3: Public-v1 read-only EPG and search queries. No browser-private fallback.
  function f3CanonicalId(value, label, maxLength) {
    if (typeof value !== 'string' || value.length > maxLength ||
        !/^[A-Za-z0-9._-]+$/.test(value)) {
      throw new Error(label + ' must be a canonical identifier');
    }
    return value;
  }

  function f3UnixSeconds(value, label) {
    const text = typeof value === 'number' && Number.isSafeInteger(value)
      ? String(value) : value;
    if (typeof text !== 'string' || !/^[1-9][0-9]{9,11}$/.test(text)) {
      throw new Error(label + ' must be Unix seconds (10..12 digits)');
    }
    return text;
  }

  function f3EpgQuery(query, window) {
    if (!query || typeof query !== 'object' || Array.isArray(query))
      throw new Error('Public EPG query must be an object');
    const permitted = window
      ? ['backendId', 'channelId', 'fromTime', 'untilTime', 'limit', 'cursor']
      : ['backendId', 'channelId', 'fromTime', 'limit'];
    Object.keys(query).forEach(function (field) {
      if (!permitted.includes(field))
        throw new Error('unsupported Public EPG query field: ' + field);
    });
    const backend = f3CanonicalId(query.backendId, 'EPG backendId', 128);
    const channel = f3CanonicalId(query.channelId, 'EPG channelId', 256);
    const from = f3UnixSeconds(query.fromTime, 'EPG fromTime');
    const fields = [
      'backendId=' + encodeURIComponent(backend),
      'channelId=' + encodeURIComponent(channel),
      'fromTime=' + from
    ];
    if (window) {
      const until = f3UnixSeconds(query.untilTime, 'EPG untilTime');
      if (Number(until) <= Number(from))
        throw new Error('EPG untilTime must follow fromTime');
      fields.push('untilTime=' + until);
    }
    if (query.limit !== undefined) {
      const maximum = window ? 192 : 2;
      if (!Number.isInteger(query.limit) ||
          query.limit < 1 || query.limit > maximum)
        throw new Error('EPG limit must be an integer from 1 to ' + maximum);
      fields.push('limit=' + query.limit);
    }
    if (window && query.cursor !== undefined) {
      if (typeof query.cursor !== 'string' ||
          query.cursor.length < 5 || query.cursor.length > 4200)
        throw new Error('EPG cursor must be a non-empty opaque cursor');
      fields.push('cursor=' + encodeURIComponent(query.cursor));
    }
    return '?' + fields.join('&');
  }

  function f3SearchQuery(query) {
    if (!query || typeof query !== 'object' || Array.isArray(query))
      throw new Error('Public search query must be an object');
    const allowed = ['backendId', 'query', 'limit', 'offset'];
    Object.keys(query).forEach(function (key) {
      if (!allowed.includes(key))
        throw new Error('unsupported Public search field: ' + key);
    });
    const backend = f3CanonicalId(query.backendId, 'Search backendId', 128);
    if (typeof query.query !== 'string' ||
        query.query.length < 2 || query.query.length > 240)
      throw new Error('Search text must contain 2..240 characters');
    const params = [
      'backendId=' + encodeURIComponent(backend),
      'query=' + encodeURIComponent(query.query)
    ];
    if (query.limit !== undefined) {
      if (!Number.isInteger(query.limit) ||
          query.limit < 1 || query.limit > 30)
        throw new Error('Public search limit must be 1..30');
      params.push('limit=' + query.limit);
    }
    if (query.offset !== undefined) {
      if (!Number.isInteger(query.offset) ||
          query.offset < 0 || query.offset > 1000000)
        throw new Error('Public search offset must be 0..1000000');
      params.push('offset=' + query.offset);
    }
    return '?' + params.join('&');
  }

  function f3AdjacentChannelQuery(query) {
    if (!query || typeof query !== 'object' || Array.isArray(query))
      throw new Error('Adjacent Channel query must be an object');
    Object.keys(query).forEach(function (field) {
      if (!['backendId', 'relativeTo', 'direction'].includes(field))
        throw new Error('unsupported adjacent Channel field: ' + field);
    });
    const backend = f3CanonicalId(query.backendId, 'Channel backendId', 128);
    const current = f3CanonicalId(query.relativeTo, 'Current channelId', 256);
    if (query.direction !== 'up' && query.direction !== 'down')
      throw new Error('Adjacent Channel direction must be up or down');
    return '?backendId=' + encodeURIComponent(backend) +
      '&limit=1&relativeTo=' + encodeURIComponent(current) +
      '&direction=' + query.direction;
  }

  function recordingCollectionQuery(query) {
    if (!query || typeof query !== 'object' || Array.isArray(query)) {
      throw new Error('Recording collection query must be an object');
    }
    const allowed = new Set(['backendId', 'limit', 'cursor', 'sort', 'order']);
    Object.keys(query).forEach(function (key) {
      if (!allowed.has(key)) {
        throw new Error('unsupported Recording query field: ' + key);
      }
    });
    if (typeof query.backendId !== 'string'
        || !/^[A-Za-z0-9._-]{1,128}$/.test(query.backendId)) {
      throw new Error('Recording collection requires canonical backendId');
    }
    const params = new URLSearchParams();
    params.set('backendId', query.backendId);
    if (query.limit !== undefined) {
      if (!Number.isInteger(query.limit) || query.limit < 1 || query.limit > 100) {
        throw new Error('Recording collection limit must be 1..100');
      }
      params.set('limit', String(query.limit));
    }
    if (query.cursor !== undefined) {
      if (typeof query.cursor !== 'string' || !/^rc1_[a-f0-9]+$/.test(query.cursor)) {
        throw new Error('Recording cursor is invalid');
      }
      params.set('cursor', query.cursor);
    }
    if (query.sort !== undefined) {
      if (query.sort !== 'recordingId') throw new Error('Recording sort must be recordingId');
      params.set('sort', query.sort);
    }
    if (query.order !== undefined) {
      if (query.order !== 'asc') throw new Error('Recording order must be asc');
      params.set('order', query.order);
    }
    return '?' + params.toString();
  }


  // F4: Public Recording IDs are not native VDR paths.
  function f4Id(value, label, pattern) {
    if (typeof value !== 'string' || !pattern.test(value))
      throw new Error(label + ' is invalid');
    return value;
  }

  function f4RecordingDetailPath(input) {
    const id = f4Id(input.recordingId, 'recordingId', /^rec_[0-9a-f]{32}$/);
    const backend = f4Id(input.backendId, 'backendId', /^[A-Za-z0-9._-]{1,128}$/);
    return '/api/v1/recordings/' + id + '?backendId=' + encodeURIComponent(backend);
  }

  // Public-v1 native Recording editing shares the authoritative VDR
  // Recording identity and the existing native Cut/Marks core.
  function f6RecordingEditPath(input, suffix) {
    if (!input || typeof input !== 'object' || Array.isArray(input))
      throw new Error('Recording edit options must be an object');
    if (suffix !== 'marks' && suffix !== 'cut')
      throw new Error('Unsupported native Recording editing resource');
    const id = f4Id(input.recordingId, 'recordingId', /^rec_[0-9a-f]{32}$/);
    const backend = f4Id(input.backendId, 'backendId', /^[A-Za-z0-9._-]{1,128}$/);
    if (backend === '.' || backend === '..')
      throw new Error('Invalid Recording edit backend');
    return '/api/v1/recordings/' + id + '/' + suffix +
      '?backendId=' + encodeURIComponent(backend);
  }

  function f4PagedQuery(input, kind) {
    if (!input || typeof input !== 'object' || Array.isArray(input))
      throw new Error('Public Recording query must be an object');
    const folder = kind === 'folders', membership = kind === 'membership';
    const allowed = folder ? ['backendId', 'folderId', 'limit', 'offset']
      : membership ? ['backendId', 'genreId', 'locale', 'limit', 'offset']
      : ['backendId', 'locale', 'limit', 'offset'];
    Object.keys(input).forEach(function (name) {
      if (!allowed.includes(name))
        throw new Error('Unsupported Recording query field: ' + name);
    });
    const backend = f4Id(input.backendId, 'backendId', /^[A-Za-z0-9._-]{1,128}$/);
    const values = ['backendId=' + encodeURIComponent(backend)];
    if (folder) {
      values.push('view=folders');
      if (input.folderId !== undefined) {
        const id = f4Id(input.folderId, 'folderId', /^fld1_[0-9a-f]{32}$/);
        values.push('folderId=' + encodeURIComponent(id));
      }
    } else {
      if (membership) {
        const id = f4Id(input.genreId, 'genreId', /^[A-Za-z0-9._-]{1,128}$/);
        values.push('genreId=' + encodeURIComponent(id));
      }
      if (input.locale !== undefined) {
        if (input.locale !== 'de' && input.locale !== 'en')
          throw new Error('Genre locale must be de or en');
        values.push('locale=' + input.locale);
      }
    }
    if (input.limit !== undefined) {
      const max = folder ? 100 : 30;
      if (!Number.isInteger(input.limit) || input.limit < 1 || input.limit > max)
        throw new Error('Recording/Genre limit is invalid');
      values.push('limit=' + input.limit);
    }
    if (input.offset !== undefined) {
      if (!Number.isInteger(input.offset) ||
          input.offset < 0 || input.offset > 1000000)
        throw new Error('Recording/Genre offset is invalid');
      values.push('offset=' + input.offset);
    }
    return '?' + values.join('&');
  }
  function recordingArtworkPath(options, binary) {
    if (!options || typeof options !== 'object' ||
        typeof options.recordingId !== 'string' ||
        !/^rec_[0-9a-f]{32}$/.test(options.recordingId) ||
        typeof options.backendId !== 'string' ||
        !/^[A-Za-z0-9._-]{1,128}$/.test(options.backendId))
      throw new Error('Artwork requires public rec_ ID and exact backendId');
    if (binary && (typeof options.artworkId !== 'string' ||
        !/^[0-9a-f]{32}$/.test(options.artworkId)))
      throw new Error('Artwork requires opaque 32-hex artworkId');
    return '/api/v1/recordings/' + options.recordingId + '/artwork' +
      (binary ? '/' + options.artworkId : '') +
      '?backendId=' + encodeURIComponent(options.backendId);
  }

  function epgArtworkPath(options, preferred) {
    if (!options || typeof options !== 'object' ||
        typeof options.backendId !== 'string' ||
        !/^[A-Za-z0-9._-]{1,128}$/.test(options.backendId) ||
        typeof options.channelId !== 'string' ||
        !/^[A-Za-z0-9._:-]{1,256}$/.test(options.channelId) ||
        typeof options.eventId !== 'string' ||
        !/^[A-Za-z0-9._:-]{1,256}$/.test(options.eventId))
      throw new Error('EPG artwork requires valid backend/channel/event IDs');
    const p = new URLSearchParams();
    p.set('backendId', options.backendId);
    p.set('channelId', options.channelId);
    p.set('eventId', options.eventId);
    return '/api/v1/epg/' + (preferred ? 'preferred-artwork' : 'artwork') +
      '?' + p.toString();
  }

  // A distinct post-Phase69 VDR SearchTimer collection. Never send
  // native IDs or transfer TimerAssignment semantics to SearchTimers.
  function searchTimerCollectionQuery(query) {
    if (!query || typeof query !== 'object' || Array.isArray(query))
      throw new Error('SearchTimer query must be an object');
    const allowed = new Set(['backendId','limit','cursor']);
    Object.keys(query).forEach(function (key) {
      if (!allowed.has(key)) throw new Error('unsupported SearchTimer field: ' + key);
    });
    if (typeof query.backendId !== 'string' ||
        !/^[A-Za-z0-9._-]{1,128}$/.test(query.backendId))
      throw new Error('SearchTimer backendId must be canonical and scoped');
    const params = new URLSearchParams();
    params.set('backend', query.backendId);
    if (query.limit !== undefined) {
      if (!Number.isInteger(query.limit) || query.limit < 1 || query.limit > 30)
        throw new Error('SearchTimer limit must be an integer from 1 to 30');
      params.set('limit', String(query.limit));
    }
    if (query.cursor !== undefined) {
      if (typeof query.cursor !== 'string' ||
          !/^st1_[0-9a-f]{2,508}$/.test(query.cursor))
        throw new Error('SearchTimer cursor must be an opaque st1_ cursor');
      params.set('cursor', query.cursor);
    }
    return '?' + params.toString();
  }

  // EPG text matches are a preview, never a recording forecast or a write.
  function searchTimerDraftPreviewQuery(query) {
    if (!query || typeof query !== 'object' || Array.isArray(query))
      throw new Error('SearchTimer draft preview query must be an object');
    const allowed = new Set(['backendId','query','mode','tolerance',
      'compareTitle','compareSubtitle','compareSummary','limit']);
    Object.keys(query).forEach(function (key) {
      if (!allowed.has(key)) throw new Error('unsupported draft preview field: ' + key);
    });
    if (typeof query.backendId !== 'string' ||
        !/^[A-Za-z0-9._-]{1,128}$/.test(query.backendId) ||
        typeof query.query !== 'string' ||
        query.query.length < 2 || query.query.length > 240 ||
        ![0,1,2,3,5].includes(query.mode) ||
        typeof query.compareTitle !== 'boolean' ||
        typeof query.compareSubtitle !== 'boolean' ||
        typeof query.compareSummary !== 'boolean' ||
        !(query.compareTitle || query.compareSubtitle || query.compareSummary))
      throw new Error('invalid SearchTimer draft preview');
    const tolerance = query.tolerance === undefined ? 0 : query.tolerance;
    if (!Number.isInteger(tolerance) || tolerance < 0 || tolerance > 3 ||
        (query.mode !== 5 && tolerance !== 0))
      throw new Error('invalid SearchTimer draft tolerance');
    if (query.limit !== undefined &&
        (!Number.isInteger(query.limit) || query.limit < 1 || query.limit > 30))
      throw new Error('invalid SearchTimer preview limit');
    const params = new URLSearchParams();
    params.set('backend',query.backendId);
    params.set('query',query.query);
    params.set('mode',String(query.mode));
    params.set('compareTitle',query.compareTitle ? '1' : '0');
    params.set('compareSubtitle',query.compareSubtitle ? '1' : '0');
    params.set('compareSummary',query.compareSummary ? '1' : '0');
    if (tolerance !== 0) params.set('tolerance',String(tolerance));
    if (query.limit !== undefined) params.set('limit',String(query.limit));
    // Preserve spaces as percent-encoded bytes, never form-urlencoded '+'.
    return '?' + params.toString().replace(/\+/g, '%20');
  }

  function searchTimerDetailPath(options) {
    if (!options || typeof options !== 'object' || Array.isArray(options))
      throw new Error('SearchTimer detail options must be an object');
    if (typeof options.searchTimerId !== 'string' ||
        !/^st_[0-9a-f]{32}$/.test(options.searchTimerId))
      throw new Error('SearchTimer ID must be opaque st_ followed by 32 hex digits');
    if (typeof options.backendId !== 'string' ||
        !/^[A-Za-z0-9._-]{1,128}$/.test(options.backendId))
      throw new Error('SearchTimer backendId must be canonical');
    return '/api/v1/search-timers/' + options.searchTimerId +
      '?backend=' + encodeURIComponent(options.backendId);
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



  function accountItemPath(options) {
    if (!options || typeof options !== 'object' || Array.isArray(options)) {
      throw new Error('Account item options must be an object');
    }
    if (typeof options.accountId !== 'string' || options.accountId === '') {
      throw new Error('Account item accountId must be a non-empty string');
    }
    if (/[\/?#]/.test(options.accountId)) {
      throw new Error('Account item accountId contains a path delimiter');
    }
    return '/api/v1/accounts/' + options.accountId;
  }


  function deviceLifecyclePath(options, credential) {
    if (!options || typeof options.deviceId !== 'string' ||
        !/^[A-Za-z0-9_-]{1,128}$/.test(options.deviceId)) {
      throw new Error('deviceId must be a bounded canonical identifier');
    }
    const base = '/api/v1/devices/' + options.deviceId;
    if (!credential) return base + '/lifecycle';
    if (typeof options.credentialId !== 'string' ||
        !/^[A-Za-z0-9_-]{1,128}$/.test(options.credentialId)) {
      throw new Error('credentialId must be a bounded canonical identifier');
    }
    return base + '/credentials/' + options.credentialId + '/lifecycle';
  }

  function deviceGrantPath(options) {
    if (!options || typeof options.deviceId !== 'string'
        || !/^[A-Za-z0-9_-]{1,128}$/.test(options.deviceId)) {
      throw new Error('deviceId must be a bounded canonical identifier');
    }
    return '/api/v1/devices/' + options.deviceId + '/grants';
  }

  function accountGrantPath(options) {
    return accountItemPath(options) + '/grants';
  }

  function accountCredentialPath(options) {
    return accountItemPath(options) + '/credentials';
  }

  function accountSessionPath(options) {
    return accountItemPath(options) + '/sessions';
  }

  function accountCredentialItemPath(options) {
    if (!options || typeof options !== 'object' || Array.isArray(options)) {
      throw new Error('Account Credential item options must be an object');
    }
    if (typeof options.credentialId !== 'string' || options.credentialId === '') {
      throw new Error('Account Credential item credentialId must be a non-empty string');
    }
    if (/[\/?#]/.test(options.credentialId)) {
      throw new Error('Account Credential item credentialId contains a path delimiter');
    }
    return accountCredentialPath(options) + '/' + options.credentialId;
  }

  function accountSessionItemPath(options) {
    if (!options || typeof options !== 'object' || Array.isArray(options)) {
      throw new Error('Account Session item options must be an object');
    }
    if (typeof options.sessionId !== 'string' || options.sessionId === '') {
      throw new Error('Account Session item sessionId must be a non-empty string');
    }
    if (/[\/?#]/.test(options.sessionId)) {
      throw new Error('Account Session item sessionId contains a path delimiter');
    }
    return accountSessionPath(options) + '/' + options.sessionId;
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



  // Global administrator view: pending, unexpired pairings only.
  function devicePairingAdministrationQuery(query) {
    const input = query && typeof query === 'object' && !Array.isArray(query)
      ? query : {};
    const allowed = new Set(['limit', 'cursor', 'sort', 'order']);
    Object.keys(input).forEach(function (key) {
      if (!allowed.has(key)) throw new Error('unsupported pairing query field: ' + key);
    });
    const params = new URLSearchParams();
    if (input.limit !== undefined) {
      if (!Number.isInteger(input.limit) || input.limit < 1 || input.limit > 100)
        throw new Error('pairing limit must be 1–100');
      params.set('limit', String(input.limit));
    }
    if (input.cursor !== undefined) {
      if (typeof input.cursor !== 'string' ||
          !/^dp1_[A-Za-z0-9_-]{1,1024}$/.test(input.cursor))
        throw new Error('invalid opaque pairing cursor');
      params.set('cursor', input.cursor);
    }
    if (input.sort !== undefined) {
      if (input.sort !== 'pairingRequestId') throw new Error('invalid pairing sort');
      params.set('sort', input.sort);
    }
    if (input.order !== undefined) {
      if (input.order !== 'asc') throw new Error('invalid pairing order');
      params.set('order', input.order);
    }
    const encoded = params.toString();
    return encoded ? '?' + encoded : '';
  }

  function devicePairingItemPath(options) {
    if (!options || typeof options !== 'object' || Array.isArray(options)) {
      throw new Error('Device Pairing options must be an object');
    }
    if (typeof options.pairingRequestId !== 'string'
        || options.pairingRequestId === '') {
      throw new Error('pairingRequestId must be a non-empty string');
    }
    if (/[\/?#]/.test(options.pairingRequestId)) {
      throw new Error('pairingRequestId contains a path delimiter');
    }
    return '/api/v1/device-pairings/' + options.pairingRequestId;
  }


  function mediaImportPath(options, suffix) {
    if (!options || typeof options !== 'object' ||
        typeof options.backendId !== 'string' ||
        !/^[A-Za-z0-9._-]{1,128}$/.test(options.backendId)) {
      throw new Error('Media Import requires canonical backendId');
    }
    return '/api/v1/backends/' + options.backendId + '/' + suffix;
  }

  function operationItemPath(options) {
    if (!options || typeof options !== 'object' || Array.isArray(options)) {
      throw new Error('Operation item options must be an object');
    }
    if (typeof options.operationId !== 'string' || options.operationId === '') {
      throw new Error('Operation item operationId must be a non-empty string');
    }
    if (/[\/?#]/.test(options.operationId)) {
      throw new Error('Operation item operationId contains a path delimiter');
    }
    return '/api/v1/operations/' + options.operationId;
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


    function requestRecordingEditMutation(options, suffix) {
      const o = options && typeof options === 'object' ? options : {};
      const path = f6RecordingEditPath(o, suffix);
      const body = o.payload;
      if (!body || typeof body !== 'object' || Array.isArray(body) ||
          body.backendId !== o.backendId ||
          body.recordingId !== o.recordingId ||
          typeof body.operationId !== 'string' ||
          !/^[A-Za-z0-9_.:-]{1,192}$/.test(body.operationId) ||
          typeof body.operationRevision !== 'string' ||
          !/^[A-Za-z0-9_.:-]{1,192}$/.test(body.operationRevision) ||
          typeof body.expectedMarksRevision !== 'string' ||
          !/^[0-9a-f]{32}$/.test(body.expectedMarksRevision)) {
        throw new Error('Recording edit requires matching backend/Recording ID and native operation revision');
      }
      // Single user-intended POST; transport never retries an uncertain cut.
      const headers = Object.assign(
        {Accept: 'application/json'},
        defaultHeaders,
        copyHeaders(o.headers),
        {'Content-Type': 'application/json'}
      );
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST', headers,
        credentials: o.credentials !== undefined
          ? o.credentials : normalized.credentials,
        cache: 'no-store', signal: o.signal,
        body: JSON.stringify(body)
      }).then(function (response) {
        return parseJsonBody(response).then(function (payload) {
          if (!response.ok)
            throw new VdrSuitePublicClientError(path, response.status, payload, response);
          return payload;
        });
      });
    }

    function requestArtworkImage(path, options) {
      const input = options && typeof options === 'object' ? options : {};
      const headers = Object.assign(
        {Accept: 'image/png, image/jpeg, image/webp'},
        defaultHeaders, copyHeaders(input.headers));
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'GET', headers: headers,
        credentials: input.credentials !== undefined
          ? input.credentials : normalized.credentials,
        cache: input.cache, signal: input.signal
      }).then(function (response) {
        if (!response.ok) {
          return parseJsonBody(response).then(function (payload) {
            throw new VdrSuitePublicClientError(path, response.status, payload, response);
          });
        }
        const type = headerValue(response, 'Content-Type');
        if (!['image/png', 'image/jpeg', 'image/webp'].includes(type))
          throw new Error('Unexpected artwork Content-Type');
        return response.arrayBuffer().then(function (bytes) {
          return {contentType: type, bytes: bytes};
        });
      });
    }

    // Public-v1 single metadata identity resolver for Web, TV and other clients.
    // Pure read despite POST: no assignment, import or native writer action.
    function requestMetadataResolve(options) {
      const o = options && typeof options === 'object' ? options : {};
      const path = mediaImportPath(o, 'metadata/resolve');
      if (!['series', 'movie'].includes(o.kind) ||
          typeof o.title !== 'string' || o.title.length < 2 ||
          o.title.length > 160)
        throw new Error('Metadata resolve requires kind and title');
      if (o.kind === 'series' &&
          (!Number.isInteger(o.seasonNumber) || o.seasonNumber < 1 ||
           !Number.isInteger(o.episodeNumber) || o.episodeNumber < 1))
        throw new Error('Metadata episode resolve requires season and episode');
      const payload = {kind: o.kind, title: o.title};
      if (o.kind === 'series') {
        payload.seasonNumber = o.seasonNumber;
        payload.episodeNumber = o.episodeNumber;
        if (typeof o.episodeTitle === 'string' && o.episodeTitle.length)
          payload.episodeTitle = o.episodeTitle;
      }
      if (o.year !== undefined) {
        if (!Number.isInteger(o.year) || o.year < 1900 || o.year > 9999)
          throw new Error('Metadata release year is invalid');
        payload.year = o.year;
      }
      const headers = Object.assign({Accept:'application/json'},
        defaultHeaders, copyHeaders(o.headers),
        {'Content-Type': 'application/json'});
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST', headers,
        credentials: o.credentials !== undefined
          ? o.credentials : normalized.credentials,
        signal: o.signal, body: JSON.stringify(payload)
      }).then(function (response) {
        return parseJsonBody(response).then(function (data) {
          if (!response.ok)
            throw new VdrSuitePublicClientError(path,
              response.status, data, response);
          return data;
        });
      });
    }

    function requestMediaImport(options) {
      const requestOptions = options && typeof options === 'object' ? options : {};
      const path = mediaImportPath(requestOptions, 'media-imports');
      if (typeof requestOptions.sourceId !== 'string' ||
          !/^[A-Za-z0-9_-]{16,64}$/.test(requestOptions.sourceId)) {
        throw new Error('Media Import requires opaque sourceId');
      }
      if (typeof requestOptions.idempotencyKey !== 'string' ||
          !/^[!-~]{1,160}$/.test(requestOptions.idempotencyKey)) {
        throw new Error('Media Import requires valid Idempotency-Key');
      }
      if (requestOptions.videoDecision !== undefined &&
          !['local', 'desktop', 'skip', 'release', 'duplicate', 'delete_duplicate'].includes(requestOptions.videoDecision)) {
        throw new Error('Media Import requires an explicit supported video decision');
      }
      if (requestOptions.metadata !== undefined &&
          (typeof requestOptions.metadata !== 'string' ||
           requestOptions.metadata.length > 4096 ||
           !/^v(?:1|2)\|(episode|movie)\|/.test(requestOptions.metadata)))
        throw new Error('Invalid Media Import metadata snapshot');
      const headers = Object.assign(
        {Accept: 'application/json'},
        defaultHeaders,
        copyHeaders(requestOptions.headers),
        {'Content-Type': 'application/json',
          'Idempotency-Key': requestOptions.idempotencyKey}
      );
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST',
        headers: headers,
        credentials: requestOptions.credentials !== undefined
          ? requestOptions.credentials : normalized.credentials,
        cache: 'no-store',
        signal: requestOptions.signal,
        body: JSON.stringify(Object.assign(
          {sourceId: requestOptions.sourceId},
          requestOptions.videoDecision
            ? {videoDecision: requestOptions.videoDecision} : {},
          requestOptions.metadata
            ? {metadata: requestOptions.metadata} : {}))
      }).then(function (response) {
        const location = headerValue(response, 'Location');
        const entityTag = headerValue(response, 'ETag');
        return parseJsonBody(response).then(function (payload) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(path, response.status, payload, response);
          }
          return {status: response.status, location: location,
            etag: entityTag, data: payload};
        });
      });
    }



    function requestMediaImportBatch(options) {
      const value = options && typeof options === 'object' ? options : {};
      const path = mediaImportPath(value, 'media-imports');
      if (typeof value.idempotencyKey !== 'string' ||
          !/^mb1_[0-9a-f]{32}$/.test(value.idempotencyKey) ||
          !Array.isArray(value.batch) ||
          value.batch.length < 2 || value.batch.length > 100 ||
          value.batch.some(function (item) {
            return !item || typeof item.sourceId !== 'string' ||
              !/^[A-Za-z0-9_-]{16,64}$/.test(item.sourceId) ||
              (item.metadata !== undefined && item.metadata !== '' &&
               (typeof item.metadata !== 'string' ||
                item.metadata.length > 4096 ||
                !/^v(?:1|2)\|(episode|movie)\|/.test(item.metadata)));
          }))
        throw new Error('Media Import batch requires confirmed opaque sources');
      const headers = Object.assign(
        {Accept: 'application/json'}, defaultHeaders,
        copyHeaders(value.headers),
        {'Content-Type': 'application/json',
         'Idempotency-Key': value.idempotencyKey});
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST', headers: headers,
        credentials: value.credentials !== undefined
          ? value.credentials : normalized.credentials,
        cache: 'no-store',
        body: JSON.stringify({batch: value.batch.map(function (item) {
          return Object.assign({sourceId: item.sourceId},
            item.metadata ? {metadata: item.metadata} : {});
        })})
      }).then(function (response) {
        const location = headerValue(response, 'Location');
        return parseJsonBody(response).then(function (payload) {
          if (!response.ok)
            throw new VdrSuitePublicClientError(path, response.status, payload, response);
          return {status: response.status, location: location, data: payload};
        });
      });
    }

    function requestMediaShrinkPlan(options) {
      const value = options && typeof options === 'object' ? options : {};
      const path = mediaImportPath(value, 'media-shrink-plans');
      if (typeof value.recordingId !== 'string' ||
          !/^rec_[0-9a-f]{32}$/.test(value.recordingId)) {
        throw new Error('HEVC plan requires canonical recordingId');
      }
      const headers = Object.assign(
        {Accept: 'application/json'}, defaultHeaders, copyHeaders(value.headers),
        {'Content-Type': 'application/json'}
      );
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST', headers: headers,
        credentials: value.credentials !== undefined
          ? value.credentials : normalized.credentials,
        cache: 'no-store', signal: value.signal,
        body: JSON.stringify({recordingId: value.recordingId})
      }).then(function (response) {
        return parseJsonBody(response).then(function (payload) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(
              path, response.status, payload, response);
          }
          return payload;
        });
      });
    }

    // Stage and Publish are distinct, explicitly idempotent Core mutations.
    // Browser requests never carry a native recording/candidate path.
    function requestMediaShrinkMutation(options, action) {
      const value = options && typeof options === 'object' ? options : {};
      if (typeof value.idempotencyKey !== 'string' ||
          !/^[A-Za-z0-9_.:-]{8,128}$/.test(value.idempotencyKey)) {
        throw new Error('HEVC mutation requires a stable Idempotency-Key');
      }
      let payload;
      let suffix;
      if (action === 'stage') {
        if (typeof value.recordingId !== 'string' ||
            !/^rec_[0-9a-f]{32}$/.test(value.recordingId) ||
            !['software', 'vaapi', 'qsv', 'nvenc'].includes(value.encoder) ||
            !Number.isInteger(value.maxHeight) ||
            (value.maxHeight !== 0 &&
              (value.maxHeight < 480 || value.maxHeight > 2160)) ||
            typeof value.nightMode !== 'boolean') {
          throw new Error('Invalid HEVC stage selection');
        }
        payload = {recordingId: value.recordingId, encoder: value.encoder,
          maxHeight: value.maxHeight, nightMode: value.nightMode};
        if (value.autoPublish === true) payload.autoPublish = true;
        suffix = 'media-shrink-stages';
      } else if (action === 'publish') {
        if (typeof value.reviewId !== 'string' ||
            !/^shk_[0-9a-f]{32}$/.test(value.reviewId) ||
            value.revision !== 1) {
          throw new Error('HEVC publish requires reviewed candidate revision 1');
        }
        payload = {reviewId: value.reviewId, revision: 1};
        suffix = 'media-shrink-publications';
      } else {
        throw new Error('Unknown HEVC mutation');
      }
      const path = mediaImportPath(value, suffix);
      const headers = Object.assign(
        {Accept: 'application/json'}, defaultHeaders, copyHeaders(value.headers),
        {'Content-Type': 'application/json',
          'Idempotency-Key': value.idempotencyKey}
      );
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST', headers: headers,
        credentials: value.credentials !== undefined
          ? value.credentials : normalized.credentials,
        cache: 'no-store', signal: value.signal,
        body: JSON.stringify(payload)
      }).then(function (response) {
        return parseJsonBody(response).then(function (body) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(
              path, response.status, body, response);
          }
          return body;
        });
      });
    }

    function requestMediaCheck(options) {
      const value = options && typeof options === 'object' ? options : {};
      const path = mediaImportPath(value, 'media-checks');
      if (typeof value.recordingId !== 'string' ||
          !/^rec_[0-9a-f]{32}$/.test(value.recordingId)) {
        throw new Error('Media Check requires canonical recordingId');
      }
      if (value.mode !== 'structure' && value.mode !== 'decode') {
        throw new Error('Media Check mode must be structure or decode');
      }
      const headers = Object.assign(
        {Accept: 'application/json'}, defaultHeaders, copyHeaders(value.headers),
        {'Content-Type': 'application/json'}
      );
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST', headers: headers,
        credentials: value.credentials !== undefined
          ? value.credentials : normalized.credentials,
        cache: 'no-store', signal: value.signal,
        body: JSON.stringify({recordingId: value.recordingId, mode: value.mode})
      }).then(function (response) {
        return parseJsonBody(response).then(function (payload) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(path, response.status, payload, response);
          }
          return payload;
        });
      });
    }

    function requestDevicePairingCreate(payload, options) {
      const requestOptions = options && typeof options === 'object' ? options : {};
      const headers = Object.assign(
        {Accept: 'application/json'},
        defaultHeaders,
        copyHeaders(requestOptions.headers)
      );
      headers['Content-Type'] = 'application/json';

      return fetchImpl(buildUrl(baseUrl, '/api/v1/device-pairings'), {
        method: 'POST',
        headers: headers,
        credentials: requestOptions.credentials !== undefined
          ? requestOptions.credentials
          : normalized.credentials,
        signal: requestOptions.signal,
        body: JSON.stringify(payload)
      }).then(function (response) {
        const location = headerValue(response, 'Location');
        return parseJsonBody(response).then(function (responsePayload) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(
              '/api/v1/device-pairings',
              response.status,
              responsePayload,
              response
            );
          }
          return {
            status: response.status,
            location: location,
            data: responsePayload
          };
        });
      });
    }


    function requestDeviceCredentialIssue(options) {
      const path = devicePairingItemPath(options) + '/credential';
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST',
        headers: {
          Accept: 'application/json',
          'X-VDR-Suite-Pairing-Token': options.pairingToken
        },
        credentials: 'omit',
        cache: 'no-store',
        signal: options.signal
      }).then(function (response) {
        return parseJsonBody(response).then(function (payload) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(
              path, response.status, payload, response
            );
          }
          return {
            status: response.status,
            data: payload
          };
        });
      });
    }

    function nativeSearchTimerPath(options, suffix) {
      const o = options && typeof options === 'object' ? options : {};
      if (typeof o.backendId !== 'string' ||
          !/^[A-Za-z0-9._-]{1,128}$/.test(o.backendId) ||
          typeof o.searchTimerId !== 'string' ||
          !/^st_[0-9a-f]{32}$/.test(o.searchTimerId))
        throw new Error('canonical public SearchTimer ID and backendId required');
      return '/api/v1/search-timers/' + o.searchTimerId + '/' + suffix +
        '?backend=' + encodeURIComponent(o.backendId);
    }

    function nativeSearchTimerMutation(options) {
      const o = options && typeof options === 'object' ? options : {};
      const booleans = ['matchCase','compareTitle','compareSubtitle','compareSummary','useDayOfWeek','useSeriesRecording','useVps','useExtendedEpgInfo','avoidRepeats','repeatCompareTitle','repeatCompareSummary','useInFavorites','unmuteSoundOnSwitch','repeatCompareSubtitle','compareTime','useTime','useDuration','compareCategories','ignoreMissingEpgCategories'];
      const integers = ['mode','dayOfWeek','priority','lifetime','marginStartMinutes','marginStopMinutes','searchTimerAction','allowedRepeats','repeatsWithinDays','deleteRecordingsAfterDays','keepRecordings','switchMinutesBefore','pauseOnRecordings','tolerance','deleteMode','deleteAfterCountRecordings','deleteAfterDaysOfFirstRecording','summaryMatch','repeatCompareSubtitleMode','repeatCompareTimeMode','useChannel','startTime','stopTime','durationMinMinutes','durationMaxMinutes','blacklistMode','activeMode'];
      const strings = ['directory','channels','channelMin','channelMax','blacklistIds','extendedEpgInfo','contentDescriptors','activeFrom','activeUntil','repeatCompareCategoriesMask'];
      const settingFields = [...booleans,...integers,...strings];
      const allowed = new Set(['backendId','searchTimerId','action','query',
        'confirmed','previewAcknowledged','ifMatch','idempotencyKey',
        'headers','credentials','signal',...settingFields]);
      Object.keys(o).forEach(function (key) {
        if (!allowed.has(key))
          throw new Error('unsupported native SearchTimer mutation option: ' + key);
      });
      const path = nativeSearchTimerPath(o, 'mutations');
      if (!['update','activate','deactivate','delete'].includes(o.action) ||
          o.confirmed !== true ||
          typeof o.ifMatch !== 'string' ||
          !/^"sr_[0-9a-f]{32}"$/.test(o.ifMatch) ||
          typeof o.idempotencyKey !== 'string' ||
          !/^[A-Za-z0-9._:-]{1,160}$/.test(o.idempotencyKey))
        throw new Error('guarded SearchTimer mutation confirmation, revision and idempotency key required');
      if (o.action === 'activate' && o.previewAcknowledged !== true)
        throw new Error('activation requires explicit fresh EPG preview acknowledgement');
      const suppliedSettings = settingFields.filter(key => o[key] !== undefined);
      if (o.action === 'update') {
        if (o.query !== undefined && (typeof o.query !== 'string' || o.query.length === 0 ||
            o.query.length > 200 || /[\x00-\x1f\x7f]/.test(o.query)))
          throw new Error('invalid SearchTimer update query');
        if (o.query === undefined && !suppliedSettings.length)
          throw new Error('SearchTimer update needs a query or settings');
        for (const key of suppliedSettings) {
          if ((booleans.includes(key) && typeof o[key] !== 'boolean') ||
              (integers.includes(key) && (!Number.isInteger(o[key]) || o[key] < -2147483648 || o[key] > 2147483647)) ||
              (strings.includes(key) && typeof o[key] !== 'string'))
            throw new Error('invalid SearchTimer update setting: ' + key);
        }
      } else if (o.query !== undefined || suppliedSettings.length) {
        throw new Error('query and settings are allowed only for update');
      }
      const headers = Object.assign({Accept:'application/json'},
        defaultHeaders,copyHeaders(o.headers));
      headers['Content-Type'] = 'application/json';
      headers['If-Match'] = o.ifMatch;
      headers['Idempotency-Key'] = o.idempotencyKey;
      const payload = {action:o.action,confirmed:true};
      if (o.action === 'update') {
        if (o.query !== undefined) payload.query = o.query;
        suppliedSettings.forEach(key => { payload[key] = o[key]; });
      }
      return fetchImpl(buildUrl(baseUrl,path), {
        method:'POST',headers,
        credentials:o.credentials !== undefined ? o.credentials : normalized.credentials,
        signal:o.signal,body:JSON.stringify(payload)
      }).then(function (response) {
        const location = headerValue(response,'Location');
        return parseJsonBody(response).then(function (data) {
          if (!response.ok) throw new VdrSuitePublicClientError(path,response.status,data,response);
          return {status:response.status,location,data};
        });
      });
    }

    // Full provider vocabulary, shared by Samsung/VIDAA/Browser consumers.
    // There is only one native CREATE route: POST /api/v1/search-timers.
    function requestSearchTimerCreate(options) {
      const o = options && typeof options === 'object' ? options : {};
      const allowed = new Set([
        'name',
        'query',
        'active',
        'directory',
        'priority',
        'lifetime',
        'marginStartMinutes',
        'marginStopMinutes',
        'useVps',
        'useChannel',
        'channels',
        'channelMin',
        'channelMax',
        'useTime',
        'startTime',
        'stopTime',
        'useDuration',
        'durationMinMinutes',
        'durationMaxMinutes',
        'useDayOfWeek',
        'dayOfWeek',
        'avoidRepeats',
        'allowedRepeats',
        'repeatsWithinDays',
        'compareTitle',
        'compareSubtitle',
        'compareSummary',
        'repeatCompareTitle',
        'repeatCompareSubtitle',
        'repeatCompareSummary',
        'compareCategories',
        'compareTime',
        'useSeriesRecording',
        'keepRecordings',
        'deleteMode',
        'searchTimerAction',
        'blacklistMode',
        'blacklistIds',
        'matchCase',
        'summaryMatch',
        'useExtendedEpgInfo',
        'extendedEpgInfo',
        'ignoreMissingEpgCategories',
        'contentDescriptors',
        'useInFavorites',
        'activeFrom',
        'activeUntil',
        'pauseOnRecordings',
        'switchMinutesBefore',
        'unmuteSoundOnSwitch',
        'deleteRecordingsAfterDays',
        'deleteAfterCountRecordings',
        'deleteAfterDaysOfFirstRecording',
        'mode',
        'tolerance',
        'confirmed',
        'backendId',
        'idempotencyKey',
        'headers',
        'credentials',
        'signal'
      ]);
      Object.keys(o).forEach(function (key) {
        if (!allowed.has(key)) throw new Error('unsupported SearchTimer field: ' + key);
      });
      if (typeof o.backendId !== 'string' ||
          !/^[A-Za-z0-9._-]{1,128}$/.test(o.backendId) ||
          typeof o.name !== 'string' || o.name !== o.query ||
          o.name.length < 2 || o.name.length > 240 ||
          typeof o.active !== 'boolean' || o.confirmed !== true ||
          !Number.isInteger(o.mode) || o.mode < 0 || o.mode > 5 ||
          typeof o.compareTitle !== 'boolean' ||
          typeof o.compareSubtitle !== 'boolean' ||
          typeof o.compareSummary !== 'boolean' ||
          !(o.compareTitle || o.compareSubtitle || o.compareSummary) ||
          typeof o.idempotencyKey !== 'string' ||
          !/^[A-Za-z0-9._:-]{1,160}$/.test(o.idempotencyKey))
        throw new Error('SearchTimer CREATE requires validated scope, settings, confirmation and idempotency key');
      const tolerance = o.tolerance === undefined ? 0 : o.tolerance;
      if (!Number.isInteger(tolerance) || tolerance < 0 ||
          tolerance > 3 || (o.mode !== 5 && tolerance !== 0))
        throw new Error('invalid SearchTimer tolerance');
      const payload = {name:o.name,query:o.query,active:o.active,
        confirmed:true,mode:o.mode,tolerance:tolerance,
        compareTitle:o.compareTitle,compareSubtitle:o.compareSubtitle,
        compareSummary:o.compareSummary};
      const optional = [
        'directory',
        'priority',
        'lifetime',
        'marginStartMinutes',
        'marginStopMinutes',
        'useVps',
        'useChannel',
        'channels',
        'channelMin',
        'channelMax',
        'useTime',
        'startTime',
        'stopTime',
        'useDuration',
        'durationMinMinutes',
        'durationMaxMinutes',
        'useDayOfWeek',
        'dayOfWeek',
        'avoidRepeats',
        'allowedRepeats',
        'repeatsWithinDays',
        'repeatCompareTitle',
        'repeatCompareSubtitle',
        'repeatCompareSummary',
        'compareCategories',
        'compareTime',
        'useSeriesRecording',
        'keepRecordings',
        'deleteMode',
        'searchTimerAction',
        'blacklistMode',
        'blacklistIds',
        'matchCase',
        'summaryMatch',
        'useExtendedEpgInfo',
        'extendedEpgInfo',
        'ignoreMissingEpgCategories',
        'contentDescriptors',
        'useInFavorites',
        'activeFrom',
        'activeUntil',
        'pauseOnRecordings',
        'switchMinutesBefore',
        'unmuteSoundOnSwitch',
        'deleteRecordingsAfterDays',
        'deleteAfterCountRecordings',
        'deleteAfterDaysOfFirstRecording'
      ];
      optional.forEach(function (key) {
        if (o[key] !== undefined) payload[key] = o[key];
      });
      const strings = new Set(['name','query','directory','channels','channelMin','channelMax','blacklistIds','extendedEpgInfo','contentDescriptors','activeFrom','activeUntil']);
      const booleans = new Set(['active','useVps','useTime','useDuration','useDayOfWeek','avoidRepeats','compareTitle','compareSubtitle','compareSummary','repeatCompareTitle','repeatCompareSubtitle','repeatCompareSummary','compareCategories','compareTime','useSeriesRecording','matchCase','useExtendedEpgInfo','ignoreMissingEpgCategories','useInFavorites','pauseOnRecordings','unmuteSoundOnSwitch','confirmed']);
      const integers = new Set(['priority','lifetime','marginStartMinutes','marginStopMinutes','useChannel','startTime','stopTime','durationMinMinutes','durationMaxMinutes','dayOfWeek','allowedRepeats','repeatsWithinDays','keepRecordings','deleteMode','searchTimerAction','blacklistMode','summaryMatch','switchMinutesBefore','deleteRecordingsAfterDays','deleteAfterCountRecordings','deleteAfterDaysOfFirstRecording','mode','tolerance']);
      Object.keys(payload).forEach(function (key) {
        const value = payload[key];
        if (strings.has(key) &&
            (typeof value !== 'string' || value.length > (key === 'name' ||
                key === 'query' ? 240 : 4096)))
          throw new Error('invalid SearchTimer text option: ' + key);
        if (booleans.has(key) && typeof value !== 'boolean')
          throw new Error('invalid SearchTimer boolean option: ' + key);
        if (integers.has(key) && (!Number.isInteger(value) ||
            value < -2147483648 || value > 2147483647))
          throw new Error('invalid SearchTimer integer option: ' + key);
      });
      if (payload.useChannel !== undefined &&
          ![0,1,2,3].includes(payload.useChannel))
        throw new Error('invalid SearchTimer channel filter');
      if (payload.useTime === true &&
          ['startTime','stopTime'].some(function (key) {
            const t = payload[key];
            return !Number.isInteger(t) || t < 0 || t > 2359 || t % 100 >= 60;
          }))
        throw new Error('invalid SearchTimer HHMM time range');
      const path = '/api/v1/search-timers?backend=' +
        encodeURIComponent(o.backendId);
      const headers = Object.assign({Accept:'application/json'},
        defaultHeaders,copyHeaders(o.headers));
      if (headers['If-Match'] !== undefined ||
          headers['if-match'] !== undefined)
        throw new Error('SearchTimer CREATE cannot have If-Match');
      headers['Content-Type'] = 'application/json';
      headers['Idempotency-Key'] = o.idempotencyKey;
      return fetchImpl(buildUrl(baseUrl,path), {
        method:'POST',headers,
        credentials:o.credentials !== undefined ? o.credentials :
          normalized.credentials,
        signal:o.signal,
        body:JSON.stringify(payload)
      }).then(function (response) {
        const location = headerValue(response,'Location');
        return parseJsonBody(response).then(function (data) {
          if (!response.ok)
            throw new VdrSuitePublicClientError(path,response.status,data,response);
          return {status:response.status,location,data};
        });
      });
    }

    // Backward-compatible convenience wrapper; no separate writer or route.
    function requestInactiveSearchTimerCreate(options) {
      const o = options && typeof options === 'object' ? options : {};
      const allowed = new Set(['backendId','name','query','active','mode',
        'tolerance','compareTitle','compareSubtitle','compareSummary',
        'idempotencyKey','previewAcknowledged','confirmed','headers',
        'credentials','signal']);
      Object.keys(o).forEach(function (key) {
        if (!allowed.has(key))
          throw new Error('unsupported SearchTimer staging field: ' + key);
      });
      if (o.active !== false || o.previewAcknowledged !== true ||
          o.confirmed !== true || ![0,1,2,3,5].includes(o.mode))
        throw new Error('Legacy staging wrapper requires inactive, preview acknowledgement and confirmation');
      const staged = Object.assign({},o);
      delete staged.previewAcknowledged;
      return requestSearchTimerCreate(staged);
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



    function requestAccountCreate(payload, options) {
      const requestOptions = options && typeof options === 'object' ? options : {};
      if (typeof requestOptions.idempotencyKey !== 'string'
          || requestOptions.idempotencyKey === '') {
        throw new Error('idempotencyKey must be a non-empty caller-owned key');
      }

      const headers = Object.assign(
        {Accept: 'application/json'},
        defaultHeaders,
        copyHeaders(requestOptions.headers)
      );
      headers['Content-Type'] = 'application/json';
      headers['Idempotency-Key'] = requestOptions.idempotencyKey;

      return fetchImpl(buildUrl(baseUrl, '/api/v1/accounts'), {
        method: 'POST',
        headers: headers,
        credentials: requestOptions.credentials !== undefined
          ? requestOptions.credentials
          : normalized.credentials,
        signal: requestOptions.signal,
        body: JSON.stringify(payload)
      }).then(function (response) {
        const entityTag = headerValue(response, 'ETag');
        const location = headerValue(response, 'Location');
        return parseJsonBody(response).then(function (responsePayload) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(
              '/api/v1/accounts',
              response.status,
              responsePayload,
              response
            );
          }
          return {
            status: response.status,
            location: location,
            etag: entityTag,
            data: responsePayload
          };
        });
      });
    }


    function requestAccountMutation(path, payload, options) {
      const requestOptions = options && typeof options === 'object' ? options : {};
      if (typeof requestOptions.ifMatch !== 'string' || requestOptions.ifMatch === '') {
        throw new Error('ifMatch must be a non-empty opaque ETag');
      }

      const headers = Object.assign(
        {Accept: 'application/json'},
        defaultHeaders,
        copyHeaders(requestOptions.headers)
      );
      headers['Content-Type'] = 'application/json';
      headers['If-Match'] = requestOptions.ifMatch;

      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST',
        headers: headers,
        credentials: requestOptions.credentials !== undefined
          ? requestOptions.credentials
          : normalized.credentials,
        signal: requestOptions.signal,
        body: JSON.stringify(payload)
      }).then(function (response) {
        const entityTag = headerValue(response, 'ETag');
        return parseJsonBody(response).then(function (responsePayload) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(
              path,
              response.status,
              responsePayload,
              response
            );
          }
          return {
            status: response.status,
            etag: entityTag,
            data: responsePayload
          };
        });
      });
    }


    function requestTimerCreate(path, options) {
      const requestOptions = options && typeof options === 'object' ? options : {};
      if (typeof requestOptions.ifMatch !== 'string' || requestOptions.ifMatch === '') {
        throw new Error('ifMatch must be a non-empty opaque ETag');
      }
      if (typeof requestOptions.idempotencyKey !== 'string'
          || requestOptions.idempotencyKey === '') {
        throw new Error('idempotencyKey must be a non-empty caller-owned key');
      }

      const headers = Object.assign(
        {Accept: 'application/json'},
        defaultHeaders,
        copyHeaders(requestOptions.headers)
      );
      headers['Content-Type'] = 'application/json';
      headers['If-Match'] = requestOptions.ifMatch;
      headers['Idempotency-Key'] = requestOptions.idempotencyKey;

      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST',
        headers: headers,
        credentials: requestOptions.credentials !== undefined
          ? requestOptions.credentials
          : normalized.credentials,
        signal: requestOptions.signal,
        body: '{}'
      }).then(function (response) {
        const entityTag = headerValue(response, 'ETag');
        const location = headerValue(response, 'Location');
        return parseJsonBody(response).then(function (payload) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(path, response.status, payload, response);
          }
          return {
            status: response.status,
            location: location,
            etag: entityTag,
            data: payload
          };
        });
      });
    }



    // F5.5.5: Submit one explicit manual native TimerIntent. This is an
    // Operation resource, NOT a synchronous Timer-creation confirmation.
    // Preserve the caller's key on a retry; never replay a native write here.
    function submitManualTimerIntent(input) {
      const o = input && typeof input === 'object' ? input : {};
      const p = o.payload;
      const path = '/api/v1/manual/timer-intents';
      const fields = ['backendId', 'channelId', 'title', 'directory',
        'startAt', 'stopAt', 'priority', 'lifetime', 'enabled', 'vps'];
      if (!p || typeof p !== 'object' || Array.isArray(p) ||
          Object.keys(p).length !== fields.length ||
          Object.keys(p).some(key => !fields.includes(key)) ||
          fields.some(key => !Object.prototype.hasOwnProperty.call(p, key)) ||
          typeof p.backendId !== 'string' ||
          !/^[A-Za-z0-9._-]{1,128}$/.test(p.backendId) ||
          typeof p.channelId !== 'string' ||
          !/^[A-Za-z0-9._:-]{1,160}$/.test(p.channelId) ||
          typeof p.title !== 'string' || p.title.length < 1 ||
          p.title.length > 1024 ||
          typeof p.directory !== 'string' || p.directory.length > 1024 ||
          !Number.isSafeInteger(p.startAt) || p.startAt <= 0 ||
          !Number.isSafeInteger(p.stopAt) ||
          p.stopAt <= p.startAt || p.stopAt - p.startAt >= 86400 ||
          !Number.isInteger(p.priority) || p.priority < 0 || p.priority > 99 ||
          !Number.isInteger(p.lifetime) || p.lifetime < 0 || p.lifetime > 99 ||
          typeof p.enabled !== 'boolean' || typeof p.vps !== 'boolean' ||
          typeof o.idempotencyKey !== 'string' ||
          !/^[!-~]{1,160}$/.test(o.idempotencyKey))
        throw new Error('Manual TimerIntent requires exact one-shot payload and Idempotency-Key');

      const headers = Object.assign({Accept: 'application/json'},
        defaultHeaders, copyHeaders(o.headers));
      if (headers['If-Match'] !== undefined ||
          headers['if-match'] !== undefined)
        throw new Error('Manual TimerIntent CREATE has no EPG If-Match');
      headers['Content-Type'] = 'application/json';
      headers['Idempotency-Key'] = o.idempotencyKey;
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST', headers,
        credentials: o.credentials !== undefined
          ? o.credentials : normalized.credentials,
        cache: 'no-store', signal: o.signal,
        body: JSON.stringify(p)
      }).then(response => parseJsonBody(response).then(data => {
        if (!response.ok)
          throw new VdrSuitePublicClientError(path, response.status, data, response);
        // HTTP 202 acknowledges only the durable Operation, not a Timer.
        return {status: response.status, data,
          location: headerValue(response, 'Location')};
      }));
    }

    // Native timers are current VDR snapshot resources, not TimerAssignments.
    // Keep their backendId/offset paging distinct from SearchTimer/Assignment
    // cursors and preserve the server's conditional ETag semantics on detail.
    function nativeTimerListQuery(query) {
      if (!query || typeof query !== 'object' || Array.isArray(query) ||
          typeof query.backendId !== 'string' ||
          !/^[A-Za-z0-9._-]{1,128}$/.test(query.backendId))
        throw new Error('Native Timer backendId required');
      const allowed = new Set(['backendId', 'offset', 'limit']);
      Object.keys(query).forEach(key => {
        if (!allowed.has(key))
          throw new Error('Unsupported native Timer query: ' + key);
      });
      const params = new URLSearchParams();
      params.set('backendId', query.backendId);
      if (query.offset !== undefined) {
        if (!Number.isInteger(query.offset) ||
            query.offset < 0 || query.offset > 100000)
          throw new Error('Native Timer offset must be 0..100000');
        params.set('offset', String(query.offset));
      }
      if (query.limit !== undefined) {
        if (!Number.isInteger(query.limit) ||
            query.limit < 1 || query.limit > 30)
          throw new Error('Native Timer limit must be 1..30');
        params.set('limit', String(query.limit));
      }
      return '?' + params.toString();
    }

    function nativeTimerItemPath(options, suffix) {
      if (!options || typeof options !== 'object' ||
          typeof options.backendId !== 'string' ||
          !/^[A-Za-z0-9._-]{1,128}$/.test(options.backendId) ||
          typeof options.timerId !== 'string' ||
          !/^[A-Za-z0-9._:-]{1,160}$/.test(options.timerId))
        throw new Error('Canonical native Timer ID and backendId required');
      return '/api/v1/native-timers/' + options.timerId +
        (suffix || '') + '?backendId=' + encodeURIComponent(options.backendId);
    }

    function submitNativeTimerMutation(input, method) {
      const options = input && typeof input === 'object' ? input : {};
      const path = nativeTimerItemPath(options,
        method === 'POST' ? '/modifications' : '');
      if (typeof options.ifMatch !== 'string' || options.ifMatch.length === 0 ||
          typeof options.idempotencyKey !== 'string' ||
          options.idempotencyKey.length === 0)
        throw new Error('Native Timer mutation requires If-Match and Idempotency-Key');
      if (method === 'POST' &&
          (!options.payload || typeof options.payload !== 'object' ||
           Array.isArray(options.payload)))
        throw new Error('Native Timer modification requires caller-owned JSON payload');
      // Uncertain writes are never dispatched a second time.
      const headers = Object.assign({Accept: 'application/json'}, defaultHeaders,
        copyHeaders(options.headers), {
          'If-Match': options.ifMatch,
          'Idempotency-Key': options.idempotencyKey
        });
      const requestOptions = {
        method, headers,
        credentials: options.credentials !== undefined
          ? options.credentials : normalized.credentials,
        signal: options.signal
      };
      if (method === 'POST') {
        headers['Content-Type'] = 'application/json';
        requestOptions.body = JSON.stringify(options.payload);
      }
      return fetchImpl(buildUrl(baseUrl, path), requestOptions).then(
        response => parseJsonBody(response).then(data => {
          if (!response.ok)
            throw new VdrSuitePublicClientError(path,response.status,data,response);
          return {status: response.status, data,
            location: headerValue(response,'Location')};
        })
      );
    }

    function playbackAccountAdminPath(input) {
      if (!input || typeof input.deviceId !== 'string' ||
          !/^[A-Za-z0-9._-]{1,128}$/.test(input.deviceId))
        throw new Error('Exact Device ID required');
      return '/api/v1/devices/' + input.deviceId + '/playback-account';
    }

    function setPlaybackAccountForDevice(input) {
      const path = playbackAccountAdminPath(input);
      if (typeof input.accountId !== 'string' ||
          !/^[A-Za-z0-9._-]{0,128}$/.test(input.accountId) ||
          !Number.isSafeInteger(input.expectedRevision) ||
          input.expectedRevision < 0)
        throw new Error('Explicit accountId and current binding revision required');
      const headers = Object.assign({Accept: 'application/json'}, defaultHeaders,
        copyHeaders(input.headers), {'Content-Type':'application/json'});
      return fetchImpl(buildUrl(baseUrl, path), {
        method:'POST', headers,
        credentials:input.credentials !== undefined ? input.credentials : normalized.credentials,
        signal:input.signal,
        body:JSON.stringify({accountId:input.accountId,
          expectedRevision:input.expectedRevision})
      }).then(response => parseJsonBody(response).then(data => {
        if (!response.ok)
          throw new VdrSuitePublicClientError(path,response.status,data,response);
        return data;
      }));
    }

    function recordingPlaybackSessionId(input) {
      if (!input || typeof input !== 'object' ||
          typeof input.sessionId !== 'string' ||
          !/^ms_[0-9a-f]{32}$/.test(input.sessionId))
        throw new Error('Canonical Device recording sessionId required');
      return input.sessionId;
    }

    function startDeviceRecordingPlayback(input) {
      const identity = recordingProgressIdentity(input);
      const capabilities = input.capabilities;
      const protocols = new Set(['hls', 'progressive']);
      const containers = new Set(['mpeg-ts', 'fmp4', 'mp4']);
      const codecs = new Set(['h264','h265','hevc','mpeg2video','aac',
        'ac3','eac3','mpeg-audio','mp2','mp3']);
      const supported = (name, allowed, nonempty) =>
        Array.isArray(capabilities[name]) &&
        (!nonempty || capabilities[name].length > 0) &&
        capabilities[name].every(x => typeof x === 'string' && allowed.has(x));
      if (!capabilities || typeof capabilities !== 'object' ||
          !supported('protocols', protocols, true) ||
          !supported('containers', containers, true) ||
          !supported('videoCodecs', codecs, false) ||
          !supported('audioCodecs', codecs, false) ||
          typeof capabilities.supportsByteRanges !== 'boolean' ||
          !Number.isInteger(capabilities.maxVideoWidth) ||
          capabilities.maxVideoWidth < 0 || capabilities.maxVideoWidth > 16384 ||
          !Number.isInteger(capabilities.maxVideoHeight) ||
          capabilities.maxVideoHeight < 0 || capabilities.maxVideoHeight > 16384 ||
          !Number.isInteger(capabilities.maxAudioChannels) ||
          capabilities.maxAudioChannels < 0 || capabilities.maxAudioChannels > 32)
        throw new Error('Explicit, measured Device codec and protocol capabilities required');
      const body = {...identity, capabilities};
      if (input.startPositionSeconds !== undefined) {
        if (!Number.isInteger(input.startPositionSeconds) ||
            input.startPositionSeconds < 0 ||
            input.startPositionSeconds > 2147483647)
          throw new Error('Invalid index-backed startPositionSeconds');
        body.startPositionSeconds = input.startPositionSeconds;
      }
      if (input.capabilityProfile !== undefined) {
        if (!input.capabilityProfile ||
            typeof input.capabilityProfile !== 'object' ||
            Array.isArray(input.capabilityProfile))
          throw new Error('Invalid verified Device capability profile');
        body.capabilityProfile = input.capabilityProfile;
      }
      // Both options are already supported by the existing Device MediaSession
      // owner; neither enables a second media path or changes the defaults.
      if (input.audioTrackId !== undefined) {
        if (typeof input.audioTrackId !== 'string' ||
            !/^audio-[1-9][0-9]{0,9}$/.test(input.audioTrackId))
          throw new Error('Invalid canonical audioTrackId');
        body.audioTrackId = input.audioTrackId;
      }
      if (input.nativeMediaAccess !== undefined) {
        if (typeof input.nativeMediaAccess !== 'boolean')
          throw new Error('nativeMediaAccess must be a boolean');
        body.nativeMediaAccess = input.nativeMediaAccess;
      }
      const path = '/api/v1/recording-playback-sessions';
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST',
        headers: Object.assign({Accept: 'application/json'}, defaultHeaders,
          copyHeaders(input.headers), {'Content-Type':'application/json'}),
        credentials: input.credentials !== undefined ? input.credentials : normalized.credentials,
        signal: input.signal, body: JSON.stringify(body)
      }).then(response => parseJsonBody(response).then(data => {
        if (!response.ok)
          throw new VdrSuitePublicClientError(path, response.status, data, response);
        return data;
      }));
    }

    function deviceSessionRead(input) {
      const sessionId = recordingPlaybackSessionId(input);
      return request('/api/v1/recording-playback-sessions/' + sessionId, input);
    }

    function deviceSessionStop(input) {
      const sessionId = recordingPlaybackSessionId(input);
      if (typeof input.backendId !== 'string' ||
          !/^[A-Za-z0-9._-]{1,128}$/.test(input.backendId))
        throw new Error('Exact backendId required');
      const path = '/api/v1/recording-playback-sessions/' + sessionId + '/stop';
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST',
        headers: Object.assign({Accept: 'application/json'}, defaultHeaders,
          copyHeaders(input.headers), {'Content-Type':'application/json'}),
        credentials: input.credentials !== undefined ? input.credentials : normalized.credentials,
        signal: input.signal,
        body: JSON.stringify({operation:'stop',backendId:input.backendId,sessionId})
      }).then(response => parseJsonBody(response).then(data => {
        if (!response.ok)
          throw new VdrSuitePublicClientError(path, response.status, data, response);
        return data;
      }));
    }

    function recordingProgressIdentity(input) {
      if (!input || typeof input !== 'object' ||
          typeof input.backendId !== 'string' ||
          !/^[A-Za-z0-9._-]{1,128}$/.test(input.backendId) ||
          input.backendId === '.' || input.backendId === '..' ||
          typeof input.recordingId !== 'string' ||
          !/^rec_[0-9a-f]{32}$/.test(input.recordingId))
        throw new Error('Public recording progress requires backendId and canonical rec_ ID');
      return {backendId: input.backendId, recordingId: input.recordingId};
    }

    function listDeviceContinueWatching(input) {
      if (!input || typeof input.backendId !== 'string' ||
          !/^[A-Za-z0-9._-]{1,128}$/.test(input.backendId) ||
          input.backendId === '.' || input.backendId === '..')
        throw new Error('Continue Watching requires exact backendId');
      return request('/api/v1/recording-playback-sessions/continue-watching?backendId=' +
        encodeURIComponent(input.backendId), input);
    }

    function readRecordingProgress(input) {
      const identity = recordingProgressIdentity(input);
      return request('/api/v1/recording-playback-sessions/progress?backendId=' +
        encodeURIComponent(identity.backendId) + '&recordingId=' +
        encodeURIComponent(identity.recordingId), input);
    }

    function saveRecordingProgress(input) {
      const identity = recordingProgressIdentity(input);
      if (!Number.isSafeInteger(input.expectedRevision) ||
          input.expectedRevision < 0 ||
          !Number.isInteger(input.positionSeconds) ||
          input.positionSeconds < 0 || input.positionSeconds > 2147483647 ||
          typeof input.operationId !== 'string' ||
          !/^[!-~]{1,128}$/.test(input.operationId) ||
          (input.positionSeconds > 0 &&
            (typeof input.sessionId !== 'string' ||
             !/^[!-~]{1,128}$/.test(input.sessionId))))
        throw new Error('Confirmed position, current revision and operationId required');
      const path = '/api/v1/recording-playback-sessions/progress';
      const headers = Object.assign({Accept: 'application/json'}, defaultHeaders,
        copyHeaders(input.headers), {'Content-Type': 'application/json'});
      const payload = Object.assign({}, identity, {
        positionSeconds: input.positionSeconds,
        expectedRevision: input.expectedRevision,
        operationId: input.operationId
      });
      if (input.sessionId !== undefined) payload.sessionId = input.sessionId;
      // No automatic resubmission: on 409 re-read the current revision and let the
      // client decide whether it really observed a newer playback position.
      return fetchImpl(buildUrl(baseUrl, path), {
        method: 'POST', headers: headers,
        credentials: input.credentials !== undefined ? input.credentials : normalized.credentials,
        signal: input.signal,
        body: JSON.stringify(payload)
      }).then(function (response) {
        return parseJsonBody(response).then(function (data) {
          if (!response.ok) {
            throw new VdrSuitePublicClientError(path, response.status, data, response);
          }
          return data;
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
      createDevicePairing(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        if (typeof normalizedOptions.displayName !== 'string'
            || normalizedOptions.displayName === '') {
          throw new Error('displayName must be a non-empty string');
        }
        if (typeof normalizedOptions.clientKind !== 'string'
            || normalizedOptions.clientKind === '') {
          throw new Error('clientKind must be a non-empty string');
        }
        if (normalizedOptions.appVersion !== undefined
            && typeof normalizedOptions.appVersion !== 'string') {
          throw new Error('appVersion must be a string when provided');
        }

        const payload = {
          displayName: normalizedOptions.displayName,
          clientKind: normalizedOptions.clientKind
        };
        if (normalizedOptions.appVersion !== undefined) {
          payload.appVersion = normalizedOptions.appVersion;
        }

        return requestDevicePairingCreate(payload, normalizedOptions);
      },
      listDevicePairingsForAdmin(options) {
        const input = options && typeof options === 'object' ? options : {};
        return request('/api/v1/device-pairings' +
          devicePairingAdministrationQuery(input.query), input);
      },
      getDevicePairingForAdmin(options) {
        const input = options && typeof options === 'object' ? options : {};
        if (input.pairingToken !== undefined)
          throw new Error('administrator pairing read must not carry a bootstrap token');
        return requestRevisioned(devicePairingItemPath(input), input);
      },
      decideDevicePairingForAdmin(options) {
        const input = options && typeof options === 'object' ? options : {};
        if (input.pairingToken !== undefined ||
            (input.decision !== 'approve' && input.decision !== 'reject'))
          throw new Error('invalid administrator pairing decision');
        return requestAccountMutation(devicePairingItemPath(input),
          {decision: input.decision}, input);
      },
      getDevicePairing(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        if (typeof normalizedOptions.pairingToken !== 'string'
            || normalizedOptions.pairingToken === '') {
          throw new Error('pairingToken must be a non-empty string');
        }

        const requestOptions = Object.assign({}, normalizedOptions, {
          headers: Object.assign(
            {},
            copyHeaders(normalizedOptions.headers),
            {'X-VDR-Suite-Pairing-Token': normalizedOptions.pairingToken}
          )
        });

        return request(
          devicePairingItemPath(normalizedOptions),
          requestOptions
        );
      },
      issueDeviceCredential(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        if (typeof normalizedOptions.pairingToken !== 'string'
            || normalizedOptions.pairingToken === '') {
          throw new Error('pairingToken must be a non-empty string');
        }
        // A lost response is terminal. Callers must not automatically repeat issuance.
        return requestDeviceCredentialIssue(normalizedOptions);
      },
      getBackends(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request('/api/v1/backends' + backendQuery(normalizedOptions.query), normalizedOptions);
      },
      getDevices(options) {
        const input = options && typeof options === 'object' ? options : {};
        return request('/api/v1/devices' +
          deviceCollectionQuery(input.query), input);
      },
      getAccounts(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request('/api/v1/accounts' + accountQuery(normalizedOptions.query), normalizedOptions);
      },
      getAccount(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestRevisioned(
          accountItemPath(normalizedOptions),
          normalizedOptions
        );
      },
      createAccount(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        if (typeof normalizedOptions.loginName !== 'string'
            || normalizedOptions.loginName === '') {
          throw new Error('loginName must be a non-empty string');
        }
        if (typeof normalizedOptions.displayName !== 'string'
            || normalizedOptions.displayName === '') {
          throw new Error('displayName must be a non-empty string');
        }
        if (typeof normalizedOptions.password !== 'string'
            || normalizedOptions.password === '') {
          throw new Error('password must be a non-empty string');
        }
        return requestAccountCreate(
          {
            loginName: normalizedOptions.loginName,
            displayName: normalizedOptions.displayName,
            password: normalizedOptions.password
          },
          normalizedOptions
        );
      },
      getDeviceLifecycle(options) {
        const input = options && typeof options === 'object' ? options : {};
        return requestRevisioned(deviceLifecyclePath(input, false), input);
      },
      revokeDevice(options) {
        const input = options && typeof options === 'object' ? options : {};
        return requestAccountMutation(deviceLifecyclePath(input, false), {}, input);
      },
      getDeviceCredentialLifecycle(options) {
        const input = options && typeof options === 'object' ? options : {};
        return requestRevisioned(deviceLifecyclePath(input, true), input);
      },
      revokeDeviceCredential(options) {
        const input = options && typeof options === 'object' ? options : {};
        return requestAccountMutation(deviceLifecyclePath(input, true), {}, input);
      },
      rotateDeviceCredential(options) {
        const input = options && typeof options === 'object' ? options : {};
        const lifecycle = deviceLifecyclePath(input, true);
        return requestAccountMutation(
          lifecycle.slice(0, -'/lifecycle'.length) + '/rotate', {}, input);
      },
      getDeviceGrants(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestRevisioned(deviceGrantPath(normalizedOptions), normalizedOptions);
      },
      setDeviceGrant(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        if (typeof normalizedOptions.permission !== 'string' ||
            normalizedOptions.permission === '' ||
            typeof normalizedOptions.backendId !== 'string' ||
            normalizedOptions.backendId === '' ||
            typeof normalizedOptions.active !== 'boolean') {
          throw new Error('Device Grant permission, backendId and active are required');
        }
        // Mirror DeviceGrantAdministrationService's exact-backend fence
        // before issuing HTTP, while preserving wildcard read grants.
        if ([
          'recordings.edit',
          'timers.create', 'timers.modify', 'timers.delete',
          'searchtimers.create', 'searchtimers.modify',
          'searchtimers.execute', 'searchtimers.delete'
        ].includes(normalizedOptions.permission) &&
            (['*', '.', '..'].includes(normalizedOptions.backendId) ||
              !/^[A-Za-z0-9_.-]{1,128}$/.test(normalizedOptions.backendId))) {
          throw new Error('Device Grant requires a concrete backendId');
        }
        return requestAccountMutation(
          deviceGrantPath(normalizedOptions),
          {permission: normalizedOptions.permission,
           backendId: normalizedOptions.backendId,
           active: normalizedOptions.active},
          normalizedOptions);
      },
      getAccountGrants(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestRevisioned(
          accountGrantPath(normalizedOptions),
          normalizedOptions
        );
      },
      getAccountCredentials(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request(accountCredentialPath(normalizedOptions), normalizedOptions);
      },
      getAccountCredential(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestRevisioned(
          accountCredentialItemPath(normalizedOptions),
          normalizedOptions
        );
      },
      revokeAccountCredential(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestAccountMutation(
          accountCredentialItemPath(normalizedOptions),
          {},
          normalizedOptions
        );
      },
      getAccountSessions(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request(accountSessionPath(normalizedOptions), normalizedOptions);
      },
      getAccountSession(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestRevisioned(
          accountSessionItemPath(normalizedOptions),
          normalizedOptions
        );
      },
      revokeAccountSession(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestAccountMutation(
          accountSessionItemPath(normalizedOptions),
          {},
          normalizedOptions
        );
      },
      setAccountGrant(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        if (typeof normalizedOptions.permission !== 'string'
            || normalizedOptions.permission === '') {
          throw new Error('permission must be a non-empty string');
        }
        if (typeof normalizedOptions.backendId !== 'string'
            || normalizedOptions.backendId === '') {
          throw new Error('backendId must be a non-empty string');
        }
        if (typeof normalizedOptions.active !== 'boolean') {
          throw new Error('active must be a boolean');
        }
        return requestAccountMutation(
          accountGrantPath(normalizedOptions),
          {
            permission: normalizedOptions.permission,
            backendId: normalizedOptions.backendId,
            active: normalizedOptions.active
          },
          normalizedOptions
        );
      },
      updateAccountDisplayName(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        if (typeof normalizedOptions.displayName !== 'string'
            || normalizedOptions.displayName === '') {
          throw new Error('displayName must be a non-empty string');
        }
        return requestAccountMutation(
          accountItemPath(normalizedOptions),
          {displayName: normalizedOptions.displayName},
          normalizedOptions
        );
      },
      activateAccount(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestAccountMutation(
          accountItemPath(normalizedOptions),
          {active: true},
          normalizedOptions
        );
      },
      deactivateAccount(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestAccountMutation(
          accountItemPath(normalizedOptions),
          {active: false},
          normalizedOptions
        );
      },
      getDevicePlaybackAccount(options) {
        return request(playbackAccountAdminPath(options), options);
      },
      setDevicePlaybackAccount(options) {
        return setPlaybackAccountForDevice(options);
      },
      getRecordingPlaybackCapabilities(options) {
        const input = options && typeof options === 'object' ? options : {};
        const keys = ['appVersion', 'firmwareVersion', 'playerVersion', 'engine'];
        if (!keys.every(key => typeof input[key] === 'string' && input[key].length > 0))
          throw new Error('Exact Device playback profile key required');
        const query = new URLSearchParams();
        keys.forEach(key => query.set(key, input[key]));
        return request('/api/v1/recording-playback-sessions/capabilities?' +
          query.toString(), input);
      },
      createRecordingPlaybackSession(options) {
        return startDeviceRecordingPlayback(options);
      },
      getRecordingPlaybackSession(options) {
        return deviceSessionRead(options);
      },
      getRecordingPlaybackTracks(options) {
        return request('/api/v1/recording-playback-sessions/' +
          recordingPlaybackSessionId(options) + '/tracks', options);
      },
      stopRecordingPlaybackSession(options) {
        return deviceSessionStop(options);
      },
      getContinueWatching(options) {
        return listDeviceContinueWatching(options);
      },
      getRecordingProgress(options) {
        return readRecordingProgress(options);
      },
      updateRecordingProgress(options) {
        return saveRecordingProgress(options);
      },
      clearRecordingProgress(options) {
        const input = options && typeof options === 'object' ? options : {};
        return saveRecordingProgress(Object.assign({}, input, {positionSeconds: 0}));
      },
      getRecordings(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request(
          '/api/v1/recordings' + recordingCollectionQuery(normalizedOptions.query),
          normalizedOptions
        );
      },

      getRecordingDetail(options) {
        const o = options && typeof options === 'object' ? options : {};
        return request(f4RecordingDetailPath(o), o);
      },
      getRecordingMarks(options) {
        const o = options && typeof options === 'object' ? options : {};
        return request(f6RecordingEditPath(o, 'marks'), o);
      },
      getRecordingCutState(options) {
        const o = options && typeof options === 'object' ? options : {};
        return request(f6RecordingEditPath(o, 'cut'), o);
      },
      submitRecordingMarksMutation(options) {
        return requestRecordingEditMutation(options, 'marks');
      },
      submitRecordingCut(options) {
        return requestRecordingEditMutation(options, 'cut');
      },
      getRecordingFolders(options) {
        const o = options && typeof options === 'object' ? options : {};
        return request('/api/v1/recordings' + f4PagedQuery(o.query, 'folders'), o);
      },
      getRecordingGenres(options) {
        const o = options && typeof options === 'object' ? options : {};
        return request('/api/v1/genres' + f4PagedQuery(o.query, 'genres'), o);
      },
      getGenreRecordings(options) {
        const o = options && typeof options === 'object' ? options : {};
        return request('/api/v1/genres/recordings' +
          f4PagedQuery(o.query, 'membership'), o);
      },
      getRecordingArtwork(options) {
        const input = options && typeof options === 'object' ? options : {};
        return request(recordingArtworkPath(input, false), input);
      },
      getRecordingArtworkImage(options) {
        const input = options && typeof options === 'object' ? options : {};
        return requestArtworkImage(recordingArtworkPath(input, true), input);
      },
      getEpgArtworkImage(options) {
        const input = options && typeof options === 'object' ? options : {};
        return requestArtworkImage(epgArtworkPath(input, false), input);
      },
      getEpgPreferredArtworkImage(options) {
        const input = options && typeof options === 'object' ? options : {};
        return requestArtworkImage(epgArtworkPath(input, true), input);
      },
      getChannels(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request('/api/v1/channels' + channelQuery(normalizedOptions.query), normalizedOptions);
      },
      // F3: explicit, backend-scoped Public-v1 reads; no Web route substitution.
      getAdjacentChannel(options) {
        const input = options && typeof options === 'object' ? options : {};
        return request('/api/v1/channels' + f3AdjacentChannelQuery(input.query), input);
      },
      getEpgWindow(options) {
        const input = options && typeof options === 'object' ? options : {};
        return request('/api/v1/epg/window' + f3EpgQuery(input.query, true), input);
      },
      getEpgNowNext(options) {
        const input = options && typeof options === 'object' ? options : {};
        return request('/api/v1/epg/now-next' + f3EpgQuery(input.query, false), input);
      },
      searchEpg(options) {
        const input = options && typeof options === 'object' ? options : {};
        return request('/api/v1/search/epg' + f3SearchQuery(input.query), input);
      },
      searchRecordings(options) {
        const input = options && typeof options === 'object' ? options : {};
        return request('/api/v1/search/recordings' + f3SearchQuery(input.query), input);
      },
      getSearchTimers(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request(
          '/api/v1/search-timers' + searchTimerCollectionQuery(normalizedOptions.query),
          normalizedOptions
        );
      },
      previewSearchTimerDraft(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request(
          '/api/v1/search-timers/draft-preview' +
          searchTimerDraftPreviewQuery(normalizedOptions.query),
          normalizedOptions
        );
      },
      createInactiveSearchTimer(options) {
        return requestInactiveSearchTimerCreate(options);
      },
      createSearchTimer(options) {
        return requestSearchTimerCreate(options);
      },
      getSearchTimerVersion(options) {
        const o = options && typeof options === 'object' ? options : {};
        return request(nativeSearchTimerPath(o,'version'), o);
      },
      mutateSearchTimer(options) {
        return nativeSearchTimerMutation(options);
      },
      getSearchTimer(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request(searchTimerDetailPath(normalizedOptions), normalizedOptions);
      },
      getNativeTimers(options) {
        const input = options && typeof options === 'object' ? options : {};
        return request('/api/v1/native-timers' +
          nativeTimerListQuery(input.query), input);
      },
      getNativeTimer(options) {
        const input = options && typeof options === 'object' ? options : {};
        return requestRevisioned(nativeTimerItemPath(input), input);
      },
      modifyNativeTimer(options) {
        return submitNativeTimerMutation(options, 'POST');
      },
      deleteNativeTimer(options) {
        return submitNativeTimerMutation(options, 'DELETE');
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
      },
      submitManualTimerIntent(options) {
        return submitManualTimerIntent(options);
      },
      submitTimerCreate(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestTimerCreate(
          timerAssignmentItemPath(normalizedOptions),
          normalizedOptions
        );
      },
      getMediaShrinkCapability(options) {
        const value = options && typeof options === 'object' ? options : {};
        return request(mediaImportPath(value, 'capabilities/media-shrink'), value);
      },
      // Read only: restores the last durable HEVC candidate for a recording
      // even when this browser has no sessionStorage operation reference.
      getMediaShrinkRecording(options) {
        const value = options && typeof options === 'object' ? options : {};
        if (typeof value.recordingId !== 'string' ||
            !/^rec_[0-9a-f]{32}$/.test(value.recordingId)) {
          throw new Error('HEVC recording requires canonical recordingId');
        }
        return request(mediaImportPath(value,
          'media-shrink-recordings/' + value.recordingId), value);
      },
      getMediaShrinkReview(options) {
        const value = options && typeof options === 'object' ? options : {};
        if (typeof value.reviewId !== 'string' ||
            !/^shk_[0-9a-f]{32}$/.test(value.reviewId)) {
          throw new Error('HEVC review requires opaque reviewId');
        }
        return request(mediaImportPath(value,
          'media-shrink-reviews/' + value.reviewId), value);
      },
      submitMediaShrinkPlan(options) {
        return requestMediaShrinkPlan(options);
      },
      submitMediaShrinkStage(options) {
        return requestMediaShrinkMutation(options, 'stage');
      },
      submitMediaShrinkPublish(options) {
        return requestMediaShrinkMutation(options, 'publish');
      },
      getMediaShrinkOperation(options) {
        const value = options && typeof options === 'object' ? options : {};
        if (typeof value.operationId !== 'string' ||
            !/^op_[0-9a-f]{32}$/.test(value.operationId)) {
          throw new Error('HEVC operation requires opaque operationId');
        }
        return request(mediaImportPath(value,
          'media-shrink-operations/' + value.operationId), value);
      },
      getMediaCheckCapability(options) {
        const value = options && typeof options === 'object' ? options : {};
        return request(mediaImportPath(value, 'capabilities/media-check'), value);
      },
      submitMediaCheck(options) {
        return requestMediaCheck(options);
      },
      getMediaCheckStatus(options) {
        const value = options && typeof options === 'object' ? options : {};
        if (typeof value.checkId !== 'string' ||
            !/^chk_[0-9a-f]{32}$/.test(value.checkId)) {
          throw new Error('Media Check requires opaque checkId');
        }
        return request(mediaImportPath(value, 'media-checks/' + value.checkId), value);
      },
      getMediaImportCapability(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return request(mediaImportPath(normalizedOptions,
          'capabilities/media-import'), normalizedOptions);
      },
      getRecentMediaImports(options) {
        const value = options && typeof options === 'object' ? options : {};
        return request(mediaImportPath(value, 'media-imports'), value);
      },
      getMediaImportSources(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        const cursor = normalizedOptions.cursor;
        if (cursor !== undefined && (typeof cursor !== 'string' ||
            !/^mi1_[0-9]{1,20}_[0-9]{1,3}$/.test(cursor))) {
          throw new Error('Media Import source cursor is invalid');
        }
        return request(mediaImportPath(normalizedOptions,
          'media-import-sources') +
          (cursor === undefined ? '' : '?cursor=' + encodeURIComponent(cursor)),
          normalizedOptions);
      },
      getMediaImportSourcePreview(options) {
        const value = options && typeof options === 'object' ? options : {};
        if (typeof value.sourceId !== 'string' ||
            !/^[A-Za-z0-9_-]{16,64}$/.test(value.sourceId)) {
          throw new Error('Media Import preview requires one opaque sourceId');
        }
        return request(mediaImportPath(value,
          'media-import-sources/' + value.sourceId + '/preview') +
          (value.refresh === true ? '?refresh=1' :
            value.cached === true ? '?cached=1' : ''), value);
      },
      resolveMetadata(options) {
        return requestMetadataResolve(options);
      },
      submitMediaImport(options) {
        return requestMediaImport(options);
      },
      submitMediaImportBatch(options) {
        return requestMediaImportBatch(options);
      },
      getOperation(options) {
        const normalizedOptions = options && typeof options === 'object' ? options : {};
        return requestRevisioned(
          operationItemPath(normalizedOptions),
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
