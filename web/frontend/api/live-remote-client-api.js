(function (global) {
  'use strict';

  const base = global.VdrSuiteClientApi;
  if (!base || typeof base.requestJson !== 'function') return;

  function normalizeOptions(options) {
    return options && typeof options === 'object' ? options : {};
  }

  function fetchClientRemoteAction(options) {
    const value = normalizeOptions(options);
    const payload = value.payload !== undefined ? value.payload : value.body;
    return base.requestJson('/api/vdr/remote/actions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, value.headers || {}),
      body: payload && typeof payload === 'object' ? JSON.stringify(payload) : payload,
      cache: value.cache || 'no-store',
      credentials: value.credentials || 'same-origin'
    });
  }

  function fetchClientLiveOverlay(options) {
    const value = normalizeOptions(options);
    return base.requestJson('/api/vdr/live/overlay', {
      query: {backend: value.backendId || 'default', _: String(Date.now())},
      cache: value.cache || 'no-store',
      credentials: value.credentials || 'same-origin'
    });
  }

  function createClientLiveUpdateSource() {
    return typeof global.EventSource === 'function'
      ? new global.EventSource('/api/vdr/live', {withCredentials: true})
      : null;
  }

  global.VdrSuiteClientApi = Object.freeze(Object.assign({}, base, {
    fetchClientRemoteAction: fetchClientRemoteAction,
    fetchClientLiveOverlay: fetchClientLiveOverlay,
    createClientLiveUpdateSource: createClientLiveUpdateSource
  }));
}(window));
