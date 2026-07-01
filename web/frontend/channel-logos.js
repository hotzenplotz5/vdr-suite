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

  let index = 0;

  function tryNextCandidate() {
    if (index >= candidates.length) {
      image.remove();
      frame.classList.remove('loaded');
      return;
    }

    image.src = candidates[index];
    index += 1;
  }

  image.addEventListener('load', () => {
    frame.classList.add('loaded');
  });

  image.addEventListener('error', () => {
    frame.classList.remove('loaded');
    tryNextCandidate();
  });

  frame.appendChild(image);
  tryNextCandidate();

  return frame;
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

  channels.slice(0, 20).forEach((channel, index) => {
    const item = document.createElement('article');
    item.className = 'list-item channel-list-item';
    const title = firstValue(
      channel,
      ['name', 'channelName', 'title', 'displayName', 'id', 'channelId'],
      'Kanal ' + String(index + 1)
    );
    const channelId = firstValue(channel, ['channelId', 'id', 'nativeId'], '-');
    const number = firstValue(channel, ['number', 'channelNumber', 'position'], String(index + 1));

    item.appendChild(createChannelLogoElement(title, channelId));

    const text = document.createElement('div');
    text.className = 'channel-text';
    text.appendChild(addText(document.createElement('div'), String(title))).className = 'list-title';
    text.appendChild(addText(
      document.createElement('div'),
      'Nummer: ' + String(number) + ' · ID: ' + String(channelId)
    )).className = 'list-meta';
    item.appendChild(text);

    list.appendChild(item);
  });

  if (channels.length > 20) {
    const info = document.createElement('article');
    info.className = 'module-placeholder';
    info.appendChild(addText(document.createElement('p'), 'Zeige 20 von ' + String(channels.length) + ' Kanälen.'));
    list.appendChild(info);
  }

  detailDataElement.appendChild(list);
};
