// Canonical Channel browser: grouped channels, parity filters, inline programme and EPG artwork.
(function (global) {
  'use strict';

  const CSS = `
#detail-data.channels2-mount{display:block!important;width:100%!important;max-width:none!important}
.channels2{display:grid;width:100%;gap:1rem}.channels2 h3,.channels2 p{margin:0}.channels2-toolbar{display:flex;align-items:flex-end;justify-content:space-between;gap:1rem}.channels2-toolbar>div:first-child{display:grid;gap:.2rem}.channels2-toolbar p{color:#94a3b8}.channels2-tools{display:flex;gap:.5rem;min-width:min(30rem,100%)}.channels2-search{flex:1 1 auto;min-height:2.75rem;padding:.65rem .8rem;border:1px solid #475569;border-radius:.72rem;background:#0f172a;color:#f8fafc;font:inherit}.channels2-filters{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:.65rem}.channels2-filter-group{display:grid;gap:.35rem;padding:.55rem .65rem;border:1px solid rgba(148,163,184,.2);border-radius:.78rem;background:rgba(15,23,42,.58)}.channels2-filter-label{color:#94a3b8;font-size:.72rem;font-weight:800;text-transform:uppercase;letter-spacing:.04em}.channels2-filter-buttons{display:flex;flex-wrap:wrap;gap:.35rem}.channels2-filter-button{min-height:2.25rem;padding:.38rem .62rem;border:1px solid rgba(148,163,184,.3);border-radius:999px;background:rgba(30,41,59,.78);color:#cbd5e1;font:inherit;font-size:.82rem;font-weight:750}.channels2-filter-button.active{border-color:rgba(56,189,248,.78);background:rgba(14,165,233,.2);color:#f0f9ff}.channels2-status{padding:.7rem .85rem;border:1px solid rgba(148,163,184,.24);border-radius:.75rem;background:rgba(15,23,42,.72);color:#cbd5e1}.channels2-list{display:grid;gap:.65rem}.channels2-group{border:1px solid rgba(148,163,184,.2);border-radius:.9rem;background:rgba(2,6,23,.58);overflow:hidden}.channels2-group>summary{display:grid;grid-template-columns:auto minmax(0,1fr) auto;align-items:center;gap:.6rem;min-height:3.1rem;padding:.75rem .85rem;cursor:pointer;list-style:none;color:#f8fafc;font-weight:850}.channels2-group>summary::-webkit-details-marker{display:none}.channels2-group>summary::before{content:'›';font-size:1.4rem;color:#7dd3fc;transition:transform .15s ease}.channels2-group[open]>summary::before{transform:rotate(90deg)}.channels2-group-count{padding:.15rem .45rem;border:1px solid rgba(148,163,184,.25);border-radius:999px;color:#94a3b8;font-size:.72rem}.channels2-group-channels{display:grid;gap:.45rem;padding:.35rem .5rem .6rem}.channels2-channel-block{display:grid;gap:.45rem}.channels2-channel{display:grid;grid-template-columns:5.25rem minmax(0,1fr) auto;gap:.75rem;align-items:center;width:100%;min-height:4.5rem;padding:.55rem .65rem;border:1px solid transparent;border-radius:.78rem;background:rgba(15,23,42,.72);color:#f8fafc;text-align:left}.channels2-channel::after{content:'›';font-size:1.35rem;color:#7dd3fc}.channels2-channel.active{border-color:rgba(56,189,248,.7);background:rgba(14,165,233,.16)}.channels2-channel.active::after{transform:rotate(90deg)}.channels2-logo{display:grid;place-items:center;width:5.25rem;height:3.15rem;padding:.25rem;border-radius:.55rem;background:rgba(248,250,252,.95);overflow:hidden}.channels2-logo img,.channels2-logo .channel-logo{display:block;width:100%!important;height:100%!important;max-width:100%!important;max-height:100%!important;object-fit:contain!important}.channels2-title{display:block;font-weight:800}.channels2-meta{display:block;margin-top:.12rem;color:#94a3b8;font-size:.82rem}.channels2-now{display:block;margin-top:.2rem;color:#bae6fd;font-size:.8rem}.channels2-inline-program{display:grid;gap:.55rem;margin:.05rem .1rem .35rem;padding:.65rem;border:1px solid rgba(56,189,248,.3);border-radius:.82rem;background:rgba(2,6,23,.82)}.channels2-program-head{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:.5rem}.channels2-date{display:grid;grid-template-columns:auto minmax(0,1fr) auto;gap:.35rem;min-width:min(31rem,100%)}.channels2-date button,.channels2-date input,.channels2-tools button{min-height:2.45rem;padding:.48rem .65rem;border-radius:.65rem}.channels2-date-current{grid-column:2;box-sizing:border-box;width:100%;max-width:100%;min-width:0;border:1px solid rgba(96,165,250,.72);background:#2563eb;color:#fff;font:inherit;font-weight:800;text-align:center;color-scheme:dark}.channels2-date-current::-webkit-calendar-picker-indicator{filter:invert(1);opacity:.9}.channels2-date-today{grid-column:1/-1;width:100%;border:1px solid rgba(96,165,250,.6)!important;background:transparent!important;color:#bfdbfe!important}.channels2-day-heading{font-size:.95rem;color:#cbd5e1}.channels2-events{display:grid;gap:.4rem}.channels2-event-block{display:grid;gap:.35rem}.channels2-event{display:grid;grid-template-columns:5.2rem minmax(0,1fr) auto;gap:.6rem;align-items:center;width:100%;padding:.68rem .72rem;border:1px solid rgba(148,163,184,.18);border-radius:.75rem;background:rgba(15,23,42,.72);color:#f8fafc;text-align:left}.channels2-event.active,.channels2-event:hover{border-color:rgba(56,189,248,.6);background:rgba(14,165,233,.12)}.channels2-time{font-weight:800;color:#bae6fd}.channels2-badge{padding:.18rem .4rem;border:1px solid rgba(148,163,184,.28);border-radius:999px;color:#cbd5e1;font-size:.7rem;font-weight:800}.channels2-detail{display:grid;grid-template-columns:minmax(0,1fr);gap:.7rem;margin:0 .15rem .35rem;padding:.8rem;border-left:3px solid rgba(56,189,248,.65);border-radius:.65rem;background:rgba(2,6,23,.92)}.channels2-artwork,.channels2-detail>.epg-detail-artwork{display:none;min-height:9rem;border-radius:.65rem;background:linear-gradient(135deg,rgba(30,64,175,.35),rgba(15,23,42,.95));background-size:cover;background-position:center}.channels2-detail.has-artwork,.channels2-detail.epg-has-artwork{grid-template-columns:minmax(10rem,16rem) minmax(0,1fr)}.channels2-detail.has-artwork>.channels2-artwork,.channels2-detail.epg-has-artwork>.epg-detail-artwork{display:block}.channels2-detail>.epg-metadata-tabs,.channels2-detail>.epg-metadata-panel,.channels2-detail>.epg-metadata-status,.channels2-detail>.channels2-actions,.channels2-detail>.channels2-feedback{grid-column:1/-1}.channels2-detail-copy{display:grid;gap:.55rem}.channels2-detail-time{color:#93c5fd;font-weight:700}.channels2-description{color:#dbe4f0;line-height:1.48;white-space:pre-line}.channels2-actions{display:flex;flex-wrap:wrap;gap:.45rem}.channels2-actions button{min-height:2.55rem;padding:.52rem .8rem;border-radius:.68rem}.channels2-secondary-action{background:transparent!important;border:1px solid rgba(96,165,250,.6)!important;color:#bfdbfe!important}.channels2-feedback{min-height:1.2rem;color:#cbd5e1}.channels2-feedback.error{color:#fecaca}.channels2-feedback.success{color:#bbf7d0}.channels2-empty{padding:.9rem;border:1px dashed rgba(148,163,184,.3);border-radius:.75rem;color:#94a3b8;text-align:center}
@media(max-width:720px){.channels2-toolbar{align-items:stretch;flex-direction:column}.channels2-tools{display:grid;grid-template-columns:1fr}.channels2-filters{grid-template-columns:1fr}.channels2-filter-buttons{flex-wrap:nowrap;overflow-x:auto;padding-bottom:.15rem}.channels2-filter-button{flex:0 0 auto}.channels2-group-channels{padding:.3rem}.channels2-inline-program{margin-inline:0;padding:.55rem}.channels2-program-head{align-items:stretch;flex-direction:column}.channels2-date{min-width:0;grid-template-columns:auto minmax(0,1fr) auto}.channels2-event{grid-template-columns:4.8rem minmax(0,1fr)}.channels2-event .channels2-badge{display:none}.channels2-detail.has-artwork,.channels2-detail.epg-has-artwork{grid-template-columns:1fr}.channels2-artwork,.channels2-detail>.epg-detail-artwork{min-height:10rem}.channels2-actions{display:grid;grid-template-columns:1fr 1fr}.channels2-actions button{width:100%;font-size:.9rem}}
  `;

  const state = {
    active: false,
    channels: [],
    visible: [],
    channel: null,
    day: new Date(),
    events: [],
    event: null,
    query: '',
    typeFilter: 'all',
    accessFilter: 'all',
    statusFilter: 'all',
    encryptionAvailable: false,
    sequence: 0,
    openGroups: Object.create(null)
  };

  const platform = () => global.VdrSuitePlatform || null;
  const api = () => platform() && platform().getClientApi ? platform().getClientApi() : global.VdrSuiteClientApi;
  const mount = () => platform() && platform().getMountTarget ? (platform().getMountTarget('channels2') || platform().getMountTarget('channels') || platform().getMountTarget('detail')) : document.getElementById('detail-data');
  const backendId = () => String(platform() && platform().getSelectedBackendId ? platform().getSelectedBackendId() : 'default') || 'default';
  const pick = (object, keys, fallback = '') => {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') return object[key];
    }
    return fallback;
  };
  const text = value => String(value == null ? '' : value).trim();
  const list = (data, key) => Array.isArray(data) ? data : data && Array.isArray(data[key]) ? data[key] : data && Array.isArray(data.items) ? data.items : data && Array.isArray(data.results) ? data.results : [];
  const epoch = value => {
    const number = Number(value);
    if (Number.isFinite(number) && number > 0) return number > 1e11 ? Math.floor(number / 1000) : Math.floor(number);
    const parsed = Date.parse(String(value || ''));
    return Number.isFinite(parsed) ? Math.floor(parsed / 1000) : 0;
  };
  const channelId = channel => text(pick(channel, ['id', 'channelId', 'nativeId']));
  const channelName = channel => text(pick(channel, ['name', 'channelName', 'title', 'displayName'], 'Kanal'));
  const channelNumber = channel => Number(pick(channel, ['number', 'channelNumber', 'position'], 0)) || 0;
  const channelGroup = channel => text(pick(channel, ['group', 'groupName', 'channelGroup', 'bouquet', 'category', 'provider', 'section'], 'Weitere Sender'));
  const eventTitle = event => text(pick(event, ['title', 'name', 'eventTitle'], 'Sendung'));
  const eventSubtitle = event => text(pick(event, ['subtitle', 'shortText', 'short_text']));
  const eventDescription = event => text(pick(event, ['description', 'longText', 'long_text', 'summary']));
  const eventChannelId = event => text(pick(event, ['channelId', 'channel', 'channel_id']));
  const eventStart = event => epoch(pick(event, ['startTime', 'start', 'beginTime'], 0));
  const eventEnd = event => {
    const start = eventStart(event);
    const explicit = epoch(pick(event, ['endTime', 'end', 'stopTime'], 0));
    const duration = Number(pick(event, ['durationSeconds', 'duration'], 0));
    return explicit > start ? explicit : start + (Number.isFinite(duration) && duration > 0 ? duration : 0);
  };

  function booleanValue(value, fallback) {
    if (value === true || value === 1 || value === '1') return true;
    if (value === false || value === 0 || value === '0') return false;
    const normalized = text(value).toLocaleLowerCase('de-DE');
    if (normalized === 'true' || normalized === 'yes' || normalized === 'ja' || normalized === 'on') return true;
    if (normalized === 'false' || normalized === 'no' || normalized === 'nein' || normalized === 'off') return false;
    return fallback;
  }

  function channelBoolean(channel, keys, fallback) {
    for (const key of keys) {
      if (channel && Object.prototype.hasOwnProperty.call(channel, key)) return booleanValue(channel[key], fallback);
    }
    return fallback;
  }

  function channelHasEncryptionInfo(channel) {
    if (!channel) return false;
    if (['encrypted', 'scrambled', 'isEncrypted', 'isScrambled'].some(key => Object.prototype.hasOwnProperty.call(channel, key))) return true;
    const caids = pick(channel, ['caids', 'CAIDs', 'caid', 'CAID'], '');
    return Array.isArray(caids) ? caids.length > 0 : text(caids) !== '';
  }

  const channelIsRadio = channel => channelBoolean(channel, ['radio', 'isRadio'], false);
  const channelIsEncrypted = channel => {
    const explicit = channelBoolean(channel, ['encrypted', 'scrambled', 'isEncrypted', 'isScrambled'], false);
    if (explicit) return true;
    const caids = pick(channel, ['caids', 'CAIDs', 'caid', 'CAID'], '');
    return Array.isArray(caids) ? caids.length > 0 : text(caids) !== '';
  };
  const channelIsEnabled = channel => channelBoolean(channel, ['enabled', 'active'], true);

  function resolvePublicArtworkUrl(value) {
    const url = text(value);
    if (!url || !url.startsWith('/api/epg/cache/')) return url;

    const publicUrl = global.VdrSuitePublicUrl;
    if (!publicUrl || typeof publicUrl.resolvePath !== 'function') return url;

    try {
      return publicUrl.resolvePath(url);
    } catch (error) {
      return '';
    }
  }

  function eventArtwork(event) {
    const artwork = event && event.artwork;
    if (artwork && artwork.available === true) {
      const url = text(artwork.url);
      if (url) return resolvePublicArtworkUrl(url);
    }
    return resolvePublicArtworkUrl(text(pick(event, ['bannerUrl', 'imageUrl', 'posterUrl', 'artworkUrl', 'image', 'poster', 'banner'])));
  }

  const addText = (element, value) => { element.textContent = String(value); return element; };
  const dayOnly = value => { const date = value instanceof Date ? value : new Date(value || Date.now()); return new Date(date.getFullYear(), date.getMonth(), date.getDate()); };
  const moveDay = (value, days) => { const date = dayOnly(value); date.setDate(date.getDate() + days); return date; };
  const dateValue = value => { const date = dayOnly(value); return `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${String(date.getDate()).padStart(2, '0')}`; };
  const sameDay = (left, right) => dateValue(left) === dateValue(right);
  const clock = value => new Date(Number(value) * 1000).toLocaleTimeString('de-DE', {hour: '2-digit', minute: '2-digit'});
  const hhmm = value => { const date = new Date(Number(value) * 1000); return date.getHours() * 100 + date.getMinutes(); };

  function visibleEventsForDay(events, selectedDay, nowSeconds) {
    const ordered = events.slice().sort((left, right) => eventStart(left) - eventStart(right));
    const parsedNow = Number(nowSeconds);
    const now = Number.isFinite(parsedNow) ? Math.floor(parsedNow) : Math.floor(Date.now() / 1000);
    if (dateValue(selectedDay) !== dateValue(new Date(now * 1000))) return ordered;
    return ordered.filter(event => eventEnd(event) > now);
  }

  function adoptCanonicalChannelNavigation() {
    const canonicalTab = document.querySelector('[data-module="channels"]');
    const replacementTab = document.querySelector('[data-module="channels2"]');
    if (!replacementTab) return;
    if (canonicalTab && canonicalTab !== replacementTab && canonicalTab.parentNode) canonicalTab.parentNode.removeChild(canonicalTab);
    replacementTab.textContent = 'Kanäle';
    replacementTab.setAttribute('data-i18n', 'module.channels');
    replacementTab.setAttribute('aria-label', 'Kanäle');
  }

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
      const group = channelGroup(channel);
      if (!groups.has(group)) groups.set(group, []);
      groups.get(group).push(channel);
    });
    return Array.from(groups.entries()).sort((left, right) => channelNumber(left[1][0]) - channelNumber(right[1][0]));
  }

  function channelMatchesFilters(channel) {
    const query = state.query.toLocaleLowerCase('de-DE');
    const matchesQuery = !query || channelName(channel).toLocaleLowerCase('de-DE').includes(query) || channelGroup(channel).toLocaleLowerCase('de-DE').includes(query) || String(channelNumber(channel)).includes(query);
    if (!matchesQuery) return false;
    if (state.typeFilter === 'tv' && channelIsRadio(channel)) return false;
    if (state.typeFilter === 'radio' && !channelIsRadio(channel)) return false;
    if (state.encryptionAvailable && state.accessFilter === 'free' && channelIsEncrypted(channel)) return false;
    if (state.encryptionAvailable && state.accessFilter === 'encrypted' && !channelIsEncrypted(channel)) return false;
    if (state.statusFilter === 'enabled' && !channelIsEnabled(channel)) return false;
    if (state.statusFilter === 'disabled' && channelIsEnabled(channel)) return false;
    return true;
  }

  function filterChannels() {
    state.visible = state.channels.filter(channelMatchesFilters);
    if (state.channel && !state.visible.some(channel => channelId(channel) === channelId(state.channel))) {
      state.channel = null;
      state.event = null;
      state.events = [];
    }
  }

  function setChannels(data) {
    state.channels = list(data, 'channels').map(channel => Object.assign({}, channel, {
      id: channelId(channel),
      name: channelName(channel),
      number: channelNumber(channel),
      group: channelGroup(channel)
    })).sort((left, right) => channelNumber(left) - channelNumber(right) || channelName(left).localeCompare(channelName(right), 'de-DE'));
    state.encryptionAvailable = state.channels.some(channelHasEncryptionInfo);
    if (!state.encryptionAvailable) state.accessFilter = 'all';
    filterChannels();
    render();
  }

  function channelStatus(channel) {
    const parts = [`Kanal ${channelNumber(channel) || '-'}`, channelIsRadio(channel) ? 'Radio' : 'TV'];
    if (state.encryptionAvailable) parts.push(channelIsEncrypted(channel) ? 'verschlüsselt' : 'frei');
    parts.push(channelIsEnabled(channel) ? 'aktiv' : 'deaktiviert');
    return parts.join(' · ');
  }

  function currentEvent(channel) {
    return channel.currentEvent || channel.now || channel.currentProgram || null;
  }

  function timerPayload(event, channel) {
    const start = eventStart(event);
    const end = eventEnd(event);
    const id = text(pick(event, ['eventId', 'id', 'nativeId']));
    return {backendId: backendId(), channelId: eventChannelId(event) || channelId(channel), title: eventTitle(event), directory: '', day: dateValue(new Date(start * 1000)), weekdays: '-------', start: hhmm(start), stop: hhmm(end), priority: 50, lifetime: 99, active: true, vps: false, aux: id ? `eventId=${id}` : ''};
  }

  function createTimer(event, channel, feedback, button) {
    const client = api();
    if (!client || typeof client.fetchClientTimerCreateAction !== 'function') {
      feedback.className = 'channels2-feedback error';
      feedback.textContent = 'Timer-API ist nicht verfügbar.';
      return;
    }
    button.disabled = true;
    feedback.textContent = 'Timer wird erstellt …';
    client.fetchClientTimerCreateAction({payload: timerPayload(event, channel), cache: 'no-store', credentials: 'same-origin'}).then(result => {
      if (result && result.success === false) throw new Error(result.message || result.error || 'Aktion wurde abgelehnt.');
      button.textContent = 'Timer erstellt';
      feedback.className = 'channels2-feedback success';
      feedback.textContent = 'Timer wurde erstellt.';
    }).catch(error => {
      button.disabled = false;
      feedback.className = 'channels2-feedback error';
      feedback.textContent = error.message;
    });
  }

  function setFormValue(form, name, value, eventName) {
    const field = form.elements[name];
    if (!field) return false;
    if (field.type === 'checkbox') field.checked = Boolean(value); else field.value = String(value == null ? '' : value);
    field.dispatchEvent(new Event(eventName || 'input', {bubbles: true}));
    return true;
  }

  function waitFor(find, timeoutMs) {
    return new Promise((resolve, reject) => {
      const started = Date.now();
      const poll = () => {
        let value = null;
        try { value = find(); } catch (error) { reject(error); return; }
        if (value) { resolve(value); return; }
        if (Date.now() - started >= timeoutMs) { reject(new Error('SearchTimer-Editor konnte nicht rechtzeitig geöffnet werden.')); return; }
        global.setTimeout(poll, 100);
      };
      poll();
    });
  }

  const optionExists = (select, value) => Boolean(select && Array.from(select.options || []).some(option => option.value === value));

  function prepareSearchTimer(event, channel, detail) {
    const feedback = detail.querySelector('.channels2-feedback');
    const title = eventTitle(event);
    const selectedChannelId = eventChannelId(event) || channelId(channel);
    const selectedGroup = channelGroup(channel);
    const navigation = document.querySelector('[data-module="searchtimers"]');
    if (!navigation || typeof navigation.click !== 'function') {
      feedback.className = 'channels2-feedback error';
      feedback.textContent = 'SearchTimer-Bereich ist nicht verfügbar.';
      return;
    }
    feedback.className = 'channels2-feedback';
    feedback.textContent = 'SearchTimer-Editor wird geöffnet …';
    navigation.click();
    waitFor(() => document.querySelector('form[data-searchtimer-editor-form="create"]'), 10000).then(form => {
      const panel = form.closest('.searchtimer-create-panel');
      if (panel) panel.open = true;
      setFormValue(form, 'name', title);
      setFormValue(form, 'query', title);
      setFormValue(form, 'active', true, 'change');
      setFormValue(form, 'compareTitle', true, 'change');
      setFormValue(form, 'compareSubtitle', false, 'change');
      setFormValue(form, 'compareSummary', false, 'change');
      setFormValue(form, 'avoidRepeats', true, 'change');
      setFormValue(form, 'channelFilterMode', 1, 'change');
      setFormValue(form, 'manualUseChannel', 1);
      setFormValue(form, 'manualChannelMin', selectedChannelId);
      setFormValue(form, 'manualChannelMax', selectedChannelId);
      return waitFor(() => {
        const group = form.elements.channelSelectorGroup;
        return group && !group.disabled && group.options.length > 0 ? group : null;
      }, 5000).then(group => {
        if (!selectedGroup || !optionExists(group, selectedGroup)) return null;
        setFormValue(form, 'channelSelectorGroup', selectedGroup, 'change');
        return waitFor(() => {
          const select = form.elements.channelId;
          return select && !select.disabled && optionExists(select, selectedChannelId) ? select : null;
        }, 5000).then(() => setFormValue(form, 'channelId', selectedChannelId, 'change'));
      });
    }).catch(() => null).then(() => waitFor(() => document.querySelector('form[data-searchtimer-editor-form="create"]'), 1000)).then(form => {
      form.scrollIntoView({behavior: 'smooth', block: 'start'});
      const first = form.elements.query || form.elements.name;
      if (first && typeof first.focus === 'function') first.focus();
    }).catch(error => global.alert(String(error && error.message ? error.message : error)));
  }

  function enhanceEventDetail(detail, event, channel, feedback) {
    const applyEnhancer = () => {
      const metadata = global.VdrSuiteEpgMetadataDetail;
      if (!metadata || typeof metadata.enhance !== 'function') return false;
      metadata.enhance(detail, event, channel);
      return true;
    };

    if (applyEnhancer()) return;

    const runtimes = global.VdrSuiteDeferredFrontendRuntimes;
    if (!runtimes || typeof runtimes.loadEpgDetail !== 'function') {
      feedback.className = 'channels2-feedback error';
      feedback.textContent = 'Erweiterte EPG-Details sind nicht verfügbar.';
      return;
    }

    runtimes.loadEpgDetail().then(() => {
      if (!detail.parentNode || detail.dataset.epgMetadataDetail === 'true') return;
      if (!applyEnhancer()) throw new Error('EPG-Metadaten-Runtime wurde nicht registriert.');
    }).catch(error => {
      if (!detail.parentNode) return;
      feedback.className = 'channels2-feedback error';
      feedback.textContent = `Erweiterte EPG-Details konnten nicht geladen werden: ${error && error.message ? error.message : error}`;
    });
  }

  function renderEventDetail(event, channel) {
    const detail = document.createElement('article');
    detail.className = 'channels2-detail';
    const artwork = eventArtwork(event);
    const copy = document.createElement('div');
    copy.className = 'channels2-detail-copy epg-detail-hero';
    copy.append(addText(document.createElement('h3'), eventTitle(event)));
    if (eventSubtitle(event)) {
      const subtitle = addText(document.createElement('p'), eventSubtitle(event));
      subtitle.className = 'channels2-meta';
      copy.appendChild(subtitle);
    }
    const meta = addText(document.createElement('p'), `${clock(eventStart(event))}–${clock(eventEnd(event))} · ${channelName(channel)}`);
    meta.className = 'channels2-detail-time';
    copy.appendChild(meta);
    const description = addText(document.createElement('p'), eventDescription(event) || 'Keine Beschreibung vorhanden.');
    description.className = 'channels2-description epg-detail-description';
    copy.appendChild(description);
    const actions = document.createElement('div');
    actions.className = 'channels2-actions epg-detail-actions';
    const timer = addText(document.createElement('button'), 'Timer erstellen');
    timer.type = 'button';
    const series = addText(document.createElement('button'), 'Serientimer vorbereiten');
    series.type = 'button';
    series.className = 'channels2-secondary-action';
    const feedback = document.createElement('p');
    feedback.className = 'channels2-feedback';
    feedback.setAttribute('role', 'status');
    timer.onclick = () => createTimer(event, channel, feedback, timer);
    series.onclick = () => prepareSearchTimer(event, channel, detail);
    actions.append(timer, series);
    if (artwork) {
      const art = document.createElement('div');
      art.className = 'channels2-artwork epg-detail-artwork';
      art.setAttribute('role', 'img');
      art.setAttribute('aria-label', `Bild zu ${eventTitle(event)}`);
      art.style.backgroundImage = `url("${artwork.replace(/"/g, '%22')}")`;
      detail.classList.add('has-artwork');
      detail.appendChild(art);
    }
    detail.append(copy, actions, feedback);
    enhanceEventDetail(detail, event, channel, feedback);
    return detail;
  }

  function renderInlineProgram(channel) {
    const section = document.createElement('section');
    section.className = 'channels2-inline-program';
    const head = document.createElement('div');
    head.className = 'channels2-program-head';
    const title = document.createElement('div');
    title.append(addText(document.createElement('h3'), channelName(channel)), addText(document.createElement('p'), `${channelGroup(channel)} · ${channelStatus(channel)}`));
    const controls = document.createElement('div');
    controls.className = 'channels2-date';
    const selectedDay = dayOnly(state.day);
    const currentDay = dayOnly(new Date());
    const selectedDayLabel = selectedDay.toLocaleDateString('de-DE', {day: '2-digit', month: '2-digit', year: 'numeric'});
    const prev = addText(document.createElement('button'), '←');
    const input = document.createElement('input');
    const next = addText(document.createElement('button'), '→');
    prev.type = next.type = 'button';
    prev.setAttribute('aria-label', 'Vorheriger Tag');
    input.type = 'date';
    input.className = 'channels2-date-current';
    input.value = dateValue(selectedDay);
    input.setAttribute('aria-label', `Datum auswählen, aktuell ${selectedDayLabel}`);
    next.setAttribute('aria-label', 'Nächster Tag');
    prev.onclick = () => { state.day = moveDay(state.day, -1); loadEvents(); };
    input.onchange = () => { state.day = dayOnly(input.value); loadEvents(); };
    next.onclick = () => { state.day = moveDay(state.day, 1); loadEvents(); };
    controls.append(prev, input, next);
    if (!sameDay(selectedDay, currentDay)) {
      const today = addText(document.createElement('button'), 'Programm heute');
      today.type = 'button';
      today.className = 'channels2-date-today';
      today.onclick = () => { state.day = currentDay; loadEvents(); };
      controls.appendChild(today);
    }
    head.append(title, controls);
    section.appendChild(head);
    const day = addText(document.createElement('p'), selectedDay.toLocaleDateString('de-DE', {weekday: 'long', day: '2-digit', month: 'long', year: 'numeric'}));
    day.className = 'channels2-day-heading';
    section.appendChild(day);
    const events = document.createElement('div');
    events.className = 'channels2-events';
    const now = Math.floor(Date.now() / 1000);
    if (!state.events.length) {
      const empty = addText(document.createElement('div'), 'Für diesen Tag wurden keine EPG-Einträge gefunden.');
      empty.className = 'channels2-empty';
      events.appendChild(empty);
    }
    state.events.forEach(event => {
      const block = document.createElement('div');
      block.className = 'channels2-event-block';
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'channels2-event';
      if (state.event === event) button.classList.add('active');
      const time = addText(document.createElement('span'), `${clock(eventStart(event))}–${clock(eventEnd(event))}`);
      time.className = 'channels2-time';
      const textBox = document.createElement('span');
      const titleText = addText(document.createElement('span'), eventTitle(event));
      titleText.className = 'channels2-title';
      textBox.appendChild(titleText);
      if (eventSubtitle(event)) {
        const sub = addText(document.createElement('span'), eventSubtitle(event));
        sub.className = 'channels2-meta';
        textBox.appendChild(sub);
      }
      const badge = addText(document.createElement('span'), eventStart(event) <= now && eventEnd(event) > now ? 'Jetzt' : 'EPG');
      badge.className = 'channels2-badge';
      button.append(time, textBox, badge);
      button.onclick = () => { state.event = state.event === event ? null : event; render(); };
      block.appendChild(button);
      if (state.event === event) block.appendChild(renderEventDetail(event, channel));
      events.appendChild(block);
    });
    section.appendChild(events);
    return section;
  }

  function filterButton(value, label, activeValue, setter) {
    const button = addText(document.createElement('button'), label);
    button.type = 'button';
    button.className = 'channels2-filter-button';
    if (value === activeValue) button.classList.add('active');
    button.setAttribute('aria-pressed', value === activeValue ? 'true' : 'false');
    button.onclick = () => { setter(value); filterChannels(); render(); };
    return button;
  }

  function renderFilterGroup(label, options, activeValue, setter) {
    const group = document.createElement('section');
    group.className = 'channels2-filter-group';
    const heading = addText(document.createElement('span'), label);
    heading.className = 'channels2-filter-label';
    const buttons = document.createElement('div');
    buttons.className = 'channels2-filter-buttons';
    options.forEach(option => buttons.appendChild(filterButton(option[0], option[1], activeValue, setter)));
    group.append(heading, buttons);
    return group;
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
    intro.append(addText(document.createElement('h3'), 'Kanäle'), addText(document.createElement('p'), 'Sender, Filter, Tagesprogramm und EPG direkt in einer Ansicht.'));
    const tools = document.createElement('div');
    tools.className = 'channels2-tools';
    const search = document.createElement('input');
    search.type = 'search';
    search.className = 'channels2-search';
    search.placeholder = 'Kanal, Nummer oder Gruppe suchen';
    search.value = state.query;
    const reload = addText(document.createElement('button'), 'Neu laden');
    reload.type = 'button';
    tools.append(search, reload);
    toolbar.append(intro, tools);
    root.appendChild(toolbar);

    const filters = document.createElement('section');
    filters.className = 'channels2-filters';
    filters.appendChild(renderFilterGroup('Typ', [['all', 'Alle'], ['tv', 'TV'], ['radio', 'Radio']], state.typeFilter, value => { state.typeFilter = value; }));
    if (state.encryptionAvailable) filters.appendChild(renderFilterGroup('Empfang', [['all', 'Alle'], ['free', 'Frei'], ['encrypted', 'Verschlüsselt']], state.accessFilter, value => { state.accessFilter = value; }));
    filters.appendChild(renderFilterGroup('Status', [['all', 'Alle'], ['enabled', 'Aktiv'], ['disabled', 'Deaktiviert']], state.statusFilter, value => { state.statusFilter = value; }));
    root.appendChild(filters);

    const status = addText(document.createElement('p'), `${state.visible.length} von ${state.channels.length} Kanälen · ${groupChannels(state.visible).length} Gruppen`);
    status.className = 'channels2-status';
    root.appendChild(status);
    const channelList = document.createElement('section');
    channelList.className = 'channels2-list';
    root.appendChild(channelList);
    target.appendChild(root);

    const groups = groupChannels(state.visible);
    if (!groups.length) {
      const empty = addText(document.createElement('div'), 'Keine passenden Kanäle gefunden.');
      empty.className = 'channels2-empty';
      channelList.appendChild(empty);
    }
    groups.forEach(([groupName, channels], index) => {
      const details = document.createElement('details');
      details.className = 'channels2-group';
      const selected = state.channel && channels.some(channel => channelId(channel) === channelId(state.channel));
      details.open = Boolean(state.query) || selected || state.openGroups[groupName] === true || (index === 0 && Object.keys(state.openGroups).length === 0);
      const summary = document.createElement('summary');
      summary.append(addText(document.createElement('span'), groupName));
      const count = addText(document.createElement('span'), String(channels.length));
      count.className = 'channels2-group-count';
      summary.appendChild(count);
      details.appendChild(summary);
      details.ontoggle = () => { state.openGroups[groupName] = details.open; };
      const body = document.createElement('div');
      body.className = 'channels2-group-channels';
      channels.forEach(channel => {
        const block = document.createElement('div');
        block.className = 'channels2-channel-block';
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'channels2-channel';
        if (state.channel && channelId(state.channel) === channelId(channel)) button.classList.add('active');
        const logo = document.createElement('span');
        logo.className = 'channels2-logo';
        if (typeof global.createChannelLogoElement === 'function') logo.appendChild(global.createChannelLogoElement(channelName(channel), channelId(channel))); else logo.textContent = String(channelNumber(channel) || '•');
        const copy = document.createElement('span');
        const title = addText(document.createElement('span'), channelName(channel));
        title.className = 'channels2-title';
        const meta = addText(document.createElement('span'), channelStatus(channel));
        meta.className = 'channels2-meta';
        copy.append(title, meta);
        const nowEvent = currentEvent(channel);
        if (nowEvent) {
          const nowText = addText(document.createElement('span'), `Jetzt: ${eventTitle(nowEvent)}${eventStart(nowEvent) ? ` · ${clock(eventStart(nowEvent))}–${clock(eventEnd(nowEvent))}` : ''}`);
          nowText.className = 'channels2-now';
          copy.appendChild(nowText);
        }
        button.append(logo, copy);
        button.onclick = () => {
          if (state.channel && channelId(state.channel) === channelId(channel)) {
            state.channel = null;
            state.event = null;
            state.events = [];
            render();
            return;
          }
          state.channel = channel;
          state.event = null;
          state.openGroups[groupName] = true;
          render();
          loadEvents();
        };
        block.appendChild(button);
        if (state.channel && channelId(state.channel) === channelId(channel)) block.appendChild(renderInlineProgram(channel));
        body.appendChild(block);
      });
      details.appendChild(body);
      channelList.appendChild(details);
    });

    search.oninput = () => { state.query = search.value.trim(); filterChannels(); render(); };
    reload.onclick = loadChannels;
  }

  function loadEvents() {
    const client = api();
    if (!client || !state.channel) { render(); return; }
    const start = dayOnly(state.day);
    const end = moveDay(start, 1);
    const range = {start: Math.floor(start.getTime() / 1000), end: Math.floor(end.getTime() / 1000)};
    const sequence = ++state.sequence;
    const cached = typeof client.fetchClientEpgCacheWindow === 'function' ? client.fetchClientEpgCacheWindow({query: {backend: backendId(), channelId: channelId(state.channel), fromTime: String(range.start), untilTime: String(range.end), limit: '0', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'}).catch(() => ({events: []})) : Promise.resolve({events: []});
    cached.then(data => list(data, 'events').length || typeof client.fetchClientEpgChannelWindow !== 'function' ? data : client.fetchClientEpgChannelWindow({query: {channelId: channelId(state.channel), from: String(range.start), timespan: String(range.end - range.start), limit: '192', _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'})).then(data => {
      if (!state.active || sequence !== state.sequence) return;
      const dayEvents = list(data, 'events').filter(event => {
        const id = eventChannelId(event);
        return (!id || id === channelId(state.channel)) && eventStart(event) < range.end && eventEnd(event) > range.start;
      });
      state.events = visibleEventsForDay(dayEvents, state.day);
      state.event = null;
      render();
    }).catch(error => {
      const target = mount();
      if (target && state.active) {
        const message = addText(document.createElement('p'), `Tagesprogramm konnte nicht geladen werden: ${error.message}`);
        message.className = 'channels2-status error';
        target.prepend(message);
      }
    });
  }

  function loadChannels() {
    const client = api();
    const target = mount();
    if (!target) return;
    target.classList.add('channels2-mount');
    target.replaceChildren();
    const loading = addText(document.createElement('p'), 'Lade Kanäle …');
    loading.className = 'channels2-status';
    target.appendChild(loading);
    if (!client || typeof client.fetchClientChannels !== 'function') {
      loading.className = 'channels2-status error';
      loading.textContent = 'Kanäle konnten nicht geladen werden: Client API ist nicht verfügbar.';
      return;
    }
    client.fetchClientChannels({query: {backend: backendId(), _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'}).then(data => {
      if (!state.active) return;
      setChannels(data);
    }).catch(error => {
      if (state.active) {
        loading.className = 'channels2-status error';
        loading.textContent = `Kanäle konnten nicht geladen werden: ${error.message}`;
      }
    });
  }

  const moduleApi = Object.freeze({
    activate(data) {
      state.active = true;
      if (data && list(data, 'channels').length) setChannels(data); else loadChannels();
    },
    renderList(data) {
      state.active = true;
      setChannels(data || {channels: []});
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

  adoptCanonicalChannelNavigation();
  global.VdrSuiteChannels2 = moduleApi;
}(window));
