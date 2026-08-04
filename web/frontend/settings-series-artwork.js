(function (global) {
  'use strict';

  const CARD_ID = 'series-artwork-backend-settings-card';
  const STYLE_ID = 'series-artwork-backend-settings-style';
  let loadingBackendId = '';

  function english() {
    return String(document.documentElement.lang || '').toLowerCase().startsWith('en');
  }

  function text(de, en) {
    return english() ? en : de;
  }

  function backendId() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedBackendId === 'function') {
      return String(platform.getSelectedBackendId() || 'default');
    }
    return 'default';
  }

  function selectedSettingsModule() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedModule === 'function') {
      return String(platform.getSelectedModule() || '') === 'settings';
    }
    return false;
  }

  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function apiPath(id) {
    return '/api/backends/' + encodeURIComponent(id) +
      '/settings/series-artwork';
  }

  function parseResponse(response) {
    return response.text().then(function (body) {
      let payload = null;
      if (body) {
        try {
          payload = JSON.parse(body);
        } catch (error) {
          throw new Error(text(
            'Ungültige Antwort des Backends.',
            'Invalid response from the backend.'
          ));
        }
      }
      if (!response.ok) {
        const message = payload && payload.error && payload.error.message
          ? String(payload.error.message)
          : text('Einstellungen konnten nicht gespeichert werden.',
              'Settings could not be saved.');
        throw new Error(message);
      }
      return payload || {};
    });
  }

  function requestSettings(id) {
    return fetch(apiPath(id), {
      method: 'GET',
      credentials: 'same-origin',
      cache: 'no-store',
      headers: {Accept: 'application/json'}
    }).then(parseResponse);
  }

  function updateSettings(id, payload) {
    return fetch(apiPath(id), {
      method: 'POST',
      credentials: 'same-origin',
      cache: 'no-store',
      headers: Object.assign({
        Accept: 'application/json',
        'Content-Type': 'application/json'
      }, csrfHeaders()),
      body: JSON.stringify(payload)
    }).then(parseResponse);
  }

  function installStyle() {
    if (document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = [
      '.series-artwork-settings-grid{display:grid;gap:1rem}',
      '.series-artwork-settings-grid label{display:grid;gap:.4rem;font-weight:600}',
      '.series-artwork-settings-grid select,.series-artwork-settings-grid input{max-width:48rem;padding:.7rem .8rem;border:1px solid var(--line,#334155);border-radius:.55rem;background:var(--panel,#0f172a);color:inherit}',
      '.series-artwork-settings-actions{display:flex;gap:.75rem;align-items:center;flex-wrap:wrap}',
      '.series-artwork-settings-status{min-height:1.4em}',
      '.series-artwork-settings-help{margin:.25rem 0 0;padding-left:1.25rem}',
      '.series-artwork-settings-token-state{font-weight:600}',
      '.series-artwork-settings-muted{opacity:.78}',
      '.series-artwork-settings-error{color:#ef4444}',
      '.series-artwork-settings-success{color:#22c55e}'
    ].join('');
    document.head.appendChild(style);
  }

  function element(tag, attributes, content) {
    const node = document.createElement(tag);
    Object.keys(attributes || {}).forEach(function (name) {
      if (name === 'className') node.className = attributes[name];
      else if (name === 'htmlFor') node.htmlFor = attributes[name];
      else node.setAttribute(name, attributes[name]);
    });
    if (content !== undefined) node.textContent = content;
    return node;
  }

  function setStatus(card, message, kind) {
    const status = card.querySelector('[data-series-artwork-status]');
    if (!status) return;
    status.textContent = message || '';
    status.className = 'series-artwork-settings-status' +
      (kind ? ' series-artwork-settings-' + kind : '');
  }

  function applySnapshot(card, snapshot) {
    const provider = card.querySelector('[data-series-artwork-provider]');
    const tokenState = card.querySelector('[data-series-artwork-token-state]');
    const tokenInput = card.querySelector('[data-series-artwork-token]');
    const clearToken = card.querySelector('[data-series-artwork-clear-token]');

    card.dataset.tmdbTokenConfigured = snapshot.tmdbTokenConfigured
      ? 'true'
      : 'false';
    provider.value = String(snapshot.provider || 'none');
    tokenState.textContent = snapshot.tmdbTokenConfigured
      ? text('TMDB-Token ist sicher hinterlegt.', 'TMDB token is stored securely.')
      : text('Kein TMDB-Token hinterlegt.', 'No TMDB token is stored.');
    tokenInput.value = '';
    tokenInput.placeholder = snapshot.tmdbTokenConfigured
      ? text('Leer lassen, um den Token beizubehalten',
          'Leave empty to keep the current token')
      : text('API Read Access Token einfügen',
          'Paste the API Read Access Token');
    clearToken.checked = false;
    clearToken.disabled = !snapshot.tmdbTokenConfigured;
  }

  function createCard(id) {
    const card = element('article', {
      id: CARD_ID,
      className: 'module-placeholder settings-card series-artwork-settings-grid'
    });

    card.appendChild(element('h3', {}, text(
      'Serienbilder',
      'Series artwork'
    )));
    card.appendChild(element('p', {className: 'series-artwork-settings-muted'}, text(
      'TVScraper bleibt die primäre Quelle. Nur wenn dort kein nutzbares Hauptbild vorhanden ist, wird der hier gewählte Fallback-Anbieter verwendet. Änderungen gelten sofort und erfordern keinen Daemon-Neustart.',
      'TVScraper remains the primary source. The selected fallback provider is used only when TVScraper has no usable primary artwork. Changes apply immediately without restarting the daemon.'
    )));

    const providerLabel = element('label', {htmlFor: 'series-artwork-provider'}, text(
      'Fallback-Anbieter',
      'Fallback provider'
    ));
    const provider = element('select', {
      id: 'series-artwork-provider',
      'data-series-artwork-provider': 'true'
    });
    [
      ['none', text('Kein externer Fallback', 'No external fallback')],
      ['tvmaze', 'TVmaze'],
      ['tmdb', 'TMDB']
    ].forEach(function (entry) {
      provider.appendChild(element('option', {value: entry[0]}, entry[1]));
    });
    providerLabel.appendChild(provider);
    card.appendChild(providerLabel);

    const tokenLabel = element('label', {htmlFor: 'series-artwork-tmdb-token'}, text(
      'TMDB API Read Access Token',
      'TMDB API Read Access Token'
    ));
    const tokenInput = element('input', {
      id: 'series-artwork-tmdb-token',
      type: 'password',
      autocomplete: 'new-password',
      spellcheck: 'false',
      'data-series-artwork-token': 'true'
    });
    tokenLabel.appendChild(tokenInput);
    card.appendChild(tokenLabel);

    card.appendChild(element('p', {
      className: 'series-artwork-settings-token-state',
      'data-series-artwork-token-state': 'true'
    }, text('Tokenstatus wird geladen …', 'Loading token status …')));

    const clearLabel = element('label', {}, '');
    const clearToken = element('input', {
      type: 'checkbox',
      'data-series-artwork-clear-token': 'true'
    });
    clearLabel.style.display = 'flex';
    clearLabel.style.alignItems = 'center';
    clearLabel.style.gap = '.55rem';
    clearLabel.appendChild(clearToken);
    clearLabel.appendChild(document.createTextNode(text(
      'Gespeicherten TMDB-Token entfernen',
      'Remove the stored TMDB token'
    )));
    card.appendChild(clearLabel);

    const help = element('div', {className: 'series-artwork-settings-muted'});
    help.appendChild(element('h3', {}, text(
      'TMDB-Token erstellen',
      'Create a TMDB token'
    )));
    const steps = element('ol', {className: 'series-artwork-settings-help'});
    [
      text('Bei TMDB anmelden oder ein kostenloses Konto erstellen.',
        'Sign in to TMDB or create a free account.'),
      text('In den Kontoeinstellungen den Bereich „API“ öffnen und bei Bedarf einen API-Zugang beantragen.',
        'Open “API” in the account settings and request API access if needed.'),
      text('Unter „API Read Access Token“ den langen Bearer-Token kopieren. Nicht den kurzen v3 API Key verwenden.',
        'Copy the long bearer token shown as “API Read Access Token”. Do not use the short v3 API key.'),
      text('Den Token hier einfügen, TMDB auswählen und speichern.',
        'Paste the token here, select TMDB, and save.')
    ].forEach(function (step) {
      steps.appendChild(element('li', {}, step));
    });
    help.appendChild(steps);
    help.appendChild(element('a', {
      href: 'https://www.themoviedb.org/settings/api',
      target: '_blank',
      rel: 'noopener noreferrer'
    }, text('TMDB-API-Einstellungen öffnen', 'Open TMDB API settings')));
    help.appendChild(element('p', {}, text(
      'Der Token wird nicht im Browser zurückgegeben und nur in einer root-lesbaren Secret-Datei gespeichert. TMDB-Daten unterliegen den TMDB-Nutzungsbedingungen und der erforderlichen Quellenangabe.',
      'The token is never returned to the browser and is stored only in a root-readable secret file. TMDB data remains subject to TMDB terms and attribution requirements.'
    )));
    card.appendChild(help);

    const actions = element('div', {className: 'series-artwork-settings-actions'});
    const save = element('button', {
      type: 'button',
      className: 'primary-button',
      'data-series-artwork-save': 'true'
    }, text('Speichern', 'Save'));
    actions.appendChild(save);
    actions.appendChild(element('span', {
      'data-series-artwork-status': 'true',
      role: 'status',
      'aria-live': 'polite'
    }, ''));
    card.appendChild(actions);

    save.addEventListener('click', function () {
      const selectedProvider = provider.value;
      const token = tokenInput.value.trim();
      const clear = clearToken.checked;

      if (selectedProvider === 'tmdb' && !token && !clear &&
          card.dataset.tmdbTokenConfigured !== 'true') {
        setStatus(card, text(
          'Für TMDB wird ein API Read Access Token benötigt.',
          'TMDB requires an API Read Access Token.'
        ), 'error');
        return;
      }

      save.disabled = true;
      setStatus(card, text(
        'Token wird geprüft und die Einstellung gespeichert …',
        'Validating the token and saving settings …'
      ));

      const payload = {
        backendId: id,
        provider: selectedProvider,
        clearTmdbReadAccessToken: clear,
        operationId: 'series-artwork-settings-' + Date.now()
      };
      if (token) payload.tmdbReadAccessToken = token;

      updateSettings(id, payload).then(function (snapshot) {
        applySnapshot(card, snapshot);
        setStatus(card, text(
          'Gespeichert. Die Einstellung ist sofort aktiv.',
          'Saved. The setting is active immediately.'
        ), 'success');
      }).catch(function (error) {
        setStatus(card, error.message, 'error');
      }).finally(function () {
        save.disabled = false;
      });
    });

    return card;
  }

  function settingsMount() {
    return document.querySelector('.settings-panel') ||
      document.querySelector('[data-settings-panel]');
  }

  function mount() {
    if (!selectedSettingsModule()) return;
    const id = backendId();
    const target = settingsMount();
    if (!target || !id) return;

    const existing = document.getElementById(CARD_ID);
    if (existing && existing.dataset.backendId === id) return;
    if (existing) existing.remove();
    if (loadingBackendId === id) return;

    installStyle();
    const card = createCard(id);
    card.dataset.backendId = id;
    target.appendChild(card);
    loadingBackendId = id;

    requestSettings(id).then(function (snapshot) {
      applySnapshot(card, snapshot);
      setStatus(card, '');
    }).catch(function (error) {
      setStatus(card, error.message, 'error');
    }).finally(function () {
      loadingBackendId = '';
    });
  }

  function start() {
    mount();
    const observer = new MutationObserver(mount);
    observer.observe(document.documentElement, {
      childList: true,
      subtree: true,
      attributes: true,
      attributeFilter: ['class', 'data-module']
    });
  }

  global.VdrSuiteSeriesArtworkSettings = Object.freeze({
    start: start,
    mount: mount
  });

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', start, {once: true});
  } else {
    start();
  }
})(window);
