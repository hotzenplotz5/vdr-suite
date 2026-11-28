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
  const DELETE_QUEUE_BY_BACKEND = new Map();

  function moveEditorRuntime() {
    const runtime = global.VdrSuiteRecordings2MoveEditor;
    if (!runtime) throw new Error('Recordings 2 Move-Editor ist nicht verfügbar.');
    return runtime;
  }

  function normalizeFolderPath(value) {
    return moveEditorRuntime().normalizeFolderPath(value);
  }

  function targetFolderPath(value) {
    return moveEditorRuntime().targetFolderPath(value);
  }

  function composeNewFolderTarget(basePath, folderName) {
    return moveEditorRuntime().composeNewFolderTarget(basePath, folderName);
  }

  function resolveMoveTarget(targetValue, newFolderValue, currentPath) {
    return moveEditorRuntime().resolveMoveTarget(targetValue, newFolderValue, currentPath);
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

  function stringList(value) {
    return Array.isArray(value)
      ? value.map(function (entry) { return String(entry || '').trim(); }).filter(Boolean)
      : [];
  }

  function actionError(result, fallback) { const errors = stringList(result && result.errors);
    if (errors.some(function (value) { return value === 'recording_cut_operation_pending_delete_blocked' || value === 'recording_cut_active_delete_blocked'; })) return 'Original kann während des laufenden Schnitts nicht gelöscht werden.';
    if (errors.some(function (value) { return ['recording_cut_state_unavailable_delete_blocked', 'recording_cut_journal_unavailable_delete_blocked', 'recording_cut_journal_invalid_delete_blocked', 'recording_identity_unavailable_delete_blocked'].includes(value); })) return 'Löschen ist gesperrt, weil der Schnittstatus nicht sicher bestätigt werden kann.';
    return errors.join('; ') || String(result && (result.message || result.error) || fallback || 'Aktion wurde abgelehnt.');
  }
  function publicRecordingId(recording) {
    return shared.publicRecordingId(recording);
  }

  function publicActionPayload(recording, backendId, action, extra) {
    const payload = {backendId, recordingId: publicRecordingId(recording),
      action, dryRun: extra.dryRun !== false};
    if (action === 'RENAME') payload.newName = extra.newName;
    if (action === 'MOVE') payload.targetFolder = extra.targetPath;
    if (action === 'DELETE' && !payload.dryRun) payload.confirmDelete = true;
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
    const publicPreflights = new WeakMap();

    function state() {
      return typeof config.getState === 'function' ? config.getState() : {};
    }

    function clientApi() {
      return shared.clientApi();
    }

    function requestFolder(path, backendId) {
      const api = clientApi();
      if (!api || typeof api.fetchClientRecordingFolder !== 'function') {
        return Promise.reject(new Error('Aufnahmeordner-API ist nicht verfügbar.'));
      }
      return api.fetchClientRecordingFolder({
        query: {
          backend: backendId || state().backendId || shared.selectedBackendId(),
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

    function publicRequest(recording, action, extra, execute, etag) {
      const api = clientApi();
      if (!api || typeof api.fetchPublicRecordingAction !== 'function')
        return Promise.reject(new Error('Public-v1-Aufnahmeaktionen sind nicht verfügbar.'));
      const backendId = recording.backendId || state().backendId || shared.selectedBackendId();
      const payload = publicActionPayload(recording, backendId, action, extra);
      return api.fetchPublicRecordingAction({execute, payload, headers: etag ? {'If-Match':etag} : {},
        cache:'no-store', credentials:'same-origin'});
    }

    function publicValidate(recording, action, extra, status, button) {
      button.disabled = true;
      publicPreflights.delete(button);
      setStatus(status, 'pending', 'Aktion wird geprüft …');
      let target, payload;
      return shared.resolvePublicRecordingId(recording, recording.backendId, clientApi())
        .then(function (id) {
          target = Object.freeze(Object.assign({}, recording, {publicRecordingId: id}));
          payload = publicActionPayload(target, target.backendId, action,
            Object.assign({}, extra, {dryRun:true}));
          return publicRequest(target, action, Object.assign({}, extra, {dryRun:true}), false);
        }).then(function (result) {
          if (!result || !result.data || result.data.valid !== true ||
              result.data.recordingId !== payload.recordingId ||
              !/^"[^"]+"$/.test(result.etag || ''))
            throw new Error(actionError(result && result.data && result.data.valid === false ? result.data : null,
              'Backend-Prüfung nicht bestätigt.'));
          publicPreflights.set(button, {recording:target, source:recording, key:JSON.stringify(payload), etag:result.etag});
          button.disabled = false;
          setStatus(status, 'success', 'Prüfung erfolgreich. Die Aktion kann ausgeführt werden.');
          return result.data;
        }).catch(function (error) {
          button.disabled = true;
          setStatus(status, 'error', error.message);
          throw error;
        });
    }

    function publicExecute(recording, action, extra, status, button) {
      const proof = publicPreflights.get(button);
      publicPreflights.delete(button);
      button.disabled = true;
      const key = proof && JSON.stringify(publicActionPayload(proof.recording, recording.backendId, action,
        Object.assign({}, extra, {dryRun:true})));
      if (!proof || proof.source !== recording || proof.key !== key) {
        const error = new Error('Ziel geändert – bitte erneut prüfen.');
        setStatus(status, 'error', error.message);
        return Promise.reject(error);
      }
      setStatus(status, 'pending', 'Aktion wird ausgeführt …');
      return publicRequest(proof.recording, action, Object.assign({}, extra, {dryRun:false}), true, proof.etag)
        .then(function (result) {
          const data = result && result.data;
          if (!data || data.success !== true || data.executed !== true ||
              data.readbackVerified !== true || data.recordingId !== publicRecordingId(proof.recording) ||
              data.backendId !== recording.backendId || data.action !== action)
            throw new Error('Ausführung nicht eindeutig bestätigt. Bitte Aufnahmezustand prüfen.');
          return data;
        }).catch(function (error) {
          setStatus(status, 'error', error.message);
          throw error;
        });
    }

    function validate(recording, action, extra, status, button) {
      return publicValidate(recording, action, extra, status, button);
    }

    function execute(recording, action, extra, status, button, successMessage) {
      return publicExecute(recording, action, extra, status, button).then(function (result) {
        setStatus(status, 'success', successMessage);
        finishAction();
        return result;
      });
    }

    function executeDelete(recording, status, button) {
      return publicExecute(recording, 'DELETE', {}, status, button).then(function (result) {
        setStatus(status, 'success', 'Aufnahme wurde in den VDR-Papierkorb verschoben.');
        if (typeof config.completeDelete === 'function') config.completeDelete(recording);
        else finishAction();
        return result;
      });
    }

    function enqueueDelete(recording, status, button, confirmDelete) {
      const backendId = recording.backendId;
      let queue = DELETE_QUEUE_BY_BACKEND.get(backendId);
      if (!queue) {
        queue = {tail: Promise.resolve(), pending: 0};
        DELETE_QUEUE_BY_BACKEND.set(backendId, queue);
      }
      const previous = queue.tail;
      const wasQueued = queue.pending > 0;
      queue.pending += 1;
      button.disabled = true;
      if (wasQueued) setStatus(status, 'pending', 'Löschen vorgemerkt – wartet auf vorherige Papierkorb-Aktion …');
      const execution = previous.then(function () {
        return validate(recording, 'DELETE', {}, status, button)
          .then(function () {
            button.disabled = true;
            if (!confirmDelete()) {
              setStatus(status, '', 'Papierkorb-Aktion abgebrochen.');
              return null;
            }
            return executeDelete(recording, status, button);
          });
      });
      // The public server already confirms deletion through independent readback.
      const settled = execution;
      queue.tail = settled;
      const cleanup = function () {
        queue.pending = Math.max(0, queue.pending - 1);
        if (queue.pending === 0 && queue.tail === settled) {
          DELETE_QUEUE_BY_BACKEND.delete(backendId);
        }
      };
      settled.then(cleanup, cleanup);
      return execution;
    }

    function editor(title) {
      const details = document.createElement('details');
      details.className = 'recordings2-action-editor';
      details.appendChild(shared.node('summary', '', title));
      const body = document.createElement('div');
      body.className = 'recordings2-action-body';
      details.appendChild(body);
      return {details, body};
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
          'Umbenennen abgeschlossen.').catch(function () {});
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
      return moveEditorRuntime().create(recording, {
        shared: shared,
        editor: editor,
        textInput: textInput,
        renderFolderBrowser: renderFolderBrowser,
        setStatus: setStatus,
        state: state,
        validate: validate,
        execute: execute
      });
    }

    function createDeleteEditor(recording) {
      const ui = editor('In Papierkorb verschieben');
      ui.body.appendChild(shared.node('p', 'recordings2-action-copy',
        'Die Freigabe wird beim Öffnen geprüft. Erst danach kannst du das Verschieben bestätigen.'));
      const status = shared.node('p', 'recordings2-action-status', 'Freigabe noch nicht geprüft.');
      const buttons = document.createElement('div');
      buttons.className = 'recordings2-action-buttons';
      let busy = false, ready = false;
      function current() { return ui.details.isConnected && String(state().backendId || shared.selectedBackendId() || 'default') === recording.backendId; }
      const apply = shared.createButton('In Papierkorb verschieben', function () {
        if (busy || !ready || !current()) return;
        busy = true; ready = false; retry.disabled = true;
        enqueueDelete(recording, status, apply, function () {
          return current() && global.confirm('Aufnahme „' + localTitle(recording) + '“ in den VDR-Papierkorb verschieben?');
        }).catch(function () {}).finally(function () {
          busy = false; apply.disabled = true; retry.disabled = false;
        });
      }, 'danger');
      apply.disabled = true;
      function check() {
        if (busy || !current()) return;
        busy = true; ready = false; retry.disabled = true;
        validate(recording, 'DELETE', {}, status, apply)
          .then(function () { ready = current(); apply.disabled = !ready; })
          .catch(function () {}).finally(function () { busy = false; retry.disabled = false; });
      }
      const retry = shared.createButton('Freigabe erneut prüfen', check);
      ui.details.addEventListener('toggle', function () { if (ui.details.open) check(); });
      buttons.append(retry, apply);
      ui.body.append(status, buttons);
      return ui.details;
    }

    function createPanel(recording) {
      recording = Object.freeze(Object.assign({}, recording, {backendId: String(recording.backendId || state().backendId || shared.selectedBackendId() || 'default')}));
      installStyles();
      const panel = document.createElement('details');
      panel.className = 'recordings2-actions';
      panel.appendChild(shared.node('summary', '', 'Aufnahmeaktionen'));
      panel.appendChild(shared.node('p', 'recordings2-action-copy',
        'Aufnahme umbenennen, verschieben oder in den Papierkorb legen.'));
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
      composeNewFolderTarget: composeNewFolderTarget,
      resolveMoveTarget: resolveMoveTarget,
      localTitle: localTitle,
      actionError: actionError,
      publicActionPayload: publicActionPayload
    })
  });
}(window));
