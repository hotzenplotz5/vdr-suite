(function (global) {
  'use strict';

  const actions = [
    'up', 'down', 'left', 'right', 'ok', 'back', 'menu', 'info',
    'red', 'green', 'yellow', 'blue',
    'zero', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine',
    'channelUp', 'channelDown', 'volumeUp', 'volumeDown', 'mute',
    'play', 'pause', 'stop', 'record', 'fastForward', 'rewind', 'next', 'previous',
    'switchChannel'
  ];
  const actionSet = new Set(actions);
  const tabOrder = [
    'overview', 'channels2', 'recordings2', 'genres',
    'epg', 'channelsort', 'timers', 'searchtimers'
  ];
  const remoteImagePath =
    '/channel-logos/vdr-suite-brand/vdr-remote-photorealistic.svg';

  const labels = {
    up: 'Nach oben',
    down: 'Nach unten',
    left: 'Nach links',
    right: 'Nach rechts',
    ok: 'OK',
    back: 'Zurück',
    menu: 'Menü',
    info: 'Info',
    red: 'Rot',
    green: 'Grün',
    yellow: 'Gelb',
    blue: 'Blau',
    zero: '0',
    one: '1',
    two: '2',
    three: '3',
    four: '4',
    five: '5',
    six: '6',
    seven: '7',
    eight: '8',
    nine: '9',
    channelUp: 'Kanal aufwärts',
    channelDown: 'Kanal abwärts',
    volumeUp: 'Lautstärke erhöhen',
    volumeDown: 'Lautstärke verringern',
    mute: 'Stumm',
    play: 'Wiedergabe',
    pause: 'Pause',
    stop: 'Stopp',
    record: 'Aufnahme',
    fastForward: 'Vorspulen',
    rewind: 'Zurückspulen',
    next: 'Weiter',
    previous: 'Zurück'
  };

  // Every visible key in the SVG has exactly one matching hotspot here.
  const hotspots = [
  ['menu', 9.444, 8.033, 18.056, 3.443, '999px'],
  ['info', 30, 8.033, 18.056, 3.443, '999px'],
  ['back', 50.556, 8.033, 18.056, 3.443, '999px'],
  ['mute', 71.111, 8.033, 18.056, 3.443, '999px'],
  ['previous', 9.444, 13.77, 18.056, 3.443, '999px'],
  ['rewind', 30, 13.77, 18.056, 3.443, '999px'],
  ['play', 50.556, 13.77, 18.056, 3.443, '999px'],
  ['pause', 71.111, 13.77, 18.056, 3.443, '999px'],
  ['record', 9.444, 18.197, 18.056, 3.443, '999px'],
  ['stop', 30, 18.197, 18.056, 3.443, '999px'],
  ['fastForward', 50.556, 18.197, 18.056, 3.443, '999px'],
  ['next', 71.111, 18.197, 18.056, 3.443, '999px'],
  ['up', 30.556, 25.738, 38.889, 5.902, '30%'],
  ['left', 16.111, 31.639, 25.556, 10.164, '30%'],
  ['ok', 35, 31.967, 30, 8.852, '50%'],
  ['right', 58.333, 31.639, 25.556, 10.164, '30%'],
  ['down', 30.556, 41.803, 38.889, 5.902, '30%'],
  ['volumeUp', 11.111, 51.803, 25, 6.721, '34%'],
  ['volumeDown', 11.111, 58.525, 25, 6.721, '34%'],
  ['channelUp', 63.889, 51.803, 25, 6.721, '34%'],
  ['channelDown', 63.889, 58.525, 25, 6.721, '34%'],
  ['red', 9.722, 69.016, 18.056, 2.787, '999px'],
  ['green', 30.556, 69.016, 18.056, 2.787, '999px'],
  ['yellow', 51.389, 69.016, 18.056, 2.787, '999px'],
  ['blue', 72.222, 69.016, 18.056, 2.787, '999px'],
  ['one', 9.722, 74.754, 19.444, 3.934, '999px'],
  ['two', 40.278, 74.754, 19.444, 3.934, '999px'],
  ['three', 70.833, 74.754, 19.444, 3.934, '999px'],
  ['four', 9.722, 79.836, 19.444, 3.934, '999px'],
  ['five', 40.278, 79.836, 19.444, 3.934, '999px'],
  ['six', 70.833, 79.836, 19.444, 3.934, '999px'],
  ['seven', 9.722, 84.918, 19.444, 3.934, '999px'],
  ['eight', 40.278, 84.918, 19.444, 3.934, '999px'],
  ['nine', 70.833, 84.918, 19.444, 3.934, '999px'],
  ['zero', 40.278, 90, 19.444, 3.934, '999px']
  ];

  let dialog = null;
  let operationCounter = 0;
  let liveSource = null;
  let lastSequenceNumber = 0;
  let actionInFlight = false;

  function clientApi() {
    if (global.VdrSuitePlatform &&
        global.VdrSuitePlatform.getClientApi) {
      return global.VdrSuitePlatform.getClientApi();
    }
    return global.VdrSuiteClientApi;
  }

  function createElement(tagName, className, text) {
    const element = document.createElement(tagName);
    if (className) {
      element.className = className;
    }
    if (text !== undefined) {
      element.textContent = text;
    }
    return element;
  }

  function selectedBackendId() {
    const platform = global.VdrSuitePlatform;
    const selected = platform &&
      platform.getSelectedBackendId &&
      String(platform.getSelectedBackendId() || '').trim();
    if (selected) {
      return selected;
    }

    const card = document.querySelector('.backend-card.selected');
    return card && card.dataset.backendId
      ? card.dataset.backendId
      : 'default';
  }

  function fetchSelectedBackend() {
    const api = clientApi();
    if (!api || !api.fetchClientBackends) {
      return Promise.reject(new Error('Backend-Client-API fehlt.'));
    }

    return api.fetchClientBackends({cache: 'no-store'}).then(function (result) {
      return (result.backends || []).find(function (backend) {
        const selector = backend.frontendSelector || backend;
        return String(selector.id || backend.backendId) === selectedBackendId();
      });
    });
  }

  function controlState(backend) {
    if (!backend) {
      return [false, 'Backend nicht gefunden.'];
    }

    const selector = backend.frontendSelector || backend;
    const capabilities = backend.capabilities || {};
    const readOnly =
      backend.readOnly ||
      selector.readOnly ||
      String(backend.accessMode || selector.accessMode) === 'read-only';

    if (readOnly || selector.canWrite === false) {
      return [false, 'Read-only: Steuerung ist serverseitig gesperrt.'];
    }
    if (backend.enabled === false) {
      return [false, 'Backend deaktiviert.'];
    }
    if (backend.online === false) {
      return [false, 'Backend offline.'];
    }

    const available =
      backend.canRemoteControl ||
      selector.canRemoteControl ||
      capabilities.remoteControl;
    return available
      ? [true, 'Fernbedienung verfügbar.']
      : [false, 'Capability remote.control nicht verfügbar.'];
  }

  function sendAction(action, channelId) {
    if (!actionSet.has(action)) {
      return Promise.reject(
        new Error('Nicht erlaubte Fernsteuerungsaktion.')
      );
    }

    const api = clientApi();
    if (!api || !api.fetchClientRemoteAction) {
      return Promise.reject(
        new Error('Fernsteuerungs-Client-API fehlt.')
      );
    }

    return fetchSelectedBackend().then(function (backend) {
      const state = controlState(backend);
      if (!state[0]) {
        throw new Error(state[1]);
      }

      const payload = {
        backendId: selectedBackendId(),
        operationId:
          'remote-' + Date.now() + '-' + (++operationCounter),
        action: action
      };

      if (action === 'switchChannel') {
        payload.channelId = String(channelId || '').trim();
        if (!payload.channelId) {
          throw new Error('VDR-Kanal-ID fehlt.');
        }
      }

      return api.fetchClientRemoteAction({payload: payload});
    });
  }

  function fetchOverlay() {
    const api = clientApi();
    if (!api || !api.fetchClientLiveOverlay) {
      return Promise.reject(
        new Error('Live-Overlay-Client-API fehlt.')
      );
    }
    return api.fetchClientLiveOverlay({backendId: selectedBackendId()});
  }

  function formatEvent(event) {
    if (!event || !event.available) {
      return 'nicht verfügbar';
    }

    function formatTime(value) {
      return new Date(value * 1000).toLocaleTimeString(
        'de-DE',
        {hour: '2-digit', minute: '2-digit'}
      );
    }

    let prefix = '';
    if (event.startTime) {
      prefix = formatTime(event.startTime);
      if (event.endTime > event.startTime) {
        prefix += '–' + formatTime(event.endTime);
      }
      prefix += ' · ';
    }
    return prefix + (event.title || 'Ohne Titel');
  }

  function setStatus(message, kind) {
    const element = dialog && dialog.querySelector('[data-status]');
    if (!element) {
      return;
    }
    element.className = 'remote-status ' + (kind || '');
    element.textContent = message;
  }

  function setControlsEnabled(enabled) {
    if (!dialog) {
      return;
    }
    dialog.querySelectorAll('[data-action]').forEach(function (element) {
      element.setAttribute('aria-disabled', enabled ? 'false' : 'true');
      element.classList.toggle('is-disabled', !enabled);
    });
  }

  function showOverlay(overlay) {
    const element = dialog && dialog.querySelector('[data-overlay]');
    if (!element) {
      return;
    }

    const channel = overlay && overlay.channel
      ? overlay.channel
      : {};
    const title = channel.available
      ? [channel.number, channel.name || channel.id]
          .filter(Boolean)
          .join(' · ')
      : 'Live-TV-Status nicht verfügbar';

    element.replaceChildren(
      createElement('strong', '', title),
      createElement(
        'span',
        '',
        'Jetzt: ' + formatEvent(overlay && overlay.present)
      )
    );
  }

  function refreshOverlay() {
    return fetchOverlay()
      .then(showOverlay)
      .catch(function (error) {
        setStatus(error.message, 'error');
      });
  }

  function stopSubscription() {
    if (liveSource && liveSource.close) {
      liveSource.close();
    }
    liveSource = null;
  }

  function subscribe() {
    stopSubscription();

    const api = clientApi();
    if (!api || !api.createClientLiveUpdateSource) {
      return;
    }

    liveSource = api.createClientLiveUpdateSource();
    if (!liveSource) {
      return;
    }

    liveSource.onmessage = function (message) {
      let update;
      try {
        update = JSON.parse(message.data || '{}');
      } catch (error) {
        return;
      }

      const sequenceNumber = Number(update.sequenceNumber) || 0;
      if (sequenceNumber && sequenceNumber <= lastSequenceNumber) {
        return;
      }
      if (sequenceNumber) {
        lastSequenceNumber = sequenceNumber;
      }

      if (String(update.backendId || 'default') === selectedBackendId() &&
          (update.changedDomains || []).includes('liveOverlay')) {
        refreshOverlay();
      }
    };
  }

  function releaseKey(element) {
    if (element) {
      element.classList.remove('is-pressed');
    }
  }

  function attachPressFeedback(element) {
    element.addEventListener('pointerdown', function (event) {
      if (element.getAttribute('aria-disabled') === 'true') {
        return;
      }
      element.classList.add('is-pressed');
      if (element.setPointerCapture && event.pointerId !== undefined) {
        element.setPointerCapture(event.pointerId);
      }
    });

    [
      'pointerup',
      'pointercancel',
      'lostpointercapture',
      'pointerleave'
    ].forEach(function (eventName) {
      element.addEventListener(eventName, function () {
        releaseKey(element);
      });
    });

    element.addEventListener('keydown', function (event) {
      if ((event.key === 'Enter' || event.key === ' ') &&
          element.getAttribute('aria-disabled') !== 'true') {
        element.classList.add('is-pressed');
      }
    });
    element.addEventListener('keyup', function () {
      releaseKey(element);
    });
    element.addEventListener('blur', function () {
      releaseKey(element);
    });
  }

  function createHotspot(definition) {
    const element = createElement('button', 'remote-key');
    element.type = 'button';
    element.dataset.action = definition[0];
    element.setAttribute(
      'aria-label',
      labels[definition[0]] || definition[0]
    );
    element.style.left = definition[1] + '%';
    element.style.top = definition[2] + '%';
    element.style.width = definition[3] + '%';
    element.style.height = definition[4] + '%';
    element.style.borderRadius = definition[5];
    attachPressFeedback(element);
    return element;
  }

  function appendRemoteStage(parent) {
    const stage = createElement('section', 'remote-stage');
    const image = createElement('img', 'remote-image');
    stage.setAttribute(
      'aria-label',
      'Fotorealistische VDR-Fernbedienung'
    );
    image.src = remoteImagePath;
    image.alt = '';
    image.draggable = false;

    stage.append(image);
    hotspots.forEach(function (definition) {
      stage.append(createHotspot(definition));
    });
    parent.append(stage);
  }

  function appendDirectChannel(parent) {
    const section = createElement('section', 'remote-direct');
    const input = createElement('input', 'remote-channel-input');
    const button = createElement(
      'button',
      'remote-channel-button',
      'Kanal wechseln'
    );

    input.placeholder = 'VDR-Kanal-ID';
    input.inputMode = 'numeric';
    input.setAttribute('aria-label', 'VDR-Kanal-ID');

    button.type = 'button';
    button.dataset.action = 'switchChannel';
    button.setAttribute('aria-label', 'Direkten Kanal wechseln');
    attachPressFeedback(button);

    section.append(input, button);
    parent.append(section);
    return input;
  }

  function dispatchAction(element, channelId) {
    if (!element ||
        element.getAttribute('aria-disabled') === 'true' ||
        actionInFlight) {
      return;
    }

    actionInFlight = true;
    element.classList.add('is-sending');
    setStatus('Sende ' + element.dataset.action + ' …');

    sendAction(element.dataset.action, channelId)
      .then(function (result) {
        setStatus(
          result.message || 'Aktion ausgeführt.',
          'success'
        );
        return refreshOverlay();
      })
      .catch(function (error) {
        setStatus(error.message, 'error');
      })
      .finally(function () {
        actionInFlight = false;
        element.classList.remove('is-sending');
        releaseKey(element);
      });
  }

  function buildDialog() {
    const modal = createElement('dialog', 'remote-dialog');
    const panel = createElement('section', 'remote-panel');
    const header = createElement('header', 'remote-header');
    const title = createElement('h2', '', 'VDR - Fernbedienung');
    const close = createElement('button', 'remote-close', '×');

    modal.setAttribute('aria-label', 'VDR - Fernbedienung');
    close.type = 'button';
    close.setAttribute('aria-label', 'Fernbedienung schließen');
    close.onclick = function () {
      modal.close();
    };

    header.append(title, close);
    panel.append(header);

    const overlay = createElement('section', 'remote-overlay');
    overlay.dataset.overlay = '1';
    panel.append(overlay);

    const status = createElement('p', 'remote-status', '');
    status.dataset.status = '1';
    panel.append(status);

    appendRemoteStage(panel);
    const channelInput = appendDirectChannel(panel);

    panel.addEventListener('click', function (event) {
      const element = event.target.closest &&
        event.target.closest('[data-action]');
      if (element) {
        dispatchAction(element, channelInput.value);
      }
    });

    modal.addEventListener('close', stopSubscription);
    modal.append(panel);
    document.body.append(modal);
    return modal;
  }

  function open() {
    if (!dialog) {
      dialog = buildDialog();
    }

    fetchSelectedBackend()
      .then(function (backend) {
        const state = controlState(backend);
        setStatus(state[1], state[0] ? 'success' : 'warning');
        setControlsEnabled(state[0]);
      })
      .catch(function (error) {
        setStatus(error.message, 'error');
        setControlsEnabled(false);
      });

    refreshOverlay();
    subscribe();

    if (dialog.showModal) {
      dialog.showModal();
    } else {
      dialog.setAttribute('open', '');
    }
  }

  function installStyles() {
    if (document.getElementById('vdr-suite-remote-style')) {
      return;
    }

    const style = createElement('style');
    style.id = 'vdr-suite-remote-style';
    style.textContent = `
.brand-feature-remote {
  cursor: pointer;
}
.brand-feature-remote:hover,
.brand-feature-remote:focus {
  border-color: rgba(56, 189, 248, .42);
  background: rgba(14, 165, 233, .12);
  outline: 0;
}
.remote-dialog {
  box-sizing: border-box;
  width: min(44rem, 100vw);
  height: 100dvh;
  max-width: none;
  max-height: none;
  padding: 0;
  margin: auto;
  border: 0;
  background: transparent;
  color: #e5edf9;
  overflow: hidden;
}
.remote-dialog::backdrop {
  background: rgba(2, 6, 23, .92);
  backdrop-filter: blur(5px);
}
.remote-panel {
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  gap: .55rem;
  width: 100%;
  height: 100%;
  padding: .65rem .8rem 1rem;
  overflow-x: hidden;
  overflow-y: auto;
  overscroll-behavior: contain;
  scrollbar-gutter: stable;
  background:
    radial-gradient(circle at 50% 28%, rgba(25, 74, 135, .24), transparent 34%),
    linear-gradient(180deg, #061126 0%, #020817 58%, #01040c 100%);
}
.remote-header {
  position: sticky;
  top: 0;
  z-index: 10;
  display: grid;
  grid-template-columns: 1fr auto;
  align-items: center;
  gap: .75rem;
  min-height: 3.2rem;
  padding: .2rem .25rem .35rem;
  background: linear-gradient(
    180deg,
    rgba(6, 17, 38, .99),
    rgba(6, 17, 38, .93)
  );
}
.remote-header h2 {
  margin: 0;
  padding-left: 3rem;
  color: #dbeafe;
  font-size: 1.12rem;
  font-weight: 800;
  letter-spacing: .045em;
  text-align: center;
}
.remote-close {
  width: 2.8rem;
  min-height: 2.8rem;
  padding: 0;
  border: 1px solid rgba(148, 163, 184, .55);
  border-radius: 50%;
  background: radial-gradient(
    circle at 35% 28%,
    #4b5563,
    #111827 58%,
    #020617
  );
  color: #fff;
  font: inherit;
  font-size: 1.25rem;
  font-weight: 800;
  box-shadow:
    0 .35rem .8rem rgba(0, 0, 0, .55),
    inset 0 1px 1px rgba(255, 255, 255, .24);
  cursor: pointer;
}
.remote-overlay,
.remote-status {
  box-sizing: border-box;
  flex: 0 0 auto;
  width: min(30rem, calc(100vw - 2rem));
  margin: 0 auto;
  border: 1px solid rgba(59, 130, 246, .32);
  border-radius: .65rem;
  background: rgba(2, 12, 29, .78);
  box-shadow: inset 0 .15rem .42rem rgba(0, 0, 0, .72);
}
.remote-overlay {
  display: grid;
  gap: .1rem;
  padding: .48rem .65rem;
}
.remote-overlay strong {
  color: #bfdbfe;
  font-size: .86rem;
}
.remote-overlay span {
  color: #aebbd0;
  font-size: .73rem;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.remote-status {
  min-height: 2rem;
  padding: .42rem .65rem;
  color: #cbd5e1;
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: .76rem;
  text-align: center;
}
.remote-status.success {
  color: #6ee7b7;
}
.remote-status.warning {
  color: #fde68a;
}
.remote-status.error {
  color: #fca5a5;
}
.remote-stage {
  position: relative;
  flex: 0 0 auto;
  width: min(27rem, calc(100vw - 1rem));
  aspect-ratio: 360 / 1220;
  margin: .2rem auto 0;
  isolation: isolate;
  filter: drop-shadow(0 1.2rem 1.5rem rgba(0, 0, 0, .5));
}
.remote-image {
  display: block;
  width: 100%;
  height: 100%;
  object-fit: contain;
  pointer-events: none;
  user-select: none;
  -webkit-user-drag: none;
}
.remote-key {
  position: absolute;
  z-index: 2;
  display: block;
  min-width: 0;
  min-height: 0;
  padding: 0;
  border: 0;
  background: transparent;
  color: transparent;
  touch-action: manipulation;
  user-select: none;
  cursor: pointer;
  outline: 0;
  -webkit-tap-highlight-color: transparent;
}
.remote-key::before {
  content: "";
  position: absolute;
  inset: 0;
  border-radius: inherit;
  opacity: 0;
  pointer-events: none;
  transition:
    opacity .06s,
    transform .06s,
    box-shadow .06s,
    background .06s;
}
.remote-key.is-pressed::before,
.remote-key:not(.is-disabled):active::before {
  opacity: 1;
  transform: translateY(2px) scale(.985);
  background: rgba(0, 0, 0, .22);
  box-shadow:
    inset 0 .38rem .65rem rgba(0, 0, 0, .82),
    inset 0 1px 0 rgba(255, 255, 255, .06);
}
.remote-key.is-sending {
  opacity: .96;
}
.remote-key:focus-visible::before {
  opacity: 1;
  box-shadow: 0 0 0 2px rgba(255, 255, 255, .9);
  background: rgba(255, 255, 255, .025);
}
.remote-key.is-disabled {
  cursor: default;
}
.remote-key.is-disabled::before {
  opacity: .2;
  background: rgba(15, 23, 42, .62);
}
.remote-direct {
  display: grid;
  grid-template-columns: 1fr auto;
  gap: .45rem;
  flex: 0 0 auto;
  width: min(30rem, calc(100vw - 2rem));
  margin: .25rem auto 0;
  padding-bottom: .5rem;
}
.remote-channel-input,
.remote-channel-button {
  box-sizing: border-box;
  min-height: 2.65rem;
  border: 1px solid #334155;
  border-radius: 999px;
  font: inherit;
}
.remote-channel-input {
  min-width: 0;
  padding: 0 .85rem;
  background: rgba(2, 6, 23, .9);
  color: #f8fafc;
  box-shadow: inset 0 .18rem .4rem rgba(0, 0, 0, .75);
}
.remote-channel-button {
  padding: 0 .9rem;
  background: linear-gradient(145deg, #1e3a5f, #0f172a);
  color: #dbeafe;
  font-size: .78rem;
  font-weight: 800;
  box-shadow:
    0 .28rem .55rem rgba(0, 0, 0, .5),
    inset 0 1px 0 rgba(255, 255, 255, .12);
  cursor: pointer;
}
.remote-channel-button.is-pressed {
  transform: translateY(2px);
  box-shadow: inset 0 .28rem .5rem rgba(0, 0, 0, .78);
}
@media (max-width: 620px) {
  .remote-dialog {
    width: 100vw;
    height: 100dvh;
  }
  .remote-panel {
    gap: .42rem;
    padding: .45rem .35rem .8rem;
  }
  .remote-header {
    min-height: 2.8rem;
  }
  .remote-header h2 {
    padding-left: 2.7rem;
    font-size: 1rem;
  }
  .remote-close {
    width: 2.55rem;
    min-height: 2.55rem;
  }
  .remote-stage {
    width: min(25rem, calc(100vw - .7rem));
  }
}
`;
    document.head.appendChild(style);
  }

  function setup() {
    installStyles();

    const navigation = document.getElementById('module-nav');
    if (navigation) {
      tabOrder.forEach(function (moduleName) {
        const item = navigation.querySelector(
          '[data-module="' + moduleName + '"]'
        );
        if (item) {
          navigation.append(item);
        }
      });
    }

    const features = document.querySelectorAll(
      '.brand-feature-strip > .brand-feature'
    );

    if (features[1]) {
      const text = features[1].querySelector('.brand-feature-text');
      if (text) {
        text.removeAttribute('data-i18n');
        text.textContent = 'Overlay vorbereitet · Streaming folgt';
      }
    }

    if (features[3]) {
      const feature = features[3];
      const title = feature.querySelector('.brand-feature-title');
      const text = feature.querySelector('.brand-feature-text');

      feature.tabIndex = 0;
      feature.setAttribute('role', 'button');
      feature.setAttribute(
        'aria-label',
        'VDR - Fernbedienung öffnen'
      );
      feature.classList.add('brand-feature-remote');

      if (title) {
        title.removeAttribute('data-i18n');
        title.textContent = 'VDR - Fernbedienung';
      }
      if (text) {
        text.removeAttribute('data-i18n');
        text.textContent = 'Groß · scrollbar · vollständig bedienbar';
      }

      feature.onclick = open;
      feature.onkeydown = function (event) {
        if (event.key === 'Enter' || event.key === ' ') {
          event.preventDefault();
          open();
        }
      };
    }
  }

  setup();

  const remoteModule = Object.freeze({
    open: open,
    actions: actions,
    hotspotActions: hotspots.map(function (entry) { return entry[0]; }),
    tabOrder: tabOrder,
    remoteImagePath: remoteImagePath,
    canControlBackend: function (backend) {
      return Boolean(controlState(backend)[0]);
    }
  });

  if (global.VdrSuitePlatform &&
      global.VdrSuitePlatform.registerModule &&
      (!global.VdrSuitePlatform.hasModule ||
       !global.VdrSuitePlatform.hasModule('remote'))) {
    global.VdrSuitePlatform.registerModule('remote', remoteModule);
  }

  global.VdrSuiteRemote = remoteModule;
})(window);
