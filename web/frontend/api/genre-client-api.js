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

  // Preserve the existing Web overview shape and canonical display order.
  // Raw page objects retain the HTTP client's cache proof; the aggregate
  // object is not itself a proven HTTP response.
  async function fetchPublicRecordingGenres(options) {
    const input = normalizeOptions(options);
    const limit = 30;
    const locale = input.locale || 'de';
    const genres = [];
    const ids = new Set();
    const positions = new Set();
    let offset = 0;
    let totals;
    do {
      const payload = await base.requestJson('/api/v1/genres', {
        query: {backendId: input.backendId, locale, limit, offset},
        credentials: input.credentials || 'same-origin',
        cache: input.cache || 'no-store',
        signal: input.signal
      });
      const page = payload && payload.page;
      const nonnegative = value => Number.isSafeInteger(value) && value >= 0;
      if (!page || !Array.isArray(payload.items) || !payload.meta ||
          payload.meta.partial !== false || page.offset !== offset ||
          page.limit !== limit || !nonnegative(page.totalCount) ||
          !nonnegative(payload.totalItems) || !nonnegative(payload.assignmentCount) ||
          payload.items.length > limit || offset + payload.items.length > page.totalCount ||
          page.hasMore !== (offset + payload.items.length < page.totalCount) ||
          (page.hasMore && !payload.items.length)) {
        throw new Error('Unvollständige Genre-Übersicht.');
      }
      if (!totals) {
        totals = {totalCount: page.totalCount, totalItems: payload.totalItems,
          assignmentCount: payload.assignmentCount};
      } else if (totals.totalCount !== page.totalCount ||
          totals.totalItems !== payload.totalItems ||
          totals.assignmentCount !== payload.assignmentCount) {
        throw new Error('Genre-Übersicht wurde während des Ladens geändert.');
      }
      for (const item of payload.items) {
        if (!item || typeof item.genreId !== 'string' || !item.genreId ||
            ids.has(item.genreId) || !nonnegative(item.displayOrder) ||
            item.displayOrder >= page.totalCount || positions.has(item.displayOrder)) {
          throw new Error('Uneinheitliche Genre-Übersicht.');
        }
        ids.add(item.genreId);
        positions.add(item.displayOrder);
        genres.push(Object.assign({}, item, {id: item.genreId}));
      }
      if (typeof input.onPage === 'function') input.onPage(payload);
      offset += payload.items.length;
      if (!page.hasMore) break;
    } while (true);
    genres.sort((a, b) => a.displayOrder - b.displayOrder);
    return {backendId: input.backendId, scope: 'recordings', locale, from: 0, until: 0,
      totalItems: totals.totalItems, assignmentCount: totals.assignmentCount, genres};
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
    fetchPublicRecordingGenres: fetchPublicRecordingGenres,
    fetchClientGenreRecordings: fetchClientGenreRecordings,
    fetchClientGenreEpg: fetchClientGenreEpg
  }));
}(window));
