'use strict';

function loadVdrSuiteDeferredRuntime(id, src, readyCheck) {
  if (typeof readyCheck === 'function' && readyCheck()) {
    return Promise.resolve();
  }

  const existing = document.getElementById(id);

  if (existing) {
    return new Promise((resolve, reject) => {
      if (
        existing.dataset.loaded === 'true' ||
        (typeof readyCheck === 'function' && readyCheck())
      ) {
        resolve();
        return;
      }

      existing.addEventListener('load', resolve, {once: true});
      existing.addEventListener('error', reject, {once: true});
    });
  }

  return new Promise((resolve, reject) => {
    const script = document.createElement('script');
    script.id = id;
    script.src = src + '?runtime=' + String(Date.now());
    script.async = false;

    script.addEventListener(
      'load',
      () => {
        script.dataset.loaded = 'true';
        resolve();
      },
      {once: true}
    );

    script.addEventListener('error', reject, {once: true});
    document.head.appendChild(script);
  });
}

function startVdrSuiteDeferredFrontendRuntimes() {
  loadVdrSuiteDeferredRuntime(
    'vdr-suite-epg-searchtimer-actions-runtime',
    '/frontend/epg-searchtimer-actions.js',
    () => Boolean(window.VdrSuiteEpgSearchTimerActions)
  ).catch(error => {
    console.error('VDR-Suite EPG SearchTimer runtime failed', error);
  });

  loadVdrSuiteDeferredRuntime(
    'vdr-suite-recording-trash-ux-runtime',
    '/frontend/recording-trash-ux.js',
    () => Boolean(window.VdrSuiteRecordingTrashUx)
  ).catch(error => {
    console.error('VDR-Suite deferred frontend runtime failed', error);
  });
}

if (typeof window !== 'undefined') {
  window.VdrSuiteDeferredFrontendRuntimes = Object.freeze({
    start: startVdrSuiteDeferredFrontendRuntimes
  });

  startVdrSuiteDeferredFrontendRuntimes();
}
