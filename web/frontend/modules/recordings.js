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

function recordingBrowserI18n() {
  if (window.VdrSuitePlatform &&
      typeof window.VdrSuitePlatform.getI18n === 'function') {
    const i18n = window.VdrSuitePlatform.getI18n();
    if (i18n) {
      return i18n;
    }
  }

  return window.VdrSuiteI18n || null;
}

function recordingBrowserTranslate(key, fallback, parameters) {
  const i18n = recordingBrowserI18n();

  if (!i18n || typeof i18n.t !== 'function') {
    return String(fallback || key || '');
  }

  return i18n.t(key, parameters || {}, fallback);
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
  let decoded = String(value || '');

  for (let pass = 0; pass < 8; pass += 1) {
    const next = decoded.replace(
      /#([0-9A-Fa-f]{2})/g,
      (_, hex) => String.fromCharCode(parseInt(hex, 16))
    );

    if (next === decoded) {
      break;
    }

    decoded = next;
  }

  return decoded
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
let recordingBrowserPendingRename = null;
let recordingBrowserPendingMove = null;
let recordingBrowserPendingDelete = null;
let recordingBrowserVisibleFolderPath = null;

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

function recordingBrowserNormalizeFolderPath(value) {
  return String(value || '')
    .trim()
    .split('/')
    .map(part => part.trim())
    .filter(part => part !== '')
    .join('/');
}

function recordingBrowserSetPendingRename(recording, folderData, newName, accepted) {
  const folderPath = folderData && typeof folderData === 'object'
    ? recordingBrowserNormalizeFolderPath(folderData.path || '')
    : '';

  recordingBrowserPendingRename = {
    folderPath: folderPath,
    newName: String(newName || '').trim(),
    accepted: accepted === true,
    recordingId: String(recordingBrowserFirstValue(
      recording,
      ['recordingId', 'id', 'nativeId'],
      ''
    ))
  };
}

function recordingBrowserConfirmPendingRename(folderData, newName) {
  const folderPath = folderData && typeof folderData === 'object'
    ? recordingBrowserNormalizeFolderPath(folderData.path || '')
    : '';
  const normalizedName = String(newName || '').trim();

  if (!recordingBrowserPendingRename ||
      recordingBrowserPendingRename.folderPath !== folderPath ||
      recordingBrowserPendingRename.newName !== normalizedName) {
    recordingBrowserPendingRename = {
      folderPath: folderPath,
      newName: normalizedName,
      accepted: true,
      recordingId: ''
    };
    return;
  }

  recordingBrowserPendingRename.accepted = true;
}

function recordingBrowserClearPendingRename() {
  recordingBrowserPendingRename = null;
  recordingBrowserCancelFolderRefreshTimer();
}

function recordingBrowserPendingRenameForFolder(path) {
  if (!recordingBrowserPendingRename) {
    return null;
  }

  return recordingBrowserPendingRename.folderPath ===
    recordingBrowserNormalizeFolderPath(path)
    ? recordingBrowserPendingRename
    : null;
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

function recordingBrowserParentDisplayFolder(label) {
  const parts = String(label || '')
    .split('/')
    .map(part => part.trim())
    .filter(part => part !== '');

  if (parts.length <= 1) {
    return '';
  }

  return parts.slice(0, -1).join(' / ');
}

function recordingBrowserRenderFolderLoading(path) {
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
}

function recordingBrowserRenderFolderLoadError(error) {
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
}

function recordingBrowserIsSingleRecordingLeaf(folderData) {
  const folders = recordingBrowserListFromResponse(folderData, 'folders');
  const recordings = recordingBrowserListFromResponse(folderData, 'recordings');
  const recordingCount = Number(folderData && folderData.recordingCount) || recordings.length;

  return folders.length === 0 &&
    recordings.length === 1 &&
    recordingCount === 1;
}

function recordingBrowserLoadServerFolder(path, offset) {
  if (!recordingBrowserFolderLoader) {
    return;
  }

  recordingBrowserCancelFolderRefreshTimer();
  recordingBrowserRenderFolderLoading(path);

  recordingBrowserFolderLoader(path || '', Number(offset) || 0)
    .then(renderServerRecordingFolder)
    .catch(recordingBrowserRenderFolderLoadError);
}

function recordingBrowserNormalizeRenameLabel(value) {
  const parts = decodeRecordingText(
    String(value || '').replace(/~/g, '/')
  )
    .split('/')
    .map(part => part.trim())
    .filter(part => part !== '');

  return parts.length > 0 ? parts[parts.length - 1] : '';
}

function recordingBrowserRenameLabelEquals(left, right) {
  return recordingBrowserNormalizeRenameLabel(left)
    .localeCompare(
      recordingBrowserNormalizeRenameLabel(right),
      'de-DE',
      { sensitivity: 'base' }
    ) === 0;
}

function recordingBrowserFolderContainsRenamedRecording(folderData, newName) {
  const expectedName = recordingBrowserNormalizeRenameLabel(newName);

  if (expectedName === '') {
    return false;
  }

  const folders = recordingBrowserListFromResponse(folderData, 'folders');
  const recordings = recordingBrowserListFromResponse(folderData, 'recordings');

  if (folders.some(folder => {
    const folderName = recordingBrowserFirstValue(folder, ['name', 'title'], '');
    const folderPath = recordingBrowserFirstValue(folder, ['path'], '');

    return recordingBrowserRenameLabelEquals(folderName, expectedName) ||
      recordingBrowserRenameLabelEquals(folderPath, expectedName);
  })) {
    return true;
  }

  return recordings.some(recording =>
    recordingBrowserRenameLabelEquals(
      recordingBrowserLocalRecordingTitle(recording, folderData),
      expectedName
    )
  );
}

function recordingBrowserOptimisticRenameTitle(recording, newName) {
  const rawTitle = String(recordingBrowserFirstValue(
    recording,
    ['title', 'name', 'displayName'],
    ''
  )).replace(/~/g, '/');
  const parts = rawTitle
    .split('/')
    .map(part => part.trim())
    .filter(part => part !== '');

  if (parts.length <= 1) {
    return String(newName || '').trim();
  }

  return parts.slice(0, -1).join('/') + '/' + String(newName || '').trim();
}

function recordingBrowserOptimisticRenamedRecording(recording, newName) {
  const optimisticRecording = Object.assign({}, recording || {});
  optimisticRecording.title = recordingBrowserOptimisticRenameTitle(
    recording,
    newName
  );
  return optimisticRecording;
}

function recordingBrowserRenderOptimisticRenameDetail(
  recording,
  folderData,
  newName,
  accepted
) {
  const optimisticRecording = recordingBrowserOptimisticRenamedRecording(
    recording,
    newName
  );

  recordingBrowserSetPendingRename(
    recording,
    folderData,
    newName,
    accepted
  );

  renderServerRecordingDetail(
    optimisticRecording,
    folderData,
    {
      renamePending: true,
      renameAccepted: accepted === true
    }
  );
}

function recordingBrowserScheduleRenameFolderReload(result, folderData, newName) {
  if (!result ||
      result.success !== true ||
      !recordingBrowserFolderLoader) {
    return false;
  }

  const folderPath = folderData && typeof folderData === 'object'
    ? String(folderData.path || '')
    : '';

  recordingBrowserConfirmPendingRename(folderData, newName);

  let attempts = 0;
  const maxAttempts = 30;

  const reloadFolder = () => {
    attempts += 1;

    recordingBrowserFolderLoader(folderPath, 0)
      .then(data => {
        const latestFolderData = data && typeof data === 'object' ? data : {};

        if (recordingBrowserFolderContainsRenamedRecording(
            latestFolderData,
            newName)) {
          recordingBrowserClearPendingRename();
          renderServerRecordingFolder(latestFolderData);
          return;
        }

        if (attempts >= maxAttempts) {
          recordingBrowserFolderRefreshTimer = null;
          return;
        }

        recordingBrowserFolderRefreshTimer =
          window.setTimeout(reloadFolder, 1000);
      })
      .catch(error => {
        (void error);

        if (attempts >= maxAttempts) {
          recordingBrowserFolderRefreshTimer = null;
          return;
        }

        recordingBrowserFolderRefreshTimer =
          window.setTimeout(reloadFolder, 1000);
      });
  };

  recordingBrowserCancelFolderRefreshTimer();
  reloadFolder();
  return true;
}

function recordingBrowserOpenServerFolder(path, parentFolderData) {
  if (!recordingBrowserFolderLoader) {
    return;
  }

  recordingBrowserCancelFolderRefreshTimer();
  recordingBrowserRenderFolderLoading(path);

  recordingBrowserFolderLoader(path || '', 0)
    .then(data => {
      const folderData = data && typeof data === 'object' ? data : {};

      if (recordingBrowserIsSingleRecordingLeaf(folderData)) {
        const recordings = recordingBrowserListFromResponse(folderData, 'recordings');
        renderServerRecordingDetail(recordings[0], parentFolderData || folderData);
        return;
      }

      renderServerRecordingFolder(folderData);
    })
    .catch(recordingBrowserRenderFolderLoadError);
}


function recordingBrowserNormalizedIdentityValue(value) {
  return String(value || '')
    .trim()
    .replace(/\\/g, '/')
    .replace(/\/+$/g, '');
}

function recordingBrowserRecordingIdentity(recording) {
  const value = recording && typeof recording === 'object' ? recording : {};
  return {
    recordingId: recordingBrowserNormalizedIdentityValue(
      recordingBrowserFirstValue(value, ['recordingId', 'id'], '')
    ),
    backendNativeId: recordingBrowserNormalizedIdentityValue(
      recordingBrowserFirstValue(value, ['backendNativeId', 'nativeId', 'nativePath'], '')
    ),
    path: recordingBrowserNormalizedIdentityValue(
      recordingBrowserFirstValue(value, ['path', 'fileName', 'directory'], '')
    ),
    title: recordingBrowserNormalizedIdentityValue(
      recordingBrowserFirstValue(value, ['title', 'name', 'displayName'], '')
    ),
    start: String(recordingBrowserFirstValue(value, ['startTime', 'start', 'date'], '')),
    duration: String(recordingBrowserFirstValue(value, ['durationSeconds', 'duration'], ''))
  };
}

function recordingBrowserSameRecording(left, right) {
  const leftIdentity = recordingBrowserRecordingIdentity(left);
  const rightIdentity = recordingBrowserRecordingIdentity(right);

  for (const key of ['recordingId', 'backendNativeId', 'path']) {
    if (leftIdentity[key] !== '' && rightIdentity[key] !== '') {
      return leftIdentity[key] === rightIdentity[key];
    }
  }

  return leftIdentity.title !== '' &&
    leftIdentity.title === rightIdentity.title &&
    leftIdentity.start !== '' &&
    leftIdentity.start === rightIdentity.start &&
    leftIdentity.duration === rightIdentity.duration;
}

function recordingBrowserFolderContainsRecording(folderData, expectedRecording) {
  return recordingBrowserListFromResponse(folderData, 'recordings')
    .some(recording => recordingBrowserSameRecording(recording, expectedRecording));
}

function recordingBrowserClearPendingDelete() {
  recordingBrowserPendingDelete = null;
  recordingBrowserCancelFolderRefreshTimer();
}

function recordingBrowserSetPendingDelete(recording, folderData, accepted, timedOut) {
  recordingBrowserPendingDelete = {
    recording: recording,
    folderData: folderData,
    folderPath: folderData && typeof folderData === 'object'
      ? String(folderData.path || '')
      : '',
    accepted: accepted === true,
    timedOut: timedOut === true
  };
}

function recordingBrowserRenderPendingDelete(recording, folderData, accepted, timedOut) {
  recordingBrowserSetPendingDelete(recording, folderData, accepted, timedOut);
  renderServerRecordingDetail(recording, folderData, {
    deletePending: true,
    deleteAccepted: accepted === true,
    deleteTimedOut: timedOut === true
  });
}

function recordingBrowserHandleDeleteFailure(recording, folderData, message) {
  recordingBrowserClearPendingDelete();
  renderServerRecordingDetail(recording, folderData, {
    deleteError: String(message || '')
  });
}

function recordingBrowserScheduleDeleteFolderReload() {
  if (!recordingBrowserPendingDelete || !recordingBrowserFolderLoader) {
    return false;
  }

  const expectedRecording = recordingBrowserPendingDelete.recording;
  const folderData = recordingBrowserPendingDelete.folderData;
  const folderPath = recordingBrowserPendingDelete.folderPath;
  let attempts = 0;
  const maxAttempts = 30;

  const reloadFolder = () => {
    attempts += 1;

    recordingBrowserFolderLoader(folderPath, 0)
      .then(data => {
        const latestFolderData = data && typeof data === 'object' ? data : {};

        if (!recordingBrowserFolderContainsRecording(
            latestFolderData,
            expectedRecording)) {
          recordingBrowserClearPendingDelete();
          renderServerRecordingFolder(latestFolderData);
          return;
        }

        if (attempts >= maxAttempts) {
          recordingBrowserFolderRefreshTimer = null;
          recordingBrowserRenderPendingDelete(
            expectedRecording,
            folderData,
            true,
            true
          );
          return;
        }

        recordingBrowserFolderRefreshTimer =
          window.setTimeout(reloadFolder, 1000);
      })
      .catch(() => {
        if (attempts >= maxAttempts) {
          recordingBrowserFolderRefreshTimer = null;
          recordingBrowserRenderPendingDelete(
            expectedRecording,
            folderData,
            true,
            true
          );
          return;
        }

        recordingBrowserFolderRefreshTimer =
          window.setTimeout(reloadFolder, 1000);
      });
  };

  recordingBrowserCancelFolderRefreshTimer();
  reloadFolder();
  return true;
}

function recordingBrowserRetryPendingDeleteReadback() {
  if (!recordingBrowserPendingDelete) {
    return false;
  }

  recordingBrowserPendingDelete.timedOut = false;
  recordingBrowserRenderPendingDelete(
    recordingBrowserPendingDelete.recording,
    recordingBrowserPendingDelete.folderData,
    true,
    false
  );
  return recordingBrowserScheduleDeleteFolderReload();
}

function recordingBrowserActionList(value) {
  if (!Array.isArray(value)) {
    return [];
  }

  return value
    .map(entry => String(entry || '').trim())
    .filter(entry => entry !== '');
}

function recordingBrowserActionPayload(recording, action, overrides) {
  const recordingId = recordingBrowserFirstValue(recording, ['recordingId', 'id', 'nativeId'], '');
  const recordingPath = recordingBrowserFirstValue(recording, ['path', 'fileName', 'directory'], '');
  const backendNativeId = recordingBrowserFirstValue(recording, ['backendNativeId', 'nativePath'], '');
  const recordingTitle = recordingBrowserFirstValue(recording, ['title', 'name', 'displayName'], '');
  const extra = overrides && typeof overrides === 'object' ? overrides : {};

  const payload = {
    recordingId: String(recordingId),
    action: String(action).toUpperCase(),
    dryRun: true
  };

  if (String(recordingPath || '').trim() !== '') {
    payload.recordingPath = String(recordingPath);
  }

  if (String(backendNativeId || '').trim() !== '') {
    payload.backendNativeId = String(backendNativeId);
  }

  if (String(recordingTitle || '').trim() !== '') {
    payload.recordingTitle = String(recordingTitle);
  }

  Object.keys(extra).forEach(key => {
    const value = extra[key];

    if (value !== undefined && value !== null && value !== '') {
      payload[key] = value;
    }
  });

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
      const message = String(result.message || '');
      const warnings = recordingBrowserActionList(result.warnings);
      const dryRunSkipped = message === 'dry-run backend execution skipped'
        || warnings.includes('dry-run only');

      if (dryRunSkipped && !result.success) {
        lines.push('Ausführung: Dry-Run / keine Backend-Ausführung');
      } else {
        lines.push('Ausführung: ' + (result.success ? 'erfolgreich' : 'nicht erfolgreich'));
      }
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

  const busyText = lines.join(' ');
  if (busyText.includes('wird ausgeführt') ||
      busyText.includes('läuft') ||
      busyText.includes('wird geprüft') ||
      busyText.includes('is running') ||
      busyText.includes('is being checked') ||
      busyText.includes('is being executed')) {
    const progress = document.createElement('progress');
    progress.className = 'recording-action-progress';
    progress.setAttribute('aria-label', 'Aktion läuft');
    target.appendChild(progress);
  }
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

function recordingBrowserPromptRenameName(recording, label, resultBox) {
  const currentTitle = recordingBrowserLocalRecordingTitle(recording, null);
  const newName = window.prompt('Neuer Name für diese Aufnahme:', currentTitle);

  if (newName === null) {
    recordingBrowserRenderActionResult(
      resultBox,
      label,
      { message: 'Umbenennen abgebrochen.' },
      null
    );
    return '';
  }

  const trimmedName = String(newName || '').trim();

  if (trimmedName === '') {
    recordingBrowserRenderActionResult(
      resultBox,
      label,
      null,
      new Error('Neuer Name darf nicht leer sein')
    );
    return '';
  }

  return trimmedName;
}


function recordingBrowserNormalizeMovePath(value) {
  return String(value || '')
    .trim()
    .replace(/~/g, '/')
    .replace(/\\/g, '/')
    .split('/')
    .map(part => part.trim())
    .filter(part => part !== '')
    .join('/');
}

function recordingBrowserMoveTargetParameter(value) {
  const rawValue = String(value || '').trim();

  if (rawValue === '/') {
    return '/';
  }

  return recordingBrowserNormalizeMovePath(rawValue);
}

function recordingBrowserMoveTargetFolderPath(targetParameter) {
  return String(targetParameter || '').trim() === '/'
    ? ''
    : recordingBrowserNormalizeMovePath(targetParameter);
}

function recordingBrowserMoveTargetLabel(targetParameter) {
  const folderPath = recordingBrowserMoveTargetFolderPath(targetParameter);
  return folderPath === ''
    ? recordingBrowserTranslate('common.rootFolder', 'Hauptordner')
    : recordingBrowserDisplayPathLabel(folderPath);
}

function recordingBrowserValidateNewMoveFolderName(value) {
  const name = String(value || '').trim();

  if (name === '') {
    return {
      valid: false,
      name: '',
      messageKey: 'recordings.move.newFolderRequired',
      fallback: 'Bitte einen Ordnernamen eingeben.'
    };
  }

  if (name === '.' || name === '..') {
    return {
      valid: false,
      name: name,
      messageKey: 'recordings.move.newFolderReserved',
      fallback: 'Die Namen . und .. sind nicht zulässig.'
    };
  }

  if (name.length > 80) {
    return {
      valid: false,
      name: name,
      messageKey: 'recordings.move.newFolderTooLong',
      fallback: 'Der Ordnername darf höchstens 80 Zeichen lang sein.'
    };
  }

  const containsForbiddenSeparator =
    name.includes('/') ||
    name.includes('\\') ||
    name.includes('~');
  const containsControlCharacter = Array.from(name).some(character => {
    const code = character.charCodeAt(0);
    return code <= 31 || code === 127;
  });

  if (containsForbiddenSeparator || containsControlCharacter) {
    return {
      valid: false,
      name: name,
      messageKey: 'recordings.move.newFolderInvalidCharacters',
      fallback: 'Der Ordnername darf weder /, \\, ~ noch Steuerzeichen enthalten.'
    };
  }

  return {
    valid: true,
    name: name,
    messageKey: '',
    fallback: ''
  };
}

function recordingBrowserJoinMoveFolderPath(parentPath, folderName) {
  const parent = recordingBrowserNormalizeMovePath(parentPath);
  const child = String(folderName || '').trim();

  return parent === '' ? child : parent + '/' + child;
}

function recordingBrowserMoveSourceFolderPath(recording, folderData) {
  const rawTitle = String(recordingBrowserFirstValue(
    recording,
    ['title', 'name', 'displayName'],
    ''
  )).replace(/~/g, '/');
  const titleParts = rawTitle
    .split('/')
    .map(part => part.trim())
    .filter(part => part !== '');

  if (titleParts.length > 1) {
    return recordingBrowserNormalizeMovePath(
      titleParts.slice(0, -1).join('/')
    );
  }

  return folderData && typeof folderData === 'object'
    ? recordingBrowserNormalizeMovePath(folderData.path || '')
    : '';
}

function recordingBrowserMoveNativeLeaf(recording) {
  const nativePath = String(recordingBrowserFirstValue(
    recording,
    ['backendNativeId', 'nativePath', 'path', 'fileName'],
    ''
  ))
    .replace(/\\/g, '/')
    .replace(/\/+$/, '');
  const parts = nativePath.split('/').filter(part => part !== '');
  return parts.length > 0 ? parts[parts.length - 1] : '';
}

function recordingBrowserMoveIdentity(recording) {
  return {
    nativeLeaf: recordingBrowserMoveNativeLeaf(recording),
    title: recordingBrowserNormalizeRenameLabel(
      recordingBrowserFirstValue(
        recording,
        ['title', 'name', 'displayName'],
        ''
      )
    ),
    start: String(recordingBrowserFirstValue(
      recording,
      ['startTime', 'start', 'date'],
      ''
    )),
    duration: String(recordingBrowserFirstValue(
      recording,
      ['durationSeconds', 'duration'],
      ''
    ))
  };
}

function recordingBrowserMoveCandidateMatches(recording, folderData, pendingMove) {
  const candidateIdentity = recordingBrowserMoveIdentity(recording);
  const expectedIdentity = pendingMove.identity || {};

  if (expectedIdentity.nativeLeaf !== '' &&
      candidateIdentity.nativeLeaf === expectedIdentity.nativeLeaf) {
    return true;
  }

  const candidateTitle = recordingBrowserNormalizeRenameLabel(
    recordingBrowserLocalRecordingTitle(recording, folderData)
  );
  const titleMatches =
    expectedIdentity.title !== '' &&
    recordingBrowserRenameLabelEquals(candidateTitle, expectedIdentity.title);
  const startMatches =
    expectedIdentity.start !== '' &&
    candidateIdentity.start === expectedIdentity.start;
  const durationMatches =
    expectedIdentity.duration !== '' &&
    candidateIdentity.duration === expectedIdentity.duration;

  return titleMatches && (startMatches || durationMatches);
}

function recordingBrowserFolderContainsMoveIdentity(folderData, pendingMove) {
  return recordingBrowserListFromResponse(folderData, 'recordings')
    .some(recording =>
      recordingBrowserMoveCandidateMatches(
        recording,
        folderData,
        pendingMove
      )
    );
}

function recordingBrowserFolderContainsMovedRecording(folderData, pendingMove) {
  const actualPath = recordingBrowserNormalizeMovePath(
    folderData && folderData.path ? folderData.path : ''
  );

  return actualPath === pendingMove.targetFolderPath &&
    recordingBrowserFolderContainsMoveIdentity(folderData, pendingMove);
}

function recordingBrowserMoveCandidateFolderPaths(folderData, pendingMove) {
  const expectedTitle = String(
    pendingMove && pendingMove.identity
      ? pendingMove.identity.title || ''
      : ''
  ).trim();

  if (expectedTitle === '') {
    return [];
  }

  return recordingBrowserListFromResponse(folderData, 'folders')
    .filter(folder => {
      const folderName = decodeRecordingText(
        recordingBrowserFirstValue(folder, ['name', 'title'], '')
      );
      const folderPath = recordingBrowserFirstValue(
        folder,
        ['path'],
        ''
      );

      return recordingBrowserRenameLabelEquals(
        folderName,
        expectedTitle
      ) || recordingBrowserRenameLabelEquals(
        folderPath,
        expectedTitle
      );
    })
    .map(folder => recordingBrowserNormalizeMovePath(
      recordingBrowserFirstValue(folder, ['path'], '')
    ))
    .filter(folderPath => folderPath !== '');
}

function recordingBrowserFindMovedRecordingInTarget(
  targetFolderData,
  pendingMove
) {
  if (recordingBrowserFolderContainsMovedRecording(
      targetFolderData,
      pendingMove)) {
    return Promise.resolve({
      found: true,
      folderData: targetFolderData
    });
  }

  if (!recordingBrowserFolderLoader) {
    return Promise.resolve({
      found: false,
      folderData: targetFolderData
    });
  }

  const candidatePaths = recordingBrowserMoveCandidateFolderPaths(
    targetFolderData,
    pendingMove
  );

  if (candidatePaths.length === 0) {
    return Promise.resolve({
      found: false,
      folderData: targetFolderData
    });
  }

  return Promise.all(candidatePaths.map(candidatePath =>
    recordingBrowserFolderLoader(candidatePath, 0)
      .then(data => {
        const candidateFolderData = data && typeof data === 'object'
          ? data
          : {};

        return recordingBrowserFolderContainsMoveIdentity(
          candidateFolderData,
          pendingMove
        )
          ? candidateFolderData
          : null;
      })
      .catch(() => null)
  )).then(candidateResults => {
    const matchedFolderData = candidateResults.find(
      candidateResult => candidateResult !== null
    );

    return {
      found: Boolean(matchedFolderData),
      folderData: matchedFolderData || targetFolderData
    };
  });
}

function recordingBrowserSetPendingMove(
  recording,
  folderData,
  targetParameter,
  accepted
) {
  recordingBrowserPendingMove = {
    recording: recording,
    folderData: folderData,
    sourceFolderPath: recordingBrowserMoveSourceFolderPath(
      recording,
      folderData
    ),
    targetParameter: targetParameter,
    targetFolderPath: recordingBrowserMoveTargetFolderPath(targetParameter),
    targetLabel: recordingBrowserMoveTargetLabel(targetParameter),
    identity: recordingBrowserMoveIdentity(recording),
    accepted: accepted === true,
    timedOut: false
  };
}

function recordingBrowserConfirmPendingMove(targetParameter) {
  if (!recordingBrowserPendingMove ||
      recordingBrowserPendingMove.targetParameter !== targetParameter) {
    return false;
  }

  recordingBrowserPendingMove.accepted = true;
  recordingBrowserPendingMove.timedOut = false;
  return true;
}

function recordingBrowserClearPendingMove() {
  recordingBrowserPendingMove = null;
  recordingBrowserCancelFolderRefreshTimer();
}

function recordingBrowserRenderMovePendingDetail(
  recording,
  folderData,
  targetParameter,
  accepted,
  timedOut
) {
  recordingBrowserSetPendingMove(
    recording,
    folderData,
    targetParameter,
    accepted
  );
  recordingBrowserPendingMove.timedOut = timedOut === true;

  renderServerRecordingDetail(
    recording,
    folderData,
    {
      movePending: true,
      moveAccepted: accepted === true,
      moveTimedOut: timedOut === true,
      moveTargetPath: targetParameter
    }
  );
}

function recordingBrowserReloadVisibleFolderAfterMove(targetFolderData, pendingMove) {
  const visibleFolderPath = recordingBrowserVisibleFolderPath;
  recordingBrowserClearPendingMove();

  if (visibleFolderPath === null) {
    renderServerRecordingFolder(targetFolderData);
    return;
  }

  if (!recordingBrowserFolderLoader) {
    renderServerRecordingFolder(targetFolderData);
    return;
  }

  recordingBrowserFolderLoader(visibleFolderPath, 0)
    .then(renderServerRecordingFolder)
    .catch(recordingBrowserRenderFolderLoadError);
}

function recordingBrowserRenderMoveTimeout(pendingMove) {
  recordingBrowserFolderRefreshTimer = null;
  pendingMove.timedOut = true;

  if (recordingBrowserVisibleFolderPath === null) {
    renderServerRecordingDetail(
      pendingMove.recording,
      pendingMove.folderData,
      {
        movePending: true,
        moveAccepted: true,
        moveTimedOut: true,
        moveTargetPath: pendingMove.targetParameter
      }
    );
    return;
  }

  if (!recordingBrowserFolderLoader) {
    return;
  }

  const visiblePath = recordingBrowserVisibleFolderPath;
  recordingBrowserFolderLoader(visiblePath, 0)
    .then(renderServerRecordingFolder)
    .catch(() => {
      /* Der bestätigte Move-Zustand bleibt sichtbar und kann erneut geprüft werden. */
    });
}

function recordingBrowserScheduleMoveTargetFolderReload() {
  if (!recordingBrowserPendingMove ||
      !recordingBrowserPendingMove.accepted ||
      recordingBrowserPendingMove.timedOut ||
      !recordingBrowserFolderLoader ||
      recordingBrowserFolderRefreshTimer !== null) {
    return false;
  }

  let attempts = 0;
  const maxAttempts = 45;

  const reloadTarget = () => {
    const pendingMove = recordingBrowserPendingMove;

    if (!pendingMove || !pendingMove.accepted) {
      recordingBrowserFolderRefreshTimer = null;
      return;
    }

    attempts += 1;

    recordingBrowserFolderLoader(pendingMove.targetFolderPath, 0)
      .then(data => {
        if (recordingBrowserPendingMove !== pendingMove) {
          recordingBrowserFolderRefreshTimer = null;
          return;
        }

        const targetFolderData = data && typeof data === 'object'
          ? data
          : {};

        return recordingBrowserFindMovedRecordingInTarget(
          targetFolderData,
          pendingMove
        );
      })
      .then(readbackResult => {
        if (recordingBrowserPendingMove !== pendingMove) {
          recordingBrowserFolderRefreshTimer = null;
          return;
        }

        if (readbackResult.found) {
          recordingBrowserReloadVisibleFolderAfterMove(
            readbackResult.folderData,
            pendingMove
          );
          return;
        }

        if (attempts >= maxAttempts) {
          recordingBrowserRenderMoveTimeout(pendingMove);
          return;
        }

        recordingBrowserFolderRefreshTimer =
          window.setTimeout(reloadTarget, 1000);
      })
      .catch(() => {
        if (recordingBrowserPendingMove !== pendingMove) {
          recordingBrowserFolderRefreshTimer = null;
          return;
        }

        if (attempts >= maxAttempts) {
          recordingBrowserRenderMoveTimeout(pendingMove);
          return;
        }

        recordingBrowserFolderRefreshTimer =
          window.setTimeout(reloadTarget, 1000);
      });
  };

  reloadTarget();
  return true;
}

function recordingBrowserRetryPendingMoveReadback() {
  if (!recordingBrowserPendingMove) {
    return;
  }

  recordingBrowserPendingMove.timedOut = false;
  recordingBrowserCancelFolderRefreshTimer();
  recordingBrowserScheduleMoveTargetFolderReload();
}

function recordingBrowserActivateConfirmedMoveReadback() {
  if (!recordingBrowserPendingMove) {
    return;
  }

  if (recordingBrowserVisibleFolderPath === null) {
    const pendingMove = recordingBrowserPendingMove;
    renderServerRecordingDetail(
      pendingMove.recording,
      pendingMove.folderData,
      {
        movePending: true,
        moveAccepted: true,
        moveTimedOut: false,
        moveTargetPath: pendingMove.targetParameter
      }
    );
    recordingBrowserScheduleMoveTargetFolderReload();
    return;
  }

  if (!recordingBrowserFolderLoader) {
    recordingBrowserScheduleMoveTargetFolderReload();
    return;
  }

  const visiblePath = recordingBrowserVisibleFolderPath;
  recordingBrowserFolderLoader(visiblePath, 0)
    .then(renderServerRecordingFolder)
    .catch(() => recordingBrowserScheduleMoveTargetFolderReload());
}

function recordingBrowserHandleMoveFailure(
  recording,
  folderData,
  message
) {
  const visibleFolderPath = recordingBrowserVisibleFolderPath;
  recordingBrowserClearPendingMove();

  if (visibleFolderPath === null || !recordingBrowserFolderLoader) {
    renderServerRecordingDetail(
      recording,
      folderData,
      { moveError: message }
    );
    return;
  }

  recordingBrowserFolderLoader(visibleFolderPath, 0)
    .then(data => {
      const folderResult = data && typeof data === 'object'
        ? Object.assign({}, data)
        : {};
      folderResult.actionNotice = recordingBrowserTranslate(
        'recordings.move.failed',
        'Verschieben fehlgeschlagen: {message}',
        { message: message }
      );
      renderServerRecordingFolder(folderResult);
    })
    .catch(error => recordingBrowserRenderFolderLoadError(error));
}

function recordingBrowserCreateMoveEditor(recording, folderData, resultBox) {
  const editor = document.createElement('details');
  editor.className = 'recording-move-editor';

  editor.appendChild(recordingBrowserAddText(
    document.createElement('summary'),
    recordingBrowserTranslate('recordings.move.title', 'Verschieben …')
  ));

  const body = document.createElement('section');
  body.className = 'module-placeholder recording-move-editor-body';

  const sourceFolderPath = recordingBrowserMoveSourceFolderPath(
    recording,
    folderData
  );
  const sourceTitle = recordingBrowserLocalRecordingTitle(
    recording,
    folderData
  );

  body.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    recordingBrowserTranslate(
      'recordings.move.recording',
      'Aufnahme: {title}',
      { title: sourceTitle }
    )
  ));
  body.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    recordingBrowserTranslate(
      'recordings.move.currentFolder',
      'Aktueller Ordner: {folder}',
      {
        folder: sourceFolderPath === ''
          ? recordingBrowserTranslate('common.rootFolder', 'Hauptordner')
          : recordingBrowserDisplayPathLabel(sourceFolderPath)
      }
    )
  ));

  const label = document.createElement('label');
  label.appendChild(recordingBrowserAddText(
    document.createElement('span'),
    recordingBrowserTranslate('recordings.move.targetFolder', 'Zielordner')
  ));

  const input = document.createElement('input');
  input.type = 'text';
  input.placeholder = recordingBrowserTranslate(
    'recordings.move.placeholder',
    'z. B. Filme/Archiv'
  );
  input.autocomplete = 'off';
  label.appendChild(input);
  body.appendChild(label);

  const selectedTarget = document.createElement('p');
  selectedTarget.textContent = recordingBrowserTranslate(
    'recordings.move.noTarget',
    'Noch kein Zielordner ausgewählt.'
  );
  body.appendChild(selectedTarget);

  const validationStatus = document.createElement('p');
  validationStatus.className =
    'recording-move-validation-status neutral';
  validationStatus.setAttribute('role', 'status');
  validationStatus.setAttribute('aria-live', 'polite');
  validationStatus.textContent = recordingBrowserTranslate(
    'recordings.move.selectThenValidate',
    'Ziel auswählen und anschließend prüfen.'
  );
  body.appendChild(validationStatus);

  const browseBox = document.createElement('section');
  browseBox.className = 'recording-move-folder-browser';
  body.appendChild(browseBox);

  const controls = document.createElement('div');
  controls.className = 'recording-action-buttons';

  const rootButton = document.createElement('button');
  rootButton.type = 'button';
  rootButton.textContent = recordingBrowserTranslate(
    'recordings.move.rootAsTarget',
    'Hauptordner als Ziel'
  );

  const browseButton = document.createElement('button');
  browseButton.type = 'button';
  browseButton.textContent = recordingBrowserTranslate(
    'recordings.move.browse',
    'Vorhandene Ordner durchsuchen'
  );
  browseButton.disabled = !recordingBrowserFolderLoader;

  const createFolderButton = document.createElement('button');
  createFolderButton.type = 'button';
  createFolderButton.textContent = recordingBrowserTranslate(
    'recordings.move.createFolder',
    'Neuen Zielordner'
  );
  createFolderButton.disabled = !recordingBrowserFolderLoader;

  const validateButton = document.createElement('button');
  validateButton.type = 'button';
  validateButton.textContent = recordingBrowserTranslate(
    'recordings.move.validate',
    'Ziel prüfen'
  );

  const executeButton = document.createElement('button');
  executeButton.type = 'button';
  executeButton.className = 'recording-move-execute-button';
  executeButton.textContent = recordingBrowserTranslate(
    'recordings.move.execute',
    'Jetzt verschieben'
  );
  executeButton.disabled = true;

  controls.appendChild(rootButton);
  controls.appendChild(browseButton);
  controls.appendChild(createFolderButton);
  controls.appendChild(validateButton);
  controls.appendChild(executeButton);
  body.appendChild(controls);
  editor.appendChild(body);

  let rootSelected = false;
  let validatedTarget = '';

  const targetParameter = () => {
    const rawValue = String(input.value || '').trim();

    if (rawValue === '/') {
      return '/';
    }

    const normalized = recordingBrowserNormalizeMovePath(rawValue);

    if (normalized !== '') {
      return normalized;
    }

    return rootSelected ? '/' : '';
  };

  const setValidationStatus = (state, message) => {
    validationStatus.className =
      'recording-move-validation-status ' + state;
    validationStatus.textContent = message;
  };

  const focusReadyMoveButton = () => {
    window.setTimeout(() => {
      executeButton.focus();
      executeButton.scrollIntoView({
        behavior: 'smooth',
        block: 'center'
      });
    }, 0);
  };

  const invalidateValidation = () => {
    validatedTarget = '';
    executeButton.disabled = true;
    executeButton.classList.remove('ready');
    setValidationStatus(
      'neutral',
      targetParameter() === ''
        ? recordingBrowserTranslate(
            'recordings.move.selectThenValidate',
            'Ziel auswählen und anschließend prüfen.'
          )
        : recordingBrowserTranslate(
            'recordings.move.targetChanged',
            'Ziel geändert – bitte erneut prüfen.'
          )
    );
  };

  const updateSelectedTarget = () => {
    const target = targetParameter();
    selectedTarget.textContent = target === ''
      ? recordingBrowserTranslate(
          'recordings.move.noTarget',
          'Noch kein Zielordner ausgewählt.'
        )
      : recordingBrowserTranslate(
          'recordings.move.selectedTarget',
          'Ausgewähltes Ziel: {target}',
          { target: recordingBrowserMoveTargetLabel(target) }
        );
  };

  const chooseTargetFolder = path => {
    const normalized = recordingBrowserNormalizeMovePath(path);
    rootSelected = normalized === '';
    input.value = normalized;
    invalidateValidation();
    updateSelectedTarget();
  };

  const renderFolderBrowserError = error => {
    browseBox.replaceChildren();
    browseBox.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      recordingBrowserTranslate(
        'recordings.move.folderLoadError',
        'Ordner konnten nicht geladen werden: {message}',
        {
          message: error && error.message
            ? error.message
            : String(error)
        }
      )
    ));
  };

  const loadFolderBrowser = (path, openCreateFolder) => {
    if (!recordingBrowserFolderLoader) {
      return;
    }

    const normalizedPath = recordingBrowserNormalizeMovePath(path);
    browseBox.replaceChildren();
    browseBox.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      recordingBrowserTranslate(
        'recordings.move.loadingFolders',
        'Lade Zielordner …'
      )
    ));

    const progress = document.createElement('progress');
    progress.className = 'recording-folder-refresh-progress';
    progress.setAttribute(
      'aria-label',
      recordingBrowserTranslate(
        'recordings.move.loadingFoldersAria',
        'Zielordner werden geladen'
      )
    );
    browseBox.appendChild(progress);

    recordingBrowserFolderLoader(normalizedPath, 0)
      .then(data => {
        const browserData = data && typeof data === 'object' ? data : {};
        const browserPath = recordingBrowserNormalizeMovePath(
          browserData.path || normalizedPath
        );
        const parentPath = recordingBrowserNormalizeMovePath(
          browserData.parentPath || ''
        );
        const folders = recordingBrowserListFromResponse(
          browserData,
          'folders'
        );

        browseBox.replaceChildren();
        browseBox.appendChild(recordingBrowserAddText(
          document.createElement('h4'),
          browserPath === ''
            ? recordingBrowserTranslate('common.rootFolder', 'Hauptordner')
            : recordingBrowserDisplayPathLabel(browserPath)
        ));

        const browserControls = document.createElement('div');
        browserControls.className = 'recording-action-buttons';

        const chooseButton = document.createElement('button');
        chooseButton.type = 'button';
        chooseButton.textContent = recordingBrowserTranslate(
          'recordings.move.useThisFolder',
          'Diesen Ordner als Ziel verwenden'
        );
        chooseButton.addEventListener('click', () =>
          chooseTargetFolder(browserPath)
        );
        browserControls.appendChild(chooseButton);

        if (browserPath !== '') {
          const backButton = document.createElement('button');
          backButton.type = 'button';
          backButton.textContent = recordingBrowserTranslate(
            'recordings.move.upOneLevel',
            'Eine Ebene zurück'
          );
          backButton.addEventListener('click', () =>
            loadFolderBrowser(parentPath, false)
          );
          browserControls.appendChild(backButton);
        }

        const inlineCreateButton = document.createElement('button');
        inlineCreateButton.type = 'button';
        inlineCreateButton.textContent = recordingBrowserTranslate(
          'recordings.move.createFolder',
          'Neuen Zielordner'
        );
        browserControls.appendChild(inlineCreateButton);
        browseBox.appendChild(browserControls);

        const createFolderPanel = document.createElement('form');
        createFolderPanel.className = 'recording-move-new-folder';
        createFolderPanel.hidden = true;

        createFolderPanel.appendChild(recordingBrowserAddText(
          document.createElement('h5'),
          recordingBrowserTranslate(
            'recordings.move.newFolderTitle',
            'Neuen Zielordner anlegen'
          )
        ));
        createFolderPanel.appendChild(recordingBrowserAddText(
          document.createElement('p'),
          recordingBrowserTranslate(
            'recordings.move.newFolderHint',
            'Der Ordner wird zusammen mit der Aufnahme beim Verschieben angelegt.'
          )
        ));

        const folderNameLabel = document.createElement('label');
        folderNameLabel.appendChild(recordingBrowserAddText(
          document.createElement('span'),
          recordingBrowserTranslate(
            'recordings.move.newFolderName',
            'Ordnername'
          )
        ));

        const folderNameInput = document.createElement('input');
        folderNameInput.type = 'text';
        folderNameInput.maxLength = 80;
        folderNameInput.autocomplete = 'off';
        folderNameInput.placeholder = recordingBrowserTranslate(
          'recordings.move.newFolderPlaceholder',
          'z. B. Anime'
        );
        folderNameLabel.appendChild(folderNameInput);
        createFolderPanel.appendChild(folderNameLabel);

        const createFolderStatus = document.createElement('p');
        createFolderStatus.className = 'recording-move-new-folder-status';
        createFolderStatus.setAttribute('role', 'status');
        createFolderStatus.setAttribute('aria-live', 'polite');
        createFolderPanel.appendChild(createFolderStatus);

        const createFolderActions = document.createElement('div');
        createFolderActions.className = 'recording-action-buttons';

        const useNewFolderButton = document.createElement('button');
        useNewFolderButton.type = 'submit';
        useNewFolderButton.textContent = recordingBrowserTranslate(
          'recordings.move.newFolderUse',
          'Neuen Ordner als Ziel verwenden'
        );

        const cancelNewFolderButton = document.createElement('button');
        cancelNewFolderButton.type = 'button';
        cancelNewFolderButton.textContent = recordingBrowserTranslate(
          'recordings.move.newFolderCancel',
          'Abbrechen'
        );

        createFolderActions.appendChild(useNewFolderButton);
        createFolderActions.appendChild(cancelNewFolderButton);
        createFolderPanel.appendChild(createFolderActions);
        browseBox.appendChild(createFolderPanel);

        const revealCreateFolderPanel = () => {
          createFolderPanel.hidden = false;
          createFolderStatus.textContent = '';
          window.setTimeout(() => folderNameInput.focus(), 0);
        };

        inlineCreateButton.addEventListener('click', revealCreateFolderPanel);
        cancelNewFolderButton.addEventListener('click', () => {
          createFolderPanel.hidden = true;
          folderNameInput.value = '';
          createFolderStatus.textContent = '';
        });

        createFolderPanel.addEventListener('submit', event => {
          event.preventDefault();

          const validation = recordingBrowserValidateNewMoveFolderName(
            folderNameInput.value
          );

          if (!validation.valid) {
            createFolderStatus.className =
              'recording-move-new-folder-status error';
            createFolderStatus.textContent = recordingBrowserTranslate(
              validation.messageKey,
              validation.fallback
            );
            folderNameInput.focus();
            return;
          }

          const targetPath = recordingBrowserJoinMoveFolderPath(
            browserPath,
            validation.name
          );
          const targetExists = folders.some(folder => {
            const folderName = decodeRecordingText(
              recordingBrowserFirstValue(folder, ['name', 'title'], '')
            );
            const folderPath = recordingBrowserNormalizeMovePath(
              recordingBrowserFirstValue(
                folder,
                ['path'],
                recordingBrowserJoinMoveFolderPath(browserPath, folderName)
              )
            );
            return folderPath === targetPath;
          });

          chooseTargetFolder(targetPath);
          setValidationStatus(
            'neutral',
            recordingBrowserTranslate(
              targetExists
                ? 'recordings.move.newFolderExistingSelected'
                : 'recordings.move.newFolderSelected',
              targetExists
                ? 'Der Ordner „{target}“ existiert bereits und wurde als Ziel ausgewählt. Bitte Ziel prüfen.'
                : 'Neues Ziel „{target}“ ausgewählt. Der Ordner wird beim Verschieben angelegt. Bitte Ziel prüfen.',
              { target: recordingBrowserMoveTargetLabel(targetPath) }
            )
          );
          createFolderPanel.hidden = true;
          validateButton.focus();
          validateButton.scrollIntoView({
            behavior: 'smooth',
            block: 'center'
          });
        });

        if (openCreateFolder === true) {
          revealCreateFolderPanel();
        }

        if (folders.length === 0) {
          browseBox.appendChild(recordingBrowserAddText(
            document.createElement('p'),
            recordingBrowserTranslate(
              'recordings.move.noSubfolders',
              'Keine weiteren Unterordner vorhanden.'
            )
          ));
          return;
        }

        folders.forEach(folder => {
          const folderName = decodeRecordingText(
            recordingBrowserFirstValue(
              folder,
              ['name', 'title'],
              recordingBrowserTranslate('common.folder', 'Ordner')
            )
          );
          const folderPath = recordingBrowserFirstValue(
            folder,
            ['path'],
            folderName
          );
          const folderButton = document.createElement('button');
          folderButton.type = 'button';
          folderButton.textContent = folderName;
          folderButton.addEventListener('click', () =>
            loadFolderBrowser(folderPath, false)
          );
          browseBox.appendChild(folderButton);
        });
      })
      .catch(renderFolderBrowserError);
  };

  input.addEventListener('input', () => {
    rootSelected = String(input.value || '').trim() === '/';
    invalidateValidation();
    updateSelectedTarget();
  });

  rootButton.addEventListener('click', () => {
    chooseTargetFolder('');
  });

  browseButton.addEventListener('click', () => {
    loadFolderBrowser('', false);
  });

  createFolderButton.addEventListener('click', () => {
    loadFolderBrowser('', true);
  });

  validateButton.addEventListener('click', () => {
    const validationTitle = recordingBrowserTranslate(
      'recordings.move.validateAction',
      'Verschieben prüfen'
    );

    if (!recordingBrowserActionRunner) {
      const message = recordingBrowserTranslate(
        'recordings.move.targetValidationUnavailable',
        'Zielprüfung ist derzeit nicht verfügbar.'
      );
      setValidationStatus('error', message);
      recordingBrowserRenderActionResult(
        resultBox,
        validationTitle,
        null,
        new Error(message)
      );
      return;
    }

    const target = targetParameter();

    if (target === '') {
      const message = recordingBrowserTranslate(
        'recordings.move.selectFirst',
        'Bitte zuerst einen Zielordner auswählen.'
      );
      setValidationStatus('error', message);
      recordingBrowserRenderActionResult(
        resultBox,
        validationTitle,
        null,
        new Error(message)
      );
      return;
    }

    if (recordingBrowserMoveTargetFolderPath(target) === sourceFolderPath) {
      const message = recordingBrowserTranslate(
        'recordings.move.alreadyInFolder',
        'Die Aufnahme befindet sich bereits in diesem Ordner.'
      );
      setValidationStatus('error', message);
      recordingBrowserRenderActionResult(
        resultBox,
        validationTitle,
        null,
        new Error(message)
      );
      return;
    }

    validateButton.disabled = true;
    executeButton.disabled = true;
    executeButton.classList.remove('ready');
    setValidationStatus(
      'pending',
      recordingBrowserTranslate(
        'recordings.move.targetValidationPending',
        'Ziel „{target}“ wird geprüft …',
        { target: recordingBrowserMoveTargetLabel(target) }
      )
    );
    recordingBrowserRenderActionResult(
      resultBox,
      validationTitle,
      {
        message: recordingBrowserTranslate(
          'recordings.move.validating',
          'Verschieben wird geprüft …'
        )
      },
      null
    );

    recordingBrowserActionRunner({
      mode: 'validate',
      action: 'MOVE',
      payload: recordingBrowserActionPayload(recording, 'MOVE', {
        dryRun: true,
        targetPath: target
      }),
      recording: recording
    })
      .then(result => {
        recordingBrowserRenderActionResult(
          resultBox,
          validationTitle,
          result,
          null
        );

        const targetStillSelected = targetParameter() === target;

        if (result && result.valid === true && targetStillSelected) {
          validatedTarget = target;
          executeButton.disabled = false;
          executeButton.classList.add('ready');
          setValidationStatus(
            'success',
            recordingBrowserTranslate(
              'recordings.move.targetReady',
              'Ziel geprüft – bereit zum Verschieben nach „{target}“.',
              { target: recordingBrowserMoveTargetLabel(target) }
            )
          );
          focusReadyMoveButton();
          return;
        }

        executeButton.disabled = true;
        executeButton.classList.remove('ready');

        if (!targetStillSelected) {
          setValidationStatus(
            'neutral',
            recordingBrowserTranslate(
              'recordings.move.targetChangedDuringValidation',
              'Ziel wurde während der Prüfung geändert – bitte erneut prüfen.'
            )
          );
          return;
        }

        setValidationStatus(
          'error',
          recordingBrowserTranslate(
            'recordings.move.targetValidationFailedDetails',
            'Zielprüfung nicht erfolgreich. Details stehen unter dem Dialog.'
          )
        );
      })
      .catch(error => {
        executeButton.disabled = true;
        executeButton.classList.remove('ready');
        setValidationStatus(
          'error',
          recordingBrowserTranslate(
            'recordings.move.targetValidationFailed',
            'Zielprüfung fehlgeschlagen: {message}',
            {
              message: error && error.message
                ? error.message
                : String(error)
            }
          )
        );
        recordingBrowserRenderActionResult(
          resultBox,
          validationTitle,
          null,
          error
        );
      })
      .finally(() => {
        validateButton.disabled = false;
      });
  });

  executeButton.addEventListener('click', () => {
    const target = targetParameter();
    const moveTitle = recordingBrowserTranslate(
      'recordings.move.title',
      'Verschieben'
    );

    if (target === '' || validatedTarget !== target) {
      executeButton.disabled = true;
      executeButton.classList.remove('ready');
      setValidationStatus(
        'error',
        recordingBrowserTranslate(
          'recordings.move.validationRequired',
          'Das aktuelle Ziel muss vor dem Verschieben erfolgreich geprüft werden.'
        )
      );
      recordingBrowserRenderActionResult(
        resultBox,
        moveTitle,
        null,
        new Error(recordingBrowserTranslate(
          'recordings.move.requiresValidation',
          'Verschieben muss vor der Ausführung erfolgreich geprüft werden.'
        ))
      );
      return;
    }

    const targetLabel = recordingBrowserMoveTargetLabel(target);

    if (!window.confirm(recordingBrowserTranslate(
        'recordings.move.confirm',
        'Aufnahme wirklich nach „{target}“ verschieben?',
        { target: targetLabel }
      ))) {
      recordingBrowserRenderActionResult(
        resultBox,
        moveTitle,
        {
          message: recordingBrowserTranslate(
            'recordings.move.notConfirmed',
            'Verschieben nicht bestätigt.'
          )
        },
        null
      );
      return;
    }

    setValidationStatus(
      'pending',
      recordingBrowserTranslate(
        'recordings.move.executing',
        'Verschieben nach „{target}“ wird ausgeführt …',
        { target: targetLabel }
      )
    );

    recordingBrowserRenderMovePendingDetail(
      recording,
      folderData,
      target,
      false,
      false
    );

    recordingBrowserActionRunner({
      mode: 'execute',
      action: 'MOVE',
      payload: recordingBrowserActionPayload(recording, 'MOVE', {
        dryRun: false,
        targetPath: target
      }),
      recording: recording
    })
      .then(result => {
        if (result && result.success === true) {
          recordingBrowserConfirmPendingMove(target);
          recordingBrowserActivateConfirmedMoveReadback();
          return;
        }

        const message = result && (result.message || result.error)
          ? String(result.message || result.error)
          : recordingBrowserTranslate(
              'recordings.move.backendRejected',
              'Backend hat das Verschieben abgelehnt'
            );

        recordingBrowserHandleMoveFailure(
          recording,
          folderData,
          message
        );
      })
      .catch(error => {
        recordingBrowserHandleMoveFailure(
          recording,
          folderData,
          error && error.message ? error.message : String(error)
        );
      });
  });

  updateSelectedTarget();
  return editor;
}

function recordingBrowserCreateDeleteEditor(recording, folderData, resultBox) {
  const editor = document.createElement('details');
  editor.className = 'recording-delete-editor';

  editor.appendChild(recordingBrowserAddText(
    document.createElement('summary'),
    recordingBrowserTranslate('recordings.delete.title', 'Löschen …')
  ));

  const body = document.createElement('section');
  body.className = 'module-placeholder recording-delete-editor-body';

  const title = recordingBrowserLocalRecordingTitle(recording, folderData);
  body.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    recordingBrowserTranslate(
      'recordings.delete.recording',
      'Aufnahme: {title}',
      { title: title }
    )
  ));

  const danger = document.createElement('p');
  danger.className = 'recording-delete-danger-hint';
  danger.textContent = recordingBrowserTranslate(
    'recordings.delete.dangerHint',
    'Die Aufnahme wird aus der aktiven VDR-Aufnahmeliste gelöscht. Nach der letzten Aufnahme verschwindet ein leerer Ordner automatisch.'
  );
  body.appendChild(danger);

  const status = document.createElement('p');
  status.className = 'recording-delete-status neutral';
  status.setAttribute('role', 'status');
  status.setAttribute('aria-live', 'polite');
  status.textContent = recordingBrowserTranslate(
    'recordings.delete.notReady',
    'Die Aufnahme muss vor dem Löschen erfolgreich geprüft werden.'
  );
  body.appendChild(status);

  const controls = document.createElement('div');
  controls.className = 'recording-action-buttons';

  const validateButton = document.createElement('button');
  validateButton.type = 'button';
  validateButton.textContent = recordingBrowserTranslate(
    'recordings.delete.validate',
    'Löschung prüfen'
  );

  const executeButton = document.createElement('button');
  executeButton.type = 'button';
  executeButton.className = 'recording-delete-execute-button';
  executeButton.textContent = recordingBrowserTranslate(
    'recordings.delete.execute',
    'Aufnahme löschen'
  );
  executeButton.disabled = true;

  controls.appendChild(validateButton);
  controls.appendChild(executeButton);
  body.appendChild(controls);
  editor.appendChild(body);

  let validated = false;

  const setStatus = (state, message) => {
    status.className = 'recording-delete-status ' + state;
    status.textContent = message;
  };

  const invalidate = message => {
    validated = false;
    executeButton.disabled = true;
    executeButton.classList.remove('ready');
    setStatus('neutral', message);
  };

  const validationTitle = recordingBrowserTranslate(
    'recordings.delete.validateAction',
    'Löschen prüfen'
  );

  const validateDelete = finalCheck => {
    if (!recordingBrowserActionRunner) {
      const message = recordingBrowserTranslate(
        'recordings.delete.validationUnavailable',
        'Löschprüfung ist derzeit nicht verfügbar.'
      );
      setStatus('error', message);
      recordingBrowserRenderActionResult(
        resultBox,
        validationTitle,
        null,
        new Error(message)
      );
      return Promise.resolve(false);
    }

    validateButton.disabled = true;
    executeButton.disabled = true;
    executeButton.classList.remove('ready');
    setStatus(
      'pending',
      recordingBrowserTranslate(
        finalCheck
          ? 'recordings.delete.finalValidation'
          : 'recordings.delete.validationPending',
        finalCheck
          ? 'Sicherheitsprüfung wird unmittelbar vor dem Löschen wiederholt …'
          : 'Aufnahme wird zum Löschen geprüft …'
      )
    );
    recordingBrowserRenderActionResult(
      resultBox,
      validationTitle,
      {
        message: recordingBrowserTranslate(
          finalCheck
            ? 'recordings.delete.finalValidation'
            : 'recordings.delete.validationPending',
          finalCheck
            ? 'Sicherheitsprüfung wird unmittelbar vor dem Löschen wiederholt …'
            : 'Aufnahme wird zum Löschen geprüft …'
        )
      },
      null
    );

    return recordingBrowserActionRunner({
      mode: 'validate',
      action: 'DELETE',
      payload: recordingBrowserActionPayload(recording, 'DELETE', {
        dryRun: true
      }),
      recording: recording
    })
      .then(result => {
        recordingBrowserRenderActionResult(
          resultBox,
          validationTitle,
          result,
          null
        );

        if (result && result.valid === true) {
          validated = true;
          executeButton.disabled = false;
          executeButton.classList.add('ready');
          setStatus(
            'success',
            recordingBrowserTranslate(
              'recordings.delete.ready',
              'Aufnahme geprüft – Löschen kann ausdrücklich bestätigt werden.'
            )
          );
          return true;
        }

        invalidate(recordingBrowserTranslate(
          finalCheck
            ? 'recordings.delete.finalValidationFailed'
            : 'recordings.delete.validationFailedDetails',
          finalCheck
            ? 'Die unmittelbar wiederholte Sicherheitsprüfung hat die Löschung abgelehnt.'
            : 'Die Aufnahme konnte nicht zum Löschen freigegeben werden. Details stehen unter dem Dialog.'
        ));
        return false;
      })
      .catch(error => {
        invalidate(recordingBrowserTranslate(
          'recordings.delete.validationFailed',
          'Löschprüfung fehlgeschlagen: {message}',
          {
            message: error && error.message
              ? error.message
              : String(error)
          }
        ));
        recordingBrowserRenderActionResult(
          resultBox,
          validationTitle,
          null,
          error
        );
        return false;
      })
      .finally(() => {
        validateButton.disabled = false;
      });
  };

  validateButton.addEventListener('click', () => {
    validateDelete(false).then(isValid => {
      if (isValid) {
        window.setTimeout(() => executeButton.focus(), 0);
      }
    });
  });

  executeButton.addEventListener('click', () => {
    if (!validated) {
      invalidate(recordingBrowserTranslate(
        'recordings.delete.notReady',
        'Die Aufnahme muss vor dem Löschen erfolgreich geprüft werden.'
      ));
      return;
    }

    executeButton.disabled = true;

    validateDelete(true).then(isValid => {
      if (!isValid) {
        return;
      }

      if (!window.confirm(recordingBrowserTranslate(
          'recordings.delete.confirm',
          'Aufnahme „{title}“ wirklich löschen? Dieser Vorgang kann über VDR-Suite nicht rückgängig gemacht werden.',
          { title: title }
        ))) {
        executeButton.disabled = false;
        executeButton.classList.add('ready');
        setStatus(
          'success',
          recordingBrowserTranslate(
            'recordings.delete.cancelled',
            'Löschen nicht bestätigt.'
          )
        );
        return;
      }

      setStatus(
        'pending',
        recordingBrowserTranslate(
          'recordings.delete.executing',
          'Aufnahme wird gelöscht …'
        )
      );
      recordingBrowserRenderPendingDelete(
        recording,
        folderData,
        false,
        false
      );

      recordingBrowserActionRunner({
        mode: 'execute',
        action: 'DELETE',
        payload: recordingBrowserActionPayload(recording, 'DELETE', {
          dryRun: false
        }),
        recording: recording
      })
        .then(result => {
          if (result && result.success === true) {
            recordingBrowserRenderPendingDelete(
              recording,
              folderData,
              true,
              false
            );
            recordingBrowserScheduleDeleteFolderReload();
            return;
          }

          const message = result && (result.message || result.error)
            ? String(result.message || result.error)
            : recordingBrowserTranslate(
                'recordings.delete.backendRejected',
                'Backend hat die Löschung abgelehnt'
              );
          recordingBrowserHandleDeleteFailure(recording, folderData, message);
        })
        .catch(error => {
          recordingBrowserHandleDeleteFailure(
            recording,
            folderData,
            error && error.message ? error.message : String(error)
          );
        });
    });
  });

  return editor;
}

function recordingBrowserCreateRenameValidateButton(recording, resultBox) {
  const button = document.createElement('button');
  button.type = 'button';
  button.textContent = 'Umbenennen prüfen';

  button.addEventListener('click', () => {
    if (!recordingBrowserActionRunner) {
      recordingBrowserRenderActionResult(
        resultBox,
        'Umbenennen prüfen',
        null,
        new Error('Recording action runner is not configured')
      );
      return;
    }

    const trimmedName = recordingBrowserPromptRenameName(recording, 'Umbenennen prüfen', resultBox);

    if (trimmedName === '') {
      return;
    }

    button.disabled = true;
    recordingBrowserRenderActionResult(
      resultBox,
      'Umbenennen prüfen',
      { message: 'Umbenennen wird geprüft …' },
      null
    );

    recordingBrowserActionRunner({
      mode: 'validate',
      action: 'RENAME',
      payload: recordingBrowserActionPayload(recording, 'RENAME', {
        dryRun: true,
        newName: trimmedName
      }),
      recording: recording
    })
      .then(result => {
        recordingBrowserRenderActionResult(resultBox, 'Umbenennen prüfen', result, null);
      })
      .catch(error => {
        recordingBrowserRenderActionResult(resultBox, 'Umbenennen prüfen', null, error);
      })
      .finally(() => {
        button.disabled = false;
      });
  });

  return button;
}

function recordingBrowserCreateRenameButton(recording, folderData, resultBox) {
  const button = document.createElement('button');
  button.type = 'button';
  button.textContent = 'Umbenennen';

  button.addEventListener('click', () => {
    if (!recordingBrowserActionRunner) {
      recordingBrowserRenderActionResult(
        resultBox,
        'Umbenennen',
        null,
        new Error('Recording action runner is not configured')
      );
      return;
    }

    const trimmedName = recordingBrowserPromptRenameName(recording, 'Umbenennen', resultBox);

    if (trimmedName === '') {
      return;
    }

    if (!window.confirm('Aufnahme wirklich umbenennen in "' + trimmedName + '"?')) {
      recordingBrowserRenderActionResult(
        resultBox,
        'Umbenennen',
        { message: 'Umbenennen nicht bestätigt.' },
        null
      );
      return;
    }

    button.disabled = true;
    recordingBrowserRenderOptimisticRenameDetail(
      recording,
      folderData,
      trimmedName,
      false
    );

    recordingBrowserActionRunner({
      mode: 'execute',
      action: 'RENAME',
      payload: recordingBrowserActionPayload(recording, 'RENAME', {
        dryRun: false,
        newName: trimmedName
      }),
      recording: recording
    })
      .then(result => {
        if (result && result.success === true) {
          if (recordingBrowserVisibleFolderPath === null) {
            recordingBrowserRenderOptimisticRenameDetail(
              recording,
              folderData,
              trimmedName,
              true
            );
          } else {
            recordingBrowserConfirmPendingRename(
              folderData,
              trimmedName
            );
          }

          recordingBrowserScheduleRenameFolderReload(
            result,
            folderData,
            trimmedName
          );
          return;
        }

        const message = result && (result.message || result.error)
          ? String(result.message || result.error)
          : 'Backend hat die Umbenennung abgelehnt';

        recordingBrowserClearPendingRename();
        renderServerRecordingDetail(
          recording,
          folderData,
          { renameError: message }
        );
      })
      .catch(error => {
        recordingBrowserClearPendingRename();
        renderServerRecordingDetail(
          recording,
          folderData,
          {
            renameError: error && error.message
              ? error.message
              : String(error)
          }
        );
      });
  });

  return button;
}

function createServerRecordingActionPanel(recording, folderData) {
  const panel = document.createElement('details');
  panel.className = 'recording-action-panel';

  panel.appendChild(recordingBrowserAddText(
    document.createElement('summary'),
    recordingBrowserTranslate(
      'recordings.actions.show',
      'Aktionen anzeigen'
    )
  ));

  panel.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    recordingBrowserTranslate(
      'recordings.actions.description',
      'Umbenennen und Verschieben werden geprüft und ausdrücklich bestätigt. Löschen besitzt zusätzlich einen roten Gefahrenbereich und eine unmittelbar wiederholte Sicherheitsprüfung.'
    )
  ));

  const actions = document.createElement('div');
  actions.className = 'recording-action-buttons';

  const resultBox = document.createElement('div');
  resultBox.className = 'recording-action-result';

  actions.appendChild(recordingBrowserCreateRenameButton(
    recording,
    folderData,
    resultBox
  ));

  actions.appendChild(recordingBrowserCreateRenameValidateButton(
    recording,
    resultBox
  ));

  actions.appendChild(recordingBrowserCreateMoveEditor(
    recording,
    folderData,
    resultBox
  ));

  actions.appendChild(recordingBrowserCreateDeleteEditor(
    recording,
    folderData,
    resultBox
  ));

  panel.appendChild(actions);
  panel.appendChild(resultBox);

  return panel;
}

function renderServerRecordingDetail(recording, folderData, options) {
  recordingBrowserCancelFolderRefreshTimer();
  recordingBrowserVisibleFolderPath = null;

  const detailOptions = options && typeof options === 'object'
    ? options
    : {};
  const renamePending = detailOptions.renamePending === true;
  const renameAccepted = detailOptions.renameAccepted === true;
  const renameError = String(detailOptions.renameError || '').trim();
  const movePending = detailOptions.movePending === true;
  const moveAccepted = detailOptions.moveAccepted === true;
  const moveTimedOut = detailOptions.moveTimedOut === true;
  const moveTargetPath = String(detailOptions.moveTargetPath || '').trim();
  const moveError = String(detailOptions.moveError || '').trim();
  const deletePending = detailOptions.deletePending === true;
  const deleteAccepted = detailOptions.deleteAccepted === true;
  const deleteTimedOut = detailOptions.deleteTimedOut === true;
  const deleteError = String(detailOptions.deleteError || '').trim();

  const list = document.createElement('section');
  list.className = 'list recording-detail-list';

  const item = document.createElement('article');
  item.className = 'module-placeholder recording-detail';

  const title = recordingBrowserLocalRecordingTitle(recording, folderData);
  const recordingId = recordingBrowserFirstValue(recording, ['recordingId', 'id', 'nativeId'], '-');
  const path = recordingBrowserFirstValue(recording, ['path', 'fileName', 'directory'], '-');
  const startTime = recordingBrowserFormatRecordingStart(recordingBrowserFirstValue(recording, ['startTime', 'start', 'date'], '-'));
  const duration = recordingBrowserFormatDurationSeconds(recordingBrowserFirstValue(recording, ['durationSeconds', 'duration'], 0));
  const size = recordingBrowserFormatSizeMb(recordingBrowserFirstValue(recording, ['sizeMb', 'sizeMB', 'size'], 0));

  item.appendChild(recordingBrowserAddText(document.createElement('h3'), String(title)));

  if (renamePending) {
    const pendingMessage = renameAccepted
      ? 'Umbenennung vom Backend bestätigt. Der VDR-Aufnahmecache wird im Hintergrund abgeglichen.'
      : 'Umbenennung wird ausgeführt. Der neue Name wird vorläufig sofort angezeigt.';

    item.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      pendingMessage
    ));

    const progress = document.createElement('progress');
    progress.className = 'recording-folder-refresh-progress';
    progress.setAttribute('aria-label', 'VDR-Aufnahmecache wird abgeglichen');
    item.appendChild(progress);
  }

  if (renameError !== '') {
    item.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      'Umbenennen fehlgeschlagen: ' + renameError
    ));
  }

  if (movePending) {
    const targetLabel = recordingBrowserMoveTargetLabel(moveTargetPath);
    const moveMessage = moveTimedOut
      ? recordingBrowserTranslate(
          'recordings.move.timeout',
          'Verschieben wurde bestätigt, aber der neue Cache-Stand ist noch nicht sichtbar. Ziel: {target}',
          { target: targetLabel }
        )
      : moveAccepted
        ? recordingBrowserTranslate(
            'recordings.move.accepted',
            'Verschieben vom Backend bestätigt. Die Aufnahme wird im Zielordner gesucht: {target}',
            { target: targetLabel }
          )
        : recordingBrowserTranslate(
            'recordings.move.pending',
            'Verschieben wird ausgeführt. Ziel: {target}',
            { target: targetLabel }
          );

    item.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      moveMessage
    ));

    if (!moveTimedOut) {
      const progress = document.createElement('progress');
      progress.className = 'recording-folder-refresh-progress';
      progress.setAttribute(
        'aria-label',
        recordingBrowserTranslate(
          'recordings.move.syncAria',
          'Verschieben wird im Aufnahme-Cache abgeglichen'
        )
      );
      item.appendChild(progress);
    } else {
      const retryButton = document.createElement('button');
      retryButton.type = 'button';
      retryButton.textContent = recordingBrowserTranslate(
        'recordings.move.cacheRetry',
        'Cache-Abgleich erneut versuchen'
      );
      retryButton.addEventListener('click', () => {
        recordingBrowserRetryPendingMoveReadback();
        if (recordingBrowserPendingMove) {
          renderServerRecordingDetail(
            recordingBrowserPendingMove.recording,
            recordingBrowserPendingMove.folderData,
            {
              movePending: true,
              moveAccepted: true,
              moveTimedOut: false,
              moveTargetPath: recordingBrowserPendingMove.targetParameter
            }
          );
          recordingBrowserScheduleMoveTargetFolderReload();
        }
      });
      item.appendChild(retryButton);
    }
  }

  if (moveError !== '') {
    item.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      recordingBrowserTranslate(
        'recordings.move.failed',
        'Verschieben fehlgeschlagen: {message}',
        { message: moveError }
      )
    ));
  }

  if (deletePending) {
    const deleteMessage = deleteTimedOut
      ? recordingBrowserTranslate(
          'recordings.delete.timedOut',
          'Die Löschung wurde bestätigt, ist im Aufnahme-Cache aber noch nicht sichtbar.'
        )
      : deleteAccepted
        ? recordingBrowserTranslate(
            'recordings.delete.accepted',
            'Löschung vom Backend bestätigt. Der VDR-Aufnahmecache wird abgeglichen.'
          )
        : recordingBrowserTranslate(
            'recordings.delete.pending',
            'Löschung wird ausgeführt. Die Aufnahme bleibt sichtbar, bis der Cache das Ergebnis bestätigt.'
          );

    item.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      deleteMessage
    ));

    if (!deleteTimedOut) {
      const progress = document.createElement('progress');
      progress.className = 'recording-folder-refresh-progress';
      progress.setAttribute(
        'aria-label',
        recordingBrowserTranslate(
          'recordings.delete.syncAria',
          'Löschung wird im Aufnahme-Cache abgeglichen'
        )
      );
      item.appendChild(progress);
    } else {
      const retryButton = document.createElement('button');
      retryButton.type = 'button';
      retryButton.textContent = recordingBrowserTranslate(
        'recordings.delete.cacheRetry',
        'Cache-Abgleich erneut versuchen'
      );
      retryButton.addEventListener('click', () => {
        recordingBrowserRetryPendingDeleteReadback();
      });
      item.appendChild(retryButton);
    }
  }

  if (deleteError !== '') {
    item.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      recordingBrowserTranslate(
        'recordings.delete.failed',
        'Löschen fehlgeschlagen: {message}',
        { message: deleteError }
      )
    ));
  }

  item.appendChild(recordingBrowserAddText(document.createElement('p'), 'Aufnahme: ' + startTime));
  item.appendChild(recordingBrowserAddText(document.createElement('p'), 'Dauer: ' + duration));

  const technicalDetails = document.createElement('details');
  technicalDetails.className = 'recording-technical-details';

  technicalDetails.appendChild(recordingBrowserAddText(
    document.createElement('summary'),
    'Technische Details anzeigen'
  ));
  technicalDetails.appendChild(recordingBrowserAddText(
    document.createElement('p'),
    'Größe: ' + size
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

  if (!renamePending && !movePending && !deletePending) {
    item.appendChild(createServerRecordingActionPanel(recording, folderData));
  }

  const backButton = document.createElement('button');
  backButton.type = 'button';

  if (renamePending) {
    backButton.textContent = 'Ordnerabgleich läuft …';
    backButton.disabled = true;
  } else {
    const detailFolderPath = folderData && typeof folderData === 'object'
      ? String(folderData.path || '')
      : '';
    backButton.textContent = deletePending
      ? recordingBrowserTranslate(
          'recordings.delete.backToRecordings',
          'Zurück zu den Aufnahmen'
        )
      : movePending
        ? recordingBrowserTranslate(
            'recordings.move.backToRecordings',
            'Zurück zu den Aufnahmen'
          )
        : 'Zurück zum Ordner';
    backButton.addEventListener('click', () => {
      if (recordingBrowserFolderLoader) {
        recordingBrowserLoadServerFolder(detailFolderPath, 0);
        return;
      }

      renderServerRecordingFolder(folderData);
    });
  }

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
    'Aufnahme: ' + startTime + ' · Dauer: ' + duration + ' · Größe: ' + size
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
  recordingBrowserVisibleFolderPath =
    recordingBrowserNormalizeFolderPath(path);
  const parentPath = String(folderData.parentPath || '');
  const cacheState = String(folderData.cacheState || folderData.state || 'empty');
  const cacheReady = Boolean(folderData.cacheReady);
  const offset = Number(folderData.offset) || 0;
  const limit = Number(folderData.limit) || 50;
  const recordingCount = Number(folderData.recordingCount) || recordings.length;
  const returnedCount = Number(folderData.returnedCount) || recordings.length;
  const totalCount = Number(folderData.totalCount) || 0;
  const externalRefreshPending = folderData.refreshPending === true;
  const actionNotice = String(folderData.actionNotice || '').trim();

  let pendingRename = recordingBrowserPendingRenameForFolder(path);

  if (pendingRename &&
      recordingBrowserFolderContainsRenamedRecording(
        folderData,
        pendingRename.newName
      )) {
    recordingBrowserClearPendingRename();
    pendingRename = null;
  }

  const renameCachePending = pendingRename !== null;
  let pendingMove = recordingBrowserPendingMove;

  if (pendingMove &&
      recordingBrowserFolderContainsMovedRecording(folderData, pendingMove)) {
    recordingBrowserClearPendingMove();
    pendingMove = null;
  }

  const moveCachePending = pendingMove !== null;
  const folderRefreshPending =
    renameCachePending || moveCachePending || externalRefreshPending;

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

  if (actionNotice !== '') {
    header.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      actionNotice
    ));
  }

  if (!cacheReady) {
    header.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      'Recording-Cache wird vom Daemon im Hintergrund gefüllt. Diese Ansicht aktualisiert sich automatisch.'
    ));
  }

  if (folderRefreshPending) {
    let pendingMessage =
      'Aufnahmeordner wird im Hintergrund aktualisiert.';
    let progressLabel = 'Aufnahmeordner wird aktualisiert';

    if (renameCachePending) {
      pendingMessage = pendingRename.accepted
        ? 'Umbenennung bestätigt. Ordner und Aufnahmen werden neu geladen.'
        : 'Umbenennung wird ausgeführt. Die Ordneransicht aktualisiert sich anschließend automatisch.';
      progressLabel =
        'Aufnahmeordner wird nach der Umbenennung aktualisiert';
    } else if (moveCachePending) {
      pendingMessage = pendingMove.timedOut
        ? recordingBrowserTranslate(
            'recordings.move.folderTimeout',
            'Verschieben bestätigt. Der Cache-Abgleich dauert länger als erwartet. Ziel: {target}',
            { target: pendingMove.targetLabel }
          )
        : pendingMove.accepted
          ? recordingBrowserTranslate(
              'recordings.move.folderAccepted',
              'Verschieben bestätigt. Die Aufnahme wird im Zielordner gesucht: {target}',
              { target: pendingMove.targetLabel }
            )
          : recordingBrowserTranslate(
              'recordings.move.pending',
              'Verschieben wird ausgeführt. Ziel: {target}',
              { target: pendingMove.targetLabel }
            );
      progressLabel = recordingBrowserTranslate(
        'recordings.move.syncAria',
        'Verschieben wird im Aufnahme-Cache abgeglichen'
      );
    }

    header.appendChild(recordingBrowserAddText(
      document.createElement('p'),
      pendingMessage
    ));

    if (!moveCachePending || !pendingMove.timedOut) {
      const progress = document.createElement('progress');
      progress.className = 'recording-folder-refresh-progress';
      progress.setAttribute('aria-label', progressLabel);
      header.appendChild(progress);
    } else {
      const retryButton = document.createElement('button');
      retryButton.type = 'button';
      retryButton.textContent = recordingBrowserTranslate(
        'recordings.move.cacheRetry',
        'Cache-Abgleich erneut versuchen'
      );
      retryButton.addEventListener('click', () => {
        recordingBrowserRetryPendingMoveReadback();
        recordingBrowserLoadServerFolder(path, offset);
      });
      header.appendChild(retryButton);
    }
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
    const folderMetaText = folderCount === 1
      ? '1 Aufnahme · antippen zum direkt Öffnen'
      : String(folderCount) + ' Aufnahme(n) · antippen zum Öffnen';

    item.appendChild(recordingBrowserAddText(
      document.createElement('div'),
      folderMetaText
    )).className = 'list-meta';

    const open = () => recordingBrowserOpenServerFolder(folderPath, folderData);
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
      folderRefreshPending
        ? renameCachePending
          ? 'Ordner und Aufnahmen werden nach der Umbenennung noch nachgeladen.'
          : moveCachePending
            ? 'Ordner und Aufnahmen werden nach dem Verschieben noch nachgeladen.'
            : 'Ordner und Aufnahmen werden noch nachgeladen.'
        : cacheReady
          ? 'Dieser Ordner enthält keine Unterordner und keine direkten Aufnahmen.'
          : 'Noch keine Cache-Daten vorhanden. Der Daemon lädt die Aufnahmen im Hintergrund.'
    ));
    list.appendChild(empty);
  }

  recordingBrowserDetailDataElement().replaceChildren(list);

  if (moveCachePending &&
      pendingMove.accepted &&
      !pendingMove.timedOut &&
      recordingBrowserFolderLoader) {
    recordingBrowserScheduleMoveTargetFolderReload();
  } else if (renameCachePending &&
      pendingRename.accepted &&
      recordingBrowserFolderLoader) {
    recordingBrowserScheduleRenameFolderReload(
      { success: true },
      { path: path },
      pendingRename.newName
    );
  } else if (!cacheReady && recordingBrowserFolderLoader) {
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
      'Aufnahme: ' + startTime + ' · Dauer: ' + duration + ' · Größe: ' + size
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
  renderNode: renderRecordingNode,
  validateNewMoveFolderName: recordingBrowserValidateNewMoveFolderName,
  joinMoveFolderPath: recordingBrowserJoinMoveFolderPath,
  sameRecording: recordingBrowserSameRecording,
  folderContainsRecording: recordingBrowserFolderContainsRecording
});

window.VdrSuiteRecordingBrowser = recordingBrowserApi;

if (window.VdrSuitePlatform &&
    typeof window.VdrSuitePlatform.registerModule === 'function' &&
    !window.VdrSuitePlatform.hasModule('recordings')) {
  window.VdrSuitePlatform.registerModule('recordings', recordingBrowserApi);
}
