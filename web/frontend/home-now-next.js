// Home rebuild H2: bounded current/next programme data owner.
//
// This module owns the Home critical-path programme read. It deliberately
// requests exactly two cached events per supplied channel and exposes the
// resulting snapshot to Home presenters. It does not own playback, timers,
// programme details or artwork enrichment.
(function (global) {
  'use strict';

  if (!global || global.VdrSuiteHomeNowNext) return;

  const state = {
    backendId: '',
    channelIds: [],
    events: [],
    loadedAt: 0,
    requestCount: 0
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

  function clear() {
    state.backendId = '';
    state.channelIds = [];
    state.events = [];
    state.loadedAt = 0;
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

    state.requestCount += 1;

    return client.fetchClientEpgCacheNowNext({
      query: {
        backend: backendId,
        channelIds: channelIds.join(','),
        fromTime: String(now),
        perChannelLimit: '2',
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(data => {
      state.backendId = backendId;
      state.channelIds = channelIds.slice();
      state.events = list(data, 'events').slice();
      state.loadedAt = Date.now();

      return data;
    });
  }

  function snapshot() {
    return {
      backendId: state.backendId,
      channelIds: state.channelIds.slice(),
      channelCount: state.channelIds.length,
      events: state.events.slice(),
      eventCount: state.events.length,
      loadedAt: state.loadedAt,
      requestCount: state.requestCount
    };
  }

  global.VdrSuiteHomeNowNext = Object.freeze({
    loadPage: loadPage,
    snapshot: snapshot,
    clear: clear
  });
}(typeof window !== 'undefined' ? window : this));
