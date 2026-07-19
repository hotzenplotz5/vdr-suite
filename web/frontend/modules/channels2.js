(function (global) {
  'use strict';

  const state = {
    active: false,
    channels: [],
    filteredChannels: [],
    selectedChannel: null,
    selectedDate: new Date(),
    events: [],
    selectedEvent: null,
    loadSequence: 0,
    filter: ''
  };

  function platform() {
    return global.VdrSuitePlatform || null;
  }

  function clientApi() {
    const runtime = platform();
    if (runtime && typeof runtime.getClientApi === 'function') {
      const api = runtime.getClientApi();
      if (api) return api;
    }
    return global.VdrSuiteClientApi || null;
  }

  function mountTarget() {
    const runtime = platform();
    if (runtime && typeof runtime.getMountTarget === 'function') {
      const target = runtime.getMountTarget('channels');
      if (target) return target;
    }
    return document.getElementById('detail-data');
  }

  function selectedBackendId() {
    const runtime = platform();
    if (runtime && typeof runtime.getSelectedBackendId === 'function') {
      const value = String(runtime.getSelectedBackendId() || '').trim();
      if (value !== '') return value;
    }
    return 'default';
  }

  function firstValue(source, keys, fallback) {
    if (!source || typeof source !== 'object') return fallback;
    for (const key of keys) {
      if (source[key] !== undefined && source[key] !== null && source[key] !== '') {
        return source[key];
      }
    }
    return fallback;
  }

  function listFromResponse(data, key) {
    if (Array.isArray(data)) return data;
    if (data && Array.isArray(data[key])) return data[key];
    if (data && Array.isArray(data.items)) return data.items;
    if (data && Array.isArray(data.results)) return data.results;
    return [];
  }

  function text(value) {
    return String(value === undefined || value === null ? '' : value).trim();
  }

  function parseEpoch(value) {
    if (value === undefined || value === null || value === '') return 0;
    const numeric = Number(value);
    if (Number.isFinite(numeric) && numeric > 0) {
      return numeric > 100000000000 ? Math.floor(numeric / 1000) : Math.floor(numeric);
    }
    const parsed = Date.parse(String(value));
    return Number.isFinite(parsed) ? Math.floor(parsed / 1000) : 0;
  }

  function channelId(channel) {
    return text(firstValue(channel, ['id', 'channelId', 'nativeId'], ''));
  }

  function channelName(channel) {
    return text(firstValue(channel, ['name', 'channelName', 'title', 'displayName'], 'Kanal'));
  }

  function channelNumber(channel) {
    return Number(firstValue(channel, ['number', 'channelNumber', 'position'], 0)) || 0;
  }

  function channelGroup(channel) {
    return text(firstValue(channel, ['group', 'groupName', 'channelGroup', 'bouquet', 'provider'], ''));
  }

  function normalizeChannel(channel) {
    return Object.assign({}, channel || {}, {
      id: channelId(channel),
      name: channelName(channel),
      number: channelNumber(channel),
      group: channelGroup(channel)
    });
  }

  function eventId(event) {
    return text(firstValue(event, ['eventId', 'id', 'nativeId'], ''));
  }

  function eventChannelId(event) {
    return text(firstValue(event, ['channelId', 'channel', 'channel_id'], ''));
  }

  function eventTitle(event) {
    return text(firstValue(event, ['title', 'name', 'eventTitle'], 'Sendung'));
  }

  function eventSubtitle(event) {
    return text(firstValue(event, ['subtitle', 'shortText', 'short_text'], ''));
  }

  function eventDescription(event) {
    return text(firstValue(event, ['description', 'longText', 'long_text', 'summary'], ''));
  }

  function eventStart(event) {
    return parseEpoch(firstValue(event, ['startTime', 'start', 'beginTime'], 0));
  }

  function eventEnd(event) {
    const start = eventStart(event);
    const explicit = parseEpoch(firstValue(event, ['endTime', 'end', 'stopTime'], 0));
    if (explicit > start) return explicit;
    const duration = Number(firstValue(event, ['durationSeconds', 'duration'], 0));
    return Number.isFinite(duration) && duration > 0 ? start + duration : start;
  }

  function dateOnly(value) {
    const source = value instanceof Date ? value : new Date(value || Date.now());
    return new Date(source.getFullYear(), source.getMonth(), source.getDate());
  }

  function addDays(value, amount) {
    const result = dateOnly(value);
    result.setDate(result.getDate() + amount);
    return result;
  }

  function dayBounds(value) {
    const startDate = dateOnly(value);
    const endDate = addDays(startDate, 1);
    return {
      start: Math.floor(startDate.getTime() / 1000),
      end: Math.floor(endDate.getTime() / 1000)
    };
  }

  function dateInputValue(value) {
    const date = dateOnly(value);
    return String(date.getFullYear()).padStart(4, '0') + '-' +
      String(date.getMonth() + 1).padStart(2, '0') + '-' +
      String(date.getDate()).padStart(2, '0');
  }

  function formatDate(value) {
    return dateOnly(value).toLocaleDateString('de-DE', {
      weekday: 'long', day: '2-digit', month: 'long', year: 'numeric'
    });
  }

  function formatClock(epoch) {
    return new Date(Number(epoch) * 1000).toLocaleTimeString('de-DE', {
      hour: '2-digit', minute: '2-digit'
    });
  }

  function hhmm(epoch) {
    const date = new Date(Number(epoch) * 1000);
    return date.getHours() * 100 + date.getMinutes();
  }

  function timerDay(epoch) {
    return dateInputValue(new Date(Number(epoch) * 1000));
  }

  function addText(element, value) {
    element.textContent = String(value);
    return element;
  }

  function installStyles() {
    if (document.getElementById('vdr-suite-channels2-style')) return;
    const style = document.createElement('style');
    style.id = 'vdr-suite-channels2-style';
    style.textContent = `
.channels2{display:grid;gap:.9rem;min-width:0}.channels2-toolbar{display:flex;flex-wrap:wrap;gap:.55rem;align-items:center;justify-content:space-between}.channels2-toolbar-group{display:flex;flex-wrap:wrap;gap:.45rem;align-items:center}.channels2-search{min-width:min(22rem,100%);padding:.62rem .72rem;border:1px solid #475569;border-radius:.72rem;background:#0f172a;color:#f8fafc;font:inherit}.channels2-status{padding:.7rem .8rem;border:1px solid rgba(148,163,184,.25);border-radius:.75rem;background:rgba(15,23,42,.72);color:#cbd5e1}.channels2-status.error{border-color:rgba(248,113,113,.5);color:#fecaca}.channels2-layout{display:grid;grid-template-columns:minmax(16rem,23rem) minmax(0,1fr);gap:.85rem;align-items:start}.channels2-channel-panel,.channels2-program-panel{display:grid;gap:.65rem;min-width:0}.channels2-channel-list{display:grid;gap:.42rem;max-height:68vh;overflow:auto;padding-right:.2rem}.channels2-channel{display:grid;grid-template-columns:3.6rem minmax(0,1fr);gap:.65rem;align-items:center;width:100%;padding:.62rem;border:1px solid rgba(148,163,184,.2);border-radius:.78rem;background:rgba(15,23,42,.75);color:#f8fafc;text-align:left}.channels2-channel:hover,.channels2-channel:focus,.channels2-channel.active{border-color:rgba(56,189,248,.62);background:rgba(14,165,233,.14);outline:none}.channels2-channel-logo{display:grid;place-items:center;width:3.6rem;height:2.5rem}.channels2-channel-logo img,.channels2-channel-logo .channel-logo{max-width:100%;max-height:100%;object-fit:contain}.channels2-channel-title{font-weight:800;overflow-wrap:anywhere}.channels2-channel-meta{margin-top:.12rem;color:#94a3b8;font-size:.84rem}.channels2-program-head{display:flex;flex-wrap:wrap;gap:.6rem;align-items:center;justify-content:space-between;padding:.72rem .78rem;border:1px solid rgba(56,189,248,.28);border-radius:.82rem;background:rgba(2,6,23,.7)}.channels2-program-head h3,.channels2-program-head p{margin:0}.channels2-date-controls{display:flex;flex-wrap:wrap;gap:.4rem;align-items:center}.channels2-date-controls button,.channels2-date-controls input,.channels2-button{min-height:2.45rem;padding:.5rem .7rem;border-radius:.68rem}.channels2-date-controls input{border:1px solid #475569;background:#0f172a;color:#f8fafc;font:inherit}.channels2-event-list{display:grid;gap:.48rem}.channels2-event{display:grid;grid-template-columns:6.2rem minmax(0,1fr) auto;gap:.65rem;align-items:center;width:100%;padding:.7rem .75rem;border:1px solid rgba(148,163,184,.2);border-radius:.8rem;background:rgba(15,23,42,.75);color:#f8fafc;text-align:left}.channels2-event:hover,.channels2-event:focus,.channels2-event.active{border-color:rgba(56,189,248,.62);background:rgba(14,165,233,.12);outline:none}.channels2-event.current{border-color:rgba(74,222,128,.5)}.channels2-event-time{font-weight:850;color:#bae6fd}.channels2-event-title{font-weight:850;overflow-wrap:anywhere}.channels2-event-subtitle{margin-top:.12rem;color:#94a3b8;font-size:.84rem}.channels2-badge{padding:.2rem .42rem;border:1px solid rgba(148,163,184,.28);border-radius:999px;color:#cbd5e1;font-size:.72rem;font-weight:800}.channels2-detail{display:grid;gap:.68rem;padding:.82rem;border:1px solid rgba(56,189,248,.34);border-radius:.86rem;background:rgba(2,6,23,.78)}.channels2-detail h3,.channels2-detail p{margin:0}.channels2-detail-meta{display:flex;flex-wrap:wrap;gap:.38rem}.channels2-actions{display:flex;flex-wrap:wrap;gap:.48rem}.channels2-actions button{min-height:2.55rem;padding:.55rem .8rem;border-radius:.68rem}.channels2-feedback{min-height:1.35rem;color:#cbd5e1}.channels2-feedback.error{color:#fecaca}.channels2-empty{padding:1rem;border:1px dashed rgba(148,163,184,.3);border-radius:.8rem;color:#94a3b8;text-align:center}
@media(max-width:900px){.channels2-layout{grid-template-columns:1fr}.channels2-channel-list{max-height:42vh}.channels2-event{grid-template-columns:5rem minmax(0,1fr)}.channels2-event .channels2-badge{grid-column:1/-1;justify-self:start}}`;
    document.head.appendChild(style);
  }

  function setStatus(message, error) {
    const target = mountTarget();
    if (!target) return;
    let status = target.querySelector('.channels2-status');
    if (!status) {
      status = document.createElement('p');
      status.className = 'channels2-status';
      target.prepend(status);
    }
    status.className = 'channels2-status' + (error ? ' error' : '');
    status.textContent = String(message || '');
  }

  function buildTimerPayload(event, channel) {
    const start = eventStart(event);
    const end = eventEnd(event);
    const id = eventId(event);
    return {
      backendId: selectedBackendId(),
      channelId: eventChannelId(event) || channelId(channel),
      title: eventTitle(event),
      directory: '',
      day: timerDay(start),
      weekdays: '-------',
      start: hhmm(start),
      stop: hhmm(end),
      priority: 50,
      lifetime: 99,
      active: true,
      vps: false,
      aux: id !== '' ? 'eventId=' + id : ''
    };
  }

  function buildSeriesPayload(event, channel) {
    const title = eventTitle(event);
    const id = eventChannelId(event) || channelId(channel);
    return {
      backendId: selectedBackendId(), name: title, query: title, active: true,
      directory: '', priority: 50, lifetime: 99, marginStartMinutes: 5,
      marginStopMinutes: 10, useVps: false, useChannel: id === '' ? 0 : 1,
      channels: '', channelMin: id, channelMax: id, useTime: false,
      startTime: 0, stopTime: 0, useDuration: false, durationMinMinutes: 0,
      durationMaxMinutes: 0, useDayOfWeek: false, dayOfWeek: 0,
      avoidRepeats: true, allowedRepeats: 0, repeatsWithinDays: 0,
      compareTitle: true, compareSubtitle: false, compareSummary: false,
      compareCategories: false, compareTime: false, useSeriesRecording: false,
      keepRecordings: 0, deleteMode: 0, searchTimerAction: 0, blacklistMode: 0,
      blacklistIds: '', mode: 0, matchCase: false, tolerance: 0,
      summaryMatch: 0, useExtendedEpgInfo: false, extendedEpgInfo: '',
      ignoreMissingEpgCategories: false, contentDescriptors: '', useInFavorites: false,
      activeFrom: '', activeUntil: '', pauseOnRecordings: false,
      switchMinutesBefore: 0, unmuteSoundOnSwitch: false,
      deleteRecordingsAfterDays: 0, deleteAfterCountRecordings: 0,
      deleteAfterDaysOfFirstRecording: 0
    };
  }

  function createTimer(event, channel, feedback, button) {
    const api = clientApi();
    if (!api || typeof api.fetchClientTimerCreateAction !== 'function') {
      feedback.className = 'channels2-feedback error';
      feedback.textContent = 'Timer-API ist nicht verfügbar.';
      return;
    }
    button.disabled = true;
    feedback.className = 'channels2-feedback';
    feedback.textContent = 'Timer wird erstellt …';
    api.fetchClientTimerCreateAction({payload: buildTimerPayload(event, channel), cache: 'no-store', credentials: 'same-origin'})
      .then(function (data) {
        if (data && data.success === false) throw new Error(data.message || data.error || 'Timer wurde abgelehnt.');
        button.textContent = 'Timer erstellt';
        feedback.textContent = 'Timer für „' + eventTitle(event) + '“ wurde erstellt.';
      })
      .catch(function (error) {
        button.disabled = false;
        feedback.className = 'channels2-feedback error';
        feedback.textContent = 'Timer konnte nicht erstellt werden: ' + error.message;
      });
  }

  function createSeriesTimer(event, channel, feedback, button) {
    const api = clientApi();
    if (!api || typeof api.fetchClientSearchTimerCreateAction !== 'function') {
      feedback.className = 'channels2-feedback error';
      feedback.textContent = 'SearchTimer-API ist nicht verfügbar.';
      return;
    }
    button.disabled = true;
    feedback.className = 'channels2-feedback';
    feedback.textContent = 'Serientimer wird erstellt …';
    api.fetchClientSearchTimerCreateAction({payload: buildSeriesPayload(event, channel), cache: 'no-store', credentials: 'same-origin'})
      .then(function (data) {
        if (data && data.success === false) throw new Error(data.message || data.error || 'SearchTimer wurde abgelehnt.');
        button.textContent = 'Serientimer erstellt';
        feedback.textContent = 'Serientimer für „' + eventTitle(event) + '“ wurde erstellt.';
      })
      .catch(function (error) {
        button.disabled = false;
        feedback.className = 'channels2-feedback error';
        feedback.textContent = 'Serientimer konnte nicht erstellt werden: ' + error.message;
      });
  }

  function openAdvancedSearchTimer(event, channel) {
    const payload = buildSeriesPayload(event, channel);
    const tab = document.querySelector('[data-module="searchtimers"]');
    if (!tab || typeof tab.click !== 'function') return;
    tab.click();
    let attempts = 0;
    const fill = function () {
      attempts += 1;
      const form = document.querySelector('form[data-searchtimer-editor-form="create"]');
      if (!form) {
        if (attempts < 30) global.setTimeout(fill, 100);
        return;
      }
      function setValue(name, value, eventName) {
        const element = form.elements[name];
        if (!element) return;
        if (element.type === 'checkbox') element.checked = Boolean(value);
        else element.value = String(value === undefined || value === null ? '' : value);
        if (eventName) element.dispatchEvent(new Event(eventName, {bubbles: true}));
      }
      setValue('name', payload.name);
      setValue('query', payload.query);
      setValue('active', true);
      setValue('compareTitle', true);
      setValue('avoidRepeats', true);
      setValue('channelFilterMode', payload.useChannel, 'change');
      global.setTimeout(function () {
        setValue('channelSelectorGroup', channelGroup(channel), 'change');
        global.setTimeout(function () { setValue('channelId', channelId(channel), 'change'); }, 80);
        form.scrollIntoView({behavior: 'smooth', block: 'start'});
      }, 80);
    };
    global.setTimeout(fill, 80);
  }

  function renderEventDetail(container) {
    container.replaceChildren();
    const event = state.selectedEvent;
    const channel = state.selectedChannel;
    if (!event || !channel) return;
    const detail = document.createElement('article');
    detail.className = 'channels2-detail';
    detail.appendChild(addText(document.createElement('h3'), eventTitle(event)));
    const subtitle = eventSubtitle(event);
    if (subtitle !== '') detail.appendChild(addText(document.createElement('p'), subtitle));
    const meta = document.createElement('div');
    meta.className = 'channels2-detail-meta';
    meta.appendChild(addText(document.createElement('span'), formatClock(eventStart(event)) + '–' + formatClock(eventEnd(event))));
    meta.lastChild.className = 'channels2-badge';
    meta.appendChild(addText(document.createElement('span'), channelName(channel)));
    meta.lastChild.className = 'channels2-badge';
    detail.appendChild(meta);
    detail.appendChild(addText(document.createElement('p'), eventDescription(event) || 'Keine Beschreibung vorhanden.'));
    const actions = document.createElement('div');
    actions.className = 'channels2-actions';
    const timer = addText(document.createElement('button'), 'Timer erstellen');
    timer.type = 'button';
    const series = addText(document.createElement('button'), 'Serientimer erstellen');
    series.type = 'button';
    const advanced = addText(document.createElement('button'), 'Erweiterter SearchTimer');
    advanced.type = 'button';
    const feedback = document.createElement('p');
    feedback.className = 'channels2-feedback';
    feedback.setAttribute('role', 'status');
    feedback.setAttribute('aria-live', 'polite');
    timer.addEventListener('click', function () { createTimer(event, channel, feedback, timer); });
    series.addEventListener('click', function () { createSeriesTimer(event, channel, feedback, series); });
    advanced.addEventListener('click', function () { openAdvancedSearchTimer(event, channel); });
    actions.appendChild(timer);
    actions.appendChild(series);
    actions.appendChild(advanced);
    detail.appendChild(actions);
    detail.appendChild(feedback);
    container.appendChild(detail);
  }

  function renderProgram(panel) {
    panel.replaceChildren();
    const channel = state.selectedChannel;
    if (!channel) {
      const empty = addText(document.createElement('div'), 'Links einen Kanal auswählen.');
      empty.className = 'channels2-empty';
      panel.appendChild(empty);
      return;
    }
    const head = document.createElement('article');
    head.className = 'channels2-program-head';
    const copy = document.createElement('div');
    copy.appendChild(addText(document.createElement('h3'), channelName(channel)));
    copy.appendChild(addText(document.createElement('p'), 'Kanal ' + String(channelNumber(channel) || '-') + (channelGroup(channel) ? ' · ' + channelGroup(channel) : '')));
    head.appendChild(copy);
    const controls = document.createElement('div');
    controls.className = 'channels2-date-controls';
    const previous = addText(document.createElement('button'), '← Tag');
    previous.type = 'button';
    const today = addText(document.createElement('button'), 'Heute');
    today.type = 'button';
    const next = addText(document.createElement('button'), 'Tag →');
    next.type = 'button';
    const date = document.createElement('input');
    date.type = 'date';
    date.value = dateInputValue(state.selectedDate);
    previous.addEventListener('click', function () { state.selectedDate = addDays(state.selectedDate, -1); loadEvents(); });
    today.addEventListener('click', function () { state.selectedDate = dateOnly(new Date()); loadEvents(); });
    next.addEventListener('click', function () { state.selectedDate = addDays(state.selectedDate, 1); loadEvents(); });
    date.addEventListener('change', function () { state.selectedDate = dateOnly(date.value); loadEvents(); });
    controls.appendChild(previous);
    controls.appendChild(today);
    controls.appendChild(next);
    controls.appendChild(date);
    head.appendChild(controls);
    panel.appendChild(head);
    panel.appendChild(addText(document.createElement('h3'), formatDate(state.selectedDate)));
    const list = document.createElement('section');
    list.className = 'channels2-event-list';
    const now = Math.floor(Date.now() / 1000);
    if (state.events.length === 0) {
      const empty = addText(document.createElement('div'), 'Für diesen Tag wurden keine EPG-Einträge gefunden.');
      empty.className = 'channels2-empty';
      list.appendChild(empty);
    }
    state.events.forEach(function (event) {
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'channels2-event';
      const start = eventStart(event);
      const end = eventEnd(event);
      if (start <= now && end > now) button.classList.add('current');
      if (state.selectedEvent === event) button.classList.add('active');
      const time = addText(document.createElement('span'), formatClock(start) + '–' + formatClock(end));
      time.className = 'channels2-event-time';
      const eventCopy = document.createElement('span');
      const title = addText(document.createElement('span'), eventTitle(event));
      title.className = 'channels2-event-title';
      eventCopy.appendChild(title);
      const subtitle = eventSubtitle(event);
      if (subtitle) {
        const sub = addText(document.createElement('span'), subtitle);
        sub.className = 'channels2-event-subtitle';
        eventCopy.appendChild(sub);
      }
      const badge = addText(document.createElement('span'), start <= now && end > now ? 'Läuft jetzt' : 'EPG');
      badge.className = 'channels2-badge';
      button.appendChild(time);
      button.appendChild(eventCopy);
      button.appendChild(badge);
      button.addEventListener('click', function () {
        state.selectedEvent = event;
        renderProgram(panel);
      });
      list.appendChild(button);
    });
    panel.appendChild(list);
    const detail = document.createElement('section');
    renderEventDetail(detail);
    panel.appendChild(detail);
  }

  function applyFilter() {
    const query = state.filter.toLocaleLowerCase('de-DE');
    state.filteredChannels = state.channels.filter(function (channel) {
      return query === '' || channelName(channel).toLocaleLowerCase('de-DE').includes(query) ||
        channelGroup(channel).toLocaleLowerCase('de-DE').includes(query) || String(channelNumber(channel)).includes(query);
    });
  }

  function renderChannelList(list, program) {
    list.replaceChildren();
    state.filteredChannels.forEach(function (channel) {
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'channels2-channel';
      if (state.selectedChannel && channelId(state.selectedChannel) === channelId(channel)) button.classList.add('active');
      const logoWrap = document.createElement('span');
      logoWrap.className = 'channels2-channel-logo';
      if (typeof global.createChannelLogoElement === 'function') logoWrap.appendChild(global.createChannelLogoElement(channelName(channel), channelId(channel)));
      else logoWrap.textContent = String(channelNumber(channel) || '•');
      const copy = document.createElement('span');
      const title = addText(document.createElement('span'), channelName(channel));
      title.className = 'channels2-channel-title';
      const meta = addText(document.createElement('span'), 'Nr. ' + String(channelNumber(channel) || '-') + (channelGroup(channel) ? ' · ' + channelGroup(channel) : ''));
      meta.className = 'channels2-channel-meta';
      copy.appendChild(title);
      copy.appendChild(meta);
      button.appendChild(logoWrap);
      button.appendChild(copy);
      button.addEventListener('click', function () {
        state.selectedChannel = channel;
        state.selectedEvent = null;
        renderChannelList(list, program);
        loadEvents();
      });
      list.appendChild(button);
    });
  }

  function renderShell() {
    installStyles();
    const target = mountTarget();
    if (!target) return;
    target.replaceChildren();
    const root = document.createElement('section');
    root.className = 'channels2';
    root.dataset.channels2Root = 'true';
    const toolbar = document.createElement('div');
    toolbar.className = 'channels2-toolbar';
    const intro = document.createElement('div');
    intro.appendChild(addText(document.createElement('h3'), 'Channels 2'));
    intro.appendChild(addText(document.createElement('p'), 'Neue unabhängige Kanal- und Tagesprogramm-Ansicht.'));
    const tools = document.createElement('div');
    tools.className = 'channels2-toolbar-group';
    const search = document.createElement('input');
    search.type = 'search';
    search.className = 'channels2-search';
    search.placeholder = 'Kanal, Nummer oder Gruppe suchen';
    search.value = state.filter;
    const refresh = addText(document.createElement('button'), 'Neu laden');
    refresh.type = 'button';
    refresh.className = 'channels2-button';
    tools.appendChild(search);
    tools.appendChild(refresh);
    toolbar.appendChild(intro);
    toolbar.appendChild(tools);
    root.appendChild(toolbar);
    const status = document.createElement('p');
    status.className = 'channels2-status';
    status.textContent = String(state.channels.length) + ' Kanäle geladen.';
    root.appendChild(status);
    const layout = document.createElement('div');
    layout.className = 'channels2-layout';
    const channelPanel = document.createElement('section');
    channelPanel.className = 'channels2-channel-panel';
    const channelList = document.createElement('div');
    channelList.className = 'channels2-channel-list';
    channelPanel.appendChild(channelList);
    const program = document.createElement('section');
    program.className = 'channels2-program-panel';
    layout.appendChild(channelPanel);
    layout.appendChild(program);
    root.appendChild(layout);
    target.appendChild(root);
    search.addEventListener('input', function () {
      state.filter = search.value.trim();
      applyFilter();
      renderChannelList(channelList, program);
      status.textContent = String(state.filteredChannels.length) + ' von ' + String(state.channels.length) + ' Kanälen sichtbar.';
    });
    refresh.addEventListener('click', loadChannels);
    renderChannelList(channelList, program);
    renderProgram(program);
  }

  function loadEvents() {
    const channel = state.selectedChannel;
    if (!channel) { renderShell(); return; }
    const api = clientApi();
    if (!api) { setStatus('Client API ist nicht verfügbar.', true); return; }
    const bounds = dayBounds(state.selectedDate);
    const sequence = ++state.loadSequence;
    setStatus('Lade Tagesprogramm für ' + channelName(channel) + ' …', false);
    const cached = typeof api.fetchClientEpgCacheWindow === 'function'
      ? api.fetchClientEpgCacheWindow({query: {backend: selectedBackendId(), channelId: channelId(channel), fromTime: String(bounds.start), untilTime: String(bounds.end), limit: '0', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'}).catch(function () { return {events: []}; })
      : Promise.resolve({events: []});
    cached.then(function (data) {
      if (listFromResponse(data, 'events').length > 0 || typeof api.fetchClientEpgCacheRefresh !== 'function') return data;
      return api.fetchClientEpgCacheRefresh({query: {backend: selectedBackendId(), channelId: channelId(channel), from: String(bounds.start), timespan: String(bounds.end - bounds.start), limit: '0', channelEventLimit: '192', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'})
        .catch(function () { return null; })
        .then(function () {
          return api.fetchClientEpgCacheWindow({query: {backend: selectedBackendId(), channelId: channelId(channel), fromTime: String(bounds.start), untilTime: String(bounds.end), limit: '0', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'}).catch(function () { return {events: []}; });
        });
    }).then(function (data) {
      const events = listFromResponse(data, 'events');
      if (events.length > 0 || typeof api.fetchClientEpgChannelWindow !== 'function') return {events: events};
      return api.fetchClientEpgChannelWindow({query: {channelId: channelId(channel), from: String(bounds.start), timespan: String(bounds.end - bounds.start), limit: '192', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'});
    }).then(function (data) {
      if (sequence !== state.loadSequence || !state.active) return;
      state.events = listFromResponse(data, 'events').filter(function (event) {
        const id = eventChannelId(event);
        const start = eventStart(event);
        const end = eventEnd(event);
        return (id === '' || id === channelId(channel)) && start < bounds.end && end > bounds.start;
      }).sort(function (left, right) { return eventStart(left) - eventStart(right); });
      state.selectedEvent = null;
      renderShell();
    }).catch(function (error) {
      if (sequence !== state.loadSequence || !state.active) return;
      setStatus('Tagesprogramm konnte nicht geladen werden: ' + error.message, true);
    });
  }

  function loadChannels() {
    const api = clientApi();
    if (!api || typeof api.fetchClientChannels !== 'function') {
      const target = mountTarget();
      if (target) {
        target.replaceChildren();
        const error = addText(document.createElement('p'), 'Kanäle konnten nicht geladen werden: Client API ist nicht verfügbar.');
        error.className = 'channels2-status error';
        target.appendChild(error);
      }
      return;
    }
    const target = mountTarget();
    if (target) {
      target.replaceChildren();
      const loading = addText(document.createElement('p'), 'Lade Kanäle für Channels 2 …');
      loading.className = 'channels2-status';
      target.appendChild(loading);
    }
    api.fetchClientChannels({query: {backend: selectedBackendId(), _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'})
      .then(function (data) {
        if (!state.active) return;
        state.channels = listFromResponse(data, 'channels').map(normalizeChannel).sort(function (left, right) {
          const diff = channelNumber(left) - channelNumber(right);
          return diff !== 0 ? diff : channelName(left).localeCompare(channelName(right), 'de-DE');
        });
        applyFilter();
        if (!state.selectedChannel && state.channels.length > 0) state.selectedChannel = state.channels[0];
        renderShell();
        if (state.selectedChannel) loadEvents();
      })
      .catch(function (error) {
        if (!state.active) return;
        const targetElement = mountTarget();
        if (targetElement) {
          targetElement.replaceChildren();
          const failure = addText(document.createElement('p'), 'Kanäle konnten nicht geladen werden: ' + error.message);
          failure.className = 'channels2-status error';
          targetElement.appendChild(failure);
        }
      });
  }

  function activate() {
    state.active = true;
    document.querySelectorAll('.module-tab').forEach(function (button) {
      button.classList.toggle('active', button.dataset.module === 'channels2');
    });
    loadChannels();
  }

  function deactivate() {
    state.active = false;
    state.loadSequence += 1;
  }

  function installNavigation() {
    const nav = document.getElementById('module-nav');
    if (!nav || nav.querySelector('[data-module="channels2"]')) return;
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'module-tab';
    button.dataset.module = 'channels2';
    button.textContent = 'Channels 2';
    const oldChannels = nav.querySelector('[data-module="channels"]');
    if (oldChannels && oldChannels.nextSibling) nav.insertBefore(button, oldChannels.nextSibling);
    else nav.appendChild(button);
    button.addEventListener('click', function (event) {
      event.preventDefault();
      event.stopImmediatePropagation();
      activate();
    }, true);
    nav.addEventListener('click', function (event) {
      const target = event.target && event.target.closest ? event.target.closest('.module-tab') : null;
      if (!target || target.dataset.module === 'channels2') return;
      deactivate();
    }, true);
    const refresh = document.getElementById('refresh-detail');
    if (refresh) {
      refresh.addEventListener('click', function (event) {
        if (!state.active) return;
        event.preventDefault();
        event.stopImmediatePropagation();
        loadChannels();
      }, true);
    }
  }

  global.VdrSuiteChannels2 = Object.freeze({activate: activate, deactivate: deactivate, refresh: loadChannels});

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', installNavigation, {once: true});
  else installNavigation();
})(window);
