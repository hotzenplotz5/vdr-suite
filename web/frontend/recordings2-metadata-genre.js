// Manual canonical Genre editor for Recordings 2 metadata.
(function (global) {
  'use strict';

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function installStyles() {
    if (document.getElementById('recordings2-metadata-genre-styles')) return;
    const style = document.createElement('style');
    style.id = 'recordings2-metadata-genre-styles';
    style.textContent = [
      '.recordings2-metadata-genre{display:grid;gap:.55rem;margin-top:.8rem;padding:.75rem;border:1px solid rgba(148,163,184,.28);border-radius:.65rem;background:rgba(2,6,23,.35)}',
      '.recordings2-metadata-genre-row{display:flex;gap:.55rem;flex-wrap:wrap;align-items:center}',
      '.recordings2-metadata-genre-row select{flex:1 1 13rem;min-height:2.5rem;padding:.5rem .7rem;border-radius:.5rem;border:1px solid #64748b;background:#0f172a;color:#f8fafc}'
    ].join('');
    document.head.appendChild(style);
  }

  function backendPath(backendId) {
    return '/api/backends/' + encodeURIComponent(text(backendId) || 'default') +
      '/recordings/metadata/genre';
  }

  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function api() {
    const client = global.VdrSuiteClientApi;
    if (!client || typeof client.requestJson !== 'function') {
      throw new Error('Client API für manuelle Genres ist nicht verfügbar.');
    }
    return client;
  }

  function genrePayload(recording, genreId) {
    return {
      resourceKey: text(recording && recording.backendNativeId),
      genreId: text(genreId)
    };
  }

  function post(backendId, payload) {
    return api().requestJson(backendPath(backendId), {
      method: 'POST',
      headers: Object.assign({
        'Content-Type': 'application/json'
      }, csrfHeaders()),
      body: JSON.stringify(payload || {}),
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function get(backendId, recording) {
    return api().requestJson(
      backendPath(backendId) + '?resourceKey=' +
        encodeURIComponent(text(recording && recording.backendNativeId)),
      {method: 'GET', cache: 'no-store', credentials: 'same-origin'}
    );
  }

  function mount(section, recording, backendId, dependencies) {
    if (!section || !recording || !dependencies) return null;
    const node = dependencies.node;
    const button = dependencies.button;
    const setStatus = dependencies.setStatus;
    const refreshDetail = dependencies.refreshDetail;
    if (typeof node !== 'function' || typeof button !== 'function' ||
        typeof setStatus !== 'function' || typeof refreshDetail !== 'function') return null;
    installStyles();

    const genreSection = node('div', 'recordings2-metadata-genre');
    genreSection.appendChild(node('strong', '', 'Genre hinzufügen'));
    genreSection.appendChild(node(
      'p',
      '',
      'Ein manuelles Genre ergänzt die automatisch erkannten VDR- und TVScraper-Genres und bleibt bei Aktualisierungen erhalten.'
    ));
    const genreRow = node('div', 'recordings2-metadata-genre-row');
    const genreSelect = document.createElement('select');
    genreSelect.setAttribute('aria-label', 'Manuelles Genre');
    const loadingOption = document.createElement('option');
    loadingOption.value = '';
    loadingOption.textContent = 'Genres werden geladen …';
    genreSelect.appendChild(loadingOption);
    genreSelect.disabled = true;

    const saveGenreButton = button('Genre speichern', function () {
      saveGenreButton.disabled = true;
      setStatus('Genre wird gespeichert …', false);
      post(backendId, genrePayload(recording, genreSelect.value)).then(function (result) {
        const selected = text(result && result.selectedGenreId);
        genreSelect.value = selected;
        clearGenreButton.disabled = !selected;
        setStatus(
          selected ? 'Genre wurde zur Aufnahme hinzugefügt.' : 'Manuelles Genre wurde entfernt.',
          false
        );
        refreshDetail();
      }).catch(function (error) {
        saveGenreButton.disabled = false;
        setStatus(error.message || String(error), true);
      });
    }, 'primary');
    saveGenreButton.disabled = true;

    const clearGenreButton = button('Manuelles Genre entfernen', function () {
      clearGenreButton.disabled = true;
      saveGenreButton.disabled = true;
      setStatus('Manuelles Genre wird entfernt …', false);
      post(backendId, genrePayload(recording, '')).then(function () {
        genreSelect.value = '';
        clearGenreButton.disabled = true;
        saveGenreButton.disabled = false;
        setStatus('Manuelles Genre wurde entfernt.', false);
        refreshDetail();
      }).catch(function (error) {
        clearGenreButton.disabled = false;
        saveGenreButton.disabled = false;
        setStatus(error.message || String(error), true);
      });
    }, 'danger');
    clearGenreButton.disabled = true;

    genreSelect.addEventListener('change', function () {
      saveGenreButton.disabled = false;
    });
    genreRow.append(genreSelect, saveGenreButton, clearGenreButton);
    genreSection.appendChild(genreRow);
    section.appendChild(genreSection);

    get(backendId, recording).then(function (result) {
      genreSelect.replaceChildren();
      const none = document.createElement('option');
      none.value = '';
      none.textContent = 'Kein manuelles Genre';
      genreSelect.appendChild(none);
      const genres = result && Array.isArray(result.genres) ? result.genres : [];
      genres.forEach(function (genreValue) {
        const option = document.createElement('option');
        option.value = text(genreValue.id);
        option.textContent = text(genreValue.label) || text(genreValue.id);
        genreSelect.appendChild(option);
      });
      const selected = text(result && result.selectedGenreId);
      genreSelect.value = selected;
      genreSelect.disabled = false;
      saveGenreButton.disabled = false;
      clearGenreButton.disabled = !selected;
    }).catch(function (error) {
      genreSelect.disabled = true;
      saveGenreButton.disabled = true;
      clearGenreButton.disabled = true;
      setStatus(error.message || String(error), true);
    });

    return genreSection;
  }

  global.VdrSuiteRecordings2MetadataGenre = Object.freeze({
    mount: mount,
    __test: Object.freeze({backendPath: backendPath, genrePayload: genrePayload})
  });
}(window));
