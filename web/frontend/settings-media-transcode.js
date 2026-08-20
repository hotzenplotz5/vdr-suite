(function (global) {
  'use strict';

  const CARD_ID = 'media-transcode-backend-settings-card';
  const STYLE_ID = 'media-transcode-backend-settings-style';
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
    return Boolean(
      platform &&
      typeof platform.getSelectedModule === 'function' &&
      String(platform.getSelectedModule() || '') === 'settings'
    );
  }

  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function apiPath(id) {
    return '/api/backends/' + encodeURIComponent(id) +
      '/settings/media-transcode';
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
          : text(
              'Transkodierungs-Einstellung konnte nicht geladen oder gespeichert werden.',
              'The transcoding setting could not be loaded or saved.'
            );
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

  function installStyle() {
    if (document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = [
      '.media-transcode-settings-grid{display:grid;gap:1rem}',
      '.media-transcode-settings-grid label{display:grid;gap:.4rem;font-weight:600}',
      '.media-transcode-settings-grid select{max-width:48rem;padding:.7rem .8rem;border:1px solid var(--line,#334155);border-radius:.55rem;background:var(--panel,#0f172a);color:inherit}',
      '.media-transcode-settings-actions{display:flex;gap:.75rem;align-items:center;flex-wrap:wrap}',
      '.media-transcode-settings-status{min-height:1.4em}',
      '.media-transcode-settings-diagnostics{display:grid;grid-template-columns:repeat(auto-fit,minmax(14rem,1fr));gap:.6rem 1.25rem;margin:0}',
      '.media-transcode-settings-diagnostics div{display:grid;gap:.15rem}',
      '.media-transcode-settings-diagnostics dt{font-weight:600}',
      '.media-transcode-settings-diagnostics dd{margin:0;opacity:.86}',
      '.media-transcode-settings-help{margin:.25rem 0 0;padding-left:1.25rem}',
      '.media-transcode-settings-muted{opacity:.78}',
      '.media-transcode-settings-error{color:#ef4444}',
      '.media-transcode-settings-success{color:#22c55e}'
    ].join('');
    document.head.appendChild(style);
  }

  function booleanText(value) {
    return value
      ? text('ja', 'yes')
      : text('nein', 'no');
  }

  function sourceText(source) {
    if (source === 'managed') return text('Web-/Backend-Einstellung', 'managed setting');
    if (source === 'environment') return text('Deployment-Umgebung', 'deployment environment');
    return text('integrierter Standard', 'built-in default');
  }

  function modeText(mode) {
    if (mode === 'software') return text('Software (x264)', 'Software (x264)');
    if (mode === 'vaapi') return 'VAAPI';
    return text('Automatisch', 'Automatic');
  }

  function setStatus(card, message, kind) {
    const status = card.querySelector('[data-media-transcode-status]');
    if (!status) return;
    status.textContent = message || '';
    status.className = 'media-transcode-settings-status' +
      (kind ? ' media-transcode-settings-' + kind : '');
  }

  function setDiagnostic(card, name, value) {
    const target = card.querySelector('[data-media-transcode-' + name + ']');
    if (target) target.textContent = value;
  }

  function applySnapshot(card, snapshot) {
    const selector = card.querySelector('[data-media-transcode-mode]');
    selector.value = snapshot.managed
      ? String(snapshot.managedMode || 'auto')
      : 'deployment';

    setDiagnostic(card, 'effective', modeText(String(snapshot.effectiveMode || 'auto')));
    setDiagnostic(card, 'source', sourceText(String(snapshot.source || 'default')));

    const threshold = Number(snapshot.minimumRealtimeThreshold);
    setDiagnostic(
      card,
      'threshold',
      Number.isFinite(threshold) ? threshold.toFixed(2) + '×' : '—'
    );

    const calibration = snapshot.calibration || {};
    setDiagnostic(
      card,
      'calibration',
      calibration.profileValid
        ? text('gültiges Profil vorhanden', 'valid profile available')
        : (calibration.profilePresent
            ? text('Profil ungültig', 'profile invalid')
            : text('kein gültiges Profil', 'no valid profile'))
    );
    setDiagnostic(
      card,
      'software',
      text('kalibriert: ', 'calibrated: ') + booleanText(Boolean(calibration.softwareCalibrated)) +
        ' · ' + text('geeignet: ', 'suitable: ') + booleanText(Boolean(calibration.softwareSuitable))
    );

    const vaapi = snapshot.vaapi || {};
    setDiagnostic(
      card,
      'vaapi',
      text('verfügbar: ', 'available: ') + booleanText(Boolean(vaapi.available)) +
        ' · ' + text('kalibriert: ', 'calibrated: ') + booleanText(Boolean(vaapi.calibrated)) +
        ' · ' + text('geeignet: ', 'suitable: ') + booleanText(Boolean(vaapi.suitable))
    );
    setDiagnostic(card, 'vaapi-reason', String(vaapi.reason || '—'));
  }

  function diagnosticRow(list, label, attribute) {
    const item = element('div', {});
    const value = element('dd', {}, '—');
    value.setAttribute('data-media-transcode-' + attribute, 'true');
    item.appendChild(element('dt', {}, label));
    item.appendChild(value);
    list.appendChild(item);
  }

  function createCard(id) {
    const card = element('article', {
      id: CARD_ID,
      className: 'module-placeholder settings-card media-transcode-settings-grid'
    });

    card.appendChild(element('h3', {}, text(
      'Medien-Transkodierung',
      'Media transcoding'
    )));
    card.appendChild(element('p', {className: 'media-transcode-settings-muted'}, text(
      'Die Auswahl wird beim Start einer neuen Wiedergabe-Session aufgelöst. Bereits laufende Sessions behalten ihren Encoder und werden nicht umgeschaltet. Für diese Einstellung ist kein Daemon-Neustart nötig.',
      'The selection is resolved when a new playback session starts. Running sessions keep their encoder and are never switched in place. This setting does not require a daemon restart.'
    )));

    const modeLabel = element('label', {htmlFor: 'media-transcode-mode'}, text(
      'Video-Encoder',
      'Video encoder'
    ));
    const selector = element('select', {
      id: 'media-transcode-mode',
      'data-media-transcode-mode': 'true'
    });
    [
      ['deployment', text('Deployment-Vorgabe (Umgebung / Auto)', 'Deployment default (environment / auto)')],
      ['auto', text('Automatisch – nur gemessen echtzeitfähig', 'Automatic – measured real-time capable only')],
      ['software', text('Software erzwingen (x264)', 'Force software (x264)')],
      ['vaapi', text('VAAPI erzwingen', 'Force VAAPI')]
    ].forEach(function (entry) {
      selector.appendChild(element('option', {value: entry[0]}, entry[1]));
    });
    modeLabel.appendChild(selector);
    card.appendChild(modeLabel);

    const diagnostics = element('dl', {className: 'media-transcode-settings-diagnostics'});
    diagnosticRow(diagnostics, text('Wirksamer Modus', 'Effective mode'), 'effective');
    diagnosticRow(diagnostics, text('Quelle', 'Source'), 'source');
    diagnosticRow(diagnostics, text('Echtzeit-Schwelle', 'Real-time threshold'), 'threshold');
    diagnosticRow(diagnostics, text('Kalibrierprofil', 'Calibration profile'), 'calibration');
    diagnosticRow(diagnostics, text('Software', 'Software'), 'software');
    diagnosticRow(diagnostics, 'VAAPI', 'vaapi');
    diagnosticRow(diagnostics, text('VAAPI-Status', 'VAAPI status'), 'vaapi-reason');
    card.appendChild(diagnostics);

    const help = element('div', {className: 'media-transcode-settings-muted'});
    const notes = element('ul', {className: 'media-transcode-settings-help'});
    [
      text(
        'Automatisch startet einen Video-Transcode nur, wenn ein kalibrierter Encoder die konfigurierte Echtzeit-Schwelle erreicht.',
        'Automatic starts a video transcode only when a calibrated encoder reaches the configured real-time threshold.'
      ),
      text(
        'Software erzwingt den vorhandenen x264-Pfad und verwendet weiterhin die getrennte Preset-/Kalibrier-Policy.',
        'Software forces the existing x264 path and still uses the separate preset/calibration policy.'
      ),
      text(
        'VAAPI erzwingen überstimmt nur die Leistungsschwelle. Fehlt das Gerät oder unterstützt VAAPI die notwendige Transformation nicht, schlägt die neue Session geschlossen fehl; es gibt keinen stillen Software-Fallback.',
        'Force VAAPI overrides only the performance threshold. If the device is unavailable or the required transformation is unsupported, the new session fails closed; there is no silent software fallback.'
      )
    ].forEach(function (note) {
      notes.appendChild(element('li', {}, note));
    });
    help.appendChild(notes);
    card.appendChild(help);

    const actions = element('div', {className: 'media-transcode-settings-actions'});
    const save = element('button', {
      type: 'button',
      className: 'primary-button',
      'data-media-transcode-save': 'true'
    }, text('Speichern', 'Save'));
    actions.appendChild(save);
    actions.appendChild(element('span', {
      'data-media-transcode-status': 'true',
      role: 'status',
      'aria-live': 'polite'
    }, ''));
    card.appendChild(actions);

    save.addEventListener('click', function () {
      save.disabled = true;
      setStatus(card, text(
        'Einstellung wird gespeichert …',
        'Saving setting …'
      ));

      const selected = selector.value;
      const payload = {
        backendId: id,
        operationId: 'media-transcode-settings-' + Date.now()
      };
      if (selected === 'deployment') {
        payload.clearManagedOverride = true;
      } else {
        payload.videoEncoderMode = selected;
      }

      updateSettings(id, payload).then(function (snapshot) {
        applySnapshot(card, snapshot);
        setStatus(card, text(
          'Gespeichert. Die Auswahl gilt für neue Wiedergabe-Sessions.',
          'Saved. The selection applies to new playback sessions.'
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

  global.VdrSuiteMediaTranscodeSettings = Object.freeze({
    start: start,
    mount: mount
  });

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', start, {once: true});
  } else {
    start();
  }
})(window);
