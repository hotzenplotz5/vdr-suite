const statusElement = document.getElementById('status');
const backendsElement = document.getElementById('backends');
const detailMetaElement = document.getElementById('detail-meta');
const detailDataElement = document.getElementById('detail-data');
const refreshDetailButton = document.getElementById('refresh-detail');
let selectedBackendId = '';
let selectedBackend = null;
let selectedModule = 'overview';
let currentSnapshot = null;
let currentChannels = null;
let currentTimers = null;

const moduleLabels = {
  overview: 'Übersicht',
  channels: 'Kanäle',
  timers: 'Timer',
  recordings: 'Aufnahmen',
  searchtimers: 'SearchTimer'
};

function addText(element, text) {
  element.textContent = text;
  return element;
}

function createBadge(label, enabled) {
  const badge = document.createElement('span');
  badge.className = 'badge ' + (enabled ? 'enabled' : 'disabled');
  badge.textContent = label + ': ' + (enabled ? 'ja' : 'nein');
  return badge;
}

function valueOrZero(value) {
  return Number.isFinite(Number(value)) ? Number(value) : 0;
}

function createMetric(label, value) {
  const card = document.createElement('article');
  card.className = 'metric-card';
  const valueElement = addText(document.createElement('div'), String(value));
  valueElement.className = 'metric-value';
  const labelElement = addText(document.createElement('div'), label);
  labelElement.className = 'metric-label';
  card.appendChild(valueElement);
  card.appendChild(labelElement);
  return card;
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

function renderSnapshotMetrics(data) {
  detailDataElement.replaceChildren();
  detailDataElement.appendChild(createMetric('Snapshot', data.snapshotAvailable ? 'ja' : 'nein'));
  detailDataElement.appendChild(createMetric('Kanäle', valueOrZero(data.channelCount)));
  detailDataElement.appendChild(createMetric('Events', valueOrZero(data.eventCount)));
  detailDataElement.appendChild(createMetric('Timer', valueOrZero(data.timerCount)));
  detailDataElement.appendChild(createMetric('Aufnahmen', valueOrZero(data.recordingCount)));
}

function renderChannelList(data) {
  const channels = listFromResponse(data, 'channels');
  detailDataElement.replaceChildren();

  const list = document.createElement('section');
  list.className = 'list';

  if (channels.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(addText(document.createElement('h3'), 'Keine Kanäle gefunden'));
    empty.appendChild(addText(document.createElement('p'), 'Der Endpunkt /api/vdr/channels hat keine Kanalliste geliefert.'));
    detailDataElement.appendChild(empty);
    return;
  }

  channels.slice(0, 20).forEach((channel, index) => {
    const item = document.createElement('article');
    item.className = 'list-item';
    const title = firstValue(
      channel,
      ['name', 'channelName', 'title', 'displayName', 'id', 'channelId'],
      'Kanal ' + String(index + 1)
    );
    const channelId = firstValue(channel, ['channelId', 'id', 'nativeId'], '-');
    const number = firstValue(channel, ['number', 'channelNumber', 'position'], String(index + 1));
    item.appendChild(addText(document.createElement('div'), String(title))).className = 'list-title';
    item.appendChild(addText(
      document.createElement('div'),
      'Nummer: ' + String(number) + ' · ID: ' + String(channelId)
    )).className = 'list-meta';
    list.appendChild(item);
  });

  if (channels.length > 20) {
    const info = document.createElement('article');
    info.className = 'module-placeholder';
    info.appendChild(addText(document.createElement('p'), 'Zeige 20 von ' + String(channels.length) + ' Kanälen.'));
    list.appendChild(info);
  }

  detailDataElement.appendChild(list);
}

function renderTimerList(data) {
  const timers = listFromResponse(data, 'timers');
  detailDataElement.replaceChildren();

  const list = document.createElement('section');
  list.className = 'list';

  if (timers.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(addText(document.createElement('h3'), 'Keine Timer gefunden'));
    empty.appendChild(addText(document.createElement('p'), 'Der Endpunkt /api/vdr/timers hat aktuell keine Timer geliefert.'));
    detailDataElement.appendChild(empty);
    return;
  }

  timers.slice(0, 20).forEach((timer, index) => {
    const item = document.createElement('article');
    item.className = 'list-item';
    const title = firstValue(
      timer,
      ['title', 'name', 'file', 'eventTitle', 'description', 'id', 'timerId'],
      'Timer ' + String(index + 1)
    );
    const timerId = firstValue(timer, ['timerId', 'id', 'nativeId'], '-');
    const channel = firstValue(timer, ['channelName', 'channel', 'channelId'], '-');
    const state = firstValue(timer, ['state', 'status', 'enabled', 'active'], '-');
    const start = firstValue(timer, ['startTime', 'start', 'begin'], '-');
    const stop = firstValue(timer, ['stopTime', 'stop', 'end'], '-');

    item.appendChild(addText(document.createElement('div'), String(title))).className = 'list-title';
    item.appendChild(addText(
      document.createElement('div'),
      'ID: ' + String(timerId) + ' · Kanal: ' + String(channel) + ' · Status: ' + String(state)
    )).className = 'list-meta';
    item.appendChild(addText(
      document.createElement('div'),
      'Start: ' + String(start) + ' · Ende: ' + String(stop)
    )).className = 'list-meta';
    list.appendChild(item);
  });

  if (timers.length > 20) {
    const info = document.createElement('article');
    info.className = 'module-placeholder';
    info.appendChild(addText(document.createElement('p'), 'Zeige 20 von ' + String(timers.length) + ' Timern.'));
    list.appendChild(info);
  }

  detailDataElement.appendChild(list);
}

function renderModulePlaceholder(moduleName, data) {
  const countMap = {
    recordings: valueOrZero(data.recordingCount),
    searchtimers: 0
  };
  const endpointMap = {
    recordings: '/api/vdr/recordings',
    searchtimers: '/api/searchtimers'
  };
  detailDataElement.replaceChildren();
  const box = document.createElement('section');
  box.className = 'module-placeholder';
  box.appendChild(addText(document.createElement('h3'), moduleLabels[moduleName] || moduleName));
  box.appendChild(addText(
    document.createElement('p'),
    'Modul vorbereitet. Aktueller Snapshot-Zähler: ' + String(countMap[moduleName] || 0) + '. Nächster Schritt: Liste aus ' + endpointMap[moduleName] + ' rendern.'
  ));
  detailDataElement.appendChild(box);
}

function renderModuleError(title, error) {
  detailDataElement.replaceChildren();
  const box = document.createElement('section');
  box.className = 'module-placeholder error';
  box.appendChild(addText(document.createElement('h3'), title));
  box.appendChild(addText(document.createElement('p'), error.message));
  detailDataElement.appendChild(box);
}

function renderModuleLoading(title, message) {
  detailDataElement.replaceChildren();
  const loading = document.createElement('section');
  loading.className = 'module-placeholder';
  loading.appendChild(addText(document.createElement('h3'), title));
  loading.appendChild(addText(document.createElement('p'), message));
  detailDataElement.appendChild(loading);
}

function loadChannels() {
  renderModuleLoading('Kanäle', 'Lade Kanalliste aus /api/vdr/channels...');

  fetch('/api/vdr/channels')
    .then(response => {
      if (!response.ok) {
        throw new Error('HTTP ' + response.status);
      }
      return response.json();
    })
    .then(data => {
      currentChannels = data;
      renderChannelList(data);
    })
    .catch(error => {
      currentChannels = null;
      renderModuleError('Kanäle konnten nicht geladen werden', error);
    });
}

function loadTimers() {
  renderModuleLoading('Timer', 'Lade Timerliste aus /api/vdr/timers...');

  fetch('/api/vdr/timers')
    .then(response => {
      if (!response.ok) {
        throw new Error('HTTP ' + response.status);
      }
      return response.json();
    })
    .then(data => {
      currentTimers = data;
      renderTimerList(data);
    })
    .catch(error => {
      currentTimers = null;
      renderModuleError('Timer konnten nicht geladen werden', error);
    });
}

function renderSelectedModule(data) {
  if (selectedModule === 'overview') {
    renderSnapshotMetrics(data);
    return;
  }

  if (selectedModule === 'channels') {
    loadChannels();
    return;
  }

  if (selectedModule === 'timers') {
    loadTimers();
    return;
  }

  renderModulePlaceholder(selectedModule, data);
}

function selectModule(moduleName) {
  selectedModule = moduleName;
  document.querySelectorAll('.module-tab').forEach(button => {
    button.classList.toggle('active', button.dataset.module === moduleName);
  });

  if (currentSnapshot) {
    renderSelectedModule(currentSnapshot);
  }
}

function markSelected(backendId) {
  selectedBackendId = backendId;
  document.querySelectorAll('.backend-card').forEach(card => {
    card.classList.toggle('selected', card.dataset.backendId === backendId);
  });
}

function loadBackendDetails(backend) {
  selectedBackend = backend;
  selectedModule = 'overview';
  selectModule('overview');
  const selector = backend.frontendSelector || backend;
  const backendId = selector.id || backend.backendId || 'default';
  markSelected(backendId);
  refreshDetailButton.disabled = true;
  detailMetaElement.className = 'detail-meta';
  detailMetaElement.textContent = 'Lade Details für ' + (selector.label || backend.backendName || backendId) + '...';
  detailDataElement.replaceChildren();

  fetch('/api/backends/' + encodeURIComponent(backendId) + '/snapshot')
    .then(response => {
      if (!response.ok) {
        throw new Error('HTTP ' + response.status);
      }
      return response.json();
    })
    .then(data => {
      currentSnapshot = data;
      detailMetaElement.textContent = 'Details für ' + (selector.label || backend.backendName || backendId);
      renderSelectedModule(data);
      refreshDetailButton.disabled = false;
    })
    .catch(error => {
      currentSnapshot = null;
      detailMetaElement.className = 'detail-meta error';
      detailMetaElement.textContent = 'Details konnten nicht geladen werden: ' + error.message;
      detailDataElement.replaceChildren();
      refreshDetailButton.disabled = false;
    });
}

function renderBackend(backend) {
  const selector = backend.frontendSelector || backend;
  const backendId = selector.id || backend.backendId || 'default';
  const card = document.createElement('article');
  card.className = 'backend-card';
  card.dataset.backendId = backendId;
  card.tabIndex = 0;
  card.setAttribute('role', 'button');
  card.setAttribute('aria-label', 'Backend ' + (selector.label || backend.backendName || backendId) + ' auswählen');

  const header = document.createElement('div');
  header.className = 'backend-header';

  const titleBlock = document.createElement('div');
  const title = addText(document.createElement('div'), selector.label || backend.backendName || backendId || 'Unbekanntes Backend');
  title.className = 'backend-title';
  const subtitle = addText(
    document.createElement('div'),
    'ID: ' + backendId + ' · Zugriff: ' + (selector.accessMode || backend.accessMode || '-')
  );
  subtitle.className = 'backend-subtitle';
  titleBlock.appendChild(title);
  titleBlock.appendChild(subtitle);

  const status = addText(document.createElement('div'), backend.online ? 'online' : 'offline');
  status.className = 'status-pill' + (backend.online ? '' : ' offline');

  header.appendChild(titleBlock);
  header.appendChild(status);
  card.appendChild(header);

  const badges = document.createElement('div');
  badges.className = 'badge-row';
  badges.appendChild(createBadge('Schreiben', Boolean(selector.canWrite)));
  badges.appendChild(createBadge('Aufnahmen', Boolean(selector.canWriteRecordings)));
  badges.appendChild(createBadge('Timer', Boolean(selector.canWriteTimers)));
  badges.appendChild(createBadge('SearchTimer', Boolean(selector.canWriteSearchTimers)));
  card.appendChild(badges);

  card.addEventListener('click', () => loadBackendDetails(backend));
  card.addEventListener('keydown', event => {
    if (event.key === 'Enter' || event.key === ' ') {
      event.preventDefault();
      loadBackendDetails(backend);
    }
  });

  return card;
}

document.querySelectorAll('.module-tab').forEach(button => {
  button.addEventListener('click', () => selectModule(button.dataset.module));
});

refreshDetailButton.addEventListener('click', () => {
  if (!selectedBackend) {
    return;
  }

  if (selectedModule === 'channels') {
    loadChannels();
    return;
  }

  if (selectedModule === 'timers') {
    loadTimers();
    return;
  }

  loadBackendDetails(selectedBackend);
});

fetch('/api/backends')
  .then(response => {
    if (!response.ok) {
      throw new Error('HTTP ' + response.status);
    }
    return response.json();
  })
  .then(data => {
    const backends = Array.isArray(data.backends) ? data.backends : [];
    statusElement.textContent = backends.length + ' Backend(s) gefunden';
    backendsElement.replaceChildren();
    backends.forEach(backend => backendsElement.appendChild(renderBackend(backend)));
    if (backends.length > 0) {
      loadBackendDetails(backends[0]);
    }
  })
  .catch(error => {
    statusElement.className = 'status error';
    statusElement.textContent = 'Backend-Auswahl konnte nicht geladen werden: ' + error.message;
  });
