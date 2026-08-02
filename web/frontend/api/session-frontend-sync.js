(function (global) {
  'use strict';

  let bound = false;
  let initialized = false;
  let previousAuthenticated = false;
  let reloadScheduled = false;

  function pageIsLeaving() {
    return Boolean(
      global.document &&
      global.document.visibilityState === 'hidden'
    );
  }

  function refreshBackendSelection() {
    if (typeof global.loadBackendSelection === 'function') {
      global.loadBackendSelection();
    }
  }

  function reloadFrontend() {
    if (reloadScheduled || pageIsLeaving()) {
      return;
    }

    reloadScheduled = true;

    if (global.location && typeof global.location.reload === 'function') {
      global.location.reload();
    }
  }

  function sessionChanged(state) {
    const authenticated = Boolean(state && state.authenticated);

    if (!initialized) {
      initialized = true;
      previousAuthenticated = authenticated;
      return;
    }

    const wasAuthenticated = previousAuthenticated;
    previousAuthenticated = authenticated;

    if (!wasAuthenticated && authenticated) {
      refreshBackendSelection();
      return;
    }

    if (wasAuthenticated && !authenticated) {
      reloadFrontend();
    }
  }

  function bindSession() {
    if (bound) {
      return true;
    }

    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.subscribe !== 'function') {
      return false;
    }

    bound = true;
    session.subscribe(sessionChanged);
    return true;
  }

  function start() {
    if (bindSession()) {
      return;
    }

    let attempts = 0;

    function retry() {
      attempts += 1;

      if (bindSession() || attempts >= 20 || typeof global.setTimeout !== 'function') {
        return;
      }

      global.setTimeout(retry, 50);
    }

    retry();
  }

  if (global.document && global.document.readyState === 'loading') {
    global.document.addEventListener('DOMContentLoaded', start, {once: true});
  } else {
    start();
  }
}(window));
