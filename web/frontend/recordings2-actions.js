// Recording action workflows for the independent Recordings 2 runtime.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  const folderArtwork = global.VdrSuiteRecordings2FolderArtwork;
  if (!shared) {
    console.error('VDR-Suite Recordings 2 shared runtime is unavailable');
    return;
  }

  const STYLE_ID = 'vdr-suite-recordings2-actions-styles';
  const READBACK_ATTEMPTS = 45;
  const READBACK_DELAY_MS = 1000;

  function normalizeFolderPath(value) {
    const raw = String(value || '').trim();
    if (raw === '/') return '/';
    return raw.replace(/~/g, '/').replace(/\\/g, '/')
      .split('/').map(function (part) { return part.trim(); })
      .filter(Boolean).join('/');
  }

  function targetFolderPath(value) {
    return normalizeFolderPath(value) === '/' ? '' : normalizeFolderPath(value);
  }

  function localTitle(recording) {
    const raw = shared.decodeDisplayText(shared.first(
      recording,
      ['title', 'name', 'displayName'],
      shared.recordingTitle(recording)
    )).replace(/~/g, '/');
    const parts = raw.split('/').map(function (part) { return part.trim(); }).filter(Boolean);
    return parts.length ? parts[parts.length - 1] : 'Aufnahme';
  }

  function nativeLeaf(recording) {
    const value = String(shared.first(
      recording,
      ['backendNativeId', 'nativePath', 'path', 'fileName'],
      ''
    )).replace(/\\/g, '/').replace(/\/+$/, '');
    const parts = value.split('/').filter(Boolean);
    return parts.length ? parts[parts.length - 1] : '';
  }

  function identity(recording) {
    return {
      nativeLeaf: nativeLeaf(recording),
      title: localTitle(recording).toLocaleLowerCase('de-DE'),
      start: String(shared.first(recording, ['startTime', 'start', 'date'], '')),
      duration: String(shared.first(recording, ['durationSeconds', 'duration'], ''))
    };
  }

  function candidateMatches(recording, expected) {
    const candidate = identity(recording);
    if (expected.nativeLeaf && candidate.nativeLeaf === expected.nativeLeaf) return true;
    if (!expected.title || candidate.title !== expected.title) return false;
    return Boolean(
      (expected.start && candidate.start === expected.start) ||
      (expected.duration && candidate.duration === expected.duration)
    );
  }

  function stringList(value) {
    return Array.isArray(value)
      ? value.map(function (entry) { return String(entry || '').trim(); }).filter(Boolean)
      : [];
  }

  function isDryRunReady(result) {
    return Boolean(result) &&
      result.success === false &&
      String(result.message || '') === 'dry-run backend execution skipped' &&
      stringList(result.warnings).includes('dry-run only') &&
      stringList(result.errors).length === 0;
  }

  function actionPayload(recording, backendId, action, extra) {
    const payload = {
      backendId: String(backendId || 'default'),
      recordingId: String(shared.first(recording, ['recordingId', 'id', 'nativeId'], '')),
      action: String(action || '').toUpperCase(),
      dryRun: true
    };
    const optional = {
      recordingPath: shared.first(recording, ['path', 'fileName', 'directory'], ''),
      backendNativeId: shared.first(recording, ['backendNativeId', 'nativePath'], ''),
      recordingTitle: shared.first(recording, ['title', 'name', 'displayName'], '')
    };
    Object.keys(optional).forEach(function (key) {
      if (String(optional[key] || '').trim()) payload[key] = String(optional[key]);
    });
    Object.keys(extra || {}).forEach(function (key) {
      const value = extra[key];
      if (value !== undefined && value !== null && value !== '') payload[key] = value;
    });
    return payload;
  }

  function installStyles() {
    if (document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = `
.recordings2-actions{display:grid;gap:.65rem;padding:.75rem;border:1px solid rgba(96,165,250,.35);border-radius:.82rem;background:rgba(15,23,42,.72)}
.recordings2-actions>summary,.recordings2-action-editor>summary{cursor:pointer;color:#f8fafc;font-weight:850}
.recordings2-action-copy{color:#94a3b8;font-size:.78rem;line-height:1.4}
.recordings2-action-list{display:grid;gap:.55rem}.recordings2-action-editor{padding:.65rem;border:1px solid rgba(148,163,184,.22);border-radius:.72rem;background:rgba(2,6,23,.65)}
.recordings2-action-body{display:grid;gap:.55rem;margin-top:.65rem}.recordings2-action-body label{display:grid;gap:.25rem;color:#cbd5e1;font-size:.78rem;font-weight:750}
.recordings2-action-body input{width:100%;min-height:2.7rem;padding:.55rem .65rem;border:1px solid #475569;border-radius:.62rem;background:#111827;color:#f8fafc;font:inherit}
.recordings2-action-buttons{display:flex;flex-wrap:wrap;gap:.45rem}.recordings2-action-buttons button{flex:1 1 10rem}.recordings2-action-buttons .danger{border-color:rgba(248,113,113,.65);background:rgba(153,27,27,.35);color:#fee2e2}
.recordings2-action-status{min-height:1.35rem;padding:.55rem .62rem;border:1px solid rgba(148,163,184,.2);border-radius:.62rem;background:rgba(30,41,59,.55);color:#cbd5e1;font-size:.78rem;line-height:1.35}
.recordings2-action-status.error{border-color:rgba(248,113,113,.5);color:#fecaca}.recordings2-action-status.success{border-color:rgba(34,197,94,.45);color:#bbf7d0}.recordings2-action-status.pending{border-color:rgba(56,189,248,.45);color:#bae6fd}
.recordings2-folder-browser{display:grid;gap:.45rem;padding:.55rem;border:1px solid rgba(148,163,184,.2);border-radius:.62rem;background:rgba(15,23,42,.7)}.recordings2-folder-browser-buttons{display:grid;grid-template-columns:repeat(auto-fit,minmax(9rem,1fr));gap:.4rem}
@media(max-width:720px){.recordings2-action-buttons{display:grid;grid-template-columns:1fr}.recordings2-action-buttons button{width:100%}}
`;
    document.head.appendChild(style);
  }

  function create(options) {
    const config = options && typeof options === 'object' ? options : {};

    function state() {
      return typeof config.getState === 'function' ? config.getState() : {};
    }

    function clientApi() {
      return shared.clientApi();
    }

    function requestFolder(path) {
      const api = clientApi();
      if (!api || typeof api.fetchClientRecordingFolder !== 'function') {
        return Promise.reject(new Error('Aufnahmeordner-API ist nicht verfügbar.'));
      }
      return api.fetchClientRecordingFolder({
        query: {
          backend: state().backendId || shared.selectedBackendId(),
          path: targetFolderPath(path),
          limit: shared.PAGE_SIZE,
          offset: 0,
          _: String(Date.now())
        },
        cache: 'no-store',
        credentials: 'same-origin'
      });
    }

    function requestBrowsableFolder(path) {
      return requestFolder(path).then(function (data) {
        if (!folderArtwork || typeof folderArtwork.resolveLeaves !== 'function') return data;
        return folderArtwork.resolveLeaves(data, requestFolder).then(function (result) {
          return Object.assign({}, data, {
            folders: result.folders,
            folderCount: result.folders.length
          });
        });
      });
    }

    function findMatchingRecording(data, expected, leafName) {
      const direct = shared.recordingList(data).find(function (candidate) {
        return candidateMatches(candidate, expected);
      });
      if (direct) return Promise.resolve(direct);

      const normalizedLeaf = String(leafName || '').trim().toLocaleLowerCase('de-DE');
      if (!normalizedLeaf) return Promise.resolve(null);
      const folder = shared.folderList(data).find(function (entry) {
        const name = shared.decodeDisplayText(shared.first(entry, ['name'], ''))
          .toLocaleLowerCase('de-DE');
        return shared.number(entry.recordingCount, 0) === 1 && name === normalizedLeaf;
      });
      if (!folder) return Promise.resolve(null);
      return requestFolder(shared.first(folder, ['path'], '')).then(function (child) {
        return shared.recordingList(child).find(function (candidate) {
          return candidateMatches(candidate, expected);
        }) || null;
      });
    }

    function run(mode, recording, action, extra) {
      const api = clientApi();
      const payload = actionPayload(
        recording,
        state().backendId || shared.selectedBackendId(),
        action,
        extra
      );
      if (!api) return Promise.reject(new Error('Recording-Action-API ist nicht verfügbar.'));
      if (mode === 'execute') {
        if (typeof api.fetchClientRecordingActionExecution !== 'function') {
          return Promise.reject(new Error('Recording-Action-Ausführung ist nicht verfügbar.'));
        }
        return api.fetchClientRecordingActionExecution({
          payload: payload,
          cache: 'no-store',
          credentials: 'same-origin'
        });
      }
      if (typeof api.fetchClientRecordingActionValidation !== 'function') {
        return Promise.reject(new Error('Recording-Action-Validierung ist nicht verfügbar.'));
      }
      return api.fetchClientRecordingActionValidation({
        payload: payload,
        cache: 'no-store',
        credentials: 'same-origin'
      });
    }

    function setStatus(node, type, message) {
      node.className = 'recordings2-action-status' + (type ? ' ' + type : '');
      node.textContent = String(message || '');
    }

    function finishAction() {
      if (typeof config.completeAction === 'function') {
        config.completeAction();
        return;
      }
      if (typeof config.closeDetail === 'function') config.closeDetail();
      if (typeof config.reload === 'function') config.reload();
    }

    function poll(check, status, successMessage) {
      let attempts = 0;
      function attempt() {
        attempts += 1;
        Promise.resolve(check()).then(function (ready) {
          if (ready) {
            setStatus(status, 'success', successMessage);
            global.setTimeout(finishAction, 250);
            return;
          }
          if (attempts >= READBACK_ATTEMPTS) {
            setStatus(status, 'error', 'Backend bestätigt, aber der neue Recording-Cache-Stand ist noch nicht sichtbar. Bitte neu laden.');
            return;
          }
          global.setTimeout(attempt, READBACK_DELAY_MS);
        }).catch(function (error) {
          if (attempts >= READBACK_ATTEMPTS) {
            setStatus(status, 'error', 'Cache-Abgleich fehlgeschlagen: ' + error.message);
            return;
          }
          global.setTimeout(attempt, READBACK_DELAY_MS);
        });
      }
      attempt();
    }

    function renameReadback(recording, newName) {
      const expected = identity(recording);
      const normalizedName = shared.decodeDisplayText(newName).toLocaleLowerCase('de-DE');
      return function () {
        return requestFolder(state().path || '').then(function (data) {
          return findMatchingRecording(data, expected, normalizedName);
        }).then(function (candidate) {
          return Boolean(candidate) &&
            localTitle(candidate).toLocaleLowerCase('de-DE') === normalizedName;
        });
      };
    }

    function deleteReadback(recording) {
      const expected = identity(recording);
      return function () {
        return requestFolder(state().path || '').then(function (data) {
          return findMatchingRecording(data, expected, expected.title);
        }).then(function (candidate) {
          return !candidate;
        });
      };
    }

    function moveReadback(recording, targetPath) {
      const expected = identity(recording);
      return function () {
        return requestFolder(targetPath).then(function (data) {
          return findMatchingRecording(data, expected, expected.title);
        }).then(function (candidate) {
          return Boolean(candidate);
        });
      };
    }

    function validate(recording, action, extra, status, executeButton, safetyCheck) {
      executeButton.disabled = true;
      setStatus(status, 'pending', 'Aktion wird geprüft …');
      return run('validate', recording, action, Object.assign({dryRun: true}, extra || {}))
        .then(function (result) {
          if (!result || result.valid !== true) {
            throw new Error(String(result && (result.message || result.error) || 'Validierung nicht freigegeben.'));
          }
          if (!safetyCheck) return result;
          setStatus(status, 'pending', 'Backend-Sicherheit wird als Dry-Run geprüft …');
          return run('execute', recording, action, Object.assign({dryRun: true}, extra || {}))
            .then(function (safetyResult) {
              if (!safetyCheck(safetyResult)) {
                throw new Error('Die Backend-Sicherheitsprüfung hat die Aktion nicht freigegeben.');
              }
              return safetyResult;
            });
        })
        .then(function (result) {
          executeButton.disabled = false;
          setStatus(status, 'success', 'Prüfung erfolgreich. Die Aktion kann ausgeführt werden.');
          return result;
        })
        .catch(function (error) {
          executeButton.disabled = true;
          setStatus(status, 'error', error.message);
          throw error;
        });
    }

    function execute(recording, action, extra, status, button, readback, successMessage) {
      button.disabled = true;
      setStatus(status, 'pending', 'Aktion wird ausgeführt …');
      return run('execute', recording, action, Object.assign({dryRun: false}, extra || {}))
        .then(function (result) {
          if (!result || result.success !== true) {
            throw new Error(String(result && (result.message || result.error) || 'Das Backend hat die Aktion abgelehnt.'));
          }
          setStatus(status, 'pending', 'Backend bestätigt. Recording-Cache wird abgeglichen …');
          poll(readback, status, successMessage);
          return result;
        })
        .catch(function (error) {
          button.disabled = false;
          setStatus(status, 'error', error.message);
          throw error;
        });
    }

    function editor(title) {
      const details = document.createElement('details');
      details.className = 'recordings2-action-editor';
      details.appendChild(shared.node('summary', '', title));
      const body = document.createElement('div');
      body.className = 'recordings2-action-body';
      details.appendChild(body);
      return {details: details, body: body};
    }

    function textInput(body, labelText, value) {
      const label = document.createElement('label');
      label.appendChild(shared.node('span', '', labelText));
      const input = document.createElement('input');
      input.type = 'text';
      input.value = value || '';
      input.autocomplete = 'off';
      label.appendChild(input);
      body.appendChild(label);
      return input;
    }

    function createRenameEditor(recording) {
      const ui = editor('Umbenennen');
      const input = textInput(ui.body, 'Neuer Name', localTitle(recording));
      const status = shared.node('p', 'recordings2-action-status', 'Neuen Namen eingeben und prüfen.');
      status.setAttribute('role', 'status');
      const buttons = document.createElement('div');
      buttons.className = 'recordings2-action-buttons';
      let apply;
      const check = shared.createButton('Prüfen', function () {
        const newName = input.value.trim();
        if (!newName) {
          setStatus(status, 'error', 'Neuer Name darf nicht leer sein.');
          return;
        }
        validate(recording, 'RENAME', {newName: newName}, status, apply).catch(function () {});
      });
      apply = shared.createButton('Umbenennen', function () {
        const newName = input.value.trim();
        if (!newName || !global.confirm('Aufnahme wirklich in „' + newName + '“ umbenennen?')) return;
        execute(recording, 'RENAME', {newName: newName}, status, apply,
          renameReadback(recording, newName), 'Umbenennen abgeschlossen.').catch(function () {});
      });
      apply.disabled = true;
      input.addEventListener('input', function () {
        apply.disabled = true;
        setStatus(status, '', 'Name geändert – bitte erneut prüfen.');
      });
      buttons.append(check, apply);
      ui.body.append(status, buttons);
      return ui.details;
    }

    function renderFolderBrowser(holder, input, status, path) {
      setStatus(status, 'pending', 'Zielordner werden geladen …');
      requestBrowsableFolder(path).then(function (data) {
        const currentPath = targetFolderPath(shared.first(data, ['path'], path));
        holder.replaceChildren();
        holder.appendChild(shared.node('strong', '', currentPath
          ? shared.decodeDisplayText(currentPath) : 'Hauptordner'));
        const controls = document.createElement('div');
        controls.className = 'recordings2-action-buttons';
        controls.appendChild(shared.createButton('Diesen Ordner wählen', function () {
          input.value = currentPath || '/';
          setStatus(status, '', 'Ziel ausgewählt – bitte prüfen.');
          holder.hidden = true;
        }));
        if (currentPath) {
          controls.appendChild(shared.createButton('Eine Ebene zurück', function () {
            renderFolderBrowser(holder, input, status, shared.first(data, ['parentPath'], ''));
          }));
        }
        holder.appendChild(controls);
        const folders = document.createElement('div');
        folders.className = 'recordings2-folder-browser-buttons';
        shared.folderList(data).forEach(function (folder) {
          folders.appendChild(shared.createButton(
            shared.decodeDisplayText(shared.first(folder, ['name'], 'Ordner')),
            function () { renderFolderBrowser(holder, input, status, shared.first(folder, ['path'], '')); }
          ));
        });
        if (!shared.folderList(data).length) {
          folders.appendChild(shared.node('span', 'recordings2-action-copy', 'Keine Unterordner.'));
        }
        holder.appendChild(folders);
        holder.hidden = false;
        setStatus(status, '', 'Zielordner auswählen.');
      }).catch(function (error) {
        setStatus(status, 'error', error.message);
      });
    }

    function createMoveEditor(recording) {
      const ui = editor('Verschieben');
      const input = textInput(ui.body, 'Zielordner', '');
      input.placeholder = 'z. B. Filme/Archiv';
      const browser = document.createElement('div');
      browser.className = 'recordings2-folder-browser';
      browser.hidden = true;
      const status = shared.node('p', 'recordings2-action-status', 'Zielordner auswählen und prüfen.');
      const buttons = document.createElement('div');
      buttons.className = 'recordings2-action-buttons';
      let apply;
      const root = shared.createButton('Hauptordner', function () {
        input.value = '/';
        apply.disabled = true;
        setStatus(status, '', 'Hauptordner ausgewählt – bitte prüfen.');
      });
      const browse = shared.createButton('Ordner auswählen', function () {
        renderFolderBrowser(browser, input, status, '');
      });
      const check = shared.createButton('Prüfen', function () {
        const targetPath = normalizeFolderPath(input.value);
        if (!targetPath) {
          setStatus(status, 'error', 'Bitte zuerst einen Zielordner auswählen.');
          return;
        }
        validate(recording, 'MOVE', {targetPath: targetPath}, status, apply).catch(function () {});
      });
      apply = shared.createButton('Verschieben', function () {
        const targetPath = normalizeFolderPath(input.value);
        if (!targetPath || !global.confirm('Aufnahme nach „' +
            (targetPath === '/' ? 'Hauptordner' : targetPath) + '“ verschieben?')) return;
        execute(recording, 'MOVE', {targetPath: targetPath}, status, apply,
          moveReadback(recording, targetPath), 'Verschieben abgeschlossen.').catch(function () {});
      });
      apply.disabled = true;
      input.addEventListener('input', function () {
        apply.disabled = true;
        setStatus(status, '', 'Ziel geändert – bitte erneut prüfen.');
      });
      buttons.append(root, browse, check, apply);
      ui.body.append(status, browser, buttons);
      return ui.details;
    }

    function createDeleteEditor(recording) {
      const ui = editor('In Papierkorb verschieben');
      ui.body.appendChild(shared.node(
        'p',
        'recordings2-action-copy',
        'Die Aufnahme wird zuerst validiert und anschließend als Dry-Run gegen die Backend-Sicherheitsregeln geprüft.'
      ));
      const status = shared.node('p', 'recordings2-action-status', 'Papierkorb-Aktion zuerst prüfen.');
      const buttons = document.createElement('div');
      buttons.className = 'recordings2-action-buttons';
      let apply;
      const check = shared.createButton('Papierkorb prüfen', function () {
        validate(recording, 'DELETE', {}, status, apply, isDryRunReady).catch(function () {});
      });
      apply = shared.createButton('In Papierkorb verschieben', function () {
        if (!global.confirm('Aufnahme „' + localTitle(recording) + '“ in den VDR-Papierkorb verschieben?')) return;
        execute(recording, 'DELETE', {}, status, apply,
          deleteReadback(recording), 'Papierkorb-Aktion abgeschlossen.').catch(function () {});
      }, 'danger');
      apply.disabled = true;
      buttons.append(check, apply);
      ui.body.append(status, buttons);
      return ui.details;
    }

    function createPanel(recording) {
      installStyles();
      const panel = document.createElement('details');
      panel.className = 'recordings2-actions';
      panel.appendChild(shared.node('summary', '', 'Aufnahmeaktionen'));
      panel.appendChild(shared.node(
        'p',
        'recordings2-action-copy',
        'Umbenennen, Verschieben und Papierkorb verwenden die vorhandene serverseitige Recording-Action-Schnittstelle.'
      ));
      const editors = document.createElement('div');
      editors.className = 'recordings2-action-list';
      editors.append(createRenameEditor(recording), createMoveEditor(recording), createDeleteEditor(recording));
      panel.appendChild(editors);
      return panel;
    }

    return Object.freeze({createPanel: createPanel});
  }

  global.VdrSuiteRecordings2Actions = Object.freeze({
    create: create,
    __test: Object.freeze({
      normalizeFolderPath: normalizeFolderPath,
      targetFolderPath: targetFolderPath,
      localTitle: localTitle,
      identity: identity,
      candidateMatches: candidateMatches,
      isDryRunReady: isDryRunReady,
      actionPayload: actionPayload
    })
  });
}(window));