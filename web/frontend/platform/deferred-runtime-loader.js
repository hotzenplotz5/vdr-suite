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

let channels2RuntimePromise = null;

function ensureVdrSuiteChannels2Runtime() {
  if (
    window.VdrSuiteChannels2 &&
    typeof window.VdrSuiteChannels2.activate === 'function'
  ) {
    return Promise.resolve(window.VdrSuiteChannels2);
  }

  if (!channels2RuntimePromise) {
    channels2RuntimePromise = loadVdrSuiteDeferredRuntime(
      'vdr-suite-channels2-runtime',
      '/frontend/channel-day-program.js',
      () => Boolean(
        window.VdrSuiteChannels2 &&
        typeof window.VdrSuiteChannels2.activate === 'function'
      )
    ).then(() => {
      if (
        !window.VdrSuiteChannels2 ||
        typeof window.VdrSuiteChannels2.activate !== 'function'
      ) {
        throw new Error(
          'Channels-2-Runtime wurde geladen, stellt aber keine activate()-API bereit.'
        );
      }

      return window.VdrSuiteChannels2;
    }).catch(error => {
      channels2RuntimePromise = null;
      throw error;
    });
  }

  return channels2RuntimePromise;
}

function startVdrSuiteDeferredFrontendRuntimes() {
  ensureVdrSuiteChannels2Runtime().catch(error => {
    console.error('VDR-Suite Channels 2 runtime failed', error);
  });

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
    ensureChannels2: ensureVdrSuiteChannels2Runtime,
    start: startVdrSuiteDeferredFrontendRuntimes
  });

  if (document.readyState === 'complete') {
    startVdrSuiteDeferredFrontendRuntimes();
  } else {
    window.addEventListener(
      'load',
      startVdrSuiteDeferredFrontendRuntimes,
      {once: true}
    );
  }
}
