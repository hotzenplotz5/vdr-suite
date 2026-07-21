(function(global) {
  'use strict';

  let mountTarget = null;
  let folderLoader = null;
  let actionRunner = null;
  let requestSequence = 0;
  let currentFolder = null;

  function textElement(tagName, text, className) {
    const element = document.createElement(tagName);
    element.textContent = String(text || '');
    if (className) element.className = className;
    return element;
  }

  function firstValue(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') return object[key];
    }
    return fallback;
  }

  function listFromResponse(data, key) {
    if (data && Array.isArray(data[key])) return data[key];
    if (data && Array.isArray(data.items) && key === 'recordings') return data.items;
    return [];
  }

  function normalizePath(value) {
    return String(value || '').split('/').map(part => part.trim()).filter(Boolean).join('/');
  }

  function decodeVdrText(value) {
    let result = String(value || '');
    for (let pass = 0; pass < 8; pass += 1) {
      const decoded = result.replace(/#([0-9a-fA-F]{2})/g, function(_, hex) {
        return String.fromCharCode(parseInt(hex, 16));
      });
      if (decoded === result) break;
      result = decoded;
    }
    return result.replace(/_/g, ' ').trim();
  }

  function displayPath(path) {
    const normalized = normalizePath(path);
    return normalized === '' ? 'Aufnahmen' : normalized.split('/').map(decodeVdrText).join(' / ');
  }

  function parentPath(path) {
    const parts = normalizePath(path).split('/').filter(Boolean);
    parts.pop();
    return parts.join('/');
  }

  function ensureMountTarget() {
    if (!mountTarget || typeof mountTarget.replaceChildren !== 'function') {
      throw new Error('Recording browser mount target is not configured');
    }
    return mountTarget;
  }

  function configureMountTarget(element) {
    if (!element || typeof element.replaceChildren !== 'function') {
      throw new Error('Recording browser mount target is invalid');
    }
    mountTarget = element;
  }

  function configureFolderLoader(loader) {
    folderLoader = typeof loader === 'function' ? loader : null;
  }

  function configureActionRunner(runner) {
    actionRunner = typeof runner === 'function' ? runner : null;
  }

  function renderMessage(title, message, busy) {
    const holder = document.createElement('section');
    holder.className = 'list recording-folder-list';
    const box = document.createElement('article');
    box.className = 'module-placeholder';
    box.appendChild(textElement('h3', title));
    box.appendChild(textElement('p', message));
    if (busy) {
      const progress = document.createElement('progress');
      progress.className = 'recording-folder-refresh-progress';
      progress.setAttribute('aria-label', title);
      box.appendChild(progress);
    }
    holder.appendChild(box);
    ensureMountTarget().replaceChildren(holder);
  }

  function folderName(folder) {
    const explicit = firstValue(folder, ['name', 'title', 'displayName', 'label'], '');
    if (explicit !== '') return decodeVdrText(explicit);
    const path = normalizePath(firstValue(folder, ['path', 'folderPath'], ''));
    const parts = path.split('/').filter(Boolean);
    return decodeVdrText(parts[parts.length - 1] || 'Ordner');
  }

  function folderPath(folder, currentPath) {
    const explicit = normalizePath(firstValue(folder, ['path', 'folderPath'], ''));
    if (explicit !== '') return explicit;
    const name = String(firstValue(folder, ['name', 'title', 'displayName'], '')).trim();
    return normalizePath([normalizePath(currentPath), name].filter(Boolean).join('/'));
  }

  function recordingTitle(recording, index) {
    const value = firstValue(recording, ['title', 'name', 'displayName', 'file'], 'Aufnahme ' + String(index + 1));
    const parts = String(value).split('/').filter(Boolean);
    return decodeVdrText(parts[parts.length - 1] || value);
  }

  function recordingMeta(recording) {
    const values = [];
    const start = firstValue(recording, ['startTime', 'start', 'date'], '');
    const duration = Number(firstValue(recording, ['durationSeconds', 'duration'], 0));
    const sizeMb = Number(firstValue(recording, ['sizeMb', 'sizeMB'], 0));
    if (start !== '') {
      const epoch = Number(start);
      values.push(Number.isFinite(epoch) && epoch > 1000000000 ? new Date(epoch * 1000).toLocaleString('de-DE') : String(start));
    }
    if (Number.isFinite(duration) && duration > 0) values.push(String(Math.round(duration / 60)) + ' min');
    if (Number.isFinite(sizeMb) && sizeMb > 0) values.push(sizeMb >= 1024 ? (sizeMb / 1024).toFixed(1) + ' GB' : Math.round(sizeMb) + ' MB');
    return values.join(' · ');
  }

  function makeInteractiveRow(title, meta, onOpen, extraClass) {
    const row = document.createElement('article');
    row.className = 'list-item' + (extraClass ? ' ' + extraClass : '');
    row.tabIndex = 0;
    row.setAttribute('role', 'button');
    row.appendChild(textElement('div', title, 'list-title'));
    if (meta) row.appendChild(textElement('div', meta, 'list-meta'));
    row.addEventListener('click', onOpen);
    row.addEventListener('keydown', function(event) {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault();
        onOpen();
      }
    });
    return row;
  }

  function renderRecordingDetail(recording, folderData, index) {
    const container = document.createElement('section');
    container.className = 'list';
    const backRow = document.createElement('article');
    backRow.className = 'module-placeholder';
    const back = document.createElement('button');
    back.type = 'button';
    back.textContent = 'Zurück zu ' + displayPath(folderData.path || '');
    back.addEventListener('click', function() { renderFolder(folderData); });
    backRow.appendChild(back);
    container.appendChild(backRow);

    const detail = document.createElement('article');
    detail.className = 'module-placeholder recording-detail';
    detail.appendChild(textElement('h3', recordingTitle(recording, index)));
    const subtitle = firstValue(recording, ['shortText', 'subtitle'], '');
    if (subtitle !== '') detail.appendChild(textElement('p', decodeVdrText(subtitle)));
    const meta = recordingMeta(recording);
    if (meta !== '') detail.appendChild(textElement('p', meta, 'list-meta'));
    const description = firstValue(recording, ['description', 'longText', 'summary'], '');
    if (description !== '') detail.appendChild(textElement('p', decodeVdrText(description)));
    container.appendChild(detail);
    ensureMountTarget().replaceChildren(container);
  }

  function renderFolder(data) {
    const folderData = data && typeof data === 'object' ? data : {};
    const path = normalizePath(folderData.path || '');
    const folders = listFromResponse(folderData, 'folders');
    const recordings = listFromResponse(folderData, 'recordings');
    currentFolder = folderData;

    const container = document.createElement('section');
    container.className = 'list recording-folder-list';
    const header = document.createElement('article');
    header.className = 'module-placeholder';
    header.appendChild(textElement('h3', displayPath(path)));
    header.appendChild(textElement('p', String(folders.length) + ' Ordner · ' + String(recordings.length) + ' Aufnahmen'));
    container.appendChild(header);

    if (path !== '') {
      const backPath = normalizePath(folderData.parentPath || parentPath(path));
      container.appendChild(makeInteractiveRow('Zurück', displayPath(backPath), function() { loadFolder(backPath, 0); }));
    }

    folders.slice().sort(function(left, right) {
      return folderName(left).localeCompare(folderName(right), 'de-DE');
    }).forEach(function(folder) {
      const targetPath = folderPath(folder, path);
      const count = Number(firstValue(folder, ['recordingCount', 'count'], 0));
      container.appendChild(makeInteractiveRow(
        folderName(folder),
        count > 0 ? String(count) + ' Aufnahmen' : 'Ordner öffnen',
        function() { loadFolder(targetPath, 0); },
        'recording-folder-item'
      ));
    });

    recordings.forEach(function(recording, index) {
      container.appendChild(makeInteractiveRow(
        recordingTitle(recording, index),
        recordingMeta(recording),
        function() { renderRecordingDetail(recording, folderData, index); },
        'recording-list-item'
      ));
    });

    if (folders.length === 0 && recordings.length === 0) {
      const empty = document.createElement('article');
      empty.className = 'module-placeholder';
      empty.appendChild(textElement('p', 'Dieser Aufnahmeordner ist leer.'));
      container.appendChild(empty);
    }
    ensureMountTarget().replaceChildren(container);
  }

  function loadFolder(path, offset) {
    const normalizedPath = normalizePath(path);
    const sequence = ++requestSequence;
    if (!folderLoader) {
      renderMessage('Aufnahmeordner konnte nicht geladen werden', 'Der Ordner-Lader ist nicht konfiguriert.', false);
      return Promise.resolve(null);
    }
    renderMessage('Lade Aufnahmeordner …', displayPath(normalizedPath), true);
    return Promise.resolve().then(function() {
      return folderLoader(normalizedPath, Number(offset) || 0);
    }).then(function(data) {
      if (sequence === requestSequence) renderFolder(data);
      return data;
    }).catch(function(error) {
      if (sequence === requestSequence) {
        renderMessage('Aufnahmeordner konnte nicht geladen werden', error && error.message ? error.message : String(error), false);
      }
      return null;
    });
  }

  function renderList(data) {
    requestSequence += 1;
    renderFolder(data);
  }

  const api = Object.freeze({
    configureMountTarget: configureMountTarget,
    configureFolderLoader: configureFolderLoader,
    configureActionRunner: configureActionRunner,
    renderList: renderList,
    loadFolder: loadFolder,
    getCurrentFolder: function() { return currentFolder; },
    hasActionRunner: function() { return actionRunner !== null; }
  });

  global.VdrSuiteRecordingBrowser = api;
  if (global.VdrSuitePlatform && typeof global.VdrSuitePlatform.registerModule === 'function' && !global.VdrSuitePlatform.hasModule('recordings')) {
    global.VdrSuitePlatform.registerModule('recordings', api);
  }
})(window);
