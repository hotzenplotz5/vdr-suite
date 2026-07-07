"use strict";

// Phase 59.10f: Recording browser owns the rich Recording renderer.
// HTTP ownership remains outside this file.

let recordingSortMode = 'name';
let recordingViewMode = 'folder';
let currentRecordingRecords = [];

function decodeRecordingText(value) {
  return String(value || '')
    .replace(/#([0-9A-Fa-f]{2})/g, (_, hex) => String.fromCharCode(parseInt(hex, 16)))
    .replace(/_/g, ' ')
    .trim();
}

function recordingSortKey(value) {
  return decodeRecordingText(value).toLocaleLowerCase('de-DE');
}

function recordingTimestamp(entry) {
  const recording = entry.recording || entry;
  const value = firstValue(recording, ['startTime', 'start', 'date'], '');
  const number = Number(value);

  if (Number.isFinite(number) && number > 0) {
    return number;
  }

  return 0;
}

const originalRecordingDisplayParts = recordingDisplayParts;
recordingDisplayParts = function(recording, index) {
  const display = originalRecordingDisplayParts(recording, index);
  return {
    folder: decodeRecordingText(display.folder),
    title: decodeRecordingText(display.title)
  };
};

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
    const display = recordingDisplayParts(recording, index);
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
  const display = recordingDisplayParts(recording, index);
  const parts = display.folder.split('/').filter(Boolean);
  return parts.length > 0 ? parts[0] : 'Ohne Genre';
}

function buildRecordingGenreTree(recordings) {
  const root = createRecordingNode('Aufnahme-Genres', null);

  recordings.forEach((recording, index) => {
    const display = recordingDisplayParts(recording, index);
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
  header.appendChild(addText(document.createElement('h3'), node.name));
  header.appendChild(addText(
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
    item.appendChild(addText(document.createElement('div'), child.name)).className = 'list-title';
    item.appendChild(addText(document.createElement('div'), countRecordingNode(child) + ' Aufnahme(n)')).className = 'list-meta';
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
    item.appendChild(addText(document.createElement('div'), entry.title)).className = 'list-title';
    item.appendChild(addText(document.createElement('div'), pathForRecordingNode(node).join(' / ') || 'Hauptordner')).className = 'list-meta';
    container.appendChild(item);
  });

  detailDataElement.replaceChildren(container);
}

const RECORDING_FOLDER_BATCH_SIZE = 80;
const RECORDING_ITEM_PAGE_SIZE = 20;

function renderRecordingList(data) {
  const recordings = listFromResponse(data, 'recordings');
  detailDataElement.replaceChildren();

  if (recordings.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(addText(document.createElement('h3'), 'Keine Aufnahmen gefunden'));
    empty.appendChild(addText(document.createElement('p'), 'Der Recording-Query-Endpunkt hat aktuell keine Aufnahmen geliefert.'));
    detailDataElement.appendChild(empty);
    return;
  }

  const reportedTotal = Number(firstValue(data || {}, ['totalCount', 'total', 'count'], recordings.length));
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
      const display = recordingDisplayParts(recording, index);
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
    pager.appendChild(addText(
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
    detailDataElement.replaceChildren();

    const list = document.createElement('section');
    list.className = 'list recording-detail-list';

    const item = document.createElement('article');
    item.className = 'module-placeholder recording-detail';

    const recordingId = firstValue(recording, ['recordingId', 'id', 'nativeId'], '-');
    const path = firstValue(recording, ['path', 'fileName', 'directory'], '-');
    const startTime = formatRecordingStart(firstValue(recording, ['startTime', 'start', 'date'], '-'));
    const duration = formatDurationSeconds(firstValue(recording, ['durationSeconds', 'duration'], 0));
    const size = formatSizeMb(firstValue(recording, ['sizeMb', 'sizeMB', 'size'], 0));
    const channel = firstValue(recording, ['channelName', 'channel', 'channelId'], '-');
    const description = firstValue(recording, ['description', 'summary', 'shortText'], '');

    item.appendChild(addText(document.createElement('h3'), entry.title));
    item.appendChild(addText(document.createElement('p'), 'Start: ' + startTime));
    item.appendChild(addText(document.createElement('p'), 'Dauer: ' + duration));
    item.appendChild(addText(document.createElement('p'), 'Größe: ' + size));
    item.appendChild(addText(document.createElement('p'), 'Sender: ' + String(channel)));
    item.appendChild(addText(document.createElement('p'), 'Pfad: ' + String(path)));
    item.appendChild(addText(document.createElement('p'), 'ID: ' + String(recordingId)));

    if (String(description).trim()) {
      item.appendChild(addText(document.createElement('p'), String(description)));
    }

    const backButton = document.createElement('button');
    backButton.type = 'button';
    backButton.textContent = 'Zurück zur Liste';
    backButton.addEventListener('click', () => {
      renderFolderNode(node, visibleFolderCount, recordingPageIndex);
    });
    item.appendChild(backButton);

    list.appendChild(item);
    detailDataElement.appendChild(list);
  }

  function createRecordingListItem(entry, openDetail) {
    const recording = entry.recording;
    const item = document.createElement('article');
    item.className = 'list-item recording-list-item';
    item.tabIndex = 0;
    item.setAttribute('role', 'button');
    item.setAttribute('aria-label', 'Aufnahme ' + entry.title + ' öffnen');

    const recordingId = firstValue(recording, ['recordingId', 'id', 'nativeId'], '-');
    const path = firstValue(recording, ['path', 'fileName', 'directory'], '-');
    const startTime = formatRecordingStart(firstValue(recording, ['startTime', 'start', 'date'], '-'));
    const duration = formatDurationSeconds(firstValue(recording, ['durationSeconds', 'duration'], 0));
    const size = formatSizeMb(firstValue(recording, ['sizeMb', 'sizeMB', 'size'], 0));

    item.appendChild(addText(document.createElement('div'), entry.title)).className = 'list-title';
    item.appendChild(addText(
      document.createElement('div'),
      'Start: ' + startTime + ' · Dauer: ' + duration + ' · Größe: ' + size + ' · antippen für Details'
    )).className = 'list-meta';
    item.appendChild(addText(document.createElement('div'), 'Pfad: ' + String(path))).className = 'list-meta';
    item.appendChild(addText(document.createElement('div'), 'ID: ' + String(recordingId))).className = 'list-meta';

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

    detailDataElement.replaceChildren();

    const list = document.createElement('section');
    list.className = 'list recording-folder-list';

    const header = document.createElement('article');
    header.className = 'module-placeholder';
    header.appendChild(addText(document.createElement('h3'), recordingFolderLabel(node)));

    const summary = [
      String(displayChildFolders.length) + ' Unterordner',
      String(recordingEntries.length) + ' Aufnahme(n) in dieser Ebene',
      String(node.totalRecordings) + ' Aufnahme(n) insgesamt',
      String(totalRecordings) + ' Aufnahme(n) im Katalog'
    ];

    if (recordingEntries.length > 0) {
      summary.push('Seite ' + String(recordingPageIndex + 1) + ' von ' + String(recordingPageCount));
    }

    header.appendChild(addText(document.createElement('p'), summary.join(' · ')));

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

      item.appendChild(addText(document.createElement('div'), folderNode.name)).className = 'list-title';
      item.appendChild(addText(
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
      empty.appendChild(addText(document.createElement('p'), 'Dieser Ordner enthält keine Aufnahmen.'));
      list.appendChild(empty);
    }

    detailDataElement.appendChild(list);
  }

  const rootNode = buildRecordingFolderTree(recordings);
  renderFolderNode(rootNode, RECORDING_FOLDER_BATCH_SIZE, 0);
}

function setRecordingBrowserRecords(records) {
  currentRecordingRecords = Array.isArray(records) ? records.slice() : [];
}

window.VdrSuiteRecordingBrowser = Object.freeze({
  decodeRecordingText: decodeRecordingText,
  setRecords: setRecordingBrowserRecords,
  renderList: renderRecordingList,
  renderRoot: renderRecordingRoot,
  renderNode: renderRecordingNode
});
