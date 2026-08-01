'use strict';

// Phase 60.17: deferred EPG timeline enhancements for channel groups, detail artwork and SearchTimer actions.
(function (global) {
  const PREVIEW_REFRESH_WINDOW_SECONDS = 14 * 24 * 60 * 60;
  const PREVIEW_CHANNEL_EVENT_LIMIT = 96;
  const TIMELINE_STYLE_ID = 'vdr-suite-epg-timeline-enhancements';
  const TIMELINE_GROUP_ALL = '__all__';
  const TIMELINE_READY_ATTEMPTS = 80;
  const TIMELINE_READY_INTERVAL_MS = 100;

  let selectedTimelineGroup = '';
  let selectedTimelineBackendId = '';
  let timelineFunctionsWrapped = false;

  function searchTimerNavigationButton() {
    return document.querySelector('[data-module="searchtimers"]');
  }

  function setFormValue(form, name, value, eventName) {
    const input = form.elements[name];
    if (!input) return;
    if (input.type === 'checkbox') input.checked = Boolean(value);
    else input.value = String(value === undefined || value === null ? '' : value);
    input.dispatchEvent(new Event(eventName || 'input', {bubbles: true}));
  }

  function showStatus(target, message, failed) {
    if (!target) return;
    let status = target.querySelector('[data-searchtimer-editor-status="true"]');
    if (!status) {
      status = document.createElement('p');
      status.dataset.searchtimerEditorStatus = 'true';
      status.setAttribute('role', 'status');
      status.setAttribute('aria-live', 'polite');
      target.appendChild(status);
    }
    status.className = failed ? 'channels2-feedback error' : 'channels2-feedback success';
    status.textContent = message;
  }

  function selectedBackendId() {
    const runtime = global.VdrSuitePlatform;
    if (runtime && typeof runtime.getSelectedBackendId === 'function') {
      const value = String(runtime.getSelectedBackendId() || '').trim();
      if (value) return value;
    }
    return 'default';
  }

  function refreshPreviewCache() {
    const client = global.VdrSuiteClientApi;
    if (!client || typeof client.fetchClientSearchTimerPreviewCacheRefresh !== 'function') {
      return Promise.reject(new Error('Preview-EPG-Cache kann nicht aktualisiert werden: Client API fehlt.'));
    }

    return client.fetchClientSearchTimerPreviewCacheRefresh({
      backendId: selectedBackendId(),
      query: {
        from: -1,
        timespan: PREVIEW_REFRESH_WINDOW_SECONDS,
        limit: 0,
        channelEventLimit: PREVIEW_CHANNEL_EVENT_LIMIT,
        _: Date.now()
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function (result) {
      const available = result && result.available === true;
      const ready = result && String(result.status || '') === 'ready';
      const eventCount = Number(result && result.eventCount || 0);
      if (!available || !ready || eventCount <= 0) {
        throw new Error('Der Preview-EPG-Cache ist nicht bereit oder enthält keine Sendungen.');
      }
      return result;
    });
  }

  function previewFeedback(button) {
    const form = button.closest('form[data-searchtimer-editor-form="create"]');
    return form ? form.querySelector('[data-searchtimer-preview-result="true"]') : null;
  }

  function installPreviewCacheGuard() {
    if (document.documentElement.dataset.searchtimerPreviewCacheGuard === 'true') return;
    document.documentElement.dataset.searchtimerPreviewCacheGuard = 'true';

    document.addEventListener('click', function (event) {
      const button = event.target && event.target.closest
        ? event.target.closest('[data-searchtimer-action="preview"]')
        : null;
      if (!button || button.dataset.previewCacheReady === 'true') return;

      event.preventDefault();
      event.stopPropagation();
      if (typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation();

      const feedback = previewFeedback(button);
      button.disabled = true;
      if (feedback) {
        feedback.className = 'searchtimer-feedback';
        feedback.textContent = 'EPG-Daten für die Vorschau werden aktualisiert …';
      }

      refreshPreviewCache()
        .then(function (result) {
          if (feedback) {
            feedback.className = 'searchtimer-feedback success';
            feedback.textContent = String(result.eventCount) + ' EPG-Sendungen geladen. Vorschau wird ausgeführt …';
          }
          button.dataset.previewCacheReady = 'true';
          button.disabled = false;
          button.click();
          delete button.dataset.previewCacheReady;
        })
        .catch(function (error) {
          button.disabled = false;
          if (feedback) {
            feedback.className = 'searchtimer-feedback error';
            feedback.textContent = String(error && error.message ? error.message : error);
          } else {
            global.alert(String(error && error.message ? error.message : error));
          }
        });
    }, true);
  }

  function openSearchTimerEditor(options) {
    const settings = options && typeof options === 'object' ? options : {};
    const title = String(settings.title || '').trim();
    const channelId = String(settings.channelId || '').trim();
    const channelGroup = String(settings.channelGroup || '').trim();
    const statusTarget = settings.statusTarget || null;
    const navigationButton = searchTimerNavigationButton();

    if (!title || !navigationButton || typeof navigationButton.click !== 'function') {
      showStatus(statusTarget, 'Der SearchTimer-Editor ist derzeit nicht erreichbar.', true);
      return Promise.reject(new Error('SearchTimer editor is not available'));
    }

    showStatus(statusTarget, 'SearchTimer-Editor für „' + title + '“ wird geöffnet …', false);
    navigationButton.click();

    return new Promise(function (resolve, reject) {
      let attempts = 0;
      const fill = function () {
        attempts += 1;
        const form = document.querySelector('form[data-searchtimer-editor-form="create"]');
        if (!form) {
          if (attempts < 40) {
            global.setTimeout(fill, 100);
            return;
          }
          showStatus(statusTarget, 'Der SearchTimer-Editor konnte nicht geöffnet werden.', true);
          reject(new Error('SearchTimer editor form did not appear'));
          return;
        }

        const panel = form.closest('details');
        if (panel) panel.open = true;
        setFormValue(form, 'name', title);
        setFormValue(form, 'query', title);
        setFormValue(form, 'active', true, 'change');
        setFormValue(form, 'compareTitle', true, 'change');
        setFormValue(form, 'avoidRepeats', true, 'change');

        if (channelId) {
          setFormValue(form, 'channelFilterMode', 1, 'change');
          global.setTimeout(function () {
            if (channelGroup) setFormValue(form, 'channelSelectorGroup', channelGroup, 'change');
            global.setTimeout(function () {
              setFormValue(form, 'channelId', channelId, 'change');
              form.scrollIntoView({behavior: 'smooth', block: 'start'});
              resolve(form);
            }, 80);
          }, 80);
          return;
        }

        form.scrollIntoView({behavior: 'smooth', block: 'start'});
        resolve(form);
      };
      global.setTimeout(fill, 80);
    });
  }

  function timelineList(data, key) {
    if (Array.isArray(data)) return data;
    if (data && Array.isArray(data[key])) return data[key];
    if (data && Array.isArray(data.items)) return data.items;
    return [];
  }

  function timelineFirstValue(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
        return object[key];
      }
    }
    return fallback;
  }

  function timelineChannelGroup(channel) {
    const value = String(timelineFirstValue(
      channel,
      ['group', 'groupName', 'channelGroup', 'bouquet', 'category', 'section'],
      'Weitere Sender'
    )).trim();
    return value || 'Weitere Sender';
  }

  function timelineChannelId(channel) {
    return String(timelineFirstValue(channel, ['id', 'channelId', 'nativeId'], '')).trim();
  }

  function timelineEventChannelId(event) {
    return String(timelineFirstValue(event, ['channelId', 'channel', 'channel_id'], '')).trim();
  }

  function timelineEventTitle(event) {
    return String(timelineFirstValue(event, ['title', 'name', 'eventTitle'], 'Sendung')).trim();
  }

  function timelineEventArtwork(event) {
    const artwork = event && event.artwork;
    if (artwork && artwork.available === true) {
      const publicUrl = String(artwork.url || '').trim();
      if (publicUrl) return publicUrl;
    }

    return String(timelineFirstValue(
      event,
      ['bannerUrl', 'imageUrl', 'posterUrl', 'artworkUrl', 'image', 'poster', 'banner'],
      ''
    )).trim();
  }

  function timelineGroupOptions(channels) {
    const counts = new Map();
    channels.forEach(function (channel) {
      const group = timelineChannelGroup(channel);
      counts.set(group, (counts.get(group) || 0) + 1);
    });
    return Array.from(counts, function (entry) {
      return {name: entry[0], count: entry[1]};
    });
  }

  function ensureTimelineGroup(channels) {
    const backendId = selectedBackendId();
    const options = timelineGroupOptions(channels);
    const names = options.map(function (option) { return option.name; });

    if (selectedTimelineBackendId !== backendId) {
      selectedTimelineBackendId = backendId;
      selectedTimelineGroup = '';
    }

    if (selectedTimelineGroup !== TIMELINE_GROUP_ALL && names.indexOf(selectedTimelineGroup) < 0) {
      selectedTimelineGroup = names.length ? names[0] : TIMELINE_GROUP_ALL;
    }

    return options;
  }

  function filterTimelineChannels(channelData) {
    const channels = timelineList(channelData, 'channels');
    ensureTimelineGroup(channels);
    if (selectedTimelineGroup === TIMELINE_GROUP_ALL) return channels.slice();
    return channels.filter(function (channel) {
      return timelineChannelGroup(channel) === selectedTimelineGroup;
    });
  }

  function timelineChannelData(channelData, channels) {
    const result = Array.isArray(channelData) ? {} : Object.assign({}, channelData || {});
    result.channels = channels;
    return result;
  }

  function safeCssUrl(url) {
    return 'url("' + String(url).replace(/["\\\r\n]/g, '') + '")';
  }

  function decorateTimelineDetailArtwork(detail, event) {
    if (!detail || typeof detail.querySelector !== 'function') return detail;

    const url = timelineEventArtwork(event);
    const hero = detail.querySelector('.epg-detail-hero');
    if (!url || !hero || typeof detail.insertBefore !== 'function') return detail;

    const artwork = document.createElement('div');
    artwork.className = 'epg-detail-artwork';
    artwork.setAttribute('role', 'img');
    artwork.setAttribute('aria-label', 'Bild zu ' + timelineEventTitle(event));
    artwork.style.backgroundImage = safeCssUrl(url);
    detail.classList.add('epg-has-artwork');
    detail.insertBefore(artwork, hero);
    return detail;
  }

  function enableTimelineSearchTimer(detail, event, channel) {
    if (!detail || typeof detail.querySelectorAll !== 'function') return;
    const buttons = Array.from(detail.querySelectorAll('.epg-detail-action'));
    const button = buttons.find(function (candidate) {
      return String(candidate.textContent || '').trim() === 'Suchtimer';
    });
    if (!button) return;

    button.disabled = false;
    button.textContent = 'Suchtimer erstellen';
    button.title = 'SearchTimer-Editor mit Titel und Kanal dieser Sendung öffnen.';
    button.addEventListener('click', function () {
      openSearchTimerEditor({
        title: timelineEventTitle(event),
        channelId: timelineEventChannelId(event) || timelineChannelId(channel),
        channelGroup: timelineChannelGroup(channel),
        statusTarget: detail
      }).catch(function () {
        return null;
      });
    });
  }

  function ensureTimelineStyles() {
    if (document.getElementById(TIMELINE_STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = TIMELINE_STYLE_ID;
    style.textContent = [
      '.epg-group-control{display:flex;align-items:flex-end;gap:.65rem;flex-wrap:wrap;margin:.75rem 0}',
      '.epg-group-field{display:grid;gap:.25rem;min-width:min(24rem,100%)}',
      '.epg-group-field label{color:#94a3b8;font-size:.74rem;font-weight:800;text-transform:uppercase;letter-spacing:.04em}',
      '.epg-group-select{min-height:2.55rem;padding:.5rem .7rem;border:1px solid #475569;border-radius:.7rem;background:#0f172a;color:#f8fafc;font:inherit}',
      '.epg-group-summary{color:#bae6fd;font-size:.85rem;font-weight:750}',
      '.epg-event-detail.epg-has-artwork{overflow:hidden}',
      '.epg-detail-artwork{width:100%;aspect-ratio:16/9;min-height:8.5rem;max-height:13rem;margin-bottom:.8rem;border-radius:.75rem;background-size:cover;background-position:center;box-shadow:inset 0 -3rem 4rem rgba(2,6,23,.38)}',
      '@media(max-width:720px){.epg-group-control{align-items:stretch;flex-direction:column}.epg-group-field{min-width:0;width:100%}.epg-detail-artwork{min-height:9rem;max-height:none}}'
    ].join('');
    document.head.appendChild(style);
  }

  function renderTimelineGroupControl(channelData) {
    if (typeof detailDataElement === 'undefined' || !detailDataElement) return;
    const intro = detailDataElement.querySelector('.epg-timeline-intro');
    if (!intro) return;

    const channels = timelineList(channelData, 'channels');
    const options = ensureTimelineGroup(channels);
    const selectedChannels = filterTimelineChannels(channelData);
    const control = document.createElement('div');
    control.className = 'epg-group-control';

    const field = document.createElement('div');
    field.className = 'epg-group-field';
    const label = document.createElement('label');
    label.textContent = 'Kanalgruppe';
    const select = document.createElement('select');
    select.className = 'epg-group-select';
    select.setAttribute('aria-label', 'Kanalgruppe für die EPG-Zeitleiste auswählen');

    const allOption = document.createElement('option');
    allOption.value = TIMELINE_GROUP_ALL;
    allOption.textContent = 'Alle Kanäle (' + String(channels.length) + ')';
    select.appendChild(allOption);

    options.forEach(function (option) {
      const element = document.createElement('option');
      element.value = option.name;
      element.textContent = option.name + ' (' + String(option.count) + ')';
      select.appendChild(element);
    });

    select.value = selectedTimelineGroup;
    select.addEventListener('change', function () {
      selectedTimelineGroup = select.value || TIMELINE_GROUP_ALL;
      if (typeof epgChannelOffset !== 'undefined') epgChannelOffset = 0;
      if (typeof selectedEpgDetail !== 'undefined') selectedEpgDetail = null;
      if (typeof loadEpgTimeline === 'function') loadEpgTimeline();
    });

    field.appendChild(label);
    field.appendChild(select);
    control.appendChild(field);

    const groupLabel = selectedTimelineGroup === TIMELINE_GROUP_ALL
      ? 'Alle Kanäle'
      : selectedTimelineGroup;
    const summary = document.createElement('div');
    summary.className = 'epg-group-summary';
    summary.textContent = groupLabel + ' · ' + String(selectedChannels.length) + ' Kanäle';
    control.appendChild(summary);

    const modeRow = intro.querySelector('.epg-view-toggle');
    if (modeRow && typeof intro.insertBefore === 'function') intro.insertBefore(control, modeRow);
    else intro.appendChild(control);

    const description = intro.querySelector('p');
    if (description) description.textContent = 'Gruppe: ' + groupLabel + ' · ' + description.textContent;

    const pager = intro.querySelector('.epg-pager');
    const limit = typeof EPG_VISIBLE_CHANNEL_LIMIT === 'number'
      ? Math.max(1, EPG_VISIBLE_CHANNEL_LIMIT)
      : 15;
    if (pager) {
      if (selectedChannels.length <= limit) {
        pager.hidden = true;
      } else {
        const pagerButtons = pager.querySelectorAll('button');
        if (pagerButtons[0]) pagerButtons[0].textContent = 'Vorherige Sender';
        if (pagerButtons[1]) pagerButtons[1].textContent = 'Nächste Sender';
      }
    }
  }

  function timelineRendererIsReady() {
    return typeof renderEpgTimeView === 'function'
      && typeof visibleEpgChannelsFromData === 'function'
      && typeof createEpgEventDetailCard === 'function';
  }

  function wrapTimelineFunctions() {
    if (timelineFunctionsWrapped) return true;
    if (!timelineRendererIsReady()) return false;

    const originalVisibleEpgChannelsFromData = visibleEpgChannelsFromData;
    const originalCreateEpgEventDetailCard = createEpgEventDetailCard;
    const originalRenderEpgTimeView = renderEpgTimeView;

    visibleEpgChannelsFromData = function (channelData) {
      const filtered = filterTimelineChannels(channelData);
      return originalVisibleEpgChannelsFromData(timelineChannelData(channelData, filtered));
    };

    createEpgEventDetailCard = function (event, channel) {
      const detail = originalCreateEpgEventDetailCard(event, channel);
      decorateTimelineDetailArtwork(detail, event);
      enableTimelineSearchTimer(detail, event, channel);
      return detail;
    };

    renderEpgTimeView = function (channelData, eventData) {
      const filtered = filterTimelineChannels(channelData);
      const result = originalRenderEpgTimeView(timelineChannelData(channelData, filtered), eventData);
      renderTimelineGroupControl(channelData);
      return result;
    };

    timelineFunctionsWrapped = true;
    document.documentElement.dataset.epgTimelineEnhancements = 'true';
    document.documentElement.dataset.epgDetailArtwork = 'true';
    delete document.documentElement.dataset.epgVerticalQuarterScale;
    return true;
  }

  function installTimelineEnhancements() {
    ensureTimelineStyles();
    let attempts = 0;
    const attempt = function () {
      if (wrapTimelineFunctions()) return;
      attempts += 1;
      if (attempts < TIMELINE_READY_ATTEMPTS) {
        global.setTimeout(attempt, TIMELINE_READY_INTERVAL_MS);
      }
    };
    attempt();
  }

  installPreviewCacheGuard();
  installTimelineEnhancements();
  global.VdrSuiteEpgSearchTimerActions = Object.freeze({
    openSearchTimerEditor: openSearchTimerEditor,
    refreshPreviewCache: refreshPreviewCache,
    timelineEnhancementsReady: function () { return timelineFunctionsWrapped; }
  });
}(window));
