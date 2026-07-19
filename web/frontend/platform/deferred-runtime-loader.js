'use strict';

function loadVdrSuiteDeferredRuntime(id, src, readyCheck) {
  if (typeof readyCheck === 'function' && readyCheck()) {
    return Promise.resolve();
  }

  const existing = document.getElementById(id);
  if (existing) {
    return new Promise((resolve, reject) => {
      if (existing.dataset.loaded === 'true') {
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
    script.addEventListener('load', () => {
      script.dataset.loaded = 'true';
      resolve();
    }, {once: true});
    script.addEventListener('error', reject, {once: true});
    document.head.appendChild(script);
  });
}

function startVdrSuiteDeferredFrontendRuntimes() {
  // The channel browser module owns channel selection, agenda selection and
  // EPG event details. Do not load the former channel-day compatibility
  // runtimes here: they capture channel clicks, create a second detail view
  // and move/scroll that view after rendering.
  //
  // recording-trash-ux.js is the installed and statically served deferred
  // frontend action bundle. It exposes both the recording trash UX and the
  // EPG SearchTimer action enhancer, so one runtime request is sufficient.
  loadVdrSuiteDeferredRuntime(
    'vdr-suite-deferred-actions-runtime',
    '/frontend/recording-trash-ux.js',
    () => Boolean(
      window.VdrSuiteRecordingTrashUx &&
      window.VdrSuiteEpgSearchTimerActions
    )
  ).catch(error => {
    console.error('VDR-Suite deferred frontend runtime failed', error);
  });
}

if (typeof window !== 'undefined') {
  if (document.readyState === 'complete') {
    startVdrSuiteDeferredFrontendRuntimes();
  } else {
    window.addEventListener('load', startVdrSuiteDeferredFrontendRuntimes, {once: true});
  }
}
