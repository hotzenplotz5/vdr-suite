(function (global) {
  'use strict';

  // Optional enhancement; never delay or replace the authenticated bootstrap.
  if (!global.isSecureContext || !global.navigator.serviceWorker || !global.VdrSuitePublicUrl) return;
  try {
    const worker = global.VdrSuitePublicUrl.resolvePath('/frontend/service-worker.js');
    const scope = global.VdrSuitePublicUrl.resolvePath('/frontend/');
    global.navigator.serviceWorker.register(worker, { scope, updateViaCache: 'none' })
      .catch(function () { /* Normal browser operation remains available. */ });
  } catch (error) { /* Unsupported or restricted browser. */ }
  // Reuse the established session lifecycle, including locally observed revocation.
  const session = global.VdrSuiteBrowserSession;
  if (session && typeof session.subscribe === 'function') {
    let previouslyAuthenticated = false;
    session.subscribe(function (state) {
      const authenticated = Boolean(state && state.authenticated);
      if (previouslyAuthenticated && !authenticated) {
        const controller = global.navigator.serviceWorker.controller;
        if (controller) {
          try { controller.postMessage({type: 'vdr-suite-artwork-clear'}); } catch (error) {}
        }
      }
      previouslyAuthenticated = authenticated;
    });
  }
}(window));
