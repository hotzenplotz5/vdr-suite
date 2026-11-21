// Home rebuild H2/H2.1: bounded current/next programme data owner.
//
// The H2 critical path remains exactly one compact Now/Next request. H2.1 adds
// optional artwork enrichment only after the caller has rendered that result.
// Artwork manifests are page-scoped, batch-loaded and cached separately.
(function (global) {
  'use strict';

  if (!global || global.VdrSuiteHomeNowNext) return;

  const state = {
    backendId: '',
    channelIds: [],
    events: [],
    loadedAt: 0,
    requestCount: 0,
    manifestRequestCount: 0,
    pageContexts: new Map(),
    manifestRequests: new Map(),
    artworkByEvent: new Map()
  };

  function text(value) {
    return value === undefined || value === null
      ? ''
      : String(value).trim();
  }

  function list(data, key) {
    if (Array.isArray(data)) return data;
    if (data && Array.isArray(data[key])) return data[key];
    if (data && Array.isArray(data.items)) return data.items;
    if (data && Array.isArray(data.results)) return data.results;
    return [];
  }

  function clientApi() {
    const platform = global.VdrSuitePlatform || null;

    if (platform &&
        typeof platform.getClientApi === 'function') {
      const client = platform.getClientApi();
      if (client) return client;
    }

    return global.VdrSuiteClientApi || null;
  }

  function normalizedChannelIds(values) {
    const source = Array.isArray(values)
      ? values
      : String(values || '').split(',');

    const result = [];
    const seen = new Set();

    source.forEach(value => {
      const id = text(value);
      if (!id || seen.has(id)) return;
      seen.add(id);
      result.push(id);
    });

    return result;
  }

  function eventId(event) {
    return text(
      event && (
        event.id ||
        event.eventId ||
        event.nativeId ||
        event.native_id
      )
    );
  }

  function eventChannelId(event) {
    return text(
      event && (
        event.channelId ||
        event.channel ||
        event.channel_id
      )
    );
  }

  function pageKey(backendId, channelIds) {
    return [
      text(backendId) || 'default',
      normalizedChannelIds(channelIds).join(',')
    ].join('\n');
  }

  function artworkKey(backendId, channelId, id) {
    return [
      text(backendId) || 'default',
      text(channelId),
      text(id)
    ].join('\n');
  }

  function forgetManifestRequestsForPage(key) {
    Array.from(state.manifestRequests.keys()).forEach(manifestKey => {
      if (manifestKey.indexOf(key + '\n') === 0) {
        state.manifestRequests.delete(manifestKey);
      }
    });
  }

  function rememberPageContext(
    backendId,
    channelIds,
    fromTime,
    events
  ) {
    const key = pageKey(backendId, channelIds);

    const eventKeys = list(events, 'events')
      .map(event => {
        const channel = eventChannelId(event);
        const id = eventId(event);

        return channel && id
          ? artworkKey(backendId, channel, id)
          : '';
      })
      .filter(Boolean);

    const context = {
      pageKey: key,
      backendId: backendId,
      channelIds: channelIds.slice(),
      fromTime: fromTime,
      eventKeys: eventKeys
    };

    /*
     * A fresh H2 snapshot invalidates artwork for those exact events until the
     * new manifest arrives. This prevents a removed artwork row from leaving a
     * stale cover behind.
     */
    eventKeys.forEach(keyValue => {
      state.artworkByEvent.delete(keyValue);
    });

    forgetManifestRequestsForPage(key);
    state.pageContexts.set(key, context);

    return context;
  }

  function clear() {
    state.backendId = '';
    state.channelIds = [];
    state.events = [];
    state.loadedAt = 0;
    state.pageContexts.clear();
    state.manifestRequests.clear();
    state.artworkByEvent.clear();
  }

  function loadPage(options) {
    const config = options || {};
    const client = clientApi();

    const backendId =
      text(config.backendId) || 'default';

    const channelIds =
      normalizedChannelIds(config.channelIds);

    if (!client ||
        typeof client.fetchClientEpgCacheNowNext !== 'function') {
      return Promise.reject(
        new Error('Home Now/Next API ist nicht verfügbar.')
      );
    }

    if (channelIds.length === 0) {
      state.backendId = backendId;
      state.channelIds = [];
      state.events = [];
      state.loadedAt = Date.now();

      return Promise.resolve({
        backendId: backendId,
        eventCount: 0,
        events: []
      });
    }

    const now = Math.floor(Date.now() / 1000);
    const fromTime = String(now);

    state.requestCount += 1;

    return client.fetchClientEpgCacheNowNext({
      query: {
        backend: backendId,
        channelIds: channelIds.join(','),
        fromTime: fromTime,
        perChannelLimit: '2',
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(data => {
      const events = list(data, 'events').slice();

      state.backendId = backendId;
      state.channelIds = channelIds.slice();
      state.events = events;
      state.loadedAt = Date.now();

      rememberPageContext(
        backendId,
        channelIds,
        fromTime,
        events
      );

      return data;
    });
  }

  function loadArtworkPage(options) {
    const config = options || {};

    const backendId =
      text(config.backendId) || 'default';

    const channelIds =
      normalizedChannelIds(config.channelIds);

    if (channelIds.length === 0) {
      return Promise.resolve(null);
    }

    const key = pageKey(backendId, channelIds);
    const context = state.pageContexts.get(key);

    /*
     * Artwork enrichment is valid only for a successfully completed H2 page.
     * This also guarantees that the manifest reuses exactly that page's
     * fromTime instead of sampling a newer timestamp.
     */
    if (!context) {
      return Promise.resolve(null);
    }

    const client = clientApi();

    if (!client ||
        typeof client.fetchClientEpgCacheNowNextArtwork !== 'function') {
      return Promise.resolve(null);
    }

    const manifestKey =
      key + '\n' + context.fromTime;

    if (state.manifestRequests.has(manifestKey)) {
      return state.manifestRequests.get(manifestKey);
    }

    state.manifestRequestCount += 1;

    const request =
      client.fetchClientEpgCacheNowNextArtwork({
        query: {
          backend: context.backendId,
          channelIds: context.channelIds.join(','),
          fromTime: context.fromTime,
          perChannelLimit: '2',
          _: String(Date.now())
        },
        cache: 'no-store',
        credentials: 'same-origin'
      }).then(data => {
        /*
         * A newer page snapshot may have superseded this request while the
         * manifest was in flight. Old enrichment must never overwrite it.
         */
        if (state.pageContexts.get(key) !== context) {
          return data;
        }

        const requestedEvents =
          new Set(context.eventKeys);

        context.eventKeys.forEach(keyValue => {
          state.artworkByEvent.delete(keyValue);
        });

        list(data, 'items').forEach(item => {
          const artwork =
            item && item.artwork &&
            typeof item.artwork === 'object'
              ? item.artwork
              : null;

          const channel = text(item && item.channelId);
          const id = text(item && item.eventId);
          const itemKey =
            artworkKey(context.backendId, channel, id);

          if (!channel ||
              !id ||
              !requestedEvents.has(itemKey) ||
              !artwork ||
              artwork.available !== true ||
              !text(artwork.url)) {
            return;
          }

          state.artworkByEvent.set(
            itemKey,
            Object.assign({}, artwork)
          );
        });

        return data;
      }).catch(error => {
        state.manifestRequests.delete(manifestKey);
        throw error;
      });

    state.manifestRequests.set(
      manifestKey,
      request
    );

    return request;
  }

  function artworkForEvent(
    backendId,
    channelId,
    id
  ) {
    return state.artworkByEvent.get(
      artworkKey(
        backendId,
        channelId,
        id
      )
    ) || null;
  }

  function snapshot() {
    return {
      backendId: state.backendId,
      channelIds: state.channelIds.slice(),
      channelCount: state.channelIds.length,
      events: state.events.slice(),
      eventCount: state.events.length,
      loadedAt: state.loadedAt,
      requestCount: state.requestCount,
      manifestRequestCount: state.manifestRequestCount,
      artworkCount: state.artworkByEvent.size
    };
  }

  global.VdrSuiteHomeNowNext = Object.freeze({
    loadPage: loadPage,
    loadArtworkPage: loadArtworkPage,
    artworkForEvent: artworkForEvent,
    snapshot: snapshot,
    clear: clear
  });
}(typeof window !== 'undefined' ? window : this));
