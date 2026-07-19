// Channels 2 platform module: grouped channel navigation and day programme.
(function (global) {
  'use strict';

  const CSS = `
#detail-data.channels2-mount{display:block!important;width:100%!important;max-width:none!important}
.channels2{display:grid;width:100%;max-width:none;gap:1rem;min-width:0}.channels2 h3,.channels2 p{margin:0}.channels2-toolbar{display:flex;align-items:flex-end;justify-content:space-between;gap:1rem;width:100%}.channels2-toolbar>div:first-child{display:grid;gap:.2rem}.channels2-toolbar>div:first-child p{color:#94a3b8}.channels2-tools{display:flex;flex-wrap:wrap;gap:.5rem;min-width:min(30rem,100%)}.channels2-search{flex:1 1 20rem;width:100%;min-height:2.75rem;padding:.65rem .8rem;border:1px solid #475569;border-radius:.72rem;background:#0f172a;color:#f8fafc;font:inherit}.channels2-status{padding:.7rem .85rem;border:1px solid rgba(148,163,184,.24);border-radius:.75rem;background:rgba(15,23,42,.72);color:#cbd5e1}.channels2-status.error,.channels2-feedback.error{color:#fecaca}.channels2-feedback.success{color:#bbf7d0}.channels2-grid{display:grid;grid-template-columns:minmax(19rem,26rem) minmax(0,1fr);gap:1rem;align-items:start;width:100%;min-width:0}.channels2-list{display:grid;gap:.55rem;max-height:calc(100vh - 13rem);overflow:auto;padding-right:.25rem;min-width:0}.channels2-group{border:1px solid rgba(148,163,184,.2);border-radius:.85rem;background:rgba(2,6,23,.56);overflow:hidden}.channels2-group>summary{display:grid;grid-template-columns:auto minmax(0,1fr) auto;align-items:center;gap:.55rem;min-height:3rem;padding:.7rem .8rem;cursor:pointer;list-style:none;color:#f8fafc;font-weight:850}.channels2-group>summary::-webkit-details-marker{display:none}.channels2-group>summary::before{content:'›';font-size:1.35rem;line-height:1;color:#7dd3fc;transform:rotate(0deg);transition:transform .15s ease}.channels2-group[open]>summary::before{transform:rotate(90deg)}.channels2-group>summary:hover,.channels2-group>summary:focus-visible{background:rgba(14,165,233,.12);outline:none}.channels2-group-count{padding:.15rem .45rem;border:1px solid rgba(148,163,184,.25);border-radius:999px;color:#94a3b8;font-size:.72rem}.channels2-group-channels{display:grid;gap:.38rem;padding:.25rem .45rem .55rem}.channels2-channel{display:grid;grid-template-columns:5rem minmax(0,1fr);gap:.75rem;align-items:center;width:100%;min-height:4.2rem;padding:.55rem .65rem;border:1px solid transparent;border-radius:.72rem;background:rgba(15,23,42,.66);color:#f8fafc;text-align:left}.channels2-channel:hover,.channels2-channel:focus-visible,.channels2-channel.active{border-color:rgba(56,189,248,.62);background:rgba(14,165,233,.14);outline:none}.channels2-logo{display:grid;place-items:center;width:5rem;height:3rem;padding:.25rem;border-radius:.5rem;background:rgba(248,250,252,.94);overflow:hidden}.channels2-logo img,.channels2-logo .channel-logo{display:block;width:100%!important;height:100%!important;max-width:100%!important;max-height:100%!important;object-fit:contain!important}.channels2-title{display:block;font-weight:800;overflow-wrap:anywhere}.channels2-meta{display:block;margin-top:.12rem;color:#94a3b8;font-size:.82rem}.channels2-program{display:grid;gap:.7rem;min-width:0;width:100%}.channels2-head{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:.65rem;padding:.8rem .9rem;border:1px solid rgba(56,189,248,.25);border-radius:.82rem;background:rgba(2,6,23,.64)}.channels2-date{display:flex;flex-wrap:wrap;gap:.4rem}.channels2-date button,.channels2-date input,.channels2-tools button{min-height:2.55rem;padding:.52rem .7rem;border-radius:.68rem}.channels2-date input{border:1px solid #475569;background:#0f172a;color:#f8fafc;font:inherit}.channels2-day-heading{color:#e2e8f0;font-size:1rem}.channels2-events{display:grid;gap:.42rem}.channels2-event{display:grid;grid-template-columns:6.2rem minmax(0,1fr) auto;gap:.65rem;align-items:center;width:100%;padding:.72rem .78rem;border:1px solid rgba(148,163,184,.18);border-radius:.78rem;background:rgba(15,23,42,.68);color:#f8fafc;text-align:left}.channels2-event:hover,.channels2-event:focus-visible,.channels2-event.active{border-color:rgba(56,189,248,.62);background:rgba(14,165,233,.12);outline:none}.channels2-event.current{border-color:rgba(74,222,128,.5)}.channels2-time{font-weight:800;color:#bae6fd}.channels2-badge{padding:.18rem .4rem;border:1px solid rgba(148,163,184,.28);border-radius:999px;color:#cbd5e1;font-size:.7rem;font-weight:800}.channels2-detail{display:grid;gap:.8rem;padding:1rem;border:1px solid rgba(56,189,248,.32);border-radius:.9rem;background:rgba(2,6,23,.78)}.channels2-detail-header{display:grid;gap:.25rem}.channels2-detail-subtitle{color:#e2e8f0}.channels2-detail-time{color:#93c5fd;font-weight:700}.channels2-description{max-width:85ch;color:#dbe4f0;line-height:1.5;white-space:pre-line}.channels2-actions{display:flex;flex-wrap:wrap;gap:.5rem}.channels2-actions button{min-height:2.7rem;padding:.58rem .9rem;border-radius:.7rem}.channels2-secondary-action{background:transparent!important;border:1px solid rgba(96,165,250,.6)!important;color:#bfdbfe!important}.channels2-feedback{min-height:1.35rem;padding:.45rem 0}.channels2-empty{padding:1rem;border:1px dashed rgba(148,163,184,.3);border-radius:.8rem;color:#94a3b8;text-align:center}
@media(max-width:1000px){.channels2-grid{grid-template-columns:minmax(17rem,22rem) minmax(0,1fr)}}
@media(max-width:780px){#detail-data.channels2-mount{padding-inline:0!important}.channels2{gap:.75rem}.channels2-toolbar{align-items:stretch;flex-direction:column}.channels2-tools{display:grid;grid-template-columns:minmax(0,1fr) auto;min-width:0}.channels2-grid{grid-template-columns:1fr}.channels2-list{max-height:none;overflow:visible}.channels2-group-channels{grid-template-columns:repeat(2,minmax(0,1fr))}.channels2-channel{grid-template-columns:4.5rem minmax(0,1fr)}.channels2-logo{width:4.5rem;height:2.7rem}.channels2-head{align-items:stretch;flex-direction:column}}
@media(max-width:520px){.channels2-group-channels{grid-template-columns:1fr}.channels2-channel{grid-template-columns:5.25rem minmax(0,1fr);min-height:4.5rem}.channels2-logo{width:5.25rem;height:3.15rem}.channels2-date{display:grid;grid-template-columns:auto 1fr auto}.channels2-date input{grid-column:1/-1;width:100%}.channels2-event{grid-template-columns:5rem minmax(0,1fr)}.channels2-event .channels2-badge{display:none}.channels2-description{max-height:14rem;overflow:auto}.channels2-actions{display:grid;grid-template-columns:1fr 1fr}.channels2-actions button{width:100%;font-size:.9rem}.channels2-tools{grid-template-columns:1fr}.channels2-tools button{width:100%}}
`;

  const state = {
    active: false,
    channels: [],
    visible: [],
    channel: null,
    day: new Date(),
    events: [],
    event: null,
    filter: '',
    sequence: 0,
    openGroups: Object.create(null)
  };

  const platform = () => global.VdrSuitePlatform || null;
  const clientApi = () => platform() && platform().getClientApi ? platform().getClientApi() : global.VdrSuiteClientApi;
  const mount = () => platform() && platform().getMountTarget ? (platform().getMountTarget('channels2') || platform().getMountTarget('detail')) : document.getElementById('detail-data');
  const backendId = () => String(platform() && platform().getSelectedBackendId ? platform().getSelectedBackendId() : 'default') || 'default';
  const pick = (object, keys, fallback) => {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') return object[key];
    }
    return fallback;
  };
  const text = value => String(value == null ? '' : value).trim();
  const list = (data, key) => Array.isArray(data) ? data : data && Array.isArray(data[key]) ? data[key] : data && Array.isArray(data.items) ? data.items : data && Array.isArray(data.results) ? data.results : [];
  const epoch = value => {
    const numeric = Number(value);
    if (Number.isFinite(numeric) && numeric > 0) return numeric > 1e11 ? Math.floor(numeric / 1000) : Math.floor(numeric);
    const parsed = Date.parse(String(value || ''));
    return Number.isFinite(parsed) ? Math.floor(parsed / 1000) : 0;
  };
  const channelId = channel => text(pick(channel, ['id', 'channelId', 'nativeId'], ''));
  const channelName = channel => text(pick(channel, ['name', 'channelName', 'title', 'displayName'], 'Kanal'));
  const channelNumber = channel => Number(pick(channel, ['number', 'channelNumber', 'position'], 0)) || 0;
  const channelGroup = channel => {
    const explicit = text(pick(channel, ['group', 'groupName', 'channelGroup', 'bouquet', 'category', 'provider', 'section'], ''));
    if (explicit) return explicit;
    const name = channelName(channel).toLocaleLowerCase('de-DE');
    const type = text(pick(channel, ['type', 'serviceType'], '')).toLocaleLowerCase('de-DE');
    if (type.includes('radio') || name.includes('radio')) return 'Radio';
    if (/das erste|zdf|ndr|wdr|swr|mdr|rbb|arte|3sat|phoenix|kika|tagesschau/.test(name)) return 'Öffentlich-rechtlich';
    if (/welt|n-tv|ntv|euronews|cnn|bbc/.test(name)) return 'Nachrichten';
    if (/sport|sky|eurosport|dazn/.test(name)) return 'Sport';
    if (/rtl|sat\.?1|prosieben|pro sieben|vox|kabel|sixx|tele 5|dmax|nitro/.test(name)) return 'Private';
    return 'Weitere Sender';
  };
  const eventTitle = event => text(pick(event, ['title', 'name', 'eventTitle'], 'Sendung'));
  const eventSubtitle = event => text(pick(event, ['subtitle', 'shortText', 'short_text'], ''));
  const eventDescription = event => text(pick(event, ['description', 'longText', 'long_text', 'summary'], ''));
  const eventChannelId = event => text(pick(event, ['channelId', 'channel', 'channel_id'], ''));
  const eventStart = event => epoch(pick(event, ['startTime', 'start', 'beginTime'], 0));
  const eventEnd = event => {
    const start = eventStart(event);
    const explicit = epoch(pick(event, ['endTime', 'end', 'stopTime'], 0));
    const duration = Number(pick(event, ['durationSeconds', 'duration'], 0));
    return explicit > start ? explicit : start + (Number.isFinite(duration) && duration > 0 ? duration : 0);
  };
  const addText = (element, value) => { element.textContent = String(value); return element; };
  const dayOnly = value => { const source = value instanceof Date ? value : new Date(value || Date.now()); return new Date(source.getFullYear(), source.getMonth(), source.getDate()); };
  const moveDay = (value, amount) => { const result = dayOnly(value); result.setDate(result.getDate() + amount); return result; };
  const dateValue = value => { const date = dayOnly(value); return `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${String(date.getDate()).padStart(2, '0')}`; };
  const clock = value => new Date(Number(value) * 1000).toLocaleTimeString('de-DE', {hour: '2-digit', minute: '2-digit'});
  const hhmm = value => { const date = new Date(Number(value) * 1000); return date.getHours() * 100 + date.getMinutes(); };

  function ensureStyles() {
    if (document.getElementById('vdr-suite-channels2-styles')) return;
    const style = document.createElement('style');
    style.id = 'vdr-suite-channels2-styles';
    style.textContent = CSS;
    document.head.appendChild(style);
  }

  function groupChannels(channels) {
    const groups = new Map();
    channels.slice().sort((left, right) => channelNumber(left) - channelNumber(right) || channelName(left).localeCompare(channelName(right), 'de-DE')).forEach(channel => {
      const name = channelGroup(channel);
      if (!groups.has(name)) groups.set(name, []);
      groups.get(name).push(channel);
    });
    return Array.from(groups.entries()).sort((left, right) => channelNumber(left[1][0]) - channelNumber(right[1][0]));
  }

  function filterChannels() {
    const query = state.filter.toLocaleLowerCase('de-DE');
    state.visible = state.channels.filter(channel => !query || channelName(channel).toLocaleLowerCase('de-DE').includes(query) || channelGroup(channel).toLocaleLowerCase('de-DE').includes(query) || String(channelNumber(channel)).includes(query));
  }

  function timerPayload(event, channel) {
    const start = eventStart(event);
    const end = eventEnd(event);
    const eventId = text(pick(event, ['eventId', 'id', 'nativeId'], ''));
    return {backendId: backendId(), channelId: eventChannelId(event) || channelId(channel), title: eventTitle(event), directory: '', day: dateValue(new Date(start * 1000)), weekdays: '-------', start: hhmm(start), stop: hhmm(end), priority: 50, lifetime: 99, active: true, vps: false, aux: eventId ? 'eventId=' + eventId : ''};
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
    api.fetchClientTimerCreateAction({payload: timerPayload(event, channel), cache: 'no-store', credentials: 'same-origin'})
      .then(result => {
        if (result && result.success === false) throw new Error(result.message || result.error || 'Aktion wurde abgelehnt.');
        button.textContent = 'Timer erstellt';
        feedback.className = 'channels2-feedback success';
        feedback.textContent = 'Timer für „' + eventTitle(event) + '“ wurde erstellt.';
      })
      .catch(error => {
        button.disabled = false;
        feedback.className = 'channels2-feedback error';
        feedback.textContent = 'Timer konnte nicht erstellt werden: ' + error.message;
      });
  }

  function prepareSearchTimer(event, channel, detail) {
    const feedback = detail.querySelector('.channels2-feedback');
    const editor = global.VdrSuiteEpgSearchTimerActions;
    if (!editor || typeof editor.openSearchTimerEditor !== 'function') {
      feedback.className = 'channels2-feedback error';
      feedback.textContent = 'SearchTimer-Editor ist nicht verfügbar.';
      return;
    }
    feedback.className = 'channels2-feedback';
    feedback.textContent = 'SearchTimer-Editor wird geöffnet …';
    editor.openSearchTimerEditor({title: eventTitle(event), channelId: eventChannelId(event) || channelId(channel), channelGroup: channelGroup(channel), statusTarget: detail})
      .catch(error => {
        feedback.className = 'channels2-feedback error';
        feedback.textContent = error.message;
      });
  }

  function renderDetail(parent) {
    if (!state.event || !state.channel) return;
    const detail = document.createElement('article');
    detail.className = 'channels2-detail';
    const header = document.createElement('header');
    header.className = 'channels2-detail-header';
    header.appendChild(addText(document.createElement('h3'), eventTitle(state.event)));
    if (eventSubtitle(state.event)) {
      const subtitle = addText(document.createElement('p'), eventSubtitle(state.event));
      subtitle.className = 'channels2-detail-subtitle';
      header.appendChild(subtitle);
    }
    const metadata = addText(document.createElement('p'), clock(eventStart(state.event)) + '–' + clock(eventEnd(state.event)) + ' · ' + channelName(state.channel));
    metadata.className = 'channels2-detail-time';
    header.appendChild(metadata);
    detail.appendChild(header);
    const description = addText(document.createElement('p'), eventDescription(state.event) || 'Keine Beschreibung vorhanden.');
    description.className = 'channels2-description';
    detail.appendChild(description);
    const actions = document.createElement('div');
    actions.className = 'channels2-actions';
    const timer = addText(document.createElement('button'), 'Timer erstellen');
    timer.type = 'button';
    const series = addText(document.createElement('button'), 'Serientimer vorbereiten');
    series.type = 'button';
    series.className = 'channels2-secondary-action';
    const feedback = document.createElement('p');
    feedback.className = 'channels2-feedback';
    feedback.setAttribute('role', 'status');
    feedback.setAttribute('aria-live', 'polite');
    timer.addEventListener('click', () => createTimer(state.event, state.channel, feedback, timer));
    series.addEventListener('click', () => prepareSearchTimer(state.event, state.channel, detail));
    actions.append(timer, series);
    detail.append(actions, feedback);
    parent.appendChild(detail);
  }

  function render() {
    ensureStyles();
    const target = mount();
    if (!target) return;
    target.classList.add('channels2-mount');
    target.replaceChildren();

    const root = document.createElement('section');
    root.className = 'channels2';
    const toolbar = document.createElement('header');
    toolbar.className = 'channels2-toolbar';
    const intro = document.createElement('div');
    intro.append(addText(document.createElement('h3'), 'Channels 2'), addText(document.createElement('p'), 'Kanalgruppen öffnen, Sender auswählen und das Tagesprogramm anzeigen.'));
    const tools = document.createElement('div');
    tools.className = 'channels2-tools';
    const search = document.createElement('input');
    search.type = 'search';
    search.className = 'channels2-search';
    search.placeholder = 'Kanal, Nummer oder Gruppe suchen';
    search.value = state.filter;
    const reload = addText(document.createElement('button'), 'Neu laden');
    reload.type = 'button';
    tools.append(search, reload);
    toolbar.append(intro, tools);
    root.appendChild(toolbar);

    const status = addText(document.createElement('p'), state.channels.length + ' Kanäle in ' + groupChannels(state.channels).length + ' Gruppen geladen.');
    status.className = 'channels2-status';
    root.appendChild(status);

    const grid = document.createElement('div');
    grid.className = 'channels2-grid';
    const channelList = document.createElement('section');
    channelList.className = 'channels2-list';
    channelList.setAttribute('aria-label', 'Kanalgruppen');
    const program = document.createElement('section');
    program.className = 'channels2-program';
    grid.append(channelList, program);
    root.appendChild(grid);
    target.appendChild(root);

    function renderChannels() {
      channelList.replaceChildren();
      const groups = groupChannels(state.visible);
      if (!groups.length) {
        const empty = addText(document.createElement('div'), 'Keine passenden Kanäle gefunden.');
        empty.className = 'channels2-empty';
        channelList.appendChild(empty);
        return;
      }
      groups.forEach(([groupName, channels], index) => {
        const details = document.createElement('details');
        details.className = 'channels2-group';
        const selectedInGroup = state.channel && channels.some(channel => channelId(channel) === channelId(state.channel));
        details.open = Boolean(state.filter) || selectedInGroup || state.openGroups[groupName] === true || (index === 0 && Object.keys(state.openGroups).length === 0);
        const summary = document.createElement('summary');
        summary.append(addText(document.createElement('span'), groupName));
        const count = addText(document.createElement('span'), String(channels.length));
        count.className = 'channels2-group-count';
        summary.appendChild(count);
        details.appendChild(summary);
        details.addEventListener('toggle', () => { state.openGroups[groupName] = details.open; });
        const body = document.createElement('div');
        body.className = 'channels2-group-channels';
        channels.forEach(channel => {
          const button = document.createElement('button');
          button.type = 'button';
          button.className = 'channels2-channel';
          if (state.channel && channelId(state.channel) === channelId(channel)) button.classList.add('active');
          const logo = document.createElement('span');
          logo.className = 'channels2-logo';
          if (typeof global.createChannelLogoElement === 'function') {
            const element = global.createChannelLogoElement(channelName(channel), channelId(channel));
            element.alt = channelName(channel);
            logo.appendChild(element);
          } else {
            logo.textContent = String(channelNumber(channel) || '•');
          }
          const copy = document.createElement('span');
          const title = addText(document.createElement('span'), channelName(channel));
          title.className = 'channels2-title';
          const meta = addText(document.createElement('span'), 'Kanal ' + (channelNumber(channel) || '-'));
          meta.className = 'channels2-meta';
          copy.append(title, meta);
          button.append(logo, copy);
          button.addEventListener('click', () => {
            state.channel = channel;
            state.event = null;
            state.openGroups[groupName] = true;
            render();
            loadEvents();
          });
          body.appendChild(button);
        });
        details.appendChild(body);
        channelList.appendChild(details);
      });
    }

    function renderProgram() {
      program.replaceChildren();
      if (!state.channel) {
        const empty = addText(document.createElement('div'), 'Eine Kanalgruppe öffnen und einen Sender auswählen.');
        empty.className = 'channels2-empty';
        program.appendChild(empty);
        return;
      }
      const head = document.createElement('article');
      head.className = 'channels2-head';
      const copy = document.createElement('div');
      copy.append(addText(document.createElement('h3'), channelName(state.channel)), addText(document.createElement('p'), channelGroup(state.channel) + ' · Kanal ' + (channelNumber(state.channel) || '-')));
      const controls = document.createElement('div');
      controls.className = 'channels2-date';
      const previous = addText(document.createElement('button'), '←');
      const today = addText(document.createElement('button'), 'Heute');
      const next = addText(document.createElement('button'), '→');
      previous.type = today.type = next.type = 'button';
      const input = document.createElement('input');
      input.type = 'date';
      input.value = dateValue(state.day);
      previous.addEventListener('click', () => { state.day = moveDay(state.day, -1); loadEvents(); });
      today.addEventListener('click', () => { state.day = dayOnly(new Date()); loadEvents(); });
      next.addEventListener('click', () => { state.day = moveDay(state.day, 1); loadEvents(); });
      input.addEventListener('change', () => { state.day = dayOnly(input.value); loadEvents(); });
      controls.append(previous, today, next, input);
      head.append(copy, controls);
      program.appendChild(head);
      const heading = addText(document.createElement('h3'), dayOnly(state.day).toLocaleDateString('de-DE', {weekday: 'long', day: '2-digit', month: 'long', year: 'numeric'}));
      heading.className = 'channels2-day-heading';
      program.appendChild(heading);
      const events = document.createElement('section');
      events.className = 'channels2-events';
      const now = Math.floor(Date.now() / 1000);
      if (!state.events.length) {
        const empty = addText(document.createElement('div'), 'Für diesen Tag wurden keine EPG-Einträge gefunden.');
        empty.className = 'channels2-empty';
        events.appendChild(empty);
      }
      state.events.forEach(event => {
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'channels2-event';
        if (eventStart(event) <= now && eventEnd(event) > now) button.classList.add('current');
        if (state.event === event) button.classList.add('active');
        const time = addText(document.createElement('span'), clock(eventStart(event)) + '–' + clock(eventEnd(event)));
        time.className = 'channels2-time';
        const eventCopy = document.createElement('span');
        const title = addText(document.createElement('span'), eventTitle(event));
        title.className = 'channels2-title';
        eventCopy.appendChild(title);
        if (eventSubtitle(event)) {
          const subtitle = addText(document.createElement('span'), eventSubtitle(event));
          subtitle.className = 'channels2-meta';
          eventCopy.appendChild(subtitle);
        }
        const badge = addText(document.createElement('span'), eventStart(event) <= now && eventEnd(event) > now ? 'Jetzt' : 'EPG');
        badge.className = 'channels2-badge';
        button.append(time, eventCopy, badge);
        button.addEventListener('click', () => { state.event = event; render(); });
        events.appendChild(button);
      });
      program.appendChild(events);
      renderDetail(program);
    }

    search.addEventListener('input', () => {
      state.filter = search.value.trim();
      filterChannels();
      renderChannels();
      status.textContent = state.visible.length + ' von ' + state.channels.length + ' Kanälen sichtbar.';
    });
    reload.addEventListener('click', loadChannels);
    renderChannels();
    renderProgram();
  }

  function loadEvents() {
    const api = clientApi();
    if (!api || !state.channel) { render(); return; }
    const start = dayOnly(state.day);
    const end = moveDay(start, 1);
    const range = {start: Math.floor(start.getTime() / 1000), end: Math.floor(end.getTime() / 1000)};
    const sequence = ++state.sequence;
    const cached = typeof api.fetchClientEpgCacheWindow === 'function'
      ? api.fetchClientEpgCacheWindow({query: {backend: backendId(), channelId: channelId(state.channel), fromTime: String(range.start), untilTime: String(range.end), limit: '0', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'}).catch(() => ({events: []}))
      : Promise.resolve({events: []});
    cached
      .then(data => list(data, 'events').length || typeof api.fetchClientEpgChannelWindow !== 'function' ? data : api.fetchClientEpgChannelWindow({query: {channelId: channelId(state.channel), from: String(range.start), timespan: String(range.end - range.start), limit: '192', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'}))
      .then(data => {
        if (!state.active || sequence !== state.sequence) return;
        state.events = list(data, 'events').filter(event => {
          const id = eventChannelId(event);
          return (!id || id === channelId(state.channel)) && eventStart(event) < range.end && eventEnd(event) > range.start;
        }).sort((left, right) => eventStart(left) - eventStart(right));
        state.event = null;
        render();
      })
      .catch(error => {
        const target = mount();
        if (target && state.active) {
          const message = addText(document.createElement('p'), 'Tagesprogramm konnte nicht geladen werden: ' + error.message);
          message.className = 'channels2-status error';
          target.prepend(message);
        }
      });
  }

  function loadChannels() {
    const api = clientApi();
    const target = mount();
    if (!target) return;
    target.classList.add('channels2-mount');
    target.replaceChildren();
    const loading = addText(document.createElement('p'), 'Lade Kanalgruppen und Sender …');
    loading.className = 'channels2-status';
    target.appendChild(loading);
    if (!api || typeof api.fetchClientChannels !== 'function') {
      loading.className = 'channels2-status error';
      loading.textContent = 'Kanäle konnten nicht geladen werden: Client API ist nicht verfügbar.';
      return;
    }
    api.fetchClientChannels({query: {backend: backendId(), _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'})
      .then(data => {
        if (!state.active) return;
        state.channels = list(data, 'channels').map(channel => Object.assign({}, channel, {
          id: channelId(channel),
          name: channelName(channel),
          number: channelNumber(channel),
          group: channelGroup(channel)
        })).sort((left, right) => channelNumber(left) - channelNumber(right) || channelName(left).localeCompare(channelName(right), 'de-DE'));
        filterChannels();
        render();
      })
      .catch(error => {
        if (!state.active) return;
        loading.className = 'channels2-status error';
        loading.textContent = 'Kanäle konnten nicht geladen werden: ' + error.message;
      });
  }

  const moduleApi = Object.freeze({
    activate() {
      state.active = true;
      loadChannels();
    },
    deactivate() {
      state.active = false;
      state.sequence += 1;
      const target = mount();
      if (target) target.classList.remove('channels2-mount');
    },
    refresh() {
      if (state.active) loadChannels();
    }
  });

  ensureStyles();
  const runtime = platform();
  if (runtime && typeof runtime.registerModule === 'function' && (!runtime.hasModule || !runtime.hasModule('channels2'))) runtime.registerModule('channels2', moduleApi);
  global.VdrSuiteChannels2 = moduleApi;
}(window));
