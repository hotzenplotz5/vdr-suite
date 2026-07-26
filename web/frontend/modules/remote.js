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
  const allowedActions = new Set(actions);
  const tabOrder = [
    'overview', 'channels2', 'recordings2', 'genres', 'epg',
    'channelsort', 'timers', 'searchtimers'
  ];

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
    next: 'Nächste',
    previous: 'Vorherige'
  };

  const symbols = {
    up: '▲',
    down: '▼',
    left: '◀',
    right: '▶',
    ok: 'OK',
    back: '↩',
    menu: 'MENU',
    info: 'INFO',
    red: '',
    green: '',
    yellow: '',
    blue: '',
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
    channelUp: '+\nCH',
    channelDown: '−\nCH',
    volumeUp: '+\nVOL',
    volumeDown: '−\nVOL',
    mute: 'MUTE',
    play: '▶',
    pause: 'Ⅱ',
    stop: '■',
    record: '●',
    fastForward: '▶▶',
    rewind: '◀◀',
    next: '▶|',
    previous: '|◀'
  };

  let dialog = null;
  let operationSequence = 0;
  let liveSource = null;
  let liveSequence = 0;

  function clientApi() {
    if (global.VdrSuitePlatform && global.VdrSuitePlatform.getClientApi) {
      return global.VdrSuitePlatform.getClientApi();
    }
    return global.VdrSuiteClientApi;
  }

  function selectedBackendId() {
    const platform = global.VdrSuitePlatform;
    const selected = platform && platform.getSelectedBackendId &&
      String(platform.getSelectedBackendId() || '').trim();
    if (selected) {
      return selected;
    }

    const card = document.querySelector('.backend-card.selected');
    return card && card.dataset.backendId || 'default';
  }

  function selectedBackend() {
    const api = clientApi();
    if (!api || !api.fetchClientBackends) {
      return Promise.reject(Error('Backend-Client-API fehlt.'));
    }

    return api.fetchClientBackends({cache: 'no-store'}).then(function (response) {
      return (response.backends || []).find(function (backend) {
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
    const readOnly = backend.readOnly || selector.readOnly ||
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

    return backend.canRemoteControl || selector.canRemoteControl || capabilities.remoteControl
      ? [true, 'Fernbedienung verfügbar.']
      : [false, 'Capability remote.control nicht verfügbar.'];
  }

  function sendAction(action, channelId) {
    if (!allowedActions.has(action)) {
      return Promise.reject(Error('Nicht erlaubte Fernsteuerungsaktion.'));
    }

    const api = clientApi();
    if (!api || !api.fetchClientRemoteAction) {
      return Promise.reject(Error('Fernsteuerungs-Client-API fehlt.'));
    }

    return selectedBackend().then(function (backend) {
      const state = controlState(backend);
      if (!state[0]) {
        throw Error(state[1]);
      }

      const payload = {
        backendId: selectedBackendId(),
        operationId: 'remote-' + Date.now() + '-' + (++operationSequence),
        action: action
      };

      if (action === 'switchChannel') {
        payload.channelId = String(channelId || '').trim();
        if (!payload.channelId) {
          throw Error('VDR-Kanal-ID fehlt.');
        }
      }

      return api.fetchClientRemoteAction({payload: payload});
    });
  }

  function fetchOverlay() {
    const api = clientApi();
    return api && api.fetchClientLiveOverlay
      ? api.fetchClientLiveOverlay({backendId: selectedBackendId()})
      : Promise.reject(Error('Live-Overlay-Client-API fehlt.'));
  }

  function element(tagName, className, text) {
    const node = document.createElement(tagName);
    if (className) {
      node.className = className;
    }
    if (text !== undefined) {
      node.textContent = text;
    }
    return node;
  }

  function formatEvent(event) {
    if (!event || !event.available) {
      return 'nicht verfügbar';
    }

    const formatTime = function (value) {
      return new Date(value * 1000).toLocaleTimeString('de-DE', {
        hour: '2-digit',
        minute: '2-digit'
      });
    };

    const time = event.startTime
      ? formatTime(event.startTime) +
        (event.endTime > event.startTime ? '–' + formatTime(event.endTime) : '') + ' · '
      : '';
    return time + (event.title || 'Ohne Titel');
  }

  function setStatus(message, kind) {
    const node = dialog && dialog.querySelector('[data-status]');
    if (node) {
      node.className = 'r-status ' + (kind || '');
      node.textContent = message;
    }
  }

  function lockControls(locked) {
    if (dialog) {
      dialog.querySelectorAll('[data-action]').forEach(function (button) {
        button.disabled = locked;
      });
    }
  }

  function showOverlay(data) {
    const target = dialog && dialog.querySelector('[data-overlay]');
    const channel = data && data.channel || {};
    const timer = data && data.timer || {};
    if (!target) {
      return;
    }

    target.replaceChildren(
      element(
        'h3',
        '',
        channel.available
          ? [channel.number, channel.name || channel.id].filter(Boolean).join(' · ')
          : 'Live-TV-Status nicht verfügbar'
      ),
      element('p', '', 'Jetzt: ' + formatEvent(data && data.present)),
      element('p', '', 'Danach: ' + formatEvent(data && data.following)),
      element(
        'p',
        '',
        'Timer: ' + (timer.recording ? 'Aufnahme läuft' : timer.active ? 'aktiv' : 'inaktiv')
      )
    );
  }

  function refreshOverlay() {
    return fetchOverlay().then(showOverlay).catch(function (error) {
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

      const sequence = Number(update.sequenceNumber) || 0;
      if (sequence && sequence <= liveSequence) {
        return;
      }
      if (sequence) {
        liveSequence = sequence;
      }

      if (
        String(update.backendId || 'default') === selectedBackendId() &&
        (update.changedDomains || []).includes('liveOverlay')
      ) {
        refreshOverlay();
      }
    };
  }

  function remoteKey(action, extraClass) {
    const button = element(
      'button',
      'r-key ' + action + (extraClass ? ' ' + extraClass : ''),
      symbols[action]
    );
    button.type = 'button';
    button.dataset.action = action;
    button.setAttribute('aria-label', labels[action]);
    button.title = labels[action];
    return button;
  }

  function appendKeys(container, keyActions, extraClass) {
    keyActions.forEach(function (action) {
      container.append(remoteKey(action, extraClass));
    });
  }

  function buildTransport(remoteBody) {
    const section = element('section', 'r-section r-transport');
    const seek = element('div', 'r-transport-row r-transport-seek');
    appendKeys(seek, ['previous', 'rewind', 'play', 'fastForward', 'next'], 'r-key-transport');

    const media = element('div', 'r-transport-row r-transport-media');
    appendKeys(media, ['record', 'pause', 'stop'], 'r-key-transport');

    section.append(seek, media);
    remoteBody.append(section);
  }

  function buildNavigation(remoteBody) {
    const section = element('section', 'r-section r-navigation');
    const tools = element('div', 'r-tool-row');
    appendKeys(tools, ['info', 'menu', 'back'], 'r-key-tool');

    const dpad = element('div', 'r-dpad');
    appendKeys(dpad, ['up', 'left', 'ok', 'right', 'down'], 'r-key-dpad');

    section.append(tools, dpad);
    remoteBody.append(section);
  }

  function buildRockerControls(remoteBody) {
    const section = element('section', 'r-section r-rocker-grid');

    const volume = element('div', 'r-rocker r-rocker-volume');
    volume.append(
      remoteKey('volumeUp', 'r-key-rocker'),
      remoteKey('volumeDown', 'r-key-rocker')
    );

    const mute = remoteKey('mute', 'r-key-mute');

    const channel = element('div', 'r-rocker r-rocker-channel');
    channel.append(
      remoteKey('channelUp', 'r-key-rocker'),
      remoteKey('channelDown', 'r-key-rocker')
    );

    section.append(volume, mute, channel);
    remoteBody.append(section);
  }

  function buildColorKeys(remoteBody) {
    const section = element('section', 'r-section r-color-grid');
    appendKeys(section, ['red', 'green', 'yellow', 'blue'], 'r-key-color');
    remoteBody.append(section);
  }

  function buildNumberKeys(remoteBody) {
    const section = element('section', 'r-section r-number-grid');
    appendKeys(
      section,
      ['one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine', 'zero'],
      'r-key-number'
    );
    remoteBody.append(section);
  }

  function buildDirectChannel(remoteBody) {
    const section = element('section', 'r-section r-direct-channel');
    const label = element('label', 'r-direct-label', 'Direkter Kanalwechsel');
    const row = element('div', 'r-switch');
    const input = element('input', 'r-input');
    const submit = element('button', 'r-key r-key-switch', 'Wechseln');

    input.placeholder = 'VDR-Kanal-ID';
    input.inputMode = 'numeric';
    input.setAttribute('aria-label', 'VDR-Kanal-ID');
    submit.type = 'button';
    submit.dataset.action = 'switchChannel';
    row.append(input, submit);
    section.append(label, row);
    remoteBody.append(section);
    return input;
  }

  function buildDialog() {
    const popup = element('dialog', 'r-dialog');
    const remoteBody = element('section', 'r-remote-body');
    const header = element('header', 'r-head');
    const title = element('h2', '', 'VDR - Fernbedienung');
    const close = element('button', 'r-key r-close', '×');

    popup.setAttribute('aria-label', 'VDR - Fernbedienung');
    close.type = 'button';
    close.setAttribute('aria-label', 'Fernbedienung schließen');
    close.title = 'Schließen';
    close.onclick = function () {
      popup.close();
    };

    header.append(title, close);
    remoteBody.append(header);

    const status = element('p', 'r-status', '');
    status.dataset.status = '1';
    remoteBody.append(status);

    const overlay = element('section', 'r-overlay');
    overlay.dataset.overlay = '1';
    remoteBody.append(overlay);

    buildTransport(remoteBody);
    buildNavigation(remoteBody);
    buildRockerControls(remoteBody);
    buildColorKeys(remoteBody);
    buildNumberKeys(remoteBody);
    const channelInput = buildDirectChannel(remoteBody);

    remoteBody.onclick = function (event) {
      const button = event.target.closest && event.target.closest('[data-action]');
      if (!button || button.disabled) {
        return;
      }

      lockControls(true);
      setStatus('Sende ' + button.dataset.action + ' …');
      sendAction(button.dataset.action, channelInput.value)
        .then(function (response) {
          setStatus(response.message || 'Aktion ausgeführt.', 'success');
          return refreshOverlay();
        })
        .catch(function (error) {
          setStatus(error.message, 'error');
        })
        .finally(function () {
          selectedBackend()
            .then(function (backend) {
              lockControls(!controlState(backend)[0]);
            })
            .catch(function () {
              lockControls(true);
            });
        });
    };

    popup.addEventListener('close', stopSubscription);
    popup.append(remoteBody);
    document.body.append(popup);
    return popup;
  }

  function open() {
    if (!dialog) {
      dialog = buildDialog();
    }

    selectedBackend()
      .then(function (backend) {
        const state = controlState(backend);
        setStatus(state[1], state[0] ? 'success' : 'warning');
        lockControls(!state[0]);
      })
      .catch(function (error) {
        setStatus(error.message, 'error');
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
    if (document.getElementById('r-style')) {
      return;
    }

    const style = element('style');
    style.id = 'r-style';
    style.textContent = `
      .brand-feature-remote {
        cursor: pointer;
      }
      .brand-feature-remote:hover,
      .brand-feature-remote:focus {
        border-color: rgba(56, 189, 248, .42);
        background: rgba(14, 165, 233, .12);
        outline: none;
      }
      .r-dialog {
        width: min(25rem, calc(100vw - .8rem));
        max-height: calc(100dvh - .8rem);
        padding: 0;
        margin: auto;
        border: 0;
        background: transparent;
        color: #f8fafc;
        overflow: visible;
      }
      .r-dialog::backdrop {
        background: rgba(2, 6, 23, .82);
        backdrop-filter: blur(4px);
      }
      .r-remote-body {
        box-sizing: border-box;
        display: grid;
        gap: .72rem;
        width: 100%;
        max-height: calc(100dvh - .8rem);
        padding: .9rem .9rem 1.25rem;
        overflow: auto;
        overscroll-behavior: contain;
        border: 1px solid #4b5563;
        border-radius: 2.2rem;
        background:
          radial-gradient(circle at 50% 0%, rgba(255, 255, 255, .09), transparent 22%),
          linear-gradient(145deg, #262626 0%, #111827 48%, #050608 100%);
        box-shadow:
          0 2.2rem 4rem rgba(0, 0, 0, .58),
          inset 0 1px 0 rgba(255, 255, 255, .12),
          inset 0 -1rem 2rem rgba(0, 0, 0, .32);
        scrollbar-width: thin;
      }
      .r-head {
        position: sticky;
        top: 0;
        z-index: 3;
        display: grid;
        grid-template-columns: 1fr auto;
        align-items: center;
        gap: .6rem;
        min-height: 2.7rem;
        padding: .1rem .12rem .2rem;
        background: linear-gradient(180deg, rgba(31, 31, 31, .98), rgba(22, 22, 24, .92));
      }
      .r-head h2 {
        margin: 0;
        padding-left: .55rem;
        font-size: 1.05rem;
        letter-spacing: .035em;
        text-align: center;
        text-transform: uppercase;
      }
      .r-close {
        width: 2.7rem;
        min-height: 2.7rem;
        padding: 0;
        border-radius: 50%;
        background: linear-gradient(145deg, #ef4444, #991b1b);
        border-color: #f87171;
        font-size: 1.45rem;
      }
      .r-status,
      .r-overlay {
        margin: 0;
        border: 1px solid #374151;
        border-radius: .7rem;
        background: linear-gradient(180deg, #07111f, #020617);
        box-shadow: inset 0 .18rem .42rem rgba(0, 0, 0, .72);
      }
      .r-status {
        min-height: 1.1rem;
        padding: .45rem .62rem;
        color: #cbd5e1;
        font-size: .82rem;
        text-align: center;
      }
      .r-status.success { color: #86efac; }
      .r-status.warning { color: #fde68a; }
      .r-status.error { color: #fca5a5; }
      .r-overlay {
        padding: .65rem .72rem;
      }
      .r-overlay h3 {
        margin: 0 0 .32rem;
        color: #bfdbfe;
        font-size: 1rem;
      }
      .r-overlay p {
        margin: .18rem 0;
        color: #cbd5e1;
        font-size: .78rem;
        line-height: 1.25;
      }
      .r-section {
        min-width: 0;
      }
      .r-key {
        position: relative;
        box-sizing: border-box;
        min-width: 0;
        min-height: 2.75rem;
        padding: .42rem .35rem;
        white-space: pre-line;
        touch-action: manipulation;
        user-select: none;
        border: 1px solid #535965;
        border-radius: 999px;
        background: linear-gradient(155deg, #30343b 0%, #171a20 54%, #090b0e 100%);
        color: #f8fafc;
        font: inherit;
        font-size: .82rem;
        font-weight: 800;
        line-height: 1.05;
        text-shadow: 0 1px 1px #000;
        box-shadow:
          0 .28rem .38rem rgba(0, 0, 0, .72),
          inset 0 1px 1px rgba(255, 255, 255, .15),
          inset 0 -.12rem .24rem rgba(0, 0, 0, .72);
        cursor: pointer;
        transition:
          transform .075s ease,
          box-shadow .075s ease,
          filter .11s ease,
          border-color .11s ease;
      }
      @media (hover: hover) {
        .r-key:not(:disabled):hover {
          transform: translateY(-1px);
          filter: brightness(1.16);
          border-color: #7c8798;
          box-shadow:
            0 .38rem .52rem rgba(0, 0, 0, .78),
            inset 0 1px 1px rgba(255, 255, 255, .2),
            inset 0 -.12rem .24rem rgba(0, 0, 0, .7);
        }
      }
      .r-key:not(:disabled):active {
        transform: translateY(3px) scale(.98);
        filter: brightness(.84);
        box-shadow:
          0 .04rem .08rem rgba(0, 0, 0, .8),
          inset 0 .24rem .42rem rgba(0, 0, 0, .74),
          inset 0 1px 0 rgba(255, 255, 255, .05);
      }
      .r-key:focus-visible {
        outline: 3px solid #38bdf8;
        outline-offset: 2px;
      }
      .r-key:disabled {
        opacity: .35;
        cursor: default;
        filter: grayscale(.45);
      }
      .r-transport {
        display: grid;
        gap: .45rem;
      }
      .r-transport-row {
        display: grid;
        gap: .42rem;
      }
      .r-transport-seek {
        grid-template-columns: repeat(5, minmax(0, 1fr));
      }
      .r-transport-media {
        grid-template-columns: repeat(3, minmax(0, 1fr));
        padding-inline: 2.8rem;
      }
      .r-key-transport {
        min-height: 2.45rem;
        font-size: 1rem;
      }
      .r-key-transport.record {
        color: #fb7185;
      }
      .r-navigation {
        display: grid;
        gap: .65rem;
      }
      .r-tool-row {
        display: grid;
        grid-template-columns: repeat(3, minmax(0, 1fr));
        gap: .55rem;
        padding-inline: .65rem;
      }
      .r-key-tool {
        min-height: 2.35rem;
        font-size: .72rem;
      }
      .r-dpad {
        display: grid;
        grid-template-columns: repeat(3, minmax(0, 1fr));
        grid-template-rows: repeat(3, 3.8rem);
        grid-template-areas:
          ". up ."
          "left ok right"
          ". down .";
        gap: .18rem;
        width: min(14.5rem, 82%);
        margin: 0 auto;
        padding: .42rem;
        border: 1px solid #444b55;
        border-radius: 50%;
        background: radial-gradient(circle, #20242b 0%, #0d0f13 68%, #050608 100%);
        box-shadow:
          0 .65rem 1.05rem rgba(0, 0, 0, .65),
          inset 0 .12rem .28rem rgba(255, 255, 255, .08),
          inset 0 -.35rem .7rem rgba(0, 0, 0, .7);
      }
      .r-dpad .up { grid-area: up; border-radius: 60% 60% 28% 28%; }
      .r-dpad .down { grid-area: down; border-radius: 28% 28% 60% 60%; }
      .r-dpad .left { grid-area: left; border-radius: 60% 28% 28% 60%; }
      .r-dpad .right { grid-area: right; border-radius: 28% 60% 60% 28%; }
      .r-dpad .ok {
        grid-area: ok;
        min-height: 3.8rem;
        border-radius: 50%;
        background: radial-gradient(circle at 38% 28%, #3f4651, #13161b 68%);
        font-size: 1.25rem;
      }
      .r-rocker-grid {
        display: grid;
        grid-template-columns: 1fr 3.8rem 1fr;
        align-items: center;
        gap: 1rem;
        padding: .2rem .9rem;
      }
      .r-rocker {
        display: grid;
        gap: .22rem;
        padding: .25rem;
        border: 1px solid #444b55;
        border-radius: 1.4rem;
        background: #0b0d11;
        box-shadow: inset 0 .15rem .35rem rgba(0, 0, 0, .75);
      }
      .r-key-rocker {
        min-height: 3.65rem;
        border-radius: 1.05rem;
        font-size: .86rem;
      }
      .r-key-mute {
        width: 3.8rem;
        min-height: 3.8rem;
        border-radius: 50%;
        font-size: .66rem;
      }
      .r-color-grid {
        display: grid;
        grid-template-columns: repeat(4, minmax(0, 1fr));
        gap: .55rem;
        padding: .25rem .35rem;
      }
      .r-key-color {
        min-height: 1.75rem;
        border-radius: 999px;
      }
      .r-color-grid .red {
        background: linear-gradient(145deg, #ef4444, #991b1b);
        border-color: #f87171;
      }
      .r-color-grid .green {
        background: linear-gradient(145deg, #22c55e, #166534);
        border-color: #4ade80;
      }
      .r-color-grid .yellow {
        background: linear-gradient(145deg, #fde047, #ca8a04);
        border-color: #fef08a;
        color: #111827;
        text-shadow: none;
      }
      .r-color-grid .blue {
        background: linear-gradient(145deg, #3b82f6, #1e40af);
        border-color: #60a5fa;
      }
      .r-number-grid {
        display: grid;
        grid-template-columns: repeat(3, minmax(0, 1fr));
        gap: .55rem 1.05rem;
        padding: .2rem 2.25rem;
      }
      .r-key-number {
        aspect-ratio: 1;
        min-height: 0;
        border-radius: 50%;
        font-size: 1.28rem;
      }
      .r-number-grid .zero {
        grid-column: 2;
      }
      .r-direct-channel {
        display: grid;
        gap: .38rem;
        padding: .7rem .45rem 0;
        border-top: 1px solid rgba(148, 163, 184, .2);
      }
      .r-direct-label {
        color: #aeb8c7;
        font-size: .74rem;
        font-weight: 700;
        text-align: center;
      }
      .r-switch {
        display: grid;
        grid-template-columns: 1fr auto;
        gap: .45rem;
      }
      .r-input {
        min-width: 0;
        min-height: 2.65rem;
        padding: 0 .75rem;
        border: 1px solid #4b5563;
        border-radius: 999px;
        background: #020617;
        color: #fff;
        font: inherit;
        box-shadow: inset 0 .18rem .35rem rgba(0, 0, 0, .72);
      }
      .r-key-switch {
        min-height: 2.65rem;
        padding-inline: .85rem;
      }
      @media (max-width: 620px) {
        .r-dialog {
          width: min(23rem, calc(100vw - .45rem));
          max-height: calc(100dvh - .45rem);
        }
        .r-remote-body {
          max-height: calc(100dvh - .45rem);
          padding: .65rem .65rem 1rem;
          gap: .55rem;
          border-radius: 1.75rem;
        }
        .r-head {
          min-height: 2.35rem;
        }
        .r-head h2 {
          font-size: .92rem;
        }
        .r-close {
          width: 2.35rem;
          min-height: 2.35rem;
        }
        .r-overlay {
          padding: .5rem .58rem;
        }
        .r-overlay h3 {
          font-size: .9rem;
        }
        .r-overlay p,
        .r-status {
          font-size: .72rem;
        }
        .r-key {
          min-height: 2.4rem;
          font-size: .76rem;
        }
        .r-transport-media {
          padding-inline: 2.1rem;
        }
        .r-dpad {
          grid-template-rows: repeat(3, 3.35rem);
          width: min(13rem, 84%);
        }
        .r-dpad .ok {
          min-height: 3.35rem;
          font-size: 1.05rem;
        }
        .r-rocker-grid {
          grid-template-columns: 1fr 3.3rem 1fr;
          gap: .7rem;
          padding-inline: .75rem;
        }
        .r-key-rocker {
          min-height: 3.2rem;
        }
        .r-key-mute {
          width: 3.3rem;
          min-height: 3.3rem;
        }
        .r-number-grid {
          gap: .45rem .9rem;
          padding-inline: 2.2rem;
        }
      }
      @media (max-height: 760px) {
        .r-remote-body {
          gap: .48rem;
          padding-top: .55rem;
          padding-bottom: .75rem;
        }
        .r-overlay {
          display: none;
        }
        .r-status {
          padding-block: .32rem;
        }
        .r-dpad {
          grid-template-rows: repeat(3, 3rem);
          width: min(11.75rem, 74%);
        }
        .r-dpad .ok {
          min-height: 3rem;
        }
        .r-key-rocker {
          min-height: 2.85rem;
        }
        .r-number-grid {
          gap: .35rem .85rem;
          padding-inline: 2.65rem;
        }
        .r-key-number {
          font-size: 1rem;
        }
      }
    `;
    document.head.appendChild(style);
  }

  function setupLauncher() {
    installStyles();

    const nav = document.getElementById('module-nav');
    if (nav) {
      tabOrder.forEach(function (moduleName) {
        const button = nav.querySelector('[data-module="' + moduleName + '"]');
        if (button) {
          nav.append(button);
        }
      });
    }

    const features = document.querySelectorAll('.brand-feature-strip > .brand-feature');
    if (features[1]) {
      const description = features[1].querySelector('.brand-feature-text');
      if (description) {
        description.removeAttribute('data-i18n');
        description.textContent = 'Overlay vorbereitet · Streaming folgt';
      }
    }

    if (features[3]) {
      const launcher = features[3];
      const title = launcher.querySelector('.brand-feature-title');
      const description = launcher.querySelector('.brand-feature-text');
      launcher.tabIndex = 0;
      launcher.setAttribute('role', 'button');
      launcher.setAttribute('aria-label', 'VDR - Fernbedienung öffnen');
      launcher.classList.add('brand-feature-remote');

      if (title) {
        title.removeAttribute('data-i18n');
        title.textContent = 'VDR - Fernbedienung';
      }
      if (description) {
        description.removeAttribute('data-i18n');
        description.textContent = 'Tastenlayout · mobil';
      }

      launcher.onclick = open;
      launcher.onkeydown = function (event) {
        if (event.key === 'Enter' || event.key === ' ') {
          event.preventDefault();
          open();
        }
      };
    }
  }

  setupLauncher();

  const moduleApi = Object.freeze({
    open: open,
    actions: actions,
    tabOrder: tabOrder,
    canControlBackend: function (backend) {
      return Boolean(controlState(backend)[0]);
    }
  });

  if (
    global.VdrSuitePlatform &&
    global.VdrSuitePlatform.registerModule &&
    (!global.VdrSuitePlatform.hasModule || !global.VdrSuitePlatform.hasModule('remote'))
  ) {
    global.VdrSuitePlatform.registerModule('remote', moduleApi);
  }

  global.VdrSuiteRemote = moduleApi;
})(window);
