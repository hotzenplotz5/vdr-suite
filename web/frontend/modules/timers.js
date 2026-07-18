// Active Timer browser with safe create, edit, toggle and delete workflows.
(function(global) {
  'use strict';

  let timerBrowserContext = Object.freeze({});
  let lastTimerData = null;
  let lastConflictReport = null;
  let channelListSequence = 0;

  const WEEKDAY_CHARACTERS = 'MTWTFSS';
  const WEEKDAY_LABELS = ['Mo', 'Di', 'Mi', 'Do', 'Fr', 'Sa', 'So'];

  function configureContext(context) {
    timerBrowserContext = Object.freeze(Object.assign({}, context || {}));
  }

  function firstValue(source, keys, fallback) {
    const helpers = timerBrowserContext.helpers || global.VdrSuiteFrontendHelpers || null;

    if (helpers && typeof helpers.firstValue === 'function') {
      return helpers.firstValue(source, keys, fallback);
    }

    if (!source || !Array.isArray(keys)) {
      return fallback;
    }

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

  function boolValue(value, fallback) {
    if (value === true || value === 1 || value === '1' || value === 'true' || value === 'yes') {
      return true;
    }
    if (value === false || value === 0 || value === '0' || value === 'false' || value === 'no') {
      return false;
    }
    return Boolean(fallback);
  }

  function integerValue(value, fallback) {
    const number = Number(value);
    return Number.isFinite(number) ? Math.trunc(number) : Number(fallback || 0);
  }

  function normalizeWeekdays(value) {
    const source = String(value || '-------');
    let result = '';

    for (let index = 0; index < 7; index += 1) {
      result += source[index] && source[index] !== '-'
        ? WEEKDAY_CHARACTERS[index]
        : '-';
    }

    return result;
  }

  function normalizeTimer(timer, index) {
    const source = timer && typeof timer === 'object' ? timer : {};
    const flags = integerValue(firstValue(source, ['flags'], 0), 0);
    const id = String(firstValue(source, ['timerId', 'id', 'nativeId'], ''));
    const weekdays = normalizeWeekdays(firstValue(source, ['weekdays'], '-------'));

    return {
      source,
      index: Number(index) || 0,
      timerId: id,
      channelId: String(firstValue(source, ['channelId', 'channel'], '')),
      channelName: String(firstValue(source, ['channelName', 'channel'], '-')),
      eventId: String(firstValue(source, ['eventId'], '')),
      title: String(firstValue(source, ['title', 'name', 'file', 'eventTitle'], 'Timer')),
      directory: String(firstValue(source, ['directory', 'folder'], '')),
      subtitle: String(firstValue(source, ['subtitle'], '')),
      aux: String(firstValue(source, ['aux'], '')),
      day: String(firstValue(source, ['day', 'date'], '')),
      weekdays,
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

  function selectedBackendId() {
    if (typeof timerBrowserContext.getSelectedBackendId === 'function') {
      const id = String(timerBrowserContext.getSelectedBackendId() || '').trim();
      if (id !== '') {
        return id;
      }
    }
    return 'default';
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
    const text = String(value || '').trim();
    const match = /^(\d{1,2}):(\d{2})$/.exec(text);
    if (!match) {
      return 0;
    }

    const hour = Number(match[1]);
    const minute = Number(match[2]);
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
      return 0;
    }

    return hour * 100 + minute;
  }

  function weekdaysFromValues(values) {
    const selected = new Set(Array.isArray(values) ? values.map(Number) : []);
    return WEEKDAY_CHARACTERS.split('').map((character, index) =>
      selected.has(index) ? character : '-'
    ).join('');
  }

  function timerActionPayload(timer, overrides) {
    const normalized = timer && timer.timerId !== undefined
      ? timer
      : normalizeTimer(timer || {}, 0);
    const changes = overrides && typeof overrides === 'object' ? overrides : {};

    return {
      backendId: String(changes.backendId !== undefined ? changes.backendId : selectedBackendId()),
      timerId: String(changes.timerId !== undefined ? changes.timerId : normalized.timerId),
      channelId: String(changes.channelId !== undefined ? changes.channelId : normalized.channelId),
      title: String(changes.title !== undefined ? changes.title : normalized.title),
      directory: String(changes.directory !== undefined ? changes.directory : normalized.directory),
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

    if (requireTimerId && String(payload.timerId || '').trim() === '') {
      errors.push('Timer-ID fehlt.');
    }
    if (String(payload.channelId || '').trim() === '') {
      errors.push('Kanal fehlt.');
    }
    if (String(payload.title || '').trim() === '') {
      errors.push('Titel fehlt.');
    }
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
    if (typeof document === 'undefined' || document.getElementById('vdr-suite-timer-workflow-styles')) {
      return;
    }

    const style = document.createElement('style');
    style.id = 'vdr-suite-timer-workflow-styles';
    style.textContent = `
.timer-module{display:grid;gap:.8rem}.timer-summary{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:.65rem}.timer-summary h3,.timer-card h3{margin:0}.timer-create-panel,.timer-card{border:1px solid rgba(96,165,250,.24);border-radius:.95rem;background:rgba(15,23,42,.72)}.timer-create-panel>summary,.timer-card>summary{cursor:pointer;padding:.8rem .9rem;color:#f8fafc;font-weight:850}.timer-create-panel[open]>summary,.timer-card[open]>summary{border-bottom:1px solid rgba(148,163,184,.18)}.timer-card.conflict{border-color:rgba(248,113,113,.72);box-shadow:0 0 0 1px rgba(248,113,113,.16)}.timer-card.recording{border-color:rgba(74,222,128,.62)}.timer-card-summary{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:.65rem;align-items:center}.timer-card-title{min-width:0}.timer-card-title strong{display:block;overflow:hidden;text-overflow:ellipsis}.timer-card-title span{display:block;margin-top:.15rem;color:#94a3b8;font-size:.86rem;font-weight:500}.timer-badges{display:flex;flex-wrap:wrap;justify-content:flex-end;gap:.32rem}.timer-badge{padding:.22rem .48rem;border-radius:999px;border:1px solid rgba(148,163,184,.28);background:rgba(2,6,23,.72);color:#cbd5e1;font-size:.74rem;font-weight:800}.timer-badge.active,.timer-badge.recording{border-color:rgba(74,222,128,.5);color:#bbf7d0}.timer-badge.inactive{border-color:rgba(148,163,184,.35);color:#cbd5e1}.timer-badge.pending,.timer-badge.vps{border-color:rgba(56,189,248,.48);color:#bae6fd}.timer-badge.conflict{border-color:rgba(248,113,113,.62);color:#fecaca}.timer-card-body,.timer-create-body{display:grid;gap:.72rem;padding:.82rem .9rem .9rem}.timer-meta-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(10rem,1fr));gap:.5rem}.timer-meta{padding:.55rem .62rem;border:1px solid rgba(148,163,184,.17);border-radius:.68rem;background:rgba(2,6,23,.52)}.timer-meta span{display:block;color:#94a3b8;font-size:.72rem;font-weight:750;text-transform:uppercase}.timer-meta strong{display:block;margin-top:.15rem;color:#f8fafc;overflow-wrap:anywhere}.timer-form{display:grid;gap:.72rem}.timer-form-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.62rem}.timer-field{display:grid;gap:.28rem;min-width:0}.timer-field.wide{grid-column:1/-1}.timer-field>span{color:#cbd5e1;font-size:.78rem;font-weight:750}.timer-field input{box-sizing:border-box;width:100%;min-width:0;min-height:2.55rem;padding:.5rem .62rem;border:1px solid #475569;border-radius:.62rem;background:#111827;color:#f8fafc;font:inherit}.timer-checkbox{display:flex;align-items:center;gap:.45rem;min-height:2.55rem}.timer-checkbox input{width:1.15rem;height:1.15rem}.timer-weekdays{display:flex;flex-wrap:wrap;gap:.35rem}.timer-weekday{display:flex;align-items:center;gap:.28rem;padding:.38rem .48rem;border:1px solid rgba(148,163,184,.24);border-radius:.58rem;background:rgba(2,6,23,.52)}.timer-actions{display:flex;flex-wrap:wrap;gap:.5rem}.timer-actions button{min-height:2.55rem;padding:.55rem .78rem;border-radius:.65rem}.timer-actions .danger{border-color:rgba(248,113,113,.55);color:#fecaca}.timer-actions .primary{border-color:rgba(56,189,248,.55)}.timer-feedback{min-height:1.2rem;padding:.58rem .65rem;border-radius:.62rem;background:rgba(2,6,23,.5);color:#cbd5e1}.timer-feedback.success{border:1px solid rgba(74,222,128,.4);color:#bbf7d0}.timer-feedback.error{border:1px solid rgba(248,113,113,.45);color:#fecaca}.timer-conflict-panel{order:-1}.timer-conflict-list{display:grid;gap:.5rem}.timer-conflict-item{padding:.58rem .65rem;border:1px solid rgba(248,113,113,.25);border-radius:.65rem;background:rgba(69,10,10,.22)}
@media(max-width:760px){.timer-card-summary{grid-template-columns:minmax(0,1fr)}.timer-badges{justify-content:flex-start}.timer-form-grid{grid-template-columns:minmax(0,1fr)}.timer-field.wide{grid-column:auto}.timer-actions{display:grid;grid-template-columns:minmax(0,1fr)}.timer-actions button{width:100%}.timer-meta-grid{grid-template-columns:minmax(0,1fr)}}
`;
    document.head.appendChild(style);
  }

  function createField(label, input, wide) {
    const field = document.createElement('label');
    field.className = 'timer-field' + (wide ? ' wide' : '');
    field.appendChild(addText(document.createElement('span'), label));
    field.appendChild(input);
    return field;
  }

  function textInput(name, value, type) {
    const input = document.createElement('input');
    input.name = name;
    input.type = type || 'text';
    input.value = value === undefined || value === null ? '' : String(value);
    return input;
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
    return Array.from(form.querySelectorAll('input[name="weekday"]:checked'))
      .map(input => Number(input.value));
  }

  function formPayload(form, timer) {
    const base = timer || normalizeTimer({}, 0);
    const weekdays = weekdaysFromValues(selectedWeekdays(form));
    const repeating = weekdays !== '-------';

    return timerActionPayload(base, {
      timerId: form.elements.timerId ? form.elements.timerId.value : base.timerId,
      channelId: form.elements.channelId.value.trim(),
      title: form.elements.title.value.trim(),
      directory: form.elements.directory.value.trim(),
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
    if (result && result.message) {
      return String(result.message);
    }
    if (result && Array.isArray(result.errors) && result.errors.length > 0) {
      return result.errors.join(' · ');
    }
    return fallback;
  }

  function actionClientFunction(type) {
    const clientApi = timerBrowserContext.clientApi || global.VdrSuiteClientApi || null;
    if (!clientApi) {
      return null;
    }

    const name = type === 'create'
      ? 'fetchClientTimerCreateAction'
      : type === 'update'
        ? 'fetchClientTimerUpdateAction'
        : 'fetchClientTimerDeleteAction';

    return typeof clientApi[name] === 'function'
      ? clientApi[name].bind(clientApi)
      : null;
  }

  function reloadTimers() {
    if (typeof timerBrowserContext.reload === 'function') {
      timerBrowserContext.reload();
    }
  }

  function executeAction(type, payload, button, feedback) {
    const action = actionClientFunction(type);
    if (!action) {
      setFeedback(feedback, 'Timer-Aktion ist nicht verfügbar.', true);
      return Promise.resolve(null);
    }

    button.disabled = true;
    setFeedback(feedback, 'Timer-Aktion wird ausgeführt …', false);

    return action({
      payload,
      cache: 'no-store',
      credentials: 'same-origin'
    })
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
      .finally(() => {
        button.disabled = false;
      });
  }

  function attachChannelSuggestions(input) {
    const clientApi = timerBrowserContext.clientApi || global.VdrSuiteClientApi || null;
    if (!clientApi || typeof clientApi.fetchClientChannels !== 'function') {
      return;
    }

    channelListSequence += 1;
    const listId = 'timer-channel-options-' + String(channelListSequence);
    const datalist = document.createElement('datalist');
    datalist.id = listId;
    input.setAttribute('list', listId);
    input.parentNode.appendChild(datalist);

    clientApi.fetchClientChannels({
      query: {backend: selectedBackendId(), _: String(Date.now())},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(data => {
      listFromResponse(data, 'channels').forEach(channel => {
        const option = document.createElement('option');
        option.value = String(firstValue(channel, ['id', 'channelId'], ''));
        option.label = String(firstValue(channel, ['name', 'channelName'], option.value));
        if (option.value !== '') {
          datalist.appendChild(option);
        }
      });
    }).catch(() => {});
  }

  function createTimerForm(timer, mode) {
    const normalized = timer || normalizeTimer({}, 0);
    const form = document.createElement('form');
    form.className = 'timer-form';

    if (mode === 'update') {
      const hiddenId = textInput('timerId', normalized.timerId, 'hidden');
      form.appendChild(hiddenId);
    }

    const grid = document.createElement('div');
    grid.className = 'timer-form-grid';

    const title = textInput('title', normalized.title === 'Timer' && mode === 'create' ? '' : normalized.title);
    title.required = true;
    grid.appendChild(createField('Titel', title, true));

    const channel = textInput('channelId', normalized.channelId);
    channel.required = true;
    grid.appendChild(createField('Kanal-ID', channel, true));

    grid.appendChild(createField('Verzeichnis', textInput('directory', normalized.directory), true));
    grid.appendChild(createField('Datum', textInput('day', normalized.day, 'date')));
    grid.appendChild(createField('Start', textInput('start', timeToInput(normalized.start), 'time')));
    grid.appendChild(createField('Ende', textInput('stop', timeToInput(normalized.stop), 'time')));
    grid.appendChild(createField('Priorität', textInput('priority', normalized.priority || 50, 'number')));
    grid.appendChild(createField('Lebensdauer', textInput('lifetime', normalized.lifetime || 99, 'number')));
    grid.appendChild(checkboxField('active', 'Aktiv', mode === 'create' ? true : normalized.active));
    grid.appendChild(checkboxField('vps', 'VPS/PDC verwenden', normalized.vps));
    grid.appendChild(createField('Aux/Metadaten', textInput('aux', normalized.aux), true));

    const weekdayField = document.createElement('div');
    weekdayField.className = 'timer-field wide';
    weekdayField.appendChild(addText(document.createElement('span'), 'Wiederholungstage (optional statt Datum)'));
    appendWeekdayInputs(weekdayField, normalized.weekdays);
    grid.appendChild(weekdayField);

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
      const payload = formPayload(form, normalized);
      const errors = validateTimerPayload(payload, mode === 'update');
      if (errors.length > 0) {
        setFeedback(feedback, errors.join(' '), true);
        return;
      }
      executeAction(mode, payload, submit, feedback);
    });

    global.setTimeout(() => attachChannelSuggestions(channel), 0);
    return form;
  }

  function appendMeta(parent, label, value) {
    const item = document.createElement('div');
    item.className = 'timer-meta';
    item.appendChild(addText(document.createElement('span'), label));
    item.appendChild(addText(document.createElement('strong'), value === '' ? '-' : value));
    parent.appendChild(item);
  }

  function timerStatus(timer) {
    if (timer.recording) return 'nimmt auf';
    if (timer.pending) return 'wartend';
    return timer.active ? 'aktiv' : 'inaktiv';
  }

  function timerScheduleLabel(timer) {
    const repeating = timer.weekdays !== '-------';
    const date = repeating ? timer.weekdays : (timer.day || 'Datum unbekannt');
    return date + ' · ' + timeToInput(timer.start) + '–' + timeToInput(timer.stop);
  }

  function conflictTimerIndices(report) {
    const indices = new Set();
    const conflicts = report && Array.isArray(report.conflicts) ? report.conflicts : [];
    conflicts.forEach(conflict => {
      const entries = Array.isArray(conflict.entries) ? conflict.entries : [];
      entries.forEach(entry => {
        const index = Number(firstValue(entry, ['timerIndex'], 0));
        if (index > 0) indices.add(index);
      });
    });
    return indices;
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
    appendMeta(meta, 'Kanal-ID', timer.channelId);
    appendMeta(meta, 'Timer-ID', timer.timerId);
    appendMeta(meta, 'Verzeichnis', timer.directory || '-');
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

    const remove = addText(document.createElement('button'), 'Timer löschen');
    remove.type = 'button';
    remove.className = 'danger';
    remove.disabled = timer.recording;
    remove.title = timer.recording ? 'Laufende Aufnahmen werden nicht über diese Oberfläche gelöscht.' : '';
    remove.addEventListener('click', () => {
      if (!global.confirm('Timer „' + timer.title + '“ wirklich löschen?')) {
        return;
      }
      executeAction('delete', {
        backendId: selectedBackendId(),
        timerId: timer.timerId
      }, remove, feedback);
    });
    actions.appendChild(remove);

    body.appendChild(actions);
    body.appendChild(feedback);
    card.appendChild(body);
    return card;
  }

  function appendConflictPanel(parent, report, error) {
    const panel = document.createElement('article');
    panel.className = 'module-placeholder timer-conflict-panel';
    panel.dataset.timerConflictPanel = 'true';

    if (error) {
      panel.classList.add('error');
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
          item.appendChild(addText(
            document.createElement('div'),
            'Timer #' + String(firstValue(entry, ['timerIndex'], '?')) +
              ' · ' + String(firstValue(entry, ['percentage'], '?')) + '%'
          ));
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
    if (conflictReport !== undefined && conflictReport !== null) {
      lastConflictReport = conflictReport;
    }

    const mountTarget = timerBrowserContext.detailDataElement;
    if (!mountTarget) {
      throw new Error('Timer browser mount target is not configured');
    }

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

    if (lastConflictReport) {
      appendConflictPanel(list, lastConflictReport, null);
    }

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
      timers.forEach((timer, index) => {
        list.appendChild(createTimerCard(timer, conflicts.has(index + 1)));
      });
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
      appendConflictPanel(target, null, error);
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
    configureContext,
    renderList,
    renderConflicts,
    normalizeTimer,
    timerActionPayload,
    validateTimerPayload,
    inputTimeToHhmm,
    timeToInput,
    weekdaysFromValues,
    normalizeWeekdays
  });

  global.VdrSuiteTimerBrowser = timerBrowserApi;

  if (global.VdrSuitePlatform &&
      typeof global.VdrSuitePlatform.registerModule === 'function' &&
      typeof global.VdrSuitePlatform.hasModule === 'function' &&
      !global.VdrSuitePlatform.hasModule('timers')) {
    global.VdrSuitePlatform.registerModule('timers', timerBrowserApi);
  }
})(window);
