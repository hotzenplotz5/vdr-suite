'use strict';

(function (global) {
  const MAX_ATTEMPTS = 80;
  const RETRY_DELAY_MS = 100;
  let installed = false;

  function install() {
    if (installed) return true;
    if (typeof createEpgEventDetailCard !== 'function') return false;

    const renderer = global.VdrSuiteEpgMetadataDetail;
    if (!renderer || typeof renderer.enhance !== 'function') return false;

    const originalCreateEpgEventDetailCard = createEpgEventDetailCard;
    createEpgEventDetailCard = function (event, channel) {
      const detail = originalCreateEpgEventDetailCard(event, channel);
      renderer.enhance(detail, event, channel);
      return detail;
    };

    installed = true;
    document.documentElement.dataset.epgMetadataDetailHook = 'true';
    return true;
  }

  let attempts = 0;
  function attemptInstall() {
    if (install()) return;
    attempts += 1;
    if (attempts < MAX_ATTEMPTS) {
      global.setTimeout(attemptInstall, RETRY_DELAY_MS);
    }
  }

  attemptInstall();
  global.VdrSuiteEpgMetadataDetailHook = Object.freeze({
    installed: function () { return installed; }
  });
}(window));
