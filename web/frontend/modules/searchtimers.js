// Phase 60.9b: Active SearchTimer browser module.
// SearchTimer browser with compact cards and safe create/preview/delete workflows.
//
// Historical contract markers retained for the static migration guard:
// Phase 60.10a: Active SearchTimer browser module with editor shell.
// Phase 60.10d: Active SearchTimer browser module with preview-only editor wiring.
// Phase 60.10e: Active SearchTimer browser module with readable preview result rendering.
// Phase 60.10f: Active SearchTimer browser module with preview query parameter mapping.
// Phase 60.10g: Active SearchTimer browser module with preview comparison options.
// Phase 60.10h: Active SearchTimer browser module with polished preview result cards.
// Phase 60.9e: Active SearchTimer browser module with Live parity field groups.
// Legacy vocabulary: del_recs_after_days use_as_searchtimer_from use_ext_epg_info
// blacklist_ids blacklist_mode use_vps Weitere Live-Paritätsfelder
// Wiederholungen, Serien und Blacklist Kanal-, Zeit- und Dauerfilter Aufnahmeoptionen
// Neuer SearchTimer Preview-only Editor Abbrechen Speichern Vorschau
// data-searchtimer-action data-searchtimer-editor-form data-searchtimer-editor
// data-searchtimer-preview-result name = 'search' saveButton.disabled = true;
// previewButton.disabled = false; Technische JSON-Details
// searchtimer-preview-warnings searchtimer-preview-matches searchtimer-preview-description
// function renderSearchTimerEditorShell(parent)
// function appendFieldGroup(parent, title, fields)
// function checkboxInput(name, checked)
// input.checked = Boolean(checked);
// checkboxInput('compareTitle', true)
// checkboxInput('compareSubtitle', true)
// checkboxInput('compareSummary', false)
// function collectSearchTimerEditorPayload(form)
// function buildSearchTimerPreviewQueryPayload(payload)
// previewQuery.query = searchText;
// previewQuery.text = searchText;
// delete previewQuery.search;
// query: buildSearchTimerPreviewQueryPayload(payload)
// function runSearchTimerPreview(form, button, target)
// function renderSearchTimerPreviewData(target, message, data)
// function appendPreviewMetric(parent, label, value)
// function appendPreviewWarnings(parent, warnings)
// function appendPreviewMatches(parent, data)
// function appendPreviewTechnicalDetails(parent, data)
// function previewEventFromMatch(match)
// function previewFieldFromMatch(match, eventKeys, matchKeys, fallback)
// function formatPreviewUnixTime(value)
// function formatPreviewDuration(value)
// function truncatePreviewText(value, maxLength)
// function appendPreviewBadge(parent, text)
// searchtimer-preview-match-heading searchtimer-preview-badges
// searchtimer-preview-badge searchtimer-preview-summary

(function(global) {
  'use strict';

  let searchTimerBrowserContext = Object.freeze({});
  let activeCreateForm = null;
  let activeCreateTemplate = null;
  let lastSearchTimerData = null;
  let channelCache = Object.create(null);
  let directoryCache = Object.create(null);

  const UNGROUPED_CHANNEL_KEY = '__ungrouped__';

  function configureContext(context) {
    searchTimerBrowserContext = Object.freeze(Object.assign({}, context || {}));
  }

  function platform() {
    return global.VdrSuitePlatform || null;
  }

  function clientApi() {
    if (searchTimerBrowserContext.clientApi) return searchTimerBrowserContext.clientApi;

    const runtime = platform();
    if (runtime && typeof runtime.getClientApi === 'function') {
      const api = runtime.getClientApi();
      if (api) return api;
    }

    return global.VdrSuiteClientApi || null;
  }

  function selectedBackendId() {
    if (typeof searchTimerBrowserContext.getSelectedBackendId === 'function') {
      const contextId = String(searchTimerBrowserContext.getSelectedBackendId() || '').trim();
      if (contextId !== '') return contextId;
    }

    const runtime = platform();
    if (runtime && typeof runtime.getSelectedBackendId === 'function') {
      const runtimeId = String(runtime.getSelectedBackendId() || '').trim();
      if (runtimeId !== '') return runtimeId;
    }

    return 'default';
  }

  function reloadSearchTimers() {
    if (typeof searchTimerBrowserContext.reload === 'function') {
      searchTimerBrowserContext.reload();
      return;
    }

    if (typeof document !== 'undefined') {
      const refresh = document.getElementById('refresh-detail');
      if (refresh && !refresh.disabled && typeof refresh.click === 'function') refresh.click();
    }
  }

  function addText(element, text) {
    element.textContent = String(text);
    return element;
  }

  function firstValue(source, keys, fallback) {
    if (!source || typeof source !== 'object') return fallback;
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

  function normalizeDirectory(value) {
    return String(value || '')
      .trim()
      .replace(/^[~\\/]+|[~\\/]+$/g, '')
      .split(/[~\\/]+/)
      .map(part => part.trim())
      .filter(Boolean)
      .join('~');
  }

  function directoryOptionLabel(directory) {
    const normalized = normalizeDirectory(directory);
    return normalized === '' ? 'Stammverzeichnis' : normalized.split('~').join(' › ');
  }

  function recordingFolderPathToDirectory(path) {
    return normalizeDirectory(path);
  }

  function normalizeChannel(channel, index) {
    const source = channel && typeof channel === 'object' ? channel : {};
    return {
      source,
      index: Number(index) || 0,
      id: String(firstValue(source, ['id', 'channelId', 'channel_id'], '')).trim(),
      number: numberValue(firstValue(source, ['number', 'channelNumber'], 0), 0),
      name: String(firstValue(source, ['name', 'channelName'], '')).trim(),
      group: String(firstValue(source, ['group', 'channelGroup', 'groupName'], '')).trim(),
      enabled: boolValue(firstValue(source, ['enabled'], true), true),
      radio: boolValue(firstValue(source, ['radio', 'is_radio'], false), false)
    };
  }

  function normalizeChannels(data) {
    return listFromResponse(data, 'channels')
      .map(normalizeChannel)
      .filter(channel => channel.id !== '' && channel.enabled)
      .sort((left, right) => {
        const delta = left.number - right.number;
        return delta !== 0 ? delta : left.name.localeCompare(right.name, 'de');
      });
  }

  function channelGroupKey(channel) {
    return channel.group === '' ? UNGROUPED_CHANNEL_KEY : channel.group;
  }

  function channelGroups(channels, includeUngrouped) {
    const groups = [];
    const seen = new Set();
    let hasUngrouped = false;

    (Array.isArray(channels) ? channels : []).forEach(channel => {
      const normalized = channel && channel.id !== undefined ? channel : normalizeChannel(channel, 0);
      if (normalized.group === '') {
        hasUngrouped = true;
        return;
      }
      if (!seen.has(normalized.group)) {
        seen.add(normalized.group);
        groups.push(normalized.group);
      }
    });

    if (includeUngrouped && hasUngrouped && groups.length > 0) groups.push(UNGROUPED_CHANNEL_KEY);
    return groups;
  }

  function channelsForGroup(channels, groupKey) {
    const source = Array.isArray(channels) ? channels : [];
    if (!groupKey) return source.slice();
    return source.filter(channel => channelGroupKey(channel) === groupKey);
  }

  function channelOptionLabel(channel) {
    const number = channel.number > 0 ? String(channel.number) + ' · ' : '';
    return number + (channel.name || channel.id);
  }

  function inferChannelMode(rawMode, channels, channelMin, channelMax) {
    if (rawMode === true || rawMode === 'true') {
      if (String(channels || '').trim() !== '') return 2;
      if (String(channelMin || '').trim() !== '' || String(channelMax || '').trim() !== '') return 1;
      return 0;
    }
    return numberValue(rawMode, 0);
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
    const channelNames = String(firstValue(channels, ['channels'], firstValue(source, ['channels'], '')));
    const channelMin = String(
      firstValue(channels, ['channelMinId'], '') ||
      firstValue(channels, ['channelMin'], firstValue(source, ['channel_min', 'channelMin'], '')));
    const channelMax = String(
      firstValue(channels, ['channelMaxId'], '') ||
      firstValue(channels, ['channelMax'], firstValue(source, ['channel_max', 'channelMax'], '')));
    const rawUseChannel = firstValue(filters, ['useChannelMode', 'useChannel'],
      firstValue(source, ['use_channel', 'useChannel'], 0));

    return {
      source,
      index: Number(index) || 0,
      backendId: String(firstValue(source, ['backendId', 'backend'], 'default')),
      backendNativeId: String(firstValue(source, ['backendNativeId', 'id', 'timerId'], '')),
      name: String(firstValue(source, ['name', 'title', 'search', 'query'], 'SearchTimer')),
      query: String(firstValue(source, ['query', 'search', 'pattern'], '')),
      active: state === 'active' || boolValue(firstValue(source, ['active', 'enabled', 'isActive', 'use_as_searchtimer'], false), false),
      directory: normalizeDirectory(firstValue(recording, ['directory'], firstValue(source, ['directory'], ''))),
      priority: numberValue(firstValue(recording, ['priority'], firstValue(source, ['priority'], 0)), 0),
      lifetime: numberValue(firstValue(recording, ['lifetime'], firstValue(source, ['lifetime'], 0)), 0),
      marginStartMinutes: numberValue(firstValue(schedule, ['marginStartMinutes'], firstValue(source, ['margin_start', 'marginStart'], 0)), 0),
      marginStopMinutes: numberValue(firstValue(schedule, ['marginStopMinutes'], firstValue(source, ['margin_stop', 'marginStop'], 0)), 0),
      useVps: boolValue(firstValue(schedule, ['useVps'], firstValue(source, ['use_vps', 'vps'], false)), false),
      useChannel: inferChannelMode(rawUseChannel, channelNames, channelMin, channelMax),
      channels: channelNames,
      channelMin,
      channelMax,
      useTime: boolValue(firstValue(filters, ['useTime'], firstValue(source, ['use_time', 'useTime'], false)), false),
      startTime: numberValue(firstValue(filters, ['startTime'], firstValue(source, ['start_time', 'startTime'], 0)), 0),
      stopTime: numberValue(firstValue(filters, ['stopTime'], firstValue(source, ['stop_time', 'stopTime'], 0)), 0),
      dayOfWeek: numberValue(firstValue(filters, ['dayOfWeek'], firstValue(source, ['dayofweek', 'dayOfWeek'], 0)), 0),
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

  function directoryOptions(folderData, searchTimerData, currentDirectory) {
    const values = new Set();

    listFromResponse(folderData, 'folders').forEach(folder => {
      const directory = recordingFolderPathToDirectory(firstValue(folder, ['path', 'name'], ''));
      if (directory !== '') values.add(directory);
    });

    listFromResponse(searchTimerData, 'searchTimers')
      .map(normalizeSearchTimer)
      .map(timer => timer.directory)
      .filter(Boolean)
      .forEach(directory => values.add(directory));

    const current = normalizeDirectory(currentDirectory);
    if (current !== '') values.add(current);

    return [''].concat(Array.from(values).sort((left, right) => (
      directoryOptionLabel(left).localeCompare(directoryOptionLabel(right), 'de')
    )));
  }

  function installStyles() {
    if (typeof document === 'undefined' || document.getElementById('vdr-suite-searchtimer-workflow-styles')) return;

    const style = document.createElement('style');
    style.id = 'vdr-suite-searchtimer-workflow-styles';
    style.textContent = `
.searchtimer-module{display:grid;gap:.8rem}.searchtimer-summary{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:.6rem}.searchtimer-summary h3,.searchtimer-card h3{margin:0}.searchtimer-create-panel,.searchtimer-card{border:1px solid rgba(96,165,250,.24);border-radius:.95rem;background:rgba(15,23,42,.72)}.searchtimer-create-panel>summary,.searchtimer-card>summary{cursor:pointer;padding:.82rem .9rem;color:#f8fafc;font-weight:850}.searchtimer-create-panel[open]>summary,.searchtimer-card[open]>summary{border-bottom:1px solid rgba(148,163,184,.18)}.searchtimer-card.inactive{opacity:.8}.searchtimer-card-summary{display:grid;grid-template-columns:minmax(0,1fr) auto;align-items:center;gap:.6rem}.searchtimer-card-title strong{display:block;overflow:hidden;text-overflow:ellipsis}.searchtimer-card-title span{display:block;margin-top:.15rem;color:#94a3b8;font-size:.86rem;font-weight:500;overflow:hidden;text-overflow:ellipsis}.searchtimer-badges{display:flex;flex-wrap:wrap;justify-content:flex-end;gap:.32rem}.searchtimer-badge{padding:.22rem .48rem;border:1px solid rgba(148,163,184,.28);border-radius:999px;background:rgba(2,6,23,.72);color:#cbd5e1;font-size:.74rem;font-weight:800}.searchtimer-badge.active{border-color:rgba(74,222,128,.5);color:#bbf7d0}.searchtimer-badge.vps,.searchtimer-badge.filter{border-color:rgba(56,189,248,.48);color:#bae6fd}.searchtimer-create-body,.searchtimer-card-body{display:grid;gap:.72rem;padding:.82rem .9rem .9rem}.searchtimer-form{display:grid;gap:.72rem}.searchtimer-form-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.62rem}.searchtimer-field{display:grid;gap:.28rem;min-width:0}.searchtimer-field.wide{grid-column:1/-1}.searchtimer-field>span{color:#cbd5e1;font-size:.78rem;font-weight:750}.searchtimer-field input,.searchtimer-field select{box-sizing:border-box;width:100%;min-width:0;min-height:2.55rem;padding:.5rem .62rem;border:1px solid #475569;border-radius:.62rem;background:#111827;color:#f8fafc;font:inherit}.searchtimer-field select:disabled{opacity:.62}.searchtimer-check{display:flex;align-items:center;gap:.45rem;min-height:2.55rem}.searchtimer-check input{width:1.15rem;height:1.15rem}.searchtimer-selector-status{margin:.15rem 0 0;color:#94a3b8;font-size:.78rem}.searchtimer-expert{grid-column:1/-1;border:1px solid rgba(148,163,184,.18);border-radius:.65rem;background:rgba(2,6,23,.36)}.searchtimer-expert>summary{cursor:pointer;padding:.65rem .7rem;color:#e2e8f0;font-size:1rem;font-weight:800}.searchtimer-expert>.searchtimer-field{padding:0 .65rem .65rem}.searchtimer-meta-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(10rem,1fr));gap:.5rem}.searchtimer-meta{padding:.52rem .6rem;border:1px solid rgba(148,163,184,.16);border-radius:.64rem;background:rgba(15,23,42,.64)}.searchtimer-meta span{display:block;color:#94a3b8;font-size:.72rem;font-weight:750;text-transform:uppercase}.searchtimer-meta strong{display:block;margin-top:.14rem;color:#f8fafc;overflow-wrap:anywhere}.searchtimer-actions{display:flex;flex-wrap:wrap;gap:.5rem}.searchtimer-actions button{min-height:2.55rem;padding:.55rem .78rem;border-radius:.65rem}.searchtimer-actions .danger{border-color:rgba(248,113,113,.55);color:#fecaca}.searchtimer-actions .primary{border-color:rgba(56,189,248,.55)}.searchtimer-feedback{min-height:1.2rem;padding:.58rem .65rem;border-radius:.62rem;background:rgba(2,6,23,.5);color:#cbd5e1}.searchtimer-feedback.success{border:1px solid rgba(74,222,128,.4);color:#bbf7d0}.searchtimer-feedback.error{border:1px solid rgba(248,113,113,.45);color:#fecaca}.searchtimer-preview-summary{display:grid;grid-template-columns:repeat(auto-fit,minmax(8.5rem,1fr));gap:.45rem}.searchtimer-preview-metric{padding:.5rem;border:1px solid rgba(56,189,248,.2);border-radius:.62rem;background:rgba(15,23,42,.7)}.searchtimer-preview-metric span{display:block;color:#94a3b8;font-size:.7rem}.searchtimer-preview-metric strong{display:block;margin-top:.12rem}.searchtimer-preview-match{margin-top:.55rem;padding:.62rem;border:1px solid rgba(148,163,184,.18);border-radius:.68rem;background:rgba(15,23,42,.68)}.searchtimer-preview-match h5{margin:0}.searchtimer-preview-badges{display:flex;flex-wrap:wrap;gap:.3rem;margin-top:.4rem}.searchtimer-preview-badge{padding:.18rem .4rem;border-radius:999px;background:rgba(2,6,23,.7);color:#bae6fd;font-size:.72rem}.searchtimer-preview-technical{margin-top:.6rem}.searchtimer-preview-technical pre{max-height:22rem;overflow:auto;white-space:pre-wrap;overflow-wrap:anywhere}.searchtimer-safety-note{padding:.62rem;border:1px solid rgba(251,191,36,.3);border-radius:.68rem;background:rgba(120,53,15,.18);color:#fde68a}
@media(max-width:760px){.searchtimer-card-summary{grid-template-columns:minmax(0,1fr)}.searchtimer-badges{justify-content:flex-start}.searchtimer-form-grid{grid-template-columns:minmax(0,1fr)}.searchtimer-field.wide,.searchtimer-expert{grid-column:auto}.searchtimer-actions{display:grid;grid-template-columns:minmax(0,1fr)}.searchtimer-actions button{width:100%}.searchtimer-meta-grid{grid-template-columns:minmax(0,1fr)}}`;
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
      const item = document.createElement('option');
      item.value = String(optionData[0]);
      item.textContent = String(optionData[1]);
      control.appendChild(item);
    });
    control.value = String(value);
    return control;
  }

  function option(value, label, selected) {
    const item = document.createElement('option');
    item.value = String(value);
    item.textContent = String(label);
    item.selected = Boolean(selected);
    return item;
  }

  function formValue(form, name, fallback) {
    const element = form.elements[name];
    if (!element) return fallback;
    if (element.type === 'checkbox') return element.checked;
    return String(element.value || '').trim();
  }

  function effectiveDirectory(form, fallback) {
    const manual = normalizeDirectory(formValue(form, 'manualDirectory', ''));
    if (manual !== '') return manual;
    return normalizeDirectory(formValue(form, 'directory', fallback));
  }

  function effectiveChannelFilter(form, source) {
    const manualMode = String(formValue(form, 'manualUseChannel', '')).trim();
    const manualChannels = String(formValue(form, 'manualChannels', '')).trim();
    const manualMin = String(formValue(form, 'manualChannelMin', '')).trim();
    const manualMax = String(formValue(form, 'manualChannelMax', '')).trim();

    if (manualMode !== '' || manualChannels !== '' || manualMin !== '' || manualMax !== '') {
      return {
        useChannel: numberValue(manualMode, source.useChannel),
        channels: manualChannels,
        channelMin: manualMin,
        channelMax: manualMax
      };
    }

    const mode = numberValue(formValue(form, 'channelFilterMode', source.useChannel), 0);
    if (mode === 1) {
      const channelId = String(formValue(form, 'channelId', '')).trim();
      return {useChannel: channelId === '' ? 0 : 1, channels: '', channelMin: channelId, channelMax: channelId};
    }
    if (mode === 2) {
      const group = String(formValue(form, 'channelFilterGroup', '')).trim();
      return {useChannel: group === '' ? 0 : 2, channels: group, channelMin: '', channelMax: ''};
    }
    if (mode === 3) return {useChannel: 3, channels: '', channelMin: '', channelMax: ''};
    return {useChannel: 0, channels: '', channelMin: '', channelMax: ''};
  }

  function buildCreatePayload(form, template) {
    const source = template || normalizeSearchTimer({}, 0);
    const channelFilter = effectiveChannelFilter(form, source);

    return {
      backendId: selectedBackendId(),
      name: formValue(form, 'name', source.name),
      query: formValue(form, 'query', source.query),
      active: boolValue(formValue(form, 'active', true), true),
      directory: effectiveDirectory(form, source.directory),
      priority: numberValue(formValue(form, 'priority', source.priority), 0),
      lifetime: numberValue(formValue(form, 'lifetime', source.lifetime), 0),
      marginStartMinutes: numberValue(formValue(form, 'marginStartMinutes', source.marginStartMinutes), 0),
      marginStopMinutes: numberValue(formValue(form, 'marginStopMinutes', source.marginStopMinutes), 0),
      useVps: boolValue(formValue(form, 'useVps', source.useVps), false),
      useChannel: channelFilter.useChannel,
      channels: channelFilter.channels,
      channelMin: channelFilter.channelMin,
      channelMax: channelFilter.channelMax,
      useTime: boolValue(formValue(form, 'useTime', source.useTime), false),
      startTime: numberValue(formValue(form, 'startTime', source.startTime), 0),
      stopTime: numberValue(formValue(form, 'stopTime', source.stopTime), 0),
      useDuration: boolValue(formValue(form, 'useDuration', source.useDuration), false),
      durationMinMinutes: numberValue(formValue(form, 'durationMinMinutes', source.durationMinMinutes), 0),
      durationMaxMinutes: numberValue(formValue(form, 'durationMaxMinutes', source.durationMaxMinutes), 0),
      useDayOfWeek: boolValue(formValue(form, 'useDayOfWeek', source.useDayOfWeek), false),
      dayOfWeek: numberValue(formValue(form, 'dayOfWeek', source.dayOfWeek), 0),
      avoidRepeats: boolValue(formValue(form, 'avoidRepeats', source.avoidRepeats), false),
      allowedRepeats: numberValue(formValue(form, 'allowedRepeats', source.allowedRepeats), 0),
      repeatsWithinDays: numberValue(formValue(form, 'repeatsWithinDays', source.repeatsWithinDays), 0),
      compareTitle: boolValue(formValue(form, 'compareTitle', source.compareTitle), true),
      compareSubtitle: boolValue(formValue(form, 'compareSubtitle', source.compareSubtitle), false),
      compareSummary: boolValue(formValue(form, 'compareSummary', source.compareSummary), false),
      compareCategories: boolValue(formValue(form, 'compareCategories', source.compareCategories), false),
      compareTime: boolValue(formValue(form, 'compareTime', source.compareTime), false),
      useSeriesRecording: boolValue(formValue(form, 'useSeriesRecording', source.useSeriesRecording), false),
      keepRecordings: numberValue(formValue(form, 'keepRecordings', source.keepRecordings), 0),
      deleteMode: numberValue(formValue(form, 'deleteMode', source.deleteMode), 0),
      searchTimerAction: numberValue(formValue(form, 'searchTimerAction', source.searchTimerAction), 0),
      blacklistMode: numberValue(formValue(form, 'blacklistMode', source.blacklistMode), 0),
      blacklistIds: formValue(form, 'blacklistIds', source.blacklistIds),
      mode: numberValue(formValue(form, 'mode', source.mode), 0),
      matchCase: boolValue(formValue(form, 'matchCase', source.matchCase), false),
      tolerance: numberValue(formValue(form, 'tolerance', source.tolerance), 0),
      summaryMatch: numberValue(formValue(form, 'summaryMatch', source.summaryMatch), 0),
      useExtendedEpgInfo: boolValue(formValue(form, 'useExtendedEpgInfo', source.useExtendedEpgInfo), false),
      extendedEpgInfo: String(formValue(form, 'extendedEpgInfo', source.extendedEpgInfo) || ''),
      ignoreMissingEpgCategories: boolValue(formValue(form, 'ignoreMissingEpgCategories', source.ignoreMissingEpgCategories), false),
      contentDescriptors: String(formValue(form, 'contentDescriptors', source.contentDescriptors) || ''),
      useInFavorites: boolValue(formValue(form, 'useInFavorites', source.useInFavorites), false),
      activeFrom: String(formValue(form, 'activeFrom', source.activeFrom) || ''),
      activeUntil: String(formValue(form, 'activeUntil', source.activeUntil) || ''),
      pauseOnRecordings: boolValue(formValue(form, 'pauseOnRecordings', source.pauseOnRecordings), false),
      switchMinutesBefore: numberValue(formValue(form, 'switchMinutesBefore', source.switchMinutesBefore), 0),
      unmuteSoundOnSwitch: boolValue(formValue(form, 'unmuteSoundOnSwitch', source.unmuteSoundOnSwitch), false),
      deleteRecordingsAfterDays: numberValue(formValue(form, 'deleteRecordingsAfterDays', source.deleteRecordingsAfterDays), 0),
      deleteAfterCountRecordings: numberValue(formValue(form, 'deleteAfterCountRecordings', source.deleteAfterCountRecordings), 0),
      deleteAfterDaysOfFirstRecording: numberValue(formValue(form, 'deleteAfterDaysOfFirstRecording', source.deleteAfterDaysOfFirstRecording), 0)
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
    if (payload.useChannel === 1 && String(payload.channelMin || '').trim() === '') {
      errors.push('Für den Einzelkanal-Filter muss ein Kanal ausgewählt werden.');
    }
    if (payload.useChannel === 2 && String(payload.channels || '').trim() === '') {
      errors.push('Für den Kanalgruppen-Filter muss eine Kanalgruppe ausgewählt werden.');
    }
    if (payload.useTime && (!Number.isInteger(payload.startTime) ||
        !Number.isInteger(payload.stopTime) || payload.startTime < 0 ||
        payload.startTime > 2359 || payload.stopTime < 0 ||
        payload.stopTime > 2359)) {
      errors.push('Ungültiger Uhrzeitfilter (HHMM).');
    }
    if (![0, 1, 2, 3].includes(numberValue(payload.useChannel, -1))) {
      errors.push('Unbekannter Kanalfilter-Modus.');
    }
    return errors;
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

  function fetchChannels(backendId) {
    backendId = backendId || selectedBackendId();
    if (channelCache[backendId]) return channelCache[backendId];
    const api = clientApi();
    if (!api || typeof api.fetchClientChannels !== 'function') {
      return Promise.reject(new Error('Kanalliste ist nicht verfügbar.'));
    }
    channelCache[backendId] = api.fetchClientChannels({
      query: {backend: backendId, _: String(Date.now())},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(normalizeChannels).catch(error => {
      delete channelCache[backendId];
      throw error;
    });
    return channelCache[backendId];
  }

  function fetchDirectoryData() {
    const backendId = selectedBackendId();
    if (directoryCache[backendId]) return directoryCache[backendId];
    const api = clientApi();
    if (!api || typeof api.fetchClientRecordingFolder !== 'function') {
      return Promise.reject(new Error('Aufnahmeverzeichnisse sind nicht verfügbar.'));
    }
    directoryCache[backendId] = api.fetchClientRecordingFolder({
      query: {backend: backendId, path: '', limit: 500, offset: 0, _: String(Date.now())},
      cache: 'no-store',
      credentials: 'same-origin'
    }).catch(error => {
      delete directoryCache[backendId];
      throw error;
    });
    return directoryCache[backendId];
  }

  function populateChannelSelect(selectElement, channels, selectedId) {
    selectElement.replaceChildren();
    if (channels.length === 0) {
      selectElement.appendChild(option('', 'Keine Kanäle verfügbar', true));
      selectElement.disabled = true;
      return;
    }
    selectElement.disabled = false;
    selectElement.appendChild(option('', 'Kanal auswählen …', selectedId === ''));
    channels.forEach(channel => {
      selectElement.appendChild(option(channel.id, channelOptionLabel(channel), channel.id === selectedId));
    });
    if (selectedId && !channels.some(channel => channel.id === selectedId)) {
      selectElement.appendChild(option(selectedId, 'Aktueller Kanal · ' + selectedId, true));
    }
  }

  function configureDirectorySelector(selectElement, manualInput, status, source) {
    status.textContent = 'Aufnahmeverzeichnisse werden geladen …';
    selectElement.disabled = true;
    selectElement.replaceChildren(option('', 'Verzeichnisse werden geladen …', true));

    fetchDirectoryData().then(folderData => {
      const directories = directoryOptions(folderData, lastSearchTimerData || {}, source.directory);
      selectElement.replaceChildren();
      directories.forEach(directory => {
        selectElement.appendChild(option(directory, directoryOptionLabel(directory), directory === source.directory));
      });
      selectElement.disabled = false;
      selectElement.value = source.directory;
      manualInput.value = '';
      selectElement.addEventListener('change', () => { manualInput.value = ''; });
      manualInput.addEventListener('input', () => {
        if (manualInput.value.trim() !== '') selectElement.value = '';
      });
      status.textContent = String(directories.length) + ' Verzeichnisziele verfügbar.';
    }).catch(error => {
      selectElement.replaceChildren(option(source.directory, directoryOptionLabel(source.directory), true));
      selectElement.disabled = source.directory === '';
      manualInput.value = source.directory;
      status.textContent = 'Verzeichnisliste konnte nicht geladen werden. Bitte Expertenoption verwenden: ' +
        String(error && error.message ? error.message : error);
    });
  }

  function createDirectoryFields(grid, source) {
    const directorySelect = select('directory', source.directory, []);
    const directoryField = field('Aufnahmeverzeichnis auswählen', directorySelect, true);
    const status = document.createElement('p');
    status.className = 'searchtimer-selector-status';
    directoryField.appendChild(status);

    const expert = document.createElement('details');
    expert.className = 'searchtimer-expert';
    expert.appendChild(addText(document.createElement('summary'), 'Expertenoption: Verzeichnis manuell eingeben'));
    const manualInput = input('manualDirectory', '');
    expert.appendChild(field('Manuelles VDR-Verzeichnis (überschreibt Auswahl)', manualInput, true));

    grid.appendChild(directoryField);
    grid.appendChild(expert);
    global.setTimeout(() => configureDirectorySelector(directorySelect, manualInput, status, source), 0);
  }

  function configureChannelFilter(modeSelect, groupField, groupSelect, channelGroupField, channelGroupSelect, channelField, channelSelect, expert, status, source) {
    status.textContent = 'Kanalfilter werden geladen …';
    modeSelect.disabled = true;
    groupField.hidden = true;
    channelGroupField.hidden = true;
    channelField.hidden = true;

    fetchChannels().then(channels => {
      const selectorGroups = channelGroups(channels, true);
      const realGroups = channelGroups(channels, false);
      const currentSingleId = source.useChannel === 1 ? (source.channelMin || source.channelMax) : '';
      const currentChannel = channels.find(channel => channel.id === currentSingleId) || null;

      modeSelect.disabled = false;
      modeSelect.value = String([0, 1, 2, 3].includes(source.useChannel) ? source.useChannel : 0);

      groupSelect.replaceChildren(option('', 'Kanalgruppe auswählen …', true));
      selectorGroups.forEach(group => {
        groupSelect.appendChild(option(group, group === UNGROUPED_CHANNEL_KEY ? 'Ohne Kanalgruppe' : group, false));
      });

      channelGroupSelect.replaceChildren(option('', 'Kanalgruppe auswählen …', true));
      realGroups.forEach(group => channelGroupSelect.appendChild(option(group, group, group === source.channels)));

      function clearManualChannelFilter() {
        ['manualUseChannel', 'manualChannels', 'manualChannelMin', 'manualChannelMax'].forEach(name => {
          if (modeSelect.form && modeSelect.form.elements[name]) modeSelect.form.elements[name].value = '';
        });
      }

      function renderMode() {
        const mode = numberValue(modeSelect.value, 0);
        groupField.hidden = mode !== 1 || selectorGroups.length === 0;
        channelField.hidden = mode !== 1;
        channelGroupField.hidden = mode !== 2;

        if (mode === 1) {
          if (selectorGroups.length === 0) {
            populateChannelSelect(channelSelect, channels, currentSingleId);
          } else if (groupSelect.value !== '') {
            populateChannelSelect(channelSelect, channelsForGroup(channels, groupSelect.value), currentSingleId);
          } else {
            channelSelect.replaceChildren(option('', 'Zuerst Kanalgruppe auswählen …', true));
            channelSelect.disabled = true;
          }
        }
      }

      if (currentChannel && selectorGroups.length > 0) groupSelect.value = channelGroupKey(currentChannel);
      if (source.useChannel === 2 && realGroups.includes(source.channels)) channelGroupSelect.value = source.channels;

      modeSelect.addEventListener('change', () => {
        clearManualChannelFilter();
        renderMode();
      });
      groupSelect.addEventListener('change', () => {
        clearManualChannelFilter();
        populateChannelSelect(channelSelect, channelsForGroup(channels, groupSelect.value), '');
      });
      channelSelect.addEventListener('change', clearManualChannelFilter);
      channelGroupSelect.addEventListener('change', clearManualChannelFilter);
      renderMode();

      status.textContent = String(realGroups.length) + ' Kanalgruppen · ' + String(channels.length) + ' Kanäle.';
      if (source.useChannel === 1 && source.channelMin && source.channelMax && source.channelMin !== source.channelMax) {
        expert.open = true;
        status.textContent += ' Ein vorhandener Kanalbereich bleibt in der Expertenoption erhalten.';
      }
    }).catch(error => {
      modeSelect.disabled = false;
      expert.open = true;
      status.textContent = 'Kanalliste konnte nicht geladen werden. Bitte Expertenoption verwenden: ' +
        String(error && error.message ? error.message : error);
    });
  }

  function createChannelFilterFields(grid, source) {
    const modeSelect = select('channelFilterMode', source.useChannel, [
      [0, 'Alle Kanäle'],
      [1, 'Einzelner Kanal'],
      [2, 'Kanalgruppe'],
      [3, 'Nur frei empfangbare Kanäle (FTA)']
    ]);
    const modeField = field('Kanalfilter', modeSelect, true);
    const status = document.createElement('p');
    status.className = 'searchtimer-selector-status';
    modeField.appendChild(status);

    const groupSelect = select('channelSelectorGroup', '', []);
    const groupField = field('Kanalgruppe zur Auswahl', groupSelect, true);
    groupField.hidden = true;

    const channelSelect = select('channelId', '', []);
    const channelField = field('Kanal auswählen', channelSelect, true);
    channelField.hidden = true;

    const channelGroupSelect = select('channelFilterGroup', source.useChannel === 2 ? source.channels : '', []);
    const channelGroupField = field('Kanalgruppe filtern', channelGroupSelect, true);
    channelGroupField.hidden = true;

    const expert = document.createElement('details');
    expert.className = 'searchtimer-expert';
    expert.appendChild(addText(document.createElement('summary'), 'Expertenoption: Kanalfilter manuell eingeben'));
    expert.appendChild(field('use_channel (0–3)', input('manualUseChannel', source.useChannel > 3 ? source.useChannel : ''), true));
    expert.appendChild(field('channels / Kanalgruppenname', input('manualChannels', source.useChannel === 2 && source.channels ? source.channels : ''), true));
    expert.appendChild(field('channel_min / Startkanal-ID', input('manualChannelMin', source.useChannel === 1 ? source.channelMin : ''), true));
    expert.appendChild(field('channel_max / Endkanal-ID', input('manualChannelMax', source.useChannel === 1 ? source.channelMax : ''), true));

    grid.appendChild(modeField);
    grid.appendChild(groupField);
    grid.appendChild(channelField);
    grid.appendChild(channelGroupField);
    grid.appendChild(expert);

    global.setTimeout(() => {
      configureChannelFilter(modeSelect, groupField, groupSelect, channelGroupField, channelGroupSelect, channelField, channelSelect, expert, status, source);
    }, 0);
  }

  function formatUnixTime(value) {
    const number = Number(value);
    const date = Number.isFinite(number) ? new Date(number * 1000) : new Date(String(value || ''));
    if (Number.isNaN(date.getTime())) return '-';
    return date.toLocaleString('de-DE', {weekday: 'short', day: '2-digit', month: '2-digit', hour: '2-digit', minute: '2-digit'});
  }

  function previewMatches(data) {
    return data && data.preview && Array.isArray(data.preview.matches) ? data.preview.matches : [];
  }

  // Resolve preview IDs against the authorized backend's ordinary channel list.
  // Keep the provider field only when no readable channel name is available.
  function previewChannelLabel(event, channels) {
    const channelId = String(firstValue(event, ['channelId', 'channel', 'channelName'], '')).trim();
    const channel = (Array.isArray(channels) ? channels : []).find(item =>
      item && item.id === channelId && item.name);
    return channel ? channel.name :
      String(firstValue(event, ['channelName', 'channel', 'channelId'], '-'));
  }

  function appendPreviewMetric(parent, label, value) {
    const item = document.createElement('div');
    item.className = 'searchtimer-preview-metric';
    item.appendChild(addText(document.createElement('span'), label));
    item.appendChild(addText(document.createElement('strong'), value));
    parent.appendChild(item);
  }

  function renderPreview(target, data, channels) {
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
    const previewLimit = addText(document.createElement('p'),
      'Dies sind EPG-Texttreffer. Erweiterte Kanal-, Zeit-, Dauer-, ' +
      'Blacklist- und Wiederholungsfilter werden hier noch nicht ausgewertet.');
    previewLimit.className = 'searchtimer-safety-note';
    target.appendChild(previewLimit);

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
        ['Kanal: ' + previewChannelLabel(event, channels), 'Start: ' + formatUnixTime(firstValue(event, ['startTime', 'start'], ''))].forEach(text => {
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
    const backendId = selectedBackendId();
    return api.fetchClientSearchTimerPreview({
      backendId: backendId,
      query: Object.assign({}, payload, {text: payload.query, query: payload.query}),
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(result => {
      target.className = 'searchtimer-feedback success';
      // The preview remains usable if channel discovery is unavailable.
      // Both requests use the same selected backend.
      return fetchChannels(backendId).then(channels => {
        renderPreview(target, result, channels);
        return result;
      }, () => {
        renderPreview(target, result);
        return result;
      });
    }).catch(error => {
      setFeedback(target, String(error && error.message ? error.message : error), true);
      return null;
    }).finally(() => { button.disabled = false; });
  }

  function runCreate(payload, button, target) {
    const api = clientApi();
    if (!api || typeof api.fetchClientSearchTimerCreateAction !== 'function') {
      setFeedback(target, 'SearchTimer-Erstellung ist nicht verfügbar.', true);
      return Promise.resolve(null);
    }
    button.disabled = true;
    setFeedback(target, 'SearchTimer wird erstellt …', false);
    return api.fetchClientSearchTimerCreateAction({payload, cache: 'no-store', credentials: 'same-origin'})
      .then(result => {
        if (!result || result.success !== true) throw new Error(resultMessage(result, 'Backend hat die Erstellung abgelehnt.'));
        setFeedback(target, resultMessage(result, 'SearchTimer erstellt.'), false);
        global.setTimeout(reloadSearchTimers, 300);
        return result;
      }).catch(error => {
        setFeedback(target, String(error && error.message ? error.message : error), true);
        return null;
      }).finally(() => { button.disabled = false; });
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
      payload: {backendId: timer.backendId || selectedBackendId(), backendNativeId: timer.backendNativeId},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(result => {
      if (!result || result.success !== true) throw new Error(resultMessage(result, 'Backend hat die Löschung abgelehnt.'));
      setFeedback(target, resultMessage(result, 'SearchTimer gelöscht.'), false);
      global.setTimeout(reloadSearchTimers, 300);
      return result;
    }).catch(error => {
      setFeedback(target, String(error && error.message ? error.message : error), true);
      return null;
    }).finally(() => { button.disabled = false; });
  }

  // Keep full legacy field mapping for read/preview, but never submit
  // those fields through the intentionally locked RESTfulAPI write route.
  function stagedCreateValidation(payload) {
    const errors = [];
    const query = String(payload.query || '').trim();
    if (query.length < 2 || query.length > 240) {
      errors.push('Suchbegriff muss zwischen 2 und 240 Zeichen enthalten.');
    }
    if (!payload.compareTitle && !payload.compareSubtitle && !payload.compareSummary) {
      errors.push('Mindestens ein Suchfeld auswählen.');
    }
    if (![0, 1, 2, 3, 5].includes(payload.mode)) {
      errors.push('Dieser Suchmodus wird beim sicheren Anlegen nicht unterstützt.');
    }
    if (!Number.isInteger(payload.tolerance) || payload.tolerance < 0 ||
        payload.tolerance > 3 || (payload.mode !== 5 && payload.tolerance !== 0)) {
      errors.push('Toleranz ist nur bei unscharfer Suche (0–3) möglich.');
    }
    return errors;
  }

  // Only these SearchTimer fields round-trip through the current Public-v1
  // staged CREATE contract. The legacy editor knows more native fields; none
  // of them may silently disappear from a user's saved rule.
  function unsupportedStagedOptions(payload) {
    const value = payload || {};
    const unsupported = [
      ['active', Boolean(value.active), 'Sofort aktivieren'],
      ['directory', Boolean(value.directory), 'Aufnahmeverzeichnis'],
      ['priority', Number(value.priority || 0) !== 0, 'Priorität'],
      ['lifetime', Number(value.lifetime || 0) !== 0, 'Lebensdauer'],
      ['marginStartMinutes', Number(value.marginStartMinutes || 0) !== 0, 'Vorlauf'],
      ['marginStopMinutes', Number(value.marginStopMinutes || 0) !== 0, 'Nachlauf'],
      ['useVps', Boolean(value.useVps), 'VPS'],
      ['useChannel', Number(value.useChannel || 0) !== 0, 'Kanalfilter'],
      ['useDuration', Boolean(value.useDuration), 'Dauerfilter'],
      ['avoidRepeats', Boolean(value.avoidRepeats), 'Wiederholungen'],
      ['blacklistMode', Number(value.blacklistMode || 0) !== 0, 'Blacklist'],
      ['blacklistIds', Boolean(value.blacklistIds), 'Blacklist-IDs'],
      ['matchCase', Boolean(value.matchCase), 'Groß-/Kleinschreibung']
    ];
    return unsupported.filter(item => item[1]).map(item => item[2]);
  }

  function newStagedIdempotencyKey() {
    // Crypto.getRandomValues also works on unencrypted private LAN origins.
    // Never use Math.random for an operation identity.
    const crypto = global.crypto;
    if (!crypto || typeof crypto.getRandomValues !== 'function') return '';
    const bytes = new Uint8Array(16);
    crypto.getRandomValues(bytes);
    return 'search-web-' +
      Array.from(bytes, byte => byte.toString(16).padStart(2, '0')).join('');
  }

  // A copied rule may contain native options absent from this limited form.
  // Prevent an apparently successful copy from silently discarding them.
  function unsupportedCopiedSettings(timer) {
    if (!timer) return [];
    return [
      ['useDayOfWeek', 'Wochentagsfilter'],
      ['compareCategories', 'EPG-Kategorienvergleich'],
      ['compareTime', 'Zeitvergleich'],
      ['useSeriesRecording', 'Serienaufnahme'],
      ['keepRecordings', 'Aufnahmen behalten'],
      ['deleteMode', 'Löschmodus'],
      ['searchTimerAction', 'Suchaktion'],
      ['summaryMatch', 'Beschreibungsübereinstimmung'],
      ['useExtendedEpgInfo', 'Erweiterte EPG-Informationen'],
      ['extendedEpgInfo', 'Erweiterte EPG-Filter'],
      ['ignoreMissingEpgCategories', 'Fehlende EPG-Kategorien'],
      ['contentDescriptors', 'Inhaltskennungen'],
      ['useInFavorites', 'Favoriten'],
      ['activeFrom', 'Aktiv ab'],
      ['activeUntil', 'Aktiv bis'],
      ['pauseOnRecordings', 'Aufnahmepause'],
      ['switchMinutesBefore', 'Umschalt-Vorlauf'],
      ['unmuteSoundOnSwitch', 'Ton bei Umschaltung'],
      ['deleteRecordingsAfterDays', 'Aufnahmen löschen nach Tagen'],
      ['deleteAfterCountRecordings', 'Aufnahmen löschen nach Anzahl'],
      ['deleteAfterDaysOfFirstRecording', 'Aufnahmen löschen ab Erstausstrahlung']
    ].filter(pair => Boolean(timer[pair[0]])).map(pair => pair[1]);
  }

  function createErrorText(error) {
    const raw = String(error && error.message ? error.message : error);
    const detail = String(error && error.payload && error.payload.detail || '');
    if (error && error.status === 503) {
      if (detail.includes('offline or read-only backend')) {
        return 'Backend ist offline oder für Schreibvorgänge gesperrt.';
      }
      if (detail.includes('valid, online backend agent')) {
        return 'Der Backend-Agent ist nicht angemeldet, nicht aktuell oder offline.';
      }
      if (detail.includes('configured backend provider and SuiteBridge')) {
        return 'Der epgsearch-Provider oder die SuiteBridge-Anbindung fehlt.';
      }
      if (detail.includes('confirmed VDR pre/post margins')) {
        return 'Die laufenden VDR-Vor-/Nachlaufwerte können über SuiteBridge nicht bestätigt werden.';
      }
    }
    return raw;
  }

  function createForm(template) {
    const source = template || normalizeSearchTimer({}, 0);
    const form = document.createElement('form');
    form.className = 'searchtimer-form';
    form.dataset.searchtimerEditorForm = 'create';

    const notice = addText(document.createElement('p'),
      'MLD-Webfrontend: Suchtimer können mit den vorhandenen epgsearch-Optionen ' +
      'direkt inaktiv oder aktiv angelegt werden. Die Vorschau zeigt nur EPG-' +
      'Texttreffer, keine tatsächliche Aufnahmeprognose. Der Browser benutzt ' +
      'die bestehende angemeldete VDR-Suite-Browser-API, nicht die Device-API.');
    notice.className = 'searchtimer-safety-note';
    form.appendChild(notice);
    const grid = document.createElement('div');
    grid.className = 'searchtimer-form-grid';
    grid.appendChild(field('Suchbegriff / Name', input('query', source.query), true));
    grid.appendChild(check('active', 'Nach dem Speichern sofort aktiv', false));
    grid.appendChild(check('compareTitle', 'Titel durchsuchen', template ? source.compareTitle : true));
    grid.appendChild(check('compareSubtitle', 'Untertitel durchsuchen', source.compareSubtitle));
    grid.appendChild(check('compareSummary', 'Beschreibung durchsuchen', source.compareSummary));
    grid.appendChild(field('Suchmodus', select('mode',
      [0, 1, 2, 3, 5].includes(source.mode) ? source.mode : 0,
      [[0, 'Phrase'], [1, 'Alle Wörter'], [2, 'Ein Wort'],
       [3, 'Exakt'], [5, 'Unscharf']])));
    grid.appendChild(field('Toleranz (nur Unscharf)', input('tolerance',
      source.mode === 5 ? source.tolerance : 0, 'number')));
    form.appendChild(grid);

    const advanced = document.createElement('details');
    advanced.className = 'searchtimer-expert';
    advanced.appendChild(addText(document.createElement('summary'),
      'Weitere epgsearch-Einstellungen'));
    const advancedGrid = document.createElement('div');
    advancedGrid.className = 'searchtimer-form-grid';
    advancedGrid.style.padding = '.65rem';
    advancedGrid.appendChild(check('useVps', 'VPS verwenden', source.useVps));
    advancedGrid.appendChild(field('Aufnahme-Priorität', input('priority', source.priority, 'number')));
    advancedGrid.appendChild(field('Lebensdauer', input('lifetime', source.lifetime, 'number')));
    advancedGrid.appendChild(field('Vorlauf in Minuten (VDR-Standard wird verwendet)',
      input('marginStartMinutes', source.marginStartMinutes, 'number')));
    advancedGrid.appendChild(field('Nachlauf in Minuten (VDR-Standard wird verwendet)',
      input('marginStopMinutes', source.marginStopMinutes, 'number')));
    createDirectoryFields(advancedGrid, source);
    createChannelFilterFields(advancedGrid, source);
    advancedGrid.appendChild(check('useDuration', 'Dauerfilter aktivieren', source.useDuration));
    advancedGrid.appendChild(field('Mindestdauer (Min.)',
      input('durationMinMinutes', source.durationMinMinutes, 'number')));
    advancedGrid.appendChild(field('Höchstdauer (Min.)',
      input('durationMaxMinutes', source.durationMaxMinutes, 'number')));
    advancedGrid.appendChild(check('avoidRepeats', 'Wiederholungen vermeiden', source.avoidRepeats));
    advancedGrid.appendChild(field('Erlaubte Wiederholungen',
      input('allowedRepeats', source.allowedRepeats, 'number')));
    advancedGrid.appendChild(field('Wiederholungen innerhalb (Tage)',
      input('repeatsWithinDays', source.repeatsWithinDays, 'number')));
    advancedGrid.appendChild(field('Blacklist-Modus',
      input('blacklistMode', source.blacklistMode, 'number')));
    advancedGrid.appendChild(field('Blacklist-IDs',
      input('blacklistIds', source.blacklistIds)));
    advancedGrid.appendChild(check('matchCase',
      'Groß-/Kleinschreibung beachten', source.matchCase));
    const extra = document.createElement('details');
    extra.className = 'searchtimer-expert';
    extra.appendChild(addText(document.createElement('summary'),
      'Weitere epgsearch-Regeloptionen'));
    const extraGrid = document.createElement('div');
    extraGrid.className = 'searchtimer-form-grid';
    extraGrid.style.padding = '.65rem';
    const extras = [
      ['useTime','Uhrzeitfilter','bool'],
      ['startTime','Beginn HHMM','number'],
      ['stopTime','Ende HHMM','number'],
      ['useDayOfWeek','Wochentage filtern','bool'],
      ['dayOfWeek','Wochentagsmaske','number'],
      ['compareCategories','EPG-Kategorien vergleichen','bool'],
      ['compareTime','Uhrzeit vergleichen','bool'],
      ['useSeriesRecording','Serienaufnahme','bool'],
      ['keepRecordings','Aufnahmen behalten','number'],
      ['deleteMode','Löschmodus','number'],
      ['searchTimerAction','Suchtimer-Aktion','number'],
      ['summaryMatch','Zusammenfassung: Übereinstimmung','number'],
      ['useExtendedEpgInfo','Erweiterte EPG-Daten','bool'],
      ['extendedEpgInfo','Erweiterte EPG-Filter','text'],
      ['ignoreMissingEpgCategories','Fehlende EPG-Kategorien ignorieren','bool'],
      ['contentDescriptors','Inhaltskennungen','text'],
      ['useInFavorites','In Favoriten verwenden','bool'],
      ['activeFrom','Gültig ab','text'],
      ['activeUntil','Gültig bis','text'],
      ['pauseOnRecordings','Aufnahme beim Suchtimer pausieren','bool'],
      ['switchMinutesBefore','Vor Umschaltung (Minuten)','number'],
      ['unmuteSoundOnSwitch','Ton bei Umschaltung aktivieren','bool'],
      ['deleteRecordingsAfterDays','Aufnahmen löschen nach Tagen','number'],
      ['deleteAfterCountRecordings','Aufnahmen löschen ab Anzahl','number'],
      ['deleteAfterDaysOfFirstRecording','Aufnahmen löschen nach erster Aufnahme','number']
    ];
    extras.forEach(([name, title, type]) => {
      extraGrid.appendChild(type === 'bool'
        ? check(name, title, source[name])
        : field(title, input(name, source[name], type)));
    });
    extra.appendChild(extraGrid);
    advancedGrid.appendChild(extra);
    advanced.appendChild(advancedGrid);
    form.appendChild(advanced);

    const actions = document.createElement('div');
    actions.className = 'searchtimer-actions';
    const previewButton = addText(document.createElement('button'), 'Vorschau');
    previewButton.type = 'button';
    previewButton.className = 'primary';
    const saveButton = addText(document.createElement('button'), 'SearchTimer speichern');
    saveButton.type = 'submit';
    saveButton.dataset.searchtimerAction = 'save';
    actions.appendChild(previewButton);
    actions.appendChild(saveButton);
    const checkOperationButton = addText(
      document.createElement('button'), 'Anlagevorgang prüfen');
    checkOperationButton.type = 'button';
    checkOperationButton.hidden = true;
    checkOperationButton.disabled = true;
    actions.appendChild(checkOperationButton);
    form.appendChild(actions);
    const feedback = document.createElement('div');
    feedback.className = 'searchtimer-feedback';
    feedback.setAttribute('role', 'status');
    feedback.setAttribute('aria-live', 'polite');
    form.appendChild(feedback);

    let pending = false;
    let submitted = false;
    function payload() {
      const result = buildCreatePayload(form, activeCreateTemplate || source);
      result.query = String(result.query || '').trim();
      result.name = result.query;
      result.backendId = selectedBackendId();
      const errors = validateCreatePayload(result);
      if (!result.backendId) errors.push('Backend fehlt.');
      if (errors.length) {
        setFeedback(feedback, errors.join(' '), true);
        return null;
      }
      return result;
    }
    previewButton.addEventListener('click', () => {
      const result = payload();
      if (result) runPreview(result, previewButton, feedback);
    });
    form.addEventListener('submit', event => {
      event.preventDefault();
      if (pending || submitted) {
        setFeedback(feedback,
          'Der vorige Anlageversuch muss im VDR geprüft werden. ' +
          'Bitte nicht erneut anlegen.', true);
        return;
      }
      const requested = payload();
      if (!requested) return;
      const api = clientApi();
      if (!api || typeof api.fetchClientSearchTimerCreateAction !== 'function' ||
          typeof api.fetchClientSearchTimers !== 'function') {
        setFeedback(feedback, 'Der Browser-Suchtimer-Pfad ist nicht verfügbar.', true);
        return;
      }
      if (!global.confirm('SearchTimer „' + requested.query +
          '“ auf Backend „' + requested.backendId + '“ ' +
          (requested.active ? 'AKTIV anlegen? Die Regel kann sofort Timer erzeugen.'
            : 'inaktiv anlegen?'))) return;
      pending = true;
      submitted = true;
      saveButton.disabled = true;
      checkOperationButton.hidden = true;
      checkOperationButton.disabled = true;
      setFeedback(feedback, 'SearchTimer wird einmalig an epgsearch gesendet …', false);
      api.fetchClientSearchTimerCreateAction({
        payload: requested, cache: 'no-store', credentials: 'same-origin'
      }).then(created => {
        // The existing Browser controller serializes application-level
        // failure with HTTP 200 and success:false, including a useful message
        // and errors array. Do not replace that evidence with a generic
        // missing-id error; the write outcome can still be uncertain.
        if (created && created.success === false) {
          const message = String(resultMessage(
            created, 'Der epgsearch-Provider hat keine Anlage bestätigt.'));
          // Errors from the underlying RESTfulAPI adapter can contain
          // requestBody/responseBody diagnostic dumps. Only show the status
          // and bounded non-payload reason tokens in the Browser.
          const details = Array.isArray(created.errors)
            ? created.errors.filter(value => typeof value === 'string' &&
                value.length > 0 && value.length <= 120 &&
                (/^status=[0-9]{3}$/.test(value) ||
                 /^readback did not find a unique created searchtimer id/.test(value))
              ).slice(0, 3)
            : [];
          throw new Error('Backend meldet: ' + message +
            (details.length ? ' (' + details.join(' · ') + ')' : ''));
        }
        if (!created || created.success !== true ||
            created.backendId !== requested.backendId ||
            !String(created.backendNativeId || '') ||
            created.query !== requested.query ||
            created.state !== (requested.active ? 'active' : 'inactive')) {
          throw new Error('Backendantwort unvollständig oder nicht dem ' +
            'angefragten Backend/Suchtimer zuzuordnen.');
        }
        return api.fetchClientSearchTimers({
          query: {backend: requested.backendId, text: requested.query, limit: 1000},
          cache: 'no-store', credentials: 'same-origin'
        }).then(readback => {
          const count = Number(readback && readback.totalCount);
          const items = listFromResponse(readback, 'searchTimers')
            .map(normalizeSearchTimer);
          const matches = items.filter(item =>
            item.backendId === requested.backendId &&
            item.backendNativeId === String(created.backendNativeId));
          if (!Number.isFinite(count) || count !== items.length ||
              matches.length !== 1 ||
              matches[0].query !== requested.query ||
              matches[0].active !== requested.active) {
            throw new Error('Suchtimer geschrieben, Backend-Readback noch unklar.');
          }
          const actual = matches[0];
          const fields = [
            'directory','priority','lifetime','marginStartMinutes',
            'marginStopMinutes','useVps','useChannel','channels',
            'channelMin','channelMax','useTime','startTime','stopTime',
            'useDayOfWeek','dayOfWeek','useDuration',
            'durationMinMinutes','durationMaxMinutes','avoidRepeats',
            'allowedRepeats','repeatsWithinDays','compareTitle',
            'compareSubtitle','compareSummary','compareCategories',
            'compareTime','useSeriesRecording','keepRecordings','deleteMode',
            'searchTimerAction','blacklistMode','blacklistIds','mode','matchCase',
            'tolerance','summaryMatch','useExtendedEpgInfo','extendedEpgInfo',
            'ignoreMissingEpgCategories','contentDescriptors','useInFavorites',
            'activeFrom','activeUntil','pauseOnRecordings','switchMinutesBefore',
            'unmuteSoundOnSwitch','deleteRecordingsAfterDays',
            'deleteAfterCountRecordings','deleteAfterDaysOfFirstRecording'
          ];
          const listFields = new Set(['blacklistIds', 'extendedEpgInfo']);
          const comparisonValue = (name, value) => listFields.has(name)
            ? String(value || '').split(/[,;]/).map(part => part.trim())
                .filter(Boolean).join(',')
            : String(value);
          const relevant = fields.filter(name => {
            if (name === 'channels') return requested.useChannel === 2;
            if (name === 'channelMin' || name === 'channelMax')
              return requested.useChannel === 1;
            if (name === 'startTime' || name === 'stopTime')
              return requested.useTime;
            if (name === 'dayOfWeek') return requested.useDayOfWeek;
            if (name === 'durationMinMinutes' || name === 'durationMaxMinutes')
              return requested.useDuration;
            return true;
          });
          const changed = relevant.filter(name =>
            comparisonValue(name, actual[name]) !==
            comparisonValue(name, requested[name]));
          if (changed.length) {
            throw new Error('Suchtimer angelegt, aber Einstellungen abweichend: ' +
              changed.join(', '));
          }
          setFeedback(feedback, 'SearchTimer „' + requested.query + '“ ' +
            (requested.active ? 'AKTIV' : 'inaktiv') +
            ' angelegt und im Backend bestätigt.', false);
          global.setTimeout(reloadSearchTimers, 300);
          return actual;
        });
      }).catch(error => {
        setFeedback(feedback,
          'Anlage oder Readback nicht zweifelsfrei bestätigt: ' +
          String(error && error.message ? error.message : error) +
          '. Bitte VDR-Suchtimerliste prüfen. Dieser Dialog wiederholt ' +
          'den Schreibvorgang nicht.', true);
      }).finally(() => { pending = false; });
    });
    activeCreateForm = form;
    return form;
  }

  function populateCreateForm(timer) {
    if (!activeCreateForm) return;
    activeCreateTemplate = timer;

    // Copy every supported editor field from the selected native rule.
    // A partial copy would otherwise silently erase advanced epgsearch flags.
    Object.keys(timer).forEach(name => {
      const control = activeCreateForm.elements[name];
      if (!control || timer[name] === undefined || timer[name] === null) return;
      if (control.type === 'checkbox') control.checked = Boolean(timer[name]);
      else control.value = String(timer[name]);
    });
    if (activeCreateForm.elements.query)
      activeCreateForm.elements.query.value = timer.query;
    if (activeCreateForm.elements.tolerance && timer.mode !== 5)
      activeCreateForm.elements.tolerance.value = '0';

    if (activeCreateForm.elements.directory) activeCreateForm.elements.directory.value = timer.directory;
    if (activeCreateForm.elements.manualDirectory) {
      activeCreateForm.elements.manualDirectory.value = activeCreateForm.elements.directory && activeCreateForm.elements.directory.value === timer.directory ? '' : timer.directory;
    }
    if (activeCreateForm.elements.channelFilterMode) activeCreateForm.elements.channelFilterMode.value = String(timer.useChannel);
    if (activeCreateForm.elements.channelFilterGroup && timer.useChannel === 2) activeCreateForm.elements.channelFilterGroup.value = timer.channels;
    if (activeCreateForm.elements.channelId && timer.useChannel === 1) activeCreateForm.elements.channelId.value = timer.channelMin || timer.channelMax;
    if (activeCreateForm.elements.manualUseChannel) activeCreateForm.elements.manualUseChannel.value = '';
    if (activeCreateForm.elements.manualChannels) activeCreateForm.elements.manualChannels.value = timer.useChannel === 2 ? timer.channels : '';
    if (activeCreateForm.elements.manualChannelMin) activeCreateForm.elements.manualChannelMin.value = timer.useChannel === 1 ? timer.channelMin : '';
    if (activeCreateForm.elements.manualChannelMax) activeCreateForm.elements.manualChannelMax.value = timer.useChannel === 1 ? timer.channelMax : '';

    const panel = activeCreateForm.closest('.searchtimer-create-panel');
    if (panel) panel.open = true;
    activeCreateForm.scrollIntoView({behavior: 'smooth', block: 'start'});
  }

  function appendMeta(parent, label, value) {
    const item = document.createElement('div');
    item.className = 'searchtimer-meta';
    item.appendChild(addText(document.createElement('span'), label));
    item.appendChild(addText(document.createElement('strong'), value === '' ? '-' : value));
    parent.appendChild(item);
  }

  function channelFilterLabel(timer) {
    if (timer.useChannel === 1) {
      return timer.channelMin === timer.channelMax || timer.channelMax === ''
        ? 'Kanal · ' + (timer.channelMin || timer.channelMax || '-')
        : 'Bereich · ' + timer.channelMin + ' bis ' + timer.channelMax;
    }
    if (timer.useChannel === 2) return 'Gruppe · ' + (timer.channels || '-');
    if (timer.useChannel === 3) return 'nur FTA';
    return 'alle';
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
      const filterBadge = addText(document.createElement('span'), 'Filter');
      filterBadge.className = 'searchtimer-badge filter';
      badges.appendChild(filterBadge);
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
    appendMeta(meta, 'Verzeichnis', directoryOptionLabel(timer.directory));
    appendMeta(meta, 'Priorität / Lebensdauer', String(timer.priority) + ' / ' + String(timer.lifetime));
    appendMeta(meta, 'Marge Start / Stop', String(timer.marginStartMinutes) + ' / ' + String(timer.marginStopMinutes) + ' Min.');
    appendMeta(meta, 'Suchfelder', [timer.compareTitle ? 'Titel' : '', timer.compareSubtitle ? 'Untertitel' : '', timer.compareSummary ? 'Beschreibung' : ''].filter(Boolean).join(', ') || '-');
    appendMeta(meta, 'Kanalfilter', channelFilterLabel(timer));
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
    const toggle = addText(document.createElement('button'), timer.active ? 'Deaktivieren' : 'Aktivieren');
    toggle.type = 'button';
    toggle.disabled = true;
    toggle.title = 'Sicheres Ändern wird erst freigegeben, wenn der vollständige Leservertrag verfügbar ist.';
    actions.appendChild(toggle);
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
    activeCreateTemplate = null;
    lastSearchTimerData = data;
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
    createPanel.dataset.searchtimerEditor = 'create';
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

  const searchTimerBrowserApi = Object.freeze({
    configureContext: configureContext,
    renderList: renderList,
    normalizeSearchTimer: normalizeSearchTimer,
    normalizeDirectory: normalizeDirectory,
    directoryOptionLabel: directoryOptionLabel,
    recordingFolderPathToDirectory: recordingFolderPathToDirectory,
    directoryOptions: directoryOptions,
    normalizeChannel: normalizeChannel,
    normalizeChannels: normalizeChannels,
    channelGroups: channelGroups,
    channelsForGroup: channelsForGroup,
    channelOptionLabel: channelOptionLabel,
    previewChannelLabel: previewChannelLabel,
    inferChannelMode: inferChannelMode,
    effectiveChannelFilter: effectiveChannelFilter,
    buildCreatePayload: buildCreatePayload,
    validateCreatePayload: validateCreatePayload,
    stagedCreateValidation: stagedCreateValidation,
    unsupportedStagedOptions: unsupportedStagedOptions,
    unsupportedCopiedSettings: unsupportedCopiedSettings,
    createErrorText: createErrorText,
    selectedBackendId: selectedBackendId
  });

  global.VdrSuiteSearchTimerBrowser = searchTimerBrowserApi;
  if (global.VdrSuitePlatform &&
      typeof global.VdrSuitePlatform.registerModule === 'function' &&
      typeof global.VdrSuitePlatform.hasModule === 'function' &&
      !global.VdrSuitePlatform.hasModule('searchtimers')) {
    global.VdrSuitePlatform.registerModule('searchtimers', searchTimerBrowserApi);
  }
})(window);
