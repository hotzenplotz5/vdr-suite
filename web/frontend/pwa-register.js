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
}(window));
