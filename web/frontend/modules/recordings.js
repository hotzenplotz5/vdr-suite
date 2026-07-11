"use strict";

// Phase 59.10q: Recording browser uses an explicit mount target boundary.
// HTTP ownership remains outside this file.

let recordingBrowserMountTarget = null;

function configureRecordingBrowserMountTarget(element) {
  if (!element || typeof element.replaceChildren !== 'function' || typeof element.appendChild !== 'function') {
    throw new Error('Recording browser mount target is invalid');
  }

  recordingBrowserMountTarget = element;
}

function configureRecordingBrowserContext(context) {
  const value = context && typeof context === 'object' ? context : {};
  configureRecordingBrowserMountTarget(value.detailDataElement);
}

function recordingBrowserDetailDataElement() {
  if (!recordingBrowserMountTarget) {
    throw new Error('Recording browser mount target is not configured');
  }

  return recordingBrowserMountTarget;
}

function recordingBrowserAddText(element, text) {
  element.textContent = text;
  return element;
}

function recordingBrowserFirstValue(object, keys, fallback) {
  for (const key of keys) {
    if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
      return object[key];
    }
  }

  return fallback;
}

function recordingBrowserListFromResponse(data, key) {
  if (Array.isArray(data)) {
    return data;
  }

  if (data && Array.isArray(data[key])) {
    return data[key];
  }

  if (data && Array.isArray(data.items)) {
    return data.items;
  }

  return [];
}

function recordingBrowserFormatDurationSeconds(value) {
  const seconds = Number(value);

  if (!Number.isFinite(seconds) || seconds <= 0) {
    return 'Unbekannt';
  }

  const minutes = Math.round(seconds / 60);

  if (minutes < 60) {
    return String(minutes) + ' min';
  }

  const hours = Math.floor(minutes / 60);
  const remainingMinutes = minutes % 60;

  if (remainingMinutes === 0) {
    return String(hours) + ' h';
  }

  return String(hours) + ' h ' + String(remainingMinutes) + ' min';
}

function recordingBrowserFormatSizeMb(value) {
  const sizeMb = Number(value);

  if (!Number.isFinite(sizeMb) || sizeMb <= 0) {
    return '-';
  }

  if (sizeMb >= 1024) {
    return (sizeMb / 1024).toFixed(1) + ' GB';
  }

  return String(Math.round(sizeMb)) + ' MB';
}

function recordingBrowserFormatRecordingStart(value) {
  if (value === undefined || value === null || value === '' || String(value) === '-1') {
    return 'Unbekannt';
  }

  const number = Number(value);

  if (Number.isFinite(number) && number <= 0) {
    return 'Unbekannt';
  }

  if (Number.isFinite(number) && number > 1000000000) {
    return new Date(number * 1000).toLocaleString('de-DE', {
      year: 'numeric',
      month: '2-digit',
      day: '2-digit',
      hour: '2-digit',
      minute: '2-digit'
    });
  }

  return String(value);
}

let recordingSortMode = 'name';
let recordingViewMode = 'folder';
let currentRecordingRecords = [];

function decodeRecordingText(value) {
  return String(value || '')
    .replace(/#([0-9A-Fa-f]{2})/g, (_, hex) => String.fromCharCode(parseInt(hex, 16)))
    .replace(/_/g, ' ')
    .trim()
    .replace(/^%+/, '')
    .trim();
}

function recordingSortKey(value) {
  return decodeRecordingText(value).toLocaleLowerCase('de-DE');
}

function recordingTimestamp(entry) {
  const recording = entry.recording || entry;
  const value = recordingBrowserFirstValue(recording, ['startTime', 'start', 'date'], '');
  const number = Number(value);

  if (Number.isFinite(number) && number > 0) {
    return number;
  }

  return 0;
}

function recordingBrowserNormalizePathText(value) {
  return String(value || '')
    .replace(/^\/srv\/vdr\/video\//, '/')
    .replace(/\/+/g, '/')
    .replace(/^\//, '');
}

function recordingBrowserDisplayParts(recording, index) {
  const rawTitle = String(recordingBrowserFirstValue(
    recording,
    ['title', 'name', 'file', 'displayName'],
    'Aufnahme ' + String(index + 1)
  ));

  const titleParts = rawTitle.split('/').filter(part => part !== '');

  if (titleParts.length > 1) {
    return {
      folder: decodeRecordingText(titleParts.slice(0, -1).join('/')),
      title: decodeRecordingText(titleParts[titleParts.length - 1])
    };
  }

  const path = recordingBrowserNormalizePathText(recordingBrowserFirstValue(
    recording,
    ['path', 'fileName', 'directory'],
    ''
  ));
  const pathParts = path.split('/').filter(part => part !== '');

  if (pathParts.length > 2) {
    return {
      folder: decodeRecordingText(pathParts.slice(0, -2).join('/')),
      title: decodeRecordingText(rawTitle)
    };
  }

  if (pathParts.length > 1) {
    return {
      folder: decodeRecordingText(pathParts.slice(0, -1).join('/')),
      title: decodeRecordingText(rawTitle)
    };
  }

  return {
    folder: 'Ohne Ordner',
    title: decodeRecordingText(rawTitle)
  };
}

function createRecordingNode(name, parent) {
  return { name, parent, folders: new Map(), items: [] };
}

function countRecordingNode(node) {
  let count = node.items.length;
  node.folders.forEach(child => { count += countRecordingNode(child); });
  return count;
}

function pathForRecordingNode(node) {
  const parts = [];
  let cursor = node;
  while (cursor && cursor.parent) {
    parts.unshift(cursor.name);
    cursor = cursor.parent;
  }
  return parts;
}

function buildRecordingTree(recordings) {
  const root = createRecordingNode('Aufnahme-Ordner', null);

  recordings.forEach((recording, index) => {
    const display = recordingBrowserDisplayParts(recording, index);
    const folders = display.folder.split('/').filter(Boolean);
    let node = root;

    folders.forEach(folder => {
      if (!node.folders.has(folder)) {
        node.folders.set(folder, createRecordingNode(folder, node));
      }
      node = node.folders.get(folder);
    });

    node.items.push({ recording, title: display.title, index });
  });

  return root;
}

function recordingGenreName(recording, index) {
  const display = recordingBrowserDisplayParts(recording, index);
  const parts = display.folder.split('/').filter(Boolean);
  return parts.length > 0 ? parts[0] : 'Ohne Genre';
}

function buildRecordingGenreTree(recordings) {
  const root = createRecordingNode('Aufnahme-Genres', null);

  recordings.forEach((recording, index) => {
    const display = recordingBrowserDisplayParts(recording, index);
    const genre = recordingGenreName(recording, index);

    if (!root.folders.has(genre)) {
      root.folders.set(genre, createRecordingNode(genre, root));
    }

    root.folders.get(genre).items.push({ recording, title: display.title, index });
  });

  return root;
}

function recordingRootNode() {
  if (recordingViewMode === 'genre') {
    return buildRecordingGenreTree(currentRecordingRecords);
  }

  return buildRecordingTree(currentRecordingRecords);
}

function renderRecordingRoot() {
  renderRecordingNode(recordingRootNode());
}

function sortedRecordingFolders(node) {
  return Array.from(node.folders.values()).sort((left, right) =>
    recordingSortKey(left.name).localeCompare(recordingSortKey(right.name), 'de-DE'));
}

function sortedRecordingItems(node) {
  const items = node.items.slice();

  if (recordingSortMode === 'date') {
    return items.sort((left, right) => {
      const dateDiff = recordingTimestamp(right) - recordingTimestamp(left);
      if (dateDiff !== 0) {
        return dateDiff;
      }
      return recordingSortKey(left.title).localeCompare(recordingSortKey(right.title), 'de-DE');
    });
  }

  return items.sort((left, right) =>
    recordingSortKey(left.title).localeCompare(recordingSortKey(right.title), 'de-DE'));
}

function renderRecordingNode(node) {
  const container = document.createElement('section');
  container.className = 'list';

  const header = document.createElement('article');
  header.className = 'module-placeholder';
  header.appendChild(recordingBrowserAddText(document.createElement('h3'), node.name));
  header.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    countRecordingNode(node) + ' Aufnahme(n) in diesem Bereich.'
  ));
  container.appendChild(header);

  if (node.parent) {
    const back = document.createElement('button');
    back.type = 'button';
    back.textContent = 'Zurück zu ' + node.parent.name;
    back.addEventListener('click', () => renderRecordingNode(node.parent));
    const row = document.createElement('article');
    row.className = 'module-placeholder';
    row.appendChild(back);
    container.appendChild(row);
  }

  sortedRecordingFolders(node).forEach(child => {
    const item = document.createElement('article');
    item.className = 'list-item';
    item.tabIndex = 0;
    item.setAttribute('role', 'button');
    item.appendChild(recordingBrowserAddText(document.createElement('div'), child.name)).className = 'list-title';
    item.appendChild(recordingBrowserAddText(document.createElement('div'), countRecordingNode(child) + ' Aufnahme(n)')).className = 'list-meta';
    const open = () => renderRecordingNode(child);
    item.addEventListener('click', open);
    item.addEventListener('keydown', event => {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault();
        open();
      }
    });
    container.appendChild(item);
  });

  sortedRecordingItems(node).forEach(entry => {
    const item = document.createElement('article');
    item.className = 'list-item';
    item.appendChild(recordingBrowserAddText(document.createElement('div'), entry.title)).className = 'list-title';
    item.appendChild(recordingBrowserAddText(document.createElement('div'), pathForRecordingNode(node).join(' / ') || 'Hauptordner')).className = 'list-meta';
    container.appendChild(item);
  });

  recordingBrowserDetailDataElement().replaceChildren(container);
}


let recordingBrowserFolderLoader = null;
let recordingBrowserActionRunner = null;
let recordingBrowserFolderRefreshTimer = null;

function configureRecordingBrowserFolderLoader(loader) {
  recordingBrowserFolderLoader = typeof loader === 'function' ? loader : null;
}

function configureRecordingBrowserActionRunner(runner) {
  recordingBrowserActionRunner = typeof runner === 'function' ? runner : null;
}

function recordingBrowserCancelFolderRefreshTimer() {
  if (recordingBrowserFolderRefreshTimer !== null) {
    window.clearTimeout(recordingBrowserFolderRefreshTimer);
    recordingBrowserFolderRefreshTimer = null;
  }
}

function recordingBrowserServerFolderLabel(path) {
  const value = String(path || '').trim();

  if (value === '') {
    return 'Aufnahme-Ordner';
  }

  return value
    .split('/')
    .filter(part => part !== '')
    .map(part => decodeRecordingText(part))
    .join(' / ');
}

function recordingBrowserDisplayPathLabel(value) {
  const raw = String(value || '').trim();

  if (raw === '') {
    return '';
  }

  return raw
    .split('/')
    .filter(part => part !== '')
    .map(part => decodeRecordingText(part))
    .join(' / ');
}

function recordingBrowserFolderBreadcrumbLabel(path) {
  const folderLabel = recordingBrowserServerFolderLabel(path);

  if (folderLabel === 'Aufnahme-Ordner') {
    return 'Pfad: Aufnahme-Ordner';
  }

  return 'Pfad: Aufnahme-Ordner / ' + folderLabel;
}

function recordingBrowserLastDisplaySegment(label) {
  const parts = String(label || '')
    .split('/')
    .map(part => part.trim())
    .filter(part => part !== '');

  if (parts.length === 0) {
    return String(label || '').trim();
  }

  return parts[parts.length - 1];
}

function recordingBrowserLocalRecordingTitle(recording, folderData) {
  const rawTitle = recordingBrowserFirstValue(
    recording,
    ['title', 'name', 'file', 'displayName'],
    'Aufnahme'
  );
  const titleLabel = recordingBrowserDisplayPathLabel(rawTitle);
  const folderPath = folderData && typeof folderData === 'object'
    ? String(folderData.path || '')
    : '';
  const folderLabel = recordingBrowserDisplayPathLabel(folderPath);

  if (folderLabel !== '' && titleLabel === folderLabel) {
    return recordingBrowserLastDisplaySegment(titleLabel);
  }

  if (folderLabel !== '' && titleLabel.startsWith(folderLabel + ' / ')) {
    return titleLabel.slice(folderLabel.length + 3).trim();
  }

  if (titleLabel.includes(' / ')) {
    return recordingBrowserLastDisplaySegment(titleLabel.replace(/ \/ /g, '/'));
  }

  return titleLabel || 'Aufnahme';
}

function recordingBrowserLoadServerFolder(path, offset) {
  if (!recordingBrowserFolderLoader) {
    return;
  }

  recordingBrowserCancelFolderRefreshTimer();

  const holder = document.createElement('section');
  holder.className = 'list recording-folder-list';

  const loading = document.createElement('article');
  loading.className = 'module-placeholder';
  loading.appendChild(recordingBrowserAddText(
    document.createElement('h3'),
    'Lade Aufnahmeordner …'
  ));
  loading.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    recordingBrowserServerFolderLabel(path)
  ));
  holder.appendChild(loading);
  recordingBrowserDetailDataElement().replaceChildren(holder);

  recordingBrowserFolderLoader(path || '', Number(offset) || 0)
    .then(renderServerRecordingFolder)
    .catch(error => {
      const errorBox = document.createElement('article');
      errorBox.className = 'module-placeholder';
      errorBox.appendChild(recordingBrowserAddText(
        document.createElement('h3'),
        'Aufnahmeordner konnte nicht geladen werden'
      ));
      errorBox.appendChild(recordingBrowserAddText(
        document.createElement('p'),
        error && error.message ? error.message : String(error)
      ));
      recordingBrowserDetailDataElement().replaceChildren(errorBox);
    });
}


function recordingBrowserActionList(value) {
  if (!Array.isArray(value)) {
    return [];
  }

  return value
    .map(entry => String(entry || '').trim())
    .filter(entry => entry !== '');
}

function recordingBrowserActionPayload(recording, action) {
  const recordingId = recordingBrowserFirstValue(recording, ['recordingId', 'id', 'nativeId'], '');
  const recordingPath = recordingBrowserFirstValue(recording, ['path', 'fileName', 'directory'], '');

  const payload = {
    recordingId: String(recordingId),
    action: String(action),
    dryRun: true
  };

  if (String(recordingPath || '').trim() !== '') {
    payload.recordingPath = String(recordingPath);
  }

  return payload;
}

function recordingBrowserRenderActionResult(target, title, result, error) {
  target.replaceChildren();

  const heading = document.createElement('h4');
  heading.textContent = title;
  target.appendChild(heading);

  const lines = [];

  if (error) {
    lines.push('Fehler: ' + (error.message ? error.message : String(error)));
  } else if (result && typeof result === 'object') {
    if (Object.prototype.hasOwnProperty.call(result, 'valid')) {
      lines.push('Validierung: ' + (result.valid ? 'gültig' : 'ungültig'));
    }

    if (Object.prototype.hasOwnProperty.call(result, 'success')) {
      lines.push('Ausführung: ' + (result.success ? 'erfolgreich' : 'nicht erfolgreich'));
    }

    if (result.type) {
      lines.push('Typ: ' + String(result.type));
    }

    if (result.action) {
      lines.push('Aktion: ' + String(result.action));
    }

    if (result.message) {
      lines.push('Meldung: ' + String(result.message));
    }

    const permissions = recordingBrowserActionList(result.requiredPermissions);
    if (permissions.length > 0) {
      lines.push('Benötigte Rechte: ' + permissions.join(', '));
    }

    const capabilities = recordingBrowserActionList(result.requiredCapabilities);
    if (capabilities.length > 0) {
      lines.push('Benötigte Fähigkeiten: ' + capabilities.join(', '));
    }

    const warnings = recordingBrowserActionList(result.warnings);
    if (warnings.length > 0) {
      lines.push('Warnungen: ' + warnings.join(' · '));
    }

    const errors = recordingBrowserActionList(result.errors);
    if (errors.length > 0) {
      lines.push('API-Fehler: ' + errors.join(' · '));
    }

    if (lines.length === 0) {
      lines.push('Antwort erhalten.');
    }
  } else {
    lines.push('Antwort erhalten.');
  }

  lines.forEach(line => {
    target.appendChild(recordingBrowserAddText(document.createElement('p'), line));
  });
}

function recordingBrowserCreateActionButton(label, mode, action, recording, resultBox) {
  const button = document.createElement('button');
  button.type = 'button';
  button.textContent = label;

  button.addEventListener('click', () => {
    if (!recordingBrowserActionRunner) {
      recordingBrowserRenderActionResult(
        resultBox,
        label,
        null,
        new Error('Recording action runner is not configured')
      );
      return;
    }

    button.disabled = true;
    recordingBrowserRenderActionResult(
      resultBox,
      label,
      { message: 'Aktion läuft …' },
      null
    );

    recordingBrowserActionRunner({
      mode: mode,
      action: action,
      payload: recordingBrowserActionPayload(recording, action),
      recording: recording
    })
      .then(result => {
        recordingBrowserRenderActionResult(resultBox, label, result, null);
      })
      .catch(error => {
        recordingBrowserRenderActionResult(resultBox, label, null, error);
      })
      .finally(() => {
        button.disabled = false;
      });
  });

  return button;
}

function createServerRecordingActionPanel(recording) {
  const panel = document.createElement('div');
  panel.className = 'recording-action-panel';

  panel.appendChild(recordingBrowserAddText(
    document.createElement('h3'),
    'Aktionen'
  ));

  panel.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    'Sicherer Action-Test: Validierung und Dry-Run. Keine echte Löschung, kein echtes Verschieben, keine echte Mutation.'
  ));

  const actions = document.createElement('div');
  actions.className = 'recording-action-buttons';

  const resultBox = document.createElement('div');
  resultBox.className = 'recording-action-result';

  actions.appendChild(recordingBrowserCreateActionButton(
    'Löschen prüfen',
    'validate',
    'DELETE',
    recording,
    resultBox
  ));

  actions.appendChild(recordingBrowserCreateActionButton(
    'Löschen Dry-Run',
    'execute',
    'DELETE',
    recording,
    resultBox
  ));

  panel.appendChild(actions);
  panel.appendChild(resultBox);

  return panel;
}

function renderServerRecordingDetail(recording, folderData) {
  recordingBrowserCancelFolderRefreshTimer();

  const list = document.createElement('section');
  list.className = 'list recording-detail-list';

  const item = document.createElement('article');
  item.className = 'module-placeholder recording-detail';

  const title = recordingBrowserLocalRecordingTitle(recording, folderData);
  const folderLabel = recordingBrowserDisplayPathLabel(
    folderData && typeof folderData === 'object'
      ? String(folderData.path || '')
      : ''
  );
  const recordingId = recordingBrowserFirstValue(recording, ['recordingId', 'id', 'nativeId'], '-');
  const path = recordingBrowserFirstValue(recording, ['path', 'fileName', 'directory'], '-');
  const startTime = recordingBrowserFormatRecordingStart(recordingBrowserFirstValue(recording, ['startTime', 'start', 'date'], '-'));
  const duration = recordingBrowserFormatDurationSeconds(recordingBrowserFirstValue(recording, ['durationSeconds', 'duration'], 0));
  const size = recordingBrowserFormatSizeMb(recordingBrowserFirstValue(recording, ['sizeMb', 'sizeMB', 'size'], 0));

  item.appendChild(recordingBrowserAddText(document.createElement('h3'), String(title)));

  if (folderLabel !== '') {
    item.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      'Ordner: ' + folderLabel
    ));
  }

  item.appendChild(recordingBrowserAddText(document.createElement('p'), 'Aufnahme: ' + startTime));
  item.appendChild(recordingBrowserAddText(document.createElement('p'), 'Dauer: ' + duration));
  item.appendChild(recordingBrowserAddText(document.createElement('p'), 'Größe: ' + size));

  const technicalDetails = document.createElement('details');
  technicalDetails.className = 'recording-technical-details';

  technicalDetails.appendChild(recordingBrowserAddText(
    document.createElement('summary'),
    'Technische Details anzeigen'
  ));
  technicalDetails.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    'Pfad: ' + String(path)
  ));
  technicalDetails.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    'ID: ' + String(recordingId)
  ));

  item.appendChild(technicalDetails);
  item.appendChild(createServerRecordingActionPanel(recording));

  const backButton = document.createElement('button');
  backButton.type = 'button';
  backButton.textContent = 'Zurück zum Ordner';
  backButton.addEventListener('click', () => renderServerRecordingFolder(folderData));
  item.appendChild(backButton);

  list.appendChild(item);
  recordingBrowserDetailDataElement().replaceChildren(list);
}

function createServerRecordingItem(recording, folderData) {
  const item = document.createElement('article');
  item.className = 'list-item recording-list-item';
  item.tabIndex = 0;
  item.setAttribute('role', 'button');

  const title = recordingBrowserLocalRecordingTitle(recording, folderData);
  const startTime = recordingBrowserFormatRecordingStart(recordingBrowserFirstValue(recording, ['startTime', 'start', 'date'], '-'));
  const duration = recordingBrowserFormatDurationSeconds(recordingBrowserFirstValue(recording, ['durationSeconds', 'duration'], 0));
  const size = recordingBrowserFormatSizeMb(recordingBrowserFirstValue(recording, ['sizeMb', 'sizeMB', 'size'], 0));

  item.appendChild(recordingBrowserAddText(document.createElement('div'), String(title))).className = 'list-title';
  item.appendChild(recordingBrowserAddText(
    document.createElement('div'),
    'Aufnahme: ' + startTime + ' · Dauer: ' + duration + ' · Größe: ' + size + ' · antippen für Details'
  )).className = 'list-meta';

  const open = () => renderServerRecordingDetail(recording, folderData);
  item.addEventListener('click', open);
  item.addEventListener('keydown', event => {
    if (event.key === 'Enter' || event.key === ' ') {
      event.preventDefault();
      open();
    }
  });

  return item;
}

function renderServerRecordingFolder(data) {
  recordingBrowserCancelFolderRefreshTimer();

  const folderData = data && typeof data === 'object' ? data : {};
  const folders = recordingBrowserListFromResponse(folderData, 'folders');
  const recordings = recordingBrowserListFromResponse(folderData, 'recordings');
  const path = String(folderData.path || '');
  const parentPath = String(folderData.parentPath || '');
  const cacheState = String(folderData.cacheState || folderData.state || 'empty');
  const cacheReady = Boolean(folderData.cacheReady);
  const offset = Number(folderData.offset) || 0;
  const limit = Number(folderData.limit) || 50;
  const recordingCount = Number(folderData.recordingCount) || recordings.length;
  const returnedCount = Number(folderData.returnedCount) || recordings.length;
  const totalCount = Number(folderData.totalCount) || 0;

  const list = document.createElement('section');
  list.className = 'list recording-folder-list';

  const header = document.createElement('article');
  header.className = 'module-placeholder';
  header.appendChild(recordingBrowserAddText(
    document.createElement('h3'),
    recordingBrowserServerFolderLabel(path)
  ));
  header.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    recordingBrowserFolderBreadcrumbLabel(path)
  ));

  const cacheLabel = cacheReady
    ? 'Cache bereit'
    : 'Cache wird aktualisiert: ' + cacheState;

  const summary = [
    'Unterordner: ' + String(folders.length),
    'Direkte Aufnahmen: ' + String(recordingCount),
    'Angezeigt: ' + String(returnedCount),
    'Gesamt im Cache: ' + String(totalCount),
    cacheLabel
  ];

  header.appendChild(recordingBrowserAddText(document.createElement('p'), summary.join(' · ')));

  if (!cacheReady) {
    header.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      'Recording-Cache wird vom Daemon im Hintergrund gefüllt. Diese Ansicht aktualisiert sich automatisch.'
    ));
  }

  if (path !== '') {
    const backButton = document.createElement('button');
    backButton.type = 'button';
    backButton.textContent = parentPath === '' ? 'Zurück zum Hauptordner' : 'Eine Ebene zurück';
    backButton.addEventListener('click', () => recordingBrowserLoadServerFolder(parentPath, 0));
    header.appendChild(backButton);
  }

  list.appendChild(header);

  folders.forEach(folder => {
    const item = document.createElement('article');
    item.className = 'list-item recording-folder-item';
    item.tabIndex = 0;
    item.setAttribute('role', 'button');

    const rawFolderName = String(recordingBrowserFirstValue(folder, ['name', 'title'], 'Ordner'));
    const folderName = decodeRecordingText(rawFolderName);
    const folderPath = String(recordingBrowserFirstValue(folder, ['path'], rawFolderName));
    const folderCount = Number(recordingBrowserFirstValue(folder, ['recordingCount', 'count', 'total'], 0));

    item.appendChild(recordingBrowserAddText(document.createElement('div'), folderName)).className = 'list-title';
    item.appendChild(recordingBrowserAddText(
      document.createElement('div'),
      String(folderCount) + ' Aufnahme(n) · antippen zum Öffnen'
    )).className = 'list-meta';

    const open = () => recordingBrowserLoadServerFolder(folderPath, 0);
    item.addEventListener('click', open);
    item.addEventListener('keydown', event => {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault();
        open();
      }
    });

    list.appendChild(item);
  });

  recordings.forEach(recording => {
    list.appendChild(createServerRecordingItem(recording, folderData));
  });

  if (recordingCount > limit || offset > 0) {
    const pager = document.createElement('article');
    pager.className = 'module-placeholder recording-pager';

    const actions = document.createElement('div');
    actions.className = 'recording-pager-actions';

    const previous = document.createElement('button');
    previous.type = 'button';
    previous.textContent = 'Vorherige ' + String(limit);
    previous.disabled = offset <= 0;
    if (!previous.disabled) {
      previous.addEventListener('click', () => {
        recordingBrowserLoadServerFolder(path, Math.max(0, offset - limit));
      });
    }
    actions.appendChild(previous);

    const next = document.createElement('button');
    next.type = 'button';
    next.textContent = 'Nächste ' + String(limit);
    next.disabled = offset + limit >= recordingCount;
    if (!next.disabled) {
      next.addEventListener('click', () => {
        recordingBrowserLoadServerFolder(path, offset + limit);
      });
    }
    actions.appendChild(next);

    pager.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      'Direkte Aufnahmen ' + String(offset + 1) + '–' + String(Math.min(offset + recordings.length, recordingCount)) + ' von ' + String(recordingCount)
    ));
    pager.appendChild(actions);
    list.appendChild(pager);
  }

  if (folders.length > 0 && recordings.length === 0) {
    const folderOnly = document.createElement('article');
    folderOnly.className = 'module-placeholder';
    folderOnly.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      'Dieser Bereich enthält Unterordner, aber keine direkten Aufnahmen.'
    ));
    list.appendChild(folderOnly);
  }

  if (folders.length === 0 && recordings.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      cacheReady
        ? 'Dieser Ordner enthält keine Unterordner und keine direkten Aufnahmen.'
        : 'Noch keine Cache-Daten vorhanden. Der Daemon lädt die Aufnahmen im Hintergrund.'
    ));
    list.appendChild(empty);
  }

  recordingBrowserDetailDataElement().replaceChildren(list);

  if (!cacheReady && recordingBrowserFolderLoader) {
    recordingBrowserFolderRefreshTimer = window.setTimeout(() => {
      recordingBrowserLoadServerFolder(path, offset);
    }, 1500);
  }
}


const RECORDING_FOLDER_BATCH_SIZE = 80;
const RECORDING_ITEM_PAGE_SIZE = 20;

function renderRecordingList(data) {
  if (data && data.recordingFolder === true) {
    renderServerRecordingFolder(data);
    return;
  }

  const recordings = recordingBrowserListFromResponse(data, 'recordings');
  recordingBrowserDetailDataElement().replaceChildren();

  if (recordings.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(recordingBrowserAddText(document.createElement('h3'), 'Keine Aufnahmen gefunden'));
    empty.appendChild(recordingBrowserAddText(document.createElement('p'), 'Der Lazy Recording Folder Cache hat aktuell keine Aufnahmen geliefert.'));
    recordingBrowserDetailDataElement().appendChild(empty);
    return;
  }

  const reportedTotal = Number(recordingBrowserFirstValue(data || {}, ['totalCount', 'total', 'count'], recordings.length));
  const totalRecordings = Number.isFinite(reportedTotal) && reportedTotal > recordings.length
    ? reportedTotal
    : recordings.length;

  function createRecordingFolderNode(name, pathSegments, parent) {
    return {
      name,
      pathSegments,
      parent,
      folders: new Map(),
      recordings: [],
      totalRecordings: 0
    };
  }

  function recordingFolderSegments(folder) {
    const value = String(folder || '').trim();

    if (value === '' || value === 'Ohne Ordner') {
      return [];
    }

    return value.split('/').map(part => part.trim()).filter(part => part !== '');
  }

  function buildRecordingFolderTree(items) {
    const rootNode = createRecordingFolderNode('Aufnahme-Ordner', [], null);

    items.forEach((recording, index) => {
      const display = recordingBrowserDisplayParts(recording, index);
      const entry = {
        recording,
        title: display.title,
        index
      };

      let node = rootNode;
      node.totalRecordings += 1;

      recordingFolderSegments(display.folder).forEach(segment => {
        if (!node.folders.has(segment)) {
          node.folders.set(
            segment,
            createRecordingFolderNode(segment, node.pathSegments.concat(segment), node)
          );
        }

        node = node.folders.get(segment);
        node.totalRecordings += 1;
      });

      node.recordings.push(entry);
    });

    return rootNode;
  }

  function sortedRecordingFolderNodes(node) {
    return Array.from(node.folders.values())
      .sort((left, right) => String(left.name).localeCompare(String(right.name), 'de-DE'));
  }

  function recordingFolderLabel(node) {
    if (!node || node.pathSegments.length === 0) {
      return 'Aufnahme-Ordner';
    }

    return node.pathSegments.join(' / ');
  }

  function createLoadMoreButton(label, action) {
    const holder = document.createElement('article');
    holder.className = 'module-placeholder';

    const button = document.createElement('button');
    button.type = 'button';
    button.textContent = label;
    button.addEventListener('click', action);

    holder.appendChild(button);
    return holder;
  }

  function createRecordingNavigationButton(label, disabled, action) {
    const button = document.createElement('button');
    button.type = 'button';
    button.textContent = label;
    button.disabled = disabled;

    if (!disabled) {
      button.addEventListener('click', action);
    }

    return button;
  }

  function createRecordingPagerControls(node, visibleFolderCount, recordingPageIndex, pageCount) {
    const pager = document.createElement('article');
    pager.className = 'module-placeholder recording-pager';

    const currentPage = recordingPageIndex + 1;
    pager.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      'Seite ' + String(currentPage) + ' von ' + String(pageCount)
        + ' · ' + String(RECORDING_ITEM_PAGE_SIZE) + ' Aufnahmen pro Seite'
    ));

    const actions = document.createElement('div');
    actions.className = 'recording-pager-actions';

    actions.appendChild(createRecordingNavigationButton(
      'Vorherige 20',
      recordingPageIndex <= 0,
      () => renderFolderNode(node, visibleFolderCount, recordingPageIndex - 1)
    ));

    actions.appendChild(createRecordingNavigationButton(
      'Nächste 20',
      recordingPageIndex >= pageCount - 1,
      () => renderFolderNode(node, visibleFolderCount, recordingPageIndex + 1)
    ));

    pager.appendChild(actions);
    return pager;
  }

  function renderRecordingDetail(entry, node, visibleFolderCount, recordingPageIndex) {
    const recording = entry.recording;
    recordingBrowserDetailDataElement().replaceChildren();

    const list = document.createElement('section');
    list.className = 'list recording-detail-list';

    const item = document.createElement('article');
    item.className = 'module-placeholder recording-detail';

    const recordingId = recordingBrowserFirstValue(recording, ['recordingId', 'id', 'nativeId'], '-');
    const path = recordingBrowserFirstValue(recording, ['path', 'fileName', 'directory'], '-');
    const startTime = recordingBrowserFormatRecordingStart(recordingBrowserFirstValue(recording, ['startTime', 'start', 'date'], '-'));
    const duration = recordingBrowserFormatDurationSeconds(recordingBrowserFirstValue(recording, ['durationSeconds', 'duration'], 0));
    const size = recordingBrowserFormatSizeMb(recordingBrowserFirstValue(recording, ['sizeMb', 'sizeMB', 'size'], 0));
    const channel = recordingBrowserFirstValue(recording, ['channelName', 'channel', 'channelId'], '-');
    const description = recordingBrowserFirstValue(recording, ['description', 'summary', 'shortText'], '');

    item.appendChild(recordingBrowserAddText(document.createElement('h3'), entry.title));
    item.appendChild(recordingBrowserAddText(document.createElement('p'), 'Aufnahme: ' + startTime));
    item.appendChild(recordingBrowserAddText(document.createElement('p'), 'Dauer: ' + duration));
    item.appendChild(recordingBrowserAddText(document.createElement('p'), 'Größe: ' + size));
    item.appendChild(recordingBrowserAddText(document.createElement('p'), 'Sender: ' + String(channel)));
    item.appendChild(recordingBrowserAddText(document.createElement('p'), 'Pfad: ' + String(path)));
    item.appendChild(recordingBrowserAddText(document.createElement('p'), 'ID: ' + String(recordingId)));

    if (String(description).trim()) {
      item.appendChild(recordingBrowserAddText(document.createElement('p'), String(description)));
    }

    const backButton = document.createElement('button');
    backButton.type = 'button';
    backButton.textContent = 'Zurück zur Liste';
    backButton.addEventListener('click', () => {
      renderFolderNode(node, visibleFolderCount, recordingPageIndex);
    });
    item.appendChild(backButton);

    list.appendChild(item);
    recordingBrowserDetailDataElement().appendChild(list);
  }

  function createRecordingListItem(entry, openDetail) {
    const recording = entry.recording;
    const item = document.createElement('article');
    item.className = 'list-item recording-list-item';
    item.tabIndex = 0;
    item.setAttribute('role', 'button');
    item.setAttribute('aria-label', 'Aufnahme ' + entry.title + ' öffnen');

    const recordingId = recordingBrowserFirstValue(recording, ['recordingId', 'id', 'nativeId'], '-');
    const path = recordingBrowserFirstValue(recording, ['path', 'fileName', 'directory'], '-');
    const startTime = recordingBrowserFormatRecordingStart(recordingBrowserFirstValue(recording, ['startTime', 'start', 'date'], '-'));
    const duration = recordingBrowserFormatDurationSeconds(recordingBrowserFirstValue(recording, ['durationSeconds', 'duration'], 0));
    const size = recordingBrowserFormatSizeMb(recordingBrowserFirstValue(recording, ['sizeMb', 'sizeMB', 'size'], 0));

    item.appendChild(recordingBrowserAddText(document.createElement('div'), entry.title)).className = 'list-title';
    item.appendChild(recordingBrowserAddText(
      document.createElement('div'),
      'Aufnahme: ' + startTime + ' · Dauer: ' + duration + ' · Größe: ' + size + ' · antippen für Details'
    )).className = 'list-meta';
    item.appendChild(recordingBrowserAddText(document.createElement('div'), 'Pfad: ' + String(path))).className = 'list-meta';
    item.appendChild(recordingBrowserAddText(document.createElement('div'), 'ID: ' + String(recordingId))).className = 'list-meta';

    item.addEventListener('click', openDetail);
    item.addEventListener('keydown', event => {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault();
        openDetail();
      }
    });

    return item;
  }

  function renderFolderNode(node, visibleFolderCount, recordingPageIndex) {
    const childFolders = sortedRecordingFolderNodes(node);
    const leafRecordingFolders = childFolders.filter(folderNode =>
      folderNode.folders.size === 0 && folderNode.recordings.length === 1
    );
    const displayChildFolders = childFolders.filter(folderNode =>
      !(folderNode.folders.size === 0 && folderNode.recordings.length === 1)
    );
    const recordingEntries = node.recordings.concat(
      leafRecordingFolders.map(folderNode => folderNode.recordings[0])
    );

    visibleFolderCount = Math.min(
      Math.max(Number(visibleFolderCount) || RECORDING_FOLDER_BATCH_SIZE, RECORDING_FOLDER_BATCH_SIZE),
      displayChildFolders.length
    );

    const recordingPageCount = Math.max(1, Math.ceil(recordingEntries.length / RECORDING_ITEM_PAGE_SIZE));
    recordingPageIndex = Math.max(0, Math.min(Number(recordingPageIndex) || 0, recordingPageCount - 1));

    const recordingStartIndex = recordingPageIndex * RECORDING_ITEM_PAGE_SIZE;
    const recordingEndIndex = recordingStartIndex + RECORDING_ITEM_PAGE_SIZE;
    const visibleRecordings = recordingEntries.slice(recordingStartIndex, recordingEndIndex);

    recordingBrowserDetailDataElement().replaceChildren();

    const list = document.createElement('section');
    list.className = 'list recording-folder-list';

    const header = document.createElement('article');
    header.className = 'module-placeholder';
    header.appendChild(recordingBrowserAddText(document.createElement('h3'), recordingFolderLabel(node)));

    const summary = [
      String(displayChildFolders.length) + ' Unterordner',
      String(recordingEntries.length) + ' Aufnahme(n) in dieser Ebene',
      String(node.totalRecordings) + ' Aufnahme(n) insgesamt',
      String(totalRecordings) + ' Aufnahme(n) im Katalog'
    ];

    if (recordingEntries.length > 0) {
      summary.push('Seite ' + String(recordingPageIndex + 1) + ' von ' + String(recordingPageCount));
    }

    header.appendChild(recordingBrowserAddText(document.createElement('p'), summary.join(' · ')));

    if (node.parent) {
      const backButton = document.createElement('button');
      backButton.type = 'button';
      backButton.textContent = 'Eine Ebene zurück';
      backButton.addEventListener('click', () => {
        renderFolderNode(node.parent, RECORDING_FOLDER_BATCH_SIZE, 0);
      });
      header.appendChild(backButton);
    }

    list.appendChild(header);

    displayChildFolders.slice(0, visibleFolderCount).forEach(folderNode => {
      const item = document.createElement('article');
      item.className = 'list-item recording-folder-item';
      item.tabIndex = 0;
      item.setAttribute('role', 'button');
      item.setAttribute('aria-label', 'Ordner ' + recordingFolderLabel(folderNode) + ' öffnen');

      item.appendChild(recordingBrowserAddText(document.createElement('div'), folderNode.name)).className = 'list-title';
      item.appendChild(recordingBrowserAddText(
        document.createElement('div'),
        String(folderNode.folders.size) + ' Unterordner · '
          + String(folderNode.recordings.length) + ' direkte Aufnahme(n) · '
          + String(folderNode.totalRecordings) + ' gesamt · antippen zum Öffnen'
      )).className = 'list-meta';

      const openFolder = () => {
        renderFolderNode(folderNode, RECORDING_FOLDER_BATCH_SIZE, 0);
      };

      item.addEventListener('click', openFolder);
      item.addEventListener('keydown', event => {
        if (event.key === 'Enter' || event.key === ' ') {
          event.preventDefault();
          openFolder();
        }
      });

      list.appendChild(item);
    });

    if (visibleFolderCount < displayChildFolders.length) {
      const remaining = displayChildFolders.length - visibleFolderCount;
      list.appendChild(createLoadMoreButton(
        'Weitere Ordner laden (' + String(remaining) + ' verbleibend)',
        () => renderFolderNode(node, visibleFolderCount + RECORDING_FOLDER_BATCH_SIZE, recordingPageIndex)
      ));
    }

    if (recordingEntries.length > RECORDING_ITEM_PAGE_SIZE) {
      list.appendChild(createRecordingPagerControls(node, visibleFolderCount, recordingPageIndex, recordingPageCount));
    }

    visibleRecordings.forEach(entry => {
      list.appendChild(createRecordingListItem(entry, () => {
        renderRecordingDetail(entry, node, visibleFolderCount, recordingPageIndex);
      }));
    });

    if (recordingEntries.length > RECORDING_ITEM_PAGE_SIZE) {
      list.appendChild(createRecordingPagerControls(node, visibleFolderCount, recordingPageIndex, recordingPageCount));
    }

    if (displayChildFolders.length === 0 && recordingEntries.length === 0) {
      const empty = document.createElement('article');
      empty.className = 'module-placeholder';
      empty.appendChild(recordingBrowserAddText(document.createElement('p'), 'Dieser Ordner enthält keine Aufnahmen.'));
      list.appendChild(empty);
    }

    recordingBrowserDetailDataElement().appendChild(list);
  }

  const rootNode = buildRecordingFolderTree(recordings);
  renderFolderNode(rootNode, RECORDING_FOLDER_BATCH_SIZE, 0);
}

function setRecordingBrowserRecords(records) {
  currentRecordingRecords = Array.isArray(records) ? records.slice() : [];
}

const recordingBrowserApi = Object.freeze({
  configureContext: configureRecordingBrowserContext,
  configureMountTarget: configureRecordingBrowserMountTarget,
  configureFolderLoader: configureRecordingBrowserFolderLoader,
  configureActionRunner: configureRecordingBrowserActionRunner,
  decodeRecordingText: decodeRecordingText,
  setRecords: setRecordingBrowserRecords,
  renderList: renderRecordingList,
  renderRoot: renderRecordingRoot,
  renderNode: renderRecordingNode
});

window.VdrSuiteRecordingBrowser = recordingBrowserApi;

if (window.VdrSuitePlatform &&
    typeof window.VdrSuitePlatform.registerModule === 'function' &&
    !window.VdrSuitePlatform.hasModule('recordings')) {
  window.VdrSuitePlatform.registerModule('recordings', recordingBrowserApi);
}
