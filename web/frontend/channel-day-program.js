// Channel day programme navigation and EPG actions.
// Loaded after app.js so it can reuse the established EPG detail and Timer workflow.
(function(global) {
  'use strict';

  const state = {
    activeView: null,
    sourceShell: null,
    sourceButton: null,
    sourceWindowY: 0,
    sourcePaneScrollTop: 0,
    channel: null,
    selectedDate: null,
    loadSequence: 0
  };

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

  function selectedBackendId() {
    const runtime = platform();
    if (runtime && typeof runtime.getSelectedBackendId === 'function') {
      const value = String(runtime.getSelectedBackendId() || '').trim();
      if (value !== '') return value;
    }
    return 'default';
  }

  function parseEpoch(value) {
    if (value === undefined || value === null || value === '') return 0;
    const number = Number(value);
    if (Number.isFinite(number) && number > 0) {
      return number > 100000000000 ? Math.floor(number / 1000) : Math.floor(number);
    }
    const parsed = Date.parse(String(value));
    return Number.isFinite(parsed) ? Math.floor(parsed / 1000) : 0;
  }

  function eventEnd(event, start) {
    const explicit = parseEpoch(firstValue(event, ['endTime', 'end', 'stopTime'], ''));
    if (explicit > start) return explicit;
    const duration = Number(firstValue(event, ['durationSeconds', 'duration'], 0));
    return Number.isFinite(duration) && duration > 0 && start > 0 ? start + duration : 0;
  }

  function eventTitle(event) {
    return String(firstValue(event, ['title', 'name', 'eventTitle'], 'Sendung'));
  }

  function eventSubtitle(event) {
    return String(firstValue(event, ['subtitle', 'shortText', 'short_text'], ''));
  }

  function eventDescription(event) {
    return String(firstValue(event, ['description', 'longText', 'long_text', 'summary'], ''));
  }

  function eventChannelId(event) {
    return String(firstValue(event, ['channelId', 'channel', 'channel_id'], '')).trim();
  }

  function channelId(channel) {
    return String(firstValue(channel, ['id', 'channelId', 'nativeId'], '')).trim();
  }

  function channelName(channel) {
    return String(firstValue(channel, ['name', 'channelName', 'title', 'displayName'], 'Kanal'));
  }

  function channelNumber(channel) {
    return String(firstValue(channel, ['number', 'channelNumber', 'position'], ''));
  }

  function channelGroup(channel) {
    return String(firstValue(channel, ['group', 'groupName', 'channelGroup', 'bouquet'], '')).trim();
  }

  function normalizeChannel(source) {
    const channel = source && typeof source === 'object' ? source : {};
    return Object.assign({}, channel, {
      id: channelId(channel),
      name: channelName(channel),
      number: channelNumber(channel),
      group: channelGroup(channel)
    });
  }

  function parseChannelButtonMetadata(metaText, title) {
    const text = String(metaText || '').trim();
    const match = /^Nr\.\s*([^·]+)\s*·\s*(.+)$/.exec(text);
    return normalizeChannel({
      id: match ? match[2].trim() : '',
      number: match ? match[1].trim() : '',
      name: String(title || '').trim()
    });
  }

  function channelFromButton(button) {
    const titleElement = button ? button.querySelector('.list-title') : null;
    const metaElement = button ? button.querySelector('.channel-text .list-meta') : null;
    return parseChannelButtonMetadata(
      metaElement ? metaElement.textContent : '',
      titleElement ? titleElement.textContent : ''
    );
  }

  function dateFromInput(value) {
    if (value instanceof Date && !Number.isNaN(value.getTime())) {
      return new Date(value.getFullYear(), value.getMonth(), value.getDate());
    }
    const match = /^(\d{4})-(\d{2})-(\d{2})$/.exec(String(value || ''));
    if (match) {
      return new Date(Number(match[1]), Number(match[2]) - 1, Number(match[3]));
    }
    const parsed = new Date(value || Date.now());
    return Number.isNaN(parsed.getTime())
      ? new Date(new Date().getFullYear(), new Date().getMonth(), new Date().getDate())
      : new Date(parsed.getFullYear(), parsed.getMonth(), parsed.getDate());
  }

  function addDays(value, amount) {
    const date = dateFromInput(value);
    date.setDate(date.getDate() + Number(amount || 0));
    return date;
  }

  function dayBounds(value) {
    const startDate = dateFromInput(value);
    const endDate = addDays(startDate, 1);
    return {
      start: Math.floor(startDate.getTime() / 1000),
      end: Math.floor(endDate.getTime() / 1000)
    };
  }

  function dateInputValue(value) {
    const date = dateFromInput(value);
    return String(date.getFullYear()).padStart(4, '0') + '-' +
      String(date.getMonth() + 1).padStart(2, '0') + '-' +
      String(date.getDate()).padStart(2, '0');
  }

  function dateHeading(value) {
    return dateFromInput(value).toLocaleDateString('de-DE', {
      weekday: 'long',
      day: '2-digit',
      month: 'long',
      year: 'numeric'
    });
  }

  function clock(epoch) {
    return new Date(Number(epoch) * 1000).toLocaleTimeString('de-DE', {
      hour: '2-digit',
      minute: '2-digit'
    });
  }

  function hhmm(epoch) {
    const date = new Date(Number(epoch) * 1000);
    return date.getHours() * 100 + date.getMinutes();
  }

  function eventEntriesForDay(events, selectedChannelId, bounds) {
    return (Array.isArray(events) ? events : [])
      .map(event => {
        const start = parseEpoch(firstValue(event, ['startTime', 'start', 'beginTime'], ''));
        return {event, start, end: eventEnd(event, start)};
      })
      .filter(entry => {
        const entryChannelId = eventChannelId(entry.event);
        const channelMatches = entryChannelId === '' || selectedChannelId === '' || entryChannelId === selectedChannelId;
        return channelMatches && entry.start > 0 && entry.end > entry.start && entry.start < bounds.end && entry.end > bounds.start;
      })
      .sort((left, right) => left.start - right.start);
  }

  function timerDay(epoch) {
    return dateInputValue(new Date(Number(epoch) * 1000));
  }

  function buildTimerPayload(event, channel) {
    const start = parseEpoch(firstValue(event, ['startTime', 'start', 'beginTime'], ''));
    const end = eventEnd(event, start);
    const eventId = String(firstValue(event, ['eventId', 'id', 'nativeId'], ''));
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
      aux: eventId !== '' ? 'eventId=' + eventId : ''
    };
  }

  function buildSeriesSearchTimerPayload(event, channel) {
    const title = eventTitle(event);
    const id = eventChannelId(event) || channelId(channel);
    return {
      backendId: selectedBackendId(),
      name: title,
      query: title,
      active: true,
      directory: '',
      priority: 50,
      lifetime: 99,
      marginStartMinutes: 5,
      marginStopMinutes: 10,
      useVps: false,
      useChannel: id === '' ? 0 : 1,
      channels: '',
      channelMin: id,
      channelMax: id,
      useTime: false,
      startTime: 0,
      stopTime: 0,
      useDuration: false,
      durationMinMinutes: 0,
      durationMaxMinutes: 0,
      useDayOfWeek: false,
      dayOfWeek: 0,
      avoidRepeats: true,
      allowedRepeats: 0,
      repeatsWithinDays: 0,
      compareTitle: true,
      compareSubtitle: false,
      compareSummary: false,
      compareCategories: false,
      compareTime: false,
      useSeriesRecording: false,
      keepRecordings: 0,
      deleteMode: 0,
      searchTimerAction: 0,
      blacklistMode: 0,
      blacklistIds: '',
      mode: 0,
      matchCase: false,
      tolerance: 0,
      summaryMatch: 0,
      useExtendedEpgInfo: false,
      extendedEpgInfo: '',
      ignoreMissingEpgCategories: false,
      contentDescriptors: '',
      useInFavorites: false,
      activeFrom: '',
      activeUntil: '',
      pauseOnRecordings: false,
      switchMinutesBefore: 0,
      unmuteSoundOnSwitch: false,
      deleteRecordingsAfterDays: 0,
      deleteAfterCountRecordings: 0,
      deleteAfterDaysOfFirstRecording: 0
    };
  }

  function applySeriesScope(payload, channel, scope) {
    const result = Object.assign({}, payload);
    const id = channelId(channel);
    const group = channelGroup(channel);
    if (scope === 'all') {
      result.useChannel = 0;
      result.channels = '';
      result.channelMin = '';
      result.channelMax = '';
    } else if (scope === 'group' && group !== '') {
      result.useChannel = 2;
      result.channels = group;
      result.channelMin = '';
      result.channelMax = '';
    } else {
      result.useChannel = id === '' ? 0 : 1;
      result.channels = '';
      result.channelMin = id;
      result.channelMax = id;
    }
    return result;
  }

  function addText(element, text) {
    element.textContent = String(text);
    return element;
  }

  function installStyles() {
    if (document.getElementById('vdr-suite-channel-day-program-styles')) return;
    const style = document.createElement('style');
    style.id = 'vdr-suite-channel-day-program-styles';
    style.textContent = `
.channel-day-program-view{display:grid;gap:.85rem;min-width:0}.channel-day-toolbar{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:.65rem}.channel-day-back,.channel-day-date-controls button,.channel-day-event-back{min-height:2.55rem;padding:.55rem .8rem;border-radius:.7rem}.channel-day-channel-head{display:flex;align-items:center;gap:.85rem;padding:.8rem .9rem;border:1px solid rgba(56,189,248,.3);border-radius:.95rem;background:rgba(2,6,23,.72)}.channel-day-channel-head img,.channel-day-channel-head .channel-logo{width:5.2rem;max-height:3.5rem;object-fit:contain}.channel-day-channel-copy{min-width:0}.channel-day-channel-copy h3,.channel-day-channel-copy p{margin:0}.channel-day-channel-copy p{margin-top:.2rem;color:#94a3b8}.channel-day-date-controls{display:flex;flex-wrap:wrap;align-items:center;gap:.4rem}.channel-day-date-controls input{min-height:2.55rem;padding:.45rem .6rem;border:1px solid #475569;border-radius:.7rem;background:#111827;color:#f8fafc;font:inherit}.channel-day-heading{margin:0;color:#f8fafc}.channel-day-status{padding:.7rem .8rem;border:1px solid rgba(148,163,184,.2);border-radius:.75rem;background:rgba(15,23,42,.7);color:#cbd5e1}.channel-day-status.error{border-color:rgba(248,113,113,.45);color:#fecaca}.channel-day-list{display:grid;gap:.5rem}.channel-day-event{display:grid;grid-template-columns:6.4rem minmax(0,1fr) auto;gap:.65rem;align-items:center;width:100%;padding:.72rem .78rem;border:1px solid rgba(148,163,184,.2);border-radius:.82rem;background:rgba(15,23,42,.76);color:#f8fafc;text-align:left}.channel-day-event:hover,.channel-day-event:focus{border-color:rgba(56,189,248,.58);background:rgba(14,165,233,.12);outline:none}.channel-day-event.current{border-color:rgba(74,222,128,.55)}.channel-day-event-time{font-weight:850;color:#bae6fd}.channel-day-event-title{font-weight:850;overflow-wrap:anywhere}.channel-day-event-subtitle{margin-top:.14rem;color:#94a3b8;font-size:.86rem}.channel-day-event-badges{display:flex;flex-wrap:wrap;justify-content:flex-end;gap:.3rem}.channel-day-badge{padding:.2rem .42rem;border:1px solid rgba(148,163,184,.28);border-radius:999px;background:rgba(2,6,23,.7);color:#cbd5e1;font-size:.72rem;font-weight:800}.channel-day-badge.current{border-color:rgba(74,222,128,.5);color:#bbf7d0}.channel-day-progress{grid-column:1/-1;height:.24rem;border-radius:999px;background:rgba(148,163,184,.18);overflow:hidden}.channel-day-progress>span{display:block;height:100%;background:#38bdf8}.channel-day-event-detail{display:grid;gap:.65rem;padding:.7rem;border:1px solid rgba(56,189,248,.34);border-radius:.9rem;background:rgba(2,6,23,.72)}.channel-day-series-panel{display:grid;gap:.62rem;padding:.72rem;border:1px solid rgba(56,189,248,.26);border-radius:.8rem;background:rgba(15,23,42,.8)}.channel-day-series-panel h4,.channel-day-series-panel p{margin:0}.channel-day-series-grid{display:grid;grid-template-columns:minmax(0,1fr) auto auto;gap:.5rem}.channel-day-series-grid select,.channel-day-series-grid button{min-height:2.55rem;padding:.5rem .65rem;border-radius:.65rem}.channel-day-series-feedback{min-height:1.25rem;color:#cbd5e1}.channel-day-series-feedback.error{color:#fecaca}.channel-day-preview-matches{display:grid;gap:.35rem}.channel-day-preview-match{padding:.45rem .55rem;border:1px solid rgba(148,163,184,.16);border-radius:.6rem;background:rgba(2,6,23,.48)}
@media(max-width:760px){.channel-day-toolbar{align-items:stretch}.channel-day-back{width:100%}.channel-day-channel-head{align-items:flex-start}.channel-day-date-controls{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));width:100%}.channel-day-date-controls input{grid-column:1/-1;width:100%;box-sizing:border-box}.channel-day-date-controls button{width:100%}.channel-day-event{grid-template-columns:4.8rem minmax(0,1fr)}.channel-day-event-badges{grid-column:1/-1;justify-content:flex-start}.channel-day-series-grid{grid-template-columns:minmax(0,1fr)}.channel-day-series-grid button,.channel-day-series-grid select{width:100%}}`;
    document.head.appendChild(style);
  }

  function resolveChannel(buttonChannel) {
    const api = clientApi();
    if (!api || typeof api.fetchClientChannels !== 'function') {
      return Promise.resolve(buttonChannel);
    }
    return api.fetchClientChannels({
      query: {backend: selectedBackendId(), _: String(Date.now())},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(data => {
      const channels = listFromResponse(data, 'channels').map(normalizeChannel);
      return channels.find(channel => channel.id === buttonChannel.id) || buttonChannel;
    }).catch(() => buttonChannel);
  }

  function cachedDayRequest(channel, bounds) {
    const api = clientApi();
    if (!api || typeof api.fetchClientEpgCacheWindow !== 'function') {
      return Promise.resolve({events: []});
    }
    return api.fetchClientEpgCacheWindow({
      query: {
        backend: selectedBackendId(),
        channelId: channelId(channel),
        fromTime: String(bounds.start),
        untilTime: String(bounds.end),
        limit: '0',
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).catch(() => ({events: []}));
  }

  function liveDayRequest(channel, bounds) {
    const api = clientApi();
    if (!api || typeof api.fetchClientEpgChannelWindow !== 'function') {
      return Promise.resolve({events: []});
    }
    return api.fetchClientEpgChannelWindow({
      query: {
        channelId: channelId(channel),
        from: String(bounds.start),
        timespan: String(Math.max(3600, bounds.end - bounds.start)),
        limit: '192',
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).catch(() => ({events: []}));
  }

  function refreshDayCache(channel, bounds) {
    const api = clientApi();
    if (!api || typeof api.fetchClientEpgCacheRefresh !== 'function') {
      return Promise.resolve(null);
    }
    return api.fetchClientEpgCacheRefresh({
      query: {
        backend: selectedBackendId(),
        channelId: channelId(channel),
        from: String(bounds.start),
        timespan: String(Math.max(3600, bounds.end - bounds.start)),
        limit: '0',
        channelEventLimit: '192',
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).catch(() => null);
  }

  function loadDayEvents(channel, bounds) {
    return cachedDayRequest(channel, bounds).then(cached => {
      if (eventEntriesForDay(listFromResponse(cached, 'events'), channelId(channel), bounds).length > 0) {
        return cached;
      }
      return refreshDayCache(channel, bounds)
        .then(() => cachedDayRequest(channel, bounds))
        .then(refreshed => {
          if (eventEntriesForDay(listFromResponse(refreshed, 'events'), channelId(channel), bounds).length > 0) {
            return refreshed;
          }
          return liveDayRequest(channel, bounds);
        });
    });
  }

  function createChannelHeader(channel) {
    const head = document.createElement('article');
    head.className = 'channel-day-channel-head';
    if (typeof global.createChannelLogoElement === 'function') {
      head.appendChild(global.createChannelLogoElement(channelName(channel), channelId(channel)));
    }
    const copy = document.createElement('div');
    copy.className = 'channel-day-channel-copy';
    copy.appendChild(addText(document.createElement('h3'), channelName(channel)));
    const parts = [];
    if (channelNumber(channel) !== '') parts.push('Kanal ' + channelNumber(channel));
    if (channelGroup(channel) !== '') parts.push(channelGroup(channel));
    if (channelId(channel) !== '') parts.push(channelId(channel));
    copy.appendChild(addText(document.createElement('p'), parts.join(' · ')));
    head.appendChild(copy);
    return head;
  }

  function seriesPreviewMatches(data) {
    if (data && data.preview && Array.isArray(data.preview.matches)) return data.preview.matches;
    return listFromResponse(data, 'matches');
  }

  function setSeriesFeedback(target, message, error) {
    target.className = 'channel-day-series-feedback' + (error ? ' error' : '');
    target.textContent = String(message || '');
  }

  function openAdvancedSearchTimer(event, channel, scope) {
    const payload = applySeriesScope(buildSeriesSearchTimerPayload(event, channel), channel, scope);
    const tab = document.querySelector('[data-module="searchtimers"]');
    if (!tab || typeof tab.click !== 'function') return;
    closeProgramView(false);
    tab.click();

    let attempts = 0;
    const fill = () => {
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
      setValue('compareSubtitle', false);
      setValue('compareSummary', false);
      setValue('avoidRepeats', true);
      setValue('channelFilterMode', payload.useChannel, 'change');

      global.setTimeout(() => {
        if (payload.useChannel === 1) {
          setValue('channelSelectorGroup', channelGroup(channel), 'change');
          global.setTimeout(() => setValue('channelId', channelId(channel), 'change'), 80);
        } else if (payload.useChannel === 2) {
          setValue('channelFilterGroup', channelGroup(channel), 'change');
        }
        form.scrollIntoView({behavior: 'smooth', block: 'start'});
      }, 80);
    };
    global.setTimeout(fill, 80);
  }

  function createSeriesPanel(event, channel) {
    const panel = document.createElement('section');
    panel.className = 'channel-day-series-panel';
    panel.dataset.channelSeriesPanel = 'true';
    panel.appendChild(addText(document.createElement('h4'), 'Serie automatisch aufnehmen'));
    panel.appendChild(addText(
      document.createElement('p'),
      'Der Titel wird als SearchTimer vorbereitet. Vor dem Speichern wird die Trefferliste geprüft.'
    ));

    const controls = document.createElement('div');
    controls.className = 'channel-day-series-grid';
    const scope = document.createElement('select');
    scope.appendChild(new Option('Nur dieser Kanal', 'channel'));
    if (channelGroup(channel) !== '') scope.appendChild(new Option('Aktuelle Kanalgruppe', 'group'));
    scope.appendChild(new Option('Alle Kanäle', 'all'));

    const preview = addText(document.createElement('button'), 'Vorschau laden');
    preview.type = 'button';
    const create = addText(document.createElement('button'), 'SearchTimer erstellen');
    create.type = 'button';
    controls.appendChild(scope);
    controls.appendChild(preview);
    controls.appendChild(create);
    panel.appendChild(controls);

    const feedback = document.createElement('div');
    feedback.className = 'channel-day-series-feedback';
    feedback.setAttribute('role', 'status');
    panel.appendChild(feedback);

    const matches = document.createElement('div');
    matches.className = 'channel-day-preview-matches';
    panel.appendChild(matches);

    function payload() {
      return applySeriesScope(buildSeriesSearchTimerPayload(event, channel), channel, scope.value);
    }

    preview.addEventListener('click', () => {
      const api = clientApi();
      if (!api || typeof api.fetchClientSearchTimerPreview !== 'function') {
        setSeriesFeedback(feedback, 'SearchTimer-Vorschau ist nicht verfügbar.', true);
        return;
      }
      preview.disabled = true;
      matches.replaceChildren();
      setSeriesFeedback(feedback, 'Vorschau wird geladen …', false);
      const request = payload();
      api.fetchClientSearchTimerPreview({
        backendId: selectedBackendId(),
        query: Object.assign({}, request, {text: request.query}),
        cache: 'no-store',
        credentials: 'same-origin'
      }).then(data => {
        const entries = seriesPreviewMatches(data);
        const total = firstValue(data && data.statistics ? data.statistics : {}, ['totalCount'], entries.length);
        setSeriesFeedback(feedback, String(total) + ' Treffer in der Vorschau.', false);
        entries.slice(0, 8).forEach(match => {
          const candidate = match && match.event ? match.event : match;
          const item = document.createElement('div');
          item.className = 'channel-day-preview-match';
          item.textContent = eventTitle(candidate) + ' · ' + clock(parseEpoch(firstValue(candidate, ['startTime', 'start'], 0)));
          matches.appendChild(item);
        });
      }).catch(error => {
        setSeriesFeedback(feedback, String(error && error.message ? error.message : error), true);
      }).finally(() => {
        preview.disabled = false;
      });
    });

    create.addEventListener('click', () => {
      const request = payload();
      if (!global.confirm('SearchTimer „' + request.name + '“ wirklich erstellen?')) return;
      const api = clientApi();
      if (!api || typeof api.fetchClientSearchTimerCreateAction !== 'function') {
        setSeriesFeedback(feedback, 'SearchTimer-Erstellung ist nicht verfügbar.', true);
        return;
      }
      create.disabled = true;
      setSeriesFeedback(feedback, 'SearchTimer wird erstellt …', false);
      api.fetchClientSearchTimerCreateAction({
        payload: request,
        cache: 'no-store',
        credentials: 'same-origin'
      }).then(result => {
        if (!result || result.success !== true) {
          throw new Error(String(result && result.message ? result.message : 'Backend hat die Erstellung abgelehnt.'));
        }
        setSeriesFeedback(feedback, String(result.message || 'SearchTimer wurde erstellt.'), false);
        create.textContent = 'SearchTimer erstellt';
      }).catch(error => {
        setSeriesFeedback(feedback, String(error && error.message ? error.message : error), true);
        create.disabled = false;
      });
    });

    return panel;
  }

  function upgradeEpgActions(detail, event, channel) {
    const buttons = Array.from(detail.querySelectorAll('.epg-detail-action'));
    const searchButton = buttons.find(button => button.textContent.trim() === 'Suchtimer');
    const moreButton = buttons.find(button => button.textContent.trim() === 'Mehr …');

    if (searchButton) {
      searchButton.disabled = false;
      searchButton.textContent = 'Serie automatisch aufnehmen';
      searchButton.title = 'SearchTimer mit Vorschau aus dieser Sendung vorbereiten.';
      searchButton.addEventListener('click', () => {
        const existing = detail.querySelector('[data-channel-series-panel="true"]');
        if (existing) {
          existing.remove();
          return;
        }
        detail.appendChild(createSeriesPanel(event, channel));
      });
    }

    if (moreButton) {
      moreButton.disabled = false;
      moreButton.textContent = 'Erweiterter SearchTimer';
      moreButton.title = 'Vollständigen SearchTimer-Editor mit dieser Sendung öffnen.';
      moreButton.addEventListener('click', () => openAdvancedSearchTimer(event, channel, 'channel'));
    }
    return detail;
  }

  function fallbackEventDetail(event, channel) {
    const detail = document.createElement('article');
    detail.className = 'module-placeholder epg-event-detail';
    detail.appendChild(addText(document.createElement('h3'), eventTitle(event)));
    const subtitle = eventSubtitle(event);
    if (subtitle !== '') detail.appendChild(addText(document.createElement('p'), subtitle));
    const description = eventDescription(event);
    if (description !== '') detail.appendChild(addText(document.createElement('p'), description));
    const actions = document.createElement('div');
    actions.className = 'epg-detail-actions';
    const timer = addText(document.createElement('button'), 'Timer erstellen');
    timer.type = 'button';
    timer.addEventListener('click', () => {
      const api = clientApi();
      if (!api || typeof api.fetchClientTimerCreateAction !== 'function') return;
      if (!global.confirm('Timer für „' + eventTitle(event) + '“ erstellen?')) return;
      api.fetchClientTimerCreateAction({payload: buildTimerPayload(event, channel), cache: 'no-store'});
    });
    actions.appendChild(timer);
    const series = addText(document.createElement('button'), 'Serie automatisch aufnehmen');
    series.type = 'button';
    series.addEventListener('click', () => detail.appendChild(createSeriesPanel(event, channel)));
    actions.appendChild(series);
    detail.appendChild(actions);
    return detail;
  }

  function createEventDetail(entry, channel, row, list) {
    list.querySelectorAll('.channel-day-event-detail').forEach(element => element.remove());
    const wrapper = document.createElement('section');
    wrapper.className = 'channel-day-event-detail';
    const back = addText(document.createElement('button'), '← Zurück zum Tagesprogramm');
    back.type = 'button';
    back.className = 'channel-day-event-back';
    back.addEventListener('click', () => {
      wrapper.remove();
      row.focus();
    });
    wrapper.appendChild(back);

    const detail = typeof global.createEpgEventDetailCard === 'function'
      ? global.createEpgEventDetailCard(entry.event, channel)
      : fallbackEventDetail(entry.event, channel);
    wrapper.appendChild(upgradeEpgActions(detail, entry.event, channel));
    row.insertAdjacentElement('afterend', wrapper);
    wrapper.scrollIntoView({behavior: 'smooth', block: 'nearest'});
  }

  function renderEventList(view, channel, data, bounds) {
    const status = view.querySelector('[data-channel-day-status="true"]');
    const list = view.querySelector('[data-channel-day-list="true"]');
    const entries = eventEntriesForDay(listFromResponse(data, 'events'), channelId(channel), bounds);
    list.replaceChildren();

    if (entries.length === 0) {
      status.textContent = 'Keine Programmdaten für diesen Tag gefunden.';
      list.appendChild(addText(document.createElement('div'), 'Für das gewählte Datum liegen keine EPG-Einträge vor.'));
      return;
    }

    status.textContent = String(entries.length) + ' Sendungen für ' + dateHeading(state.selectedDate) + '.';
    const now = Math.floor(Date.now() / 1000);

    entries.forEach(entry => {
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'channel-day-event';
      const current = entry.start <= now && now < entry.end;
      if (current) row.classList.add('current');

      row.appendChild(addText(document.createElement('div'), clock(entry.start) + '–' + clock(entry.end))).className = 'channel-day-event-time';
      const content = document.createElement('div');
      content.appendChild(addText(document.createElement('div'), eventTitle(entry.event))).className = 'channel-day-event-title';
      const subtitle = eventSubtitle(entry.event);
      if (subtitle !== '' && subtitle !== eventTitle(entry.event)) {
        content.appendChild(addText(document.createElement('div'), subtitle)).className = 'channel-day-event-subtitle';
      }
      row.appendChild(content);

      const badges = document.createElement('div');
      badges.className = 'channel-day-event-badges';
      if (current) {
        const badge = addText(document.createElement('span'), 'Läuft jetzt');
        badge.className = 'channel-day-badge current';
        badges.appendChild(badge);
      }
      const duration = Math.max(0, Math.round((entry.end - entry.start) / 60));
      const durationBadge = addText(document.createElement('span'), String(duration) + ' Min.');
      durationBadge.className = 'channel-day-badge';
      badges.appendChild(durationBadge);
      row.appendChild(badges);

      if (current) {
        const progress = document.createElement('div');
        progress.className = 'channel-day-progress';
        const bar = document.createElement('span');
        bar.style.width = String(Math.max(0, Math.min(100, ((now - entry.start) / (entry.end - entry.start)) * 100))) + '%';
        progress.appendChild(bar);
        row.appendChild(progress);
      }

      row.addEventListener('click', () => createEventDetail(entry, channel, row, list));
      list.appendChild(row);
    });
  }

  function loadSelectedDay(view, channel) {
    const sequence = ++state.loadSequence;
    const bounds = dayBounds(state.selectedDate);
    const heading = view.querySelector('[data-channel-day-heading="true"]');
    const status = view.querySelector('[data-channel-day-status="true"]');
    const list = view.querySelector('[data-channel-day-list="true"]');
    heading.textContent = dateHeading(state.selectedDate);
    status.className = 'channel-day-status';
    status.textContent = 'Tagesprogramm wird geladen …';
    list.replaceChildren();

    loadDayEvents(channel, bounds).then(data => {
      if (sequence !== state.loadSequence || !view.isConnected) return;
      renderEventList(view, channel, data, bounds);
    }).catch(error => {
      if (sequence !== state.loadSequence || !view.isConnected) return;
      status.className = 'channel-day-status error';
      status.textContent = 'Tagesprogramm konnte nicht geladen werden: ' + String(error && error.message ? error.message : error);
    });
  }

  function createProgramView(channel) {
    const view = document.createElement('section');
    view.className = 'channel-day-program-view';
    view.dataset.channelDayProgram = 'true';

    const toolbar = document.createElement('div');
    toolbar.className = 'channel-day-toolbar';
    const back = addText(document.createElement('button'), '← Zurück zur Kanalliste');
    back.type = 'button';
    back.className = 'channel-day-back';
    back.addEventListener('click', () => closeProgramView(true));
    toolbar.appendChild(back);

    const controls = document.createElement('div');
    controls.className = 'channel-day-date-controls';
    const previous = addText(document.createElement('button'), '‹ Vortag');
    previous.type = 'button';
    const dateInput = document.createElement('input');
    dateInput.type = 'date';
    dateInput.value = dateInputValue(state.selectedDate);
    const today = addText(document.createElement('button'), 'Heute');
    today.type = 'button';
    const next = addText(document.createElement('button'), 'Nächster Tag ›');
    next.type = 'button';
    controls.appendChild(previous);
    controls.appendChild(today);
    controls.appendChild(next);
    controls.appendChild(dateInput);
    toolbar.appendChild(controls);
    view.appendChild(toolbar);
    view.appendChild(createChannelHeader(channel));

    const heading = document.createElement('h3');
    heading.className = 'channel-day-heading';
    heading.dataset.channelDayHeading = 'true';
    view.appendChild(heading);

    const status = document.createElement('div');
    status.className = 'channel-day-status';
    status.dataset.channelDayStatus = 'true';
    status.setAttribute('role', 'status');
    view.appendChild(status);

    const list = document.createElement('div');
    list.className = 'channel-day-list';
    list.dataset.channelDayList = 'true';
    view.appendChild(list);

    function selectDate(value) {
      state.selectedDate = dateFromInput(value);
      dateInput.value = dateInputValue(state.selectedDate);
      loadSelectedDay(view, channel);
    }

    previous.addEventListener('click', () => selectDate(addDays(state.selectedDate, -1)));
    today.addEventListener('click', () => selectDate(new Date()));
    next.addEventListener('click', () => selectDate(addDays(state.selectedDate, 1)));
    dateInput.addEventListener('change', () => selectDate(dateInput.value));
    return view;
  }

  function closeProgramView(restorePosition) {
    state.loadSequence += 1;
    if (state.activeView && state.activeView.isConnected) state.activeView.remove();
    if (state.sourceShell && state.sourceShell.isConnected) state.sourceShell.hidden = false;
    const button = state.sourceButton;
    const pane = state.sourceShell ? state.sourceShell.querySelector('.channel-browser-channel-pane') : null;
    if (pane) pane.scrollTop = state.sourcePaneScrollTop;
    if (restorePosition) {
      global.setTimeout(() => {
        global.scrollTo(0, state.sourceWindowY);
        if (button && typeof button.focus === 'function') button.focus({preventScroll: true});
      }, 0);
    }
    state.activeView = null;
    state.sourceShell = null;
    state.sourceButton = null;
    state.channel = null;
  }

  function openProgramView(button) {
    if (state.activeView) return;
    const root = document.getElementById('detail-data');
    const shell = button.closest('.channel-browser-shell');
    if (!root || !shell) return;

    state.sourceWindowY = global.scrollY || 0;
    state.sourceShell = shell;
    state.sourceButton = button;
    const pane = shell.querySelector('.channel-browser-channel-pane');
    state.sourcePaneScrollTop = pane ? pane.scrollTop : 0;
    state.selectedDate = dateFromInput(new Date());
    shell.hidden = true;

    const provisional = channelFromButton(button);
    resolveChannel(provisional).then(channel => {
      if (!state.sourceShell || !state.sourceShell.isConnected) return;
      state.channel = channel;
      const view = createProgramView(channel);
      state.activeView = view;
      root.appendChild(view);
      loadSelectedDay(view, channel);
      global.scrollTo({top: Math.max(0, root.getBoundingClientRect().top + global.scrollY - 12), behavior: 'smooth'});
    });
  }

  function install() {
    const root = document.getElementById('detail-data');
    if (!root || root.dataset.channelDayProgramBound === 'true') return;
    root.dataset.channelDayProgramBound = 'true';
    installStyles();

    root.addEventListener('click', event => {
      const button = event.target && event.target.closest
        ? event.target.closest('.channel-browser-item')
        : null;
      if (!button || !root.contains(button) || state.activeView) return;
      event.preventDefault();
      event.stopPropagation();
      if (typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation();
      openProgramView(button);
    }, true);

    const observer = new MutationObserver(() => {
      if (state.activeView && !root.contains(state.activeView)) {
        state.activeView = null;
        state.sourceShell = null;
        state.sourceButton = null;
        state.channel = null;
      }
    });
    observer.observe(root, {childList: true});
  }

  const api = Object.freeze({
    parseChannelButtonMetadata,
    dayBounds,
    dateInputValue,
    eventEntriesForDay,
    buildTimerPayload,
    buildSeriesSearchTimerPayload,
    applySeriesScope,
    normalizeChannel
  });
  global.VdrSuiteChannelDayProgram = api;

  if (typeof document !== 'undefined') {
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', install);
    else install();
  }
})(window);
