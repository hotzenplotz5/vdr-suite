// Phase 60.10a: Active SearchTimer browser module with editor shell.
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

  function formatSearchTimerValue(value) {
    if (Array.isArray(value)) {
      return value.length === 0 ? '-' : value.join(', ');
    }

    if (value && typeof value === 'object') {
      return JSON.stringify(value);
    }

    return value === undefined || value === null || value === '' ? '-' : String(value);
  }

  function appendMeta(parent, label, value) {
    const row = document.createElement('div');
    row.className = 'list-meta';

    const strong = addText(document.createElement('strong'), label + ': ');
    row.appendChild(strong);
    row.appendChild(document.createTextNode(formatSearchTimerValue(value)));

    parent.appendChild(row);
  }

  function appendFieldGroup(parent, title, fields) {
    const group = document.createElement('section');
    group.className = 'searchtimer-field-group';

    group.appendChild(addText(document.createElement('h4'), title));

    fields.forEach(field => {
      appendMeta(parent === group ? group : group, field.label, field.value);
    });

    parent.appendChild(group);
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

  function appendEditorField(parent, label, input) {
    const row = document.createElement('label');
    row.className = 'searchtimer-editor-field';

    const caption = addText(document.createElement('span'), label);
    row.appendChild(caption);
    row.appendChild(input);
    parent.appendChild(row);
  }

  function textInput(name, placeholder) {
    const input = document.createElement('input');
    input.type = 'text';
    input.name = name;
    input.placeholder = placeholder || '';
    input.disabled = true;
    return input;
  }

  function checkboxInput(name) {
    const input = document.createElement('input');
    input.type = 'checkbox';
    input.name = name;
    input.disabled = true;
    return input;
  }

  function numberInput(name, placeholder) {
    const input = document.createElement('input');
    input.type = 'number';
    input.name = name;
    input.placeholder = placeholder || '';
    input.disabled = true;
    return input;
  }

  function renderSearchTimerEditorShell(parent) {
    const editor = document.createElement('aside');
    editor.className = 'module-placeholder searchtimer-editor-shell';
    editor.dataset.searchtimerEditor = 'create';

    const heading = document.createElement('div');
    heading.className = 'searchtimer-editor-heading';
    heading.appendChild(addText(document.createElement('h3'), 'Neuer SearchTimer'));

    const notice = addText(
      document.createElement('p'),
      'Editor-Schale vorbereitet. Vorschau, Validierung und Speichern werden in den nächsten Schritten an die vorhandenen Client-API-Aktionen angebunden.'
    );
    heading.appendChild(notice);
    editor.appendChild(heading);

    const form = document.createElement('form');
    form.className = 'searchtimer-editor-form';
    form.dataset.searchtimerEditorForm = 'create';

    appendEditorField(form, 'Suchbegriff', textInput('search', 'z. B. Tatort'));
    appendEditorField(form, 'Aktiv', checkboxInput('use_as_searchtimer'));
    appendEditorField(form, 'VPS/PDC', checkboxInput('use_vps'));
    appendEditorField(form, 'Verzeichnis', textInput('directory', 'optional'));
    appendEditorField(form, 'Priorität', numberInput('priority', '50'));
    appendEditorField(form, 'Lebensdauer', numberInput('lifetime', '99'));
    appendEditorField(form, 'Kanalfilter', textInput('channels', 'optional'));
    appendEditorField(form, 'Zeitfenster Start', textInput('start_time', 'HHMM'));
    appendEditorField(form, 'Zeitfenster Stop', textInput('stop_time', 'HHMM'));
    appendEditorField(form, 'Blacklist-IDs', textInput('blacklist_ids', 'optional'));

    const actions = document.createElement('div');
    actions.className = 'searchtimer-editor-actions';

    const previewButton = addText(document.createElement('button'), 'Vorschau');
    previewButton.type = 'button';
    previewButton.disabled = true;
    previewButton.dataset.searchtimerAction = 'preview';

    const saveButton = addText(document.createElement('button'), 'Speichern');
    saveButton.type = 'button';
    saveButton.disabled = true;
    saveButton.dataset.searchtimerAction = 'save';

    const cancelButton = addText(document.createElement('button'), 'Abbrechen');
    cancelButton.type = 'button';
    cancelButton.disabled = true;
    cancelButton.dataset.searchtimerAction = 'cancel';

    actions.appendChild(previewButton);
    actions.appendChild(saveButton);
    actions.appendChild(cancelButton);

    form.appendChild(actions);
    editor.appendChild(form);

    parent.appendChild(editor);
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

    appendFieldGroup(card, 'Basis', [
      { label: 'Status', value: formatBoolean(firstValue(searchTimer, ['active', 'enabled', 'isActive', 'use_as_searchtimer'], '')) },
      { label: 'Suche', value: firstValue(searchTimer, ['search', 'query', 'name', 'title'], '-') },
      { label: 'Backend-ID', value: firstValue(searchTimer, ['backendId', 'backend'], '-') },
      { label: 'Native ID', value: firstValue(searchTimer, ['backendNativeId', 'id', 'timerId'], '-') }
    ]);

    appendFieldGroup(card, 'Aufnahmeoptionen', [
      { label: 'Verzeichnis', value: firstValue(searchTimer, ['directory', 'folder'], '-') },
      { label: 'Priorität', value: firstValue(searchTimer, ['priority'], '-') },
      { label: 'Lebensdauer', value: firstValue(searchTimer, ['lifetime'], '-') },
      { label: 'Start-Marge', value: firstValue(searchTimer, ['margin_start', 'marginStart', 'startMargin'], '-') },
      { label: 'Stop-Marge', value: firstValue(searchTimer, ['margin_stop', 'marginStop', 'stopMargin'], '-') },
      { label: 'VPS/PDC', value: formatBoolean(firstValue(searchTimer, ['use_vps', 'vps', 'useVps', 'vpsEnabled', 'pdc'], '')) }
    ]);

    appendFieldGroup(card, 'Kanal-, Zeit- und Dauerfilter', [
      { label: 'Kanalfilter aktiv', value: formatBoolean(firstValue(searchTimer, ['use_channel', 'useChannel'], '')) },
      { label: 'Kanäle', value: firstValue(searchTimer, ['channels', 'channel', 'channelId', 'channelName', 'channelFilter'], '-') },
      { label: 'Kanal min/max', value: String(firstValue(searchTimer, ['channel_min', 'channelMin'], '-')) + ' / ' + String(firstValue(searchTimer, ['channel_max', 'channelMax'], '-')) },
      { label: 'Zeitfilter aktiv', value: formatBoolean(firstValue(searchTimer, ['use_time', 'useTime'], '')) },
      { label: 'Start/Stop', value: String(firstValue(searchTimer, ['start_time', 'startTime', 'start'], '-')) + ' – ' + String(firstValue(searchTimer, ['stop_time', 'stopTime', 'stop'], '-')) },
      { label: 'Wochentage', value: firstValue(searchTimer, ['dayofweek', 'weekdays', 'days'], '-') },
      { label: 'Dauer min/max', value: String(firstValue(searchTimer, ['duration_min', 'durationMin'], '-')) + ' / ' + String(firstValue(searchTimer, ['duration_max', 'durationMax'], '-')) }
    ]);

    appendFieldGroup(card, 'Wiederholungen, Serien und Blacklist', [
      { label: 'Duplikate vermeiden', value: formatBoolean(firstValue(searchTimer, ['avoid_repeats', 'avoidRepeats', 'avoidDuplicates', 'skipRepeats'], '')) },
      { label: 'Erlaubte Wiederholungen', value: firstValue(searchTimer, ['allowed_repeats', 'allowedRepeats'], '-') },
      { label: 'Wiederholungen innerhalb Tage', value: firstValue(searchTimer, ['repeats_within_days', 'repeatsWithinDays'], '-') },
      { label: 'Serienaufnahme', value: formatBoolean(firstValue(searchTimer, ['use_series_recording', 'useSeriesRecording'], '')) },
      { label: 'Aufnahmen behalten', value: firstValue(searchTimer, ['keep_recs', 'keepRecs'], '-') },
      { label: 'Blacklist-Modus', value: firstValue(searchTimer, ['blacklist_mode', 'blacklistMode'], '-') },
      { label: 'Blacklist-IDs', value: firstValue(searchTimer, ['blacklist_ids', 'blacklistIds', 'blacklists', 'blacklist'], '-') }
    ]);

    appendFieldGroup(card, 'Weitere Live-Paritätsfelder', [
      { label: 'Suchmodus', value: firstValue(searchTimer, ['mode'], '-') },
      { label: 'Groß/Kleinschreibung', value: formatBoolean(firstValue(searchTimer, ['match_case', 'matchCase'], '')) },
      { label: 'Toleranz', value: firstValue(searchTimer, ['tolerance'], '-') },
      { label: 'Summary-Match', value: firstValue(searchTimer, ['summary_match', 'summaryMatch'], '-') },
      { label: 'Extended EPG', value: firstValue(searchTimer, ['use_ext_epg_info', 'ext_epg_info', 'content_descriptors'], '-') },
      { label: 'Favoriten', value: formatBoolean(firstValue(searchTimer, ['use_in_favorites', 'useInFavorites'], '')) },
      { label: 'Gültig von/bis', value: String(firstValue(searchTimer, ['use_as_searchtimer_from', 'validFrom'], '-')) + ' – ' + String(firstValue(searchTimer, ['use_as_searchtimer_til', 'validUntil'], '-')) },
      { label: 'Cleanup', value: String(firstValue(searchTimer, ['del_recs_after_days', 'deleteRecordingsAfterDays'], '-')) + ' / ' + String(firstValue(searchTimer, ['del_after_count_recs', 'deleteAfterCountRecordings'], '-')) }
    ]);

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
    renderSearchTimerEditorShell(list);

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
