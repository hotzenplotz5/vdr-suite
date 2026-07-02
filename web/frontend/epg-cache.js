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
    '/api/epg/cache/refresh?backend=' + encodedBackendId + '&from=-1&chevents=2',
    { method: 'POST' }
  )
    .then(() => fetchJsonOrThrow(
      '/api/epg/cache/now-next?backend=' + encodedBackendId +
        '&fromTime=' + String(nowSeconds) +
        '&limit=1000'
    ))
    .then(data => {
      if (listFromResponse(data, 'events').length === 0) {
        return loadLiveNowNextEvents();
      }

      return data;
    })
    .catch(() => loadLiveNowNextEvents());
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

    const timeText = typeof channelProgramTimeText === 'function'
      ? channelProgramTimeText(currentProgram)
      : '';
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

loadChannels = function() {
  renderModuleLoading('Kanäle', 'Lade Kanalliste und gecachtes laufendes Programm...');

  const backendId = frontendSelectedBackendId();

  const channelsRequest = fetch('/api/vdr/channels')
    .then(response => {
      if (!response.ok) {
        throw new Error('HTTP ' + response.status);
      }

      return response.json();
    });

  const eventsRequest = loadCachedNowNextEvents(backendId);

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
};
