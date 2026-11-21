// Phase 65.D ADR-0056 canonical playback-owner lifecycle publication helper.
//
// This is intentionally browser-local. It publishes state already owned by the
// persistent playback owner and never polls the server, creates MediaSessions,
// owns transports, or invents a second playback authority.
//
// Slice 3 adds playback-presentation continuity. lifecycleRevision remains only
// local publication ordering and is deliberately independent from the
// decoder-significant presentation incarnation.
(function (global) {
  'use strict';

  if (!global || global.VdrSuitePlaybackOwnerLifecycle) return;

  const LIFECYCLE_VERSION = 1;
  const GENERATION_KEY = 'generation';

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function nullableText(value) {
    const normalized = text(value).trim();
    return normalized || null;
  }

  function continuitySnapshot(generation, state) {
    const value = {};
    value[GENERATION_KEY] = generation;
    value.state = state;
    return Object.freeze(value);
  }

  function create(initial) {
    const listeners = new Set();
    let revision = 0;
    let presentationGeneration = 0;
    let transportReplacementPending = false;
    let current = Object.freeze(Object.assign({
      lifecycleVersion: LIFECYCLE_VERSION,
      lifecycleRevision: revision,
      state: 'idle',
      sessionId: null,
      transport: 'none',
      transition: 'snapshot',
      continuity: continuitySnapshot(presentationGeneration, 'idle')
    }, initial && typeof initial === 'object' ? initial : {}, {
      continuity: continuitySnapshot(presentationGeneration, 'idle')
    }));

    function snapshot() {
      return current;
    }

    function continuityFor(transition, patch, previousSessionId) {
      let state = presentationGeneration > 0 ? 'stable' : 'idle';

      if (transition === 'session-started') {
        if (presentationGeneration === 0) presentationGeneration = 1;
        transportReplacementPending = false;
        state = 'stable';
      }
      else if (transition === 'session-replaced') {
        if (transportReplacementPending) {
          transportReplacementPending = false;
        }
        else {
          presentationGeneration += 1;
        }
        state = 'stable';
      }
      else if (transition === 'seek-started' ||
               transition === 'session-replacing' ||
               transition === 'transport-replacing' ||
               transition === 'relinquishing') {
        state = presentationGeneration > 0 ? 'replacing' : 'idle';
      }
      else if (transition === 'transport-replaced') {
        // Fallback owners build their initial transport before any authorized
        // MediaSession exists. That preparation is not a presentation change.
        // Replacing an already-authoritative presentation is different: bump
        // once here and let the following replacement session stabilize the
        // same generation rather than double-counting it.
        if (presentationGeneration > 0 && previousSessionId) {
          presentationGeneration += 1;
          transportReplacementPending = true;
          state = 'replacing';
        }
        else {
          state = presentationGeneration > 0 ? 'replacing' : 'idle';
        }
      }
      else if (transition === 'seek-completed') {
        presentationGeneration += 1;
        transportReplacementPending = false;
        state = 'stable';
      }
      else if (transition === 'stopped' ||
               transition === 'destroyed' ||
               transition === 'relinquished') {
        transportReplacementPending = false;
        state = 'inactive';
      }
      else if (transition === 'stop-requested' ||
               transition === 'page-teardown') {
        state = presentationGeneration > 0 ? 'stopping' : 'idle';
      }
      else if (transition === 'start-requested') {
        state = presentationGeneration > 0 ? 'replacing' : 'starting';
      }
      else if (patch && (patch.state === 'playing' || patch.state === 'paused')) {
        state = presentationGeneration > 0 ? 'stable' : 'idle';
      }

      return continuitySnapshot(presentationGeneration, state);
    }

    function publish(change) {
      const patch = change && typeof change === 'object' ? change : {};
      const transition = text(
        Object.prototype.hasOwnProperty.call(patch, 'transition')
          ? patch.transition
          : current.transition
      ) || 'state-changed';
      const previousSessionId = nullableText(current.sessionId);
      const continuity = continuityFor(transition, patch, previousSessionId);
      revision += 1;
      current = Object.freeze(Object.assign({}, current, patch, {
        lifecycleVersion: LIFECYCLE_VERSION,
        lifecycleRevision: revision,
        sessionId: nullableText(
          Object.prototype.hasOwnProperty.call(patch, 'sessionId')
            ? patch.sessionId
            : current.sessionId
        ),
        transition: transition,
        continuity: continuity
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
