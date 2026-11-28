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
    if (!normalized.scope || normalized.scope === 'recordings')
      return fetchPublicRecordingGenres(normalized);
    if (normalized.scope === 'epg') return fetchPublicEpgGenreOverview(normalized);
    return fetchLegacyGenreOverview(normalized);
  }

  function fetchLegacyGenreOverview(normalized) {
    return base.requestJson('/api/metadata/genres', withQuery(normalized, {
      backend: normalized.backendId,
      scope: normalized.scope,
      locale: normalized.locale,
      from: normalized.from,
      until: normalized.until
    }));
  }

  // Reconstruct the existing hierarchy from the public canonical taxonomy.
  // Distinct items and overlapping assignments are different server totals.
  async function fetchPublicEpgGenreOverview(input) {
    const locale = input.locale || 'de', limit = 30;
    const categories = [], parents = new Map(), seen = new Set();
    let from = input.from, until = input.until, offset = 0, totals;
    const nonnegative = n => Number.isSafeInteger(n) && n >= 0;
    do {
      let payload;
      try {
        payload = await base.requestJson('/api/v1/genres/epg', {
          query: {backendId:input.backendId, locale, limit, offset,
            fromTime:from, untilTime:until},
          credentials:input.credentials || 'same-origin',
          cache:input.cache || 'no-store', signal:input.signal
        });
      } catch (error) {
        // The older TV-only query rejects window parameters; legacy is
        // independently authenticated. No fallback on provider/transport errors.
        if (!offset && error && (error.status === 404 || error.status === 501 ||
            (error.status === 400 && error.code === 'invalid_request' && from && until) ||
            (error.status === 403 && (error.code === 'permission_denied' ||
              error.code === 'backend_scope_denied'))))
          return fetchLegacyGenreOverview(input);
        throw error;
      }
      const page = payload && payload.page;
      if (!offset && page && payload.fromTime === undefined)
        return fetchLegacyGenreOverview(input);
      if (!page || !Array.isArray(payload.items) || !payload.meta ||
          payload.meta.partial !== false || page.limit !== limit || page.offset !== offset ||
          !nonnegative(page.totalCount) || !nonnegative(payload.totalItems) ||
          !nonnegative(payload.assignmentCount) || !nonnegative(payload.fromTime) ||
          !nonnegative(payload.untilTime) || payload.untilTime <= payload.fromTime ||
          (from !== undefined && payload.fromTime !== from) ||
          (until !== undefined && payload.untilTime !== until) ||
          payload.items.length > limit || offset + payload.items.length > page.totalCount ||
          page.hasMore !== (offset + payload.items.length < page.totalCount) ||
          (page.hasMore && !payload.items.length))
        throw new Error('Unvollständige EPG-Genre-Übersicht.');
      from = payload.fromTime; until = payload.untilTime;
      if (!totals) totals = [page.totalCount, payload.totalItems, payload.assignmentCount];
      else if (totals[0] !== page.totalCount || totals[1] !== payload.totalItems ||
          totals[2] !== payload.assignmentCount)
        throw new Error('EPG-Genre-Übersicht wurde während des Ladens geändert.');
      for (const item of payload.items) {
        if (!item || !item.genreId || !item.contentClass ||
            !nonnegative(item.count) || typeof item.label !== 'string')
          throw new Error('Uneinheitliche EPG-Genre-Übersicht.');
        const key = item.contentClass + ':' + item.genreId;
        if (seen.has(key)) throw new Error('Uneinheitliche EPG-Genre-Übersicht.');
        seen.add(key);
        const entry = Object.assign({}, item, {id:item.genreId, children:[]});
        if (item.genreId === item.contentClass) {
          categories.push(entry);
          parents.set(item.contentClass, entry);
        } else {
          const parent = parents.get(item.contentClass);
          if (!parent) throw new Error('Unvollständige EPG-Genre-Hierarchie.');
          for (const field of ['label','labelDe','labelEn']) {
            const prefix = parent[field] + ' › ';
            if (typeof entry[field] === 'string' && entry[field].startsWith(prefix))
              entry[field] = entry[field].slice(prefix.length);
          }
          parent.children.push(entry);
        }
      }
      offset += payload.items.length;
      if (!page.hasMore) break;
    } while (true);
    return {backendId:input.backendId, scope:'epg', locale, taxonomy:'epg-browse-v1',
      from, until, totalItems:totals[1], assignmentCount:totals[2],
      categories, genres:categories};
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

  // Adapt the public membership page to the established browser list shape.
  // Pass the original response to the existing Home cache ticket, not the
  // projected object. Native identifiers are intentionally not fabricated.
  async function fetchPublicGenreRecordings(options) {
    const input = normalizeOptions(options);
    const payload = await base.requestJson('/api/v1/genres/recordings', {
      query: {
        backendId: input.backendId, genreId: input.genreId,
        locale: input.locale || 'de', limit: input.limit === undefined ? 30 : input.limit,
        offset: input.offset === undefined ? 0 : input.offset
      },
      credentials: input.credentials || 'same-origin',
      cache: input.cache || 'no-store',
      signal: input.signal
    });
    const page = payload.page;
    if (typeof input.onPage === 'function') input.onPage(payload);
    return Object.assign({}, payload, {
      recordings: payload.items,
      total: page.totalCount,
      totalCount: page.totalCount,
      hasMore: page.hasMore
    });
  }

  // Resolve an opaque list identity to the existing private Browser
  // detail model. Never infer a native path from a display title or timestamp.
  async function fetchClientRecordingForPublicId(options) {
    const input = normalizeOptions(options);
    const payload = await base.requestJson('/api/vdr/recordings/folder', {
      query: {backend: input.backendId, recordingId: input.recordingId},
      credentials: input.credentials || 'same-origin',
      cache: input.cache || 'no-store',
      signal: input.signal
    });
    const recording = payload && payload.recording;
    if (!recording || payload.publicRecordingId !== input.recordingId ||
        recording.backendId !== input.backendId || !recording.backendNativeId) {
      throw new Error('Aufnahmeidentität konnte nicht eindeutig zugeordnet werden.');
    }
    return Object.assign({}, payload, {
      recording: Object.assign({}, recording, {publicRecordingId: payload.publicRecordingId})
    });
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

  function fetchLegacyGenreEpg(normalized) {
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

  // Keep the established 48-card page and exact selected overview window.
  // The public response supplies cached card facts, including sender and poster,
  // so this read never needs supplemental channel or metadata lookups.
  async function fetchClientGenreEpg(options) {
    const normalized = normalizeOptions(options);
    const limit = normalized.limit === undefined ? 48 : normalized.limit;
    const offset = normalized.offset === undefined ? 0 : normalized.offset;
    let payload;
    try {
      payload = await base.requestJson('/api/v1/genres/epg/events', {
        query: {backendId:normalized.backendId, contentClass: normalized.contentClass,
          genreId:normalized.genreId || undefined, locale:normalized.locale || 'de',
          fromTime:normalized.from, untilTime:normalized.until, limit, offset},
        credentials:normalized.credentials || 'same-origin',
        cache:normalized.cache || 'no-store', signal:normalized.signal
      });
    } catch (error) {
      if (error && (error.status === 404 || error.status === 501 ||
          (error.status === 400 && error.code === 'invalid_request') ||
          (error.status === 403 && (error.code === 'permission_denied' ||
            error.code === 'backend_scope_denied'))))
        return fetchLegacyGenreEpg(normalized);
      throw error;
    }
    if (payload && payload.meta && payload.meta.cardPresentation === undefined)
      return fetchLegacyGenreEpg(normalized);
    const page = payload && payload.page;
    if (!page || !Array.isArray(payload.items) || !payload.meta ||
        payload.meta.partial !== false || payload.meta.cardPresentation !== true ||
        page.limit !== limit || page.offset !== offset ||
        !Number.isSafeInteger(page.totalCount) || page.totalCount < 0 ||
        (normalized.from !== undefined && payload.fromTime !== normalized.from) ||
        (normalized.until !== undefined && payload.untilTime !== normalized.until) ||
        payload.items.length > limit ||
        page.hasMore !== (offset + payload.items.length < page.totalCount))
      throw new Error('Unvollständige EPG-Genre-Ergebnisse.');
    const items = payload.items.map(event => {
      const image = event.artwork || {};
      const suffix = '?backendId=' + encodeURIComponent(event.backendId) +
        '&channelId=' + encodeURIComponent(event.channelId) +
        '&eventId=' + encodeURIComponent(event.eventId);
      const direct = '/api/v1/epg/artwork' + suffix;
      const poster = '/api/v1/epg/preferred-artwork' + suffix + '&variant=genre';
      return Object.assign({}, event, {
        id:event.eventId, startTime:Number(event.startTime), endTime:Number(event.endTime),
        artwork:Object.assign({}, image, {
          url:image.available ? (image.portrait ? poster : direct) : '',
          fallbackUrl:direct
        })
      });
    });
    return {backendId:normalized.backendId, contentClass:normalized.contentClass,
      genreId:normalized.genreId || '', from:payload.fromTime, until:payload.untilTime,
      items, limit, offset, total:page.totalCount, hasMore:page.hasMore};
  }

  global.VdrSuiteClientApi = Object.freeze(Object.assign({}, base, {
    fetchClientGenres: fetchClientGenres,
    fetchPublicRecordingGenres: fetchPublicRecordingGenres,
    fetchClientGenreRecordings: fetchClientGenreRecordings,
    fetchPublicGenreRecordings: fetchPublicGenreRecordings,
    fetchClientRecordingForPublicId: fetchClientRecordingForPublicId,
    fetchClientGenreEpg: fetchClientGenreEpg
  }));
}(window));
