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
    previous: 'Vorherige',
    switchChannel: 'Kanal wechseln'
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
  let actionInFlight = false;

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
    if (!dialog) {
      return;
    }
    dialog.querySelectorAll('[data-action]').forEach(function (button) {
      button.disabled = locked;
    });
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

  function installPressFeedback(button) {
    const press = function () {
      if (!button.disabled) {
        button.classList.add('is-pressed');
      }
    };
    const release = function () {
      button.classList.remove('is-pressed');
    };

    button.addEventListener('pointerdown', press);
    button.addEventListener('pointerup', release);
    button.addEventListener('pointercancel', release);
    button.addEventListener('pointerleave', release);
    button.addEventListener('lostpointercapture', release);
    button.addEventListener('blur', release);
    button.addEventListener('keydown', function (event) {
      if (event.key === 'Enter' || event.key === ' ') {
        press();
      }
    });
    button.addEventListener('keyup', function (event) {
      if (event.key === 'Enter' || event.key === ' ') {
        release();
      }
    });
    return button;
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
    return installPressFeedback(button);
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
    const submit = installPressFeedback(element('button', 'r-key r-key-switch', 'Wechseln'));

    input.placeholder = 'VDR-Kanal-ID';
    input.inputMode = 'numeric';
    input.setAttribute('aria-label', 'VDR-Kanal-ID');
    submit.type = 'button';
    submit.dataset.action = 'switchChannel';
    submit.setAttribute('aria-label', labels.switchChannel);
    row.append(input, submit);
    section.append(label, row);
    remoteBody.append(section);
    return input;
  }

  function buildDialog() {
    const popup = element('dialog', 'r-dialog');
    const remoteBody = element('section', 'r-remote-body');
    const header = element('header', 'r-head');
    const brand = element('div', 'r-brand');
    const brandMark = element('span', 'r-brand-mark', 'VDR');
    const title = element('h2', '', 'Fernbedienung');
    const close = installPressFeedback(element('button', 'r-key r-close', '×'));

    popup.setAttribute('aria-label', 'VDR - Fernbedienung');
    close.type = 'button';
    close.setAttribute('aria-label', 'Fernbedienung schließen');
    close.title = 'Schließen';
    close.onclick = function () {
      popup.close();
    };

    brand.append(brandMark, title);
    header.append(brand, close);
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
      if (!button || button.disabled || actionInFlight) {
        return;
      }

      actionInFlight = true;
      button.classList.add('is-sending');
      button.setAttribute('aria-busy', 'true');
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
          actionInFlight = false;
          button.classList.remove('is-sending');
          button.removeAttribute('aria-busy');
          selectedBackend()
            .then(function (backend) {
              lockControls(!controlState(backend)[0]);
            })
            .catch(function () {
              lockControls(true);
            });
        });
    };

    popup.addEventListener('close', function () {
      actionInFlight = false;
      stopSubscription();
    });
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
        width: min(24rem, calc(100vw - .65rem));
        max-height: calc(100dvh - .65rem);
        padding: 0;
        margin: auto;
        border: 0;
        background: transparent;
        color: #f8fafc;
        overflow: visible;
      }
      .r-dialog::backdrop {
        background:
          radial-gradient(circle at 50% 20%, rgba(30, 64, 175, .23), transparent 38%),
          rgba(2, 6, 23, .88);
        backdrop-filter: blur(5px);
      }
      .r-remote-body {
        position: relative;
        isolation: isolate;
        box-sizing: border-box;
        display: grid;
        gap: .72rem;
        width: 100%;
        max-height: calc(100dvh - .65rem);
        padding: 1rem .95rem 1.35rem;
        overflow: auto;
        overscroll-behavior: contain;
        border: 1px solid #4b4d52;
        border-radius: 3.2rem 3.2rem 2.65rem 2.65rem;
        background:
          radial-gradient(ellipse at 22% 4%, rgba(255,255,255,.13), transparent 18%),
          radial-gradient(ellipse at 77% 96%, rgba(255,255,255,.035), transparent 24%),
          repeating-radial-gradient(circle at 30% 20%, rgba(255,255,255,.018) 0 1px, rgba(0,0,0,.018) 1px 3px),
          linear-gradient(112deg, #242629 0%, #101114 34%, #08090b 66%, #202226 100%);
        box-shadow:
          0 2.4rem 5rem rgba(0, 0, 0, .76),
          0 .55rem 1.2rem rgba(0, 0, 0, .7),
          inset .13rem 0 .2rem rgba(255, 255, 255, .09),
          inset -.2rem 0 .28rem rgba(0, 0, 0, .82),
          inset 0 .16rem .18rem rgba(255, 255, 255, .1),
          inset 0 -1.25rem 2.5rem rgba(0, 0, 0, .46);
        scrollbar-width: thin;
      }
      .r-remote-body::before {
        content: '';
        position: absolute;
        inset: .32rem;
        z-index: -1;
        pointer-events: none;
        border: 1px solid rgba(255,255,255,.035);
        border-radius: 2.9rem 2.9rem 2.35rem 2.35rem;
        box-shadow:
          inset 0 .15rem .5rem rgba(255,255,255,.025),
          inset 0 -.3rem .75rem rgba(0,0,0,.5);
      }
      .r-remote-body::after {
        content: '';
        position: absolute;
        inset: 0;
        z-index: 20;
        pointer-events: none;
        border-radius: inherit;
        background: linear-gradient(104deg, rgba(255,255,255,.04), transparent 15%, transparent 78%, rgba(255,255,255,.018));
        mix-blend-mode: screen;
      }
      .r-head {
        position: sticky;
        top: 0;
        z-index: 25;
        display: grid;
        grid-template-columns: 1fr auto;
        align-items: center;
        gap: .65rem;
        min-height: 3rem;
        padding: .15rem .15rem .28rem;
        background: linear-gradient(180deg, rgba(25,26,29,.99), rgba(13,14,16,.94));
      }
      .r-brand {
        display: flex;
        align-items: baseline;
        justify-content: center;
        gap: .42rem;
        min-width: 0;
      }
      .r-brand-mark {
        color: #93c5fd;
        font-size: .8rem;
        font-weight: 950;
        letter-spacing: .16em;
        text-shadow: 0 0 .7rem rgba(59,130,246,.35);
      }
      .r-head h2 {
        margin: 0;
        color: #e5e7eb;
        font-size: .9rem;
        font-weight: 800;
        letter-spacing: .08em;
        text-transform: uppercase;
      }
      .r-close {
        width: 2.75rem;
        min-height: 2.75rem;
        padding: 0;
        border-radius: 50%;
        background: radial-gradient(circle at 35% 28%, #ff7272, #d11f28 46%, #780812 100%);
        border-color: #ff7b83;
        color: #fff;
        font-size: 1.4rem;
      }
      .r-status,
      .r-overlay {
        margin: 0;
        border: 1px solid #26384d;
        border-radius: .55rem;
        background:
          linear-gradient(180deg, rgba(5,18,33,.98), rgba(1,8,16,.98)),
          #020617;
        box-shadow:
          inset 0 .22rem .5rem rgba(0, 0, 0, .82),
          inset 0 1px 0 rgba(125,211,252,.08),
          0 .2rem .35rem rgba(0,0,0,.45);
      }
      .r-status {
        min-height: 1.05rem;
        padding: .38rem .58rem;
        color: #9fb8cf;
        font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
        font-size: .72rem;
        text-align: center;
        text-shadow: 0 0 .45rem currentColor;
      }
      .r-status.success { color: #6ee7b7; }
      .r-status.warning { color: #fde68a; }
      .r-status.error { color: #fca5a5; }
      .r-overlay {
        padding: .62rem .72rem;
      }
      .r-overlay h3 {
        margin: 0 0 .28rem;
        color: #b7dcff;
        font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
        font-size: .94rem;
        text-shadow: 0 0 .55rem rgba(96,165,250,.45);
      }
      .r-overlay p {
        margin: .15rem 0;
        color: #a7bdd1;
        font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
        font-size: .7rem;
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
        -webkit-tap-highlight-color: transparent;
        border: 1px solid #5e6269;
        border-radius: 999px;
        background:
          radial-gradient(ellipse at 35% 20%, rgba(255,255,255,.16), transparent 42%),
          linear-gradient(160deg, #363a41 0%, #1c1f24 48%, #0a0c0f 100%);
        color: #f7f8fa;
        font: inherit;
        font-size: .8rem;
        font-weight: 850;
        line-height: 1.05;
        text-shadow: 0 1px 1px #000;
        box-shadow:
          0 .31rem 0 #050608,
          0 .5rem .72rem rgba(0, 0, 0, .72),
          inset 0 1px 1px rgba(255, 255, 255, .2),
          inset .08rem 0 .12rem rgba(255,255,255,.045),
          inset 0 -.15rem .25rem rgba(0, 0, 0, .82);
        cursor: pointer;
        transform: translate3d(0, 0, 0);
        transition:
          transform .07s ease,
          box-shadow .07s ease,
          filter .11s ease,
          border-color .11s ease;
      }
      .r-key::before {
        content: '';
        position: absolute;
        inset: .11rem .22rem auto;
        height: 38%;
        pointer-events: none;
        border-radius: inherit;
        background: linear-gradient(180deg, rgba(255,255,255,.09), transparent);
      }
      @media (hover: hover) {
        .r-key:not(:disabled):hover {
          transform: translate3d(0, -1px, 0);
          filter: brightness(1.16);
          border-color: #858b95;
          box-shadow:
            0 .37rem 0 #050608,
            0 .62rem .84rem rgba(0, 0, 0, .78),
            inset 0 1px 1px rgba(255, 255, 255, .24),
            inset 0 -.15rem .25rem rgba(0, 0, 0, .78);
        }
      }
      .r-key:not(:disabled):active,
      .r-key.is-pressed {
        transform: translate3d(0, .28rem, 0) scale(.985);
        filter: brightness(.78);
        box-shadow:
          0 .035rem 0 #030405,
          0 .08rem .12rem rgba(0, 0, 0, .75),
          inset 0 .28rem .52rem rgba(0, 0, 0, .82),
          inset 0 1px 0 rgba(255, 255, 255, .035);
      }
      .r-key.is-sending {
        border-color: #38bdf8;
        box-shadow:
          0 .31rem 0 #050608,
          0 0 .75rem rgba(56,189,248,.35),
          inset 0 1px 1px rgba(255,255,255,.2),
          inset 0 -.15rem .25rem rgba(0,0,0,.82);
      }
      .r-key:focus-visible {
        outline: 3px solid #38bdf8;
        outline-offset: 3px;
      }
      .r-key:disabled {
        opacity: .38;
        cursor: default;
        filter: grayscale(.5);
      }
      .r-transport {
        display: grid;
        gap: .5rem;
        padding-top: .1rem;
      }
      .r-transport-row {
        display: grid;
        gap: .48rem;
      }
      .r-transport-seek {
        grid-template-columns: repeat(5, minmax(0, 1fr));
      }
      .r-transport-media {
        grid-template-columns: repeat(3, minmax(0, 1fr));
        padding-inline: 2.7rem;
      }
      .r-key-transport {
        min-height: 2.4rem;
        font-size: .95rem;
      }
      .r-key-transport.record {
        color: #fb7185;
      }
      .r-navigation {
        display: grid;
        gap: .72rem;
      }
      .r-tool-row {
        display: grid;
        grid-template-columns: repeat(3, minmax(0, 1fr));
        gap: .62rem;
        padding-inline: .7rem;
      }
      .r-key-tool {
        min-height: 2.25rem;
        font-size: .68rem;
      }
      .r-dpad {
        display: grid;
        grid-template-columns: repeat(3, minmax(0, 1fr));
        grid-template-rows: repeat(3, 3.75rem);
        grid-template-areas:
          '. up .'
          'left ok right'
          '. down .';
        gap: .2rem;
        width: min(14rem, 82%);
        margin: 0 auto;
        padding: .5rem;
        border: 1px solid #555a62;
        border-radius: 50%;
        background:
          radial-gradient(circle at 40% 28%, #33373e 0%, #17191d 45%, #08090b 78%, #030405 100%);
        box-shadow:
          0 .75rem 1.25rem rgba(0, 0, 0, .72),
          0 .22rem 0 #040506,
          inset 0 .16rem .28rem rgba(255, 255, 255, .09),
          inset 0 -.45rem .8rem rgba(0, 0, 0, .74);
      }
      .r-dpad .up { grid-area: up; border-radius: 64% 64% 30% 30%; }
      .r-dpad .down { grid-area: down; border-radius: 30% 30% 64% 64%; }
      .r-dpad .left { grid-area: left; border-radius: 64% 30% 30% 64%; }
      .r-dpad .right { grid-area: right; border-radius: 30% 64% 64% 30%; }
      .r-dpad .ok {
        grid-area: ok;
        min-height: 3.75rem;
        border-radius: 50%;
        background:
          radial-gradient(circle at 36% 25%, rgba(255,255,255,.18), transparent 34%),
          radial-gradient(circle, #4b515b 0%, #171a1f 66%, #08090c 100%);
        font-size: 1.12rem;
      }
      .r-rocker-grid {
        display: grid;
        grid-template-columns: 1fr 3.6rem 1fr;
        align-items: center;
        gap: 1rem;
        padding: .15rem .85rem;
      }
      .r-rocker {
        display: grid;
        gap: .2rem;
        padding: .28rem;
        border: 1px solid #454951;
        border-radius: 1.45rem;
        background: linear-gradient(180deg, #0e1013, #050608);
        box-shadow:
          inset 0 .22rem .42rem rgba(0, 0, 0, .84),
          0 .25rem .5rem rgba(0,0,0,.55);
      }
      .r-key-rocker {
        min-height: 3.55rem;
        border-radius: 1.05rem;
        font-size: .82rem;
      }
      .r-key-mute {
        width: 3.6rem;
        min-height: 3.6rem;
        border-radius: 50%;
        font-size: .62rem;
      }
      .r-color-grid {
        display: grid;
        grid-template-columns: repeat(4, minmax(0, 1fr));
        gap: .62rem;
        padding: .28rem .4rem;
      }
      .r-key-color {
        min-height: 1.65rem;
        border-radius: 999px;
      }
      .r-color-grid .red {
        background: radial-gradient(circle at 35% 24%, #ff8888, #e12835 48%, #7b0711 100%);
        border-color: #ff7b83;
      }
      .r-color-grid .green {
        background: radial-gradient(circle at 35% 24%, #72f2a2, #20b85a 48%, #075328 100%);
        border-color: #5ee38e;
      }
      .r-color-grid .yellow {
        background: radial-gradient(circle at 35% 24%, #fff5a5, #edcf24 48%, #8d7105 100%);
        border-color: #fff195;
        color: #111827;
        text-shadow: none;
      }
      .r-color-grid .blue {
        background: radial-gradient(circle at 35% 24%, #8fc7ff, #2f7de1 48%, #123b83 100%);
        border-color: #82baff;
      }
      .r-number-grid {
        display: grid;
        grid-template-columns: repeat(3, minmax(0, 1fr));
        gap: .6rem 1.1rem;
        padding: .25rem 2.2rem;
      }
      .r-key-number {
        aspect-ratio: 1;
        min-height: 0;
        border-radius: 50%;
        font-size: 1.22rem;
      }
      .r-number-grid .zero {
        grid-column: 2;
      }
      .r-direct-channel {
        display: grid;
        gap: .38rem;
        padding: .78rem .45rem 0;
        border-top: 1px solid rgba(148, 163, 184, .17);
      }
      .r-direct-label {
        color: #98a3af;
        font-size: .7rem;
        font-weight: 750;
        letter-spacing: .04em;
        text-align: center;
        text-transform: uppercase;
      }
      .r-switch {
        display: grid;
        grid-template-columns: 1fr auto;
        gap: .5rem;
      }
      .r-input {
        min-width: 0;
        min-height: 2.55rem;
        padding: 0 .75rem;
        border: 1px solid #3b4654;
        border-radius: 999px;
        background: linear-gradient(180deg, #040b14, #010409);
        color: #dbeafe;
        font: inherit;
        box-shadow:
          inset 0 .22rem .45rem rgba(0, 0, 0, .82),
          inset 0 1px 0 rgba(125,211,252,.05);
      }
      .r-key-switch {
        min-height: 2.55rem;
        padding-inline: .9rem;
      }
      @media (max-width: 620px) {
        .r-dialog {
          width: min(22.5rem, calc(100vw - .35rem));
          max-height: calc(100dvh - .35rem);
        }
        .r-remote-body {
          max-height: calc(100dvh - .35rem);
          padding: .72rem .68rem 1.05rem;
          gap: .56rem;
          border-radius: 2.65rem 2.65rem 2.2rem 2.2rem;
        }
        .r-head {
          min-height: 2.45rem;
        }
        .r-head h2 {
          font-size: .77rem;
        }
        .r-brand-mark {
          font-size: .68rem;
        }
        .r-close {
          width: 2.4rem;
          min-height: 2.4rem;
        }
        .r-overlay {
          padding: .5rem .58rem;
        }
        .r-overlay h3 {
          font-size: .86rem;
        }
        .r-overlay p,
        .r-status {
          font-size: .66rem;
        }
        .r-key {
          min-height: 2.35rem;
          font-size: .72rem;
        }
        .r-transport-media {
          padding-inline: 2rem;
        }
        .r-dpad {
          grid-template-rows: repeat(3, 3.25rem);
          width: min(12.5rem, 84%);
        }
        .r-dpad .ok {
          min-height: 3.25rem;
          font-size: 1rem;
        }
        .r-rocker-grid {
          grid-template-columns: 1fr 3.2rem 1fr;
          gap: .68rem;
          padding-inline: .7rem;
        }
        .r-key-rocker {
          min-height: 3.05rem;
        }
        .r-key-mute {
          width: 3.2rem;
          min-height: 3.2rem;
        }
        .r-number-grid {
          gap: .45rem .9rem;
          padding-inline: 2.05rem;
        }
      }
      @media (max-height: 760px) {
        .r-remote-body {
          gap: .46rem;
          padding-top: .55rem;
          padding-bottom: .75rem;
        }
        .r-overlay {
          display: none;
        }
        .r-status {
          padding-block: .3rem;
        }
        .r-dpad {
          grid-template-rows: repeat(3, 2.9rem);
          width: min(11.4rem, 74%);
        }
        .r-dpad .ok {
          min-height: 2.9rem;
        }
        .r-key-rocker {
          min-height: 2.75rem;
        }
        .r-number-grid {
          gap: .34rem .82rem;
          padding-inline: 2.55rem;
        }
        .r-key-number {
          font-size: .98rem;
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
        description.textContent = 'Fotorealistisch · mobil';
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
}(window));
