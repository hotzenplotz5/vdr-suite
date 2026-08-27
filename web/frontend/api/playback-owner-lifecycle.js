// Phase 65.D ADR-0056 canonical playback-owner lifecycle publication helper.
//
// This is intentionally browser-local. It publishes state already owned by the
// persistent playback owner and never polls the server, creates MediaSessions,
// owns transports, or invents playback-presentation generation semantics.
(function (global) {
  'use strict';

  if (!global || global.VdrSuitePlaybackOwnerLifecycle) return;

  const LIFECYCLE_VERSION = 1;

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function nullableText(value) {
    const normalized = text(value).trim();
    return normalized || null;
  }

  function create(initial) {
    const listeners = new Set();
    let revision = 0;
    let current = Object.freeze(Object.assign({
      lifecycleVersion: LIFECYCLE_VERSION,
      lifecycleRevision: revision,
      state: 'idle',
      sessionId: null,
      transport: 'none',
      transition: 'snapshot'
    }, initial && typeof initial === 'object' ? initial : {}));

    function snapshot() {
      return current;
    }

    function publish(change) {
      const patch = change && typeof change === 'object' ? change : {};
      revision += 1;
      current = Object.freeze(Object.assign({}, current, patch, {
        lifecycleVersion: LIFECYCLE_VERSION,
        lifecycleRevision: revision,
        sessionId: nullableText(
          Object.prototype.hasOwnProperty.call(patch, 'sessionId')
            ? patch.sessionId
            : current.sessionId
        )
      }));
      listeners.forEach(function (listener) {
        try { listener(current); } catch (error) {}
      });
      return current;
    }

    function subscribe(listener) {
      if (typeof listener !== 'function') return function () {};
      let active = true;
      listeners.add(listener);
      try { listener(current); } catch (error) {}
      return function unsubscribe() {
        if (!active) return;
        active = false;
        listeners.delete(listener);
      };
    }

    function clear() {
      listeners.clear();
    }

    return Object.freeze({
      snapshot: snapshot,
      publish: publish,
      subscribe: subscribe,
      clear: clear
    });
  }

  global.VdrSuitePlaybackOwnerLifecycle = Object.freeze({
    lifecycleVersion: LIFECYCLE_VERSION,
    create: create
  });
}(window));
