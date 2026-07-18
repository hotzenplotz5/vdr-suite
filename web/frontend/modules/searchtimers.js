// SearchTimer browser with compact cards and safe create/preview/delete workflows.
(function(global) {
  'use strict';

  let searchTimerBrowserContext = Object.freeze({});
  let activeCreateForm = null;

  function configureContext(context) {
    searchTimerBrowserContext = Object.freeze(Object.assign({}, context || {}));
  }

  function addText(element, text) {
    element.textContent = String(text);
    return element;
  }

  function firstValue(source, keys, fallback) {
    if (!source || typeof source !== 'object') {
      return fallback;
    }

    for (const key of keys) {
      if (Object.prototype.hasOwnProperty.call(source, key) &&
          source[key] !== undefined && source[key] !== null && source[key] !== '') {
        return source[key];
      }
    }

    return fallback;
  }

  function listFromResponse(data, key) {
    if (Array.isArray(data)) return data;
    if (data && Array.isArray(data[key])) return data[key];
    if (data && Array.isArray(data.items)) return data.items;
    return [];
  }

  function boolValue(value, fallback) {
    if (value === true || value === 1 || value === '1' || value === 'true' || value === 'yes') return true;
    if (value === false || value === 0 || value === '0' || value === 'false' || value === 'no') return false;
    return Boolean(fallback);
  }

  function numberValue(value, fallback) {
    const number = Number(value);
    return Number.isFinite(number) ? Math.trunc(number) : Number(fallback || 0);
  }

  function objectValue(source, key) {
    const value = source && typeof source === 'object' ? source[key] : null;
    return value && typeof value === 'object' ? value : {};
  }

  function normalizeSearchTimer(searchTimer, index) {
    const source = searchTimer && typeof searchTimer === 'object' ? searchTimer : {};
    const recording = objectValue(source, 'recordingOptions');
    const schedule = objectValue(source, 'scheduleOptions');
    const filters = objectValue(source, 'filterOptions');
    const comparison = objectValue(source, 'comparisonOptions');
    const repeats = objectValue(source, 'repeatOptions');
    const channels = objectValue(source, 'channelOptions');
    const series = objectValue(source, 'seriesOptions');
    const blacklist = objectValue(source, 'blacklistOptions');
    const match = objectValue(source, 'matchOptions');
    const extended = objectValue(source, 'extendedEpgOptions');
    const validity = objectValue(source, 'validityOptions');
    const actions = objectValue(source, 'actionOptions');
    const state = String(firstValue(source, ['state'], '')).toLowerCase();

    return {
      source,
      index: Number(index) || 0,
      backendId: String(firstValue(source, ['backendId', 'backend'], 'default')),
      backendNativeId: String(firstValue(source, ['backendNativeId', 'id', 'timerId'], '')),
      name: String(firstValue(source, ['name', 'title', 'search', 'query'], 'SearchTimer')),
      query: String(firstValue(source, ['query', 'search', 'pattern'], '')),
      active: state === 'active' || boolValue(firstValue(source, ['active', 'enabled', 'isActive', 'use_as_searchtimer'], false), false),
      directory: String(firstValue(recording, ['directory'], firstValue(source, ['directory'], ''))),
      priority: numberValue(firstValue(recording, ['priority'], firstValue(source, ['priority'], 0)), 0),
      lifetime: numberValue(firstValue(recording, ['lifetime'], firstValue(source, ['lifetime'], 0)), 0),
      marginStartMinutes: numberValue(firstValue(schedule, ['marginStartMinutes'], firstValue(source, ['margin_start', 'marginStart'], 0)), 0),
      marginStopMinutes: numberValue(firstValue(schedule, ['marginStopMinutes'], firstValue(source, ['margin_stop', 'marginStop'], 0)), 0),
      useVps: boolValue(firstValue(schedule, ['useVps'], firstValue(source, ['use_vps', 'vps'], false)), false),
      useChannel: boolValue(firstValue(filters, ['useChannel'], firstValue(source, ['use_channel', 'useChannel'], false)), false),
      channels: String(firstValue(channels, ['channels'], firstValue(source, ['channels'], ''))),
      channelMin: numberValue(firstValue(channels, ['channelMin'], firstValue(source, ['channel_min', 'channelMin'], 0)), 0),
      channelMax: numberValue(firstValue(channels, ['channelMax'], firstValue(source, ['channel_max', 'channelMax'], 0)), 0),
      useDuration: boolValue(firstValue(filters, ['useDuration'], firstValue(source, ['use_duration', 'useDuration'], false)), false),
      durationMinMinutes: numberValue(firstValue(filters, ['durationMinMinutes'], firstValue(source, ['duration_min', 'durationMin'], 0)), 0),
      durationMaxMinutes: numberValue(firstValue(filters, ['durationMaxMinutes'], firstValue(source, ['duration_max', 'durationMax'], 0)), 0),
      useDayOfWeek: boolValue(firstValue(filters, ['useDayOfWeek'], firstValue(source, ['use_dayofweek', 'useDayOfWeek'], false)), false),
      compareTitle: boolValue(firstValue(comparison, ['compareTitle'], firstValue(source, ['compare_title', 'use_title'], true)), true),
      compareSubtitle: boolValue(firstValue(comparison, ['compareSubtitle'], firstValue(source, ['compare_subtitle', 'use_subtitle'], false)), false),
      compareSummary: boolValue(firstValue(comparison, ['compareSummary'], firstValue(source, ['compare_summary', 'use_description'], false)), false),
      compareCategories: boolValue(firstValue(comparison, ['compareCategories'], firstValue(source, ['compare_categories'], false)), false),
      compareTime: boolValue(firstValue(comparison, ['compareTime'], firstValue(source, ['compare_time'], false)), false),
      avoidRepeats: boolValue(firstValue(repeats, ['avoidRepeats'], firstValue(source, ['avoid_repeats'], false)), false),
      allowedRepeats: numberValue(firstValue(repeats, ['allowedRepeats'], firstValue(source, ['allowed_repeats'], 0)), 0),
      repeatsWithinDays: numberValue(firstValue(repeats, ['repeatsWithinDays'], firstValue(source, ['repeats_within_days'], 0)), 0),
      useSeriesRecording: boolValue(firstValue(series, ['useSeriesRecording'], firstValue(source, ['use_series_recording'], false)), false),
      keepRecordings: numberValue(firstValue(series, ['keepRecordings'], firstValue(source, ['keep_recs'], 0)), 0),
      deleteMode: numberValue(firstValue(series, ['deleteMode'], firstValue(source, ['del_mode'], 0)), 0),
      searchTimerAction: numberValue(firstValue(series, ['searchTimerAction'], firstValue(source, ['search_timer_action'], 0)), 0),
      blacklistMode: numberValue(firstValue(blacklist, ['blacklistMode'], firstValue(source, ['blacklist_mode'], 0)), 0),
      blacklistIds: String(firstValue(blacklist, ['blacklistIds'], firstValue(source, ['blacklist_ids'], ''))),
      mode: numberValue(firstValue(match, ['mode'], firstValue(source, ['mode'], 0)), 0),
      matchCase: boolValue(firstValue(match, ['matchCase'], firstValue(source, ['match_case'], false)), false),
      tolerance: numberValue(firstValue(match, ['tolerance'], firstValue(source, ['tolerance'], 0)), 0),
      summaryMatch: numberValue(firstValue(match, ['summaryMatch'], firstValue(source, ['summary_match'], 0)), 0),
      useExtendedEpgInfo: boolValue(firstValue(extended, ['useExtendedEpgInfo'], firstValue(source, ['use_ext_epg_info'], false)), false),
      extendedEpgInfo: String(firstValue(extended, ['extendedEpgInfo'], firstValue(source, ['ext_epg_info'], ''))),
      ignoreMissingEpgCategories: boolValue(firstValue(extended, ['ignoreMissingEpgCategories'], firstValue(source, ['ignore_missing_epg_cats'], false)), false),
      contentDescriptors: String(firstValue(extended, ['contentDescriptors'], firstValue(source, ['content_descriptors'], ''))),
      useInFavorites: boolValue(firstValue(validity, ['useInFavorites'], firstValue(source, ['use_in_favorites'], false)), false),
      activeFrom: String(firstValue(validity, ['activeFrom'], firstValue(source, ['use_as_searchtimer_from'], ''))),
      activeUntil: String(firstValue(validity, ['activeUntil'], firstValue(source, ['use_as_searchtimer_til'], ''))),
      pauseOnRecordings: boolValue(firstValue(actions, ['pauseOnRecordings'], firstValue(source, ['pause_on_recs'], false)), false),
      switchMinutesBefore: numberValue(firstValue(actions, ['switchMinutesBefore'], firstValue(source, ['switch_min_before'], 0)), 0),
      unmuteSoundOnSwitch: boolValue(firstValue(actions, ['unmuteSoundOnSwitch'], firstValue(source, ['unmute_sound_on_switch'], false)), false),
      deleteRecordingsAfterDays: numberValue(firstValue(actions, ['deleteRecordingsAfterDays'], firstValue(source, ['del_recs_after_days'], 0)), 0),
      deleteAfterCountRecordings: numberValue(firstValue(actions, ['deleteAfterCountRecordings'], firstValue(source, ['del_after_count_recs'], 0)), 0),
      deleteAfterDaysOfFirstRecording: numberValue(firstValue(actions, ['deleteAfterDaysOfFirstRecording'], firstValue(source, ['del_after_days_of_first_rec'], 0)), 0),
      updateSafe: false
    };
  }

  function selectedBackendId() {
    if (typeof searchTimerBrowserContext.getSelectedBackendId === 'function') {
      const id = String(searchTimerBrowserContext.getSelectedBackendId() || '').trim();
      if (id !== '') return id;
    }
    return 'default';
  }

  function installStyles() {
    if (typeof document === 'undefined' || document.getElementById('vdr-suite-searchtimer-workflow-styles')) return;

    const style = document.createElement('style');
    style.id = 'vdr-suite-searchtimer-workflow-styles';
    style.textContent = `
.searchtimer-module{display:grid;gap:.8rem}.searchtimer-summary{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:.6rem}.searchtimer-summary h3,.searchtimer-card h3{margin:0}.searchtimer-create-panel,.searchtimer-card{border:1px solid rgba(96,165,250,.24);border-radius:.95rem;background:rgba(15,23,42,.72)}.searchtimer-create-panel>summary,.searchtimer-card>summary{cursor:pointer;padding:.82rem .9rem;color:#f8fafc;font-weight:850}.searchtimer-create-panel[open]>summary,.searchtimer-card[open]>summary{border-bottom:1px solid rgba(148,163,184,.18)}.searchtimer-card.inactive{opacity:.8}.searchtimer-card-summary{display:grid;grid-template-columns:minmax(0,1fr) auto;align-items:center;gap:.6rem}.searchtimer-card-title strong{display:block;overflow:hidden;text-overflow:ellipsis}.searchtimer-card-title span{display:block;margin-top:.15rem;color:#94a3b8;font-size:.86rem;font-weight:500;overflow:hidden;text-overflow:ellipsis}.searchtimer-badges{display:flex;flex-wrap:wrap;justify-content:flex-end;gap:.32rem}.searchtimer-badge{padding:.22rem .48rem;border:1px solid rgba(148,163,184,.28);border-radius:999px;background:rgba(2,6,23,.72);color:#cbd5e1;font-size:.74rem;font-weight:800}.searchtimer-badge.active{border-color:rgba(74,222,128,.5);color:#bbf7d0}.searchtimer-badge.vps,.searchtimer-badge.filter{border-color:rgba(56,189,248,.48);color:#bae6fd}.searchtimer-create-body,.searchtimer-card-body{display:grid;gap:.72rem;padding:.82rem .9rem .9rem}.searchtimer-form{display:grid;gap:.72rem}.searchtimer-form-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.62rem}.searchtimer-field{display:grid;gap:.28rem;min-width:0}.searchtimer-field.wide{grid-column:1/-1}.searchtimer-field>span{color:#cbd5e1;font-size:.78rem;font-weight:750}.searchtimer-field input,.searchtimer-field select{box-sizing:border-box;width:100%;min-width:0;min-height:2.55rem;padding:.5rem .62rem;border:1px solid #475569;border-radius:.62rem;background:#111827;color:#f8fafc;font:inherit}.searchtimer-check{display:flex;align-items:center;gap:.45rem;min-height:2.55rem}.searchtimer-check input{width:1.15rem;height:1.15rem}.searchtimer-section{padding:.65rem;border:1px solid rgba(148,163,184,.17);border-radius:.72rem;background:rgba(2,6,23,.48)}.searchtimer-section h4{margin:0 0 .48rem;color:#e2e8f0}.searchtimer-meta-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(10rem,1fr));gap:.5rem}.searchtimer-meta{padding:.52rem .6rem;border:1px solid rgba(148,163,184,.16);border-radius:.64rem;background:rgba(15,23,42,.64)}.searchtimer-meta span{display:block;color:#94a3b8;font-size:.72rem;font-weight:750;text-transform:uppercase}.searchtimer-meta strong{display:block;margin-top:.14rem;color:#f8fafc;overflow-wrap:anywhere}.searchtimer-actions{display:flex;flex-wrap:wrap;gap:.5rem}.searchtimer-actions button{min-height:2.55rem;padding:.55rem .78rem;border-radius:.65rem}.searchtimer-actions .danger{border-color:rgba(248,113,113,.55);color:#fecaca}.searchtimer-actions .primary{border-color:rgba(56,189,248,.55)}.searchtimer-feedback{min-height:1.2rem;padding:.58rem .65rem;border-radius:.62rem;background:rgba(2,6,23,.5);color:#cbd5e1}.searchtimer-feedback.success{border:1px solid rgba(74,222,128,.4);color:#bbf7d0}.searchtimer-feedback.error{border:1px solid rgba(248,113,113,.45);color:#fecaca}.searchtimer-preview-summary{display:grid;grid-template-columns:repeat(auto-fit,minmax(8.5rem,1fr));gap:.45rem}.searchtimer-preview-metric{padding:.5rem;border:1px solid rgba(56,189,248,.2);border-radius:.62rem;background:rgba(15,23,42,.7)}.searchtimer-preview-metric span{display:block;color:#94a3b8;font-size:.7rem}.searchtimer-preview-metric strong{display:block;margin-top:.12rem}.searchtimer-preview-match{margin-top:.55rem;padding:.62rem;border:1px solid rgba(148,163,184,.18);border-radius:.68rem;background:rgba(15,23,42,.68)}.searchtimer-preview-match h5{margin:0}.searchtimer-preview-badges{display:flex;flex-wrap:wrap;gap:.3rem;margin-top:.4rem}.searchtimer-preview-badge{padding:.18rem .4rem;border-radius:999px;background:rgba(2,6,23,.7);color:#bae6fd;font-size:.72rem}.searchtimer-preview-technical{margin-top:.6rem}.searchtimer-preview-technical pre{max-height:22rem;overflow:auto;white-space:pre-wrap;overflow-wrap:anywhere}.searchtimer-safety-note{padding:.62rem;border:1px solid rgba(251,191,36,.3);border-radius:.68rem;background:rgba(120,53,15,.18);color:#fde68a}
@media(max-width:760px){.searchtimer-card-summary{grid-template-columns:minmax(0,1fr)}.searchtimer-badges{justify-content:flex-start}.searchtimer-form-grid{grid-template-columns:minmax(0,1fr)}.searchtimer-field.wide{grid-column:auto}.searchtimer-actions{display:grid;grid-template-columns:minmax(0,1fr)}.searchtimer-actions button{width:100%}.searchtimer-meta-grid{grid-template-columns:minmax(0,1fr)}}
`;
    document.head.appendChild(style);
  }

  function input(name, value, type) {
    const element = document.createElement('input');
    element.name = name;
    element.type = type || 'text';
    element.value = value === undefined || value === null ? '' : String(value);
    return element;
  }

  function field(label, control, wide) {
    const row = document.createElement('label');
    row.className = 'searchtimer-field' + (wide ? ' wide' : '');
    row.appendChild(addText(document.createElement('span'), label));
    row.appendChild(control);
    return row;
  }

  function check(name, label, checked) {
    const row = document.createElement('label');
    row.className = 'searchtimer-check';
    const control = document.createElement('input');
    control.type = 'checkbox';
    control.name = name;
    control.checked = Boolean(checked);
    row.appendChild(control);
    row.appendChild(addText(document.createElement('span'), label));
    return row;
  }

  function select(name, value, options) {
    const control = document.createElement('select');
    control.name = name;
    options.forEach(optionData => {
      const option = document.createElement('option');
      option.value = String(optionData[0]);
      option.textContent = String(optionData[1]);
      control.appendChild(option);
    });
    control.value = String(value);
    return control;
  }

  function formValue(form, name, fallback) {
    const element = form.elements[name];
    if (!element) return fallback;
    if (element.type === 'checkbox') return element.checked;
    return String(element.value || '').trim();
  }

  function buildCreatePayload(form, template) {
    const source = template || normalizeSearchTimer({}, 0);
    const channels = formValue(form, 'channels', source.channels);
    const useChannel = String(channels || '').trim() !== '';

    return {
      backendId: selectedBackendId(),
      name: formValue(form, 'name', source.name),
      query: formValue(form, 'query', source.query),
      active: boolValue(formValue(form, 'active', true), true),
      directory: formValue(form, 'directory', source.directory),
      priority: numberValue(formValue(form, 'priority', source.priority), 0),
      lifetime: numberValue(formValue(form, 'lifetime', source.lifetime), 0),
      marginStartMinutes: numberValue(formValue(form, 'marginStartMinutes', source.marginStartMinutes), 0),
      marginStopMinutes: numberValue(formValue(form, 'marginStopMinutes', source.marginStopMinutes), 0),
      useVps: boolValue(formValue(form, 'useVps', source.useVps), false),
      useChannel: useChannel ? 1 : 0,
      channels,
      channelMin: String(source.channelMin || ''),
      channelMax: String(source.channelMax || ''),
      useTime: false,
      startTime: 0,
      stopTime: 0,
      useDuration: boolValue(formValue(form, 'useDuration', source.useDuration), false),
      durationMinMinutes: numberValue(formValue(form, 'durationMinMinutes', source.durationMinMinutes), 0),
      durationMaxMinutes: numberValue(formValue(form, 'durationMaxMinutes', source.durationMaxMinutes), 0),
      useDayOfWeek: false,
      dayOfWeek: 0,
      avoidRepeats: boolValue(formValue(form, 'avoidRepeats', source.avoidRepeats), false),
      allowedRepeats: numberValue(formValue(form, 'allowedRepeats', source.allowedRepeats), 0),
      repeatsWithinDays: numberValue(formValue(form, 'repeatsWithinDays', source.repeatsWithinDays), 0),
      compareTitle: boolValue(formValue(form, 'compareTitle', source.compareTitle), true),
      compareSubtitle: boolValue(formValue(form, 'compareSubtitle', source.compareSubtitle), false),
      compareSummary: boolValue(formValue(form, 'compareSummary', source.compareSummary), false),
      compareCategories: source.compareCategories,
      compareTime: source.compareTime,
      useSeriesRecording: source.useSeriesRecording,
      keepRecordings: source.keepRecordings,
      deleteMode: source.deleteMode,
      searchTimerAction: source.searchTimerAction,
      blacklistMode: numberValue(formValue(form, 'blacklistMode', source.blacklistMode), 0),
      blacklistIds: formValue(form, 'blacklistIds', source.blacklistIds),
      mode: numberValue(formValue(form, 'mode', source.mode), 0),
      matchCase: boolValue(formValue(form, 'matchCase', source.matchCase), false),
      tolerance: numberValue(formValue(form, 'tolerance', source.tolerance), 0),
      summaryMatch: source.summaryMatch,
      useExtendedEpgInfo: source.useExtendedEpgInfo,
      extendedEpgInfo: source.extendedEpgInfo,
      ignoreMissingEpgCategories: source.ignoreMissingEpgCategories,
      contentDescriptors: source.contentDescriptors,
      useInFavorites: source.useInFavorites,
      activeFrom: source.activeFrom,
      activeUntil: source.activeUntil,
      pauseOnRecordings: source.pauseOnRecordings,
      switchMinutesBefore: source.switchMinutesBefore,
      unmuteSoundOnSwitch: source.unmuteSoundOnSwitch,
      deleteRecordingsAfterDays: source.deleteRecordingsAfterDays,
      deleteAfterCountRecordings: source.deleteAfterCountRecordings,
      deleteAfterDaysOfFirstRecording: source.deleteAfterDaysOfFirstRecording
    };
  }

  function validateCreatePayload(payload) {
    const errors = [];
    if (String(payload.query || '').trim() === '') errors.push('Suchbegriff fehlt.');
    if (String(payload.name || '').trim() === '') payload.name = String(payload.query || '').trim();
    if (!payload.compareTitle && !payload.compareSubtitle && !payload.compareSummary) {
      errors.push('Mindestens Titel, Untertitel oder Beschreibung muss durchsucht werden.');
    }
    if (payload.useDuration && payload.durationMaxMinutes > 0 && payload.durationMinMinutes > payload.durationMaxMinutes) {
      errors.push('Minimale Dauer darf nicht größer als maximale Dauer sein.');
    }
    return errors;
  }

  function clientApi() {
    return searchTimerBrowserContext.clientApi || global.VdrSuiteClientApi || null;
  }

  function reloadSearchTimers() {
    if (typeof searchTimerBrowserContext.reload === 'function') {
      searchTimerBrowserContext.reload();
    }
  }

  function setFeedback(target, message, error) {
    target.className = 'searchtimer-feedback ' + (error ? 'error' : 'success');
    target.textContent = String(message || '');
  }

  function resultMessage(result, fallback) {
    if (result && result.message) return String(result.message);
    if (result && Array.isArray(result.errors) && result.errors.length > 0) return result.errors.join(' · ');
    return fallback;
  }

  function formatBoolean(value) {
    return boolValue(value, false) ? 'ja' : 'nein';
  }

  function formatUnixTime(value) {
    const number = Number(value);
    const date = Number.isFinite(number) ? new Date(number * 1000) : new Date(String(value || ''));
    if (Number.isNaN(date.getTime())) return '-';
    return date.toLocaleString('de-DE', {weekday:'short', day:'2-digit', month:'2-digit', hour:'2-digit', minute:'2-digit'});
  }

  function previewMatches(data) {
    return data && data.preview && Array.isArray(data.preview.matches)
      ? data.preview.matches
      : [];
  }

  function appendPreviewMetric(parent, label, value) {
    const item = document.createElement('div');
    item.className = 'searchtimer-preview-metric';
    item.appendChild(addText(document.createElement('span'), label));
    item.appendChild(addText(document.createElement('strong'), value));
    parent.appendChild(item);
  }

  function renderPreview(target, data) {
    target.replaceChildren();
    const summary = document.createElement('section');
    summary.className = 'searchtimer-preview-summary';
    const statistics = data && data.statistics ? data.statistics : {};
    const epgInput = data && data.epgInput ? data.epgInput : {};
    appendPreviewMetric(summary, 'Engine', String(firstValue(data || {}, ['previewEngine'], '-')));
    appendPreviewMetric(summary, 'EPG', String(firstValue(epgInput, ['status'], '-')));
    appendPreviewMetric(summary, 'Treffer', String(firstValue(statistics, ['totalCount'], previewMatches(data).length)));
    appendPreviewMetric(summary, 'Kanäle', String(firstValue(statistics, ['channelCount'], 0)));
    target.appendChild(summary);

    const matches = previewMatches(data);
    if (matches.length === 0) {
      target.appendChild(addText(document.createElement('p'), 'Keine Treffer in der aktuellen Vorschau.'));
    } else {
      matches.slice(0, 40).forEach((match, index) => {
        const event = match && match.event && typeof match.event === 'object' ? match.event : match;
        const card = document.createElement('article');
        card.className = 'searchtimer-preview-match';
        card.appendChild(addText(document.createElement('h5'), String(firstValue(event, ['title', 'name'], 'Treffer ' + String(index + 1)))));
        const badges = document.createElement('div');
        badges.className = 'searchtimer-preview-badges';
        [
          'Kanal: ' + String(firstValue(event, ['channelName', 'channel', 'channelId'], '-')),
          'Start: ' + formatUnixTime(firstValue(event, ['startTime', 'start'], ''))
        ].forEach(text => {
          const badge = addText(document.createElement('span'), text);
          badge.className = 'searchtimer-preview-badge';
          badges.appendChild(badge);
        });
        card.appendChild(badges);
        target.appendChild(card);
      });
    }

    const technical = document.createElement('details');
    technical.className = 'searchtimer-preview-technical';
    technical.appendChild(addText(document.createElement('summary'), 'Technische JSON-Details'));
    const pre = document.createElement('pre');
    pre.textContent = JSON.stringify(data, null, 2).slice(0, 12000);
    technical.appendChild(pre);
    target.appendChild(technical);
  }

  function runPreview(payload, button, target) {
    const api = clientApi();
    if (!api || typeof api.fetchClientSearchTimerPreview !== 'function') {
      setFeedback(target, 'SearchTimer-Vorschau ist nicht verfügbar.', true);
      return Promise.resolve(null);
    }

    button.disabled = true;
    setFeedback(target, 'SearchTimer-Vorschau wird geladen …', false);

    return api.fetchClientSearchTimerPreview({
      backendId: selectedBackendId(),
      query: Object.assign({}, payload, {text: payload.query, query: payload.query}),
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(result => {
      target.className = 'searchtimer-feedback success';
      renderPreview(target, result);
      return result;
    }).catch(error => {
      setFeedback(target, String(error && error.message ? error.message : error), true);
      return null;
    }).finally(() => {
      button.disabled = false;
    });
  }

  function runCreate(payload, button, target) {
    const api = clientApi();
    if (!api || typeof api.fetchClientSearchTimerCreateAction !== 'function') {
      setFeedback(target, 'SearchTimer-Erstellung ist nicht verfügbar.', true);
      return Promise.resolve(null);
    }

    button.disabled = true;
    setFeedback(target, 'SearchTimer wird erstellt …', false);

    return api.fetchClientSearchTimerCreateAction({
      payload,
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(result => {
      if (!result || result.success !== true) {
        throw new Error(resultMessage(result, 'Backend hat die Erstellung abgelehnt.'));
      }
      setFeedback(target, resultMessage(result, 'SearchTimer erstellt.'), false);
      global.setTimeout(reloadSearchTimers, 300);
      return result;
    }).catch(error => {
      setFeedback(target, String(error && error.message ? error.message : error), true);
      return null;
    }).finally(() => {
      button.disabled = false;
    });
  }

  function runDelete(timer, button, target) {
    const api = clientApi();
    if (!api || typeof api.fetchClientSearchTimerDeleteAction !== 'function') {
      setFeedback(target, 'SearchTimer-Löschung ist nicht verfügbar.', true);
      return Promise.resolve(null);
    }

    button.disabled = true;
    setFeedback(target, 'SearchTimer wird gelöscht …', false);

    return api.fetchClientSearchTimerDeleteAction({
      payload: {
        backendId: timer.backendId || selectedBackendId(),
        backendNativeId: timer.backendNativeId
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(result => {
      if (!result || result.success !== true) {
        throw new Error(resultMessage(result, 'Backend hat die Löschung abgelehnt.'));
      }
      setFeedback(target, resultMessage(result, 'SearchTimer gelöscht.'), false);
      global.setTimeout(reloadSearchTimers, 300);
      return result;
    }).catch(error => {
      setFeedback(target, String(error && error.message ? error.message : error), true);
      return null;
    }).finally(() => {
      button.disabled = false;
    });
  }

  function createForm(template) {
    const source = template || normalizeSearchTimer({}, 0);
    const form = document.createElement('form');
    form.className = 'searchtimer-form';

    const grid = document.createElement('div');
    grid.className = 'searchtimer-form-grid';
    grid.appendChild(field('Name', input('name', source.name === 'SearchTimer' ? '' : source.name), true));
    grid.appendChild(field('Suchbegriff', input('query', source.query), true));
    grid.appendChild(check('active', 'Aktiv', template ? source.active : true));
    grid.appendChild(check('useVps', 'VPS/PDC', source.useVps));
    grid.appendChild(check('compareTitle', 'Titel durchsuchen', template ? source.compareTitle : true));
    grid.appendChild(check('compareSubtitle', 'Untertitel durchsuchen', source.compareSubtitle));
    grid.appendChild(check('compareSummary', 'Beschreibung durchsuchen', source.compareSummary));
    grid.appendChild(check('avoidRepeats', 'Duplikate vermeiden', source.avoidRepeats));
    grid.appendChild(field('Verzeichnis', input('directory', source.directory), true));
    grid.appendChild(field('Priorität', input('priority', source.priority, 'number')));
    grid.appendChild(field('Lebensdauer', input('lifetime', source.lifetime, 'number')));
    grid.appendChild(field('Start-Marge (Min.)', input('marginStartMinutes', source.marginStartMinutes, 'number')));
    grid.appendChild(field('Stop-Marge (Min.)', input('marginStopMinutes', source.marginStopMinutes, 'number')));
    grid.appendChild(field('Kanalfilter', input('channels', source.channels), true));
    grid.appendChild(check('useDuration', 'Dauerfilter verwenden', source.useDuration));
    grid.appendChild(field('Dauer min. (Min.)', input('durationMinMinutes', source.durationMinMinutes, 'number')));
    grid.appendChild(field('Dauer max. (Min.)', input('durationMaxMinutes', source.durationMaxMinutes, 'number')));
    grid.appendChild(field('Erlaubte Wiederholungen', input('allowedRepeats', source.allowedRepeats, 'number')));
    grid.appendChild(field('Wiederholungen innerhalb Tage', input('repeatsWithinDays', source.repeatsWithinDays, 'number')));
    grid.appendChild(field('Suchmodus', select('mode', source.mode, [[0,'Phrase'],[1,'Alle Wörter'],[2,'Ein Wort'],[3,'Exakt'],[4,'Regulärer Ausdruck'],[5,'Unscharf']])));
    grid.appendChild(check('matchCase', 'Groß-/Kleinschreibung', source.matchCase));
    grid.appendChild(field('Toleranz', input('tolerance', source.tolerance, 'number')));
    grid.appendChild(field('Blacklist-Modus', input('blacklistMode', source.blacklistMode, 'number')));
    grid.appendChild(field('Blacklist-IDs', input('blacklistIds', source.blacklistIds), true));
    form.appendChild(grid);

    const actions = document.createElement('div');
    actions.className = 'searchtimer-actions';
    const preview = addText(document.createElement('button'), 'Vorschau');
    preview.type = 'button';
    preview.className = 'primary';
    const save = addText(document.createElement('button'), 'SearchTimer erstellen');
    save.type = 'submit';
    actions.appendChild(preview);
    actions.appendChild(save);
    form.appendChild(actions);

    const feedback = document.createElement('div');
    feedback.className = 'searchtimer-feedback';
    feedback.setAttribute('role', 'status');
    feedback.setAttribute('aria-live', 'polite');
    form.appendChild(feedback);

    function payload() {
      const result = buildCreatePayload(form, source);
      const errors = validateCreatePayload(result);
      if (errors.length > 0) {
        setFeedback(feedback, errors.join(' '), true);
        return null;
      }
      return result;
    }

    preview.addEventListener('click', () => {
      const result = payload();
      if (result) runPreview(result, preview, feedback);
    });

    form.addEventListener('submit', event => {
      event.preventDefault();
      const result = payload();
      if (!result) return;
      if (!global.confirm('SearchTimer „' + result.name + '“ wirklich erstellen?')) return;
      runCreate(result, save, feedback);
    });

    activeCreateForm = form;
    return form;
  }

  function populateCreateForm(timer) {
    if (!activeCreateForm) return;
    const values = {
      name: timer.name + ' – Kopie',
      query: timer.query,
      directory: timer.directory,
      priority: timer.priority,
      lifetime: timer.lifetime,
      marginStartMinutes: timer.marginStartMinutes,
      marginStopMinutes: timer.marginStopMinutes,
      channels: timer.channels,
      durationMinMinutes: timer.durationMinMinutes,
      durationMaxMinutes: timer.durationMaxMinutes,
      allowedRepeats: timer.allowedRepeats,
      repeatsWithinDays: timer.repeatsWithinDays,
      mode: timer.mode,
      tolerance: timer.tolerance,
      blacklistMode: timer.blacklistMode,
      blacklistIds: timer.blacklistIds
    };

    Object.keys(values).forEach(name => {
      if (activeCreateForm.elements[name]) activeCreateForm.elements[name].value = String(values[name] || '');
    });
    ['active','useVps','compareTitle','compareSubtitle','compareSummary','avoidRepeats','useDuration','matchCase'].forEach(name => {
      if (activeCreateForm.elements[name]) activeCreateForm.elements[name].checked = Boolean(timer[name]);
    });

    const panel = activeCreateForm.closest('.searchtimer-create-panel');
    if (panel) panel.open = true;
    activeCreateForm.scrollIntoView({behavior:'smooth', block:'start'});
  }

  function appendMeta(parent, label, value) {
    const item = document.createElement('div');
    item.className = 'searchtimer-meta';
    item.appendChild(addText(document.createElement('span'), label));
    item.appendChild(addText(document.createElement('strong'), value === '' ? '-' : value));
    parent.appendChild(item);
  }

  function createCard(timer) {
    const card = document.createElement('details');
    card.className = 'searchtimer-card' + (timer.active ? '' : ' inactive');

    const summary = document.createElement('summary');
    const summaryGrid = document.createElement('div');
    summaryGrid.className = 'searchtimer-card-summary';
    const title = document.createElement('div');
    title.className = 'searchtimer-card-title';
    title.appendChild(addText(document.createElement('strong'), timer.name));
    title.appendChild(addText(document.createElement('span'), timer.query || 'Kein Suchbegriff'));
    summaryGrid.appendChild(title);

    const badges = document.createElement('div');
    badges.className = 'searchtimer-badges';
    const status = addText(document.createElement('span'), timer.active ? 'aktiv' : 'inaktiv');
    status.className = 'searchtimer-badge ' + (timer.active ? 'active' : '');
    badges.appendChild(status);
    if (timer.useVps) {
      const vps = addText(document.createElement('span'), 'VPS');
      vps.className = 'searchtimer-badge vps';
      badges.appendChild(vps);
    }
    if (timer.useChannel || timer.useDuration || timer.avoidRepeats) {
      const filter = addText(document.createElement('span'), 'Filter');
      filter.className = 'searchtimer-badge filter';
      badges.appendChild(filter);
    }
    summaryGrid.appendChild(badges);
    summary.appendChild(summaryGrid);
    card.appendChild(summary);

    const body = document.createElement('div');
    body.className = 'searchtimer-card-body';

    const meta = document.createElement('div');
    meta.className = 'searchtimer-meta-grid';
    appendMeta(meta, 'Backend-ID', timer.backendId);
    appendMeta(meta, 'Native ID', timer.backendNativeId);
    appendMeta(meta, 'Verzeichnis', timer.directory || '-');
    appendMeta(meta, 'Priorität / Lebensdauer', String(timer.priority) + ' / ' + String(timer.lifetime));
    appendMeta(meta, 'Marge Start / Stop', String(timer.marginStartMinutes) + ' / ' + String(timer.marginStopMinutes) + ' Min.');
    appendMeta(meta, 'Suchfelder', [timer.compareTitle ? 'Titel' : '', timer.compareSubtitle ? 'Untertitel' : '', timer.compareSummary ? 'Beschreibung' : ''].filter(Boolean).join(', ') || '-');
    appendMeta(meta, 'Kanäle', timer.channels || 'alle');
    appendMeta(meta, 'Duplikate', timer.avoidRepeats ? 'vermeiden' : 'zulassen');
    body.appendChild(meta);

    const safety = document.createElement('p');
    safety.className = 'searchtimer-safety-note';
    safety.textContent = 'Vorhandene SearchTimer werden noch nicht direkt überschrieben: Der aktuelle Leservertrag liefert Zeitfenster und konkreten Wochentag nicht vollständig zurück. „Als Vorlage“ übernimmt die sicher bekannten Werte in einen neuen SearchTimer.';
    body.appendChild(safety);

    const actions = document.createElement('div');
    actions.className = 'searchtimer-actions';
    const copy = addText(document.createElement('button'), 'Als Vorlage öffnen');
    copy.type = 'button';
    copy.addEventListener('click', () => populateCreateForm(timer));
    actions.appendChild(copy);

    const edit = addText(document.createElement('button'), timer.active ? 'Deaktivieren' : 'Aktivieren');
    edit.type = 'button';
    edit.disabled = true;
    edit.title = 'Sicheres Ändern wird erst freigegeben, wenn der vollständige Leservertrag verfügbar ist.';
    actions.appendChild(edit);

    const remove = addText(document.createElement('button'), 'SearchTimer löschen');
    remove.type = 'button';
    remove.className = 'danger';
    actions.appendChild(remove);
    body.appendChild(actions);

    const feedback = document.createElement('div');
    feedback.className = 'searchtimer-feedback';
    feedback.setAttribute('role', 'status');
    body.appendChild(feedback);

    remove.addEventListener('click', () => {
      if (!global.confirm('SearchTimer „' + timer.name + '“ wirklich löschen?')) return;
      runDelete(timer, remove, feedback);
    });

    const technical = document.createElement('details');
    technical.className = 'searchtimer-preview-technical';
    technical.appendChild(addText(document.createElement('summary'), 'Technische Details'));
    const pre = document.createElement('pre');
    pre.textContent = JSON.stringify(timer.source, null, 2);
    technical.appendChild(pre);
    body.appendChild(technical);

    card.appendChild(body);
    return card;
  }

  function renderList(data) {
    installStyles();
    const mountTarget = searchTimerBrowserContext.detailDataElement;
    if (!mountTarget) throw new Error('SearchTimer browser mount target is not configured');

    const searchTimers = listFromResponse(data, 'searchTimers').map(normalizeSearchTimer);
    mountTarget.replaceChildren();

    const list = document.createElement('section');
    list.className = 'list searchtimer-module';

    const summary = document.createElement('article');
    summary.className = 'module-placeholder searchtimer-summary';
    summary.appendChild(addText(document.createElement('h3'), 'SearchTimer'));
    summary.appendChild(addText(document.createElement('p'), String(searchTimers.length) + ' SearchTimer geladen.'));
    list.appendChild(summary);

    const createPanel = document.createElement('details');
    createPanel.className = 'searchtimer-create-panel';
    createPanel.appendChild(addText(document.createElement('summary'), 'Neuen SearchTimer erstellen'));
    const createBody = document.createElement('div');
    createBody.className = 'searchtimer-create-body';
    createBody.appendChild(createForm(null));
    createPanel.appendChild(createBody);
    list.appendChild(createPanel);

    if (searchTimers.length === 0) {
      const empty = document.createElement('article');
      empty.className = 'module-placeholder';
      empty.appendChild(addText(document.createElement('h3'), 'Keine SearchTimer'));
      empty.appendChild(addText(document.createElement('p'), 'Der Backend-Endpunkt hat keine SearchTimer geliefert.'));
      list.appendChild(empty);
    } else {
      searchTimers.forEach(timer => list.appendChild(createCard(timer)));
    }

    mountTarget.appendChild(list);
  }

  const api = Object.freeze({
    configureContext,
    renderList,
    normalizeSearchTimer,
    buildCreatePayload,
    validateCreatePayload,
    selectedBackendId
  });

  global.VdrSuiteSearchTimerBrowser = api;

  if (global.VdrSuitePlatform &&
      typeof global.VdrSuitePlatform.registerModule === 'function' &&
      typeof global.VdrSuitePlatform.hasModule === 'function' &&
      !global.VdrSuitePlatform.hasModule('searchtimers')) {
    global.VdrSuitePlatform.registerModule('searchtimers', api);
  }
})(window);
