(function (global) {
  'use strict';

  const path = '/api/v1/addons/rectools/activation';

  // Use the same bounded request correlation as the proven administrator API
  // mutation. This is diagnostic identity, never an authorization credential.
  function mutationCorrelationHeader() {
    if (!global.crypto || typeof global.crypto.getRandomValues !== 'function')
      return {};
    const bytes = new Uint8Array(12);
    global.crypto.getRandomValues(bytes);
    return {'X-Correlation-ID': 'addon-activation-' +
      Array.from(bytes, function (byte) {
        return byte.toString(16).padStart(2, '0');
      }).join('')};
  }

  function node(tag, text) {
    const result = global.document.createElement(tag);
    if (typeof text === 'string') result.textContent = text;
    return result;
  }

  function request(backendId, mutation, revision, enabled) {
    const session = global.VdrSuiteBrowserSession;
    const restore = session && typeof session.restore === 'function'
      ? Promise.resolve(session.restore()) : Promise.resolve();
    return restore.then(function () {
      const csrf = session && typeof session.csrfHeaders === 'function'
        ? session.csrfHeaders() : {};
      const url = mutation ? path : path + '?backendId=' + encodeURIComponent(backendId);
      const options = {
        credentials: 'same-origin', cache: 'no-store',
        headers: Object.assign({'Accept': 'application/json'}, mutation
          ? Object.assign({'Content-Type': 'application/json'},
              csrf || {}, mutationCorrelationHeader())
          : {})
      };
      if (mutation) {
        options.method = 'POST';
        options.body = JSON.stringify({
          backendId: backendId,
          desiredEnabled: enabled,
          expectedRevision: revision
        });
      }
      return global.fetch(url, options).then(function (response) {
        return response.text().then(function (text) {
          let body;
          try { body = text ? JSON.parse(text) : {}; }
          catch (error) { throw new Error('Ungültige Server-Antwort'); }
          if (!response.ok) {
            const message = body && typeof body.error === 'string'
              ? body.error : 'HTTP ' + response.status;
            const failure = new Error(message + ' (' + response.status + ')');
            failure.httpStatus = response.status;
            throw failure;
          }
          if (!body || body.backendId !== backendId ||
              typeof body.desiredEnabled !== 'boolean' ||
              !Number.isSafeInteger(body.revision) || body.revision < 0) {
            throw new Error('Ungültiger Aktivierungsstatus');
          }
          return body;
        });
      });
    });
  }

  // One existing administrator login; one GET and one guarded POST.
  // The server owns the root file and rejects nonlocal/invalid directories.
  const sourcesPath = '/api/v1/addons/rectools/sources';
  function sourcePath(value) {
    if (typeof value !== 'string' || value.length < 2 ||
        value.length > 512 || !value.startsWith('/') ||
        value.endsWith('/') || /[\\\x00-\x1f\x7f]/.test(value)) return false;
    return value.slice(1).split('/').every(function (part) {
      return part && part !== '.' && part !== '..';
    });
  }
  function sourceName(value) {
    return typeof value === 'string' && value.length > 0 &&
      value.length <= 64 &&
      !/[/\\"\x00-\x1f\x7f]/.test(value);
  }
  function overlap(a, b) {
    return a === b || a.startsWith(b + '/') || b.startsWith(a + '/');
  }
  function sourceRequest(backendId, revision, sources) {
    const session = global.VdrSuiteBrowserSession;
    const restoring = session && typeof session.restore === 'function'
      ? Promise.resolve(session.restore()) : Promise.resolve();
    const mutation = Array.isArray(sources);
    return restoring.then(function () {
      const headers = {'Accept': 'application/json'};
      const options = {credentials: 'same-origin', cache: 'no-store', headers};
      const url = mutation
        ? sourcesPath : sourcesPath + '?backendId=' + encodeURIComponent(backendId);
      if (mutation) {
        options.method = 'POST';
        headers['Content-Type'] = 'application/json';
        Object.assign(headers, session && typeof session.csrfHeaders === 'function'
          ? session.csrfHeaders() : {});
        options.body = JSON.stringify({
          backendId, revision,
          entries: encodeURIComponent(sources.map(function (item) {
            return item.path + '\t' + item.name;
          }).join('\n'))
        });
      }
      return global.fetch(url, options).then(function (response) {
        return response.text().then(function (text) {
          let data;
          try { data = JSON.parse(text); }
          catch (_) { throw new Error('Ungültige Serverantwort'); }
          if (!response.ok) {
            const code = data && data.error;
            const err = new Error(code === 'sources_changed'
              ? 'Die Einstellungen wurden zwischenzeitlich geändert. Neu laden.'
              : code === 'backend_is_not_local'
                ? 'Dieses Backend liegt auf einem anderen Server.'
                : code === 'import_in_progress'
                  ? 'Gerade läuft ein Import. Bitte danach erneut speichern.'
                  : code === 'invalid_sources' || code === 'overlapping_roots'
                    ? 'Ordner ungültig, fehlen oder überschneiden sich.'
                    : 'Importverzeichnisse können nicht gespeichert oder geladen werden.');
            err.status = response.status;
            throw err;
          }
          if (!data || data.backendId !== backendId ||
              typeof data.videoRoot !== 'string' ||
              !/^[0-9a-f]{16}$/.test(data.revision || '') ||
              !Array.isArray(data.sources) ||
              data.sources.length < 1 || data.sources.length > 17 ||
              !data.sources.every(function (item) {
                return item && sourcePath(item.path) && sourceName(item.name);
              })) throw new Error('Ungültige Importverzeichnis-Antwort');
          return data;
        });
      });
    });
  }

  function appendSourceEditor(card, backendId) {
    const section = node('section');
    section.hidden = true;
    section.className = 'settings-media-import-source-editor';
    section.appendChild(node('h4', 'Importverzeichnisse'));
    section.appendChild(node('p',
      'Importordner hinzufügen, benennen oder entfernen. ' +
      'Der oberste Eintrag ist das Standardverzeichnis.'));
    const destination = node('p', '');
    const rowsContainer = node('div');
    rowsContainer.className = 'settings-media-import-source-list';
    const add = node('button', '+ Importordner hinzufügen');
    const save = node('button', 'Speichern');
    const status = node('p', 'Importverzeichnisse werden geladen …');
    status.setAttribute('role', 'status');
    add.type = 'button';
    save.type = 'button';
    section.appendChild(destination);
    section.appendChild(rowsContainer);
    section.appendChild(add);
    section.appendChild(save);
    section.appendChild(status);
    card.appendChild(section);

    let state = null;
    let rows = [];
    let busy = false;

    function refreshControls() {
      add.disabled = busy || !state || rows.length >= 17;
      save.disabled = busy || !state;
    }
    function renderRows() {
      rowsContainer.replaceChildren();
      rows.forEach(function (entry, index) {
        const row = node('div');
        row.className = 'settings-media-import-source-row';
        const labelPath = node('label', 'Verzeichnis');
        const inputPath = node('input');
        inputPath.type = 'text';
        inputPath.value = entry.path;
        labelPath.appendChild(inputPath);
        const labelName = node('label', 'Anzeigename');
        const inputName = node('input');
        inputName.type = 'text';
        inputName.value = entry.name;
        labelName.appendChild(inputName);
        inputPath.addEventListener('input', function () {
          entry.path = inputPath.value;
          status.textContent = 'Noch nicht gespeichert.';
        });
        inputName.addEventListener('input', function () {
          entry.name = inputName.value;
          status.textContent = 'Noch nicht gespeichert.';
        });
        row.appendChild(labelPath);
        row.appendChild(labelName);
        if (index === 0) row.appendChild(node('strong', 'Standard'));
        else {
          const makeDefault = node('button', 'Als Standard festlegen');
          makeDefault.type = 'button';
          makeDefault.addEventListener('click', function () {
            rows.splice(index, 1);
            rows.unshift(entry);
            status.textContent = 'Noch nicht gespeichert.';
            renderRows();
          });
          row.appendChild(makeDefault);
        }
        if (rows.length > 1) {
          const remove = node('button', 'Entfernen');
          remove.type = 'button';
          remove.addEventListener('click', function () {
            rows.splice(index, 1);
            status.textContent = 'Noch nicht gespeichert.';
            renderRows();
          });
          row.appendChild(remove);
        }
        rowsContainer.appendChild(row);
      });
      refreshControls();
    }
    function load() {
      busy = true;
      refreshControls();
      return sourceRequest(backendId).then(function (value) {
        state = value;
        rows = value.sources.map(function (item) {
          return {path: item.path, name: item.name};
        });
        destination.textContent = 'Aufnahmeziel: ' + value.videoRoot +
          ' (bleibt unverändert)';
        section.hidden = false;
        renderRows();
        status.textContent = 'Einstellungen geladen.';
      }).catch(function (error) {
        state = null;
        // A failed backend configuration read is visible to an already
        // authorized administrator; 401/403 must still hide the panel.
        section.hidden = error.status === 401 || error.status === 403;
        status.textContent = 'Quellen konnten nicht geladen werden: ' +
          error.message;
      }).finally(function () {
        busy = false;
        refreshControls();
      });
    }

    add.addEventListener('click', function () {
      if (busy || !state || rows.length >= 17) return;
      rows.push({path: '', name: ''});
      renderRows();
      status.textContent = 'Neuen Ordner eintragen und speichern.';
    });
    save.addEventListener('click', function () {
      if (busy || !state || rows.length === 0 || rows.length > 17) return;
      for (let i = 0; i < rows.length; ++i) {
        if (!sourcePath(rows[i].path) || !sourceName(rows[i].name) ||
            overlap(rows[i].path, state.videoRoot) ||
            rows.some(function (other, j) {
              return j !== i && overlap(rows[i].path, other.path);
            })) {
          status.textContent = 'Bitte gültige, getrennte Verzeichnisse und Namen eingeben.';
          return;
        }
      }
      busy = true;
      refreshControls();
      status.textContent = 'Importverzeichnisse werden gespeichert …';
      sourceRequest(backendId, state.revision, rows).then(function (result) {
        state = result;
        rows = result.sources.map(function (item) {
          return {path: item.path, name: item.name};
        });
        renderRows();
        status.textContent = 'Gespeichert. Der Backend-Agent übernimmt die ' +
          'Änderung beim nächsten Scan; kein Neustart erforderlich.';
      }).catch(function (error) {
        status.textContent = 'Nicht gespeichert: ' + error.message;
      }).finally(function () {
        busy = false;
        refreshControls();
      });
    });
    return {section: section, load: load};
  }

  function render(parent, backendId) {
    const card = node('article');
    // Never flash an administrator-only panel before the protected read.
    card.hidden = true;
    card.className = 'module-placeholder settings-card settings-addons-card';
    card.appendChild(node('h3', 'Media Tools'));
    card.appendChild(node('p',
      'Media Tools werden separat installiert. Diese Anzeige prüft ' +
      'den gespeicherten Aktivierungszustand. Die Verfügbarkeit der ' +
      'lokalen Worker wird bei Import- und Shrink-Aufträgen geprüft.'));
    const status = node('p', 'Aktivierung wird geladen …');
    status.setAttribute('role', 'status');
    status.setAttribute('aria-live', 'polite');
    const details = node('p', '');
    const refresh = node('button', 'Status aktualisieren');
    const toggle = node('button', 'Aktivieren');
    refresh.type = 'button';
    toggle.type = 'button';
    toggle.disabled = true;
    card.appendChild(status);
    card.appendChild(details);
    card.appendChild(refresh);
    card.appendChild(toggle);
    const sourceEditor = appendSourceEditor(card, backendId);
    parent.appendChild(card);
    if (typeof backendId !== 'string' ||
        !/^[a-zA-Z0-9_.:-]{1,128}$/.test(backendId) ||
        backendId === '.' || backendId === '..') {
      card.hidden = false;
      status.textContent = 'Zuerst ein gültiges Backend auswählen.';
      refresh.disabled = true;
      return card;
    }

    let current = null;
    let busy = false;
    function controls() {
      refresh.disabled = busy;
      // The admin enable/disable preference must remain usable while the
      // worker is offline. Actual imports validate worker readiness later.
      toggle.disabled = busy || !current;
      toggle.title = '';
    }
    function reload() {
      busy = true; current = null; controls();
      status.textContent = 'Aktivierung wird geprüft …';
      return request(backendId, false).then(function (result) {
        current = result;
        card.hidden = false;
        // Source configuration is not an Agent-lease operation.
        sourceEditor.load();
        // A current, independently verified provider proves more than a
        // saved intent, but still says nothing about dpkg package ownership.
        const trusted = result.providerVerified === true;
        const ready = result.agentReady === true;
        const bound = result.activationBoundToProvider === true;
        status.textContent = result.desiredEnabled
          ? (trusted && ready && bound
            ? 'Media Tools aktiviert und betriebsbereit'
            : 'Media Tools aktiviert; Worker-Verfügbarkeit wird beim Auftrag geprüft')
          : 'Media Tools deaktiviert';
        details.textContent = 'Backend: ' + backendId +
          ' · Freigabe-Revision: ' + result.revision;
        toggle.textContent = result.desiredEnabled ? 'Deaktivieren' : 'Aktivieren';
      }).catch(function (error) {
        details.textContent = '';
        sourceEditor.section.hidden = true;
        if (error.httpStatus === 401 || error.httpStatus === 403) {
          // No role.admin grant: no visible administration controls.
          card.hidden = true;
          status.textContent = '';
          return;
        }
        card.hidden = false;
        status.textContent = 'Aktivierung nicht verfügbar: ' + error.message;
      }).finally(function () {
        busy = false; controls();
      });
    }

    refresh.addEventListener('click', reload);
    toggle.addEventListener('click', function () {
      if (!current || busy) return;
      const wanted = !current.desiredEnabled;
      const question = wanted
        ? 'Media Tools für Backend ' + backendId + ' aktivieren?'
        : 'Media Tools für Backend ' + backendId + ' deaktivieren?';
      if (!global.confirm(question)) return;
      const expectedRevision = current.revision;
      busy = true; controls();
      status.textContent = 'Administratorfreigabe wird gespeichert …';
      request(backendId, true, expectedRevision, wanted).then(function () {
        return reload();
      }).catch(function (error) {
        // Refresh the authoritative revision, then preserve the failure
        // message. Previously reload() replaced it immediately and left
        // operators with a misleading "deaktiviert" message.
        return reload().then(function () {
          if (!card.hidden) {
            status.textContent = 'Änderung abgelehnt: ' + error.message;
          }
        });
      }).finally(function () { busy = false; controls(); });
    });
    reload();
    return card;
  }

  global.VdrSuiteAddonSettings = Object.freeze({render: render});
})(window);
