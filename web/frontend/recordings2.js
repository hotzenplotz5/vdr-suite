(function (global) {
  'use strict';

  const MODULE_NAME = 'recordings2';
  const PAGE_SIZE = 50;
  let active = false;
  let requestSequence = 0;
  let currentPath = '';

  function mountTarget() {
    return document.getElementById('detail-data');
  }

  function addText(parent, tagName, text, className) {
    const element = document.createElement(tagName);
    element.textContent = String(text || '');
    if (className) element.className = className;
    parent.appendChild(element);
    return element;
  }

  function firstValue(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
        return object[key];
      }
    }
    return fallback;
  }

  function listFromResponse(data, key) {
    if (data && Array.isArray(data[key])) return data[key];
    if (data && Array.isArray(data.items) && key === 'recordings') return data.items;
    return [];
  }

  function decodeVdrText(value) {
    let decoded = String(value || '');
    for (let pass = 0; pass < 8; pass += 1) {
      const next = decoded.replace(/#([0-9A-Fa-f]{2})/g, function (_, hex) {
        return String.fromCharCode(parseInt(hex, 16));
      });
      if (next === decoded) break;
      decoded = next;
    }
    return decoded.replace(/_/g, ' ').trim();
  }

  function normalizePath(value) {
    return String(value || '')
      .split('/')
      .map(function (part) { return part.trim(); })
      .filter(Boolean)
      .join('/');
  }

  function joinPath(parent, child) {
    const left = normalizePath(parent);
    const right = normalizePath(child);
    return left && right ? left + '/' + right : left || right;
  }

  function parentPath(path) {
    const parts = normalizePath(path).split('/').filter(Boolean);
    parts.pop();
    return parts.join('/');
  }

  function selectedBackendId() {
    if (global.VdrSuitePlatform && typeof global.VdrSuitePlatform.getSelectedBackendId === 'function') {
      return String(global.VdrSuitePlatform.getSelectedBackendId() || 'default');
    }
    return 'default';
  }

  function fetchFolder(path, offset) {
    const query = {
      backend: selectedBackendId(),
      path: normalizePath(path),
      limit: PAGE_SIZE,
      offset: Number(offset) || 0,
      _: String(Date.now())
    };

    const clientApi = global.VdrSuiteClientApi;
    if (clientApi && typeof clientApi.fetchClientRecordingFolder === 'function') {
      return clientApi.fetchClientRecordingFolder({
        query: query,
        cache: 'no-store',
        credentials: 'same-origin'
      });
    }

    const url = '/api/vdr/recordings/folder?' + new URLSearchParams(query).toString();
    return global.fetch(url, {
      cache: 'no-store',
      credentials: 'same-origin',
      headers: { Accept: 'application/json' }
    }).then(function (response) {
      if (!response.ok) throw new Error('HTTP ' + response.status);
      return response.json();
    });
  }

  function activateTab() {
    document.querySelectorAll('.module-tab').forEach(function (tab) {
      tab.classList.toggle('active', tab.dataset.module === MODULE_NAME);
    });
  }

  function shell(title, subtitle) {
    const section = document.createElement('section');
    section.className = 'list recordings2-browser';
    const header = document.createElement('article');
    header.className = 'module-placeholder';
    addText(header, 'h3', title);
    if (subtitle) addText(header, 'p', subtitle);
    section.appendChild(header);
    return section;
  }

  function showLoading(path) {
    const target = mountTarget();
    if (!target) return;
    const section = shell('Recordings 2 lädt …', path ? 'Aufnahme-Ordner / ' + decodeVdrText(path) : 'Aufnahme-Ordner');
    const progress = document.createElement('progress');
    progress.setAttribute('aria-label', 'Recordings 2 lädt Aufnahmeordner');
    section.firstChild.appendChild(progress);
    target.replaceChildren(section);
  }

  function showError(error, path) {
    const target = mountTarget();
    if (!target) return;
    const section = shell('Recordings 2: Ordner konnte nicht geladen werden', path || 'Hauptordner');
    addText(section.firstChild, 'p', error && error.message ? error.message : String(error));
    const retry = document.createElement('button');
    retry.type = 'button';
    retry.textContent = 'Erneut laden';
    retry.addEventListener('click', function () { loadFolder(path, 0); });
    section.firstChild.appendChild(retry);
    target.replaceChildren(section);
  }

  function folderName(folder) {
    const raw = firstValue(folder, ['displayName', 'name', 'title', 'path'], 'Ordner');
    const parts = normalizePath(raw).split('/').filter(Boolean);
    return decodeVdrText(parts.length ? parts[parts.length - 1] : raw);
  }

  function folderPath(folder, basePath) {
    const explicit = normalizePath(firstValue(folder, ['path', 'folderPath'], ''));
    return explicit || joinPath(basePath, firstValue(folder, ['name', 'displayName', 'title'], ''));
  }

  function recordingTitle(recording) {
    const raw = firstValue(recording, ['title', 'name', 'displayName', 'file'], 'Aufnahme');
    const decoded = decodeVdrText(raw);
    const parts = decoded.split('/').filter(Boolean);
    return parts.length ? parts[parts.length - 1] : decoded;
  }

  function recordingMeta(recording) {
    const values = [];
    const start = firstValue(recording, ['startTime', 'start', 'date'], '');
    const duration = Number(firstValue(recording, ['durationSeconds', 'duration'], 0));
    if (start) {
      const epoch = Number(start);
      values.push(Number.isFinite(epoch) && epoch > 1000000000
        ? new Date(epoch * 1000).toLocaleString('de-DE')
        : String(start));
    }
    if (Number.isFinite(duration) && duration > 0) values.push(Math.round(duration / 60) + ' min');
    return values.join(' · ');
  }

  function renderRecordingDetail(recording, folderData) {
    const target = mountTarget();
    if (!target) return;
    const section = shell(recordingTitle(recording), recordingMeta(recording));
    const detail = document.createElement('article');
    detail.className = 'module-placeholder recording-detail';
    const description = firstValue(recording, ['description', 'shortText', 'summary', 'subtitle'], 'Keine Beschreibung vorhanden.');
    addText(detail, 'p', decodeVdrText(description));
    const path = firstValue(recording, ['path', 'fileName', 'directory'], '');
    if (path) addText(detail, 'p', String(path));
    const back = document.createElement('button');
    back.type = 'button';
    back.textContent = 'Zurück zum Ordner';
    back.addEventListener('click', function () { renderFolder(folderData); });
    detail.appendChild(back);
    section.appendChild(detail);
    target.replaceChildren(section);
  }

  function renderFolder(data) {
    if (!active) return;
    const target = mountTarget();
    if (!target) return;

    const path = normalizePath(firstValue(data, ['path'], currentPath));
    currentPath = path;
    const folders = listFromResponse(data, 'folders');
    const recordings = listFromResponse(data, 'recordings');
    const title = path ? decodeVdrText(path.split('/').pop()) : 'Recordings 2';
    const subtitle = (folders.length + recordings.length) + ' Einträge · ' + (path ? 'Aufnahme-Ordner / ' + decodeVdrText(path) : 'Aufnahme-Ordner');
    const section = shell(title, subtitle);

    if (path) {
      const backRow = document.createElement('article');
      backRow.className = 'module-placeholder';
      const back = document.createElement('button');
      back.type = 'button';
      back.textContent = 'Eine Ebene zurück';
      back.addEventListener('click', function () { loadFolder(parentPath(path), 0); });
      backRow.appendChild(back);
      section.appendChild(backRow);
    }

    folders.forEach(function (folder) {
      const row = document.createElement('article');
      row.className = 'list-item recordings2-folder';
      row.tabIndex = 0;
      row.setAttribute('role', 'button');
      addText(row, 'div', '📁 ' + folderName(folder), 'list-title');
      const count = Number(firstValue(folder, ['recordingCount', 'count'], 0));
      addText(row, 'div', count > 0 ? count + ' Aufnahme(n)' : 'Ordner öffnen', 'list-meta');
      const open = function () { loadFolder(folderPath(folder, path), 0); };
      row.addEventListener('click', open);
      row.addEventListener('keydown', function (event) {
        if (event.key === 'Enter' || event.key === ' ') {
          event.preventDefault();
          open();
        }
      });
      section.appendChild(row);
    });

    recordings.forEach(function (recording) {
      const row = document.createElement('article');
      row.className = 'list-item recording-list-item recordings2-recording';
      row.tabIndex = 0;
      row.setAttribute('role', 'button');
      addText(row, 'div', recordingTitle(recording), 'list-title');
      addText(row, 'div', recordingMeta(recording) || 'Aufnahme', 'list-meta');
      const open = function () { renderRecordingDetail(recording, data); };
      row.addEventListener('click', open);
      row.addEventListener('keydown', function (event) {
        if (event.key === 'Enter' || event.key === ' ') {
          event.preventDefault();
          open();
        }
      });
      section.appendChild(row);
    });

    if (folders.length === 0 && recordings.length === 0) {
      const empty = document.createElement('article');
      empty.className = 'module-placeholder';
      addText(empty, 'p', 'Dieser Aufnahmeordner ist leer.');
      section.appendChild(empty);
    }

    target.replaceChildren(section);
  }

  function loadFolder(path, offset) {
    active = true;
    activateTab();
    currentPath = normalizePath(path);
    const sequence = ++requestSequence;
    showLoading(currentPath);
    fetchFolder(currentPath, offset).then(function (data) {
      if (!active || sequence !== requestSequence) return;
      renderFolder(data && typeof data === 'object' ? data : {});
    }).catch(function (error) {
      if (!active || sequence !== requestSequence) return;
      showError(error, currentPath);
    });
  }

  function injectTab() {
    if (document.querySelector('.module-tab[data-module="' + MODULE_NAME + '"]')) return;
    const oldRecordings = document.querySelector('.module-tab[data-module="recordings"]');
    const navigation = oldRecordings && oldRecordings.parentElement;
    if (!navigation) return;
    const tab = document.createElement('button');
    tab.type = 'button';
    tab.className = 'module-tab';
    tab.dataset.module = MODULE_NAME;
    tab.textContent = 'Recordings 2';
    oldRecordings.insertAdjacentElement('afterend', tab);
  }

  document.addEventListener('click', function (event) {
    const tab = event.target && event.target.closest
      ? event.target.closest('.module-tab[data-module="' + MODULE_NAME + '"]')
      : null;
    if (!tab) return;
    event.preventDefault();
    event.stopPropagation();
    event.stopImmediatePropagation();
    loadFolder('', 0);
  }, true);

  document.addEventListener('click', function (event) {
    const otherTab = event.target && event.target.closest ? event.target.closest('.module-tab') : null;
    if (otherTab && otherTab.dataset.module !== MODULE_NAME) active = false;
  }, true);

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', injectTab, { once: true });
  } else {
    injectTab();
  }

  global.VdrSuiteRecordings2 = Object.freeze({
    open: function () { loadFolder('', 0); },
    reload: function () { loadFolder(currentPath, 0); }
  });
})(window);
