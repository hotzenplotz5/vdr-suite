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
  if (!nav) return null;

  let button = nav.querySelector('[data-module="channels2"]');
  if (!button) {
    button = document.createElement('button');
    button.type = 'button';
    button.className = 'module-tab';
    button.dataset.module = 'channels2';
    button.textContent = 'Channels 2';

    const channels = nav.querySelector('[data-module="channels"]');
    if (channels && channels.nextSibling) nav.insertBefore(button, channels.nextSibling);
    else nav.appendChild(button);
  }

  button.disabled = true;
  button.title = 'Channels 2 wird geladen …';
  return button;
}

function showChannels2RuntimeError(button, error) {
  if (button) {
    button.disabled = false;
    button.title = 'Channels 2 konnte nicht geladen werden';
  }

  const target = document.getElementById('detail-data');
  if (!target) return;

  const message = document.createElement('p');
  message.className = 'status error';
  message.textContent = 'Channels 2 konnte nicht geladen werden: ' +
    (error && error.message ? error.message : String(error));
  target.replaceChildren(message);
}

function bindVdrSuiteChannels2Tab(button) {
  if (!button || button.dataset.channels2ReadyBound === 'true') return;

  button.dataset.channels2ReadyBound = 'true';
  button.disabled = false;
  button.title = 'Neue Kanalansicht öffnen';

  button.addEventListener('click', event => {
    event.preventDefault();
    event.stopImmediatePropagation();

    const moduleApi = window.VdrSuiteChannels2;
    if (!moduleApi || typeof moduleApi.activate !== 'function') {
      showChannels2RuntimeError(button, new Error('Runtime API ist nicht verfügbar'));
      return;
    }

    try {
      moduleApi.activate();
    } catch (error) {
      console.error('VDR-Suite Channels 2 activation failed', error);
      showChannels2RuntimeError(button, error);
    }
  }, true);
}

function startVdrSuiteDeferredFrontendRuntimes() {
  const channels2Button = ensureVdrSuiteChannels2Tab();

  loadVdrSuiteDeferredRuntime(
    'vdr-suite-channels2-runtime',
    '/frontend/channel-day-program.js',
    () => Boolean(window.VdrSuiteChannels2)
  ).then(() => {
    if (!window.VdrSuiteChannels2 ||
        typeof window.VdrSuiteChannels2.activate !== 'function') {
      throw new Error('channel-day-program.js loaded without VdrSuiteChannels2 API');
    }
    bindVdrSuiteChannels2Tab(channels2Button);
  }).catch(error => {
    console.error('VDR-Suite Channels 2 runtime failed', error);
    showChannels2RuntimeError(channels2Button, error);
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
  if (document.readyState === 'complete') {
    startVdrSuiteDeferredFrontendRuntimes();
  } else {
    window.addEventListener('load', startVdrSuiteDeferredFrontendRuntimes, {once: true});
  }
}
