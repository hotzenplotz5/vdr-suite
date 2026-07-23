(function (global) {
  'use strict';

  const REMOTE_ACTION_ROUTE = '/api/vdr/remote/actions';
  const LIVE_OVERLAY_ROUTE = '/api/vdr/live/overlay';

  const ACTION_DEFINITIONS = Object.freeze([
    Object.freeze({ action: 'menu', label: 'Menu', group: 'navigation' }),
    Object.freeze({ action: 'up', label: '▲', group: 'navigation' }),
    Object.freeze({ action: 'info', label: 'Info', group: 'navigation' }),
    Object.freeze({ action: 'left', label: '◀', group: 'navigation' }),
    Object.freeze({ action: 'ok', label: 'OK', group: 'navigation', emphasis: true }),
    Object.freeze({ action: 'right', label: '▶', group: 'navigation' }),
    Object.freeze({ action: 'back', label: 'Zurück', labelEn: 'Back', group: 'navigation' }),
    Object.freeze({ action: 'down', label: '▼', group: 'navigation' }),
    Object.freeze({ action: 'mute', label: 'Stumm', labelEn: 'Mute', group: 'navigation' }),
    Object.freeze({ action: 'red', label: 'Rot', labelEn: 'Red', group: 'colour' }),
    Object.freeze({ action: 'green', label: 'Grün', labelEn: 'Green', group: 'colour' }),
    Object.freeze({ action: 'yellow', label: 'Gelb', labelEn: 'Yellow', group: 'colour' }),
    Object.freeze({ action: 'blue', label: 'Blau', labelEn: 'Blue', group: 'colour' }),
    Object.freeze({ action: 'one', label: '1', group: 'digits' }),
    Object.freeze({ action: 'two', label: '2', group: 'digits' }),
    Object.freeze({ action: 'three', label: '3', group: 'digits' }),
    Object.freeze({ action: 'four', label: '4', group: 'digits' }),
    Object.freeze({ action: 'five', label: '5', group: 'digits' }),
    Object.freeze({ action: 'six', label: '6', group: 'digits' }),
    Object.freeze({ action: 'seven', label: '7', group: 'digits' }),
    Object.freeze({ action: 'eight', label: '8', group: 'digits' }),
    Object.freeze({ action: 'nine', label: '9', group: 'digits' }),
    Object.freeze({ action: 'zero', label: '0', group: 'digits' }),
    Object.freeze({ action: 'channelDown', label: 'Kanal −', labelEn: 'Channel −', group: 'channel' }),
    Object.freeze({ action: 'channelUp', label: 'Kanal +', labelEn: 'Channel +', group: 'channel' }),
    Object.freeze({ action: 'volumeDown', label: 'Lautstärke −', labelEn: 'Volume −', group: 'volume' }),
    Object.freeze({ action: 'volumeUp', label: 'Lautstärke +', labelEn: 'Volume +', group: 'volume' }),
    Object.freeze({ action: 'previous', label: 'Vorherige', labelEn: 'Previous', group: 'playback' }),
    Object.freeze({ action: 'rewind', label: 'Rücklauf', labelEn: 'Rewind', group: 'playback' }),
    Object.freeze({ action: 'play', label: 'Wiedergabe', labelEn: 'Play', group: 'playback' }),
    Object.freeze({ action: 'pause', label: 'Pause', group: 'playback' }),
    Object.freeze({ action: 'stop', label: 'Stopp', labelEn: 'Stop', group: 'playback' }),
    Object.freeze({ action: 'record', label: 'Aufnahme', labelEn: 'Record', group: 'playback' }),
    Object.freeze({ action: 'fastForward', label: 'Vorlauf', labelEn: 'Fast forward', group: 'playback' }),
    Object.freeze({ action: 'next', label: 'Nächste', labelEn: 'Next', group: 'playback' })
  ]);

  const ALLOWED_ACTIONS = Object.freeze(
    ACTION_DEFINITIONS.map(function (definition) {
      return definition.action;
    }).concat(['switchChannel'])
  );

  const ALLOWED_ACTION_SET = new Set(ALLOWED_ACTIONS);
  let operationCounter = 0;
  let dialogElement = null;
  let currentBackend = null;
  let actionInFlight = false;

  function platform() {
    return global.VdrSuitePlatform || null;
  }

  function clientApi() {
    const frontendPlatform = platform();

    if (frontendPlatform && typeof frontendPlatform.getClientApi === 'function') {
      return frontendPlatform.getClientApi();
    }

    return global.VdrSuiteClientApi || null;
  }

  function locale() {
    const frontendPlatform = platform();
    const i18n = frontendPlatform && typeof frontendPlatform.getI18n === 'function'
      ? frontendPlatform.getI18n()
      : global.VdrSuiteI18n;

    if (i18n && typeof i18n.getLocale === 'function') {
      return i18n.getLocale() === 'en' ? 'en' : 'de';
    }

    return 'de';
  }

  function text(de, en) {
    return locale() === 'en' ? String(en || de) : String(de);
  }

  function selectedBackendId() {
    const frontendPlatform = platform();

    if (frontendPlatform && typeof frontendPlatform.getSelectedBackendId === 'function') {
      return String(frontendPlatform.getSelectedBackendId() || 'default');
    }

    return 'default';
  }

  function operationId() {
    operationCounter += 1;
    return 'remote-' + Date.now().toString(36) + '-' + operationCounter.toString(36);
  }

  function normalizeBackendList(payload) {
    if (Array.isArray(payload)) {
      return payload;
    }

    if (payload && Array.isArray(payload.backends)) {
      return payload.backends;
    }

    return [];
  }

  function controlStateForBackend(backend) {
    if (!backend || typeof backend !== 'object') {
      return Object.freeze({
        allowed: false,
        reason: text('Backend nicht gefunden.', 'Backend not found.')
      });
    }

    const selector = backend.frontendSelector || backend;
    const readOnly = backend.readOnly === true || selector.readOnly === true ||
      String(backend.accessMode || selector.accessMode || '') === 'read-only';
    const enabled = backend.enabled !== false;
    const online = backend.online !== false;
    const capability = backend.canRemoteControl === true || selector.canRemoteControl === true;

    if (readOnly) {
      return Object.freeze({
        allowed: false,
        reason: text(
          'Dieses Backend ist schreibgeschützt. Fernsteuerungsaktionen sind serverseitig gesperrt.',
          'This backend is read-only. Remote-control actions are blocked by the server.'
        )
      });
    }

    if (!enabled) {
      return Object.freeze({
        allowed: false,
        reason: text('Dieses Backend ist deaktiviert.', 'This backend is disabled.')
      });
    }

    if (!online) {
      return Object.freeze({
        allowed: false,
        reason: text('Dieses Backend ist derzeit offline.', 'This backend is currently offline.')
      });
    }

    if (!capability) {
      return Object.freeze({
        allowed: false,
        reason: text(
          'Das Backend bietet die Capability remote.control nicht an.',
          'The backend does not provide the remote.control capability.'
        )
      });
    }

    return Object.freeze({
      allowed: true,
      reason: text('Fernsteuerung verfügbar.', 'Remote control available.')
    });
  }

  function fetchBackends() {
    const api = clientApi();

    if (!api || typeof api.fetchClientBackends !== 'function') {
      return Promise.reject(new Error('VDR-Suite Client API backend wrapper is not available'));
    }

    return api.fetchClientBackends({
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(normalizeBackendList);
  }

  function fetchSelectedBackend() {
    const backendId = selectedBackendId();

    return fetchBackends().then(function (backends) {
      const backend = backends.find(function (candidate) {
        const selector = candidate && candidate.frontendSelector
          ? candidate.frontendSelector
          : candidate;
        return String((selector && selector.id) || candidate.backendId || '') === backendId;
      });

      currentBackend = backend || null;
      return currentBackend;
    });
  }

  function fetchOverlay() {
    const api = clientApi();

    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('VDR-Suite Client API request wrapper is not available'));
    }

    return api.requestJson(LIVE_OVERLAY_ROUTE, {
      query: { backend: selectedBackendId() },
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function requestAction(action, channelId) {
    const normalizedAction = String(action || '');

    if (!ALLOWED_ACTION_SET.has(normalizedAction)) {
      return Promise.reject(new Error('Remote action is not allowlisted: ' + normalizedAction));
    }

    const api = clientApi();

    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('VDR-Suite Client API request wrapper is not available'));
    }

    const payload = {
      backendId: selectedBackendId(),
      operationId: operationId(),
      action: normalizedAction
    };

    if (normalizedAction === 'switchChannel') {
      const normalizedChannelId = String(channelId || '').trim();

      if (!normalizedChannelId) {
        return Promise.reject(new Error(text('Kanal-ID fehlt.', 'Channel ID is required.')));
      }

      payload.channelId = normalizedChannelId;
    }

    return api.requestJson(REMOTE_ACTION_ROUTE, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function sendAction(action, channelId) {
    return fetchSelectedBackend().then(function (backend) {
      const state = controlStateForBackend(backend);

      if (!state.allowed) {
        throw new Error(state.reason);
      }

      return requestAction(action, channelId);
    });
  }

  function createElement(tagName, className, content) {
    const element = global.document.createElement(tagName);

    if (className) {
      element.className = className;
    }

    if (content !== undefined && content !== null) {
      element.textContent = String(content);
    }

    return element;
  }

  function statusElement() {
    return dialogElement ? dialogElement.querySelector('[data-remote-status]') : null;
  }

  function setStatus(message, kind) {
    const element = statusElement();

    if (!element) {
      return;
    }

    element.textContent = String(message || '');
    element.className = 'vdr-remote-status' + (kind ? ' ' + kind : '');
  }

  function setButtonsDisabled(disabled) {
    if (!dialogElement) {
      return;
    }

    dialogElement.querySelectorAll('[data-remote-action]').forEach(function (button) {
      button.disabled = disabled;
    });

    const channelInput = dialogElement.querySelector('[data-remote-channel-id]');
    const channelButton = dialogElement.querySelector('[data-remote-switch-channel]');

    if (channelInput) {
      channelInput.disabled = disabled;
    }

    if (channelButton) {
      channelButton.disabled = disabled;
    }
  }

  function formatClock(epochSeconds) {
    const value = Number(epochSeconds);

    if (!Number.isFinite(value) || value <= 0) {
      return '--:--';
    }

    return new Date(value * 1000).toLocaleTimeString(
      locale() === 'en' ? 'en-GB' : 'de-DE',
      { hour: '2-digit', minute: '2-digit' }
    );
  }

  function renderOverlay(snapshot) {
    if (!dialogElement) {
      return;
    }

    const channel = dialogElement.querySelector('[data-remote-overlay-channel]');
    const present = dialogElement.querySelector('[data-remote-overlay-present]');
    const following = dialogElement.querySelector('[data-remote-overlay-following]');

    const channelData = snapshot && snapshot.channel ? snapshot.channel : {};
    const presentData = snapshot && snapshot.present ? snapshot.present : {};
    const followingData = snapshot && snapshot.following ? snapshot.following : {};

    if (channel) {
      channel.textContent = channelData.available
        ? [channelData.number, channelData.name].filter(Boolean).join(' · ')
        : text('Aktueller Kanal nicht verfügbar', 'Current channel unavailable');
    }

    if (present) {
      present.textContent = presentData.available
        ? formatClock(presentData.startTime) + '–' + formatClock(presentData.endTime) + ' · ' + String(presentData.title || '')
        : text('Keine laufende Sendung verfügbar', 'No current programme available');
    }

    if (following) {
      following.textContent = followingData.available
        ? text('Danach: ', 'Next: ') + String(followingData.title || '')
        : text('Keine folgende Sendung verfügbar', 'No following programme available');
    }
  }

  function refreshDialogState() {
    setStatus(text('Backend und Live-Status werden geladen …', 'Loading backend and live status …'));
    setButtonsDisabled(true);

    return Promise.allSettled([fetchSelectedBackend(), fetchOverlay()]).then(function (results) {
      const backendResult = results[0];
      const overlayResult = results[1];

      if (overlayResult.status === 'fulfilled') {
        renderOverlay(overlayResult.value);
      } else {
        renderOverlay(null);
      }

      if (backendResult.status !== 'fulfilled') {
        setStatus(backendResult.reason.message, 'error');
        return;
      }

      const state = controlStateForBackend(backendResult.value);
      setButtonsDisabled(!state.allowed);
      setStatus(state.reason, state.allowed ? 'success' : 'warning');
    });
  }

  function actionLabel(definition) {
    return locale() === 'en'
      ? String(definition.labelEn || definition.label)
      : String(definition.label);
  }

  function runUiAction(action, channelId) {
    if (actionInFlight) {
      return;
    }

    actionInFlight = true;
    setButtonsDisabled(true);
    setStatus(text('Aktion wird ausgeführt …', 'Executing action …'));

    sendAction(action, channelId)
      .then(function (result) {
        setStatus(
          String((result && result.message) || text('Aktion ausgeführt.', 'Action executed.')),
          'success'
        );
        return fetchOverlay().then(renderOverlay, function () {});
      })
      .catch(function (error) {
        setStatus(error.message, 'error');
      })
      .finally(function () {
        actionInFlight = false;
        const state = controlStateForBackend(currentBackend);
        setButtonsDisabled(!state.allowed);
      });
  }

  function appendActionGroup(container, groupName, className) {
    const group = createElement('div', className || 'vdr-remote-button-grid');

    ACTION_DEFINITIONS.filter(function (definition) {
      return definition.group === groupName;
    }).forEach(function (definition) {
      const button = createElement('button', 'vdr-remote-button', actionLabel(definition));
      button.type = 'button';
      button.dataset.remoteAction = definition.action;

      if (definition.emphasis) {
        button.classList.add('primary');
      }

      if (definition.group === 'colour') {
        button.classList.add('colour', definition.action);
      }

      button.addEventListener('click', function () {
        runUiAction(definition.action);
      });
      group.appendChild(button);
    });

    container.appendChild(group);
  }

  function ensureStyles() {
    if (!global.document || global.document.getElementById('vdr-remote-module-styles')) {
      return;
    }

    const style = createElement('style');
    style.id = 'vdr-remote-module-styles';
    style.textContent = [
      '.vdr-remote-dialog{width:min(46rem,calc(100vw - 1rem));max-height:calc(100vh - 1rem);padding:0;border:1px solid rgba(96,165,250,.42);border-radius:1.15rem;background:#07111f;color:#e2e8f0;box-shadow:0 1.5rem 4rem rgba(2,6,23,.7)}',
      '.vdr-remote-dialog::backdrop{background:rgba(2,6,23,.78);backdrop-filter:blur(4px)}',
      '.vdr-remote-shell{display:grid;gap:.9rem;padding:1rem;overflow:auto;max-height:calc(100vh - 3rem)}',
      '.vdr-remote-header{display:flex;align-items:flex-start;justify-content:space-between;gap:1rem}',
      '.vdr-remote-header h2{margin:0;font-size:1.25rem}',
      '.vdr-remote-close{min-width:2.7rem}',
      '.vdr-remote-overlay{display:grid;gap:.35rem;padding:.85rem;border:1px solid rgba(96,165,250,.22);border-radius:.85rem;background:rgba(15,23,42,.72)}',
      '.vdr-remote-channel{font-size:1.05rem;font-weight:800;color:#f8fafc}',
      '.vdr-remote-present{font-weight:700}',
      '.vdr-remote-following{color:#94a3b8}',
      '.vdr-remote-status{margin:0;padding:.65rem .75rem;border-radius:.65rem;background:rgba(30,41,59,.78)}',
      '.vdr-remote-status.success{color:#bbf7d0}.vdr-remote-status.warning{color:#fde68a}.vdr-remote-status.error{color:#fecaca}',
      '.vdr-remote-section{display:grid;gap:.55rem}',
      '.vdr-remote-section h3{margin:0;font-size:.82rem;text-transform:uppercase;letter-spacing:.06em;color:#93c5fd}',
      '.vdr-remote-button-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:.5rem}',
      '.vdr-remote-colour-grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:.5rem}',
      '.vdr-remote-digits{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:.5rem}',
      '.vdr-remote-wide-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.5rem}',
      '.vdr-remote-playback{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:.5rem}',
      '.vdr-remote-button{min-height:3rem;padding:.55rem .4rem;border:1px solid rgba(148,163,184,.28);border-radius:.75rem;background:#172033;color:#f8fafc;font:inherit;font-weight:750;touch-action:manipulation}',
      '.vdr-remote-button:hover:not(:disabled),.vdr-remote-button:focus-visible:not(:disabled){border-color:#38bdf8;background:#0f3550;outline:none}',
      '.vdr-remote-button.primary{background:#075985;border-color:#38bdf8}',
      '.vdr-remote-button.colour.red{border-bottom:5px solid #ef4444}.vdr-remote-button.colour.green{border-bottom:5px solid #22c55e}.vdr-remote-button.colour.yellow{border-bottom:5px solid #eab308}.vdr-remote-button.colour.blue{border-bottom:5px solid #3b82f6}',
      '.vdr-remote-button:disabled{opacity:.42;cursor:not-allowed}',
      '.vdr-remote-channel-switch{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:.5rem}',
      '.vdr-remote-channel-switch input{min-width:0;padding:.7rem;border:1px solid rgba(148,163,184,.35);border-radius:.7rem;background:#020617;color:#f8fafc;font:inherit}',
      '@media(max-width:600px){.vdr-remote-dialog{width:100vw;height:100vh;max-height:100vh;border-radius:0;border:0}.vdr-remote-shell{max-height:none;min-height:100vh;box-sizing:border-box}.vdr-remote-playback{grid-template-columns:repeat(2,minmax(0,1fr))}.vdr-remote-button{min-height:3.35rem}}'
    ].join('');
    global.document.head.appendChild(style);
  }

  function buildDialog() {
    ensureStyles();

    const dialog = createElement('dialog', 'vdr-remote-dialog');
    dialog.setAttribute('aria-labelledby', 'vdr-remote-title');

    const shell = createElement('section', 'vdr-remote-shell');
    const header = createElement('header', 'vdr-remote-header');
    const heading = createElement('h2', '', text('Fernsteuerung', 'Remote control'));
    heading.id = 'vdr-remote-title';
    const close = createElement('button', 'vdr-remote-button vdr-remote-close', '×');
    close.type = 'button';
    close.setAttribute('aria-label', text('Fernsteuerung schließen', 'Close remote control'));
    close.addEventListener('click', function () {
      dialog.close();
    });
    header.appendChild(heading);
    header.appendChild(close);
    shell.appendChild(header);

    const overlay = createElement('section', 'vdr-remote-overlay');
    const channel = createElement('div', 'vdr-remote-channel', text('Aktueller Kanal wird geladen …', 'Loading current channel …'));
    const present = createElement('div', 'vdr-remote-present', '');
    const following = createElement('div', 'vdr-remote-following', '');
    channel.dataset.remoteOverlayChannel = 'true';
    present.dataset.remoteOverlayPresent = 'true';
    following.dataset.remoteOverlayFollowing = 'true';
    overlay.appendChild(channel);
    overlay.appendChild(present);
    overlay.appendChild(following);
    shell.appendChild(overlay);

    const status = createElement('p', 'vdr-remote-status', '');
    status.dataset.remoteStatus = 'true';
    shell.appendChild(status);

    const navigationSection = createElement('section', 'vdr-remote-section');
    navigationSection.appendChild(createElement('h3', '', text('Navigation', 'Navigation')));
    appendActionGroup(navigationSection, 'navigation', 'vdr-remote-button-grid');
    shell.appendChild(navigationSection);

    const colourSection = createElement('section', 'vdr-remote-section');
    colourSection.appendChild(createElement('h3', '', text('Farbtasten', 'Colour keys')));
    appendActionGroup(colourSection, 'colour', 'vdr-remote-colour-grid');
    shell.appendChild(colourSection);

    const digitSection = createElement('section', 'vdr-remote-section');
    digitSection.appendChild(createElement('h3', '', text('Ziffern', 'Digits')));
    appendActionGroup(digitSection, 'digits', 'vdr-remote-digits');
    shell.appendChild(digitSection);

    const channelSection = createElement('section', 'vdr-remote-section');
    channelSection.appendChild(createElement('h3', '', text('Kanal und Lautstärke', 'Channel and volume')));
    appendActionGroup(channelSection, 'channel', 'vdr-remote-wide-grid');
    appendActionGroup(channelSection, 'volume', 'vdr-remote-wide-grid');

    const channelSwitch = createElement('div', 'vdr-remote-channel-switch');
    const channelInput = createElement('input');
    channelInput.type = 'text';
    channelInput.maxLength = 128;
    channelInput.placeholder = text('VDR-Kanal-ID, z. B. C-1-1079-10351', 'VDR channel ID, e.g. C-1-1079-10351');
    channelInput.dataset.remoteChannelId = 'true';
    const channelButton = createElement('button', 'vdr-remote-button', text('Direkt wechseln', 'Switch directly'));
    channelButton.type = 'button';
    channelButton.dataset.remoteSwitchChannel = 'true';
    channelButton.addEventListener('click', function () {
      runUiAction('switchChannel', channelInput.value);
    });
    channelSwitch.appendChild(channelInput);
    channelSwitch.appendChild(channelButton);
    channelSection.appendChild(channelSwitch);
    shell.appendChild(channelSection);

    const playbackSection = createElement('section', 'vdr-remote-section');
    playbackSection.appendChild(createElement('h3', '', text('Wiedergabe', 'Playback')));
    appendActionGroup(playbackSection, 'playback', 'vdr-remote-playback');
    shell.appendChild(playbackSection);

    dialog.appendChild(shell);
    global.document.body.appendChild(dialog);
    return dialog;
  }

  function ensureDialog() {
    if (!dialogElement) {
      dialogElement = buildDialog();
    }

    return dialogElement;
  }

  function open() {
    const dialog = ensureDialog();

    if (typeof dialog.showModal === 'function') {
      if (!dialog.open) {
        dialog.showModal();
      }
    } else {
      dialog.setAttribute('open', '');
    }

    return refreshDialogState();
  }

  function installQuickAccess() {
    if (!global.document) {
      return false;
    }

    const formerTitle = global.document.querySelector('[data-i18n="shell.modularSuite"]');
    const tile = formerTitle ? formerTitle.closest('.brand-feature') : null;

    if (!tile) {
      return false;
    }

    const description = tile.querySelector('[data-i18n="shell.modularSuiteDescription"]');
    tile.removeAttribute('data-brand-module');
    tile.dataset.remoteControlLaunch = 'true';
    tile.tabIndex = 0;
    tile.setAttribute('role', 'button');
    tile.setAttribute('aria-label', text('Fernsteuerung öffnen', 'Open remote control'));

    formerTitle.removeAttribute('data-i18n');
    formerTitle.textContent = text('Fernsteuerung', 'Remote control');

    if (description) {
      description.removeAttribute('data-i18n');
      description.textContent = text('Backend sicher bedienen', 'Control backend safely');
    }

    function activate(event) {
      if (event.type === 'keydown' && event.key !== 'Enter' && event.key !== ' ') {
        return;
      }

      event.preventDefault();
      open();
    }

    tile.addEventListener('click', activate);
    tile.addEventListener('keydown', activate);
    return true;
  }

  function refreshLocalizedQuickAccess() {
    if (!global.document) {
      return;
    }

    const tile = global.document.querySelector('[data-remote-control-launch="true"]');

    if (!tile) {
      return;
    }

    const title = tile.querySelector('.brand-feature-title');
    const description = tile.querySelector('.brand-feature-text');
    tile.setAttribute('aria-label', text('Fernsteuerung öffnen', 'Open remote control'));

    if (title) {
      title.textContent = text('Fernsteuerung', 'Remote control');
    }

    if (description) {
      description.textContent = text('Backend sicher bedienen', 'Control backend safely');
    }
  }

  const moduleApi = Object.freeze({
    allowedActions: ALLOWED_ACTIONS,
    controlStateForBackend: controlStateForBackend,
    fetchOverlay: fetchOverlay,
    sendAction: sendAction,
    open: open,
    installQuickAccess: installQuickAccess
  });

  global.VdrSuiteRemoteControl = moduleApi;

  const frontendPlatform = platform();
  if (frontendPlatform &&
      typeof frontendPlatform.registerModule === 'function' &&
      (!frontendPlatform.hasModule || !frontendPlatform.hasModule('remote'))) {
    frontendPlatform.registerModule('remote', moduleApi);
  }

  if (global.document) {
    installQuickAccess();

    if (typeof global.addEventListener === 'function') {
      global.addEventListener('vdr-suite:locale-changed', refreshLocalizedQuickAccess);
    }
  }
}(window));
