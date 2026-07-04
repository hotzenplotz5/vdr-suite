let channelListViewMode = 'groups';
let channelListVisibleCount = 20;
let channelListFilters = {
  tv: false,
  radio: false,
  free: false,
  encrypted: false,
  enabled: false,
  disabled: false
};

const CHANNEL_LIST_PAGE_SIZE = 20;

function normalizeChannelLogoName(value) {
  return String(value || '')
    .trim()
    .replace(/\s+/g, ' ')
    .toLocaleLowerCase('de-DE');
}

function addUniqueValue(values, value) {
  const normalized = normalizeChannelLogoName(value);

  if (normalized !== '' && !values.includes(normalized)) {
    values.push(normalized);
  }
}

function addChannelLogoAliases(values, value) {
  const normalized = normalizeChannelLogoName(value);
  const withoutQuality = normalized.replace(/\s+(hd|uhd|sd)$/i, '');

  const aliasMap = {
    'zdfneo hd': ['zdf_neo hd', 'zdf neo hd', 'zdfneo', 'zdf_neo', 'zdf neo'],
    'zdfneo': ['zdf_neo', 'zdf neo'],
    'zdfinfo hd': ['zdfinfo', 'zdf.info', 'zdf info hd', 'zdf info'],
    'zdfinfo': ['zdf.info', 'zdf info'],
    'tagesschau24 hd': ['tagesschau24', 'tagesschau 24 hd', 'tagesschau 24'],
    'tagesschau24': ['tagesschau 24'],
    'one hd': ['one'],
    'phoenix hd': ['phoenix'],
    'arte hd': ['arte'],
    '3sat hd': ['3sat'],
    'das erste hd': ['das erste'],
    'ndr fs hh hd': ['ndr fs hh', 'ndr fernsehen hd', 'ndr fernsehen'],
    'ndr fs mv hd': ['ndr fs mv', 'ndr fernsehen hd', 'ndr fernsehen'],
    'ndr fs nds hd': ['ndr fs nds', 'ndr fernsehen hd', 'ndr fernsehen'],
    'ndr fs sh hd': ['ndr fs sh', 'ndr fernsehen hd', 'ndr fernsehen'],
    'ntv': ['n-tv', 'n tv'],
    'welt hd': ['welt'],
    'pro sieben hd': ['pro sieben', 'prosieben hd', 'prosieben', 'pro7 hd', 'pro7'],
    'pro sieben': ['prosieben', 'pro7'],
    'sat.1 hd': ['sat.1', 'sat1 hd', 'sat1'],
    'sat.1': ['sat1'],
    'rtlup hd': ['rtlup', 'rtl up hd', 'rtl up'],
    'rtlup': ['rtl up'],
    'kabel eins hd': ['kabel eins', 'kabeleins hd', 'kabeleins'],
    'kabel eins': ['kabeleins'],
    'sixx hd': ['sixx'],
    'dmax hd': ['dmax'],
    'tele 5 hd': ['tele 5', 'tele5 hd', 'tele5'],
    'sport1 hd': ['sport1'],
    'servustv hd deutschland': ['servustv deutschland', 'servus tv hd deutschland', 'servus tv deutschland'],
    'servustv deutschland': ['servus tv deutschland']
  };

  [normalized, withoutQuality].forEach(key => {
    const aliases = aliasMap[key] || [];
    aliases.forEach(alias => addUniqueValue(values, alias));
  });

  if (/^zdf[a-z]/.test(normalized)) {
    addUniqueValue(values, normalized.replace(/^zdf/, 'zdf_'));
    addUniqueValue(values, normalized.replace(/^zdf/, 'zdf '));
  }

  if (/^zdf[a-z]/.test(withoutQuality)) {
    addUniqueValue(values, withoutQuality.replace(/^zdf/, 'zdf_'));
    addUniqueValue(values, withoutQuality.replace(/^zdf/, 'zdf '));
  }
}

function addChannelLogoNameVariants(values, value) {
  const normalized = normalizeChannelLogoName(value);

  if (normalized === '') {
    return;
  }

  addUniqueValue(values, normalized);
  addUniqueValue(values, normalized.replace(/\s*;.*$/, ''));
  addUniqueValue(values, normalized.replace(/\s*\(.*\)\s*$/, ''));
  addUniqueValue(values, normalized.replace(/\s+(hd|uhd|sd)$/i, ''));
  addUniqueValue(values, normalized.replace(/\s+/g, '_'));
  addUniqueValue(values, normalized.replace(/\s+/g, '-'));
  addUniqueValue(values, normalized.replace(/\s+/g, ''));

  const withoutQuality = normalized.replace(/\s+(hd|uhd|sd)$/i, '');
  addUniqueValue(values, withoutQuality.replace(/\s+/g, '_'));
  addUniqueValue(values, withoutQuality.replace(/\s+/g, '-'));
  addUniqueValue(values, withoutQuality.replace(/\s+/g, ''));
  addChannelLogoAliases(values, normalized);
}

function channelLogoPathForName(name, extension) {
  const parts = normalizeChannelLogoName(name)
    .split('/')
    .map(part => part.trim())
    .filter(part => part !== '');

  if (parts.length === 0) {
    return '';
  }

  return '/channel-logos/' + parts.map(part => encodeURIComponent(part)).join('/') + extension;
}

function uniqueChannelLogoCandidates(title, channelId) {
  const names = [];
  const normalizedTitle = normalizeChannelLogoName(title);
  const normalizedChannelId = normalizeChannelLogoName(channelId);

  addChannelLogoNameVariants(names, normalizedTitle);

  if (normalizedChannelId !== '' && normalizedChannelId !== '-' && normalizedChannelId !== normalizedTitle) {
    addChannelLogoNameVariants(names, normalizedChannelId);
  }

  const candidates = [];

  names.forEach(name => {
    ['.png', '.svg'].forEach(extension => {
      const path = channelLogoPathForName(name, extension);
      if (path !== '' && !candidates.includes(path)) {
        candidates.push(path);
      }
    });
  });

  return candidates;
}

function channelLogoInitial(title) {
  const text = String(title || '?').trim();

  if (text === '') {
    return '?';
  }

  return Array.from(text)[0].toLocaleUpperCase('de-DE');
}

function createChannelLogoElement(title, channelId) {
  const frame = document.createElement('div');
  frame.className = 'channel-logo-frame';
  frame.title = 'Logo-Kandidaten: ' + uniqueChannelLogoCandidates(title, channelId).slice(0, 4).join(' | ');

  const fallback = addText(document.createElement('div'), channelLogoInitial(title));
  fallback.className = 'channel-logo-fallback';
  frame.appendChild(fallback);

  const candidates = uniqueChannelLogoCandidates(title, channelId);

  if (candidates.length === 0) {
    return frame;
  }

  const image = document.createElement('img');
  image.className = 'channel-logo';
  image.alt = 'Logo ' + String(title || channelId || 'Kanal');
  image.style.display = 'block';
  image.style.maxWidth = '100%';
  image.style.maxHeight = '100%';
  image.style.objectFit = 'contain';
  image.style.opacity = '0';

  let index = 0;

  function tryNextCandidate() {
    if (index >= candidates.length) {
      image.remove();
      frame.classList.remove('loaded');
      fallback.style.display = '';
      return;
    }

    image.src = candidates[index];
    index += 1;
  }

  image.addEventListener('load', () => {
    frame.classList.add('loaded');
    image.style.opacity = '1';
    fallback.style.display = 'none';
  });

  image.addEventListener('error', () => {
    frame.classList.remove('loaded');
    image.style.opacity = '0';
    fallback.style.display = '';
    tryNextCandidate();
  });

  frame.appendChild(image);
  tryNextCandidate();

  return frame;
}

function channelNumber(channel, fallback) {
  const value = Number(firstValue(channel, ['number', 'channelNumber', 'position'], fallback));
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

    const leftName = normalizeChannelLogoName(firstValue(left, ['name', 'channelName', 'title', 'displayName'], ''));
    const rightName = normalizeChannelLogoName(firstValue(right, ['name', 'channelName', 'title', 'displayName'], ''));
    return leftName.localeCompare(rightName, 'de-DE');
  });
}

function channelGroupName(channel) {
  const group = String(firstValue(channel, ['group', 'groupName', 'bouquet', 'category'], '')).trim();
  return group === '' ? 'Ohne Gruppe' : group;
}

function hasRealChannelGroups(channels) {
  const groups = new Set();

  channels.forEach(channel => {
    const group = String(firstValue(channel, ['group', 'groupName', 'bouquet', 'category'], '')).trim();
    if (group !== '') {
      groups.add(group);
    }
  });

  return groups.size > 0;
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
  controls.className = 'module-nav';
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
  controls.className = 'module-nav';
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
  controls.className = 'module-nav';
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

  const start = channelProgramClock(firstValue(event, ['startTime', 'start', 'beginTime'], ''));
  const end = channelProgramClock(firstValue(event, ['endTime', 'end', 'stopTime'], ''));

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
  const title = firstValue(
    channel,
    ['name', 'channelName', 'title', 'displayName', 'id', 'channelId'],
    'Kanal ' + String(index + 1)
  );
  const channelId = firstValue(channel, ['channelId', 'id', 'nativeId'], '-');
  const currentProgram = channelCurrentProgram(channel);

  item.appendChild(createChannelLogoElement(title, channelId));

  const text = document.createElement('div');
  text.className = 'channel-text';
  text.appendChild(addText(document.createElement('div'), String(title))).className = 'list-title';

  if (currentProgram) {
    const programTitle = firstValue(currentProgram, ['title', 'name', 'eventTitle'], 'Laufendes Programm');
    const subtitle = firstValue(currentProgram, ['subtitle', 'shortText', 'short_text'], '');
    const timeText = channelProgramTimeText(currentProgram);

    text.appendChild(addText(
      document.createElement('div'),
      'Jetzt: ' + String(programTitle)
    )).className = 'list-meta';

    if (timeText !== '' || subtitle !== '') {
      const details = [timeText, subtitle].filter(value => String(value).trim() !== '').join(' · ');
      text.appendChild(addText(document.createElement('div'), details)).className = 'list-meta';
    }
  } else {
    text.appendChild(addText(
      document.createElement('div'),
      'Jetzt: keine EPG-Information'
    )).className = 'list-meta';
  }

  text.appendChild(addText(document.createElement('div'), channelStatusText(channel, encryptionAvailable))).className = 'list-meta';

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

  header.appendChild(addText(document.createElement('h3'), label));
  header.appendChild(addText(
    document.createElement('p'),
    String(channels.length) + ' Kanal/Kanäle · Nummern ' + String(firstNumber) + '–' + String(lastNumber)
  ));
  list.appendChild(header);

  channels.forEach((channel, index) => {
    list.appendChild(renderChannelItem(channel, globalOffset + index, encryptionAvailable));
  });
}

renderChannelList = function(data) {
  const channels = listFromResponse(data, 'channels');
  const dataEvents = Array.isArray(data.events) ? data.events : [];
  const fallbackEvents = currentEvents ? listFromResponse(currentEvents, 'events') : [];
  const events = dataEvents.length > 0 ? dataEvents : fallbackEvents;
  const nowSeconds = Math.floor(Date.now() / 1000);

  detailDataElement.replaceChildren();

  if (channels.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(addText(document.createElement('h3'), 'Keine Kanäle gefunden'));
    empty.appendChild(addText(document.createElement('p'), 'Der Endpunkt /api/vdr/channels hat keine Kanalliste geliefert.'));
    detailDataElement.appendChild(empty);
    return;
  }

  const encryptionAvailable = channelsHaveEncryptionInfo(channels);

  if (!encryptionAvailable) {
    channelListFilters.free = false;
    channelListFilters.encrypted = false;
  }

  if (!hasRealChannelGroups(channels)) {
    channelListViewMode = 'number';
  }

  const filteredChannels = sortedChannels(filterChannels(channels, encryptionAvailable));
  const visibleCount = Math.min(channelListVisibleCount, filteredChannels.length);
  const visibleChannels = filteredChannels.slice(0, visibleCount);

  const shell = document.createElement('section');
  shell.className = 'list channel-browser-module';

  const overview = document.createElement('article');
  overview.className = 'module-placeholder channel-browser-intro';
  overview.appendChild(addText(document.createElement('h3'), 'Kanalliste'));
  overview.appendChild(addText(
    document.createElement('p'),
    'Zeige ' + String(visibleChannels.length) + ' von ' + String(filteredChannels.length) +
      ' gefilterten Kanälen · ' + String(channels.length) + ' gesamt. Kanal links anklicken, rechts Programm ansehen.'
  ));

  renderChannelViewButtons(overview, channels);
  renderChannelFilterButtons(overview, channels, encryptionAvailable);
  renderChannelPagingControls(overview, channels, filteredChannels.length);
  shell.appendChild(overview);

  if (visibleChannels.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(addText(document.createElement('h3'), 'Keine Kanäle im Filter'));
    empty.appendChild(addText(document.createElement('p'), 'Wähle einen anderen Filter.'));
    shell.appendChild(empty);
    detailDataElement.appendChild(shell);
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

  function eventKey(entry) {
    return [
      frontendEventChannelId(entry.event),
      String(entry.start),
      String(entry.end),
      String(firstValue(entry.event, ['eventId', 'id', 'nativeId'], epgEventTitle(entry.event)))
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
    const channelId = firstValue(channel, ['channelId', 'id', 'nativeId'], '');
    const number = firstValue(channel, ['number', 'channelNumber', 'position'], String(index + 1));

    const row = document.createElement('div');
    row.className = 'channel-list-item';

    if (typeof createChannelLogoElement === 'function') {
      const logo = createChannelLogoElement(channelTitle, channelId);
      logo.classList.add('epg-channel-logo');
      row.appendChild(logo);
    }

    const textBlock = document.createElement('div');
    textBlock.className = 'channel-text';
    textBlock.appendChild(addText(document.createElement('div'), channelTitle)).className = 'list-title';
    textBlock.appendChild(addText(
      document.createElement('div'),
      'Nr. ' + String(number) + ' · ' + String(channelId || '-')
    )).className = 'list-meta';

    const entries = channelEntries(channel);
    const current = currentEntry(entries);

    if (current) {
      textBlock.appendChild(addText(
        document.createElement('div'),
        'Jetzt: ' + epgEventTitle(current.event) + ' · '
          + formatEpgClockFromEpoch(current.start) + '–' + formatEpgClockFromEpoch(current.end)
      )).className = 'list-meta channel-browser-now';
    }

    row.appendChild(textBlock);
    button.appendChild(row);

    button.addEventListener('click', () => {
      state.selectedIndex = index;
      state.selectedEventKey = '';
      renderAll();
    });

    return button;
  }

  function renderAgendaRow(entry, channel, active) {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'channel-agenda-row' + (active ? ' active' : '');

    if (entry.start <= nowSeconds && nowSeconds < entry.end) {
      button.classList.add('current');
    }

    const timeBox = document.createElement('div');
    timeBox.className = 'channel-agenda-timebox';

    timeBox.appendChild(addText(
      document.createElement('div'),
      formatEpgClockFromEpoch(entry.start) + '–' + formatEpgClockFromEpoch(entry.end)
    )).className = 'channel-agenda-time';

    timeBox.appendChild(addText(
      document.createElement('div'),
      formatEpgDuration(entry.start, entry.end)
    )).className = 'channel-agenda-duration';

    const content = document.createElement('div');
    content.className = 'channel-agenda-content';

    content.appendChild(addText(document.createElement('div'), epgEventTitle(entry.event))).className = 'channel-agenda-title';

    const subtitle = epgEventSubtitle(entry.event);
    if (subtitle !== '' && subtitle !== epgEventTitle(entry.event)) {
      content.appendChild(addText(document.createElement('div'), subtitle)).className = 'channel-agenda-subtitle';
    }

    button.appendChild(timeBox);
    button.appendChild(content);

    button.addEventListener('click', () => {
      state.selectedEventKey = eventKey(entry);
      renderAll();
    });

    return button;
  }

  function renderChannelPane() {
    channelPane.replaceChildren();

    visibleChannels.forEach((channel, index) => {
      channelPane.appendChild(renderChannelButton(channel, index));
    });
  }

  function renderDetailPane() {
    detailPane.replaceChildren();

    const channel = selectedChannel();
    if (!channel) {
      const empty = document.createElement('article');
      empty.className = 'module-placeholder';
      empty.appendChild(addText(document.createElement('h3'), 'Kein Kanal ausgewählt'));
      detailPane.appendChild(empty);
      return;
    }

    const channelTitle = epgChannelTitle(channel, state.selectedIndex);
    const channelId = firstValue(channel, ['channelId', 'id', 'nativeId'], '');
    const number = firstValue(channel, ['number', 'channelNumber', 'position'], String(state.selectedIndex + 1));
    const entries = channelEntries(channel);
    const active = selectedEntry(entries);
    const current = currentEntry(entries);
    const next = nextEntry(entries);

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
    titleBlock.appendChild(addText(document.createElement('h3'), channelTitle));
    titleBlock.appendChild(addText(
      document.createElement('p'),
      'Kanalnummer ' + String(number) + ' · ' + String(channelId || '-')
    ));

    head.appendChild(titleBlock);
    hero.appendChild(head);

    const summary = document.createElement('div');
    summary.className = 'channel-browser-summary';

    const nowCard = document.createElement('div');
    nowCard.className = 'channel-browser-summary-card';
    nowCard.appendChild(addText(document.createElement('div'), 'Läuft jetzt')).className = 'channel-browser-summary-label';
    nowCard.appendChild(addText(
      document.createElement('div'),
      current
        ? epgEventTitle(current.event) + ' · ' + formatEpgClockFromEpoch(current.start) + '–' + formatEpgClockFromEpoch(current.end)
        : 'keine laufende Sendung'
    )).className = 'channel-browser-summary-value';
    summary.appendChild(nowCard);

    const nextCard = document.createElement('div');
    nextCard.className = 'channel-browser-summary-card';
    nextCard.appendChild(addText(document.createElement('div'), 'Als nächstes')).className = 'channel-browser-summary-label';
    nextCard.appendChild(addText(
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
    agenda.appendChild(addText(document.createElement('h3'), 'Programm'));
    agenda.appendChild(addText(
      document.createElement('p'),
      'Zeit links, Sendung rechts. Mit Mausrad scrollen oder Eintrag anklicken.'
    )).className = 'channel-agenda-hint';

    const scroll = document.createElement('div');
    scroll.className = 'channel-agenda-scroll';

    if (entries.length === 0) {
      scroll.appendChild(addText(
        document.createElement('div'),
        'Keine Programmdaten im aktuellen Zeitfenster gefunden.'
      )).className = 'channel-agenda-empty';
    } else {
      entries.forEach(entry => {
        scroll.appendChild(renderAgendaRow(entry, channel, active && eventKey(entry) === eventKey(active)));
      });
    }

    agenda.appendChild(scroll);
    detailPane.appendChild(agenda);

    if (active) {
      detailPane.appendChild(createEpgEventDetailCard(active.event, channel));
    }
  }

  function renderAll() {
    renderChannelPane();
    renderDetailPane();
  }

  workbench.appendChild(channelPane);
  workbench.appendChild(detailPane);
  shell.appendChild(workbench);
  detailDataElement.appendChild(shell);

  renderAll();
};
