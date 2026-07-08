// Phase 60.9b: Active SearchTimer browser module.
// SearchTimer is visible in the frontend module navigation.
// Owns SearchTimer list rendering through the frontend platform registry.
// Live parity capability slots are rendered for VPS, blacklist, filters, preview and write actions.

(function(global) {
  'use strict';

  let searchTimerBrowserContext = Object.freeze({});

  const liveParityCapabilities = Object.freeze([
    'active',
    'vps',
    'blacklist',
    'channel-filter',
    'time-window',
    'weekdays',
    'duplicate-avoidance',
    'preview',
    'create',
    'update',
    'delete'
  ]);

  function firstValue(source, keys, fallback) {
    if (!source || typeof source !== 'object') {
      return fallback;
    }

    for (const key of keys) {
      if (Object.prototype.hasOwnProperty.call(source, key) &&
          source[key] !== undefined &&
          source[key] !== null &&
          source[key] !== '') {
        return source[key];
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

  function addText(element, text) {
    element.textContent = String(text);
    return element;
  }

  function configureContext(context) {
    searchTimerBrowserContext = Object.freeze(Object.assign({}, context || {}));
  }

  function getLiveParityCapabilities() {
    return liveParityCapabilities.slice();
  }

  function capabilityLabel(capability) {
    const labels = {
      active: 'Aktiv/Inaktiv',
      vps: 'VPS/PDC',
      blacklist: 'Blacklist',
      'channel-filter': 'Kanalfilter',
      'time-window': 'Zeitfenster',
      weekdays: 'Wochentage',
      'duplicate-avoidance': 'Duplikate vermeiden',
      preview: 'Vorschau',
      create: 'Erstellen',
      update: 'Ändern',
      delete: 'Löschen'
    };

    return labels[capability] || capability;
  }

  function formatBoolean(value) {
    if (value === true || value === '1' || value === 1 || value === 'yes' || value === 'true') {
      return 'ja';
    }

    if (value === false || value === '0' || value === 0 || value === 'no' || value === 'false') {
      return 'nein';
    }

    return '-';
  }

  function appendMeta(parent, label, value) {
    const row = document.createElement('div');
    row.className = 'list-meta';

    const strong = addText(document.createElement('strong'), label + ': ');
    row.appendChild(strong);
    row.appendChild(document.createTextNode(value === undefined || value === null || value === '' ? '-' : String(value)));

    parent.appendChild(row);
  }

  function renderCapabilitySlots(parent) {
    const panel = document.createElement('article');
    panel.className = 'module-placeholder searchtimer-parity-panel';

    panel.appendChild(addText(document.createElement('h3'), 'Live-Parität · vorbereitete SearchTimer-Felder'));
    panel.appendChild(addText(
      document.createElement('p'),
      'Diese UI-Slots sind vorbereitet und werden in den nächsten Schritten mit echten Backend-Feldern und Schreibaktionen verbunden.'
    ));

    const list = document.createElement('div');
    list.className = 'list';

    liveParityCapabilities.forEach(capability => {
      const item = document.createElement('div');
      item.className = 'list-meta searchtimer-capability-slot';
      item.dataset.searchtimerCapability = capability;
      item.appendChild(addText(document.createElement('span'), capabilityLabel(capability)));
      list.appendChild(item);
    });

    panel.appendChild(list);
    parent.appendChild(panel);
  }

  function searchTimerTitle(searchTimer, index) {
    return firstValue(
      searchTimer,
      ['title', 'name', 'search', 'pattern', 'expression', 'query', 'id', 'timerId'],
      'SearchTimer ' + String(index + 1)
    );
  }

  function renderSearchTimerCard(searchTimer, index) {
    const card = document.createElement('article');
    card.className = 'module-placeholder searchtimer-card';

    card.appendChild(addText(document.createElement('h3'), searchTimerTitle(searchTimer, index)));

    appendMeta(card, 'Status', formatBoolean(firstValue(searchTimer, ['active', 'enabled', 'isActive'], '')));
    appendMeta(card, 'VPS/PDC', formatBoolean(firstValue(searchTimer, ['vps', 'useVps', 'vpsEnabled', 'pdc'], '')));
    appendMeta(card, 'Blacklist', firstValue(searchTimer, ['blacklist', 'blacklists', 'blacklistName', 'exclude'], '-'));
    appendMeta(card, 'Kanalfilter', firstValue(searchTimer, ['channel', 'channelId', 'channelName', 'channelFilter', 'channels'], '-'));
    appendMeta(card, 'Zeitfenster', firstValue(searchTimer, ['timeWindow', 'timeRange', 'startTime', 'start'], '-'));
    appendMeta(card, 'Wochentage', firstValue(searchTimer, ['weekdays', 'days'], '-'));
    appendMeta(card, 'Duplikate', formatBoolean(firstValue(searchTimer, ['avoidRepeats', 'avoidDuplicates', 'skipRepeats'], '')));

    return card;
  }

  function renderList(data) {
    const mountTarget = searchTimerBrowserContext.detailDataElement;

    if (!mountTarget) {
      throw new Error('SearchTimer browser mount target is not configured');
    }

    const searchTimers = listFromResponse(data, 'searchTimers');

    mountTarget.replaceChildren();

    const list = document.createElement('section');
    list.className = 'list searchtimer-module';

    const header = document.createElement('article');
    header.className = 'module-placeholder searchtimer-summary';
    header.appendChild(addText(document.createElement('h3'), 'SearchTimer'));
    header.appendChild(addText(
      document.createElement('p'),
      String(searchTimers.length) + ' SearchTimer geladen. Rendering erfolgt über web/frontend/modules/searchtimers.js.'
    ));
    list.appendChild(header);

    renderCapabilitySlots(list);

    if (searchTimers.length === 0) {
      const empty = document.createElement('article');
      empty.className = 'module-placeholder';
      empty.appendChild(addText(document.createElement('h3'), 'Keine SearchTimer'));
      empty.appendChild(addText(document.createElement('p'), 'Der Backend-Endpunkt hat keine SearchTimer geliefert.'));
      list.appendChild(empty);
    } else {
      searchTimers.forEach((searchTimer, index) => {
        list.appendChild(renderSearchTimerCard(searchTimer, index));
      });
    }

    mountTarget.appendChild(list);
  }

  const searchTimerBrowserApi = Object.freeze({
    configureContext: configureContext,
    getLiveParityCapabilities: getLiveParityCapabilities,
    renderList: renderList
  });

  global.VdrSuiteSearchTimerBrowser = searchTimerBrowserApi;

  if (global.VdrSuitePlatform &&
      typeof global.VdrSuitePlatform.registerModule === 'function' &&
      typeof global.VdrSuitePlatform.hasModule === 'function' &&
      !global.VdrSuitePlatform.hasModule('searchtimers')) {
    global.VdrSuitePlatform.registerModule('searchtimers', searchTimerBrowserApi);
  }
})(window);
