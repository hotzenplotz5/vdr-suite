(function (global) {
  'use strict';

  const base = global.VdrSuiteClientApi;
  if (!base || typeof base.requestJson !== 'function') {
    console.error('VDR-Suite base Client API is unavailable for genre routes');
    return;
  }

  function normalizeOptions(options) {
    return options && typeof options === 'object' ? options : {};
  }

  function withQuery(options, additions) {
    const normalized = normalizeOptions(options);
    return Object.assign({}, normalized, {
      query: Object.assign({}, normalized.query || {}, additions || {})
    });
  }

  function fetchClientGenres(options) {
    const normalized = normalizeOptions(options);
    return base.requestJson('/api/metadata/genres', withQuery(normalized, {
      backend: normalized.backendId,
      scope: normalized.scope,
      locale: normalized.locale,
      from: normalized.from,
      until: normalized.until
    }));
  }

  function fetchClientGenreRecordings(options) {
    const normalized = normalizeOptions(options);
    return base.requestJson('/api/metadata/genres/recordings', withQuery(normalized, {
      backend: normalized.backendId,
      genre: normalized.genreId,
      limit: normalized.limit,
      offset: normalized.offset
    }));
  }

  function fetchClientGenreEpg(options) {
    const normalized = normalizeOptions(options);
    return base.requestJson('/api/metadata/genres/epg', withQuery(normalized, {
      backend: normalized.backendId,
      contentClass: normalized.contentClass,
      genre: normalized.genreId,
      from: normalized.from,
      until: normalized.until,
      limit: normalized.limit,
      offset: normalized.offset
    }));
  }

  global.VdrSuiteClientApi = Object.freeze(Object.assign({}, base, {
    fetchClientGenres: fetchClientGenres,
    fetchClientGenreRecordings: fetchClientGenreRecordings,
    fetchClientGenreEpg: fetchClientGenreEpg
  }));
}(window));
