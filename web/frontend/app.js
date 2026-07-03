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

const EPG_TIMELINE_VISIBLE_SECONDS = 24 * 60 * 60;
const EPG_TIMELINE_TICK_SECONDS = 2 * 60 * 60;
const EPG_TIMELINE_CONTEXT_BEFORE_SECONDS = 0;
const EPG_TIMELINE_WINDOW_ANCHOR_SECONDS = 30 * 60;
const EPG_TIMELINE_MAX_PAGE_OFFSET = 1;

const moduleLabels = {
  overview: 'Übersicht',
  channels: 'Kanäle',
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

function renderRecordingList(data) {
  const recordings = listFromResponse(data, 'recordings');
  detailDataElement.replaceChildren();

  if (recordings.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(addText(document.createElement('h3'), 'Keine Aufnahmen gefunden'));
    empty.appendChild(addText(document.createElement('p'), 'Der Endpunkt /api/vdr/recordings hat aktuell keine Aufnahmen geliefert.'));
    detailDataElement.appendChild(empty);
    return;
  }

  const groups = groupRecordings(recordings);

  function renderFolderOverview() {
    detailDataElement.replaceChildren();

    const list = document.createElement('section');
    list.className = 'list';

    const header = document.createElement('article');
    header.className = 'module-placeholder';
    header.appendChild(addText(document.createElement('h3'), 'Aufnahme-Ordner'));
    header.appendChild(addText(
      document.createElement('p'),
      String(groups.size) + ' Ordner · ' + String(recordings.length) + ' Aufnahme(n)'
    ));
    list.appendChild(header);

    groups.forEach((items, folder) => {
      const item = document.createElement('article');
      item.className = 'list-item';
      item.tabIndex = 0;
      item.setAttribute('role', 'button');
      item.setAttribute('aria-label', 'Ordner ' + folder + ' öffnen');

      item.appendChild(addText(document.createElement('div'), folder)).className = 'list-title';
      item.appendChild(addText(
        document.createElement('div'),
        String(items.length) + ' Aufnahme(n) · antippen zum Öffnen'
      )).className = 'list-meta';

      const openFolder = () => renderFolderRecordings(folder, items);
      item.addEventListener('click', openFolder);
      item.addEventListener('keydown', event => {
        if (event.key === 'Enter' || event.key === ' ') {
          event.preventDefault();
          openFolder();
        }
      });

      list.appendChild(item);
    });

    detailDataElement.appendChild(list);
  }

  function renderFolderRecordings(folder, items) {
    detailDataElement.replaceChildren();

    const list = document.createElement('section');
    list.className = 'list';

    const header = document.createElement('article');
    header.className = 'module-placeholder';
    header.appendChild(addText(document.createElement('h3'), folder));
    header.appendChild(addText(document.createElement('p'), String(items.length) + ' Aufnahme(n)'));

    const backButton = document.createElement('button');
    backButton.type = 'button';
    backButton.textContent = 'Zurück zu Ordnern';
    backButton.addEventListener('click', renderFolderOverview);
    header.appendChild(backButton);

    list.appendChild(header);

    items.slice(0, 20).forEach(entry => {
      const recording = entry.recording;
      const item = document.createElement('article');
      item.className = 'list-item';

      const recordingId = firstValue(recording, ['recordingId', 'id', 'nativeId'], '-');
      const path = firstValue(recording, ['path', 'fileName', 'directory'], '-');
      const startTime = formatRecordingStart(firstValue(recording, ['startTime', 'start', 'date'], '-'));
      const duration = formatDurationSeconds(firstValue(recording, ['durationSeconds', 'duration'], 0));
      const size = formatSizeMb(firstValue(recording, ['sizeMb', 'sizeMB', 'size'], 0));

      item.appendChild(addText(document.createElement('div'), entry.title)).className = 'list-title';
      item.appendChild(addText(
        document.createElement('div'),
        'Start: ' + startTime + ' · Dauer: ' + duration + ' · Größe: ' + size
      )).className = 'list-meta';
      item.appendChild(addText(document.createElement('div'), 'Pfad: ' + String(path))).className = 'list-meta';
      item.appendChild(addText(document.createElement('div'), 'ID: ' + String(recordingId))).className = 'list-meta';

      list.appendChild(item);
    });

    if (items.length > 20) {
      const info = document.createElement('article');
      info.className = 'module-placeholder';
      info.appendChild(addText(
        document.createElement('p'),
        'Zeige 20 von ' + String(items.length) + ' Aufnahmen in diesem Ordner.'
      ));
      list.appendChild(info);
    }

    detailDataElement.appendChild(list);
  }

  renderFolderOverview();
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

function epgEventsForChannel(channel, events, nowSeconds) {
  (void nowSeconds);
  const channelId = frontendChannelId(channel);
  if (channelId === '') {
    return [];
  }

  return events
    .filter(event => frontendEventChannelId(event) === channelId)
    .map(event => {
      const start = parseFrontendEventEpoch(firstValue(event, ['startTime', 'start', 'beginTime'], ''));
      const end = frontendEventEnd(event, start);
      return { event, start, end };
    })
    .filter(entry => entry.start > 0 && entry.end > 0)
    .sort((left, right) => left.start - right.start);
}

function renderEpgEventDetail(event, channel) {
  const existing = detailDataElement.querySelector('.epg-event-detail');
  if (existing) {
    existing.remove();
  }

  const start = parseFrontendEventEpoch(firstValue(event, ['startTime', 'start', 'beginTime'], ''));
  const end = frontendEventEnd(event, start);
  const detail = document.createElement('article');
  detail.className = 'module-placeholder epg-event-detail';

  detail.appendChild(addText(document.createElement('h3'), epgEventTitle(event)));

  const meta = [
    'Kanal: ' + epgChannelTitle(channel, 0),
    formatEpgClockFromEpoch(start) + '–' + formatEpgClockFromEpoch(end)
  ];
  detail.appendChild(addText(document.createElement('p'), meta.join(' · ')));

  const subtitle = epgEventSubtitle(event);
  if (subtitle !== '') {
    detail.appendChild(addText(document.createElement('p'), subtitle));
  }

  const eventId = firstValue(event, ['eventId', 'id', 'nativeId'], '');
  const channelId = frontendEventChannelId(event);
  const technical = [];
  if (channelId !== '') {
    technical.push('channelId=' + channelId);
  }
  if (eventId !== '') {
    technical.push('eventId=' + String(eventId));
  }
  if (technical.length > 0) {
    detail.appendChild(addText(document.createElement('p'), technical.join(' · ')));
  }

  detailDataElement.appendChild(detail);
  detail.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
}

function createEpgEventCard(entry, channel) {
  const event = entry.event;
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'epg-event-card';
  button.setAttribute('aria-label', 'EPG Details für ' + epgEventTitle(event) + ' öffnen');

  const time = addText(
    document.createElement('div'),
    formatEpgClockFromEpoch(entry.start) + '–' + formatEpgClockFromEpoch(entry.end)
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

  button.addEventListener('click', () => renderEpgEventDetail(event, channel));
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

function epgEventsOverlappingBounds(channel, events, bounds) {
  return epgEventsForChannel(channel, events, bounds.start)
    .filter(entry => entry.end > bounds.start && entry.start < bounds.end);
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

function createEpgVerticalTimeGrid(visibleChannels, events, bounds, nowSeconds) {
  const grid = document.createElement('section');
  grid.className = 'epg-vertical-time-grid';

  const columnTemplate = '5.8rem repeat(' + String(Math.max(visibleChannels.length, 1)) + ', minmax(13rem, 1fr))';

  const headerRow = document.createElement('div');
  headerRow.className = 'epg-vertical-header-row';
  headerRow.style.gridTemplateColumns = columnTemplate;

  const corner = addText(document.createElement('div'), 'Zeit');
  corner.className = 'epg-vertical-corner';
  headerRow.appendChild(corner);

  visibleChannels.forEach((channel, index) => {
    headerRow.appendChild(createEpgVerticalChannelHeader(channel, index));
  });

  grid.appendChild(headerRow);

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

    const channelEvents = epgEventsOverlappingBounds(channel, events, bounds);
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
      card.style.height = Math.max(position.width, 3.2).toFixed(3) + '%';
      card.title = epgEventTitle(entry.event)
        + ' · ' + formatEpgClockFromEpoch(entry.start)
        + '–' + formatEpgClockFromEpoch(entry.end);

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

  grid.appendChild(body);
  return grid;
}

function renderEpgTimeView(channelData, eventData) {
  const channels = listFromResponse(channelData, 'channels');
  const events = listFromResponse(eventData, 'events');
  const nowSeconds = Math.floor(Date.now() / 1000);
  const bounds = epgTimelineBounds(nowSeconds);
  const limit = 5;
  const visibleChannels = channels.slice(epgChannelOffset, epgChannelOffset + limit);

  detailDataElement.replaceChildren();

  const list = document.createElement('section');
  list.className = 'list epg-timeline-module';

  const header = document.createElement('article');
  header.className = 'module-placeholder epg-timeline-intro';
  header.appendChild(addText(document.createElement('h3'), 'EPG Zeitleiste'));

  const rangeText = channels.length === 0
    ? 'Keine Kanäle gefunden.'
    : 'Zeige Kanäle ' + String(epgChannelOffset + 1) + '–' + String(epgChannelOffset + visibleChannels.length) + ' von ' + String(channels.length) + '.';

  const firstVisibleChannelEventCount = visibleChannels.length > 0
    ? epgEventsOverlappingBounds(visibleChannels[0], events, bounds).length
    : 0;

  const secondVisibleChannelEventCount = visibleChannels.length > 1
    ? epgEventsOverlappingBounds(visibleChannels[1], events, bounds).length
    : 0;

  header.appendChild(addText(
    document.createElement('p'),
    rangeText
      + ' Zeitfenster: ' + formatEpgClockFromEpoch(bounds.start) + '–' + formatEpgClockFromEpoch(bounds.end)
      + ' · Jetzt: ' + formatEpgClockFromEpoch(nowSeconds)
      + ' · Quelle: ' + String(eventData.__source || 'cache')
      + ' · URL: ' + String(eventData.__debugUrl || '-')
      + ' · Events geladen: ' + String(events.length)
      + ' · Kanal 1: ' + String(firstVisibleChannelEventCount) + ' Events'
      + ' · Kanal 2: ' + String(secondVisibleChannelEventCount) + ' Events.'
  ));

  const cacheStatus = addText(document.createElement('p'), epgWarmCacheStatus);
  cacheStatus.className = 'epg-cache-status';
  cacheStatus.dataset.epgCacheStatus = 'true';
  header.appendChild(cacheStatus);

  const modeRow = document.createElement('div');
  modeRow.className = 'epg-view-toggle';

  const timeView = document.createElement('button');
  timeView.type = 'button';
  timeView.className = 'epg-view-button ' + (epgTimeAxisMode === 'horizontal' ? 'active' : '');
  timeView.textContent = 'Zeit horizontal · Zeitachse oben';
  timeView.addEventListener('click', () => {
    epgTimelineMode = 'time';
    epgTimeAxisMode = 'horizontal';
    if (currentChannels && currentEvents) {
      renderEpgTimeView(currentChannels, currentEvents);
      return;
    }
    loadEpgTimeline();
  });

  const verticalTimeView = document.createElement('button');
  verticalTimeView.type = 'button';
  verticalTimeView.className = 'epg-view-button ' + (epgTimeAxisMode === 'vertical' ? 'active' : '');
  verticalTimeView.textContent = 'Zeit vertikal · Kanäle oben';
  verticalTimeView.addEventListener('click', () => {
    epgTimelineMode = 'time';
    epgTimeAxisMode = 'vertical';
    if (currentChannels && currentEvents) {
      renderEpgTimeView(currentChannels, currentEvents);
      return;
    }
    loadEpgTimeline();
  });

  const timelineView = document.createElement('button');
  timelineView.type = 'button';
  timelineView.className = 'epg-view-button';
  timelineView.textContent = 'Timeline · 30 Kanäle';
  timelineView.addEventListener('click', () => {
    epgTimelineMode = 'timeline';
    renderEpgTimelineModePlaceholder();
  });

  modeRow.appendChild(timeView);
  modeRow.appendChild(verticalTimeView);
  modeRow.appendChild(timelineView);
  header.appendChild(modeRow);

  const pager = document.createElement('div');
  pager.className = 'epg-pager';

  const previous = document.createElement('button');
  previous.type = 'button';
  previous.textContent = 'Vorherige 5';
  previous.disabled = epgChannelOffset <= 0;
  previous.addEventListener('click', () => {
    epgChannelOffset = Math.max(0, epgChannelOffset - limit);
    loadEpgTimeline();
  });

  const next = document.createElement('button');
  next.type = 'button';
  next.textContent = 'Nächste 5';
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
    detailDataElement.appendChild(list);
    return;
  }

  if (epgTimeAxisMode === 'vertical') {
    const verticalGrid = createEpgVerticalTimeGrid(visibleChannels, events, bounds, nowSeconds);
    list.appendChild(verticalGrid);
    detailDataElement.appendChild(list);
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

    const channelEvents = epgEventsOverlappingBounds(channel, events, bounds);
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
      card.style.width = Math.max(position.width, 1.4).toFixed(3) + '%';
      card.title = epgEventTitle(entry.event)
        + ' · ' + formatEpgClockFromEpoch(entry.start)
        + '–' + formatEpgClockFromEpoch(entry.end);

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

  list.appendChild(grid);
  detailDataElement.appendChild(list);
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
  const statusUrl = '/api/epg/cache/status'
    + '?backend=' + encodeURIComponent(backendId)
    + '&_=' + encodeURIComponent(String(Date.now()));

  return fetch(statusUrl, { cache: 'no-store' })
    .then(response => {
      if (!response.ok) {
        throw new Error('HTTP ' + response.status);
      }

      return response.json();
    })
    .catch(error => ({
      __statusError: error.message
    }));
}

function listEventsFromEpgResponse(data) {
  return listFromResponse(data, 'events');
}

function fetchCachedEpgWindowForVisibleChannel(channel) {
  const channelId = frontendChannelId(channel);

  if (channelId === '') {
    return Promise.resolve({
      events: [],
      eventCount: 0,
      __source: 'empty-channel',
      __debugUrl: 'channel-without-id'
    });
  }

  const backendId = selectedEpgBackendId();
  const bounds = epgWindowBounds();

  const cacheUrl = '/api/epg/cache/window'
    + '?backend=' + encodeURIComponent(backendId)
    + '&channelId=' + encodeURIComponent(channelId)
    + '&fromTime=' + encodeURIComponent(String(bounds.from))
    + '&untilTime=' + encodeURIComponent(String(bounds.until))
    + '&limit=0'
    + '&_=' + encodeURIComponent(String(Date.now()));

  return fetch(cacheUrl, { cache: 'no-store' })
    .then(response => {
      if (!response.ok) {
        throw new Error('EPG-Cache Kanal HTTP ' + response.status);
      }

      return response.json();
    })
    .then(data => {
      data.__source = 'cache-channel';
      data.__debugUrl = cacheUrl;
      return data;
    });
}

function mergeVisibleEpgChannelWindows(responses) {
  const events = [];
  let cacheHits = 0;

  responses.forEach(response => {
    const responseEvents = listEventsFromEpgResponse(response);

    if (responseEvents.length > 0) {
      cacheHits += 1;
    }

    responseEvents.forEach(event => events.push(event));
  });

  return {
    events,
    eventCount: events.length,
    __source: cacheHits > 0 ? 'cache-visible-24h' : 'cache-empty',
    __partialWindow: true,
    __debugUrl: String(responses.length) + ' sichtbare Kanalabfragen · Treffer: ' + String(cacheHits)
  };
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

  return Promise.all(
    visibleChannels.map(channel => fetchCachedEpgWindowForVisibleChannel(channel))
  ).then(mergeVisibleEpgChannelWindows);
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
  const limit = 5;
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

  const channelsRequest = fetch('/api/vdr/channels', { cache: 'no-store' })
    .then(response => {
      if (!response.ok) {
        throw new Error('Kanäle HTTP ' + response.status);
      }
      return response.json();
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
  renderModuleLoading('Kanäle', 'Lade Kanalliste und laufendes Programm...');

  const channelsRequest = fetch('/api/vdr/channels')
    .then(response => {
      if (!response.ok) {
        throw new Error('HTTP ' + response.status);
      }
      return response.json();
    });

  const epgWindowUrl = '/api/epg/time-window?from=-1&timespan=86400&_=' + encodeURIComponent(String(Date.now()));

  const eventsRequest = fetch(epgWindowUrl, { cache: 'no-store' })
    .then(response => {
      if (response.ok) {
        return response.json();
      }

      return fetch('/api/vdr/events')
        .then(fallbackResponse => {
          if (!fallbackResponse.ok) {
            return { events: [] };
          }
          return fallbackResponse.json();
        });
    })
    .catch(() => ({ events: [] }));

  Promise.all([channelsRequest, eventsRequest])
    .then(([channelData, eventData]) => {
      const enrichedData = attachCurrentEventsToChannelData(channelData, eventData);
      currentChannels = enrichedData;
      currentEvents = eventData;
      renderChannelList(enrichedData);
    })
    .catch(error => {
      currentChannels = null;
      currentEvents = null;
      renderModuleError('Kanäle konnten nicht geladen werden', error);
    });
}

function loadTimers() {
  renderModuleLoading('Timer', 'Lade aktuelle Timerliste direkt vom VDR...');

  fetch('/api/vdr/timers/live')
    .then(response => {
      if (response.ok) {
        return response.json();
      }

      return fetch('/api/vdr/timers')
        .then(fallbackResponse => {
          if (!fallbackResponse.ok) {
            throw new Error('HTTP ' + fallbackResponse.status);
          }

          return fallbackResponse.json();
        });
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
  renderModuleLoading('Aufnahmen', 'Lade Aufnahmeliste aus /api/vdr/recordings...');

  fetch('/api/vdr/recordings')
    .then(response => {
      if (!response.ok) {
        throw new Error('HTTP ' + response.status);
      }
      return response.json();
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

function renderSelectedModule(data) {
  if (selectedModule === 'overview') {
    renderSnapshotMetrics(data);
    return;
  }

  if (selectedModule === 'channels') {
    loadChannels();
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
