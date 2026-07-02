function ensureCachedEpgDetailStyles() {
  if (document.getElementById('cached-epg-detail-styles')) {
    return;
  }

  const style = document.createElement('style');
  style.id = 'cached-epg-detail-styles';
  style.textContent = `
.cached-channel-epg-detail {
  gap: 0.75rem;
}
.cached-channel-epg-header {
  border-color: rgba(96, 165, 250, 0.4);
  background:
    radial-gradient(circle at top left, rgba(37, 99, 235, 0.18), transparent 38%),
    #020617;
}
.cached-epg-header-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.9rem;
}
.cached-epg-title-row {
  display: flex;
  align-items: center;
  gap: 0.85rem;
  min-width: 0;
}
.cached-epg-title-text {
  min-width: 0;
}
.cached-epg-header-meta {
  color: #cbd5e1;
  font-size: 0.92rem;
}
.cached-epg-back-button {
  flex: 0 0 auto;
}
.cached-epg-summary {
  color: #bfdbfe;
  font-weight: 700;
}
.cached-epg-event-card {
  display: grid;
  gap: 0.5rem;
  border-color: rgba(148, 163, 184, 0.28);
}
.cached-epg-event-card.current {
  border-color: rgba(96, 165, 250, 0.55);
  background:
    radial-gradient(circle at top left, rgba(37, 99, 235, 0.16), transparent 42%),
    #020617;
}
.cached-epg-event-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 0.75rem;
}
.cached-epg-status-badge {
  flex: 0 0 auto;
  border-radius: 999px;
  padding: 0.22rem 0.62rem;
  font-size: 0.8rem;
  font-weight: 800;
  border: 1px solid rgba(148, 163, 184, 0.3);
  color: #e2e8f0;
  background: rgba(51, 65, 85, 0.68);
}
.cached-epg-status-badge.current {
  color: #dbeafe;
  background: rgba(37, 99, 235, 0.34);
  border-color: rgba(96, 165, 250, 0.46);
}
.cached-epg-status-badge.next {
  color: #bbf7d0;
  background: rgba(22, 163, 74, 0.18);
  border-color: rgba(34, 197, 94, 0.32);
}
.cached-epg-event-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 0.4rem;
  align-items: center;
}
.cached-epg-chip {
  display: inline-flex;
  align-items: center;
  border-radius: 999px;
  padding: 0.25rem 0.55rem;
  background: rgba(30, 41, 59, 0.9);
  color: #cbd5e1;
  font-size: 0.88rem;
}
.cached-epg-description {
  color: #dbeafe;
  line-height: 1.42;
  overflow-wrap: anywhere;
}
.cached-epg-overview-link {
  cursor: pointer;
}
.cached-epg-overview-link:focus {
  outline: 3px solid #60a5fa;
  outline-offset: 3px;
}
.cached-epg-channel-progress {
  display: grid;
  gap: 0.55rem;
  border-color: rgba(96, 165, 250, 0.42);
  background:
    radial-gradient(circle at top left, rgba(37, 99, 235, 0.14), transparent 42%),
    rgba(15, 23, 42, 0.92);
}
.cached-epg-channel-progress.done {
  border-color: rgba(34, 197, 94, 0.34);
}
.cached-epg-channel-progress.error {
  border-color: rgba(248, 113, 113, 0.42);
}
.cached-epg-progress-title {
  color: #dbeafe;
  font-weight: 800;
}
.cached-epg-progress-text {
  color: #cbd5e1;
}
.cached-epg-progress-track {
  height: 0.45rem;
  overflow: hidden;
  border-radius: 999px;
  background: rgba(30, 41, 59, 0.96);
  border: 1px solid rgba(96, 165, 250, 0.2);
}
.cached-epg-progress-bar {
  height: 100%;
  width: 42%;
  border-radius: 999px;
  background: linear-gradient(90deg, #2563eb, #0ea5e9, #22d3ee);
  animation: cachedEpgProgressBar 1.05s ease-in-out infinite;
}
@keyframes cachedEpgProgressBar {
  0% { transform: translateX(-115%); }
  55% { transform: translateX(85%); }
  100% { transform: translateX(245%); }
}
@media (max-width: 760px) {
  .cached-epg-header-row {
    align-items: stretch;
    flex-direction: column;
  }
  .cached-epg-back-button {
    width: 100%;
  }
  .cached-epg-event-header {
    flex-direction: column-reverse;
  }
  .cached-epg-status-badge {
    align-self: flex-start;
  }
}
`;
  document.head.appendChild(style);
}

ensureCachedEpgDetailStyles();

let cachedEpgLastChangeSequence = null;
let cachedEpgChangeFeedTimer = null;

function cachedEpgEnsureLiveStatusBox() {
  let box = document.getElementById('vdr-suite-live-status');

  if (box) {
    return box;
  }

  box = document.createElement('div');
  box.id = 'vdr-suite-live-status';
  box.className = 'detail-meta';
  box.textContent = 'Live-Status: Verbindung wird vorbereitet...';

  const detail = document.getElementById('detail-meta');
  if (detail && detail.parentNode) {
    detail.parentNode.insertBefore(box, detail.nextSibling);
    return box;
  }

  document.body.appendChild(box);
  return box;
}

function cachedEpgSetLiveStatus(text, error) {
  const box = cachedEpgEnsureLiveStatusBox();
  box.className = error ? 'detail-meta error' : 'detail-meta';
  box.textContent = text;
}

function cachedEpgPollChangeFeedOnce() {
  return fetchJsonOrThrow('/api/vdr/changes')
    .then(data => {
      const sequence = Number(data.latestSequenceNumber || 0);
      const generation = Number(data.latestSnapshotGeneration || 0);
      const entries = listFromResponse(data, 'entries');

      if (cachedEpgLastChangeSequence === null) {
        cachedEpgLastChangeSequence = sequence;
        cachedEpgSetLiveStatus(
          'Live-Status: verbunden · Snapshot ' + String(generation) +
            ' · Sequenz ' + String(sequence),
          false
        );
        return;
      }

      if (sequence > cachedEpgLastChangeSequence) {
        cachedEpgLastChangeSequence = sequence;
        const last = entries.length > 0 ? entries[entries.length - 1] : null;
        const backend = last && last.backendId ? String(last.backendId) : 'default';
        const domains = last && Array.isArray(last.changedDomains)
          ? last.changedDomains.join(', ')
          : 'Snapshot';

        cachedEpgSetLiveStatus(
          'Live-Status: Änderung erkannt · Backend ' + backend +
            ' · ' + domains +
            ' · Sequenz ' + String(sequence),
          false
        );
        return;
      }

      cachedEpgSetLiveStatus(
        'Live-Status: verbunden · Snapshot ' + String(generation) +
          ' · Sequenz ' + String(sequence),
        false
      );
    })
    .catch(error => {
      cachedEpgSetLiveStatus(
        'Live-Status: Change-Feed nicht erreichbar · ' + error.message,
        true
      );
    });
}

function cachedEpgStartChangeFeedPolling() {
  if (cachedEpgChangeFeedTimer) {
    return;
  }

  cachedEpgPollChangeFeedOnce();
  cachedEpgChangeFeedTimer = setInterval(cachedEpgPollChangeFeedOnce, 5000);
}

cachedEpgStartChangeFeedPolling();



function frontendSelectedBackendId() {
  if (selectedBackendId && String(selectedBackendId).trim() !== '') {
    return String(selectedBackendId).trim();
  }

  if (selectedBackend) {
    const selector = selectedBackend.frontendSelector || selectedBackend;
    return String(selector.id || selectedBackend.backendId || 'default').trim();
  }

  return 'default';
}

function fetchJsonOrThrow(url, options) {
  return fetch(url, options || {})
    .then(response => {
      if (!response.ok) {
        throw new Error('HTTP ' + response.status);
      }

      return response.json();
    });
}

function loadLiveNowNextEvents() {
  return fetch('/api/epg/now-next?from=-1')
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
}

function loadCachedNowNextEvents(backendId) {
  const normalizedBackendId = backendId && String(backendId).trim() !== ''
    ? String(backendId).trim()
    : 'default';
  const encodedBackendId = encodeURIComponent(normalizedBackendId);
  const nowSeconds = Math.floor(Date.now() / 1000);

  return fetchJsonOrThrow(
    '/api/epg/cache/now-next?backend=' + encodedBackendId +
      '&fromTime=' + String(nowSeconds) +
      '&limit=1000'
  )
    .catch(() => ({ events: [] }));
}

function loadCachedChannelEvents(backendId, channelId) {
  const normalizedBackendId = backendId && String(backendId).trim() !== ''
    ? String(backendId).trim()
    : 'default';
  const encodedBackendId = encodeURIComponent(normalizedBackendId);
  const encodedChannelId = encodeURIComponent(channelId);
  const nowSeconds = Math.floor(Date.now() / 1000);

  return fetchJsonOrThrow(
    '/api/epg/cache/refresh?backend=' + encodedBackendId +
      '&channelId=' + encodedChannelId +
      '&from=-1&chevents=10',
    { method: 'POST' }
  )
    .then(() => fetchJsonOrThrow(
      '/api/epg/cache/now-next?backend=' + encodedBackendId +
        '&channelId=' + encodedChannelId +
        '&fromTime=' + String(nowSeconds) +
        '&limit=10'
    ));
}

function cachedEpgChannelTitle(channel, fallbackIndex) {
  return firstValue(
    channel,
    ['name', 'channelName', 'title', 'displayName', 'id', 'channelId'],
    'Kanal ' + String(fallbackIndex + 1)
  );
}

function cachedEpgProgramTitle(event) {
  return firstValue(event, ['title', 'name', 'eventTitle'], 'Laufendes Programm');
}

function cachedEpgProgramSubtitle(event) {
  return firstValue(event, ['subtitle', 'shortText', 'short_text'], '');
}

function cachedEpgProgramDescription(event) {
  return firstValue(event, ['description', 'summary', 'text'], '');
}

function cachedEpgEventLabel(index) {
  if (index === 0) {
    return 'Jetzt';
  }

  if (index === 1) {
    return 'Als Nächstes';
  }

  return 'Später';
}

function cachedEpgEventBadgeClass(index) {
  if (index === 0) {
    return 'cached-epg-status-badge current';
  }

  if (index === 1) {
    return 'cached-epg-status-badge next';
  }

  return 'cached-epg-status-badge';
}

function cachedEpgTimeText(event) {
  if (typeof channelProgramTimeText === 'function') {
    return channelProgramTimeText(event);
  }

  return '';
}

function appendCachedEpgChip(container, text) {
  const value = String(text || '').trim();

  if (value === '') {
    return;
  }

  const chip = document.createElement('span');
  chip.className = 'cached-epg-chip';
  chip.textContent = value;
  container.appendChild(chip);
}

function activateCachedEpgChannelsTab() {
  selectedModule = 'channels';

  document.querySelectorAll('.module-tab').forEach(button => {
    const isChannels = String(button.textContent || '').trim() === moduleLabels.channels;
    button.classList.toggle('active', isChannels);
  });
}

function openCachedChannelEpgDetail(channel) {
  activateCachedEpgChannelsTab();
  renderCachedChannelEpgDetail(channel);
}

function makeCachedEpgOverviewRowOpenDetails(item, channel, title) {
  item.classList.add('cached-epg-overview-link');
  item.tabIndex = 0;
  item.setAttribute('role', 'button');
  item.setAttribute('aria-label', 'EPG-Details für ' + String(title) + ' öffnen');
  item.title = 'EPG-Details für ' + String(title) + ' öffnen';

  const openDetails = () => openCachedChannelEpgDetail(channel);

  item.addEventListener('click', openDetails);
  item.addEventListener('keydown', event => {
    if (event.key === 'Enter' || event.key === ' ') {
      event.preventDefault();
      openDetails();
    }
  });
}

function cachedEpgRenderNowOverviewRows(container, channels, events) {
  const nowSeconds = Math.floor(Date.now() / 1000);
  let visibleCount = 0;

  channels.forEach((channel, index) => {
    const currentProgram = findCurrentEventForChannel(channel, events, nowSeconds);

    if (!currentProgram) {
      return;
    }

    visibleCount += 1;

    const title = cachedEpgChannelTitle(channel, index);
    const channelId = frontendChannelId(channel);
    const item = document.createElement('article');
    item.className = 'list-item channel-list-item';

    makeCachedEpgOverviewRowOpenDetails(item, channel, title);

    if (typeof createChannelLogoElement === 'function') {
      item.appendChild(createChannelLogoElement(title, channelId));
    }

    const text = document.createElement('div');
    text.className = 'channel-text';
    text.appendChild(addText(document.createElement('div'), String(title))).className = 'list-title';
    text.appendChild(addText(
      document.createElement('div'),
      'Jetzt: ' + String(cachedEpgProgramTitle(currentProgram))
    )).className = 'list-meta';

    const timeText = cachedEpgTimeText(currentProgram);
    const subtitle = cachedEpgProgramSubtitle(currentProgram);
    const detailParts = [timeText, subtitle]
      .filter(value => String(value || '').trim() !== '');

    if (detailParts.length > 0) {
      text.appendChild(addText(document.createElement('div'), detailParts.join(' · '))).className = 'list-meta';
    }

    item.appendChild(text);
    container.appendChild(item);
  });

  return visibleCount;
}

function renderCachedChannelEpgEvent(container, event, index) {
  const item = document.createElement('article');
  item.className = index === 0
    ? 'list-item cached-epg-event-card current'
    : 'list-item cached-epg-event-card';

  const header = document.createElement('div');
  header.className = 'cached-epg-event-header';

  const text = document.createElement('div');
  text.className = 'cached-epg-title-text';
  text.appendChild(addText(
    document.createElement('div'),
    String(cachedEpgProgramTitle(event))
  )).className = 'list-title';

  const badge = document.createElement('span');
  badge.className = cachedEpgEventBadgeClass(index);
  badge.textContent = cachedEpgEventLabel(index);

  header.appendChild(text);
  header.appendChild(badge);
  item.appendChild(header);

  const meta = document.createElement('div');
  meta.className = 'cached-epg-event-meta';
  appendCachedEpgChip(meta, cachedEpgTimeText(event));
  appendCachedEpgChip(meta, cachedEpgProgramSubtitle(event));

  if (meta.childNodes.length > 0) {
    item.appendChild(meta);
  }

  const description = cachedEpgProgramDescription(event);
  if (String(description).trim() !== '') {
    item.appendChild(addText(document.createElement('div'), String(description))).className = 'cached-epg-description';
  }

  container.appendChild(item);
}

function renderCachedChannelEpgDetail(channel) {
  const backendId = frontendSelectedBackendId();
  const channelId = frontendChannelId(channel);
  const title = cachedEpgChannelTitle(channel, 0);

  detailDataElement.replaceChildren();

  const list = document.createElement('section');
  list.className = 'list cached-channel-epg-detail';

  const header = document.createElement('article');
  header.className = 'module-placeholder cached-channel-epg-header';

  const headerRow = document.createElement('div');
  headerRow.className = 'cached-epg-header-row';

  const titleRow = document.createElement('div');
  titleRow.className = 'cached-epg-title-row';

  if (typeof createChannelLogoElement === 'function') {
    titleRow.appendChild(createChannelLogoElement(title, channelId));
  }

  const titleText = document.createElement('div');
  titleText.className = 'cached-epg-title-text';
  titleText.appendChild(addText(document.createElement('h3'), String(title)));
  titleText.appendChild(addText(
    document.createElement('div'),
    'Backend ' + backendId + ' · Kanal ' + channelId
  )).className = 'cached-epg-header-meta';
  titleRow.appendChild(titleText);

  const backButton = document.createElement('button');
  backButton.type = 'button';
  backButton.className = 'cached-epg-back-button';
  backButton.textContent = 'Zurück zur Kanalliste';
  backButton.addEventListener('click', () => {
    if (currentChannels) {
      renderChannelList(currentChannels);
      return;
    }

    loadChannels();
  });

  headerRow.appendChild(titleRow);
  headerRow.appendChild(backButton);
  header.appendChild(headerRow);
  list.appendChild(header);

  const loading = document.createElement('article');
  loading.className = 'module-placeholder';
  loading.appendChild(addText(document.createElement('p'), 'Lade EPG für diesen Kanal...'));
  list.appendChild(loading);
  detailDataElement.appendChild(list);

  loadCachedChannelEvents(backendId, channelId)
    .then(data => {
      if (!list.isConnected || selectedModule !== 'channels') {
        return;
      }

      loading.remove();
      const events = listFromResponse(data, 'events');

      if (events.length === 0) {
        const empty = document.createElement('article');
        empty.className = 'module-placeholder';
        empty.appendChild(addText(document.createElement('h3'), 'Keine EPG-Daten gefunden'));
        empty.appendChild(addText(document.createElement('p'), 'Der Cache enthält aktuell keine Events für diesen Kanal.'));
        list.appendChild(empty);
        return;
      }

      const summary = document.createElement('article');
      summary.className = 'module-placeholder cached-epg-summary';
      summary.appendChild(addText(
        document.createElement('p'),
        String(events.length) + ' Sendung(en) aus dem Cache geladen. Der erste Eintrag ist die laufende Sendung.'
      ));
      list.appendChild(summary);

      events.forEach((event, index) => renderCachedChannelEpgEvent(list, event, index));
    })
    .catch(error => {
      if (!list.isConnected || selectedModule !== 'channels') {
        return;
      }

      loading.remove();
      const box = document.createElement('article');
      box.className = 'module-placeholder error';
      box.appendChild(addText(document.createElement('h3'), 'EPG-Details konnten nicht geladen werden'));
      box.appendChild(addText(document.createElement('p'), error.message));
      list.appendChild(box);
    });
}

function renderCachedEpgNowOverview() {
  const existingOverview = detailDataElement.querySelector('.epg-now-overview');

  if (existingOverview) {
    existingOverview.remove();
  }

  const backendId = frontendSelectedBackendId();
  const loading = document.createElement('section');
  loading.className = 'list epg-now-overview';

  const placeholder = document.createElement('article');
  placeholder.className = 'module-placeholder';
  placeholder.appendChild(addText(document.createElement('h3'), 'EPG: Jetzt läuft'));
  placeholder.appendChild(addText(document.createElement('p'), 'Lade gecachte Jetzt-läuft-Übersicht...'));
  loading.appendChild(placeholder);
  detailDataElement.appendChild(loading);

  const channelsRequest = fetchJsonOrThrow('/api/vdr/channels');
  const eventsRequest = loadCachedNowNextEvents(backendId);

  Promise.all([channelsRequest, eventsRequest])
    .then(([channelData, eventData]) => {
      if (!loading.isConnected || selectedModule !== 'overview' || frontendSelectedBackendId() !== backendId) {
        return;
      }

      const channels = typeof sortedChannels === 'function'
        ? sortedChannels(listFromResponse(channelData, 'channels'))
        : listFromResponse(channelData, 'channels');
      const events = listFromResponse(eventData, 'events');

      loading.replaceChildren();

      const header = document.createElement('article');
      header.className = 'module-placeholder';
      header.appendChild(addText(document.createElement('h3'), 'EPG: Jetzt läuft'));
      header.appendChild(addText(
        document.createElement('p'),
        'Backend ' + backendId + ' · ' + String(events.length) + ' gecachte Event(s) im aktuellen Fenster.'
      ));
      loading.appendChild(header);

      const visibleCount = cachedEpgRenderNowOverviewRows(loading, channels, events);

      if (visibleCount === 0) {
        const empty = document.createElement('article');
        empty.className = 'module-placeholder';
        empty.appendChild(addText(document.createElement('h3'), 'Keine laufenden EPG-Daten gefunden'));
        empty.appendChild(addText(document.createElement('p'), 'Der Cache enthält aktuell keine passenden Jetzt-läuft-Events.'));
        loading.appendChild(empty);
        return;
      }

      header.appendChild(addText(
        document.createElement('p'),
        'Angezeigt: ' + String(visibleCount) + ' laufende Programme.'
      ));
    })
    .catch(error => {
      if (!loading.isConnected || selectedModule !== 'overview') {
        return;
      }

      loading.replaceChildren();
      const box = document.createElement('article');
      box.className = 'module-placeholder error';
      box.appendChild(addText(document.createElement('h3'), 'EPG-Übersicht konnte nicht geladen werden'));
      box.appendChild(addText(document.createElement('p'), error.message));
      loading.appendChild(box);
    });
}

function renderCachedEpgNowOverviewIfCurrent() {
  if (selectedModule !== 'overview' || !currentSnapshot) {
    return;
  }

  if (detailDataElement.querySelector('.epg-now-overview')) {
    return;
  }

  renderCachedEpgNowOverview();
}

const renderSnapshotMetricsWithoutCachedEpg = renderSnapshotMetrics;

renderSnapshotMetrics = function(data) {
  renderSnapshotMetricsWithoutCachedEpg(data);
  renderCachedEpgNowOverview();
};

setTimeout(renderCachedEpgNowOverviewIfCurrent, 0);
setTimeout(renderCachedEpgNowOverviewIfCurrent, 250);

if (typeof renderChannelItem === 'function') {
  const renderChannelItemWithoutCachedEpgDetail = renderChannelItem;

  renderChannelItem = function(channel, index, encryptionAvailable) {
    const item = renderChannelItemWithoutCachedEpgDetail(channel, index, encryptionAvailable);
    const title = cachedEpgChannelTitle(channel, index);
    const channelId = frontendChannelId(channel);
    item.dataset.cachedEpgChannelId = channelId;

    item.tabIndex = 0;
    item.setAttribute('role', 'button');
    item.setAttribute('aria-label', 'EPG für ' + String(title) + ' öffnen');
    item.title = 'EPG für ' + String(title) + ' öffnen';

    const openDetails = () => openCachedChannelEpgDetail(channel);

    item.addEventListener('click', openDetails);
    item.addEventListener('keydown', event => {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault();
        openDetails();
      }
    });

    return item;
  };
}

let cachedEpgChannelLoadSequence = 0;

function cachedEpgChannelListElement() {
  return detailDataElement.querySelector('.list');
}

function removeCachedEpgChannelProgress() {
  const existing = detailDataElement.querySelector('.cached-epg-channel-progress');

  if (existing) {
    existing.remove();
  }
}

function renderCachedEpgChannelProgress(message, state) {
  const list = cachedEpgChannelListElement();

  if (!list) {
    return;
  }

  removeCachedEpgChannelProgress();

  const box = document.createElement('article');
  box.className = 'module-placeholder cached-epg-channel-progress' + (state ? ' ' + state : '');

  const title = state === 'done'
    ? 'EPG ergänzt'
    : state === 'error'
      ? 'EPG konnte nicht ergänzt werden'
      : 'EPG wird im Hintergrund ergänzt';

  box.appendChild(addText(document.createElement('div'), title)).className = 'cached-epg-progress-title';
  box.appendChild(addText(document.createElement('div'), message)).className = 'cached-epg-progress-text';

  if (state !== 'done' && state !== 'error') {
    const track = document.createElement('div');
    track.className = 'cached-epg-progress-track';

    const bar = document.createElement('div');
    bar.className = 'cached-epg-progress-bar';

    track.appendChild(bar);
    box.appendChild(track);
  }

  const first = list.firstElementChild;
  if (first && first.nextSibling) {
    list.insertBefore(box, first.nextSibling);
    return;
  }

  list.appendChild(box);
}

function renderChannelsWithoutBlockingEpg(channelData) {
  currentChannels = channelData;
  currentEvents = { events: [] };
  renderChannelList(channelData);
  renderCachedEpgChannelProgress(
    'Kanäle sind bereits sichtbar. Laufende Programme werden nachgeladen.',
    'loading'
  );
}

function renderChannelsWithCachedEpg(channelData, eventData) {
  const enrichedData = attachCurrentEventsToChannelData(channelData, eventData);
  currentChannels = enrichedData;
  currentEvents = eventData;
  renderChannelList(enrichedData);
}

function cachedEpgChannelListElement() {
  return detailDataElement.querySelector('.list');
}

function removeCachedEpgChannelProgress() {
  const existing = detailDataElement.querySelector('.cached-epg-channel-progress');

  if (existing) {
    existing.remove();
  }
}

function renderCachedEpgChannelProgress(message, state) {
  const list = cachedEpgChannelListElement();

  if (!list) {
    return;
  }

  removeCachedEpgChannelProgress();

  const box = document.createElement('article');
  box.className = 'module-placeholder cached-epg-channel-progress' + (state ? ' ' + state : '');

  const title = state === 'done'
    ? 'EPG ergänzt'
    : state === 'error'
      ? 'EPG konnte nicht ergänzt werden'
      : 'EPG wird ergänzt';

  box.appendChild(addText(document.createElement('div'), title)).className = 'cached-epg-progress-title';
  box.appendChild(addText(document.createElement('div'), message)).className = 'cached-epg-progress-text';

  if (state !== 'done' && state !== 'error') {
    const track = document.createElement('div');
    track.className = 'cached-epg-progress-track';

    const bar = document.createElement('div');
    bar.className = 'cached-epg-progress-bar';

    track.appendChild(bar);
    box.appendChild(track);
  }

  const first = list.firstElementChild;
  if (first && first.nextSibling) {
    list.insertBefore(box, first.nextSibling);
    return;
  }

  list.appendChild(box);
}

function cachedEpgProgramLineForItem(item) {
  const lines = Array.from(item.querySelectorAll('.list-meta'));

  return lines.find(line =>
    String(line.textContent || '').trim().startsWith('Jetzt:'));
}

function cachedEpgPaintVisibleChannelPrograms(eventData) {
  const events = listFromResponse(eventData, 'events');
  const channels = listFromResponse(currentChannels, 'channels');
  const nowSeconds = Math.floor(Date.now() / 1000);
  const channelsById = new Map();

  channels.forEach(channel => {
    const channelId = frontendChannelId(channel);
    if (channelId !== '') {
      channelsById.set(channelId, channel);
    }
  });

  let updated = 0;

  document.querySelectorAll('.channel-list-item[data-cached-epg-channel-id]').forEach(item => {
    const channelId = String(item.dataset.cachedEpgChannelId || '').trim();
    const channel = channelsById.get(channelId);

    if (!channel) {
      return;
    }

    const event = findCurrentEventForChannel(channel, events, nowSeconds);

    if (!event) {
      return;
    }

    const programLine = cachedEpgProgramLineForItem(item);

    if (!programLine) {
      return;
    }

    const programTitle = cachedEpgProgramTitle(event);
    const timeText = cachedEpgTimeText(event);
    const subtitle = cachedEpgProgramSubtitle(event);
    const detailParts = [timeText, subtitle]
      .filter(value => String(value || '').trim() !== '');

    programLine.textContent = detailParts.length > 0
      ? 'Jetzt: ' + String(programTitle) + ' · ' + detailParts.join(' · ')
      : 'Jetzt: ' + String(programTitle);

    updated += 1;
  });

  return updated;
}

function cachedEpgStartChannelProgramLoad(backendId, loadSequence) {
  const start = () => {
    if (loadSequence !== cachedEpgChannelLoadSequence ||
        selectedModule !== 'channels' ||
        frontendSelectedBackendId() !== backendId) {
      return;
    }

    loadCachedNowNextEvents(backendId)
      .then(eventData => {
        if (loadSequence !== cachedEpgChannelLoadSequence ||
            selectedModule !== 'channels' ||
            frontendSelectedBackendId() !== backendId) {
          return;
        }

        currentEvents = eventData;
        const updated = cachedEpgPaintVisibleChannelPrograms(eventData);

        renderCachedEpgChannelProgress(
          updated > 0
            ? String(updated) + ' sichtbare Kanal/Kanäle mit laufendem Programm ergänzt.'
            : 'Keine laufenden Programme für die sichtbaren Kanäle im Cache gefunden.',
          updated > 0 ? 'done' : 'error'
        );

        setTimeout(() => {
          if (loadSequence === cachedEpgChannelLoadSequence && selectedModule === 'channels') {
            removeCachedEpgChannelProgress();
          }
        }, 1800);
      })
      .catch(() => {
        if (loadSequence !== cachedEpgChannelLoadSequence ||
            selectedModule !== 'channels') {
          return;
        }

        renderCachedEpgChannelProgress(
          'Die Kanalliste bleibt nutzbar. EPG wird später erneut versucht.',
          'error'
        );
      });
  };

  if (typeof requestAnimationFrame === 'function') {
    requestAnimationFrame(() => requestAnimationFrame(start));
    return;
  }

  setTimeout(start, 120);
}

loadChannels = function() {
  renderModuleLoading('Kanäle', 'Lade Kanalliste...');

  const backendId = frontendSelectedBackendId();
  const loadSequence = ++cachedEpgChannelLoadSequence;

  fetchJsonOrThrow('/api/vdr/channels')
    .then(channelData => {
      if (loadSequence !== cachedEpgChannelLoadSequence || selectedModule !== 'channels') {
        return;
      }

      currentChannels = channelData;
      currentEvents = { events: [] };
      renderChannelList(channelData);

      renderCachedEpgChannelProgress(
        'Kanäle sind sichtbar. Laufende Programme werden nachgeladen.',
        'loading'
      );

      cachedEpgStartChannelProgramLoad(backendId, loadSequence);
    })
    .catch(error => {
      if (loadSequence !== cachedEpgChannelLoadSequence) {
        return;
      }

      currentChannels = null;
      currentEvents = null;
      renderModuleError('Kanäle konnten nicht geladen werden', error);
    });
};
