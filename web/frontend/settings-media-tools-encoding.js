(function (global) {
  'use strict';
  // MT.ENC3.2: import and HEVC software quality, not playback transcoding.
  const CARD_ID = 'media-tools-encoding-settings-card';
  const STYLE_ID = 'media-tools-encoding-settings-style';
  const API = '/api/v1/addons/media-tools/encoding';
  let loading = false;

  function english() {
    return String(document.documentElement.lang || '').toLowerCase().startsWith('en');
  }
  function tr(de, en) { return english() ? en : de; }
  function node(tag, content) {
    const element = document.createElement(tag);
    if (content !== undefined) element.textContent = content;
    return element;
  }
  function backendId() {
    const platform = global.VdrSuitePlatform;
    return platform && typeof platform.getSelectedBackendId === 'function'
      ? String(platform.getSelectedBackendId() || 'default') : 'default';
  }
  function selectedSettings() {
    const platform = global.VdrSuitePlatform;
    return platform && typeof platform.getSelectedModule === 'function' &&
      String(platform.getSelectedModule() || '') === 'settings';
  }
  function request(id, mutation) {
    const session = global.VdrSuiteBrowserSession;
    const ready = session && typeof session.restore === 'function'
      ? Promise.resolve(session.restore()) : Promise.resolve();
    return ready.then(function () {
      const headers = {'Accept': 'application/json'};
      if (mutation) {
        headers['Content-Type'] = 'application/json';
        Object.assign(headers,
          session && typeof session.csrfHeaders === 'function'
            ? session.csrfHeaders() : {});
      }
      return global.fetch(mutation ? API : API + '?backendId=' +
        encodeURIComponent(id), {
        method: mutation ? 'POST' : 'GET',
        credentials: 'same-origin',
        cache: 'no-store',
        headers: headers,
        body: mutation ? JSON.stringify(mutation) : undefined
      }).then(function (response) {
        return response.text().then(function (body) {
          let payload;
          try { payload = JSON.parse(body); }
          catch (_) { throw new Error(tr('Ungültige Serverantwort', 'Invalid server response')); }
          if (!response.ok) {
            const code = String(payload && payload.error || response.status);
            throw new Error(code === 'encoding_changed'
              ? tr('Einstellungen zwischenzeitlich geändert. Bitte neu öffnen.',
                   'Settings changed elsewhere. Please reopen.')
              : code === 'backend_is_not_local'
                ? tr('Nur lokaler Agent konfigurierbar.',
                     'Only the local agent can be configured.')
                : tr('Encoding-Einstellungen nicht verfügbar: ',
                     'Encoding settings unavailable: ') + code);
          }
          if (!payload || payload.backendId !== id ||
              !/^[0-9a-f]{16}$/.test(payload.revision || '') ||
              !Number.isInteger(payload.importCrf) ||
              !Number.isInteger(payload.shrinkCrf) ||
              payload.importCrf < 0 || payload.importCrf > 51 ||
              payload.shrinkCrf < 0 || payload.shrinkCrf > 51 ||
              typeof payload.importPreset !== 'string' ||
              typeof payload.shrinkPreset !== 'string')
            throw new Error(tr('Ungültige Encoding-Antwort', 'Invalid encoding response'));
          return payload;
        });
      });
    });
  }
  function chooser(label, options) {
    const wrapper = node('label');
    wrapper.appendChild(node('span', label));
    const select = node('select');
    options.forEach(function (entry) {
      const option = node('option', entry[1]);
      option.value = String(entry[0]);
      select.appendChild(option);
    });
    wrapper.appendChild(select);
    return {wrapper: wrapper, select: select};
  }
  function selectValue(select, value) {
    const selected = String(value);
    if (!Array.from(select.options).some(function (o) { return o.value === selected; })) {
      const old = node('option', tr('Bisheriger Wert: ', 'Existing value: ') + selected);
      old.value = selected;
      select.appendChild(old);
    }
    select.value = selected;
  }
  function createCard(id) {
    const card = node('article');
    card.id = CARD_ID;
    card.dataset.backendId = id;
    card.className = 'module-placeholder settings-card media-tools-encoding-grid';
    card.appendChild(node('h3', 'Media Tools – Encoding'));
    card.appendChild(node('p', tr(
      'H.264-Import und HEVC-Verkleinerung. Wiedergabe-Transkodierung bleibt getrennt.',
      'H.264 import and HEVC shrink. Playback transcoding remains separate.'
    )));
    const h264 = chooser(tr('H.264 – Qualität', 'H.264 – quality'), [
      [19, tr('Hoch (CRF 19)', 'High (CRF 19)')],
      [23, tr('Ausgewogen (CRF 23)', 'Balanced (CRF 23)')],
      [27, tr('Hohe Kompression (CRF 27)', 'High compression (CRF 27)')]
    ]);
    const h264Speed = chooser(tr('H.264 – Geschwindigkeit', 'H.264 – speed'), [
      ['fast', tr('Schnell', 'Fast')],
      ['medium', tr('Ausgewogen', 'Balanced')],
      ['slow', tr('Langsam', 'Slow')]
    ]);
    const h265 = chooser(tr('HEVC – Qualität', 'HEVC – quality'), [
      [23, tr('Hoch (CRF 23)', 'High (CRF 23)')],
      [27, tr('Ausgewogen (CRF 27)', 'Balanced (CRF 27)')],
      [30, tr('Hohe Kompression (CRF 30)', 'High compression (CRF 30)')]
    ]);
    const h265Speed = chooser(tr('HEVC – Geschwindigkeit', 'HEVC – speed'), [
      ['fast', tr('Schnell', 'Fast')],
      ['medium', tr('Ausgewogen', 'Balanced')],
      ['slow', tr('Langsam', 'Slow')]
    ]);
    [h264, h264Speed, h265, h265Speed].forEach(function (control) {
      card.appendChild(control.wrapper);
    });
    card.appendChild(node('p', tr(
      'CRF und diese Presets steuern x264/x265 und einen Software-Fallback. ' +
      'VAAPI, QSV und NVENC behalten ihre eigenen Qualitätsparameter; ' +
      'VDR-Format, Main/8 Bit, MPEG-TS, GOP, Audio und Index bleiben fest.',
      'CRF and these presets control x264/x265 and software fallback. ' +
      'VAAPI, QSV and NVENC retain their separate quality parameters; ' +
      'VDR format, Main/8-bit, MPEG-TS, GOP, audio and index remain fixed.'
    )));
    const save = node('button', tr('Speichern', 'Save'));
    save.type = 'button';
    save.className = 'primary-button';
    save.disabled = true;
    card.appendChild(save);
    const info = node('p', tr('Einstellungen werden geladen …', 'Loading settings …'));
    info.setAttribute('role', 'status');
    card.appendChild(info);
    let current = null;
    function apply(snapshot) {
      current = snapshot;
      selectValue(h264.select, snapshot.importCrf);
      selectValue(h264Speed.select, snapshot.importPreset);
      selectValue(h265.select, snapshot.shrinkCrf);
      selectValue(h265Speed.select, snapshot.shrinkPreset);
      save.disabled = false;
    }
    save.addEventListener('click', function () {
      if (!current) return;
      save.disabled = true;
      info.textContent = tr('Speichern …', 'Saving …');
      request(id, {
        backendId: id,
        revision: current.revision,
        importCrf: Number(h264.select.value),
        importPreset: h264Speed.select.value,
        shrinkCrf: Number(h265.select.value),
        shrinkPreset: h265Speed.select.value
      }).then(function (saved) {
        apply(saved);
        info.textContent = tr(
          'Gespeichert – für neue Aufträge; laufende Aufträge bleiben unverändert.',
          'Saved for new jobs; running jobs are unchanged.'
        );
      }).catch(function (error) {
        info.textContent = error.message;
      }).finally(function () { save.disabled = !current; });
    });
    request(id).then(function (snapshot) {
      apply(snapshot);
      info.textContent = tr('Agent-Konfiguration geladen.', 'Agent configuration loaded.');
    }).catch(function (error) { info.textContent = error.message; });
    return card;
  }
  function mount() {
    if (!selectedSettings() || loading) return;
    const target = document.querySelector('.settings-panel') ||
      document.querySelector('[data-settings-panel]');
    const id = backendId();
    if (!target || !id) return;
    const existing = document.getElementById(CARD_ID);
    if (existing && existing.dataset.backendId === id) return;
    if (existing) existing.remove();
    loading = true;
    if (!document.getElementById(STYLE_ID)) {
      const style = node('style');
      style.id = STYLE_ID;
      style.textContent = '.media-tools-encoding-grid{display:grid;gap:.8rem}' +
        '.media-tools-encoding-grid label{display:grid;gap:.3rem;font-weight:600}' +
        '.media-tools-encoding-grid select{max-width:35rem;padding:.6rem;' +
        'border:1px solid var(--line,#334155);border-radius:.5rem;' +
        'background:var(--panel,#0f172a);color:inherit}';
      document.head.appendChild(style);
    }
    target.appendChild(createCard(id));
    loading = false;
  }
  function start() {
    mount();
    new MutationObserver(mount).observe(document.documentElement, {
      childList: true, subtree: true, attributes: true,
      attributeFilter: ['class', 'data-module']
    });
  }
  global.VdrSuiteMediaToolsEncodingSettings = Object.freeze({start: start, mount: mount});
  if (document.readyState === 'loading')
    document.addEventListener('DOMContentLoaded', start, {once: true});
  else start();
})(window);
