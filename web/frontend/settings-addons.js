(function (global) {
  'use strict';

  const path = '/api/v1/addons/rectools/activation';

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
          ? Object.assign({'Content-Type': 'application/json'}, csrf || {})
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

  // UX2: a draft-only admin interface. The browser does not get source paths
  // from the Agent, and MUST NOT write the protected root inventory. This
  // prints a human-reviewed OFFLINE --check command, never executes it.
  function sourceRootPath(value) {
    if (typeof value !== 'string' || value.length > 512 ||
        !/^\/[A-Za-z0-9_./-]+$/.test(value) || value === '/' ||
        value.endsWith('/')) return false;
    const parts = value.slice(1).split('/');
    return parts.every(function (part) {
      return part.length > 0 && part !== '.' && part !== '..';
    });
  }

  function sourceName(value) {
    // A strict subset of the protected TSV alias grammar, with no quoting
    // metacharacters or variable expansions in generated shell arguments.
    return typeof value === 'string' &&
      /^[A-Za-z0-9 _.-]{1,64}$/.test(value) && value.trim() === value;
  }

  function shellValue(value) {
    // Only called after the strict allowlist validation above.
    return "'" + value + "'";
  }

  function sourcePathsOverlap(a, b) {
    return a === b || a.indexOf(b + '/') === 0 || b.indexOf(a + '/') === 0;
  }

  function appendSourceDraft(card) {
    const section = node('section');
    section.className = 'settings-media-import-source-draft';
    section.appendChild(node('h4', 'Importquellen vorbereiten'));
    section.appendChild(node('p',
      'Administrator-Entwurf für den bestehenden Offline-Konfigurator. ' +
      'Es werden weder Konfigurationsdateien geändert noch neue Rechte erteilt. ' +
      'Die aktuellen Root-Pfade kann der Browser nicht auslesen.'));

    function field(title, hint) {
      const label = node('label', title);
      const input = node('input');
      input.type = 'text';
      input.value = '';
      input.placeholder = hint;
      label.appendChild(input);
      section.appendChild(label);
      return input;
    }

    const root = field('Standard-Importverzeichnis',
      'Absoluter Pfad auf dem Backend');
    const name = field('Anzeigename der Standardquelle', 'Filme');
    const video = field('VDR-Aufnahmeverzeichnis',
      'Absoluter Pfad auf dem Backend');
    const rows = node('div');
    rows.className = 'settings-media-import-additional-roots';
    section.appendChild(rows);
    const extras = [];

    const add = node('button', 'Weitere Quelle hinzufügen');
    add.type = 'button';
    const prepare = node('button', 'Prüfkommando vorbereiten');
    prepare.type = 'button';
    const message = node('p');
    message.setAttribute('role', 'status');
    const preview = node('pre');
    preview.className = 'settings-media-import-review-command';
    preview.textContent = '';

    function clearPreview() {
      preview.textContent = '';
      message.textContent = 'Entwurf geändert: Prüfbefehl neu erstellen.';
    }

    add.addEventListener('click', function () {
      if (extras.length >= 16) {
        message.textContent = 'Höchstens 16 zusätzliche Importquellen erlaubt.';
        return;
      }
      const row = node('div');
      const pathLabel = node('label', 'Zusätzliches Importverzeichnis');
      const pathInput = node('input');
      pathInput.type = 'text';
      pathInput.placeholder = 'Absoluter Backend-Pfad';
      pathLabel.appendChild(pathInput);
      const nameLabel = node('label', 'Anzeigename');
      const nameInput = node('input');
      nameInput.type = 'text';
      nameInput.placeholder = 'Serien';
      nameLabel.appendChild(nameInput);
      row.appendChild(pathLabel);
      row.appendChild(nameLabel);
      rows.appendChild(row);
      extras.push({pathInput: pathInput, nameInput: nameInput});
      pathInput.addEventListener('input', clearPreview);
      nameInput.addEventListener('input', clearPreview);
      add.disabled = extras.length >= 16;
      clearPreview();
    });

    prepare.addEventListener('click', function () {
      preview.textContent = '';
      const paths = [root.value].concat(extras.map(function (entry) {
        return entry.pathInput.value;
      }));
      const labels = [name.value].concat(extras.map(function (entry) {
        return entry.nameInput.value;
      }));
      if (!sourceRootPath(video.value) ||
          paths.some(function (path) { return !sourceRootPath(path); }) ||
          labels.some(function (label) { return !sourceName(label); })) {
        message.textContent = 'Ungültiger Pfad oder Anzeigename. ' +
          'Nur absolute kanonische Pfade und einfache Namen (maximal 64 Zeichen) verwenden.';
        return;
      }
      const all = [video.value].concat(paths);
      for (let i = 0; i < all.length; ++i) {
        for (let j = i + 1; j < all.length; ++j) {
          if (sourcePathsOverlap(all[i], all[j])) {
            message.textContent = 'Quellverzeichnisse dürfen sich weder ' +
              'überschneiden noch im VDR-Aufnahmeverzeichnis liegen.';
            return;
          }
        }
      }
      const args = [shellValue(root.value), shellValue(video.value),
        '--check', '--default-name', shellValue(name.value)];
      extras.forEach(function (entry) {
        args.push('--add-root', shellValue(entry.pathInput.value),
          shellValue(entry.nameInput.value));
      });
      preview.textContent =
        'sudo bash tools/addons/configure_media_tools_paths.sh ' +
        args.join(' ');
      message.textContent = 'Nur für die manuelle Prüfung auf dem Backend. ' +
        'Der Befehl enthält --check und verändert keine Importkonfiguration. ' +
        'Ein tatsächliches Übernehmen erfordert einen gesondert geprüften ' +
        'Offline-Administrationsvorgang mit angehaltenem Backend-Agenten.';
    });
    [root, name, video].forEach(function (input) {
      input.addEventListener('input', clearPreview);
    });
    section.appendChild(add);
    section.appendChild(prepare);
    section.appendChild(message);
    section.appendChild(preview);
    card.appendChild(section);
  }

  function render(parent, backendId) {
    const card = node('article');
    // Never flash an administrator-only panel before the protected read.
    card.hidden = true;
    card.className = 'module-placeholder settings-card settings-addons-card';
    card.appendChild(node('h3', 'Media Tools'));
    card.appendChild(node('p',
      'Import und HEVC-Shrink benötigen einen geprüften Backend-Agent, ' +
      'sowie aktuelle technische Integritätsprüfungen. Die Installation ' +
      'aktiviert das Addon; andere Benutzer erhalten getrennte Rechte.'));
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
    appendSourceDraft(card);
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
      toggle.disabled = busy || !current;
    }
    function reload() {
      busy = true; current = null; controls();
      status.textContent = 'Aktivierung wird geprüft …';
      return request(backendId, false).then(function (result) {
        current = result;
        card.hidden = false;
        status.textContent = result.activatedByInstall
          ? 'Durch Administratorinstallation aktiviert'
          : (result.desiredEnabled
            ? 'Addon aktiviert (Laufzeitstatus gesondert prüfen)'
            : 'Addon deaktiviert oder noch nicht installiert');
        details.textContent = 'Backend: ' + backendId +
          ' · Freigabe-Revision: ' + result.revision;
        toggle.textContent = result.desiredEnabled ? 'Deaktivieren' : 'Aktivieren';
      }).catch(function (error) {
        details.textContent = '';
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
        ? 'Media Tools für Backend ' + backendId +
          ' aktivieren? Nur zuvor geprüfte Handler und vorhandene Benutzerrechte dürfen Aufträge ausführen.'
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
