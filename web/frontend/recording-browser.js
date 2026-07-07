"use strict";

// Phase 59.10b: extracted Recording browser runtime from index.html.
// HTTP ownership remains in window.VdrSuiteClientApi through app.js.

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
