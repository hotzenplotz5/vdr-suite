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
