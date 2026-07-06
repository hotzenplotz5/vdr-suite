

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
let currentEvents = null;
let currentTimers = null;
let currentTimerConflicts = null;
let currentSearchTimers = null;
let currentRecordings = null;
let epgChannelOffset = 0;
let epgTimelineMode = 'time';
let epgWarmCacheInFlight = false;
let epgWarmCacheLastStartedAt = 0;
let epgWarmCacheLastBackendId = '';
let epgWarmCacheStatus = 'EPG-Cache wird vom Daemon im Hintergrund vorbereitet.';
let epgLoadedBackendId = '';
let epgTimeAxisMode = 'horizontal';
let epgTimeWindowPageOffset = 0;
let epgProgramView = 'horizontal';
let selectedEpgDetail = null;
let epgSuppressClickUntil = 0;

const EPG_TIMELINE_VISIBLE_SECONDS = 24 * 60 * 60;
const EPG_TIMELINE_TICK_SECONDS = 2 * 60 * 60;
const EPG_TIMELINE_CONTEXT_BEFORE_SECONDS = 0;
const EPG_TIMELINE_WINDOW_ANCHOR_SECONDS = 30 * 60;
const EPG_TIMELINE_MAX_PAGE_OFFSET = 1;
const EPG_VISIBLE_CHANNEL_LIMIT = 15;

const moduleLabels = {
  overview: 'Übersicht',
  channels: 'Kanäle',
  channelsort: 'Kanäle sortieren',
  epg: 'EPG Zeitleiste',
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

function listSearchTimersFromResponse(data) {
  if (Array.isArray(data)) {
    return data;
  }

  if (data && Array.isArray(data.searchTimers)) {
    return data.searchTimers;
  }

  if (data && Array.isArray(data.searchtimers)) {
    return data.searchtimers;
  }

  if (data && Array.isArray(data.timers)) {
    return data.timers;
  }

  if (data && Array.isArray(data.items)) {
    return data.items;
  }

  return [];
}

function searchTimerActive(searchTimer) {
  const value = firstValue(searchTimer, ['active', 'enabled', 'isActive'], false);
  return value === true || value === 'true' || value === 1 || value === '1';
}

function parseFrontendEventEpoch(value) {
  if (value === undefined || value === null || value === '') {
    return 0;
  }

  const number = Number(value);
  if (Number.isFinite(number) && number > 0) {
    return number > 100000000000 ? Math.floor(number / 1000) : Math.floor(number);
  }

  const parsed = Date.parse(String(value));
  if (Number.isFinite(parsed)) {
    return Math.floor(parsed / 1000);
  }

  return 0;
}

function frontendChannelId(channel) {
  return String(firstValue(channel, ['id', 'channelId', 'nativeId'], '')).trim();
}

function frontendEventChannelId(event) {
  return String(firstValue(event, ['channelId', 'channel', 'channel_id'], '')).trim();
}

function frontendEventEnd(event, start) {
  const explicitEnd = parseFrontendEventEpoch(firstValue(event, ['endTime', 'end', 'stopTime'], ''));
  if (explicitEnd > start) {
    return explicitEnd;
  }

  const duration = Number(firstValue(event, ['durationSeconds', 'duration'], 0));
  if (Number.isFinite(duration) && duration > 0 && start > 0) {
    return start + duration;
  }

  return 0;
}

function findCurrentEventForChannel(channel, events, nowSeconds) {
  const channelId = frontendChannelId(channel);
  if (channelId === '') {
    return null;
  }

  for (const event of events) {
    if (frontendEventChannelId(event) !== channelId) {
      continue;
    }

    const start = parseFrontendEventEpoch(firstValue(event, ['startTime', 'start', 'beginTime'], ''));
    const end = frontendEventEnd(event, start);

    if (start > 0 && end > 0 && start <= nowSeconds && nowSeconds < end) {
      return event;
    }
  }

  return null;
}

function attachCurrentEventsToChannelData(channelData, eventData) {
  const channels = listFromResponse(channelData, 'channels');
  const events = listFromResponse(eventData, 'events');
  const nowSeconds = Math.floor(Date.now() / 1000);

  const enrichedChannels = channels.map(channel => Object.assign(
    {},
    channel,
    {
      currentEvent: findCurrentEventForChannel(channel, events, nowSeconds)
    }
  ));

  const result = Array.isArray(channelData)
    ? { channels: enrichedChannels }
    : Object.assign({}, channelData);

  result.channels = enrichedChannels;
  result.events = events;

  return result;
}

function timerIdPart(timer, index) {
  const timerId = firstValue(timer, ['id', 'timerId', 'nativeId'], '');
  const parts = String(timerId).split(':');
  return parts.length > index ? parts[index] : '';
}

function formatVdrClock(value) {
  if (value === undefined || value === null || value === '') {
    return '-';
  }

  const text = String(value).trim();

  if (text === '') {
    return '-';
  }

  if (text.includes(':') || text.includes('-') || text.includes('T')) {
    return text;
  }

  if (/^\d{1,4}$/.test(text)) {
    const padded = text.padStart(4, '0');
    return padded.slice(0, 2) + ':' + padded.slice(2, 4);
  }

  return text;
}

function timerStartValue(timer) {
  const explicit = firstValue(timer, ['startTime', 'beginTime'], '');
  if (explicit !== '') {
    return explicit;
  }

  const raw = firstValue(timer, ['start', 'begin'], '');
  if (raw !== '') {
    return raw;
  }

  return timerIdPart(timer, 3);
}

function timerEndValue(timer) {
  const explicit = firstValue(timer, ['endTime', 'stopTime'], '');
  if (explicit !== '') {
    return explicit;
  }

  const raw = firstValue(timer, ['stop', 'end'], '');
  if (raw !== '') {
    return raw;
  }

  return timerIdPart(timer, 4);
}

function formatTimerStatus(timer) {
  if (timer.recording === true) {
    return 'nimmt auf';
  }

  if (timer.pending === true) {
    return 'wartend';
  }

  if (timer.enabled === true || timer.active === true) {
    return 'aktiv';
  }

  if (timer.enabled === false || timer.active === false) {
    return 'inaktiv';
  }

  const status = firstValue(timer, ['state', 'status'], '-');

  if (status === true || status === 'true') {
    return 'aktiv';
  }

  if (status === false || status === 'false') {
    return 'inaktiv';
  }

  return String(status);
}

function formatDurationSeconds(value) {
  const seconds = Number(value);

  if (!Number.isFinite(seconds) || seconds <= 0) {
    return '-';
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

function formatSizeMb(value) {
  const sizeMb = Number(value);

  if (!Number.isFinite(sizeMb) || sizeMb <= 0) {
    return '-';
  }

  if (sizeMb >= 1024) {
    return (sizeMb / 1024).toFixed(1) + ' GB';
  }

  return String(Math.round(sizeMb)) + ' MB';
}

function formatRecordingStart(value) {
  if (value === undefined || value === null || value === '' || String(value) === '-1') {
    return '-';
  }

  const number = Number(value);

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

function normalizePathText(value) {
  return String(value || '')
    .replace(/^\/srv\/vdr\/video\//, '/')
    .replace(/\/+/g, '/')
    .replace(/^\//, '');
}

function recordingDisplayParts(recording, index) {
  const rawTitle = String(firstValue(
    recording,
    ['title', 'name', 'file', 'displayName'],
    'Aufnahme ' + String(index + 1)
  ));

  const titleParts = rawTitle.split('/').filter(part => part !== '');

  if (titleParts.length > 1) {
    return {
      folder: titleParts.slice(0, -1).join('/'),
      title: titleParts[titleParts.length - 1]
    };
  }

  const path = normalizePathText(firstValue(recording, ['path', 'fileName', 'directory'], ''));
  const pathParts = path.split('/').filter(part => part !== '');

  if (pathParts.length > 2) {
    return {
      folder: pathParts.slice(0, -2).join('/'),
      title: rawTitle
    };
  }

  if (pathParts.length > 1) {
    return {
      folder: pathParts.slice(0, -1).join('/'),
      title: rawTitle
    };
  }

  return {
    folder: 'Ohne Ordner',
    title: rawTitle
  };
}

function groupRecordings(recordings) {
  const groups = new Map();

  recordings.forEach((recording, index) => {
    const display = recordingDisplayParts(recording, index);
    if (!groups.has(display.folder)) {
      groups.set(display.folder, []);
    }
    groups.get(display.folder).push({
      recording,
      title: display.title,
      index
    });
  });

  return groups;
}

function renderSnapshotMetrics(data) {
  detailDataElement.replaceChildren();
  detailDataElement.appendChild(createMetric('Snapshot', data.snapshotAvailable ? 'ja' : 'nein'));
  detailDataElement.appendChild(createMetric('Kanäle', valueOrZero(data.channelCount)));
  detailDataElement.appendChild(createMetric('Events', valueOrZero(data.eventCount)));
  detailDataElement.appendChild(createMetric('Timer', valueOrZero(data.timerCount)));
  detailDataElement.appendChild(createMetric('Aufnahmen', valueOrZero(data.recordingCount)));
}


let renderChannelList = function() {
  renderModuleError(
    'Kanalbrowser-Modul nicht geladen',
    new Error('/frontend/channel-browser.js konnte renderChannelList nicht registrieren.')
  );
};


function timerConflictEntries(conflict) {
  if (!conflict || !Array.isArray(conflict.entries)) {
    return [];
  }

  return conflict.entries;
}

function timerConflictCount(report) {
  if (!report || typeof report !== 'object') {
    return 0;
  }

  const count = Number(firstValue(report, ['count', 'total'], 0));
  if (Number.isFinite(count) && count > 0) {
    return count;
  }

  if (Array.isArray(report.conflicts)) {
    return report.conflicts.length;
  }

  return 0;
}

function formatTimerConflictTime(value) {
  const epoch = Number(value);

  if (!Number.isFinite(epoch) || epoch <= 0) {
    return '-';
  }

  return new Date(epoch * 1000).toLocaleString('de-DE', {
    weekday: 'short',
    day: '2-digit',
    month: '2-digit',
    hour: '2-digit',
    minute: '2-digit'
  });
}

function describeTimerConflictEntry(entry) {
  const timerIndex = firstValue(entry, ['timerIndex', 'timer', 'index'], '-');
  const percentage = firstValue(entry, ['percentage', 'percent'], '-');
  const concurrent = Array.isArray(entry.concurrentTimerIndices)
    ? entry.concurrentTimerIndices.join(', ')
    : String(firstValue(entry, ['concurrentTimerIndices', 'concurrent'], '-'));

  return 'Timer ' + String(timerIndex)
    + ' · ' + String(percentage) + '%'
    + ' · beteiligt: ' + concurrent;
}

function appendTimerConflictSummary(parent, report) {
  const count = timerConflictCount(report);

  if (count <= 0) {
    return;
  }

  const conflicts = Array.isArray(report.conflicts) ? report.conflicts : [];

  const box = document.createElement('article');
  box.className = 'module-placeholder timer-conflict-panel timer-conflict-panel-alert timer-conflict-summary';
  box.setAttribute('role', 'alert');

  const headline = addText(
    document.createElement('h3'),
    String(count) + ' Timerkonflikt' + (count === 1 ? '' : 'e') + ' erkannt'
  );
  box.appendChild(headline);

  const metaParts = [];
  metaParts.push('Quelle: ' + String(firstValue(report, ['source'], 'restfulapi-epgsearch')));

  if (report.checkAdvised === true) {
    metaParts.push('Neuberechnung empfohlen');
  }

  box.appendChild(addText(document.createElement('p'), metaParts.join(' · '))).className = 'timer-conflict-meta';

  const list = document.createElement('div');
  list.className = 'timer-conflict-list';

  conflicts.slice(0, 5).forEach((conflict, index) => {
    const item = document.createElement('div');
    item.className = 'timer-conflict-item';

    item.appendChild(addText(
      document.createElement('div'),
      'Konflikt ' + String(index + 1) + ' · ' + formatTimerConflictTime(conflict.conflictTime)
    )).className = 'timer-conflict-title';

    timerConflictEntries(conflict).forEach(entry => {
      item.appendChild(addText(
        document.createElement('div'),
        describeTimerConflictEntry(entry)
      )).className = 'timer-conflict-entry';
    });

    const raw = firstValue(conflict, ['raw'], '');
    if (raw !== '') {
      item.appendChild(addText(
        document.createElement('div'),
        'RAW: ' + String(raw)
      )).className = 'timer-conflict-raw';
    }

    list.appendChild(item);
  });

  if (conflicts.length > 5) {
    list.appendChild(addText(
      document.createElement('div'),
      'Zeige 5 von ' + String(conflicts.length) + ' Konflikten.'
    )).className = 'timer-conflict-meta';
  }

  box.appendChild(list);
  parent.appendChild(box);
}

function renderTimerList(data, conflictReport) {
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
    const subtitle = firstValue(timer, ['subtitle'], '');
    const timerId = firstValue(timer, ['timerId', 'id', 'nativeId'], '-');
    const channel = firstValue(timer, ['channelName', 'channel', 'channelId'], '-');
    const status = formatTimerStatus(timer);
    const start = formatVdrClock(timerStartValue(timer));
    const stop = formatVdrClock(timerEndValue(timer));

    item.appendChild(addText(document.createElement('div'), String(title))).className = 'list-title';

    if (subtitle !== '') {
      item.appendChild(addText(document.createElement('div'), String(subtitle))).className = 'list-meta';
    }

    item.appendChild(addText(
      document.createElement('div'),
      'Kanal: ' + String(channel) + ' · Status: ' + status
    )).className = 'list-meta';
    item.appendChild(addText(
      document.createElement('div'),
      'Start: ' + start + ' · Ende: ' + stop
    )).className = 'list-meta';
    item.appendChild(addText(
      document.createElement('div'),
      'ID: ' + String(timerId)
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


function timerConflictListFromReport(report) {
  if (report && Array.isArray(report.conflicts)) {
    return report.conflicts;
  }

  return [];
}

function formatTimerConflictTime(value) {
  const number = Number(value);

  if (Number.isFinite(number) && number > 1000000000) {
    return new Date(number * 1000).toLocaleString("de-DE", {
      year: "numeric",
      month: "2-digit",
      day: "2-digit",
      hour: "2-digit",
      minute: "2-digit"
    });
  }

  return formatVdrClock(value);
}

function timerConflictTimerLabel(timers, timerIndex) {
  const index = Number(timerIndex);
  const timer = Array.isArray(timers) && Number.isFinite(index) && index > 0
    ? timers[index - 1]
    : null;

  if (!timer) {
    return "Timer #" + String(timerIndex);
  }

  const title = firstValue(timer, ["title", "name", "eventTitle", "description", "id", "timerId"], "Timer " + String(timerIndex));
  const start = formatVdrClock(timerStartValue(timer));
  const stop = formatVdrClock(timerEndValue(timer));
  const suffix = start !== "-" || stop !== "-" ? " · " + start + "–" + stop : "";

  return "Timer #" + String(timerIndex) + ": " + String(title) + suffix;
}

function appendTimerConflictPanel(report, timers, error) {
  const previous = detailDataElement.querySelector("[data-timer-conflict-panel=\"true\"]");

  if (previous) {
    previous.remove();
  }

  const panel = document.createElement("article");
  panel.className = "module-placeholder timer-conflict-panel";
  panel.dataset.timerConflictPanel = "true";

  if (error) {
    panel.appendChild(addText(document.createElement("h3"), "Timer-Konflikte konnten nicht geladen werden"));
    panel.appendChild(addText(document.createElement("p"), error.message));
  } else if (report && report.available === false) {
    panel.appendChild(addText(document.createElement("h3"), "Timer-Konfliktprüfung nicht verfügbar"));
    panel.appendChild(addText(document.createElement("p"), firstValue(report, ["error"], "Der Konflikt-Endpunkt ist nicht verfügbar.")));
  } else {
    const conflicts = timerConflictListFromReport(report);
    const count = Number(firstValue(report || {}, ["count"], conflicts.length));
    const total = Number(firstValue(report || {}, ["total"], conflicts.length));
    const source = firstValue(report || {}, ["source"], "unbekannt");
    const activeConflictCount = Number.isFinite(count) ? count : conflicts.length;

    if (activeConflictCount > 0 || conflicts.length > 0) {
      panel.classList.add("timer-conflict-panel-alert");
      panel.setAttribute("aria-label", "Achtung: aktive Timer-Konflikte");
    } else {
      panel.classList.add("timer-conflict-panel-ok");
    }

    if (conflicts.length === 0 && count === 0) {
      panel.appendChild(addText(document.createElement("h3"), "Keine Timer-Konflikte gemeldet"));
      panel.appendChild(addText(document.createElement("p"), "Quelle: " + String(source)));
    } else {
      panel.appendChild(addText(document.createElement("h3"), "Timer-Konflikte: " + String(activeConflictCount)));
      panel.appendChild(addText(document.createElement("p"), "Quelle: " + String(source) + " · Gesamt: " + String(Number.isFinite(total) ? total : conflicts.length)));

      conflicts.slice(0, 10).forEach((conflict, conflictIndex) => {
        const conflictBlock = document.createElement("div");
        conflictBlock.className = "list-meta";

        const time = formatTimerConflictTime(firstValue(conflict, ["conflictTime", "time"], ""));
        const entries = Array.isArray(conflict.entries) ? conflict.entries : [];

        conflictBlock.appendChild(addText(document.createElement("strong"), "Konflikt " + String(conflictIndex + 1) + " · " + time));

        entries.forEach(entry => {
          const timerIndex = firstValue(entry, ["timerIndex"], "?");
          const percentage = firstValue(entry, ["percentage"], "?");
          const concurrent = Array.isArray(entry.concurrentTimerIndices)
            ? entry.concurrentTimerIndices.join(", ")
            : String(firstValue(entry, ["concurrentTimerIndices"], "-"));

          conflictBlock.appendChild(addText(
            document.createElement("div"),
            timerConflictTimerLabel(timers, timerIndex) + " · " + String(percentage) + "% · parallel: " + concurrent
          ));
        });

        panel.appendChild(conflictBlock);
      });
    }
  }

  const target = detailDataElement.querySelector(".list") || detailDataElement;

  if (target.firstChild) {
    target.insertBefore(panel, target.firstChild);
  } else {
    target.appendChild(panel);
  }
}

function loadTimerConflictPanel(timers) {
  const clientApi = window.VdrSuiteClientApi;

  if (!clientApi || typeof clientApi.fetchClientTimerConflicts !== "function") {
    if (selectedModule !== "timers") {
      return;
    }

    appendTimerConflictPanel(
      null,
      timers,
      new Error("Client API wrapper is not available")
    );
    return;
  }

  clientApi.fetchClientTimerConflicts()
    .then(report => {
      if (selectedModule !== "timers") {
        return;
      }

      appendTimerConflictPanel(report, timers, null);
    })
    .catch(error => {
      if (selectedModule !== "timers") {
        return;
      }

      appendTimerConflictPanel(null, timers, error);
    });
}

function renderSearchTimerList(data) {
  const searchTimers = listSearchTimersFromResponse(data);
  detailDataElement.replaceChildren();

  const list = document.createElement('section');
  list.className = 'list searchtimer-list';

  if (searchTimers.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(addText(document.createElement('h3'), 'Keine SearchTimer gefunden'));
    empty.appendChild(addText(document.createElement('p'), 'Der SearchTimer-Endpunkt hat aktuell keine Einträge geliefert.'));
    detailDataElement.appendChild(empty);
    return;
  }

  searchTimers.forEach((searchTimer, index) => {
    const item = document.createElement('article');
    item.className = 'list-item searchtimer-card';

    const name = firstValue(
      searchTimer,
      ['name', 'title', 'query', 'backendNativeId', 'id'],
      'SearchTimer ' + String(index + 1)
    );

    const query = firstValue(searchTimer, ['query', 'search', 'pattern', 'expression'], '');
    const backendId = firstValue(searchTimer, ['backendId', 'backend', 'source'], '');
    const nativeId = firstValue(searchTimer, ['backendNativeId', 'nativeId', 'id'], '');
    const active = searchTimerActive(searchTimer);

    const header = document.createElement('div');
    header.className = 'searchtimer-header';

    const titleBlock = document.createElement('div');
    titleBlock.appendChild(addText(document.createElement('div'), String(name))).className = 'list-title';

    if (query !== '' && query !== name) {
      titleBlock.appendChild(addText(document.createElement('div'), 'Suche: ' + String(query))).className = 'list-meta searchtimer-query';
    }

    const status = addText(document.createElement('span'), active ? 'aktiv' : 'inaktiv');
    status.className = 'searchtimer-status ' + (active ? 'enabled' : 'disabled');

    header.appendChild(titleBlock);
    header.appendChild(status);
    item.appendChild(header);

    const metaParts = [];
    if (backendId !== '') {
      metaParts.push('Backend: ' + String(backendId));
    }
    if (nativeId !== '') {
      metaParts.push('ID: ' + String(nativeId));
    }

    if (metaParts.length > 0) {
      item.appendChild(addText(document.createElement('div'), metaParts.join(' · '))).className = 'list-meta searchtimer-technical';
    }

    list.appendChild(item);
  });

  detailDataElement.appendChild(list);
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
    empty.appendChild(addText(document.createElement('p'), 'Der Endpunkt /api/vdr/recordings/query hat aktuell keine Aufnahmen geliefert.'));
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

function formatEpgClockFromEpoch(epochSeconds) {
  const value = Number(epochSeconds);
  if (!Number.isFinite(value) || value <= 0) {
    return '--:--';
  }

  return new Date(value * 1000).toLocaleTimeString('de-DE', {
    hour: '2-digit',
    minute: '2-digit'
  });
}

function epgEventTitle(event) {
  return String(firstValue(event, ['title', 'name', 'eventTitle', 'shortText'], 'Ohne Titel'));
}

function epgEventSubtitle(event) {
  return String(firstValue(event, ['subtitle', 'shortText', 'description'], ''));
}

function epgChannelTitle(channel, index) {
  return String(firstValue(
    channel,
    ['name', 'channelName', 'title', 'displayName', 'id', 'channelId'],
    'Kanal ' + String(index + 1)
  ));
}

function epgEventDescription(event) {
  return String(firstValue(event, ['description', 'longText', 'details', 'synopsis'], ''));
}

function buildEpgVisibleEventIndex(visibleChannels, events, bounds) {
  const index = new Map();
  const visibleChannelIds = new Set();

  visibleChannels.forEach(channel => {
    const channelId = frontendChannelId(channel);
    index.set(channelId, []);

    if (channelId !== '') {
      visibleChannelIds.add(channelId);
    }
  });

  events.forEach(event => {
    const channelId = frontendEventChannelId(event);

    if (!visibleChannelIds.has(channelId)) {
      return;
    }

    const start = parseFrontendEventEpoch(firstValue(event, ['startTime', 'start', 'beginTime'], ''));
    const end = frontendEventEnd(event, start);

    if (start <= 0 || end <= 0 || end <= bounds.start || start >= bounds.end) {
      return;
    }

    index.get(channelId).push({ event, start, end });
  });

  index.forEach(entries => {
    entries.sort((left, right) => left.start - right.start);
  });

  return index;
}

function epgIndexedEventsForChannel(eventIndex, channel) {
  return eventIndex.get(frontendChannelId(channel)) || [];
}

function formatEpgDuration(start, end) {
  if (!Number.isFinite(start) || !Number.isFinite(end) || end <= start) {
    return '-';
  }

  const minutes = Math.round((end - start) / 60);
  if (minutes < 60) {
    return String(minutes) + ' min';
  }

  const hours = Math.floor(minutes / 60);
  const rest = minutes % 60;

  if (rest === 0) {
    return String(hours) + ' h';
  }

  return String(hours) + ' h ' + String(rest) + ' min';
}

function appendEpgDetailMeta(parent, label, value) {
  const item = document.createElement('div');
  item.className = 'epg-detail-meta-item';

  const key = addText(document.createElement('span'), label);
  key.className = 'epg-detail-meta-label';

  const val = addText(document.createElement('strong'), String(value));
  val.className = 'epg-detail-meta-value';

  item.appendChild(key);
  item.appendChild(val);
  parent.appendChild(item);
}

function formatEpgTimerDay(epochSeconds) {
  const date = new Date(epochSeconds * 1000);
  const year = String(date.getFullYear());
  const month = String(date.getMonth() + 1).padStart(2, '0');
  const day = String(date.getDate()).padStart(2, '0');

  return year + '-' + month + '-' + day;
}

function formatEpgTimerClockValue(epochSeconds) {
  const date = new Date(epochSeconds * 1000);
  return Number(
    String(date.getHours()).padStart(2, '0') +
    String(date.getMinutes()).padStart(2, '0')
  );
}

const EPG_TIMER_DETAIL_SYNC_INTERVAL_MS = 3500;
let epgTimerDetailSyncIntervalId = null;
let epgTimerDetailSyncInFlight = false;
let epgLiveTimerCache = [];

function epgTimerDetailCards() {
  return Array.from(document.querySelectorAll('[data-epg-timer-sync="true"]'));
}

function epgTimerText(value) {
  return String(value || '')
    .trim()
    .replace(/\s+/g, ' ')
    .toLowerCase();
}

function epgTimerEpochSeconds(value) {
  const parsed = parseFrontendEventEpoch(value);
  if (Number.isFinite(parsed) && parsed > 0) {
    return parsed;
  }

  const numeric = Number(value);
  if (Number.isFinite(numeric) && numeric > 1000000000) {
    return numeric;
  }

  const date = new Date(String(value || ''));
  if (!Number.isNaN(date.getTime())) {
    return Math.floor(date.getTime() / 1000);
  }

  return 0;
}

function epgTimerDetailMetadata(detail) {
  return {
    channelId: String(detail.dataset.epgTimerChannelId || ''),
    eventId: String(detail.dataset.epgTimerEventId || ''),
    title: String(detail.dataset.epgTimerTitle || ''),
    start: Number(detail.dataset.epgTimerStart || 0),
    end: Number(detail.dataset.epgTimerEnd || 0),
    createdTimerId: String(detail.dataset.epgTimerCreatedTimerId || '')
  };
}

function epgTimerFromResponse(data) {
  return listFromResponse(data, 'timers');
}

function epgTimerMatchesDetail(timer, metadata) {
  const timerId = String(firstValue(timer, ['timerId', 'id', 'nativeId'], ''));
  if (metadata.createdTimerId !== '' && timerId !== '') {
    return metadata.createdTimerId === timerId;
  }

  const timerChannelId = String(firstValue(timer, ['channelId', 'channel'], ''));
  if (metadata.channelId !== '' && timerChannelId !== '' && metadata.channelId !== timerChannelId) {
    return false;
  }

  const timerEventId = String(firstValue(timer, ['eventId', 'eventID', 'eventNativeId'], ''));
  if (metadata.eventId !== '' && timerEventId !== '') {
    return metadata.eventId === timerEventId;
  }

  const timerTitle = epgTimerText(firstValue(timer, ['title', 'name', 'eventTitle'], ''));
  const detailTitle = epgTimerText(metadata.title);
  const titleMatches = timerTitle !== '' && detailTitle !== '' && timerTitle === detailTitle;

  const timerStart = epgTimerEpochSeconds(firstValue(timer, ['startTime', 'start', 'beginTime'], ''));
  const timerEnd = epgTimerEpochSeconds(firstValue(timer, ['endTime', 'stop', 'stopTime'], ''));

  const startMatches =
    metadata.start > 0 &&
    timerStart > 0 &&
    Math.abs(timerStart - metadata.start) <= 180;

  const endMatches =
    metadata.end > 0 &&
    timerEnd > 0 &&
    Math.abs(timerEnd - metadata.end) <= 180;

  return startMatches && (endMatches || titleMatches);
}

function clearEpgTimerSuccessFeedback(container) {
  container.querySelectorAll('[data-epg-timer-status="true"].success').forEach(element => {
    element.remove();
  });
}

function setEpgTimerDetailState(detail, matchingTimer) {
  const button = detail.querySelector('[data-epg-create-timer-action="true"]');
  if (!button || button.classList.contains('pending')) {
    return;
  }

  if (matchingTimer) {
    const timerId = String(firstValue(matchingTimer, ['timerId', 'id', 'nativeId'], ''));
    if (timerId !== '') {
      detail.dataset.epgTimerCreatedTimerId = timerId;
    }

    detail.dataset.epgTimerState = 'present';
    button.disabled = true;
    button.classList.add('timer-present');
    button.textContent = timerRecording(matchingTimer) ? 'Nimmt auf' : 'Timer vorhanden';
    button.title = 'Für diese EPG-Sendung existiert bereits ein Timer.';
    return;
  }

  if (detail.dataset.epgTimerState === 'present') {
    clearEpgTimerSuccessFeedback(detail);
  }

  detail.dataset.epgTimerState = 'absent';
  detail.dataset.epgTimerCreatedTimerId = '';
  button.disabled = false;
  button.classList.remove('timer-present');
  button.textContent = 'Timer erstellen';
  button.title = 'Timer aus dieser EPG-Sendung auf dem ausgewählten VDR erstellen.';
}

function syncEpgTimerDetailWithTimers(detail, timers) {
  const metadata = epgTimerDetailMetadata(detail);
  const matchingTimer = timers.find(timer => epgTimerMatchesDetail(timer, metadata));
  setEpgTimerDetailState(detail, matchingTimer || null);
}

function syncEpgTimerDetailStates() {
  const details = epgTimerDetailCards();
  if (details.length === 0) {
    if (epgTimerDetailSyncIntervalId !== null) {
      window.clearInterval(epgTimerDetailSyncIntervalId);
      epgTimerDetailSyncIntervalId = null;
    }
    return;
  }

  if (epgTimerDetailSyncInFlight) {
    return;
  }

  epgTimerDetailSyncInFlight = true;

  fetch('/api/vdr/timers/live', { cache: 'no-store' })
    .then(response => {
      if (!response.ok) {
        throw new Error('HTTP ' + String(response.status));
      }
      return response.json();
    })
    .then(data => {
      epgLiveTimerCache = epgTimerFromResponse(data);
      epgTimerDetailCards().forEach(detail => {
        syncEpgTimerDetailWithTimers(detail, epgLiveTimerCache);
      });
    })
    .catch(() => {
      /* Live-Sync ist Komfort. Bei Fehler bleibt die lokale Anzeige unverändert. */
    })
    .finally(() => {
      epgTimerDetailSyncInFlight = false;
    });
}

function startEpgTimerDetailSync() {
  if (epgTimerDetailSyncIntervalId === null) {
    epgTimerDetailSyncIntervalId = window.setInterval(
      syncEpgTimerDetailStates,
      EPG_TIMER_DETAIL_SYNC_INTERVAL_MS
    );
  }
}

function syncEpgTimerDetailStatesSoon() {
  startEpgTimerDetailSync();
  window.setTimeout(syncEpgTimerDetailStates, 250);
}

function buildEpgTimerCreatePayload(event, channel) {
  const start = parseFrontendEventEpoch(firstValue(event, ['startTime', 'start', 'beginTime'], ''));
  const end = frontendEventEnd(event, start);
  const eventId = firstValue(event, ['eventId', 'id', 'nativeId'], '');
  const channelId = frontendEventChannelId(event) || frontendChannelId(channel);

  return {
    backendId: selectedEpgBackendId(),
    channelId,
    title: epgEventTitle(event),
    directory: '',
    day: formatEpgTimerDay(start),
    weekdays: '-------',
    start: formatEpgTimerClockValue(start),
    stop: formatEpgTimerClockValue(end),
    priority: 50,
    lifetime: 99,
    active: true,
    aux: eventId !== '' ? 'eventId=' + String(eventId) : ''
  };
}

function clearEpgTimerFeedback(container) {
  container.querySelectorAll('[data-epg-timer-preview="true"], [data-epg-timer-status="true"]').forEach(element => {
    element.remove();
  });
}

function appendEpgTimerFeedback(container, element) {
  const technical = container.querySelector('.epg-event-technical');
  if (technical) {
    container.insertBefore(element, technical);
    return;
  }

  container.appendChild(element);
}

function showEpgTimerPayloadPreview(container, event, channel) {
  clearEpgTimerFeedback(container);

  const payload = buildEpgTimerCreatePayload(event, channel);

  const preview = document.createElement('div');
  preview.className = 'epg-timer-preview';
  preview.dataset.epgTimerPreview = 'true';

  preview.appendChild(addText(document.createElement('h4'), 'Timer-Vorschau'));

  const summary = document.createElement('div');
  summary.className = 'epg-timer-preview-summary';

  appendEpgDetailMeta(summary, 'Backend', payload.backendId);
  appendEpgDetailMeta(summary, 'Kanal', payload.channelId || '-');
  appendEpgDetailMeta(summary, 'Titel', payload.title);
  appendEpgDetailMeta(summary, 'Datum', payload.day);
  appendEpgDetailMeta(summary, 'Start', String(payload.start));
  appendEpgDetailMeta(summary, 'Stop', String(payload.stop));

  preview.appendChild(summary);

  const note = addText(
    document.createElement('p'),
    'Noch nicht gesendet. Das ist nur die vorbereitete Nutzlast für /api/vdr/timers/actions/create.'
  );
  note.className = 'epg-timer-preview-note';
  preview.appendChild(note);

  const code = addText(document.createElement('pre'), JSON.stringify(payload, null, 2));
  code.className = 'epg-timer-preview-code';
  preview.appendChild(code);

  appendEpgTimerFeedback(container, preview);
}

function epgTimerResultDetails(result) {
  const details = [];

  if (result && Array.isArray(result.errors)) {
    result.errors.forEach(error => details.push(String(error)));
  }

  if (result && Array.isArray(result.warnings)) {
    result.warnings.forEach(warning => details.push(String(warning)));
  }

  return details;
}

function showEpgTimerStatus(container, success, title, message, details) {
  clearEpgTimerFeedback(container);

  const status = document.createElement('div');
  status.className = 'epg-timer-status ' + (success ? 'success' : 'error');
  status.dataset.epgTimerStatus = 'true';

  status.appendChild(addText(document.createElement('h4'), title));
  status.appendChild(addText(document.createElement('p'), message));

  if (Array.isArray(details) && details.length > 0) {
    const list = document.createElement('ul');
    details.forEach(detail => {
      list.appendChild(addText(document.createElement('li'), String(detail)));
    });
    status.appendChild(list);
  }

  appendEpgTimerFeedback(container, status);
}

function parseEpgTimerCreateResponse(response, text) {
  let data = {};

  if (text !== '') {
    try {
      data = JSON.parse(text);
    } catch (parseError) {
      data = { message: text };
    }
  }

  if (!response.ok) {
    const message = String(data.error || data.message || ('HTTP ' + String(response.status)));
    const error = new Error(message);
    error.data = data;
    throw error;
  }

  return data;
}

function validateEpgTimerPayload(payload) {
  const missing = [];

  if (!payload.backendId) {
    missing.push('Backend fehlt');
  }

  if (!payload.channelId) {
    missing.push('Kanal-ID fehlt');
  }

  if (!payload.title) {
    missing.push('Titel fehlt');
  }

  if (!payload.day) {
    missing.push('Datum fehlt');
  }

  if (!Number.isFinite(Number(payload.start)) || Number(payload.start) <= 0) {
    missing.push('Startzeit fehlt');
  }

  if (!Number.isFinite(Number(payload.stop)) || Number(payload.stop) <= 0) {
    missing.push('Stopzeit fehlt');
  }

  return missing;
}

function createEpgTimerFromDetail(container, event, channel, button) {
  const payload = buildEpgTimerCreatePayload(event, channel);
  const validationErrors = validateEpgTimerPayload(payload);

  if (validationErrors.length > 0) {
    showEpgTimerStatus(
      container,
      false,
      'Timer kann nicht erstellt werden',
      'Die EPG-Sendung enthält nicht alle nötigen Timerdaten.',
      validationErrors
    );
    return;
  }

  const originalLabel = button ? button.textContent : '';
  if (button) {
    button.disabled = true;
    button.classList.add('pending');
    button.textContent = 'Erstelle …';
  }

  clearEpgTimerFeedback(container);

  fetch('/api/vdr/timers/actions/create', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    cache: 'no-store',
    body: JSON.stringify(payload)
  })
    .then(response => response.text().then(text => parseEpgTimerCreateResponse(response, text)))
    .then(result => {
      if (result && result.success === false) {
        showEpgTimerStatus(
          container,
          false,
          'Timer wurde nicht erstellt',
          String(result.message || 'Der VDR hat die Timer-Erstellung abgelehnt.'),
          epgTimerResultDetails(result)
        );

        if (button) {
          button.disabled = false;
          button.classList.remove('pending');
          button.textContent = originalLabel;
        }
        return;
      }

      const timerId = result && result.timerId ? String(result.timerId) : '';
      showEpgTimerStatus(
        container,
        true,
        'Timer erstellt',
        timerId !== ''
          ? 'Timer wurde erfolgreich erstellt. Timer-ID: ' + timerId
          : String((result && result.message) || 'Timer wurde erfolgreich erstellt.'),
        epgTimerResultDetails(result)
      );

      if (timerId !== '') {
        container.dataset.epgTimerCreatedTimerId = timerId;
      }
      container.dataset.epgTimerState = 'present';

      if (button) {
        button.classList.remove('pending');
        button.textContent = 'Timer erstellt';
      }

      syncEpgTimerDetailStatesSoon();
    })
    .catch(error => {
      const data = error && error.data ? error.data : {};
      const details = epgTimerResultDetails(data);

      showEpgTimerStatus(
        container,
        false,
        'Timer-Fehler',
        String((error && error.message) || 'Timer konnte nicht erstellt werden.'),
        details
      );

      if (button) {
        button.disabled = false;
        button.classList.remove('pending');
        button.textContent = originalLabel;
      }
    });
}

function createEpgDetailAction(label, hint, onClick) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'epg-detail-action';
  button.textContent = label;
  button.title = hint;

  if (typeof onClick === 'function') {
    button.disabled = false;
    button.addEventListener('click', onClick);
  } else {
    button.disabled = true;
  }

  return button;
}

function createEpgEventDetailCard(event, channel) {
  const start = parseFrontendEventEpoch(firstValue(event, ['startTime', 'start', 'beginTime'], ''));
  const end = frontendEventEnd(event, start);
  const channelTitle = epgChannelTitle(channel, 0);
  const detail = document.createElement('article');
  detail.className = 'module-placeholder epg-event-detail epg-event-detail-action-panel';

  const syncEventId = String(firstValue(event, ['eventId', 'id', 'nativeId'], ''));
  const syncChannelId = frontendEventChannelId(event) || frontendChannelId(channel);

  detail.dataset.epgTimerSync = 'true';
  detail.dataset.epgTimerState = 'unknown';
  detail.dataset.epgTimerEventId = syncEventId;
  detail.dataset.epgTimerChannelId = syncChannelId;
  detail.dataset.epgTimerTitle = epgEventTitle(event);
  detail.dataset.epgTimerStart = String(start);
  detail.dataset.epgTimerEnd = String(end);
  detail.dataset.epgTimerCreatedTimerId = '';

  const hero = document.createElement('div');
  hero.className = 'epg-detail-hero';

  const eyebrow = addText(document.createElement('div'), 'EPG-Details');
  eyebrow.className = 'epg-detail-eyebrow';
  hero.appendChild(eyebrow);

  const title = addText(document.createElement('h3'), epgEventTitle(event));
  title.className = 'epg-detail-title';
  hero.appendChild(title);

  const subtitle = epgEventSubtitle(event);
  if (subtitle !== '') {
    const subtitleElement = addText(document.createElement('p'), subtitle);
    subtitleElement.className = 'epg-detail-subtitle';
    hero.appendChild(subtitleElement);
  }

  detail.appendChild(hero);

  const metaGrid = document.createElement('div');
  metaGrid.className = 'epg-detail-meta-grid';

  appendEpgDetailMeta(metaGrid, 'Kanal', channelTitle);
  appendEpgDetailMeta(metaGrid, 'Zeit', formatEpgClockFromEpoch(start) + '–' + formatEpgClockFromEpoch(end));
  appendEpgDetailMeta(metaGrid, 'Dauer', formatEpgDuration(start, end));
  appendEpgDetailMeta(metaGrid, 'Backend', selectedEpgBackendId());

  detail.appendChild(metaGrid);

  const description = epgEventDescription(event);
  if (description !== '' && description !== subtitle) {
    const descriptionBox = document.createElement('div');
    descriptionBox.className = 'epg-detail-description';
    descriptionBox.appendChild(addText(document.createElement('h4'), 'Beschreibung'));
    descriptionBox.appendChild(addText(document.createElement('p'), description));
    detail.appendChild(descriptionBox);
  }

  const actions = document.createElement('div');
  actions.className = 'epg-detail-actions';
  actions.setAttribute('aria-label', 'EPG Aktionen');

  const createTimerAction = createEpgDetailAction(
    'Timer erstellen',
    'Timer aus dieser EPG-Sendung auf dem ausgewählten VDR erstellen.',
    clickEvent => createEpgTimerFromDetail(detail, event, channel, clickEvent.currentTarget)
  );
  createTimerAction.classList.add('primary');
  createTimerAction.dataset.epgCreateTimerAction = 'true';
  actions.appendChild(createTimerAction);

  actions.appendChild(createEpgDetailAction(
    'Timerdaten prüfen',
    'Timerdaten aus dieser EPG-Sendung berechnen und anzeigen.',
    () => showEpgTimerPayloadPreview(detail, event, channel)
  ));
  actions.appendChild(createEpgDetailAction('Suchtimer', 'Aktion vorbereitet. Echte SearchTimer-Anbindung folgt später.'));
  actions.appendChild(createEpgDetailAction('Mehr …', 'Weitere EPG-Aktionen werden später angebunden.'));

  detail.appendChild(actions);

  const eventId = syncEventId;
  const channelId = syncChannelId;
  const technical = [];

  if (channelId !== '') {
    technical.push('channelId=' + channelId);
  }

  if (eventId !== '') {
    technical.push('eventId=' + String(eventId));
  }

  if (technical.length > 0) {
    const technicalLine = addText(document.createElement('p'), technical.join(' · '));
    technicalLine.className = 'epg-event-technical';
    detail.appendChild(technicalLine);
  }

  syncEpgTimerDetailStatesSoon();

  return detail;
}

function renderEpgSideDetail() {
  const holder = detailDataElement.querySelector('[data-epg-side-detail="true"]');

  if (!holder) {
    return;
  }

  holder.replaceChildren();

  if (!selectedEpgDetail || !selectedEpgDetail.event || !selectedEpgDetail.channel) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder epg-detail-empty';
    empty.appendChild(addText(document.createElement('h3'), 'EPG-Details'));
    empty.appendChild(addText(document.createElement('p'), 'Sendung anklicken, dann erscheinen die Details hier.'));
    holder.appendChild(empty);
    return;
  }

  holder.appendChild(createEpgEventDetailCard(selectedEpgDetail.event, selectedEpgDetail.channel));
}

function alignEpgSideDetailToSource(sourceElement) {
  (void sourceElement);

  const holder = detailDataElement.querySelector('[data-epg-side-detail="true"]');
  if (holder) {
    holder.style.removeProperty('--epg-detail-top');
  }
}

function renderEpgEventDetail(event, channel, sourceElement) {
  selectedEpgDetail = { event, channel };

  if (
    isMobileEpgLayout()
    && sourceElement
    && sourceElement.classList
    && sourceElement.classList.contains('epg-program-event')
  ) {
    renderMobileEpgInlineDetail(event, channel, sourceElement);
    return;
  }

  clearMobileEpgInlineDetails(null);
  renderEpgSideDetail();
  alignEpgSideDetailToSource(sourceElement);

  const holder = detailDataElement.querySelector('[data-epg-side-detail="true"]');
  const desktop = window.matchMedia && window.matchMedia('(min-width: 1100px)').matches;

  if (holder && !desktop) {
    holder.scrollIntoView({ behavior: 'smooth', block: 'start' });
  }
}

function isMobileEpgLayout() {
  return window.matchMedia && window.matchMedia('(max-width: 720px)').matches;
}

function clearMobileEpgInlineDetails(focusElement) {
  detailDataElement.querySelectorAll('[data-epg-inline-detail="true"]').forEach(element => {
    element.remove();
  });

  detailDataElement.querySelectorAll('.epg-program-card-expanded').forEach(card => {
    card.classList.remove('epg-program-card-expanded');
  });

  detailDataElement.querySelectorAll('.epg-program-event.selected').forEach(button => {
    button.classList.remove('selected');
    button.removeAttribute('aria-expanded');
  });

  if (focusElement && typeof focusElement.focus === 'function') {
    try {
      focusElement.focus({ preventScroll: true });
    } catch (focusError) {
      void focusError;
      focusElement.focus();
    }
  }
}

function renderMobileEpgInlineDetail(event, channel, sourceElement) {
  const card = sourceElement ? sourceElement.closest('.epg-program-card') : null;
  const events = card ? card.querySelector('.epg-program-events') : null;

  if (!card || !events) {
    renderEpgSideDetail();
    alignEpgSideDetailToSource(sourceElement);
    return;
  }

  clearMobileEpgInlineDetails(null);

  card.classList.add('epg-program-card-expanded');
  sourceElement.classList.add('selected');
  sourceElement.setAttribute('aria-expanded', 'true');

  const inline = document.createElement('section');
  inline.className = 'epg-program-inline-detail';
  inline.dataset.epgInlineDetail = 'true';

  const back = document.createElement('button');
  back.type = 'button';
  back.className = 'epg-mobile-back-button';
  back.textContent = 'Zurück zur EPG-Liste';
  back.addEventListener('click', () => {
    selectedEpgDetail = null;
    clearMobileEpgInlineDetails(sourceElement);
    renderEpgSideDetail();
  });

  inline.appendChild(back);
  inline.appendChild(createEpgEventDetailCard(event, channel));

  events.insertAdjacentElement('afterend', inline);
  renderEpgSideDetail();

  window.setTimeout(() => {
    inline.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }, 30);
}

function createEpgEventCard(entry, channel) {
  const event = entry.event;
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'epg-event-card';
  button.setAttribute('aria-label', 'EPG Details für ' + epgEventTitle(event) + ' öffnen');

  const time = addText(
    document.createElement('div'),
    (new Date(entry.start * 1000).toLocaleDateString('de-DE', { weekday: 'short', day: '2-digit', month: '2-digit' }) + ' ' + formatEpgClockFromEpoch(entry.start) + '–' + formatEpgClockFromEpoch(entry.end))
  );
  time.className = 'epg-event-time';

  const title = addText(document.createElement('div'), epgEventTitle(event));
  title.className = 'epg-event-title';

  const subtitle = epgEventSubtitle(event);
  button.appendChild(time);
  button.appendChild(title);

  if (subtitle !== '') {
    const subtitleElement = addText(document.createElement('div'), subtitle);
    subtitleElement.className = 'epg-event-subtitle';
    button.appendChild(subtitleElement);
  }

  button.addEventListener('click', clickEvent => {
    if (Date.now() < epgSuppressClickUntil) {
      clickEvent.preventDefault();
      return;
    }

    renderEpgEventDetail(event, channel, clickEvent.currentTarget);
  });
  return button;
}

function epgTimelineBounds(nowSeconds) {
  const baseStart = Math.floor(
    (nowSeconds - EPG_TIMELINE_CONTEXT_BEFORE_SECONDS) / EPG_TIMELINE_WINDOW_ANCHOR_SECONDS
  ) * EPG_TIMELINE_WINDOW_ANCHOR_SECONDS;

  const pageOffset = Math.max(
    0,
    Math.min(EPG_TIMELINE_MAX_PAGE_OFFSET, Number(epgTimeWindowPageOffset || 0))
  );

  const start = baseStart + (pageOffset * EPG_TIMELINE_VISIBLE_SECONDS);

  return {
    start,
    end: start + EPG_TIMELINE_VISIBLE_SECONDS,
    duration: EPG_TIMELINE_VISIBLE_SECONDS
  };
}

function epgTimelinePercent(epochSeconds, bounds) {
  return ((epochSeconds - bounds.start) / bounds.duration) * 100;
}

function epgEventPositionForBounds(entry, bounds) {
  const visibleStart = Math.max(entry.start, bounds.start);
  const visibleEnd = Math.min(entry.end, bounds.end);

  if (visibleEnd <= visibleStart) {
    return null;
  }

  const left = epgTimelinePercent(visibleStart, bounds);
  const width = ((visibleEnd - visibleStart) / bounds.duration) * 100;

  return {
    left,
    width,
    startsBeforeWindow: entry.start < bounds.start,
    endsAfterWindow: entry.end > bounds.end
  };
}

function appendEpgTimelineTicks(track, bounds, withLabels) {
  for (let tick = bounds.start; tick <= bounds.end; tick += EPG_TIMELINE_TICK_SECONDS) {
    const left = epgTimelinePercent(tick, bounds);

    const line = document.createElement('div');
    line.className = 'epg-time-grid-line';
    line.style.left = left.toFixed(3) + '%';
    track.appendChild(line);

    if (withLabels) {
      const label = addText(document.createElement('span'), formatEpgClockFromEpoch(tick));
      label.className = 'epg-time-grid-label';
      label.style.left = left.toFixed(3) + '%';
      track.appendChild(label);
    }
  }
}

function appendEpgNowLine(track, bounds, nowSeconds, withLabel) {
  if (nowSeconds < bounds.start || nowSeconds > bounds.end) {
    return;
  }

  const line = document.createElement('div');
  line.className = 'epg-now-line';
  line.style.left = epgTimelinePercent(nowSeconds, bounds).toFixed(3) + '%';

  if (withLabel) {
    const label = addText(document.createElement('span'), 'Jetzt ' + formatEpgClockFromEpoch(nowSeconds));
    label.className = 'epg-now-line-label';
    line.appendChild(label);
  }

  track.appendChild(line);
}

function appendEpgVerticalTimelineTicks(track, bounds, withLabels) {
  for (let tick = bounds.start; tick <= bounds.end; tick += EPG_TIMELINE_TICK_SECONDS) {
    const top = epgTimelinePercent(tick, bounds);

    const line = document.createElement('div');
    line.className = 'epg-vertical-grid-line';
    line.style.top = top.toFixed(3) + '%';
    track.appendChild(line);

    if (withLabels) {
      const label = addText(document.createElement('span'), formatEpgClockFromEpoch(tick));
      label.className = 'epg-vertical-grid-label';
      label.style.top = top.toFixed(3) + '%';
      track.appendChild(label);
    }
  }
}

function appendEpgVerticalNowLine(track, bounds, nowSeconds, withLabel) {
  if (nowSeconds < bounds.start || nowSeconds > bounds.end) {
    return;
  }

  const line = document.createElement('div');
  line.className = 'epg-vertical-now-line';
  line.style.top = epgTimelinePercent(nowSeconds, bounds).toFixed(3) + '%';

  if (withLabel) {
    const label = addText(document.createElement('span'), 'Jetzt ' + formatEpgClockFromEpoch(nowSeconds));
    label.className = 'epg-vertical-now-line-label';
    line.appendChild(label);
  }

  track.appendChild(line);
}

function createEpgVerticalChannelHeader(channel, index) {
  const header = document.createElement('div');
  header.className = 'epg-vertical-channel-header';

  const channelTitleText = epgChannelTitle(channel, epgChannelOffset + index);
  const channelId = firstValue(channel, ['channelId', 'id', 'nativeId'], '');

  if (typeof createChannelLogoElement === 'function') {
    const logo = createChannelLogoElement(channelTitleText, channelId);
    logo.classList.add('epg-channel-logo');
    header.appendChild(logo);
  }

  const title = addText(document.createElement('h3'), channelTitleText);
  header.appendChild(title);

  return header;
}



function epgVerticalColumnTemplate(visibleChannelCount) {
  const channelCount = Math.max(Number(visibleChannelCount) || 0, 1);

  if (typeof window !== 'undefined' &&
      typeof window.matchMedia === 'function' &&
      window.matchMedia('(max-width: 720px)').matches) {
    return '3rem repeat(' + String(channelCount) + ', minmax(calc(100vw - 5.2rem), calc(100vw - 5.2rem)))';
  }

  return '5.8rem repeat(' + String(channelCount) + ', minmax(13rem, 1fr))';
}

function enableEpgDragPan(surface, horizontalTarget, options) {
  if (!surface || !horizontalTarget) {
    return;
  }

  const config = options || {};
  const classTarget = config.classTarget || surface;
  const threshold = 14;

  let pointerActive = false;
  let dragging = false;
  let startX = 0;
  let startY = 0;
  let startScrollLeft = 0;
  let startWindowScrollY = 0;

  const addDraggingClass = () => {
    surface.classList.add('dragging');
    horizontalTarget.classList.add('dragging');
    classTarget.classList.add('dragging');
  };

  const removeDraggingClass = () => {
    surface.classList.remove('dragging');
    horizontalTarget.classList.remove('dragging');
    classTarget.classList.remove('dragging');
  };

  const endDrag = () => {
    if (dragging) {
      epgSuppressClickUntil = Date.now() + 250;
    }

    pointerActive = false;
    dragging = false;
    removeDraggingClass();
  };

  surface.addEventListener('pointerdown', event => {
    if (event.button !== undefined && event.button !== 0) {
      return;
    }

    pointerActive = true;
    dragging = false;
    startX = event.clientX;
    startY = event.clientY;
    startScrollLeft = horizontalTarget.scrollLeft;
    startWindowScrollY = window.scrollY || window.pageYOffset || 0;
  });

  surface.addEventListener('pointermove', event => {
    if (!pointerActive) {
      return;
    }

    const deltaX = event.clientX - startX;
    const deltaY = event.clientY - startY;

    if (!dragging && Math.hypot(deltaX, deltaY) < threshold) {
      return;
    }

    if (!dragging) {
      dragging = true;
      addDraggingClass();
    }

    horizontalTarget.scrollLeft = startScrollLeft - deltaX;

    if (config.verticalWindow !== false) {
      window.scrollTo(window.scrollX || window.pageXOffset || 0, startWindowScrollY - deltaY);
    }

    event.preventDefault();
  });

  surface.addEventListener('pointerup', endDrag);
  surface.addEventListener('pointercancel', endDrag);
  surface.addEventListener('pointerleave', endDrag);
}


function enableEpgVerticalHorizontalScroll(grid, topScroller, scrollContent, dragSurface) {
  const inner = topScroller.firstElementChild;
  let syncing = false;

  const updateTopScrollerWidth = () => {
    if (!inner) {
      return;
    }

    const width = Math.max(scrollContent.scrollWidth, scrollContent.clientWidth);
    inner.style.width = String(width) + 'px';
  };

  const syncFromTop = () => {
    if (syncing) {
      return;
    }

    syncing = true;
    scrollContent.scrollLeft = topScroller.scrollLeft;
    syncing = false;
  };

  const syncFromContent = () => {
    if (syncing) {
      return;
    }

    syncing = true;
    topScroller.scrollLeft = scrollContent.scrollLeft;
    syncing = false;
  };

  topScroller.addEventListener('scroll', syncFromTop, { passive: true });
  scrollContent.addEventListener('scroll', syncFromContent, { passive: true });

  const setDragging = (surface, enabled) => {
    surface.classList.toggle('dragging', enabled);
    topScroller.classList.toggle('dragging', enabled);
    scrollContent.classList.toggle('dragging', enabled);
    grid.classList.toggle('dragging', enabled);
  };

  const bindHorizontalDrag = surface => {
    if (!surface || surface.dataset.epgVerticalHorizontalDragBound === 'true') {
      return;
    }

    surface.dataset.epgVerticalHorizontalDragBound = 'true';

    let pointerActive = false;
    let dragging = false;
    let startX = 0;
    let startScrollLeft = 0;

    const endDrag = () => {
      if (dragging) {
        epgSuppressClickUntil = Date.now() + 260;
      }

      pointerActive = false;
      dragging = false;
      setDragging(surface, false);
    };

    surface.addEventListener('pointerdown', event => {
      if (event.button !== undefined && event.button !== 0) {
        return;
      }

      pointerActive = true;
      dragging = false;
      startX = event.clientX;
      startScrollLeft = scrollContent.scrollLeft;
    });

    surface.addEventListener('pointermove', event => {
      if (!pointerActive) {
        return;
      }

      const deltaX = event.clientX - startX;

      if (!dragging && Math.abs(deltaX) < 7) {
        return;
      }

      if (!dragging) {
        dragging = true;
        setDragging(surface, true);

        if (surface.setPointerCapture && event.pointerId !== undefined) {
          surface.setPointerCapture(event.pointerId);
        }
      }

      scrollContent.scrollLeft = startScrollLeft - deltaX;
      topScroller.scrollLeft = scrollContent.scrollLeft;
      event.preventDefault();
    }, { passive: false });

    surface.addEventListener('pointerup', endDrag);
    surface.addEventListener('pointercancel', endDrag);
    surface.addEventListener('pointerleave', endDrag);
  };

  const bindTwoAxisDrag = surface => {
    if (!surface || surface.dataset.epgVerticalTwoAxisDragBound === 'true') {
      return;
    }

    surface.dataset.epgVerticalTwoAxisDragBound = 'true';

    let pointerActive = false;
    let dragging = false;
    let startX = 0;
    let startY = 0;
    let startScrollLeft = 0;
    let startScrollTop = 0;

    const endDrag = () => {
      if (dragging) {
        epgSuppressClickUntil = Date.now() + 260;
      }

      pointerActive = false;
      dragging = false;
      setDragging(surface, false);
    };

    surface.addEventListener('pointerdown', event => {
      if (event.button !== undefined && event.button !== 0) {
        return;
      }

      pointerActive = true;
      dragging = false;
      startX = event.clientX;
      startY = event.clientY;
      startScrollLeft = scrollContent.scrollLeft;
      startScrollTop = scrollContent.scrollTop;
    });

    surface.addEventListener('pointermove', event => {
      if (!pointerActive) {
        return;
      }

      const deltaX = event.clientX - startX;
      const deltaY = event.clientY - startY;

      if (!dragging && Math.hypot(deltaX, deltaY) < 7) {
        return;
      }

      if (!dragging) {
        dragging = true;
        setDragging(surface, true);

        if (surface.setPointerCapture && event.pointerId !== undefined) {
          surface.setPointerCapture(event.pointerId);
        }
      }

      scrollContent.scrollLeft = startScrollLeft - deltaX;
      scrollContent.scrollTop = startScrollTop - deltaY;
      topScroller.scrollLeft = scrollContent.scrollLeft;
      event.preventDefault();
    }, { passive: false });

    surface.addEventListener('pointerup', endDrag);
    surface.addEventListener('pointercancel', endDrag);
    surface.addEventListener('pointerleave', endDrag);
  };

  bindHorizontalDrag(topScroller);
  bindHorizontalDrag(dragSurface);
  bindTwoAxisDrag(scrollContent);

  requestAnimationFrame(updateTopScrollerWidth);
  setTimeout(updateTopScrollerWidth, 120);

  if (typeof ResizeObserver === 'function') {
    const observer = new ResizeObserver(updateTopScrollerWidth);
    observer.observe(grid);
    observer.observe(scrollContent);
  } else {
    window.addEventListener('resize', updateTopScrollerWidth);
  }
}


function createEpgVerticalTimeGrid(visibleChannels, eventIndex, bounds, nowSeconds) {
  const grid = document.createElement('section');
  grid.className = 'epg-vertical-time-grid';

  const topScroller = document.createElement('div');
  topScroller.className = 'epg-vertical-channel-scrollbar';
  topScroller.setAttribute('aria-label', 'Kanäle horizontal verschieben');

  const topScrollerInner = document.createElement('div');
  topScrollerInner.className = 'epg-vertical-channel-scrollbar-inner';
  topScroller.appendChild(topScrollerInner);

  const scrollContent = document.createElement('div');
  scrollContent.className = 'epg-vertical-scroll-content';

  const columnTemplate = epgVerticalColumnTemplate(visibleChannels.length);

  const headerRow = document.createElement('div');
  headerRow.className = 'epg-vertical-header-row';
  headerRow.style.gridTemplateColumns = columnTemplate;

  const corner = addText(document.createElement('div'), 'Zeit');
  corner.className = 'epg-vertical-corner';
  headerRow.appendChild(corner);

  visibleChannels.forEach((channel, index) => {
    headerRow.appendChild(createEpgVerticalChannelHeader(channel, index));
  });

  scrollContent.appendChild(headerRow);

  const body = document.createElement('div');
  body.className = 'epg-vertical-body';
  body.style.gridTemplateColumns = columnTemplate;

  const timeTrack = document.createElement('div');
  timeTrack.className = 'epg-vertical-time-track';
  appendEpgVerticalTimelineTicks(timeTrack, bounds, true);
  appendEpgVerticalNowLine(timeTrack, bounds, nowSeconds, true);
  body.appendChild(timeTrack);

  visibleChannels.forEach(channel => {
    const track = document.createElement('div');
    track.className = 'epg-vertical-channel-track';
    appendEpgVerticalTimelineTicks(track, bounds, false);
    appendEpgVerticalNowLine(track, bounds, nowSeconds, false);

    const channelEvents = epgIndexedEventsForChannel(eventIndex, channel);
    let renderedEvents = 0;

    channelEvents.forEach(entry => {
      const position = epgEventPositionForBounds(entry, bounds);

      if (!position) {
        return;
      }

      const card = createEpgEventCard(entry, channel);
      card.classList.add('epg-vertical-time-event');

      if (position.startsBeforeWindow) {
        card.classList.add('starts-before-window');
      }
      if (position.endsAfterWindow) {
        card.classList.add('ends-after-window');
      }

      card.style.top = position.left.toFixed(3) + '%';
      card.style.height = position.width.toFixed(3) + '%';
      card.title = epgEventTitle(entry.event)
        + ' · ' + (new Date(entry.start * 1000).toLocaleDateString('de-DE', { weekday: 'short', day: '2-digit', month: '2-digit' }) + ' ' + formatEpgClockFromEpoch(entry.start)
        + '–' + formatEpgClockFromEpoch(entry.end));

      track.appendChild(card);
      renderedEvents += 1;
    });

    if (renderedEvents === 0) {
      const empty = addText(document.createElement('p'), 'Kein EPG im sichtbaren Zeitfenster.');
      empty.className = 'epg-empty-channel epg-vertical-empty';
      track.appendChild(empty);
    }

    body.appendChild(track);
  });

  scrollContent.appendChild(body);

  grid.appendChild(topScroller);
  grid.appendChild(scrollContent);

  enableEpgVerticalHorizontalScroll(grid, topScroller, scrollContent, headerRow);

  return grid;
}

function appendEpgSidebarLine(parent, label, value) {
  const line = document.createElement('div');
  line.className = 'epg-sidebar-line';

  const key = addText(document.createElement('span'), label);
  key.className = 'epg-sidebar-label';

  const val = addText(document.createElement('strong'), String(value));
  val.className = 'epg-sidebar-value';

  line.appendChild(key);
  line.appendChild(val);
  parent.appendChild(line);
}

function renderEpgSidebar(channelData, eventData) {
  (void channelData);
  (void eventData);

  const aside = document.createElement('aside');
  aside.className = 'epg-sidebar epg-detail-sidebar';

  const detailHolder = document.createElement('div');
  detailHolder.className = 'epg-side-detail';
  detailHolder.dataset.epgSideDetail = 'true';

  aside.appendChild(detailHolder);

  return aside;
}

function renderEpgWorkbench(list, channelData, eventData) {
  const workbench = document.createElement('section');
  workbench.className = 'epg-workbench';

  const main = document.createElement('div');
  main.className = 'epg-workbench-main';
  main.appendChild(list);

  workbench.appendChild(main);
  workbench.appendChild(renderEpgSidebar(channelData, eventData));

  detailDataElement.appendChild(workbench);
  renderEpgSideDetail();
  alignEpgSideDetailToSource(null);
}

function epgCurrentEntryForChannel(channelEvents, nowSeconds) {
  return channelEvents
    .find(entry => entry.start <= nowSeconds && nowSeconds < entry.end) || null;
}

function epgNextEntryForChannel(channelEvents, nowSeconds) {
  return channelEvents
    .find(entry => entry.start > nowSeconds) || null;
}

function epgProgramProgressPercent(entry, nowSeconds) {
  if (!entry || entry.end <= entry.start) {
    return 0;
  }

  const value = ((nowSeconds - entry.start) / (entry.end - entry.start)) * 100;
  return Math.max(0, Math.min(100, value));
}

function createEpgProgramChannelHeader(channel, index) {
  const header = document.createElement('div');
  header.className = 'epg-program-channel';

  const channelTitleText = epgChannelTitle(channel, epgChannelOffset + index);
  const channelId = firstValue(channel, ['channelId', 'id', 'nativeId'], '');

  if (typeof createChannelLogoElement === 'function') {
    const logo = createChannelLogoElement(channelTitleText, channelId);
    logo.classList.add('epg-channel-logo');
    header.appendChild(logo);
  }

  const title = addText(document.createElement('h3'), channelTitleText);
  header.appendChild(title);

  return header;
}

function createEpgProgramEmpty(text) {
  const empty = addText(document.createElement('p'), text);
  empty.className = 'epg-program-empty';
  return empty;
}

function createEpgProgramEventButton(entry, channel, label, nowSeconds) {
  if (!entry) {
    return createEpgProgramEmpty(label + ': keine Sendung gefunden.');
  }

  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'epg-program-event';
  button.setAttribute('aria-label', 'EPG Details für ' + epgEventTitle(entry.event) + ' öffnen');

  const labelElement = addText(document.createElement('div'), label);
  labelElement.className = 'epg-program-label';
  button.appendChild(labelElement);

  const time = addText(
    document.createElement('div'),
    (new Date(entry.start * 1000).toLocaleDateString('de-DE', { weekday: 'short', day: '2-digit', month: '2-digit' }) + ' ' + formatEpgClockFromEpoch(entry.start) + '–' + formatEpgClockFromEpoch(entry.end))
  );
  time.className = 'epg-event-time';
  button.appendChild(time);

  const title = addText(document.createElement('div'), epgEventTitle(entry.event));
  title.className = 'epg-event-title';
  button.appendChild(title);

  const subtitle = epgEventSubtitle(entry.event);
  if (subtitle !== '') {
    const subtitleElement = addText(document.createElement('div'), subtitle);
    subtitleElement.className = 'epg-event-subtitle';
    button.appendChild(subtitleElement);
  }

  if (entry.start <= nowSeconds && nowSeconds < entry.end) {
    button.classList.add('current');

    const progress = document.createElement('div');
    progress.className = 'epg-program-progress';

    const bar = document.createElement('div');
    bar.className = 'epg-program-progress-bar';
    bar.style.width = epgProgramProgressPercent(entry, nowSeconds).toFixed(1) + '%';

    progress.appendChild(bar);
    button.appendChild(progress);

    const remainingMinutes = Math.max(0, Math.ceil((entry.end - nowSeconds) / 60));
    const remaining = addText(document.createElement('div'), 'endet in ' + String(remainingMinutes) + ' min');
    remaining.className = 'epg-program-remaining';
    button.appendChild(remaining);
  } else {
    button.classList.add('next');
  }

  button.addEventListener('click', clickEvent => {
    if (Date.now() < epgSuppressClickUntil) {
      clickEvent.preventDefault();
      return;
    }

    renderEpgEventDetail(entry.event, channel, clickEvent.currentTarget);
  });
  return button;
}

function createEpgProgramCard(channel, index, channelEvents, nowSeconds, viewMode) {
  const card = document.createElement('article');
  card.className = 'epg-program-card';

  card.appendChild(createEpgProgramChannelHeader(channel, index));

  const body = document.createElement('div');
  body.className = 'epg-program-events';

  const current = epgCurrentEntryForChannel(channelEvents, nowSeconds);
  const next = epgNextEntryForChannel(channelEvents, nowSeconds);

  if (viewMode === 'live') {
    body.appendChild(createEpgProgramEventButton(current, channel, 'läuft jetzt', nowSeconds));
    body.appendChild(createEpgProgramEventButton(next, channel, 'als nächstes', nowSeconds));
  } else if (viewMode === 'now') {
    body.appendChild(createEpgProgramEventButton(current, channel, 'läuft jetzt', nowSeconds));
  } else if (viewMode === 'next') {
    body.appendChild(createEpgProgramEventButton(next, channel, 'als nächstes', nowSeconds));
  }

  card.appendChild(body);
  return card;
}

function createEpgProgramViewGrid(visibleChannels, eventIndex, nowSeconds, viewMode) {
  const grid = document.createElement('section');
  grid.className = 'epg-program-grid epg-program-grid-' + viewMode;

  visibleChannels.forEach((channel, index) => {
    const channelEvents = epgIndexedEventsForChannel(eventIndex, channel);
    grid.appendChild(createEpgProgramCard(channel, index, channelEvents, nowSeconds, viewMode));
  });

  return grid;
}

function epgIsMobileViewport() {
  return typeof window !== 'undefined'
    && typeof window.matchMedia === 'function'
    && window.matchMedia('(max-width: 720px)').matches;
}

function epgTimeAxisViewIsSelected() {
  return epgProgramView === 'horizontal' || epgProgramView === 'vertical';
}

function renderEpgTimeView(channelData, eventData) {
  const mobileEpgViewport = epgIsMobileViewport();

  if (mobileEpgViewport && epgTimeAxisViewIsSelected()) {
    epgProgramView = 'live';
    epgTimeAxisMode = 'horizontal';
  }

  const channels = listFromResponse(channelData, 'channels');
  const events = listFromResponse(eventData, 'events');
  const nowSeconds = Math.floor(Date.now() / 1000);

  if (isMobileEpgLayout() && epgProgramView === 'horizontal') {
    epgProgramView = 'live';
    epgTimeAxisMode = 'horizontal';
  }

  const bounds = epgTimelineBounds(nowSeconds);
  const limit = Math.max(1, EPG_VISIBLE_CHANNEL_LIMIT);
  const maxChannelOffset = Math.max(0, Math.floor(Math.max(0, channels.length - 1) / limit) * limit);

  if (epgChannelOffset < 0 || epgChannelOffset > maxChannelOffset) {
    epgChannelOffset = maxChannelOffset;
  }

  const visibleChannels = channels.slice(epgChannelOffset, epgChannelOffset + limit);
  const visibleEventIndex = buildEpgVisibleEventIndex(visibleChannels, events, bounds);

  detailDataElement.replaceChildren();

  const list = document.createElement('section');
  list.className = 'list epg-timeline-module';

  const header = document.createElement('article');
  header.className = 'module-placeholder epg-timeline-intro';
  header.appendChild(addText(document.createElement('h3'), 'EPG Zeitleiste'));

  const rangeText = channels.length === 0
    ? 'Keine Kanäle gefunden.'
    : 'Zeige Kanäle ' + String(epgChannelOffset + 1) + '–' + String(epgChannelOffset + visibleChannels.length) + ' von ' + String(channels.length) + '.';

  const windowText = new Date(bounds.start * 1000).toLocaleDateString('de-DE', {
    weekday: 'short',
    day: '2-digit',
    month: '2-digit'
  }) + ' ' + formatEpgClockFromEpoch(bounds.start) + '–' + formatEpgClockFromEpoch(bounds.end);

  const nowText = new Date(nowSeconds * 1000).toLocaleDateString('de-DE', {
    weekday: 'short',
    day: '2-digit',
    month: '2-digit'
  }) + ' ' + formatEpgClockFromEpoch(nowSeconds);

  header.appendChild(addText(
    document.createElement('p'),
    rangeText
      + ' Zeitfenster: ' + windowText
      + ' · Jetzt: ' + nowText
      + ' · Quelle: ' + String(eventData.__source || 'cache')
      + ' · Events geladen: ' + String(events.length) + '.'
  ));

  const cacheStatus = addText(document.createElement('p'), epgWarmCacheStatus);
  cacheStatus.className = 'epg-cache-status';
  cacheStatus.dataset.epgCacheStatus = 'true';
  header.appendChild(cacheStatus);

  if (isMobileEpgLayout()) {
    const mobileHint = addText(
      document.createElement('p'),
      'Mobile Ansicht: Zeitachsen sind deaktiviert. Sendung antippen, die Kachel klappt mit Details auf.'
    );
    mobileHint.className = 'epg-mobile-mode-note';
    header.appendChild(mobileHint);
  }

  const modeRow = document.createElement('div');
  modeRow.className = 'epg-view-toggle';

  const timeView = document.createElement('button');
  timeView.type = 'button';
  timeView.className = 'epg-view-button ' + (epgProgramView === 'horizontal' ? 'active' : '');
  timeView.classList.add('epg-desktop-only');
  timeView.textContent = 'Zeit horizontal · Zeitachse oben';
  timeView.addEventListener('click', () => {
    epgTimelineMode = 'time';
    epgProgramView = 'horizontal';
    epgTimeAxisMode = 'horizontal';
    if (currentChannels && currentEvents) {
      renderEpgTimeView(currentChannels, currentEvents);
      return;
    }
    loadEpgTimeline();
  });

  const verticalTimeView = document.createElement('button');
  verticalTimeView.type = 'button';
  verticalTimeView.className = 'epg-view-button ' + (epgProgramView === 'vertical' ? 'active' : '');
  verticalTimeView.classList.add('epg-desktop-only');
  verticalTimeView.textContent = 'Zeit vertikal · Kanäle oben';
  verticalTimeView.addEventListener('click', () => {
    epgTimelineMode = 'time';
    epgProgramView = 'vertical';
    epgTimeAxisMode = 'vertical';
    if (currentChannels && currentEvents) {
      renderEpgTimeView(currentChannels, currentEvents);
      return;
    }
    loadEpgTimeline();
  });

  const liveView = document.createElement('button');
  liveView.type = 'button';
  liveView.className = 'epg-view-button ' + (epgProgramView === 'live' ? 'active' : '');
  liveView.textContent = 'Live-Liste';
  liveView.addEventListener('click', () => {
    epgTimelineMode = 'time';
    epgProgramView = 'live';
    epgTimeWindowPageOffset = 0;
    loadEpgTimeline();
  });

  const nowView = document.createElement('button');
  nowView.type = 'button';
  nowView.className = 'epg-view-button ' + (epgProgramView === 'now' ? 'active' : '');
  nowView.textContent = 'Läuft jetzt';
  nowView.addEventListener('click', () => {
    epgTimelineMode = 'time';
    epgProgramView = 'now';
    epgTimeWindowPageOffset = 0;
    loadEpgTimeline();
  });

  const nextView = document.createElement('button');
  nextView.type = 'button';
  nextView.className = 'epg-view-button ' + (epgProgramView === 'next' ? 'active' : '');
  nextView.textContent = 'Als nächstes';
  nextView.addEventListener('click', () => {
    epgTimelineMode = 'time';
    epgProgramView = 'next';
    epgTimeWindowPageOffset = 0;
    loadEpgTimeline();
  });

  if (!mobileEpgViewport) {
    modeRow.appendChild(timeView);
    modeRow.appendChild(verticalTimeView);
  }
  modeRow.appendChild(liveView);
  modeRow.appendChild(nowView);
  modeRow.appendChild(nextView);
  header.appendChild(modeRow);

  const pager = document.createElement('div');
  pager.className = 'epg-pager';

  const previous = document.createElement('button');
  previous.type = 'button';
  previous.textContent = 'Vorherige ' + String(limit);
  previous.disabled = epgChannelOffset <= 0;
  previous.addEventListener('click', () => {
    epgChannelOffset = Math.max(0, epgChannelOffset - limit);
    loadEpgTimeline();
  });

  const next = document.createElement('button');
  next.type = 'button';
  next.textContent = 'Nächste ' + String(limit);
  next.disabled = epgChannelOffset + limit >= channels.length;
  next.addEventListener('click', () => {
    epgChannelOffset = epgChannelOffset + limit;
    loadEpgTimeline();
  });

  pager.appendChild(previous);
  pager.appendChild(next);
  header.appendChild(pager);

  const timePager = document.createElement('div');
  timePager.className = 'epg-time-window-pager';
  timePager.classList.add('epg-desktop-only');

  const current24h = document.createElement('button');
  current24h.type = 'button';
  current24h.textContent = 'Aktuelle 24h';
  current24h.className = 'epg-view-button ' + (epgTimeWindowPageOffset === 0 ? 'active' : '');
  current24h.disabled = epgTimeWindowPageOffset === 0;
  current24h.addEventListener('click', () => {
    epgTimeWindowPageOffset = 0;
    loadEpgTimeline();
  });

  const next24h = document.createElement('button');
  next24h.type = 'button';
  next24h.textContent = 'Nächste 24h';
  next24h.className = 'epg-view-button ' + (epgTimeWindowPageOffset === 1 ? 'active' : '');
  next24h.disabled = epgTimeWindowPageOffset === 1;
  next24h.addEventListener('click', () => {
    epgTimeWindowPageOffset = 1;

    if (epgProgramView === 'live' || epgProgramView === 'now' || epgProgramView === 'next') {
      epgProgramView = 'horizontal';
      epgTimeAxisMode = 'horizontal';
    }

    loadEpgTimeline();
  });

  timePager.appendChild(current24h);
  timePager.appendChild(next24h);
  header.appendChild(timePager);


  list.appendChild(header);

  if (visibleChannels.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(addText(document.createElement('h3'), 'Keine EPG-Kanäle'));
    empty.appendChild(addText(document.createElement('p'), 'Die Kanalliste ist leer oder der Offset liegt außerhalb der Kanalliste.'));
    list.appendChild(empty);
    renderEpgWorkbench(list, channelData, eventData);
    return;
  }

  if (epgProgramView === 'live' || epgProgramView === 'now' || epgProgramView === 'next') {
    const programGrid = createEpgProgramViewGrid(visibleChannels, visibleEventIndex, nowSeconds, epgProgramView);
    enableEpgDragPan(programGrid, programGrid, {
      classTarget: programGrid,
      verticalWindow: true
    });
    list.appendChild(programGrid);
    renderEpgWorkbench(list, channelData, eventData);
    return;
  }

  if (epgProgramView === 'vertical' || epgTimeAxisMode === 'vertical') {
    const verticalGrid = createEpgVerticalTimeGrid(visibleChannels, visibleEventIndex, bounds, nowSeconds);
    list.appendChild(verticalGrid);
    renderEpgWorkbench(list, channelData, eventData);
    return;
  }

  const grid = document.createElement('section');
  grid.className = 'epg-time-grid';

  const scaleRow = document.createElement('div');
  scaleRow.className = 'epg-time-scale-row';

  const scaleChannel = addText(document.createElement('div'), 'Kanal');
  scaleChannel.className = 'epg-time-scale-channel';
  scaleRow.appendChild(scaleChannel);

  const scaleTrack = document.createElement('div');
  scaleTrack.className = 'epg-time-scale-track';
  appendEpgTimelineTicks(scaleTrack, bounds, true);
  appendEpgNowLine(scaleTrack, bounds, nowSeconds, true);
  scaleRow.appendChild(scaleTrack);
  grid.appendChild(scaleRow);

  visibleChannels.forEach((channel, index) => {
    const row = document.createElement('article');
    row.className = 'epg-time-row';

    const channelHeader = document.createElement('div');
    channelHeader.className = 'epg-time-row-channel';

    const channelTitleText = epgChannelTitle(channel, epgChannelOffset + index);
    const channelId = firstValue(channel, ['channelId', 'id', 'nativeId'], '');

    if (typeof createChannelLogoElement === 'function') {
      const logo = createChannelLogoElement(channelTitleText, channelId);
      logo.classList.add('epg-channel-logo');
      channelHeader.appendChild(logo);
    }

    const channelTitle = addText(document.createElement('h3'), channelTitleText);
    channelHeader.appendChild(channelTitle);
    row.appendChild(channelHeader);

    const track = document.createElement('div');
    track.className = 'epg-time-row-track';
    appendEpgTimelineTicks(track, bounds, false);
    appendEpgNowLine(track, bounds, nowSeconds, false);

    const channelEvents = epgIndexedEventsForChannel(visibleEventIndex, channel);
    let renderedEvents = 0;

    channelEvents.forEach(entry => {
      const position = epgEventPositionForBounds(entry, bounds);

      if (!position) {
        return;
      }

      const card = createEpgEventCard(entry, channel);
      card.classList.add('epg-time-event');
      if (position.startsBeforeWindow) {
        card.classList.add('starts-before-window');
      }
      if (position.endsAfterWindow) {
        card.classList.add('ends-after-window');
      }

      card.style.left = position.left.toFixed(3) + '%';
      card.style.width = position.width.toFixed(3) + '%';
      card.title = epgEventTitle(entry.event)
        + ' · ' + (new Date(entry.start * 1000).toLocaleDateString('de-DE', { weekday: 'short', day: '2-digit', month: '2-digit' }) + ' ' + formatEpgClockFromEpoch(entry.start)
        + '–' + formatEpgClockFromEpoch(entry.end));

      track.appendChild(card);
      renderedEvents += 1;
    });

    if (renderedEvents === 0) {
      const empty = addText(document.createElement('p'), 'Kein EPG im sichtbaren Zeitfenster.');
      empty.className = 'epg-empty-channel epg-time-empty';
      track.appendChild(empty);
    }

    row.appendChild(track);
    grid.appendChild(row);
  });

  enableEpgDragPan(grid, grid, {
    classTarget: grid,
    verticalWindow: true
  });

  list.appendChild(grid);
  renderEpgWorkbench(list, channelData, eventData);
}

function renderEpgTimelineModePlaceholder() {
  detailDataElement.replaceChildren();

  const list = document.createElement('section');
  list.className = 'list epg-timeline-module';

  const box = document.createElement('article');
  box.className = 'module-placeholder epg-timeline-preview';
  box.appendChild(addText(document.createElement('h3'), 'Timeline · 30 Kanäle'));
  box.appendChild(addText(
    document.createElement('p'),
    'Diese Ansicht kommt als nächster Schritt: Kanäle auf der Y-Achse, Zeit auf der X-Achse, 30 Kanäle pro Seite.'
  ));

  const back = document.createElement('button');
  back.type = 'button';
  back.textContent = 'Zur Zeitansicht zurück';
  back.addEventListener('click', () => {
    epgTimelineMode = 'time';
    if (currentChannels && currentEvents) {
      renderEpgTimeView(currentChannels, currentEvents);
      return;
    }
    loadEpgTimeline();
  });
  box.appendChild(back);

  list.appendChild(box);
  detailDataElement.appendChild(list);
}

function renderEpgTimelineLoading() {
  detailDataElement.replaceChildren();

  const box = document.createElement('section');
  box.className = 'module-placeholder epg-loading-box';

  box.appendChild(addText(document.createElement('h3'), 'EPG Zeitleiste'));
  box.appendChild(addText(
    document.createElement('p'),
    'Lade 24 Stunden EPG-Daten aus dem lokalen Daemon-Cache...'
  ));

  const progress = document.createElement('div');
  progress.className = 'epg-loading-progress';
  progress.setAttribute('role', 'progressbar');
  progress.setAttribute('aria-label', 'EPG wird geladen');

  const bar = document.createElement('div');
  bar.className = 'epg-loading-progress-bar';
  progress.appendChild(bar);

  box.appendChild(progress);

  const hint = addText(
    document.createElement('p'),
    'Kanäle und Sendungen werden lokal aus SQLite gelesen und danach im Browser gefiltert.'
  );
  hint.className = 'epg-loading-hint';
  box.appendChild(hint);

  detailDataElement.appendChild(box);
}

function selectedEpgBackendId() {
  if (selectedBackendId) {
    return selectedBackendId;
  }

  if (selectedBackend && selectedBackend.frontendSelector && selectedBackend.frontendSelector.id) {
    return selectedBackend.frontendSelector.id;
  }

  if (selectedBackend && selectedBackend.backendId) {
    return selectedBackend.backendId;
  }

  return 'default';
}

function updateEpgWarmCacheStatusText() {
  const status = detailDataElement.querySelector('[data-epg-cache-status]');
  if (status) {
    status.textContent = epgWarmCacheStatus;
  }
}

function epgWindowBounds() {
  const nowSeconds = Math.floor(Date.now() / 1000);
  const bounds = epgTimelineBounds(nowSeconds);

  return {
    from: bounds.start,
    until: bounds.end
  };
}

function formatEpgCacheTimestamp(epochSeconds) {
  const value = Number(epochSeconds);

  if (!Number.isFinite(value) || value <= 0) {
    return '-';
  }

  return new Date(value * 1000).toLocaleTimeString('de-DE', {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit'
  });
}

function formatEpgCacheDurationMs(durationMs) {
  const value = Number(durationMs);

  if (!Number.isFinite(value) || value <= 0) {
    return '-';
  }

  if (value < 1000) {
    return String(Math.round(value)) + 'ms';
  }

  return (value / 1000).toFixed(1).replace('.', ',') + 's';
}

function describeEpgCacheStatus(status, loadedEventCount) {
  if (!status) {
    return 'EPG-Cache-Status konnte nicht geladen werden.';
  }

  if (status.__statusError) {
    return 'EPG-Cache-Status konnte nicht geladen werden: ' + status.__statusError;
  }

  const eventCount = Number(status.eventCount || loadedEventCount || 0);
  const parts = [];

  parts.push(status.ready ? 'Cache bereit' : 'Cache noch leer');
  parts.push(String(eventCount) + ' Events in SQLite');

  if (status.lastRefreshKnown) {
    parts.push('letzter Warmup ' + formatEpgCacheTimestamp(status.lastRefreshFinishedAt));
    parts.push('Dauer ' + formatEpgCacheDurationMs(status.lastRefreshDurationMs));

    if (status.lastRefreshStored !== true) {
      parts.push('letzter Warmup nicht gespeichert');
    }
  } else {
    parts.push('noch kein Warmup seit Daemon-Start erfasst');
  }

  if (status.lastError) {
    parts.push('Fehler: ' + String(status.lastError));
  }

  return parts.join(' · ');
}

function fetchEpgCacheStatusForBackend(backendId) {
  const statusQuery = {
    backend: backendId,
    _: String(Date.now())
  };
  const clientApi = window.VdrSuiteClientApi;

  if (!clientApi || typeof clientApi.fetchClientEpgCacheStatus !== 'function') {
    return Promise.resolve({
      __statusError: 'Client API wrapper is not available'
    });
  }

  return clientApi.fetchClientEpgCacheStatus({
    query: statusQuery,
    cache: 'no-store'
  })
    .catch(error => ({
      __statusError: error.message
    }));
}

function listEventsFromEpgResponse(data) {
  return listFromResponse(data, 'events');
}


function visibleEpgChannelIds(visibleChannels) {
  const seen = new Set();
  const channelIds = [];

  visibleChannels.forEach(channel => {
    const channelId = frontendChannelId(channel);

    if (channelId === '' || seen.has(channelId)) {
      return;
    }

    seen.add(channelId);
    channelIds.push(channelId);
  });

  return channelIds;
}

function fetchCachedEpgWindowForVisibleChannels(visibleChannels) {
  const channelIds = visibleEpgChannelIds(visibleChannels);

  if (channelIds.length === 0) {
    return Promise.resolve({
      events: [],
      eventCount: 0,
      __source: 'empty-channel',
      __partialWindow: true,
      __debugUrl: 'keine sichtbaren Kanal-IDs'
    });
  }

  const backendId = selectedEpgBackendId();
  const bounds = epgWindowBounds();
  const clientApi = window.VdrSuiteClientApi;

  if (!clientApi || typeof clientApi.fetchClientEpgCacheWindow !== 'function') {
    return Promise.reject(new Error('Client API wrapper is not available'));
  }

  return clientApi.fetchClientEpgCacheWindow({
    query: {
      backend: backendId,
      channelIds: channelIds.join(','),
      fromTime: String(bounds.from),
      untilTime: String(bounds.until),
      limit: '0',
      _: String(Date.now())
    },
    cache: 'no-store'
  })
    .then(data => {
      data.__source = 'cache-visible-batch-24h';
      data.__partialWindow = true;
      data.__debugUrl = '1 sichtbare Kanal-Batch-Abfrage · Kanäle: ' + String(channelIds.length);
      return data;
    });
}

function fetchVisibleCachedEpgWindow(channelData) {
  const visibleChannels = visibleEpgChannelsFromData(channelData);

  if (visibleChannels.length === 0) {
    return Promise.resolve({
      events: [],
      eventCount: 0,
      __source: 'cache-empty',
      __partialWindow: true,
      __debugUrl: 'keine sichtbaren Kanäle'
    });
  }

  return fetchCachedEpgWindowForVisibleChannels(visibleChannels);
}

function fetchCachedOrLiveEpgWindow(channelData) {
  const backendId = selectedEpgBackendId();
  const statusRequest = fetchEpgCacheStatusForBackend(backendId);

  return Promise.all([fetchVisibleCachedEpgWindow(channelData), statusRequest])
    .then(([data, status]) => {
      const events = listEventsFromEpgResponse(data);
      data.__cacheStatus = status;

      epgWarmCacheStatus = describeEpgCacheStatus(status, events.length);
      updateEpgWarmCacheStatusText();

      return data;
    });
}

function visibleEpgChannelsFromData(channelData) {
  const channels = listFromResponse(channelData, 'channels');
  const limit = EPG_VISIBLE_CHANNEL_LIMIT;
  const maxOffset = Math.max(0, channels.length - limit);

  if (epgChannelOffset > maxOffset) {
    epgChannelOffset = maxOffset;
  }

  if (epgChannelOffset < 0) {
    epgChannelOffset = 0;
  }

  return channels.slice(epgChannelOffset, epgChannelOffset + limit);
}

function loadEpgTimeline() {
  renderEpgTimelineLoading();

  const clientApi = window.VdrSuiteClientApi;

  if (!clientApi || typeof clientApi.fetchClientChannels !== 'function') {
    currentChannels = null;
    currentEvents = null;
    if (typeof epgLoadedBackendId !== 'undefined') {
      epgLoadedBackendId = '';
    }
    renderModuleError(
      'EPG Zeitleiste konnte nicht geladen werden',
      new Error('Client API wrapper is not available')
    );
    return;
  }

  const channelsRequest = clientApi.fetchClientChannels({
    cache: 'no-store'
  });

  channelsRequest
    .then(channelData => fetchCachedOrLiveEpgWindow(channelData)
      .then(eventData => [channelData, eventData]))
    .then(([channelData, eventData]) => {
      currentChannels = channelData;
      currentEvents = eventData;
      if (typeof epgLoadedBackendId !== 'undefined') {
        epgLoadedBackendId = selectedEpgBackendId();
      }

      if (epgTimelineMode === 'timeline') {
        renderEpgTimelineModePlaceholder();
        return;
      }

      renderEpgTimeView(channelData, eventData);
    })
    .catch(error => {
      currentChannels = null;
      currentEvents = null;
      if (typeof epgLoadedBackendId !== 'undefined') {
        epgLoadedBackendId = '';
      }
      renderModuleError('EPG Zeitleiste konnte nicht geladen werden', error);
    });
}

function renderEpgTimelinePlaceholder(data) {
  (void data);

  if (currentChannels && currentEvents) {
    if (typeof epgLoadedBackendId !== 'undefined' && epgLoadedBackendId !== selectedEpgBackendId()) {
      loadEpgTimeline();
      return;
    }

    if (epgTimelineMode === 'timeline') {
      renderEpgTimelineModePlaceholder();
      return;
    }

    renderEpgTimeView(currentChannels, currentEvents);
    return;
  }

  loadEpgTimeline();
}

function renderModulePlaceholder(moduleName, data) {
  const countMap = {
    searchtimers: 0
  };
  const endpointMap = {
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
  renderModuleLoading('Kanäle', 'Lade Kanalliste...');

  const backendId = selectedEpgBackendId();
  const channelQuery = {
    backend: backendId,
    _: String(Date.now())
  };
  const url = '/api/vdr/channels'
    + '?backend=' + encodeURIComponent(backendId)
    + '&_=' + encodeURIComponent(channelQuery._);
  const clientApi = window.VdrSuiteClientApi;

  if (!clientApi || typeof clientApi.fetchClientChannels !== 'function') {
    currentChannels = null;
    currentEvents = null;
    renderModuleError(
      'Kanalliste konnte nicht geladen werden',
      new Error('Client API wrapper is not available')
    );
    return;
  }

  clientApi.fetchClientChannels({
    query: channelQuery,
    cache: 'no-store',
    credentials: 'same-origin'
  })
    .then(channelData => {
      currentChannels = channelData;
      currentEvents = null;

      renderChannelList(channelData);

      return fetchCachedOrLiveEpgWindow(channelData)
        .then(eventData => {
          currentEvents = eventData;

          if (selectedModule !== 'channels') {
            return;
          }

          const enrichedChannelData = Array.isArray(channelData)
            ? { channels: channelData }
            : Object.assign({}, channelData);

          enrichedChannelData.events = listEventsFromEpgResponse(eventData);
          enrichedChannelData.__epgSource = String(eventData.__source || 'cache');
          enrichedChannelData.__epgDebugUrl = String(eventData.__debugUrl || '');

          renderChannelList(enrichedChannelData);
        })
        .catch(error => {
          (void error);
          currentEvents = null;
        });
    })
    .catch(error => {
      currentChannels = null;
      currentEvents = null;

      detailDataElement.replaceChildren();

      const box = document.createElement('article');
      box.className = 'module-placeholder';
      box.appendChild(addText(document.createElement('h3'), 'Kanalliste konnte nicht geladen werden'));
      box.appendChild(addText(document.createElement('p'), error.message));
      box.appendChild(addText(document.createElement('p'), url));
      detailDataElement.appendChild(box);
    });
}

function loadTimers() {
  renderModuleLoading('Timer', 'Lade aktuelle Timerliste direkt vom VDR...');

  const clientApi = window.VdrSuiteClientApi;

  if (!clientApi || typeof clientApi.fetchClientTimers !== 'function') {
    currentTimers = null;
    renderModuleError(
      'Timer konnten nicht geladen werden',
      new Error('Client API wrapper is not available')
    );
    return;
  }

  clientApi.fetchClientTimers()
    .then(data => {
      currentTimers = data;
      renderTimerList(data);
      loadTimerConflictPanel(listFromResponse(data, "timers"));
    })
    .catch(error => {
      currentTimers = null;
      renderModuleError('Timer konnten nicht geladen werden', error);
    });
}

function loadSearchTimers() {
  renderModuleLoading('SearchTimer', 'Lade SearchTimer...');

  fetch('/api/vdr/searchtimers')
    .then(response => {
      if (response.ok) {
        return response.json();
      }

      return fetch('/api/searchtimers')
        .then(fallbackResponse => {
          if (!fallbackResponse.ok) {
            throw new Error('HTTP ' + fallbackResponse.status);
          }
          return fallbackResponse.json();
        });
    })
    .then(data => {
      currentSearchTimers = data;
      renderSearchTimerList(data);
    })
    .catch(error => {
      currentSearchTimers = null;
      renderModuleError('SearchTimer konnten nicht geladen werden', error);
    });
}

function loadRecordings() {
  renderModuleLoading('Aufnahmen', 'Lade Aufnahmeliste aus /api/vdr/recordings/query...');

  const clientApi = window.VdrSuiteClientApi;
  const backendId = selectedEpgBackendId();

  if (!clientApi || typeof clientApi.fetchClientRecordings !== 'function') {
    currentRecordings = null;
    renderModuleError(
      'Aufnahmen konnten nicht geladen werden',
      new Error('Client API wrapper is not available')
    );
    return;
  }

  clientApi.fetchClientRecordings({
    query: {
      backend: backendId,
      limit: 0,
      _: String(Date.now())
    },
    cache: 'no-store',
    credentials: 'same-origin'
  })
    .then(data => {
      currentRecordings = data;
      renderRecordingList(data);
    })
    .catch(error => {
      currentRecordings = null;
      renderModuleError('Aufnahmen konnten nicht geladen werden', error);
    });
}

function appendSettingsLine(parent, label, value) {
  const row = document.createElement('div');
  row.className = 'settings-line';

  const labelElement = addText(document.createElement('span'), label);
  labelElement.className = 'settings-label';

  const valueElement = addText(document.createElement('strong'), value === undefined || value === null || value === '' ? '-' : String(value));
  valueElement.className = 'settings-value';

  row.appendChild(labelElement);
  row.appendChild(valueElement);
  parent.appendChild(row);
}

function settingsBoolean(value) {
  return value ? 'ja' : 'nein';
}

function renderSettingsView(data) {
  detailDataElement.replaceChildren();

  const panel = document.createElement('section');
  panel.className = 'settings-panel';

  const intro = document.createElement('article');
  intro.className = 'module-placeholder settings-card';
  intro.appendChild(addText(document.createElement('h3'), 'Einstellungen'));
  intro.appendChild(addText(
    document.createElement('p'),
    'Zentrale Oberfläche für Frontend-Einstellungen, Backend-Auswahl und technische Backend-Informationen.'
  ));
  panel.appendChild(intro);

  const backend = selectedBackend || {};
  const selector = backend.frontendSelector || backend;
  const backendId = selectedEpgBackendId();

  const backendCard = document.createElement('article');
  backendCard.className = 'module-placeholder settings-card';
  backendCard.appendChild(addText(document.createElement('h3'), 'Backendinfo'));

  appendSettingsLine(backendCard, 'Name', selector.label || backend.backendName || backendId);
  appendSettingsLine(backendCard, 'Backend-ID', backendId);
  appendSettingsLine(backendCard, 'Online', settingsBoolean(Boolean(backend.online)));
  appendSettingsLine(backendCard, 'Zugriff', selector.accessMode || backend.accessMode || '-');
  appendSettingsLine(backendCard, 'Schreiben', settingsBoolean(Boolean(selector.canWrite)));
  appendSettingsLine(backendCard, 'Aufnahmen', settingsBoolean(Boolean(selector.canWriteRecordings)));
  appendSettingsLine(backendCard, 'Timer', settingsBoolean(Boolean(selector.canWriteTimers)));
  appendSettingsLine(backendCard, 'SearchTimer', settingsBoolean(Boolean(selector.canWriteSearchTimers)));

  panel.appendChild(backendCard);

  const runtimeCard = document.createElement('article');
  runtimeCard.className = 'module-placeholder settings-card';
  runtimeCard.appendChild(addText(document.createElement('h3'), 'Frontend'));

  appendSettingsLine(runtimeCard, 'Aktives Modul', selectedModule);
  appendSettingsLine(runtimeCard, 'EPG-Ansicht', epgTimeAxisMode === 'vertical' ? 'Zeit vertikal' : 'Zeit horizontal');
  appendSettingsLine(runtimeCard, 'EPG-Kanaloffset', epgChannelOffset);
  appendSettingsLine(runtimeCard, 'EPG-Zeitfenster', epgTimeWindowPageOffset === 0 ? 'aktuelle 24h' : 'nächste 24h');

  if (data && typeof data === 'object') {
    appendSettingsLine(runtimeCard, 'Snapshot-Sequenz', firstValue(data, ['sequence', 'snapshotSequence', 'snapshotId'], '-'));
    appendSettingsLine(runtimeCard, 'Live-Status', firstValue(data, ['liveStatus', 'status'], '-'));
  }

  panel.appendChild(runtimeCard);

  detailDataElement.appendChild(panel);
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

  if (selectedModule === 'channelsort') {
    loadChannelSorter();
    return;
  }

  if (selectedModule === 'epg') {
    renderEpgTimelinePlaceholder(data);
    return;
  }

  if (selectedModule === 'timers') {
    loadTimers();
    return;
  }

  if (selectedModule === 'recordings') {
    loadRecordings();
    return;
  }

  if (selectedModule === 'searchtimers') {
    loadSearchTimers();
    return;
  }

  if (selectedModule === 'settings') {
    renderSettingsView(data);
    return;
  }

  renderModulePlaceholder(selectedModule, data);
}

function selectModule(moduleName) {
  selectedModule = moduleName;

  if (moduleName === 'channels') {
    if (typeof selectedChannelId !== 'undefined') {
      selectedChannelId = '';
    }
    if (typeof selectedChannel !== 'undefined') {
      selectedChannel = null;
    }
    if (typeof selectedChannelNumber !== 'undefined') {
      selectedChannelNumber = null;
    }
    if (typeof selectedChannelDetail !== 'undefined') {
      selectedChannelDetail = null;
    }
  }

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

document.querySelectorAll('[data-brand-module]').forEach(button => {
  const openModule = () => {
    const moduleName = button.dataset.brandModule;
    if (!moduleName) {
      return;
    }

    selectModule(moduleName);

    if (detailDataElement && typeof detailDataElement.scrollIntoView === 'function') {
      detailDataElement.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
  };

  button.addEventListener('click', openModule);
  button.addEventListener('keydown', event => {
    if (event.key === 'Enter' || event.key === ' ') {
      event.preventDefault();
      openModule();
    }
  });
});

refreshDetailButton.addEventListener('click', () => {
  if (!selectedBackend) {
    return;
  }

  if (selectedModule === 'channels') {
    loadChannels();
    return;
  }

  if (selectedModule === 'channelsort') {
    loadChannelSorter();
    return;
  }

  if (selectedModule === 'epg') {
    renderSelectedModule(currentSnapshot || {});
    return;
  }

  if (selectedModule === 'timers') {
    loadTimers();
    return;
  }

  if (selectedModule === 'recordings') {
    loadRecordings();
    return;
  }

  if (selectedModule === 'searchtimers') {
    loadSearchTimers();
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
    statusElement.textContent = '';
    statusElement.hidden = true;
    backendsElement.replaceChildren();
    backends.forEach(backend => backendsElement.appendChild(renderBackend(backend)));
    if (backends.length > 0) {
      loadBackendDetails(backends[0]);
    }
  })
  .catch(error => {
    statusElement.hidden = false;
    statusElement.className = 'status error';
    statusElement.textContent = 'Backend-Auswahl konnte nicht geladen werden: ' + error.message;
  });

// Phase 58.90b: integrated standalone pointer channel sorter.
// Stable version: drag only on the left handle, no post-move focus restore.

let channelSorterData = null;
let channelSorterBusy = false;
let channelSorterPointer = null;

function channelSorterBackendId() {
  if (typeof selectedEpgBackendId === 'function') {
    const backendId = selectedEpgBackendId();
    if (String(backendId || '').trim() !== '') {
      return String(backendId);
    }
  }

  if (typeof selectedBackendId !== 'undefined' && String(selectedBackendId || '').trim() !== '') {
    return String(selectedBackendId);
  }

  return 'default';
}

function channelSorterNumber(channel, fallback) {
  const value = Number(firstValue(channel, ['number', 'channelNumber', 'position'], fallback));
  return Number.isFinite(value) && value > 0 ? value : fallback;
}

function channelSorterTitle(channel, index) {
  return String(firstValue(
    channel,
    ['name', 'channelName', 'title', 'displayName', 'id', 'channelId'],
    'Kanal ' + String(index + 1)
  ));
}

function channelSorterId(channel) {
  return String(firstValue(channel, ['channelId', 'id', 'nativeId'], ''));
}

function channelSorterChannels(data) {
  return listFromResponse(data, 'channels')
    .slice()
    .sort((left, right) => {
      const numberDiff = channelSorterNumber(left, 999999) - channelSorterNumber(right, 999999);
      if (numberDiff !== 0) {
        return numberDiff;
      }

      return channelSorterTitle(left, 0).localeCompare(channelSorterTitle(right, 0), 'de-DE');
    });
}

function channelSorterSetStatus(message, error) {
  const status = document.querySelector('.channel-sorter-status');
  if (!status) {
    return;
  }

  status.textContent = message;
  status.classList.toggle('error', Boolean(error));
}

function channelSorterApiMove(sourceNumber, targetNumber) {
  return fetch('/api/vdr/channels/move', {
    method: 'POST',
    cache: 'no-store',
    credentials: 'same-origin',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      backendId: channelSorterBackendId(),
      sourceNumber: sourceNumber,
      targetNumber: targetNumber
    })
  })
    .then(response => response.json()
      .catch(() => ({}))
      .then(data => ({ response, data })))
    .then(result => {
      const response = result.response;
      const data = result.data || {};

      if (!response.ok || data.success !== true) {
        const message = data.message || data.error || ('HTTP ' + String(response.status));
        throw new Error(message);
      }

      return data;
    });
}

function channelSorterClearDragVisuals() {
  document.querySelectorAll('.channel-sorter-card').forEach(card => {
    card.classList.remove('drag-source');
    card.classList.remove('drop-target');
  });

  const ghost = document.querySelector('.channel-sorter-ghost');
  if (ghost) {
    ghost.remove();
  }

  document.body.classList.remove('channel-sorter-dragging');
}

function channelSorterTargetAt(list, channels, clientX, clientY) {
  const element = document.elementFromPoint(clientX, clientY);
  const card = element ? element.closest('.channel-sorter-card') : null;

  if (!card || !list.contains(card)) {
    return null;
  }

  const index = Number(card.dataset.index);
  if (!Number.isFinite(index) || index < 0 || index >= channels.length) {
    return null;
  }

  return { card, index };
}

function channelSorterAutoScroll(list, clientY) {
  const rect = list.getBoundingClientRect();
  const edge = 76;
  const step = 24;

  if (clientY < rect.top + edge) {
    list.scrollTop = Math.max(0, list.scrollTop - step);
    return;
  }

  if (clientY > rect.bottom - edge) {
    list.scrollTop += step;
  }
}

function channelSorterFinishMove(source, targetIndex, channels) {
  if (!source || channelSorterBusy) {
    return;
  }

  if (targetIndex === source.index) {
    channelSorterSetStatus('Quelle und Ziel sind identisch.');
    return;
  }

  const targetChannel = channels[targetIndex];
  if (!targetChannel) {
    channelSorterSetStatus('Kein Ziel gewählt.', true);
    return;
  }

  const sourceNumber = channelSorterNumber(source.channel, source.index + 1);
  const targetNumber = channelSorterNumber(targetChannel, targetIndex + 1);
  const sourceTitle = channelSorterTitle(source.channel, source.index);
  const targetTitle = channelSorterTitle(targetChannel, targetIndex);

  if (sourceNumber === targetNumber) {
    channelSorterSetStatus('Quelle und Ziel sind identisch.');
    return;
  }

  const confirmed = window.confirm(
    'Kanal verschieben?\n\n' +
    sourceTitle + ' von Nr. ' + String(sourceNumber) +
    ' auf Position Nr. ' + String(targetNumber) + '.\n' +
    'Zielposition aktuell: ' + targetTitle + '.'
  );

  if (!confirmed) {
    channelSorterSetStatus('Verschieben abgebrochen.');
    return;
  }

  channelSorterBusy = true;
  channelSorterSetStatus(
    'Verschiebe ' + sourceTitle + ' von Nr. ' + String(sourceNumber) +
    ' auf Nr. ' + String(targetNumber) + '...'
  );

  channelSorterApiMove(sourceNumber, targetNumber)
    .then(() => {
      channelSorterBusy = false;
      channelSorterSetStatus('Kanal verschoben. Lade Kanalliste neu...');
      loadChannelSorter();
    })
    .catch(error => {
      channelSorterBusy = false;
      channelSorterSetStatus('Kanal konnte nicht verschoben werden: ' + error.message, true);
      renderChannelSorter(channelSorterData);
    });
}

function channelSorterBeginPointerDrag(event, card, channel, index, channels, list) {
  if (channelSorterBusy || channelSorterPointer) {
    return;
  }

  if (event.pointerType === 'mouse' && event.button !== undefined && event.button !== 0) {
    return;
  }

  event.preventDefault();
  event.stopPropagation();

  channelSorterClearDragVisuals();

  const rect = card.getBoundingClientRect();
  const ghost = card.cloneNode(true);
  const offsetX = event.clientX - rect.left;
  const offsetY = event.clientY - rect.top;

  let targetIndex = index;

  channelSorterPointer = {
    pointerId: event.pointerId,
    channel,
    index,
    card,
    ghost
  };

  ghost.classList.add('channel-sorter-ghost');
  ghost.style.width = String(rect.width) + 'px';
  ghost.style.height = String(rect.height) + 'px';
  ghost.style.left = String(rect.left) + 'px';
  ghost.style.top = String(rect.top) + 'px';

  document.body.appendChild(ghost);
  document.body.classList.add('channel-sorter-dragging');
  card.classList.add('drag-source');

  if (card.setPointerCapture && event.pointerId !== undefined) {
    try {
      card.setPointerCapture(event.pointerId);
    } catch (captureError) {
      (void captureError);
    }
  }

  const moveGhost = moveEvent => {
    ghost.style.left = String(moveEvent.clientX - offsetX) + 'px';
    ghost.style.top = String(moveEvent.clientY - offsetY) + 'px';
  };

  const updateTarget = moveEvent => {
    moveGhost(moveEvent);
    channelSorterAutoScroll(list, moveEvent.clientY);

    list.querySelectorAll('.channel-sorter-card').forEach(item => {
      item.classList.remove('drop-target');
    });

    const hit = channelSorterTargetAt(list, channels, moveEvent.clientX, moveEvent.clientY);
    if (!hit || hit.index === index) {
      targetIndex = index;
      return;
    }

    targetIndex = hit.index;
    hit.card.classList.add('drop-target');

    const sourceTitle = channelSorterTitle(channel, index);
    const targetTitle = channelSorterTitle(channels[targetIndex], targetIndex);
    channelSorterSetStatus(sourceTitle + ' → Zielposition: ' + targetTitle);
  };

  const cleanup = () => {
    document.removeEventListener('pointermove', onPointerMove, true);
    document.removeEventListener('pointerup', onPointerUp, true);
    document.removeEventListener('pointercancel', onPointerCancel, true);

    if (card.releasePointerCapture && event.pointerId !== undefined) {
      try {
        card.releasePointerCapture(event.pointerId);
      } catch (releaseError) {
        (void releaseError);
      }
    }

    channelSorterClearDragVisuals();
    channelSorterPointer = null;
  };

  const onPointerMove = moveEvent => {
    if (!channelSorterPointer || moveEvent.pointerId !== channelSorterPointer.pointerId) {
      return;
    }

    moveEvent.preventDefault();
    moveEvent.stopPropagation();
    updateTarget(moveEvent);
  };

  const onPointerUp = upEvent => {
    if (!channelSorterPointer || upEvent.pointerId !== channelSorterPointer.pointerId) {
      return;
    }

    upEvent.preventDefault();
    upEvent.stopPropagation();

    updateTarget(upEvent);
    cleanup();

    channelSorterFinishMove({ channel, index }, targetIndex, channels);
  };

  const onPointerCancel = cancelEvent => {
    if (!channelSorterPointer || cancelEvent.pointerId !== channelSorterPointer.pointerId) {
      return;
    }

    cancelEvent.preventDefault();
    cancelEvent.stopPropagation();

    cleanup();
    channelSorterSetStatus('Verschieben abgebrochen.');
  };

  document.addEventListener('pointermove', onPointerMove, true);
  document.addEventListener('pointerup', onPointerUp, true);
  document.addEventListener('pointercancel', onPointerCancel, true);

  moveGhost(event);
  channelSorterSetStatus(
    'Quelle: ' + channelSorterTitle(channel, index) +
    '. Auf Zielkachel loslassen.'
  );
}

function createChannelSorterCard(channel, index, channels, list) {
  const card = document.createElement('article');
  card.className = 'channel-sorter-card';
  card.dataset.index = String(index);
  card.dataset.number = String(channelSorterNumber(channel, index + 1));
  card.tabIndex = 0;
  card.setAttribute('role', 'button');
  card.setAttribute('aria-label', channelSorterTitle(channel, index) + ' verschieben');

  card.addEventListener('dragstart', event => {
    event.preventDefault();
  }, true);

  const handle = addText(document.createElement('div'), '↕');
  handle.className = 'channel-sorter-handle';
  handle.setAttribute('aria-label', channelSorterTitle(channel, index) + ' ziehen');
  handle.addEventListener('pointerdown', event => {
    channelSorterBeginPointerDrag(event, card, channel, index, channels, list);
  }, { passive: false });
  card.appendChild(handle);

  const title = channelSorterTitle(channel, index);
  const channelId = channelSorterId(channel);

  if (typeof createChannelLogoElement === 'function') {
    const logo = createChannelLogoElement(title, channelId);
    logo.classList.add('epg-channel-logo');
    card.appendChild(logo);
  }

  const text = document.createElement('div');
  text.className = 'channel-sorter-text';

  const name = addText(document.createElement('div'), title);
  name.className = 'channel-sorter-title';
  text.appendChild(name);

  const meta = addText(
    document.createElement('div'),
    'Nr. ' + String(channelSorterNumber(channel, index + 1)) + ' · ' + String(channelId || '-')
  );
  meta.className = 'channel-sorter-meta';
  text.appendChild(meta);

  card.appendChild(text);

  return card;
}

function renderChannelSorter(data) {
  channelSorterData = data;

  const channels = channelSorterChannels(data);
  detailDataElement.replaceChildren();

  const shell = document.createElement('section');
  shell.className = 'channel-sorter-shell';

  const intro = document.createElement('article');
  intro.className = 'module-placeholder channel-sorter-intro';
  intro.appendChild(addText(document.createElement('h3'), 'Kanäle sortieren'));
  intro.appendChild(addText(
    document.createElement('p'),
    'Eigenständige Sortieroberfläche. Am linken ↕-Griff ziehen; auf dem Rest der Liste normal scrollen.'
  ));

  const toolbar = document.createElement('div');
  toolbar.className = 'channel-sorter-toolbar';

  const reload = document.createElement('button');
  reload.type = 'button';
  reload.textContent = 'Neu laden';
  reload.disabled = channelSorterBusy;
  reload.addEventListener('click', () => loadChannelSorter());
  toolbar.appendChild(reload);

  const status = addText(
    document.createElement('span'),
    channels.length === 0
      ? 'Keine Kanäle gefunden.'
      : String(channels.length) + ' Kanäle geladen.'
  );
  status.className = 'channel-sorter-status';
  toolbar.appendChild(status);

  intro.appendChild(toolbar);
  shell.appendChild(intro);

  if (channels.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(addText(document.createElement('h3'), 'Keine Kanäle gefunden'));
    empty.appendChild(addText(document.createElement('p'), 'Der Endpunkt /api/vdr/channels hat keine Kanäle geliefert.'));
    shell.appendChild(empty);
    detailDataElement.appendChild(shell);
    return;
  }

  const list = document.createElement('section');
  list.className = 'channel-sorter-list';
  list.setAttribute('aria-label', 'Kanalliste sortieren');

  channels.forEach((channel, index) => {
    list.appendChild(createChannelSorterCard(channel, index, channels, list));
  });

  shell.appendChild(list);
  detailDataElement.appendChild(shell);
}

function loadChannelSorter() {
  renderModuleLoading('Kanäle sortieren', 'Lade Kanalliste für Sortierung...');

  const backendId = channelSorterBackendId();
  const url = '/api/vdr/channels'
    + '?backend=' + encodeURIComponent(backendId)
    + '&_=' + encodeURIComponent(String(Date.now()));

  fetch(url, {
    cache: 'no-store',
    credentials: 'same-origin'
  })
    .then(response => {
      if (!response.ok) {
        throw new Error('Kanalliste HTTP ' + response.status);
      }

      return response.json();
    })
    .then(data => {
      channelSorterData = data;
      currentChannels = data;
      renderChannelSorter(data);
    })
    .catch(error => {
      channelSorterData = null;
      renderModuleError('Kanalsortierer konnte nicht geladen werden', error);
    });
}
