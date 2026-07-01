let channelListViewMode = 'groups';
let channelListFilterMode = 'all';
let channelListVisibleCount = 20;

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

function filterChannels(channels) {
  return channels.filter(channel => {
    const radio = channelBoolean(channel, ['radio', 'isRadio'], false);
    const encrypted = channelBoolean(channel, ['encrypted', 'scrambled', 'isEncrypted'], false);
    const enabled = channelBoolean(channel, ['enabled', 'active'], true);

    if (channelListFilterMode === 'tv') {
      return !radio;
    }

    if (channelListFilterMode === 'radio') {
      return radio;
    }

    if (channelListFilterMode === 'free') {
      return !encrypted;
    }

    if (channelListFilterMode === 'encrypted') {
      return encrypted;
    }

    if (channelListFilterMode === 'enabled') {
      return enabled;
    }

    if (channelListFilterMode === 'disabled') {
      return !enabled;
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

function renderChannelFilterButtons(container, channels) {
  const controls = document.createElement('div');
  controls.className = 'module-nav';
  controls.setAttribute('aria-label', 'Kanallisten-Filter');

  const filters = [
    ['all', 'Alle'],
    ['tv', 'TV'],
    ['radio', 'Radio'],
    ['free', 'Frei'],
    ['encrypted', 'Verschlüsselt'],
    ['enabled', 'Aktiv'],
    ['disabled', 'Deaktiviert']
  ];

  filters.forEach(([value, label]) => {
    renderChannelButton(
      controls,
      label,
      channelListFilterMode === value,
      false,
      () => {
        channelListFilterMode = value;
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

function channelStatusText(channel) {
  const radio = channelBoolean(channel, ['radio', 'isRadio'], false);
  const encrypted = channelBoolean(channel, ['encrypted', 'scrambled', 'isEncrypted'], false);
  const enabled = channelBoolean(channel, ['enabled', 'active'], true);
  const parts = [];

  parts.push(radio ? 'Radio' : 'TV');
  parts.push(encrypted ? 'verschlüsselt' : 'frei');
  parts.push(enabled ? 'aktiv' : 'deaktiviert');

  return parts.join(' · ');
}

function renderChannelItem(channel, index) {
  const item = document.createElement('article');
  item.className = 'list-item channel-list-item';
  const title = firstValue(
    channel,
    ['name', 'channelName', 'title', 'displayName', 'id', 'channelId'],
    'Kanal ' + String(index + 1)
  );
  const channelId = firstValue(channel, ['channelId', 'id', 'nativeId'], '-');
  const number = channelNumber(channel, index + 1);
  const group = channelGroupName(channel);

  item.appendChild(createChannelLogoElement(title, channelId));

  const text = document.createElement('div');
  text.className = 'channel-text';
  text.appendChild(addText(document.createElement('div'), String(title))).className = 'list-title';
  text.appendChild(addText(
    document.createElement('div'),
    'Nummer: ' + String(number) + ' · ID: ' + String(channelId)
  )).className = 'list-meta';
  text.appendChild(addText(document.createElement('div'), channelStatusText(channel))).className = 'list-meta';

  if (group !== 'Ohne Gruppe') {
    text.appendChild(addText(document.createElement('div'), 'Gruppe: ' + group)).className = 'list-meta';
  }

  item.appendChild(text);
  return item;
}

function renderChannelSection(list, label, channels, globalOffset) {
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
    list.appendChild(renderChannelItem(channel, globalOffset + index));
  });
}

renderChannelList = function(data) {
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

  if (!hasRealChannelGroups(channels)) {
    channelListViewMode = 'number';
  }

  const filteredChannels = sortedChannels(filterChannels(channels));
  const visibleCount = Math.min(channelListVisibleCount, filteredChannels.length);
  const visibleChannels = filteredChannels.slice(0, visibleCount);

  const overview = document.createElement('article');
  overview.className = 'module-placeholder';
  overview.appendChild(addText(document.createElement('h3'), 'Kanalliste'));
  overview.appendChild(addText(
    document.createElement('p'),
    'Zeige ' + String(visibleChannels.length) + ' von ' + String(filteredChannels.length) +
      ' gefilterten Kanälen · ' + String(channels.length) + ' gesamt.'
  ));
  renderChannelViewButtons(overview, channels);
  renderChannelFilterButtons(overview, channels);
  renderChannelPagingControls(overview, channels, filteredChannels.length);
  list.appendChild(overview);

  if (visibleChannels.length === 0) {
    const empty = document.createElement('article');
    empty.className = 'module-placeholder';
    empty.appendChild(addText(document.createElement('h3'), 'Keine Kanäle im Filter'));
    empty.appendChild(addText(document.createElement('p'), 'Wähle einen anderen Filter.'));
    list.appendChild(empty);
    detailDataElement.appendChild(list);
    return;
  }

  const sections = channelListViewMode === 'groups'
    ? groupChannelsByVdrGroup(visibleChannels)
    : groupChannelsByLoadedPages(visibleChannels);

  let offset = 0;
  sections.forEach(([label, sectionChannels]) => {
    renderChannelSection(list, label, sectionChannels, offset);
    offset += sectionChannels.length;
  });

  detailDataElement.appendChild(list);
};
