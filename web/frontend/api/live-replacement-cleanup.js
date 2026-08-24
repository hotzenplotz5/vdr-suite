// Phase 65.D Live-TV replacement cleanup hardening.
//
// The persistent playback shell deliberately yields the old browser owner
// before the daemon performs STOP A -> OPEN B through replacesSessionId. If the
// replacement request fails before a new session id is returned, the browser no
// longer owns A and must best-effort stop that yielded session. The stop is
// idempotent when the daemon already processed the replacement.
(function (global) {
  'use strict';

  if (global.VdrSuiteLiveReplacementCleanup) return;

  function text(value) {
    return value === undefined || value === null ? '' : String(value).trim();
  }

  function safeSessionId(value) {
    const id = text(value);
    if (!id || id.length > 128) return '';
    return /^[A-Za-z0-9._:-]+$/.test(id) ? id : '';
  }

  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function cleanupReplacement(backendId, sessionId) {
    const id = safeSessionId(sessionId);
    const api = global.VdrSuiteClientApi;
    if (!id || !api || typeof api.requestJson !== 'function') {
      return Promise.resolve(null);
    }
    return api.requestJson('/api/media/sessions', {
      method: 'POST',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify({
        resourceKind: 'live-channel',
        backendId: text(backendId || 'default'),
        sessionId: id,
        operation: 'stop'
      }),
      cache: 'no-store',
      credentials: 'same-origin'
    }).catch(function () {
      return null;
    });
  }

  function wrapLivePanel(panel, backendId, replacementId) {
    const id = safeSessionId(replacementId);
    if (!id || !panel || typeof panel !== 'object' || typeof panel.start !== 'function') {
      return panel;
    }

    const wrapped = {};
    Object.keys(panel).forEach(function (key) { wrapped[key] = panel[key]; });
    const start = panel.start;
    let cleanupIssued = false;

    function cleanupOnce() {
      if (cleanupIssued) return Promise.resolve(null);
      cleanupIssued = true;
      return cleanupReplacement(backendId, id);
    }

    wrapped.start = function () {
      let result;
      try {
        result = start.apply(panel, arguments);
      } catch (error) {
        cleanupOnce();
        throw error;
      }
      return Promise.resolve(result).then(function (newSessionId) {
        if (safeSessionId(newSessionId)) return newSessionId;
        return cleanupOnce().then(function () { return newSessionId; });
      }, function (error) {
        return cleanupOnce().then(function () { throw error; });
      });
    };
    return Object.freeze(wrapped);
  }

  function wrapPlayback(source) {
    if (!source || typeof source !== 'object' || typeof source.createLivePanel !== 'function') {
      return source;
    }
    const wrapped = {};
    Object.keys(source).forEach(function (key) { wrapped[key] = source[key]; });
    const createLivePanel = source.createLivePanel;
    wrapped.createLivePanel = function (channel, backendId, options) {
      const settings = options && typeof options === 'object' ? options : {};
      const panel = createLivePanel.call(source, channel, backendId, settings);
      return wrapLivePanel(panel, backendId, settings.replacesSessionId || '');
    };
    return Object.freeze(wrapped);
  }

  let facade = null;
  const descriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
  if (descriptor && typeof descriptor.get === 'function' && typeof descriptor.set === 'function') {
    const baseGet = descriptor.get;
    const baseSet = descriptor.set;
    facade = wrapPlayback(baseGet.call(global));
    Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
      configurable: true,
      enumerable: descriptor.enumerable !== false,
      get: function () { return facade; },
      set: function (value) {
        baseSet.call(global, value);
        facade = wrapPlayback(baseGet.call(global));
      }
    });
  }
  else {
    facade = wrapPlayback(global.VdrSuiteRecordings2Playback);
    global.VdrSuiteRecordings2Playback = facade;
  }

  global.VdrSuiteLiveReplacementCleanup = Object.freeze({
    __test: Object.freeze({
      safeSessionId: safeSessionId,
      cleanupReplacement: cleanupReplacement,
      wrapLivePanel: wrapLivePanel,
      wrapPlayback: wrapPlayback
    })
  });
}(window));
