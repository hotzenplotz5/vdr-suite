// Independent Channels 2 module: channel list, day programme, event details and timer actions.
(function (global) {
  'use strict';

  const state = {
    active: false,
    channels: [],
    visibleChannels: [],
    channel: null,
    day: new Date(),
    events: [],
    event: null,
    filter: '',
    sequence: 0
  };

  function platform() { return global.VdrSuitePlatform || null; }
  function api() {
    const runtime = platform();
    return runtime && typeof runtime.getClientApi === 'function'
      ? runtime.getClientApi()
      : global.VdrSuiteClientApi || null;
  }
  function mount() {
    const runtime = platform();
    return runtime && typeof runtime.getMountTarget === 'function'
      ? runtime.getMountTarget('channels') || document.getElementById('detail-data')
      : document.getElementById('detail-data');
  }
  function backendId() {
    const runtime = platform();
    const value = runtime && typeof runtime.getSelectedBackendId === 'function'
      ? String(runtime.getSelectedBackendId() || '').trim()
      : '';
    return value || 'default';
  }
  function first(source, keys, fallback) {
    for (const key of keys) {
      if (source && source[key] !== undefined && source[key] !== null && source[key] !== '') return source[key];
    }
    return fallback;
  }
  function list(data, key) {
    if (Array.isArray(data)) return data;
    if (data && Array.isArray(data[key])) return data[key];
    if (data && Array.isArray(data.items)) return data.items;
    if (data && Array.isArray(data.results)) return data.results;
    return [];
  }
  function string(value) { return String(value === undefined || value === null ? '' : value).trim(); }
  function epoch(value) {
    const numeric = Number(value);
    if (Number.isFinite(numeric) && numeric > 0) return numeric > 100000000000 ? Math.floor(numeric / 1000) : Math.floor(numeric);
    const parsed = Date.parse(String(value || ''));
    return Number.isFinite(parsed) ? Math.floor(parsed / 1000) : 0;
  }
  function channelId(channel) { return string(first(channel, ['id', 'channelId', 'nativeId'], '')); }
  function channelName(channel) { return string(first(channel, ['name', 'channelName', 'title', 'displayName'], 'Kanal')); }
  function channelNumber(channel) { return Number(first(channel, ['number', 'channelNumber', 'position'], 0)) || 0; }
  function channelGroup(channel) { return string(first(channel, ['group', 'groupName', 'channelGroup', 'bouquet', 'provider'], '')); }
  function eventId(event) { return string(first(event, ['eventId', 'id', 'nativeId'], '')); }
  function eventChannelId(event) { return string(first(event, ['channelId', 'channel', 'channel_id'], '')); }
  function eventTitle(event) { return string(first(event, ['title', 'name', 'eventTitle'], 'Sendung')); }
  function eventSubtitle(event) { return string(first(event, ['subtitle', 'shortText', 'short_text'], '')); }
  function eventDescription(event) { return string(first(event, ['description', 'longText', 'long_text', 'summary'], '')); }
  function eventStart(event) { return epoch(first(event, ['startTime', 'start', 'beginTime'], 0)); }
  function eventEnd(event) {
    const start = eventStart(event);
    const explicit = epoch(first(event, ['endTime', 'end', 'stopTime'], 0));
    const duration = Number(first(event, ['durationSeconds', 'duration'], 0));
    return explicit > start ? explicit : start + (Number.isFinite(duration) && duration > 0 ? duration : 0);
  }
  function day(value) {
    const source = value instanceof Date ? value : new Date(value || Date.now());
    return new Date(source.getFullYear(), source.getMonth(), source.getDate());
  }
  function moveDay(value, amount) { const result = day(value); result.setDate(result.getDate() + amount); return result; }
  function bounds(value) {
    const start = day(value);
    const end = moveDay(start, 1);
    return {start: Math.floor(start.getTime() / 1000), end: Math.floor(end.getTime() / 1000)};
  }
  function dateValue(value) {
    const date = day(value);
    return String(date.getFullYear()).padStart(4, '0') + '-' + String(date.getMonth() + 1).padStart(2, '0') + '-' + String(date.getDate()).padStart(2, '0');
  }
  function clock(value) { return new Date(Number(value) * 1000).toLocaleTimeString('de-DE', {hour: '2-digit', minute: '2-digit'}); }
  function hhmm(value) { const date = new Date(Number(value) * 1000); return date.getHours() * 100 + date.getMinutes(); }
  function addText(element, value) { element.textContent = String(value); return element; }

  function installStyles() {
    if (document.getElementById('vdr-suite-channels2-styles')) return;
    const style = document.createElement('style');
    style.id = 'vdr-suite-channels2-styles';
    style.textContent = `
.channels2{display:grid;gap:.85rem;min-width:0}.channels2-toolbar{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:.55rem}.channels2-tools{display:flex;flex-wrap:wrap;gap:.45rem}.channels2-search{min-width:min(22rem,100%);padding:.6rem .72rem;border:1px solid #475569;border-radius:.72rem;background:#0f172a;color:#f8fafc;font:inherit}.channels2-status{padding:.7rem .8rem;border:1px solid rgba(148,163,184,.25);border-radius:.75rem;background:rgba(15,23,42,.72);color:#cbd5e1}.channels2-status.error{border-color:rgba(248,113,113,.5);color:#fecaca}.channels2-grid{display:grid;grid-template-columns:minmax(16rem,23rem) minmax(0,1fr);gap:.85rem;align-items:start}.channels2-list{display:grid;gap:.42rem;max-height:68vh;overflow:auto}.channels2-channel{display:grid;grid-template-columns:3.6rem minmax(0,1fr);gap:.6rem;align-items:center;width:100%;padding:.62rem;border:1px solid rgba(148,163,184,.2);border-radius:.78rem;background:rgba(15,23,42,.75);color:#f8fafc;text-align:left}.channels2-channel:hover,.channels2-channel:focus,.channels2-channel.active{border-color:rgba(56,189,248,.62);background:rgba(14,165,233,.14);outline:none}.channels2-logo{display:grid;place-items:center;width:3.6rem;height:2.5rem}.channels2-logo img,.channels2-logo .channel-logo{max-width:100%;max-height:100%;object-fit:contain}.channels2-title{display:block;font-weight:850;overflow-wrap:anywhere}.channels2-meta{display:block;margin-top:.12rem;color:#94a3b8;font-size:.84rem}.channels2-program{display:grid;gap:.62rem;min-width:0}.channels2-head{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:.55rem;padding:.72rem .78rem;border:1px solid rgba(56,189,248,.28);border-radius:.82rem;background:rgba(2,6,23,.72)}.channels2-head h3,.channels2-head p{margin:0}.channels2-date{display:flex;flex-wrap:wrap;gap:.4rem}.channels2-date button,.channels2-date input,.channels2-tools button{min-height:2.45rem;padding:.5rem .68rem;border-radius:.68rem}.channels2-date input{border:1px solid #475569;background:#0f172a;color:#f8fafc;font:inherit}.channels2-events{display:grid;gap:.46rem}.channels2-event{display:grid;grid-template-columns:6.2rem minmax(0,1fr) auto;gap:.62rem;align-items:center;width:100%;padding:.7rem .75rem;border:1px solid rgba(148,163,184,.2);border-radius:.8rem;background:rgba(15,23,42,.75);color:#f8fafc;text-align:left}.channels2-event:hover,.channels2-event:focus,.channels2-event.active{border-color:rgba(56,189,248,.62);background:rgba(14,165,233,.12);outline:none}.channels2-event.current{border-color:rgba(74,222,128,.5)}.channels2-time{font-weight:850;color:#bae6fd}.channels2-badge{padding:.2rem .42rem;border:1px solid rgba(148,163,184,.28);border-radius:999px;color:#cbd5e1;font-size:.72rem;font-weight:800}.channels2-detail{display:grid;gap:.65rem;padding:.8rem;border:1px solid rgba(56,189,248,.34);border-radius:.86rem;background:rgba(2,6,23,.78)}.channels2-detail h3,.channels2-detail p{margin:0}.channels2-actions{display:flex;flex-wrap:wrap;gap:.48rem}.channels2-actions button{min-height:2.55rem;padding:.55rem .8rem;border-radius:.68rem}.channels2-feedback{min-height:1.3rem;color:#cbd5e1}.channels2-feedback.error{color:#fecaca}.channels2-empty{padding:1rem;border:1px dashed rgba(148,163,184,.3);border-radius:.8rem;color:#94a3b8;text-align:center}@media(max-width:900px){.channels2-grid{grid-template-columns:1fr}.channels2-list{max-height:42vh}.channels2-event{grid-template-columns:5rem minmax(0,1fr)}.channels2-event .channels2-badge{grid-column:1/-1;justify-self:start}}`;
    document.head.appendChild(style);
  }

  function timerPayload(event, channel) {
    const start = eventStart(event);
    const end = eventEnd(event);
    return {
      backendId: backendId(), channelId: eventChannelId(event) || channelId(channel),
      title: eventTitle(event), directory: '', day: dateValue(new Date(start * 1000)),
      weekdays: '-------', start: hhmm(start), stop: hhmm(end), priority: 50,
      lifetime: 99, active: true, vps: false,
      aux: eventId(event) ? 'eventId=' + eventId(event) : ''
    };
  }

  function searchTimerPayload(event, channel) {
    const title = eventTitle(event);
    const id = eventChannelId(event) || channelId(channel);
    return {
      backendId: backendId(), name: title, query: title, active: true, directory: '',
      priority: 50, lifetime: 99, marginStartMinutes: 5, marginStopMinutes: 10,
      useVps: false, useChannel: id ? 1 : 0, channels: '', channelMin: id,
      channelMax: id, useTime: false, startTime: 0, stopTime: 0,
      useDuration: false, durationMinMinutes: 0, durationMaxMinutes: 0,
      useDayOfWeek: false, dayOfWeek: 0, avoidRepeats: true, allowedRepeats: 0,
      repeatsWithinDays: 0, compareTitle: true, compareSubtitle: false,
      compareSummary: false, compareCategories: false, compareTime: false,
      useSeriesRecording: false, keepRecordings: 0, deleteMode: 0,
      searchTimerAction: 0, blacklistMode: 0, blacklistIds: '', mode: 0,
      matchCase: false, tolerance: 0, summaryMatch: 0, useExtendedEpgInfo: false,
      extendedEpgInfo: '', ignoreMissingEpgCategories: false, contentDescriptors: '',
      useInFavorites: false, activeFrom: '', activeUntil: '', pauseOnRecordings: false,
      switchMinutesBefore: 0, unmuteSoundOnSwitch: false, deleteRecordingsAfterDays: 0,
      deleteAfterCountRecordings: 0, deleteAfterDaysOfFirstRecording: 0
    };
  }

  function runAction(kind, event, channel, feedback, button) {
    const client = api();
    const method = kind === 'timer' ? 'fetchClientTimerCreateAction' : 'fetchClientSearchTimerCreateAction';
    if (!client || typeof client[method] !== 'function') {
      feedback.className = 'channels2-feedback error';
      feedback.textContent = (kind === 'timer' ? 'Timer' : 'SearchTimer') + '-API ist nicht verfügbar.';
      return;
    }
    button.disabled = true;
    feedback.className = 'channels2-feedback';
    feedback.textContent = kind === 'timer' ? 'Timer wird erstellt …' : 'Serientimer wird erstellt …';
    client[method]({
      payload: kind === 'timer' ? timerPayload(event, channel) : searchTimerPayload(event, channel),
      cache: 'no-store', credentials: 'same-origin'
    }).then(function (result) {
      if (result && result.success === false) throw new Error(result.message || result.error || 'Aktion wurde abgelehnt.');
      button.textContent = kind === 'timer' ? 'Timer erstellt' : 'Serientimer erstellt';
      feedback.textContent = (kind === 'timer' ? 'Timer' : 'Serientimer') + ' für „' + eventTitle(event) + '“ wurde erstellt.';
    }).catch(function (error) {
      button.disabled = false;
      feedback.className = 'channels2-feedback error';
      feedback.textContent = 'Aktion fehlgeschlagen: ' + error.message;
    });
  }

  function openAdvanced(event, channel) {
    const payload = searchTimerPayload(event, channel);
    const tab = document.querySelector('[data-module="searchtimers"]');
    if (!tab) return;
    tab.click();
    let attempts = 0;
    const fill = function () {
      attempts += 1;
      const form = document.querySelector('form[data-searchtimer-editor-form="create"]');
      if (!form) { if (attempts < 30) global.setTimeout(fill, 100); return; }
      function set(name, value, eventName) {
        const input = form.elements[name];
        if (!input) return;
        if (input.type === 'checkbox') input.checked = Boolean(value); else input.value = String(value || '');
        if (eventName) input.dispatchEvent(new Event(eventName, {bubbles: true}));
      }
      set('name', payload.name); set('query', payload.query); set('active', true);
      set('compareTitle', true); set('avoidRepeats', true); set('channelFilterMode', payload.useChannel, 'change');
      global.setTimeout(function () {
        set('channelSelectorGroup', channelGroup(channel), 'change');
        global.setTimeout(function () { set('channelId', channelId(channel), 'change'); }, 80);
        form.scrollIntoView({behavior: 'smooth', block: 'start'});
      }, 80);
    };
    global.setTimeout(fill, 80);
  }

  function filterChannels() {
    const query = state.filter.toLocaleLowerCase('de-DE');
    state.visibleChannels = state.channels.filter(function (channel) {
      return !query || channelName(channel).toLocaleLowerCase('de-DE').includes(query) ||
        channelGroup(channel).toLocaleLowerCase('de-DE').includes(query) ||
        String(channelNumber(channel)).includes(query);
    });
  }

  function renderDetail(parent) {
    const event = state.event;
    const channel = state.channel;
    if (!event || !channel) return;
    const detail = document.createElement('article');
    detail.className = 'channels2-detail';
    detail.appendChild(addText(document.createElement('h3'), eventTitle(event)));
    if (eventSubtitle(event)) detail.appendChild(addText(document.createElement('p'), eventSubtitle(event)));
    detail.appendChild(addText(document.createElement('p'), clock(eventStart(event)) + '–' + clock(eventEnd(event)) + ' · ' + channelName(channel)));
    detail.appendChild(addText(document.createElement('p'), eventDescription(event) || 'Keine Beschreibung vorhanden.'));
    const actions = document.createElement('div');
    actions.className = 'channels2-actions';
    const timer = addText(document.createElement('button'), 'Timer erstellen');
    const series = addText(document.createElement('button'), 'Serientimer erstellen');
    const advanced = addText(document.createElement('button'), 'Erweiterter SearchTimer');
    timer.type = series.type = advanced.type = 'button';
    const feedback = document.createElement('p');
    feedback.className = 'channels2-feedback';
    feedback.setAttribute('role', 'status');
    feedback.setAttribute('aria-live', 'polite');
    timer.addEventListener('click', function () { runAction('timer', event, channel, feedback, timer); });
    series.addEventListener('click', function () { runAction('series', event, channel, feedback, series); });
    advanced.addEventListener('click', function () { openAdvanced(event, channel); });
    actions.append(timer, series, advanced);
    detail.append(actions, feedback);
    parent.appendChild(detail);
  }

  function render() {
    installStyles();
    const target = mount();
    if (!target) return;
    target.replaceChildren();
    const root = document.createElement('section');
    root.className = 'channels2';
    const toolbar = document.createElement('div');
    toolbar.className = 'channels2-toolbar';
    const intro = document.createElement('div');
    intro.append(addText(document.createElement('h3'), 'Channels 2'), addText(document.createElement('p'), 'Neue unabhängige Kanal- und Tagesprogramm-Ansicht.'));
    const tools = document.createElement('div');
    tools.className = 'channels2-tools';
    const search = document.createElement('input');
    search.type = 'search'; search.className = 'channels2-search'; search.placeholder = 'Kanal, Nummer oder Gruppe suchen'; search.value = state.filter;
    const reload = addText(document.createElement('button'), 'Neu laden'); reload.type = 'button';
    tools.append(search, reload); toolbar.append(intro, tools); root.appendChild(toolbar);
    const status = addText(document.createElement('p'), state.channels.length + ' Kanäle geladen.');
    status.className = 'channels2-status'; root.appendChild(status);
    const grid = document.createElement('div'); grid.className = 'channels2-grid';
    const channelList = document.createElement('section'); channelList.className = 'channels2-list';
    const program = document.createElement('section'); program.className = 'channels2-program';
    grid.append(channelList, program); root.appendChild(grid); target.appendChild(root);

    function renderChannels() {
      channelList.replaceChildren();
      state.visibleChannels.forEach(function (channel) {
        const button = document.createElement('button'); button.type = 'button'; button.className = 'channels2-channel';
        if (state.channel && channelId(state.channel) === channelId(channel)) button.classList.add('active');
        const logo = document.createElement('span'); logo.className = 'channels2-logo';
        if (typeof global.createChannelLogoElement === 'function') logo.appendChild(global.createChannelLogoElement(channelName(channel), channelId(channel)));
        else logo.textContent = String(channelNumber(channel) || '•');
        const copy = document.createElement('span');
        const title = addText(document.createElement('span'), channelName(channel)); title.className = 'channels2-title';
        const meta = addText(document.createElement('span'), 'Nr. ' + (channelNumber(channel) || '-') + (channelGroup(channel) ? ' · ' + channelGroup(channel) : '')); meta.className = 'channels2-meta';
        copy.append(title, meta); button.append(logo, copy);
        button.addEventListener('click', function () { state.channel = channel; state.event = null; render(); loadEvents(); });
        channelList.appendChild(button);
      });
    }

    function renderProgram() {
      program.replaceChildren();
      if (!state.channel) { const empty = addText(document.createElement('div'), 'Links einen Kanal auswählen.'); empty.className = 'channels2-empty'; program.appendChild(empty); return; }
      const head = document.createElement('article'); head.className = 'channels2-head';
      const copy = document.createElement('div'); copy.append(addText(document.createElement('h3'), channelName(state.channel)), addText(document.createElement('p'), 'Kanal ' + (channelNumber(state.channel) || '-') + (channelGroup(state.channel) ? ' · ' + channelGroup(state.channel) : '')));
      const controls = document.createElement('div'); controls.className = 'channels2-date';
      const previous = addText(document.createElement('button'), '← Tag'); const today = addText(document.createElement('button'), 'Heute'); const next = addText(document.createElement('button'), 'Tag →');
      previous.type = today.type = next.type = 'button';
      const input = document.createElement('input'); input.type = 'date'; input.value = dateValue(state.day);
      previous.addEventListener('click', function () { state.day = moveDay(state.day, -1); loadEvents(); });
      today.addEventListener('click', function () { state.day = day(new Date()); loadEvents(); });
      next.addEventListener('click', function () { state.day = moveDay(state.day, 1); loadEvents(); });
      input.addEventListener('change', function () { state.day = day(input.value); loadEvents(); });
      controls.append(previous, today, next, input); head.append(copy, controls); program.appendChild(head);
      const heading = addText(document.createElement('h3'), day(state.day).toLocaleDateString('de-DE', {weekday: 'long', day: '2-digit', month: 'long', year: 'numeric'})); program.appendChild(heading);
      const events = document.createElement('section'); events.className = 'channels2-events';
      const now = Math.floor(Date.now() / 1000);
      if (!state.events.length) { const empty = addText(document.createElement('div'), 'Für diesen Tag wurden keine EPG-Einträge gefunden.'); empty.className = 'channels2-empty'; events.appendChild(empty); }
      state.events.forEach(function (event) {
        const button = document.createElement('button'); button.type = 'button'; button.className = 'channels2-event';
        if (eventStart(event) <= now && eventEnd(event) > now) button.classList.add('current');
        if (state.event === event) button.classList.add('active');
        const time = addText(document.createElement('span'), clock(eventStart(event)) + '–' + clock(eventEnd(event))); time.className = 'channels2-time';
        const copy = document.createElement('span'); const title = addText(document.createElement('span'), eventTitle(event)); title.className = 'channels2-title'; copy.appendChild(title);
        if (eventSubtitle(event)) { const sub = addText(document.createElement('span'), eventSubtitle(event)); sub.className = 'channels2-meta'; copy.appendChild(sub); }
        const badge = addText(document.createElement('span'), eventStart(event) <= now && eventEnd(event) > now ? 'Läuft jetzt' : 'EPG'); badge.className = 'channels2-badge';
        button.append(time, copy, badge); button.addEventListener('click', function () { state.event = event; render(); }); events.appendChild(button);
      });
      program.appendChild(events); renderDetail(program);
    }

    search.addEventListener('input', function () { state.filter = search.value.trim(); filterChannels(); renderChannels(); status.textContent = state.visibleChannels.length + ' von ' + state.channels.length + ' Kanälen sichtbar.'; });
    reload.addEventListener('click', loadChannels);
    renderChannels(); renderProgram();
  }

  function loadEvents() {
    const client = api();
    if (!client || !state.channel) { render(); return; }
    const range = bounds(state.day);
    const sequence = ++state.sequence;
    const cached = typeof client.fetchClientEpgCacheWindow === 'function'
      ? client.fetchClientEpgCacheWindow({query: {backend: backendId(), channelId: channelId(state.channel), fromTime: String(range.start), untilTime: String(range.end), limit: '0', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'}).catch(function () { return {events: []}; })
      : Promise.resolve({events: []});
    cached.then(function (data) {
      if (list(data, 'events').length || typeof client.fetchClientEpgCacheRefresh !== 'function') return data;
      return client.fetchClientEpgCacheRefresh({query: {backend: backendId(), channelId: channelId(state.channel), from: String(range.start), timespan: String(range.end - range.start), limit: '0', channelEventLimit: '192', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'})
        .catch(function () { return null; })
        .then(function () { return client.fetchClientEpgCacheWindow({query: {backend: backendId(), channelId: channelId(state.channel), fromTime: String(range.start), untilTime: String(range.end), limit: '0', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'}).catch(function () { return {events: []}; }); });
    }).then(function (data) {
      if (list(data, 'events').length || typeof client.fetchClientEpgChannelWindow !== 'function') return data;
      return client.fetchClientEpgChannelWindow({query: {channelId: channelId(state.channel), from: String(range.start), timespan: String(range.end - range.start), limit: '192', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'});
    }).then(function (data) {
      if (!state.active || sequence !== state.sequence) return;
      state.events = list(data, 'events').filter(function (event) {
        const id = eventChannelId(event); return (!id || id === channelId(state.channel)) && eventStart(event) < range.end && eventEnd(event) > range.start;
      }).sort(function (left, right) { return eventStart(left) - eventStart(right); });
      state.event = null; render();
    }).catch(function (error) {
      const target = mount(); if (!target || !state.active) return; const message = addText(document.createElement('p'), 'Tagesprogramm konnte nicht geladen werden: ' + error.message); message.className = 'channels2-status error'; target.prepend(message);
    });
  }

  function loadChannels() {
    const client = api();
    const target = mount();
    if (!target) return;
    target.replaceChildren();
    const loading = addText(document.createElement('p'), 'Lade Channels 2 …'); loading.className = 'channels2-status'; target.appendChild(loading);
    if (!client || typeof client.fetchClientChannels !== 'function') { loading.className = 'channels2-status error'; loading.textContent = 'Kanäle konnten nicht geladen werden: Client API ist nicht verfügbar.'; return; }
    client.fetchClientChannels({query: {backend: backendId(), _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'})
      .then(function (data) {
        if (!state.active) return;
        state.channels = list(data, 'channels').map(function (channel) { return Object.assign({}, channel, {id: channelId(channel), name: channelName(channel), number: channelNumber(channel), group: channelGroup(channel)}); }).sort(function (left, right) { return channelNumber(left) - channelNumber(right) || channelName(left).localeCompare(channelName(right), 'de-DE'); });
        filterChannels(); if (!state.channel && state.channels.length) state.channel = state.channels[0]; render(); if (state.channel) loadEvents();
      })
      .catch(function (error) { if (!state.active) return; loading.className = 'channels2-status error'; loading.textContent = 'Kanäle konnten nicht geladen werden: ' + error.message; });
  }

  function activate() {
    state.active = true;
    document.querySelectorAll('.module-tab').forEach(function (button) { button.classList.toggle('active', button.dataset.module === 'channels2'); });
    loadChannels();
  }
  function deactivate() { state.active = false; state.sequence += 1; }

  function install() {
    const nav = document.getElementById('module-nav');
    if (!nav || nav.querySelector('[data-module="channels2"]')) return;
    const button = addText(document.createElement('button'), 'Channels 2');
    button.type = 'button'; button.className = 'module-tab'; button.dataset.module = 'channels2';
    const old = nav.querySelector('[data-module="channels"]');
    if (old && old.nextSibling) nav.insertBefore(button, old.nextSibling); else nav.appendChild(button);
    button.addEventListener('click', function (event) { event.preventDefault(); event.stopImmediatePropagation(); activate(); }, true);
    nav.addEventListener('click', function (event) { const tab = event.target && event.target.closest ? event.target.closest('.module-tab') : null; if (tab && tab.dataset.module !== 'channels2') deactivate(); }, true);
    const refresh = document.getElementById('refresh-detail');
    if (refresh) refresh.addEventListener('click', function (event) { if (!state.active) return; event.preventDefault(); event.stopImmediatePropagation(); loadChannels(); }, true);
  }

  global.VdrSuiteChannels2 = Object.freeze({activate: activate, deactivate: deactivate, refresh: loadChannels});
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', install, {once: true}); else install();
})(window);
