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
  loadVdrSuiteDeferredRuntime(
    'vdr-suite-channel-day-program-runtime',
    '/frontend/channel-day-program.js',
    () => Boolean(window.VdrSuiteChannelDayProgram)
  )
    .then(() => loadVdrSuiteDeferredRuntime(
      'vdr-suite-channel-day-program-compat-runtime',
      '/frontend/channel-day-program-compat.js',
      () => Boolean(window.VdrSuiteChannelDayProgramCompat)
    ))
    .then(() => loadVdrSuiteDeferredRuntime(
      'vdr-suite-recording-trash-ux-runtime',
      '/frontend/recording-trash-ux.js',
      () => Boolean(window.VdrSuiteRecordingTrashUx)
    ))
    .catch(error => {
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
