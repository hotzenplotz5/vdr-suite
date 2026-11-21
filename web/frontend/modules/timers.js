// Phase 60.8a: Active Timer browser module.
// Owns Timer list rendering through the frontend platform registry.
// Extended with safe CRUD workflows, grouped channel selection and recording-directory selection.
(function(global) {
  'use strict';

  let timerBrowserContext = Object.freeze({});
  let lastTimerData = null;
  let lastConflictReport = null;
  let channelCache = Object.create(null);
  let directoryCache = Object.create(null);

  const WEEKDAY_CHARACTERS = 'MTWTFSS';
  const WEEKDAY_LABELS = ['Mo', 'Di', 'Mi', 'Do', 'Fr', 'Sa', 'So'];
  const UNGROUPED_CHANNEL_KEY = '__ungrouped__';

  function configureContext(context) {
    timerBrowserContext = Object.freeze(Object.assign({}, context || {}));
  }

  function platform() {
    return global.VdrSuitePlatform || null;
  }

  function resolvedClientApi() {
    if (timerBrowserContext.clientApi) return timerBrowserContext.clientApi;

    const runtime = platform();
    if (runtime && typeof runtime.getClientApi === 'function') {
      const clientApi = runtime.getClientApi();
      if (clientApi) return clientApi;
    }

    return global.VdrSuiteClientApi || null;
  }

  function selectedBackendId() {
    if (typeof timerBrowserContext.getSelectedBackendId === 'function') {
      const contextId = String(timerBrowserContext.getSelectedBackendId() || '').trim();
      if (contextId !== '') return contextId;
    }

    const runtime = platform();
    if (runtime && typeof runtime.getSelectedBackendId === 'function') {
      const runtimeId = String(runtime.getSelectedBackendId() || '').trim();
      if (runtimeId !== '') return runtimeId;
    }

    return 'default';
  }

  function reloadTimers() {
    if (typeof timerBrowserContext.reload === 'function') {
      timerBrowserContext.reload();
      return;
    }

    if (typeof document !== 'undefined') {
      const refresh = document.getElementById('refresh-detail');
      if (refresh && !refresh.disabled && typeof refresh.click === 'function') refresh.click();
    }
  }

  function firstValue(source, keys, fallback) {
    const helpers = timerBrowserContext.helpers || global.VdrSuiteFrontendHelpers || null;
    if (helpers && typeof helpers.firstValue === 'function') {
      return helpers.firstValue(source, keys, fallback);
    }

    if (!source || !Array.isArray(keys)) return fallback;
    for (const key of keys) {
      if (source[key] !== undefined && source[key] !== null && source[key] !== '') {
        return source[key];
      }
    }
    return fallback;
  }

  function listFromResponse(data, key) {
    const helpers = timerBrowserContext.helpers || global.VdrSuiteFrontendHelpers || null;
    if (helpers && typeof helpers.listFromResponse === 'function') {
      return helpers.listFromResponse(data, key);
    }
    if (Array.isArray(data)) return data;
    if (data && Array.isArray(data[key])) return data[key];
    if (data && Array.isArray(data.items)) return data.items;
    return [];
  }

  function addText(element, text) {
    element.textContent = String(text);
    return element;
  }

  function boolValue(value, fallback) {
    if (value === true || value === 1 || value === '1' || value === 'true' || value === 'yes') return true;
    if (value === false || value === 0 || value === '0' || value === 'false' || value === 'no') return false;
    return Boolean(fallback);
  }

  function integerValue(value, fallback) {
    const number = Number(value);
    return Number.isFinite(number) ? Math.trunc(number) : Number(fallback || 0);
  }

  function normalizeWeekdays(value) {
    const source = String(value || '-------');
    return WEEKDAY_CHARACTERS.split('').map((character, index) => (
      source[index] && source[index] !== '-' ? character : '-'
    )).join('');
  }

  function weekdaysFromValues(values) {
    const selected = new Set(Array.isArray(values) ? values.map(Number) : []);
    return WEEKDAY_CHARACTERS.split('').map((character, index) => (
      selected.has(index) ? character : '-'
    )).join('');
  }

  function timeToInput(value) {
    const number = integerValue(value, 0);
    if (number > 1000000000) {
      const date = new Date(number * 1000);
      return String(date.getHours()).padStart(2, '0') + ':' +
        String(date.getMinutes()).padStart(2, '0');
    }
    const padded = String(Math.max(0, number)).padStart(4, '0').slice(-4);
    return padded.slice(0, 2) + ':' + padded.slice(2);
  }

  function inputTimeToHhmm(value) {
    const match = /^(\d{1,2}):(\d{2})$/.exec(String(value || '').trim());
    if (!match) return 0;
    const hour = Number(match[1]);
    const minute = Number(match[2]);
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return 0;
    return hour * 100 + minute;
  }

  function normalizeTimer(timer, index) {
    const source = timer && typeof timer === 'object' ? timer : {};
    const flags = integerValue(firstValue(source, ['flags'], 0), 0);
    return {
      source,
      index: Number(index) || 0,
      timerId: String(firstValue(source, ['timerId', 'id', 'nativeId'], '')),
      channelId: String(firstValue(source, ['channelId', 'channel'], '')),
      channelName: String(firstValue(source, ['channelName', 'channel'], '-')),
      eventId: String(firstValue(source, ['eventId'], '')),
      title: String(firstValue(source, ['title', 'name', 'file', 'eventTitle'], 'Timer')),
      directory: normalizeTimerDirectory(firstValue(source, ['directory', 'folder'], '')),
      subtitle: String(firstValue(source, ['subtitle'], '')),
      aux: String(firstValue(source, ['aux'], '')),
      day: String(firstValue(source, ['day', 'date'], '')),
      weekdays: normalizeWeekdays(firstValue(source, ['weekdays'], '-------')),
      start: integerValue(firstValue(source, ['startTime', 'start'], 0), 0),
      stop: integerValue(firstValue(source, ['endTime', 'stop'], 0), 0),
      flags,
      priority: integerValue(firstValue(source, ['priority'], 50), 50),
      lifetime: integerValue(firstValue(source, ['lifetime'], 99), 99),
      active: boolValue(firstValue(source, ['enabled', 'active'], (flags & 1) !== 0), false),
      vps: boolValue(firstValue(source, ['vps'], (flags & 4) !== 0), false),
      recording: boolValue(firstValue(source, ['recording'], (flags & 8) !== 0), false),
      pending: boolValue(firstValue(source, ['pending'], false), false)
    };
  }

  function normalizeChannel(channel, index) {
    const source = channel && typeof channel === 'object' ? channel : {};
    return {
      source,
      index: Number(index) || 0,
      id: String(firstValue(source, ['id', 'channelId', 'channel_id'], '')).trim(),
      number: integerValue(firstValue(source, ['number', 'channelNumber'], 0), 0),
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
        const numberDelta = left.number - right.number;
        if (numberDelta !== 0) return numberDelta;
        return left.name.localeCompare(right.name, 'de');
      });
  }

  function channelGroupKey(channel) {
    return channel.group === '' ? UNGROUPED_CHANNEL_KEY : channel.group;
  }

  function channelGroups(channels) {
    const source = Array.isArray(channels) ? channels : [];
    const namedGroups = [];
    let hasUngrouped = false;
    const seen = new Set();

    source.forEach(channel => {
      const normalized = channel && channel.id !== undefined ? channel : normalizeChannel(channel, 0);
      if (normalized.group === '') {
        hasUngrouped = true;
        return;
      }
      if (!seen.has(normalized.group)) {
        seen.add(normalized.group);
        namedGroups.push(normalized.group);
      }
    });

    if (namedGroups.length === 0) return [];
    if (hasUngrouped) namedGroups.push(UNGROUPED_CHANNEL_KEY);
    return namedGroups;
  }

  function channelsForGroup(channels, groupKey) {
    const source = Array.isArray(channels) ? channels : [];
    if (!groupKey) return source.slice();
    return source.filter(channel => channelGroupKey(channel) === groupKey);
  }

  function channelOptionLabel(channel) {
    const number = channel.number > 0 ? String(channel.number) + ' · ' : '';
    const name = channel.name || channel.id;
    return number + name;
  }

  function normalizeTimerDirectory(value) {
    return String(value || '')
      .trim()
      .replace(/^[~\/]+|[~\/]+$/g, '')
      .split(/[~\/]+/)
      .map(part => part.trim())
      .filter(Boolean)
      .join('~');
  }

  function recordingFolderPathToTimerDirectory(path) {
    return normalizeTimerDirectory(path);
  }

  function directoryOptionLabel(directory) {
    const normalized = normalizeTimerDirectory(directory);
    return normalized === '' ? 'Stammverzeichnis' : normalized.split('~').join(' › ');
  }

  function timerDirectories(data) {
    return listFromResponse(data, 'timers')
      .map(normalizeTimer)
      .map(timer => timer.directory)
      .filter(directory => directory !== '');
  }

  function directoryOptions(folderData, timerData, currentDirectory) {
    const values = new Set();

    listFromResponse(folderData, 'folders').forEach(folder => {
      const directory = recordingFolderPathToTimerDirectory(
        firstValue(folder, ['path', 'name'], '')
      );
      if (directory !== '') values.add(directory);
    });

    timerDirectories(timerData).forEach(directory => values.add(directory));

    const current = normalizeTimerDirectory(currentDirectory);
    if (current !== '') values.add(current);

    return [''].concat(Array.from(values).sort((left, right) => (
      directoryOptionLabel(left).localeCompare(directoryOptionLabel(right), 'de')
    )));
  }

  function timerActionPayload(timer, overrides) {
    const normalized = timer && timer.timerId !== undefined ? timer : normalizeTimer(timer || {}, 0);
    const changes = overrides && typeof overrides === 'object' ? overrides : {};
    return {
      backendId: String(changes.backendId !== undefined ? changes.backendId : selectedBackendId()),
      timerId: String(changes.timerId !== undefined ? changes.timerId : normalized.timerId),
      channelId: String(changes.channelId !== undefined ? changes.channelId : normalized.channelId),
      title: String(changes.title !== undefined ? changes.title : normalized.title),
      directory: normalizeTimerDirectory(
        changes.directory !== undefined ? changes.directory : normalized.directory
      ),
      day: String(changes.day !== undefined ? changes.day : normalized.day),
      weekdays: normalizeWeekdays(changes.weekdays !== undefined ? changes.weekdays : normalized.weekdays),
      start: integerValue(changes.start !== undefined ? changes.start : normalized.start, 0),
      stop: integerValue(changes.stop !== undefined ? changes.stop : normalized.stop, 0),
      priority: integerValue(changes.priority !== undefined ? changes.priority : normalized.priority, 50),
      lifetime: integerValue(changes.lifetime !== undefined ? changes.lifetime : normalized.lifetime, 99),
      active: boolValue(changes.active !== undefined ? changes.active : normalized.active, true),
      vps: boolValue(changes.vps !== undefined ? changes.vps : normalized.vps, false),
      aux: String(changes.aux !== undefined ? changes.aux : normalized.aux)
    };
  }

  function validateTimerPayload(payload, requireTimerId) {
    const errors = [];
    const repeating = normalizeWeekdays(payload.weekdays) !== '-------';
    if (requireTimerId && String(payload.timerId || '').trim() === '') errors.push('Timer-ID fehlt.');
    if (String(payload.channelId || '').trim() === '') errors.push('Kanal fehlt.');
    if (String(payload.title || '').trim() === '') errors.push('Titel fehlt.');
    if (!repeating && String(payload.day || '').trim() === '') {
      errors.push('Datum fehlt. Alternativ mindestens einen Wochentag auswählen.');
    }
    if (integerValue(payload.start, 0) <= 0 || integerValue(payload.stop, 0) <= 0) {
      errors.push('Start und Ende müssen gültige Uhrzeiten sein.');
    }
    if (integerValue(payload.priority, -1) < 0 || integerValue(payload.priority, 100) > 99) {
      errors.push('Priorität muss zwischen 0 und 99 liegen.');
    }
    if (integerValue(payload.lifetime, -1) < 0 || integerValue(payload.lifetime, 100) > 99) {
      errors.push('Lebensdauer muss zwischen 0 und 99 liegen.');
    }
    return errors;
  }

  function installStyles() {
    if (typeof document === 'undefined' || document.getElementById('vdr-suite-timer-workflow-styles')) return;
    const style = document.createElement('style');
    style.id = 'vdr-suite-timer-workflow-styles';
    style.textContent = `
.timer-module{display:grid;gap:.8rem}.timer-summary{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:.65rem}.timer-summary h3,.timer-card h3{margin:0}.timer-create-panel,.timer-card{border:1px solid rgba(96,165,250,.24);border-radius:.95rem;background:rgba(15,23,42,.72)}.timer-create-panel>summary,.timer-card>summary{cursor:pointer;padding:.8rem .9rem;color:#f8fafc;font-weight:850}.timer-create-panel[open]>summary,.timer-card[open]>summary{border-bottom:1px solid rgba(148,163,184,.18)}.timer-card.conflict{border-color:rgba(248,113,113,.72)}.timer-card.recording{border-color:rgba(74,222,128,.62)}.timer-card-summary{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:.65rem;align-items:center}.timer-card-title strong{display:block;overflow:hidden;text-overflow:ellipsis}.timer-card-title span{display:block;margin-top:.15rem;color:#94a3b8;font-size:.86rem;font-weight:500}.timer-badges{display:flex;flex-wrap:wrap;justify-content:flex-end;gap:.32rem}.timer-badge{padding:.22rem .48rem;border-radius:999px;border:1px solid rgba(148,163,184,.28);background:rgba(2,6,23,.72);color:#cbd5e1;font-size:.74rem;font-weight:800}.timer-badge.active,.timer-badge.recording{border-color:rgba(74,222,128,.5);color:#bbf7d0}.timer-badge.pending,.timer-badge.vps{border-color:rgba(56,189,248,.48);color:#bae6fd}.timer-badge.conflict{border-color:rgba(248,113,113,.62);color:#fecaca}.timer-card-body,.timer-create-body{display:grid;gap:.72rem;padding:.82rem .9rem .9rem}.timer-meta-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(10rem,1fr));gap:.5rem}.timer-meta{padding:.55rem .62rem;border:1px solid rgba(148,163,184,.17);border-radius:.68rem;background:rgba(2,6,23,.52)}.timer-meta span{display:block;color:#94a3b8;font-size:.72rem;font-weight:750;text-transform:uppercase}.timer-meta strong{display:block;margin-top:.15rem;color:#f8fafc;overflow-wrap:anywhere}.timer-form{display:grid;gap:.72rem}.timer-form-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.62rem}.timer-field{display:grid;gap:.28rem;min-width:0}.timer-field.wide{grid-column:1/-1}.timer-field>span{color:#cbd5e1;font-size:.78rem;font-weight:750}.timer-field input,.timer-field select{box-sizing:border-box;width:100%;min-width:0;min-height:2.55rem;padding:.5rem .62rem;border:1px solid #475569;border-radius:.62rem;background:#111827;color:#f8fafc;font:inherit}.timer-field select:disabled{opacity:.62}.timer-checkbox{display:flex;align-items:center;gap:.45rem;min-height:2.55rem}.timer-checkbox input{width:1.15rem;height:1.15rem}.timer-weekdays{display:flex;flex-wrap:wrap;gap:.35rem}.timer-weekday{display:flex;align-items:center;gap:.28rem;padding:.38rem .48rem;border:1px solid rgba(148,163,184,.24);border-radius:.58rem;background:rgba(2,6,23,.52)}.timer-selector-status{margin:.15rem 0 0;color:#94a3b8;font-size:.78rem}.timer-expert{grid-column:1/-1;border:1px solid rgba(148,163,184,.18);border-radius:.65rem;background:rgba(2,6,23,.36)}.timer-expert>summary{cursor:pointer;padding:.55rem .65rem;color:#94a3b8;font-size:.78rem;font-weight:750}.timer-expert>.timer-field{padding:0 .65rem .65rem}.timer-actions{display:flex;flex-wrap:wrap;gap:.5rem}.timer-actions button{min-height:2.55rem;padding:.55rem .78rem;border-radius:.65rem}.timer-actions .danger{border-color:rgba(248,113,113,.55);color:#fecaca}.timer-actions .primary{border-color:rgba(56,189,248,.55)}.timer-feedback{min-height:1.2rem;padding:.58rem .65rem;border-radius:.62rem;background:rgba(2,6,23,.5);color:#cbd5e1}.timer-feedback.success{border:1px solid rgba(74,222,128,.4);color:#bbf7d0}.timer-feedback.error{border:1px solid rgba(248,113,113,.45);color:#fecaca}.timer-conflict-list{display:grid;gap:.5rem}.timer-conflict-item{padding:.58rem .65rem;border:1px solid rgba(248,113,113,.25);border-radius:.65rem;background:rgba(69,10,10,.22)}
@media(max-width:760px){.timer-card-summary{grid-template-columns:minmax(0,1fr)}.timer-badges{justify-content:flex-start}.timer-form-grid{grid-template-columns:minmax(0,1fr)}.timer-field.wide,.timer-expert{grid-column:auto}.timer-actions{display:grid;grid-template-columns:minmax(0,1fr)}.timer-actions button{width:100%}.timer-meta-grid{grid-template-columns:minmax(0,1fr)}}`;
    document.head.appendChild(style);
  }

  function textInput(name, value, type) {
    const input = document.createElement('input');
    input.name = name;
    input.type = type || 'text';
    input.value = value === undefined || value === null ? '' : String(value);
    return input;
  }

  function selectInput(name) {
    const select = document.createElement('select');
    select.name = name;
    return select;
  }

  function createField(label, input, wide) {
    const field = document.createElement('label');
    field.className = 'timer-field' + (wide ? ' wide' : '');
    field.appendChild(addText(document.createElement('span'), label));
    field.appendChild(input);
    return field;
  }

  function checkboxField(name, label, checked) {
    const field = document.createElement('label');
    field.className = 'timer-checkbox';
    const input = document.createElement('input');
    input.type = 'checkbox';
    input.name = name;
    input.checked = Boolean(checked);
    field.appendChild(input);
    field.appendChild(addText(document.createElement('span'), label));
    return field;
  }

  function option(value, label, selected) {
    const item = document.createElement('option');
    item.value = String(value);
    item.textContent = String(label);
    item.selected = Boolean(selected);
    return item;
  }

  function appendWeekdayInputs(parent, weekdays) {
    const wrapper = document.createElement('div');
    wrapper.className = 'timer-weekdays';
    WEEKDAY_LABELS.forEach((label, index) => {
      const item = document.createElement('label');
      item.className = 'timer-weekday';
      const input = document.createElement('input');
      input.type = 'checkbox';
      input.name = 'weekday';
      input.value = String(index);
      input.checked = String(weekdays || '-------')[index] !== '-';
      item.appendChild(input);
      item.appendChild(addText(document.createElement('span'), label));
      wrapper.appendChild(item);
    });
    parent.appendChild(wrapper);
  }

  function selectedWeekdays(form) {
    return Array.from(form.querySelectorAll('input[name="weekday"]:checked')).map(input => Number(input.value));
  }

  function effectiveChannelId(form) {
    const manual = form.elements.manualChannelId
      ? String(form.elements.manualChannelId.value || '').trim()
      : '';
    if (manual !== '') return manual;
    return form.elements.channelId ? String(form.elements.channelId.value || '').trim() : '';
  }

  function effectiveDirectory(form) {
    const manual = form.elements.manualDirectory
      ? normalizeTimerDirectory(form.elements.manualDirectory.value)
      : '';
    if (manual !== '') return manual;
    return form.elements.directory
      ? normalizeTimerDirectory(form.elements.directory.value)
      : '';
  }

  function formPayload(form, timer) {
    const weekdays = weekdaysFromValues(selectedWeekdays(form));
    const repeating = weekdays !== '-------';
    return timerActionPayload(timer, {
      timerId: form.elements.timerId ? form.elements.timerId.value : timer.timerId,
      channelId: effectiveChannelId(form),
      title: form.elements.title.value.trim(),
      directory: effectiveDirectory(form),
      day: repeating ? '' : form.elements.day.value.trim(),
      weekdays,
      start: inputTimeToHhmm(form.elements.start.value),
      stop: inputTimeToHhmm(form.elements.stop.value),
      priority: integerValue(form.elements.priority.value, 50),
      lifetime: integerValue(form.elements.lifetime.value, 99),
      active: form.elements.active.checked,
      vps: form.elements.vps.checked,
      aux: form.elements.aux.value
    });
  }

  function setFeedback(target, message, error) {
    target.className = 'timer-feedback ' + (error ? 'error' : 'success');
    target.textContent = String(message || '');
  }

  function resultMessage(result, fallback) {
    if (result && result.message) return String(result.message);
    if (result && Array.isArray(result.errors) && result.errors.length > 0) return result.errors.join(' · ');
    return fallback;
  }

  function actionClientFunction(type) {
    const clientApi = resolvedClientApi();
    if (!clientApi) return null;
    const name = type === 'create'
      ? 'fetchClientTimerCreateAction'
      : type === 'update'
        ? 'fetchClientTimerUpdateAction'
        : 'fetchClientTimerDeleteAction';
    return typeof clientApi[name] === 'function' ? clientApi[name].bind(clientApi) : null;
  }

  function executeAction(type, payload, button, feedback) {
    const action = actionClientFunction(type);
    if (!action) {
      setFeedback(feedback, 'Timer-Aktion ist nicht verfügbar.', true);
      return Promise.resolve(null);
    }
    button.disabled = true;
    setFeedback(feedback, 'Timer-Aktion wird ausgeführt …', false);
    return action({payload, cache: 'no-store', credentials: 'same-origin'})
      .then(result => {
        if (!result || result.success !== true) {
          throw new Error(resultMessage(result, 'Backend hat die Timer-Aktion abgelehnt.'));
        }
        setFeedback(feedback, resultMessage(result, 'Timer-Aktion erfolgreich.'), false);
        global.setTimeout(reloadTimers, 250);
        return result;
      })
      .catch(error => {
        setFeedback(feedback, String(error && error.message ? error.message : error), true);
        return null;
      })
      .finally(() => { button.disabled = false; });
  }

  function fetchChannels() {
    const backendId = selectedBackendId();
    if (channelCache[backendId]) return channelCache[backendId];

    const clientApi = resolvedClientApi();
    if (!clientApi || typeof clientApi.fetchClientChannels !== 'function') {
      return Promise.reject(new Error('Kanalliste ist nicht verfügbar.'));
    }

    channelCache[backendId] = clientApi.fetchClientChannels({
      query: {backend: backendId, _: String(Date.now())},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(normalizeChannels).catch(error => {
      delete channelCache[backendId];
      throw error;
    });

    return channelCache[backendId];
  }

  function fetchRecordingDirectories() {
    const backendId = selectedBackendId();
    if (directoryCache[backendId]) return directoryCache[backendId];

    const clientApi = resolvedClientApi();
    if (!clientApi || typeof clientApi.fetchClientRecordingFolder !== 'function') {
      return Promise.reject(new Error('Aufnahmeverzeichnisse sind nicht verfügbar.'));
    }

    directoryCache[backendId] = clientApi.fetchClientRecordingFolder({
      query: {
        backend: backendId,
        path: '',
        limit: 1,
        offset: 0,
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).catch(error => {
      delete directoryCache[backendId];
      throw error;
    });

    return directoryCache[backendId];
  }

  function populateChannelSelect(select, channels, selectedId) {
    select.replaceChildren();
    if (channels.length === 0) {
      select.appendChild(option('', 'Keine Kanäle in dieser Gruppe', true));
      select.disabled = true;
      return;
    }

    select.disabled = false;
    select.appendChild(option('', 'Kanal auswählen …', selectedId === ''));
    channels.forEach(channel => {
      select.appendChild(option(channel.id, channelOptionLabel(channel), channel.id === selectedId));
    });

    if (selectedId !== '' && !channels.some(channel => channel.id === selectedId)) {
      select.appendChild(option(selectedId, 'Aktueller Kanal · ' + selectedId, true));
    }
  }

  function configureChannelSelectors(groupField, groupSelect, channelSelect, manualInput, status, timer) {
    status.textContent = 'Kanäle werden geladen …';
    groupSelect.disabled = true;
    channelSelect.disabled = true;
    channelSelect.replaceChildren(option('', 'Kanäle werden geladen …', true));

    fetchChannels().then(channels => {
      const groups = channelGroups(channels);
      const currentChannel = channels.find(channel => channel.id === timer.channelId) || null;

      if (groups.length === 0) {
        groupField.hidden = true;
        populateChannelSelect(channelSelect, channels, timer.channelId);
        status.textContent = String(channels.length) + ' Kanäle verfügbar.';
        return;
      }

      groupField.hidden = false;
      groupSelect.disabled = false;
      groupSelect.replaceChildren();
      groupSelect.appendChild(option('', 'Kanalgruppe auswählen …', false));
      groups.forEach(group => {
        const label = group === UNGROUPED_CHANNEL_KEY ? 'Ohne Kanalgruppe' : group;
        groupSelect.appendChild(option(group, label, false));
      });

      const initialGroup = currentChannel ? channelGroupKey(currentChannel) : '';
      groupSelect.value = initialGroup;
      if (initialGroup !== '') {
        populateChannelSelect(channelSelect, channelsForGroup(channels, initialGroup), timer.channelId);
      } else {
        channelSelect.replaceChildren(option('', 'Zuerst Kanalgruppe auswählen …', true));
        channelSelect.disabled = true;
      }

      groupSelect.addEventListener('change', () => {
        manualInput.value = '';
        populateChannelSelect(channelSelect, channelsForGroup(channels, groupSelect.value), '');
      });
      channelSelect.addEventListener('change', () => { manualInput.value = ''; });
      manualInput.addEventListener('input', () => {
        if (manualInput.value.trim() !== '') channelSelect.value = '';
      });
      status.textContent = String(groups.length) + ' Kanalgruppen · ' + String(channels.length) + ' Kanäle.';
    }).catch(error => {
      groupField.hidden = true;
      channelSelect.replaceChildren(option('', 'Kanalliste konnte nicht geladen werden', true));
      channelSelect.disabled = true;
      manualInput.value = timer.channelId;
      status.textContent = 'Kanalliste konnte nicht geladen werden. Bitte Expertenoption verwenden: ' +
        String(error && error.message ? error.message : error);
    });
  }

  function createChannelFields(grid, timer) {
    const groupSelect = selectInput('channelGroup');
    const groupField = createField('Kanalgruppe', groupSelect, true);
    groupField.hidden = true;

    const channelSelect = selectInput('channelId');
    const channelField = createField('Kanal auswählen', channelSelect, true);
    const status = document.createElement('p');
    status.className = 'timer-selector-status';
    channelField.appendChild(status);

    const expert = document.createElement('details');
    expert.className = 'timer-expert';
    expert.appendChild(addText(document.createElement('summary'), 'Expertenoption: Kanal-ID manuell eingeben'));
    const manualInput = textInput('manualChannelId', '');
    expert.appendChild(createField('Manuelle Kanal-ID (überschreibt Auswahl)', manualInput, true));

    grid.appendChild(groupField);
    grid.appendChild(channelField);
    grid.appendChild(expert);
    global.setTimeout(() => {
      configureChannelSelectors(groupField, groupSelect, channelSelect, manualInput, status, timer);
    }, 0);
  }

  function populateDirectorySelect(select, directories, selectedDirectory) {
    const normalizedSelected = normalizeTimerDirectory(selectedDirectory);
    select.replaceChildren();
    directories.forEach(directory => {
      select.appendChild(option(
        directory,
        directoryOptionLabel(directory),
        directory === normalizedSelected
      ));
    });
    select.disabled = false;
  }

  function configureDirectorySelector(select, manualInput, status, timer) {
    status.textContent = 'Aufnahmeverzeichnisse werden geladen …';
    select.disabled = true;
    select.replaceChildren(option('', 'Aufnahmeverzeichnisse werden geladen …', true));

    fetchRecordingDirectories().then(folderData => {
      const directories = directoryOptions(folderData, lastTimerData, timer.directory);
      populateDirectorySelect(select, directories, timer.directory);
      const existingCount = Math.max(0, directories.length - 1);
      status.textContent = existingCount > 0
        ? String(existingCount) + ' vorhandene Aufnahmeverzeichnisse · Stammverzeichnis verfügbar.'
        : 'Keine vorhandenen Aufnahmeverzeichnisse gefunden · Stammverzeichnis verfügbar.';

      select.addEventListener('change', () => { manualInput.value = ''; });
      manualInput.addEventListener('input', () => {
        if (manualInput.value.trim() !== '') select.value = '';
      });
    }).catch(error => {
      const fallback = directoryOptions(null, lastTimerData, timer.directory);
      populateDirectorySelect(select, fallback, timer.directory);
      status.textContent = 'Aufnahmeverzeichnisse konnten nicht vollständig geladen werden. ' +
        'Bekannte Timer-Verzeichnisse bleiben auswählbar; weitere Verzeichnisse bitte über die Expertenoption eingeben: ' +
        String(error && error.message ? error.message : error);
    });
  }

  function createDirectoryFields(grid, timer) {
    const directorySelect = selectInput('directory');
    const directoryField = createField('Aufnahmeverzeichnis auswählen', directorySelect, true);
    const status = document.createElement('p');
    status.className = 'timer-selector-status';
    directoryField.appendChild(status);

    const expert = document.createElement('details');
    expert.className = 'timer-expert';
    expert.appendChild(addText(document.createElement('summary'), 'Expertenoption: Verzeichnis manuell eingeben'));
    const manualInput = textInput('manualDirectory', '');
    expert.appendChild(createField(
      'Manuelles Verzeichnis (überschreibt Auswahl, Unterordner mit ~ trennen)',
      manualInput,
      true
    ));

    grid.appendChild(directoryField);
    grid.appendChild(expert);
    global.setTimeout(() => {
      configureDirectorySelector(directorySelect, manualInput, status, timer);
    }, 0);
  }

  function createTimerForm(timer, mode) {
    const form = document.createElement('form');
    form.className = 'timer-form';
    if (mode === 'update') form.appendChild(textInput('timerId', timer.timerId, 'hidden'));

    const grid = document.createElement('div');
    grid.className = 'timer-form-grid';
    const title = textInput('title', mode === 'create' && timer.title === 'Timer' ? '' : timer.title);
    title.required = true;

    grid.appendChild(createField('Titel', title, true));
    createChannelFields(grid, timer);
    createDirectoryFields(grid, timer);
    grid.appendChild(createField('Datum', textInput('day', timer.day, 'date')));
    grid.appendChild(createField('Start', textInput('start', timeToInput(timer.start), 'time')));
    grid.appendChild(createField('Ende', textInput('stop', timeToInput(timer.stop), 'time')));
    grid.appendChild(createField('Priorität', textInput('priority', timer.priority || 50, 'number')));
    grid.appendChild(createField('Lebensdauer', textInput('lifetime', timer.lifetime || 99, 'number')));
    grid.appendChild(checkboxField('active', 'Aktiv', mode === 'create' ? true : timer.active));
    grid.appendChild(checkboxField('vps', 'VPS/PDC verwenden', timer.vps));
    grid.appendChild(createField('Aux/Metadaten', textInput('aux', timer.aux), true));

    const weekdays = document.createElement('div');
    weekdays.className = 'timer-field wide';
    weekdays.appendChild(addText(document.createElement('span'), 'Wiederholungstage (optional statt Datum)'));
    appendWeekdayInputs(weekdays, timer.weekdays);
    grid.appendChild(weekdays);
    form.appendChild(grid);

    const actions = document.createElement('div');
    actions.className = 'timer-actions';
    const submit = addText(document.createElement('button'), mode === 'create' ? 'Timer erstellen' : 'Änderungen speichern');
    submit.type = 'submit';
    submit.className = 'primary';
    actions.appendChild(submit);
    form.appendChild(actions);

    const feedback = document.createElement('div');
    feedback.className = 'timer-feedback';
    feedback.setAttribute('role', 'status');
    feedback.setAttribute('aria-live', 'polite');
    form.appendChild(feedback);

    form.addEventListener('submit', event => {
      event.preventDefault();
      const payload = formPayload(form, timer);
      const errors = validateTimerPayload(payload, mode === 'update');
      if (errors.length > 0) {
        setFeedback(feedback, errors.join(' '), true);
        return;
      }
      executeAction(mode, payload, submit, feedback);
    });
    return form;
  }

  function appendMeta(parent, label, value) {
    const item = document.createElement('div');
    item.className = 'timer-meta';
    item.appendChild(addText(document.createElement('span'), label));
    item.appendChild(addText(document.createElement('strong'), value === '' ? '-' : value));
    parent.appendChild(item);
  }

  function timerDeleteLabel(timer) {
    return timer && timer.recording ? 'Aufnahme stoppen' : 'Timer löschen';
  }

  function timerDeletePrompt(timer) {
    const title = timer && timer.title ? timer.title : 'Timer';
    return timer && timer.recording
      ? 'Laufende Aufnahme „' + title + '“ wirklich stoppen und den zugehörigen Timer löschen?'
      : 'Timer „' + title + '“ wirklich löschen?';
  }

  function timerStatus(timer) {
    if (timer.recording) return 'nimmt auf';
    if (timer.pending) return 'wartend';
    return timer.active ? 'aktiv' : 'inaktiv';
  }

  function timerScheduleLabel(timer) {
    const date = timer.weekdays !== '-------' ? timer.weekdays : (timer.day || 'Datum unbekannt');
    return date + ' · ' + timeToInput(timer.start) + '–' + timeToInput(timer.stop);
  }

  function conflictTimerIndices(report) {
    const indices = new Set();
    const conflicts = report && Array.isArray(report.conflicts) ? report.conflicts : [];
    conflicts.forEach(conflict => {
      (Array.isArray(conflict.entries) ? conflict.entries : []).forEach(entry => {
        const index = Number(firstValue(entry, ['timerIndex'], 0));
        if (index > 0) indices.add(index);
      });
    });
    return indices;
  }

  function timerConflictTimerLabel(timers, timerIndex) {
    const index = Number(timerIndex);
    const timer = Array.isArray(timers) && index > 0 ? timers[index - 1] : null;
    return timer ? 'Timer #' + String(index) + ': ' + timer.title : 'Timer #' + String(timerIndex);
  }

  function badge(text, className) {
    const item = addText(document.createElement('span'), text);
    item.className = 'timer-badge ' + className;
    return item;
  }

  function createTimerCard(timer, hasConflict) {
    const card = document.createElement('details');
    card.className = 'timer-card';
    if (hasConflict) card.classList.add('conflict');
    if (timer.recording) card.classList.add('recording');

    const summary = document.createElement('summary');
    const summaryGrid = document.createElement('div');
    summaryGrid.className = 'timer-card-summary';
    const title = document.createElement('div');
    title.className = 'timer-card-title';
    title.appendChild(addText(document.createElement('strong'), timer.title));
    title.appendChild(addText(document.createElement('span'), timer.channelName + ' · ' + timerScheduleLabel(timer)));
    summaryGrid.appendChild(title);

    const badges = document.createElement('div');
    badges.className = 'timer-badges';
    badges.appendChild(badge(timerStatus(timer), timer.recording ? 'recording' : timer.pending ? 'pending' : timer.active ? 'active' : 'inactive'));
    if (timer.vps) badges.appendChild(badge('VPS', 'vps'));
    if (hasConflict) badges.appendChild(badge('Konflikt', 'conflict'));
    summaryGrid.appendChild(badges);
    summary.appendChild(summaryGrid);
    card.appendChild(summary);

    const body = document.createElement('div');
    body.className = 'timer-card-body';
    const meta = document.createElement('div');
    meta.className = 'timer-meta-grid';
    appendMeta(meta, 'Kanal', timer.channelName);
    appendMeta(meta, 'Kanal-ID', timer.channelId);
    appendMeta(meta, 'Timer-ID', timer.timerId);
    appendMeta(meta, 'Verzeichnis', directoryOptionLabel(timer.directory));
    appendMeta(meta, 'Priorität / Lebensdauer', String(timer.priority) + ' / ' + String(timer.lifetime));
    if (timer.subtitle) appendMeta(meta, 'Untertitel', timer.subtitle);
    if (timer.eventId) appendMeta(meta, 'Event-ID', timer.eventId);
    body.appendChild(meta);

    const editor = document.createElement('details');
    editor.className = 'timer-create-panel';
    editor.appendChild(addText(document.createElement('summary'), 'Timer bearbeiten'));
    const editBody = document.createElement('div');
    editBody.className = 'timer-create-body';
    editBody.appendChild(createTimerForm(timer, 'update'));
    editor.appendChild(editBody);
    body.appendChild(editor);

    const actions = document.createElement('div');
    actions.className = 'timer-actions';
    const feedback = document.createElement('div');
    feedback.className = 'timer-feedback';
    feedback.setAttribute('role', 'status');

    const toggle = addText(document.createElement('button'), timer.active ? 'Deaktivieren' : 'Aktivieren');
    toggle.type = 'button';
    toggle.disabled = timer.recording;
    toggle.addEventListener('click', () => {
      executeAction('update', timerActionPayload(timer, {active: !timer.active}), toggle, feedback);
    });
    actions.appendChild(toggle);

    const remove = addText(document.createElement('button'), timerDeleteLabel(timer));
    remove.type = 'button';
    remove.className = 'danger';
    remove.title = timer.recording
      ? 'Laufende Aufnahme beenden und den zugehörigen Timer löschen.'
      : '';
    remove.addEventListener('click', () => {
      if (!global.confirm(timerDeletePrompt(timer))) return;
      executeAction('delete', {backendId: selectedBackendId(), timerId: timer.timerId}, remove, feedback);
    });
    actions.appendChild(remove);

    body.appendChild(actions);
    body.appendChild(feedback);
    card.appendChild(body);
    return card;
  }

  function appendConflictPanel(parent, report, timers, error) {
    const panel = document.createElement('article');
    panel.className = 'module-placeholder timer-conflict-panel';
    panel.dataset.timerConflictPanel = 'true'; // data-timer-conflict-panel

    if (error) {
      panel.appendChild(addText(document.createElement('h3'), 'Timer-Konflikte konnten nicht geladen werden'));
      panel.appendChild(addText(document.createElement('p'), error.message));
      parent.appendChild(panel);
      return;
    }

    if (report && report.available === false) {
      panel.appendChild(addText(document.createElement('h3'), 'Timer-Konfliktprüfung nicht verfügbar'));
      panel.appendChild(addText(document.createElement('p'), String(firstValue(report, ['error'], 'Der Konflikt-Endpunkt ist nicht verfügbar.'))));
      parent.appendChild(panel);
      return;
    }

    const conflicts = report && Array.isArray(report.conflicts) ? report.conflicts : [];
    const count = Number(firstValue(report || {}, ['count'], conflicts.length));
    panel.classList.add(count > 0 ? 'timer-conflict-panel-alert' : 'timer-conflict-panel-ok');
    panel.appendChild(addText(document.createElement('h3'), count > 0 ? 'Timer-Konflikte: ' + String(count) : 'Keine Timer-Konflikte gemeldet'));
    panel.appendChild(addText(document.createElement('p'), 'Quelle: ' + String(firstValue(report || {}, ['source'], 'unbekannt'))));

    if (conflicts.length > 0) {
      const list = document.createElement('div');
      list.className = 'timer-conflict-list';
      conflicts.slice(0, 10).forEach((conflict, index) => {
        const item = document.createElement('div');
        item.className = 'timer-conflict-item';
        item.appendChild(addText(document.createElement('strong'), 'Konflikt ' + String(index + 1)));
        (Array.isArray(conflict.entries) ? conflict.entries : []).forEach(entry => {
          const timerIndex = firstValue(entry, ['timerIndex'], '?');
          item.appendChild(addText(document.createElement('div'), timerConflictTimerLabel(timers, timerIndex) + ' · ' + String(firstValue(entry, ['percentage'], '?')) + '%'));
        });
        list.appendChild(item);
      });
      panel.appendChild(list);
    }
    parent.appendChild(panel);
  }

  function renderList(data, conflictReport) {
    installStyles();
    lastTimerData = data;
    if (conflictReport !== undefined && conflictReport !== null) lastConflictReport = conflictReport;

    const mountTarget = timerBrowserContext.detailDataElement;
    if (!mountTarget) throw new Error('Timer browser mount target is not configured');

    const timers = listFromResponse(data, 'timers').map(normalizeTimer);
    const conflicts = conflictTimerIndices(lastConflictReport);
    mountTarget.replaceChildren();

    const list = document.createElement('section');
    list.className = 'list timer-module';
    const summary = document.createElement('article');
    summary.className = 'module-placeholder timer-summary';
    summary.appendChild(addText(document.createElement('h3'), 'Timer'));
    summary.appendChild(addText(document.createElement('p'), String(timers.length) + ' Timer geladen.'));
    list.appendChild(summary);

    if (lastConflictReport) appendConflictPanel(list, lastConflictReport, timers, null);

    const createPanel = document.createElement('details');
    createPanel.className = 'timer-create-panel';
    createPanel.appendChild(addText(document.createElement('summary'), 'Neuen Timer erstellen'));
    const createBody = document.createElement('div');
    createBody.className = 'timer-create-body';
    createBody.appendChild(createTimerForm(normalizeTimer({priority: 50, lifetime: 99, enabled: true}, 0), 'create'));
    createPanel.appendChild(createBody);
    list.appendChild(createPanel);

    if (timers.length === 0) {
      const empty = document.createElement('article');
      empty.className = 'module-placeholder';
      empty.appendChild(addText(document.createElement('h3'), 'Keine Timer gefunden'));
      empty.appendChild(addText(document.createElement('p'), 'Der VDR hat aktuell keine Timer geliefert.'));
      list.appendChild(empty);
    } else {
      timers.forEach((timer, index) => list.appendChild(createTimerCard(timer, conflicts.has(index + 1))));
    }
    mountTarget.appendChild(list);
  }

  function renderConflicts(report, timers, error) {
    if (error) {
      const mountTarget = timerBrowserContext.detailDataElement;
      if (!mountTarget) return;
      const target = mountTarget.querySelector('.timer-module') || mountTarget;
      const previous = target.querySelector('[data-timer-conflict-panel="true"]');
      if (previous) previous.remove();
      appendConflictPanel(target, null, Array.isArray(timers) ? timers.map(normalizeTimer) : [], error);
      return;
    }

    lastConflictReport = report;
    if (lastTimerData !== null) {
      renderList(lastTimerData, report);
      return;
    }
    renderList({timers: Array.isArray(timers) ? timers : []}, report);
  }

  const timerBrowserApi = Object.freeze({
    configureContext: configureContext,
    renderList: renderList,
    renderConflicts: renderConflicts,
    normalizeTimer: normalizeTimer,
    normalizeChannel: normalizeChannel,
    normalizeChannels: normalizeChannels,
    channelGroups: channelGroups,
    channelsForGroup: channelsForGroup,
    channelOptionLabel: channelOptionLabel,
    normalizeTimerDirectory: normalizeTimerDirectory,
    recordingFolderPathToTimerDirectory: recordingFolderPathToTimerDirectory,
    directoryOptionLabel: directoryOptionLabel,
    directoryOptions: directoryOptions,
    timerActionPayload: timerActionPayload,
    timerDeleteLabel: timerDeleteLabel,
    timerDeletePrompt: timerDeletePrompt,
    validateTimerPayload: validateTimerPayload,
    inputTimeToHhmm: inputTimeToHhmm,
    timeToInput: timeToInput,
    weekdaysFromValues: weekdaysFromValues,
    normalizeWeekdays: normalizeWeekdays
  });

  global.VdrSuiteTimerBrowser = timerBrowserApi;
  if (global.VdrSuitePlatform &&
      typeof global.VdrSuitePlatform.registerModule === 'function' &&
      typeof global.VdrSuitePlatform.hasModule === 'function' &&
      !global.VdrSuitePlatform.hasModule('timers')) {
    global.VdrSuitePlatform.registerModule('timers', timerBrowserApi);
  }
})(window);
