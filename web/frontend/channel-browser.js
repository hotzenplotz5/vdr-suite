// Phase 59.11b: Channel browser drag-scrolls the selected-channel programme list.
// Channel browser view, filters, grouped navigation and programme agenda.
// Depends on app.js helpers and channel-logos.js logo helpers.

let channelListViewMode = 'groups';
let channelListVisibleCount = 20;
let channelListOpenGroups = {};
let channelListFilters = {
  tv: false,
  radio: false,
  free: false,
  encrypted: false,
  enabled: false,
  disabled: false
};

const CHANNEL_LIST_PAGE_SIZE = 20;

const CHANNEL_BROWSER_EPG_RETRY_DELAY_MS = 900;

let channelBrowserContext = {};

function configureChannelBrowserContext(context) {
  channelBrowserContext = context && typeof context === 'object' ? Object.assign({}, context) : {};
}

function channelBrowserDetailDataElement() {
  const element = channelBrowserContext.detailDataElement;

  if (!element || typeof element.replaceChildren !== 'function' || typeof element.appendChild !== 'function') {
    throw new Error('Channel browser detail data element is not configured');
  }

  return element;
}

function channelBrowserAddText(element, text) {
  element.textContent = text;
  return element;
}

function channelBrowserFirstValue(object, keys, fallback) {
  for (const key of keys) {
    if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
      return object[key];
    }
  }

  return fallback;
}

function channelBrowserListFromResponse(data, key) {
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

function channelBrowserListEventsFromEpgResponse(data) {
  return channelBrowserListFromResponse(data, 'events');
}

let channelBrowserEpgPrefetchInFlight = false;
let channelBrowserEpgPrefetchLastStartedAt = 0;

function channelBrowserBuildDataWithEvents(channelData, eventData) {
  const enriched = Array.isArray(channelData)
    ? { channels: channelData }
    : Object.assign({}, channelData);

  enriched.events = channelBrowserListEventsFromEpgResponse(eventData);
  enriched.__epgSource = String(eventData.__source || 'cache');
  enriched.__epgDebugUrl = String(eventData.__debugUrl || '');

  return enriched;
}

function scheduleChannelBrowserEpgPrefetch(channelData) {
  if (typeof fetchCachedOrLiveEpgWindow !== 'function') {
    return;
  }

  if (typeof selectedModule !== 'undefined' && selectedModule !== 'channels') {
    return;
  }

  if (channelBrowserEpgPrefetchInFlight) {
    return;
  }

  const now = Date.now();
  if (now - channelBrowserEpgPrefetchLastStartedAt < CHANNEL_BROWSER_EPG_RETRY_DELAY_MS) {
    return;
  }

  channelBrowserEpgPrefetchInFlight = true;
  channelBrowserEpgPrefetchLastStartedAt = now;

  fetchCachedOrLiveEpgWindow(channelData)
    .then(eventData => {
      currentEvents = eventData;

      if (typeof selectedModule !== 'undefined' && selectedModule !== 'channels') {
        return;
      }

      renderChannelList(channelBrowserBuildDataWithEvents(channelData, eventData));
    })
    .catch(error => {
      (void error);
    })
    .finally(() => {
      channelBrowserEpgPrefetchInFlight = false;
    });
}


function fetchChannelBrowserChannelWindow(channel) {
  if (typeof fetchCachedEpgWindowForVisibleChannel !== 'function') {
    return Promise.resolve({ events: [] });
  }

  return fetchCachedEpgWindowForVisibleChannel(channel)
    .then(eventData => {
      if (channelBrowserListEventsFromEpgResponse(eventData).length > 0) {
        return eventData;
      }

      const channelId = frontendChannelId(channel);
      if (channelId === '') {
        return eventData;
      }

      const backendId = selectedEpgBackendId();
      const bounds = epgWindowBounds();
      const clientApi = window.VdrSuiteClientApi;

      if (!clientApi || typeof clientApi.fetchClientEpgCacheRefresh !== 'function') {
        return eventData;
      }

      return clientApi.fetchClientEpgCacheRefresh({
        query: {
          backend: backendId,
          channelId: channelId,
          from: String(bounds.from),
          timespan: String(Math.max(7200, bounds.until - bounds.from)),
          limit: '0',
          channelEventLimit: '96',
          _: String(Date.now())
        },
        cache: 'no-store'
      })
        .then(() => fetchCachedEpgWindowForVisibleChannel(channel)
          .catch(() => eventData))
        .catch(() => eventData);
    })
    .catch(() => ({ events: [] }));
}

function epgEventsForChannel(channel, sourceEvents, nowSeconds) {
  const channelId = frontendChannelId(channel);
  const current = Number(nowSeconds) || Math.floor(Date.now() / 1000);
  const entries = [];

  if (channelId === '') {
    return entries;
  }

  (Array.isArray(sourceEvents) ? sourceEvents : []).forEach(event => {
    if (frontendEventChannelId(event) !== channelId) {
      return;
    }

    const start = parseFrontendEventEpoch(channelBrowserFirstValue(event, ['startTime', 'start', 'beginTime'], ''));
    const end = frontendEventEnd(event, start);

    if (!Number.isFinite(start) || !Number.isFinite(end) || start <= 0 || end <= current) {
      return;
    }

    entries.push({ event, start, end });
  });

  entries.sort((left, right) => left.start - right.start);
  return entries;
}

function channelDragRecentlyEnded(element) {
  if (!element) {
    return false;
  }

  const endedAt = Number(element.dataset.channelDragEndedAt || 0);
  return Number.isFinite(endedAt) && Date.now() - endedAt < 320;
}

function enableChannelMouseDragScroll(element, axis) {
  if (!element || element.dataset.channelDragScrollBound === 'true') {
    return;
  }

  element.dataset.channelDragScrollBound = 'true';

  const scrollAxis = axis === 'x' || axis === 'y' || axis === 'both' ? axis : 'both';
  const threshold = 7;

  let pointerActive = false;
  let dragging = false;
  let startX = 0;
  let startY = 0;
  let startScrollLeft = 0;
  let startScrollTop = 0;

  const canScrollX = () => scrollAxis === 'x' || scrollAxis === 'both';
  const canScrollY = () => scrollAxis === 'y' || scrollAxis === 'both';

  const stopDrag = () => {
    if (dragging) {
      element.dataset.channelDragEndedAt = String(Date.now());
    }

    pointerActive = false;
    dragging = false;
    element.classList.remove('dragging');
  };

  element.addEventListener('pointerdown', event => {
    if (event.pointerType && event.pointerType !== 'mouse') {
      return;
    }

    if (event.button !== undefined && event.button !== 0) {
      return;
    }

    pointerActive = true;
    dragging = false;
    startX = event.clientX;
    startY = event.clientY;
    startScrollLeft = element.scrollLeft;
    startScrollTop = element.scrollTop;
  });

  element.addEventListener('pointermove', event => {
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
      element.classList.add('dragging');
    }

    if (canScrollX()) {
      element.scrollLeft = startScrollLeft - deltaX;
    }

    if (canScrollY()) {
      element.scrollTop = startScrollTop - deltaY;
    }

    event.preventDefault();
  }, { passive: false });

  element.addEventListener('pointerup', stopDrag);
  element.addEventListener('pointercancel', stopDrag);
  element.addEventListener('pointerleave', stopDrag);
}

function channelNumber(channel, fallback) {
  const value = Number(channelBrowserFirstValue(channel, ['number', 'channelNumber', 'position'], fallback));
  return Number.isFinite(value) && value > 0 ? value : fallback;
}

function channelBoolean(channel, keys, fallback) {
  for (const key of keys) {
    if (Object.prototype.hasOwnProperty.call(channel, key)) {
      return Boolean(channel[key]);
    }
  }

  return fallback;
}

function channelHasField(channel, keys) {
  return keys.some(key => Object.prototype.hasOwnProperty.call(channel, key));
}

function channelHasUsableCaids(channel) {
  const caids = channel.caids || channel.CAIDs || channel.caid || channel.CAID;

  if (Array.isArray(caids)) {
    return caids.length > 0;
  }

  return caids !== undefined && caids !== null && String(caids).trim() !== '';
}

function channelsHaveEncryptionInfo(channels) {
  return channels.some(channel =>
    channelHasField(channel, ['encrypted', 'scrambled', 'isEncrypted', 'isScrambled']) ||
    channelHasUsableCaids(channel)
  );
}

function sortedChannels(channels) {
  return channels.slice().sort((left, right) => {
    const numberDiff = channelNumber(left, 999999) - channelNumber(right, 999999);
    if (numberDiff !== 0) {
      return numberDiff;
    }

    const leftName = normalizeChannelLogoName(channelBrowserFirstValue(left, ['name', 'channelName', 'title', 'displayName'], ''));
    const rightName = normalizeChannelLogoName(channelBrowserFirstValue(right, ['name', 'channelName', 'title', 'displayName'], ''));
    return leftName.localeCompare(rightName, 'de-DE');
  });
}

function channelGroupName(channel) {
  const explicit = String(channelBrowserFirstValue(channel, [
    'group',
    'groupName',
    'channelGroup',
    'bouquet',
    'category',
    'provider',
    'section'
  ], '')).trim();

  if (explicit !== '') {
    return explicit;
  }

  const title = normalizeChannelLogoName(channelBrowserFirstValue(
    channel,
    ['name', 'channelName', 'title', 'displayName', 'id', 'channelId'],
    ''
  ));
  const type = normalizeChannelLogoName(channelBrowserFirstValue(channel, ['type', 'serviceType'], ''));

  if (type.includes('radio') || title.includes('radio')) {
    return 'Radio';
  }

  if (
    title.includes('das erste') ||
    title.includes('daserste') ||
    title.includes('zdf') ||
    title.includes('ndr') ||
    title.includes('wdr') ||
    title.includes('swr') ||
    title.includes('br ') ||
    title.includes('br-') ||
    title.includes('hr-') ||
    title.includes('mdr') ||
    title.includes('rbb') ||
    title.includes('arte') ||
    title.includes('3sat') ||
    title.includes('one') ||
    title.includes('phoenix') ||
    title.includes('kika') ||
    title.includes('tagesschau')
  ) {
    return 'Öffentlich-rechtlich';
  }

  if (
    title.includes('welt') ||
    title.includes('ntv') ||
    title.includes('n-tv') ||
    title.includes('euronews') ||
    title.includes('cnn') ||
    title.includes('bbc')
  ) {
    return 'Nachrichten';
  }

  if (
    title.includes('sport') ||
    title.includes('sky') ||
    title.includes('eurosport') ||
    title.includes('dazn')
  ) {
    return 'Sport';
  }

  if (
    title.includes('rtl') ||
    title.includes('sat.1') ||
    title.includes('sat1') ||
    title.includes('pro sieben') ||
    title.includes('prosieben') ||
    title.includes('vox') ||
    title.includes('kabel') ||
    title.includes('sixx') ||
    title.includes('tele 5') ||
    title.includes('dmax') ||
    title.includes('nitro')
  ) {
    return 'Private';
  }

  return 'Weitere Sender';
}

function hasRealChannelGroups(channels) {
  return channels.length > 0;
}

function clearChannelFilters() {
  channelListFilters = {
    tv: false,
    radio: false,
    free: false,
    encrypted: false,
    enabled: false,
    disabled: false
  };
}

function isAnyChannelFilterActive(encryptionAvailable) {
  return channelListFilters.tv ||
    channelListFilters.radio ||
    (encryptionAvailable && (channelListFilters.free || channelListFilters.encrypted)) ||
    channelListFilters.enabled ||
    channelListFilters.disabled;
}

function toggleChannelFilter(value, encryptionAvailable) {
  if (value === 'all') {
    clearChannelFilters();
    return;
  }

  if (!encryptionAvailable && (value === 'free' || value === 'encrypted')) {
    return;
  }

  if (value === 'tv') {
    channelListFilters.tv = !channelListFilters.tv;
    if (channelListFilters.tv) {
      channelListFilters.radio = false;
    }
    return;
  }

  if (value === 'radio') {
    channelListFilters.radio = !channelListFilters.radio;
    if (channelListFilters.radio) {
      channelListFilters.tv = false;
    }
    return;
  }

  if (value === 'free') {
    channelListFilters.free = !channelListFilters.free;
    if (channelListFilters.free) {
      channelListFilters.encrypted = false;
    }
    return;
  }

  if (value === 'encrypted') {
    channelListFilters.encrypted = !channelListFilters.encrypted;
    if (channelListFilters.encrypted) {
      channelListFilters.free = false;
    }
    return;
  }

  if (value === 'enabled') {
    channelListFilters.enabled = !channelListFilters.enabled;
    if (channelListFilters.enabled) {
      channelListFilters.disabled = false;
    }
    return;
  }

  if (value === 'disabled') {
    channelListFilters.disabled = !channelListFilters.disabled;
    if (channelListFilters.disabled) {
      channelListFilters.enabled = false;
    }
  }
}

function matchesSelectedPair(value, positiveSelected, negativeSelected) {
  if (!positiveSelected && !negativeSelected) {
    return true;
  }

  if (positiveSelected && negativeSelected) {
    return true;
  }

  return value ? positiveSelected : negativeSelected;
}

function filterChannels(channels, encryptionAvailable) {
  return channels.filter(channel => {
    const radio = channelBoolean(channel, ['radio', 'isRadio'], false);
    const encrypted = channelBoolean(channel, ['encrypted', 'scrambled', 'isEncrypted'], false);
    const enabled = channelBoolean(channel, ['enabled', 'active'], true);

    if (!matchesSelectedPair(radio, channelListFilters.radio, channelListFilters.tv)) {
      return false;
    }

    if (encryptionAvailable &&
        !matchesSelectedPair(encrypted, channelListFilters.encrypted, channelListFilters.free)) {
      return false;
    }

    if (!matchesSelectedPair(enabled, channelListFilters.enabled, channelListFilters.disabled)) {
      return false;
    }

    return true;
  });
}

function groupChannelsByVdrGroup(channels) {
  const groups = new Map();

  sortedChannels(channels).forEach(channel => {
    const groupName = channelGroupName(channel);
    if (!groups.has(groupName)) {
      groups.set(groupName, []);
    }
    groups.get(groupName).push(channel);
  });

  return Array.from(groups.entries()).sort((left, right) => {
    const leftFirst = channelNumber(left[1][0], 999999);
    const rightFirst = channelNumber(right[1][0], 999999);
    return leftFirst - rightFirst;
  });
}

function groupChannelsByLoadedPages(channels) {
  const groups = new Map();

  sortedChannels(channels).forEach((channel, index) => {
    const start = Math.floor(index / CHANNEL_LIST_PAGE_SIZE) * CHANNEL_LIST_PAGE_SIZE + 1;
    const end = start + CHANNEL_LIST_PAGE_SIZE - 1;
    const label = String(start) + '–' + String(end);

    if (!groups.has(label)) {
      groups.set(label, []);
    }

    groups.get(label).push(channel);
  });

  return Array.from(groups.entries());
}

function renderChannelButton(container, label, active, disabled, onClick) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'module-tab' + (active ? ' active' : '');
  button.textContent = label;
  button.disabled = disabled;
  button.addEventListener('click', onClick);
  container.appendChild(button);
}

function renderChannelViewButtons(container, channels) {
  const controls = document.createElement('div');
  controls.className = 'module-nav channel-browser-controls';
  controls.setAttribute('aria-label', 'Kanallisten-Ansicht');

  const groupsAvailable = hasRealChannelGroups(channels);

  renderChannelButton(
    controls,
    'Gruppen',
    channelListViewMode === 'groups',
    !groupsAvailable,
    () => {
      channelListViewMode = 'groups';
      channelListVisibleCount = CHANNEL_LIST_PAGE_SIZE;
      renderChannelList({ channels });
    }
  );

  renderChannelButton(
    controls,
    'Kanalnummer',
    channelListViewMode === 'number',
    false,
    () => {
      channelListViewMode = 'number';
      channelListVisibleCount = CHANNEL_LIST_PAGE_SIZE;
      renderChannelList({ channels });
    }
  );

  container.appendChild(controls);
}

function renderChannelFilterButtons(container, channels, encryptionAvailable) {
  const controls = document.createElement('div');
  controls.className = 'module-nav channel-browser-controls';
  controls.setAttribute('aria-label', 'Kanallisten-Filter');

  const filters = [
    ['all', 'Alle'],
    ['tv', 'TV'],
    ['radio', 'Radio']
  ];

  if (encryptionAvailable) {
    filters.push(['free', 'Frei']);
    filters.push(['encrypted', 'Verschlüsselt']);
  }

  filters.push(['enabled', 'Aktiv']);
  filters.push(['disabled', 'Deaktiviert']);

  filters.forEach(([value, label]) => {
    const active = value === 'all'
      ? !isAnyChannelFilterActive(encryptionAvailable)
      : Boolean(channelListFilters[value]);

    renderChannelButton(
      controls,
      label,
      active,
      false,
      () => {
        toggleChannelFilter(value, encryptionAvailable);
        channelListVisibleCount = CHANNEL_LIST_PAGE_SIZE;
        renderChannelList({ channels });
      }
    );
  });

  container.appendChild(controls);
}

function renderChannelPagingControls(container, channels, filteredCount) {
  const controls = document.createElement('div');
  controls.className = 'module-nav channel-browser-controls';
  controls.setAttribute('aria-label', 'Kanallisten-Paginierung');

  if (channelListVisibleCount < filteredCount) {
    renderChannelButton(
      controls,
      'Weitere 20 laden',
      false,
      false,
      () => {
        channelListVisibleCount += CHANNEL_LIST_PAGE_SIZE;
        renderChannelList({ channels });
      }
    );
  }

  if (channelListVisibleCount > CHANNEL_LIST_PAGE_SIZE) {
    renderChannelButton(
      controls,
      'Zurück auf 20',
      false,
      false,
      () => {
        channelListVisibleCount = CHANNEL_LIST_PAGE_SIZE;
        renderChannelList({ channels });
      }
    );
  }

  if (controls.childElementCount > 0) {
    container.appendChild(controls);
  }
}

function channelStatusText(channel, encryptionAvailable) {
  const radio = channelBoolean(channel, ['radio', 'isRadio'], false);
  const enabled = channelBoolean(channel, ['enabled', 'active'], true);
  const parts = [];

  parts.push(radio ? 'Radio' : 'TV');

  if (encryptionAvailable) {
    const encrypted = channelBoolean(channel, ['encrypted', 'scrambled', 'isEncrypted'], false);
    parts.push(encrypted ? 'verschlüsselt' : 'frei');
  }

  parts.push(enabled ? 'aktiv' : 'deaktiviert');

  return parts.join(' · ');
}

function channelProgramClock(value) {
  const number = Number(value);
  let date = null;

  if (Number.isFinite(number) && number > 0) {
    date = new Date((number > 100000000000 ? number : number * 1000));
  } else {
    const parsed = Date.parse(String(value || ''));
    if (Number.isFinite(parsed)) {
      date = new Date(parsed);
    }
  }

  if (!date || Number.isNaN(date.getTime())) {
    return '';
  }

  return date.toLocaleTimeString('de-DE', {
    hour: '2-digit',
    minute: '2-digit'
  });
}

function channelProgramTimeText(event) {
  if (!event) {
    return '';
  }

  const start = channelProgramClock(channelBrowserFirstValue(event, ['startTime', 'start', 'beginTime'], ''));
  const end = channelProgramClock(channelBrowserFirstValue(event, ['endTime', 'end', 'stopTime'], ''));

  if (start !== '' && end !== '') {
    return start + '–' + end;
  }

  if (start !== '') {
    return 'seit ' + start;
  }

  return '';
}

function channelCurrentProgram(channel) {
  return channel.currentEvent || channel.now || channel.currentProgram || null;
}

function renderChannelItem(channel, index, encryptionAvailable) {
  const item = document.createElement('article');
  item.className = 'list-item channel-list-item';
  const title = channelBrowserFirstValue(
    channel,
    ['name', 'channelName', 'title', 'displayName', 'id', 'channelId'],
    'Kanal ' + String(index + 1)
  );
  const channelId = channelBrowserFirstValue(channel, ['channelId', 'id', 'nativeId'], '-');
  const currentProgram = channelCurrentProgram(channel);

  item.appendChild(createChannelLogoElement(title, channelId));

  const text = document.createElement('div');
  text.className = 'channel-text';
  text.appendChild(channelBrowserAddText(document.createElement('div'), String(title))).className = 'list-title';

  if (currentProgram) {
    const programTitle = channelBrowserFirstValue(currentProgram, ['title', 'name', 'eventTitle'], 'Laufendes Programm');
    const subtitle = channelBrowserFirstValue(currentProgram, ['subtitle', 'shortText', 'short_text'], '');
    const timeText = channelProgramTimeText(currentProgram);

    text.appendChild(channelBrowserAddText(
      document.createElement('div'),
      'Jetzt: ' + String(programTitle)
    )).className = 'list-meta';

    if (timeText !== '' || subtitle !== '') {
      const details = [timeText, subtitle].filter(value => String(value).trim() !== '').join(' · ');
      text.appendChild(channelBrowserAddText(document.createElement('div'), details)).className = 'list-meta';
    }
  } else {
    text.appendChild(channelBrowserAddText(
      document.createElement('div'),
      'Jetzt: keine EPG-Information'
    )).className = 'list-meta';
  }

  text.appendChild(channelBrowserAddText(document.createElement('div'), channelStatusText(channel, encryptionAvailable))).className = 'list-meta';

  item.appendChild(text);
  return item;
}

function renderChannelSection(list, label, channels, globalOffset, encryptionAvailable) {
  if (channels.length === 0) {
    return;
  }

  const header = document.createElement('article');
  header.className = 'module-placeholder';
  const firstNumber = channelNumber(channels[0], 0);
  const lastNumber = channelNumber(channels[channels.length - 1], 0);

  header.appendChild(channelBrowserAddText(document.createElement('h3'), label));
  header.appendChild(channelBrowserAddText(
    document.createElement('p'),
    String(channels.length) + ' Kanal/Kanäle · Nummern ' + String(firstNumber) + '–' + String(lastNumber)
  ));
  list.appendChild(header);

  channels.forEach((channel, index) => {
    list.appendChild(renderChannelItem(channel, globalOffset + index, encryptionAvailable));
  });
}

function renderChannelBrowserList(data) {
  const channels = channelBrowserListFromResponse(data, 'channels');
  const dataEvents = Array.isArray(data.events) ? data.events : [];
  const fallbackEvents = currentEvents ? channelBrowserListFromResponse(currentEvents, 'events') : [];
  const events = dataEvents.length > 0 ? dataEvents : fallbackEvents;
  const nowSeconds = Math.floor(Date.now() / 1000);

  channelBrowserDetailDataElement().replaceChildren();

  if (channels.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(channelBrowserAddText(document.createElement('h3'), 'Keine Kanäle gefunden'));
    empty.appendChild(channelBrowserAddText(document.createElement('p'), 'Der Endpunkt /api/vdr/channels hat keine Kanalliste geliefert.'));
    channelBrowserDetailDataElement().appendChild(empty);
    return;
  }

  const encryptionAvailable = channelsHaveEncryptionInfo(channels);

  if (!encryptionAvailable) {
    channelListFilters.free = false;
    channelListFilters.encrypted = false;
  }

  if (channelListViewMode !== 'groups' && channelListViewMode !== 'number') {
    channelListViewMode = 'groups';
  }

  const filteredChannels = sortedChannels(filterChannels(channels, encryptionAvailable));
  const visibleCount = channelListViewMode === 'groups'
    ? filteredChannels.length
    : Math.min(channelListVisibleCount, filteredChannels.length);
  const visibleChannels = filteredChannels.slice(0, visibleCount);

  const shell = document.createElement('section');
  shell.className = 'list channel-browser-module';

  const overview = document.createElement('article');
  overview.className = 'module-placeholder channel-browser-intro';
  overview.appendChild(channelBrowserAddText(document.createElement('h3'), 'Kanalliste'));
  overview.appendChild(channelBrowserAddText(
    document.createElement('p'),
    channelListViewMode === 'groups'
      ? 'Zeige ' + String(filteredChannels.length) + ' gefilterte Kanäle in einklappbaren Gruppen · ' + String(channels.length) + ' gesamt. Gruppe öffnen, Kanal links anklicken, rechts Programm ansehen.'
      : 'Zeige ' + String(visibleChannels.length) + ' von ' + String(filteredChannels.length) +
        ' gefilterten Kanälen · ' + String(channels.length) + ' gesamt. Kanal links anklicken, rechts Programm ansehen.'
  ));

  renderChannelViewButtons(overview, channels);
  renderChannelFilterButtons(overview, channels, encryptionAvailable);

  if (channelListViewMode === 'number') {
    renderChannelPagingControls(overview, channels, filteredChannels.length);
  }

  shell.appendChild(overview);

  if (visibleChannels.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(channelBrowserAddText(document.createElement('h3'), 'Keine Kanäle im Filter'));
    empty.appendChild(channelBrowserAddText(document.createElement('p'), 'Wähle einen anderen Filter.'));
    shell.appendChild(empty);
    channelBrowserDetailDataElement().appendChild(shell);
    return;
  }

  const workbench = document.createElement('section');
  workbench.className = 'channel-browser-workbench';

  const channelPane = document.createElement('div');
  channelPane.className = 'channel-browser-list';

  const detailPane = document.createElement('div');
  detailPane.className = 'channel-browser-detail';

  const state = {
    selectedIndex: 0,
    selectedEventKey: ''
  };

  function resetChannelBrowserMobileHorizontalScroll() {
    if (!(window.matchMedia && window.matchMedia('(max-width: 760px)').matches)) {
      return;
    }

    document.documentElement.scrollLeft = 0;
    document.body.scrollLeft = 0;
    shell.scrollLeft = 0;
    workbench.scrollLeft = 0;
    channelPane.scrollLeft = 0;
    detailPane.scrollLeft = 0;

    if (window.scrollX !== 0) {
      window.scrollTo(0, window.scrollY);
    }
  }

  const channelBrowserPrefetchedChannelIds = {};
  let channelBrowserGroupPrefetchInFlight = false;

  function channelBrowserEventIdentity(event) {
    return frontendEventChannelId(event) + '|'
      + String(channelBrowserFirstValue(event, ['eventId', 'id', 'nativeId'], '')) + '|'
      + String(channelBrowserFirstValue(event, ['startTime', 'start', 'beginTime'], '')) + '|'
      + epgEventTitle(event);
  }

  function mergeChannelBrowserPrefetchedEvents(responses) {
    const known = {};
    let added = 0;

    events.forEach(event => {
      known[channelBrowserEventIdentity(event)] = true;
    });

    responses.forEach(response => {
      channelBrowserListEventsFromEpgResponse(response).forEach(event => {
        const key = channelBrowserEventIdentity(event);
        if (known[key]) {
          return;
        }

        known[key] = true;
        events.push(event);
        added += 1;
      });
    });

    if (added > 0) {
      currentEvents = {
        events,
        eventCount: events.length,
        __source: 'cache-channel-browser-prefetch',
        __partialWindow: true,
        __debugUrl: 'channel-browser group prefetch'
      };
    }

    return added;
  }

  function prefetchChannelBrowserChannels(channelsToPrefetch) {
    if (channelBrowserGroupPrefetchInFlight || typeof fetchChannelBrowserChannelWindow !== 'function') {
      return;
    }

    const candidates = channelsToPrefetch
      .filter(channel => {
        const channelId = frontendChannelId(channel);
        return channelId !== '' &&
          channelBrowserPrefetchedChannelIds[channelId] !== true &&
          channelEntries(channel).length === 0;
      })
      .slice(0, 12);

    if (candidates.length === 0) {
      return;
    }

    candidates.forEach(channel => {
      channelBrowserPrefetchedChannelIds[frontendChannelId(channel)] = true;
    });

    channelBrowserGroupPrefetchInFlight = true;

    Promise.all(candidates.map(channel => fetchChannelBrowserChannelWindow(channel)))
      .then(responses => {
        if (mergeChannelBrowserPrefetchedEvents(responses) > 0) {
          renderAll();
        }
      })
      .finally(() => {
        channelBrowserGroupPrefetchInFlight = false;
      });
  }

  function eventKey(entry) {
    return [
      frontendEventChannelId(entry.event),
      String(entry.start),
      String(entry.end),
      String(channelBrowserFirstValue(entry.event, ['eventId', 'id', 'nativeId'], epgEventTitle(entry.event)))
    ].join(':');
  }

  function channelEntries(channel) {
    return epgEventsForChannel(channel, events, nowSeconds)
      .filter(entry => entry.end > nowSeconds)
      .slice(0, 48);
  }

  function currentEntry(entries) {
    return entries.find(entry => entry.start <= nowSeconds && nowSeconds < entry.end) || null;
  }

  function nextEntry(entries) {
    return entries.find(entry => entry.start >= nowSeconds) || null;
  }

  function selectedChannel() {
    if (state.selectedIndex < 0 || state.selectedIndex >= visibleChannels.length) {
      state.selectedIndex = 0;
    }

    return visibleChannels[state.selectedIndex] || null;
  }

  function selectedEntry(entries) {
    if (entries.length === 0) {
      return null;
    }

    if (state.selectedEventKey !== '') {
      const match = entries.find(entry => eventKey(entry) === state.selectedEventKey);
      if (match) {
        return match;
      }
    }

    return currentEntry(entries) || entries[0];
  }

  function renderChannelButton(channel, index) {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'channel-browser-item' + (index === state.selectedIndex ? ' active' : '');
    button.setAttribute('aria-pressed', index === state.selectedIndex ? 'true' : 'false');

    const channelTitle = epgChannelTitle(channel, index);
    const channelId = channelBrowserFirstValue(channel, ['channelId', 'id', 'nativeId'], '');
    const number = channelBrowserFirstValue(channel, ['number', 'channelNumber', 'position'], String(index + 1));

    const row = document.createElement('div');
    row.className = 'channel-list-item';

    if (typeof createChannelLogoElement === 'function') {
      const logo = createChannelLogoElement(channelTitle, channelId);
      logo.classList.add('epg-channel-logo');
      row.appendChild(logo);
    }

    const textBlock = document.createElement('div');
    textBlock.className = 'channel-text';
    textBlock.appendChild(channelBrowserAddText(document.createElement('div'), channelTitle)).className = 'list-title';
    textBlock.appendChild(channelBrowserAddText(
      document.createElement('div'),
      'Nr. ' + String(number) + ' · ' + String(channelId || '-')
    )).className = 'list-meta';

    const entries = channelEntries(channel);
    const current = currentEntry(entries);
    const preview = current || nextEntry(entries);

    if (preview) {
      textBlock.appendChild(channelBrowserAddText(
        document.createElement('div'),
        (current ? 'Jetzt: ' : 'Als nächstes: ') + epgEventTitle(preview.event) + ' · '
          + formatEpgClockFromEpoch(preview.start) + '–' + formatEpgClockFromEpoch(preview.end)
      )).className = 'list-meta channel-browser-now';
    } else {
      textBlock.appendChild(channelBrowserAddText(
        document.createElement('div'),
        'EPG wird geladen...'
      )).className = 'list-meta channel-browser-now';
    }

    row.appendChild(textBlock);
    button.appendChild(row);

    button.addEventListener('click', () => {
      if (channelDragRecentlyEnded(channelPane)) {
        return;
      }

      state.selectedIndex = index;
      state.selectedEventKey = '';
      renderAll();
    });

    return button;
  }

  function renderAgendaRow(entry, channel, active) {
    const row = document.createElement('div');
    row.className = 'channel-agenda-row' + (active ? ' active' : '');
    row.setAttribute('role', 'button');
    row.setAttribute('tabindex', '0');

    if (entry.start <= nowSeconds && nowSeconds < entry.end) {
      row.classList.add('current');
    }

    const timeBox = document.createElement('div');
    timeBox.className = 'channel-agenda-timebox';

    timeBox.appendChild(channelBrowserAddText(
      document.createElement('div'),
      new Date(entry.start * 1000).toLocaleDateString('de-DE', { weekday: 'short', day: '2-digit', month: '2-digit' })
    )).className = 'channel-agenda-date';

    timeBox.appendChild(channelBrowserAddText(
      document.createElement('div'),
      formatEpgClockFromEpoch(entry.start) + '–' + formatEpgClockFromEpoch(entry.end)
    )).className = 'channel-agenda-time';

    timeBox.appendChild(channelBrowserAddText(
      document.createElement('div'),
      formatEpgDuration(entry.start, entry.end)
    )).className = 'channel-agenda-duration';

    const content = document.createElement('div');
    content.className = 'channel-agenda-content';

    content.appendChild(channelBrowserAddText(document.createElement('div'), epgEventTitle(entry.event))).className = 'channel-agenda-title';

    const subtitle = epgEventSubtitle(entry.event);
    if (subtitle !== '' && subtitle !== epgEventTitle(entry.event)) {
      content.appendChild(channelBrowserAddText(document.createElement('div'), subtitle)).className = 'channel-agenda-subtitle';
    }

    row.appendChild(timeBox);
    row.appendChild(content);

    const activateRow = () => {
      const scrollArea = row.closest('.channel-agenda-scroll');
      if (channelDragRecentlyEnded(scrollArea)) {
        return;
      }

      state.selectedEventKey = eventKey(entry);
      renderAll();
    };

    row.addEventListener('click', activateRow);

    row.addEventListener('keydown', event => {
      if (event.key !== 'Enter' && event.key !== ' ') {
        return;
      }

      event.preventDefault();
      activateRow();
    });

    return row;
  }

  function renderChannelPane() {
    channelPane.replaceChildren();

    if (channelListViewMode !== 'groups') {
      visibleChannels.forEach((channel, index) => {
        channelPane.appendChild(renderChannelButton(channel, index));
      });
      prefetchChannelBrowserChannels(visibleChannels);
      return;
    }

    const selected = selectedChannel();
    const selectedGroup = selected ? channelGroupName(selected) : '';

    if (selectedGroup !== '' && Object.keys(channelListOpenGroups).length === 0) {
      channelListOpenGroups[selectedGroup] = true;
    }

    const groups = new Map();

    visibleChannels.forEach((channel, index) => {
      const group = channelGroupName(channel);
      if (!groups.has(group)) {
        groups.set(group, []);
      }
      groups.get(group).push({ channel, index });
    });

    groups.forEach((items, group) => {
      const open = channelListOpenGroups[group] === true;
      const section = document.createElement('section');
      section.className = 'channel-browser-group' + (open ? ' open' : '');

      const toggle = document.createElement('button');
      toggle.type = 'button';
      toggle.className = 'channel-browser-group-toggle';
      toggle.setAttribute('aria-expanded', open ? 'true' : 'false');

      const chevron = channelBrowserAddText(document.createElement('span'), open ? '▾' : '▸');
      chevron.className = 'channel-browser-group-chevron';
      toggle.appendChild(chevron);

      const copy = document.createElement('span');
      copy.className = 'channel-browser-group-copy';
      copy.appendChild(channelBrowserAddText(document.createElement('strong'), group));
      copy.appendChild(channelBrowserAddText(
        document.createElement('span'),
        String(items.length) + ' Sender · ' + (open ? 'einklappen' : 'ausklappen')
      ));
      toggle.appendChild(copy);

      toggle.addEventListener('click', () => {
        if (channelDragRecentlyEnded(channelPane)) {
          return;
        }

        channelListOpenGroups[group] = !open;
        renderChannelPane();
        resetChannelBrowserMobileHorizontalScroll();

        requestAnimationFrame(() => {
          enableChannelMouseDragScroll(channelPane, 'y');
        });
      });

      section.appendChild(toggle);

      if (open) {
        const itemList = document.createElement('div');
        itemList.className = 'channel-browser-group-items';

        items.forEach(entry => {
          itemList.appendChild(renderChannelButton(entry.channel, entry.index));
        });

        prefetchChannelBrowserChannels(items.map(entry => entry.channel));

        section.appendChild(itemList);
      }

      channelPane.appendChild(section);
    });

    const hint = channelBrowserAddText(
      document.createElement('p'),
      'Tipp: Kanalliste mit gedrückter Maustaste hoch/runter ziehen.'
    );
    hint.className = 'channel-browser-group-footer';
    channelPane.appendChild(hint);
  }

  function renderDetailPane() {
    detailPane.replaceChildren();

    const channel = selectedChannel();
    if (!channel) {
      const empty = document.createElement('article');
      empty.className = 'module-placeholder';
      empty.appendChild(channelBrowserAddText(document.createElement('h3'), 'Kein Kanal ausgewählt'));
      detailPane.appendChild(empty);
      return;
    }

    const channelTitle = epgChannelTitle(channel, state.selectedIndex);
    const channelId = channelBrowserFirstValue(channel, ['channelId', 'id', 'nativeId'], '');
    const number = channelBrowserFirstValue(channel, ['number', 'channelNumber', 'position'], String(state.selectedIndex + 1));
    const entries = channelEntries(channel);
    const active = selectedEntry(entries);
    const current = currentEntry(entries);
    const next = nextEntry(entries);

    if (entries.length === 0 &&
        channelBrowserEpgPrefetchInFlight === false &&
        typeof fetchCachedEpgWindowForVisibleChannel === 'function') {
      const prefetchNow = Date.now();
      if (prefetchNow - channelBrowserEpgPrefetchLastStartedAt >= CHANNEL_BROWSER_EPG_RETRY_DELAY_MS) {
        channelBrowserEpgPrefetchInFlight = true;
        channelBrowserEpgPrefetchLastStartedAt = prefetchNow;

        fetchCachedEpgWindowForVisibleChannel(channel)
          .then(eventData => {
            if (channelBrowserListEventsFromEpgResponse(eventData).length > 0) {
              return eventData;
            }

            const channelId = frontendChannelId(channel);
            if (channelId === '') {
              return eventData;
            }

            const backendId = selectedEpgBackendId();
            const bounds = epgWindowBounds();
            const clientApi = window.VdrSuiteClientApi;

            if (!clientApi || typeof clientApi.fetchClientEpgCacheRefresh !== 'function') {
              return eventData;
            }

            return clientApi.fetchClientEpgCacheRefresh({
              query: {
                backend: backendId,
                channelId: channelId,
                from: String(bounds.from),
                timespan: String(Math.max(7200, bounds.until - bounds.from)),
                limit: '0',
                channelEventLimit: '96',
                _: String(Date.now())
              },
              cache: 'no-store'
            })
              .then(() => fetchCachedEpgWindowForVisibleChannel(channel)
                .catch(() => eventData))
              .catch(() => eventData);
          })
          .then(eventData => {
            const incomingEvents = channelBrowserListEventsFromEpgResponse(eventData);
            incomingEvents.forEach(event => events.push(event));
            currentEvents = {
              events,
              eventCount: events.length,
              __source: 'cache-channel-browser-selected',
              __partialWindow: true,
              __debugUrl: String(eventData.__debugUrl || '')
            };
            renderAll();
          })
          .catch(error => {
            (void error);
          })
          .finally(() => {
            channelBrowserEpgPrefetchInFlight = false;
          });
      }
    }

    const hero = document.createElement('article');
    hero.className = 'module-placeholder channel-browser-selected';

    const head = document.createElement('div');
    head.className = 'channel-browser-selected-head';

    if (typeof createChannelLogoElement === 'function') {
      const logo = createChannelLogoElement(channelTitle, channelId);
      logo.classList.add('channel-browser-detail-logo');
      head.appendChild(logo);
    }

    const titleBlock = document.createElement('div');
    titleBlock.className = 'channel-browser-detail-headline';
    titleBlock.appendChild(channelBrowserAddText(document.createElement('h3'), channelTitle));
    titleBlock.appendChild(channelBrowserAddText(
      document.createElement('p'),
      'Kanalnummer ' + String(number) + ' · ' + String(channelId || '-')
    ));

    head.appendChild(titleBlock);
    hero.appendChild(head);

    const summary = document.createElement('div');
    summary.className = 'channel-browser-summary';

    const nowCard = document.createElement('div');
    nowCard.className = 'channel-browser-summary-card';
    nowCard.appendChild(channelBrowserAddText(document.createElement('div'), 'Läuft jetzt')).className = 'channel-browser-summary-label';
    nowCard.appendChild(channelBrowserAddText(
      document.createElement('div'),
      current
        ? epgEventTitle(current.event) + ' · ' + formatEpgClockFromEpoch(current.start) + '–' + formatEpgClockFromEpoch(current.end)
        : 'keine laufende Sendung'
    )).className = 'channel-browser-summary-value';
    summary.appendChild(nowCard);

    const nextCard = document.createElement('div');
    nextCard.className = 'channel-browser-summary-card';
    nextCard.appendChild(channelBrowserAddText(document.createElement('div'), 'Als nächstes')).className = 'channel-browser-summary-label';
    nextCard.appendChild(channelBrowserAddText(
      document.createElement('div'),
      next
        ? epgEventTitle(next.event) + ' · ' + formatEpgClockFromEpoch(next.start) + '–' + formatEpgClockFromEpoch(next.end)
        : 'keine nächste Sendung'
    )).className = 'channel-browser-summary-value';
    summary.appendChild(nextCard);

    hero.appendChild(summary);
    detailPane.appendChild(hero);

    const agenda = document.createElement('article');
    agenda.className = 'module-placeholder channel-agenda-card';
    agenda.appendChild(channelBrowserAddText(document.createElement('h3'), 'Programm'));
    agenda.appendChild(channelBrowserAddText(
      document.createElement('p'),
      'Zeit links, Sendung rechts. Mit Mausrad oder gedrückter Maustaste scrollen, Eintrag anklicken für Details.'
    )).className = 'channel-agenda-hint';

    const scroll = document.createElement('div');
    scroll.className = 'channel-agenda-scroll';
    scroll.title = 'Programm mit gedrückter Maustaste hoch/runter ziehen.';
    scroll.setAttribute('aria-label', 'Programm des ausgewählten Kanals');

    if (entries.length === 0) {
      scroll.appendChild(channelBrowserAddText(
        document.createElement('div'),
        'Keine Programmdaten im aktuellen Zeitfenster gefunden.'
      )).className = 'channel-agenda-empty';
    } else {
      entries.forEach(entry => {
        const rowActive = active && eventKey(entry) === eventKey(active);
        scroll.appendChild(renderAgendaRow(entry, channel, rowActive));

        if (rowActive && state.selectedEventKey.length > 0 &&
            window.matchMedia && window.matchMedia('(max-width: 760px)').matches) {
          const inlineDetail = createEpgEventDetailCard(entry.event, channel);
          inlineDetail.classList.add('channel-agenda-inline-detail');
          scroll.appendChild(inlineDetail);
        }
      });
    }

    agenda.appendChild(scroll);
    detailPane.appendChild(agenda);

    if (active && !(window.matchMedia && window.matchMedia('(max-width: 760px)').matches)) {
      detailPane.appendChild(createEpgEventDetailCard(active.event, channel));
    }
  }

  function renderAll() {
    renderChannelPane();
    renderDetailPane();

    requestAnimationFrame(() => {
      enableChannelMouseDragScroll(channelPane, 'y');
      enableChannelMouseDragScroll(detailPane.querySelector('.channel-agenda-scroll'), 'y');

      resetChannelBrowserMobileHorizontalScroll();
    });
  }

  workbench.appendChild(channelPane);
  workbench.appendChild(detailPane);
  shell.appendChild(workbench);
  channelBrowserDetailDataElement().appendChild(shell);

  if (!visibleChannels.some(channel => channelEntries(channel).length > 0)) {
    scheduleChannelBrowserEpgPrefetch(data);
  }

  renderAll();
}

renderChannelList = function(data) {
  return renderChannelBrowserList(data);
};

if (typeof window !== 'undefined') {
  window.VdrSuiteChannelBrowser = Object.freeze({
    configureContext: configureChannelBrowserContext,
    renderList: renderChannelBrowserList
  });
}
