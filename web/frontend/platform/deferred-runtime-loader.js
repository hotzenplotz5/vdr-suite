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

function ensureVdrSuiteChannels2Tab() {
  const nav = document.getElementById('module-nav');
  if (!nav) {
    return null;
  }

  let button = nav.querySelector('[data-module="channels2"]');
  if (!button) {
    button = document.createElement('button');
    button.type = 'button';
    button.className = 'module-tab';
    button.dataset.module = 'channels2';
    button.textContent = 'Channels 2';

    const channels = nav.querySelector('[data-module="channels"]');
    if (channels && channels.nextSibling) {
      nav.insertBefore(button, channels.nextSibling);
    } else {
      nav.appendChild(button);
    }
  }

  if (button.dataset.channels2LoaderBound !== 'true') {
    button.dataset.channels2LoaderBound = 'true';
    button.addEventListener('click', event => {
      event.preventDefault();
      event.stopImmediatePropagation();

      document.querySelectorAll('.module-tab').forEach(tab => {
        tab.classList.toggle('active', tab === button);
      });

      if (window.VdrSuiteChannels2 &&
          typeof window.VdrSuiteChannels2.activate === 'function') {
        window.VdrSuiteChannels2.activate();
        return;
      }

      loadVdrSuiteDeferredRuntime(
        'vdr-suite-channels2-runtime',
        '/frontend/channel-day-program.js',
        () => Boolean(window.VdrSuiteChannels2)
      ).then(() => {
        if (window.VdrSuiteChannels2 &&
            typeof window.VdrSuiteChannels2.activate === 'function') {
          window.VdrSuiteChannels2.activate();
        }
      }).catch(error => {
        console.error('VDR-Suite Channels 2 runtime failed', error);
      });
    }, true);
  }

  return button;
}

function startVdrSuiteDeferredFrontendRuntimes() {
  ensureVdrSuiteChannels2Tab();

  loadVdrSuiteDeferredRuntime(
    'vdr-suite-channels2-runtime',
    '/frontend/channel-day-program.js',
    () => Boolean(window.VdrSuiteChannels2)
  ).catch(error => {
    console.error('VDR-Suite Channels 2 runtime failed', error);
  });

  // The original channel browser remains isolated while Channels 2 is tested
  // as a separate module with its own channel, EPG detail and action ownership.
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
  if (document.readyState === 'complete') {
    startVdrSuiteDeferredFrontendRuntimes();
  } else {
    window.addEventListener('load', startVdrSuiteDeferredFrontendRuntimes, {once: true});
  }
}
