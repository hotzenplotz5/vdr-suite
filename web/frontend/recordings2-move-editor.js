// Move-target composition and editor for Recordings 2 actions.
(function (global) {
  'use strict';

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

  function composeNewFolderTarget(basePath, folderName) {
    const rawName = String(folderName || '').trim();
    const normalizedName = normalizeFolderPath(rawName);
    if (!rawName || !normalizedName || normalizedName === '/' ||
        normalizedName.indexOf('/') >= 0) {
      return '';
    }
    const base = targetFolderPath(basePath);
    return base ? base + '/' + normalizedName : normalizedName;
  }

  function resolveMoveTarget(targetValue, newFolderValue, currentPath) {
    const pendingFolder = String(newFolderValue || '').trim();
    if (!pendingFolder) return normalizeFolderPath(targetValue);
    const explicitBase = String(targetValue || '').trim();
    const basePath = explicitBase
      ? targetFolderPath(targetValue)
      : targetFolderPath(currentPath);
    return composeNewFolderTarget(basePath, pendingFolder);
  }

  function create(recording, dependencies) {
    const deps = dependencies || {};
    const shared = deps.shared;
    const editor = deps.editor;
    const textInput = deps.textInput;
    const renderFolderBrowser = deps.renderFolderBrowser;
    const setStatus = deps.setStatus;
    const state = deps.state;
    const validate = deps.validate;
    const execute = deps.execute;
    const moveReadback = deps.moveReadback;
    if (!shared || typeof editor !== 'function' || typeof textInput !== 'function' ||
        typeof renderFolderBrowser !== 'function' || typeof setStatus !== 'function' ||
        typeof state !== 'function' || typeof validate !== 'function' ||
        typeof execute !== 'function' || typeof moveReadback !== 'function') {
      throw new Error('Recordings 2 Move-Editor-Abhängigkeiten sind nicht verfügbar.');
    }

    const ui = editor('Verschieben');
    ui.body.appendChild(shared.node(
      'p',
      'recordings2-action-copy',
      'VDR-Aufnahmeordner werden beim Verschieben angelegt. Du kannst einen bestehenden Ordner wählen oder hier einen neuen Zielordner erstellen.'
    ));
    const input = textInput(ui.body, 'Zielordner', '');
    input.placeholder = 'z. B. Filme/Archiv';
    const newFolder = textInput(ui.body, 'Neuer Ordner', '');
    newFolder.placeholder = 'z. B. Klassiker';
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

    function resolvePendingTarget() {
      const targetPath = resolveMoveTarget(input.value, newFolder.value, state().path);
      if (!targetPath) return '';
      if (String(newFolder.value || '').trim()) {
        input.value = targetPath;
        newFolder.value = '';
        setStatus(
          status,
          '',
          'Neuer Zielordner „' + targetPath +
            '“ vorbereitet – er wird beim Verschieben angelegt.'
        );
      }
      return targetPath;
    }

    const createFolderTarget = shared.createButton('Neuen Ordner als Ziel', function () {
      const targetPath = resolvePendingTarget();
      if (!targetPath) {
        setStatus(
          status,
          'error',
          'Bitte einen einzelnen gültigen Ordnernamen ohne „/“ oder „~“ eingeben.'
        );
        return;
      }
      apply.disabled = true;
      setStatus(
        status,
        '',
        'Neuer Zielordner „' + targetPath +
          '“ vorbereitet – er wird beim Verschieben angelegt. Bitte prüfen.'
      );
    });
    const check = shared.createButton('Prüfen', function () {
      const targetPath = resolvePendingTarget();
      if (!targetPath) {
        setStatus(
          status,
          'error',
          'Bitte zuerst einen Zielordner auswählen oder einen neuen Ordner eingeben.'
        );
        return;
      }
      validate(recording, 'MOVE', {targetPath: targetPath}, status, apply)
        .catch(function () {});
    });
    apply = shared.createButton('Verschieben', function () {
      const targetPath = resolvePendingTarget();
      if (!targetPath || !global.confirm(
        'Aufnahme nach „' +
          (targetPath === '/' ? 'Hauptordner' : targetPath) +
          '“ verschieben?'
      )) return;
      execute(
        recording,
        'MOVE',
        {targetPath: targetPath},
        status,
        apply,
        moveReadback(recording, targetPath),
        'Verschieben abgeschlossen.'
      ).catch(function () {});
    });
    apply.disabled = true;
    input.addEventListener('input', function () {
      apply.disabled = true;
      setStatus(status, '', 'Ziel geändert – bitte erneut prüfen.');
    });
    newFolder.addEventListener('input', function () {
      apply.disabled = true;
    });
    buttons.append(root, browse, createFolderTarget, check, apply);
    ui.body.append(status, browser, buttons);
    return ui.details;
  }

  global.VdrSuiteRecordings2MoveEditor = Object.freeze({
    create: create,
    normalizeFolderPath: normalizeFolderPath,
    targetFolderPath: targetFolderPath,
    composeNewFolderTarget: composeNewFolderTarget,
    resolveMoveTarget: resolveMoveTarget
  });
}(window));
