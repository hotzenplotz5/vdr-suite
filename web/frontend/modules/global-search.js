(function (global) {
  'use strict';

  const MINIMUM_QUERY_LENGTH = 2;
  const DEBOUNCE_MS = 280;
  const REQUEST_TIMEOUT_MS = 12000;
  const state = {
    query: '',
    result: null,
    status: 'empty',
    error: null,
    scrollTop: 0,
    open: false
  };
  let dialog = null;
  let input = null;
  let results = null;
  let status = null;
  let scroll = null;
  let debounceTimer = null;

  const text = (value, fallback) => String(
    value === undefined || value === null ? (fallback || '') : value
  ).trim();

  function selectedBackendId() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedBackendId === 'function') {
      return text(platform.getSelectedBackendId(), 'default');
    }
    return 'default';
  }

  function node(tag, className, value) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (value !== undefined) element.textContent = text(value);
    return element;
  }

  function button(label, className, handler) {
    const element = node('button', className, label);
    element.type = 'button';
    if (handler) element.addEventListener('click', handler);
    return element;
  }

  function createRequestCoordinator(request, timeoutMs) {
    let sequence = 0;
    let controller = null;
    return {
      run: function (value) {
        sequence += 1;
        const current = sequence;
        if (controller && typeof controller.abort === 'function') controller.abort();
        controller = typeof global.AbortController === 'function'
          ? new global.AbortController()
          : null;
        const requestController = controller;
        let timedOut = false;
        const timeout = requestController && timeoutMs > 0
          ? global.setTimeout(function () {
              timedOut = true;
              requestController.abort();
            }, timeoutMs)
          : null;
        return Promise.resolve(request(value, requestController ? requestController.signal : undefined))
          .then(function (payload) {
            return {current: current === sequence, payload: payload};
          })
          .catch(function (error) {
            if (error && error.name === 'AbortError') {
              if (timedOut) {
                return {
                  current: current === sequence,
                  error: new Error('Die Suche dauerte zu lange und wurde abgebrochen.')
                };
              }
              return {current: false, aborted: true, error: error};
            }
            return {current: current === sequence, error: error};
          })
          .finally(function () {
            if (timeout !== null) global.clearTimeout(timeout);
          });
      },
      invalidate: function () {
        sequence += 1;
        if (controller && typeof controller.abort === 'function') controller.abort();
        controller = null;
      }
    };
  }

  const coordinator = createRequestCoordinator(function (query, signal) {
    const api = global.VdrSuiteClientApi;
    if (!api || typeof api.fetchClientGlobalSearch !== 'function') {
      return Promise.reject(new Error('Die VDR-Suite-Such-API ist nicht verfügbar.'));
    }
    return api.fetchClientGlobalSearch({
      backendId: selectedBackendId(),
      signal: signal,
      cache: 'no-store',
      query: {
        query: query,
        limit: 24,
        offset: 0,
        _: Date.now()
      }
    });
  }, REQUEST_TIMEOUT_MS);

  function installStyles() {
    const recordingsShared = global.VdrSuiteRecordings2Shared;
    if (recordingsShared && typeof recordingsShared.installStyles === 'function') {
      recordingsShared.installStyles();
    }
    if (document.getElementById('vdr-suite-global-search-style')) return;
    const style = node('style');
    style.id = 'vdr-suite-global-search-style';
    style.textContent = `
      .global-search-dialog{box-sizing:border-box;width:min(52rem,calc(100vw - 1rem));max-height:min(90dvh,54rem);padding:0;border:1px solid rgba(96,165,250,.5);border-radius:1.1rem;background:#07111f;color:#e2e8f0;box-shadow:0 2rem 6rem rgba(2,6,23,.72)}
      .global-search-dialog::backdrop{background:rgba(2,6,23,.76);backdrop-filter:blur(3px)}
      .global-search-shell{display:grid;grid-template-rows:auto auto minmax(0,1fr);max-height:min(90dvh,54rem)}
      .global-search-header{display:flex;align-items:flex-start;justify-content:space-between;gap:1rem;padding:1rem 1rem .7rem;border-bottom:1px solid #1e293b}
      .global-search-header h2{margin:0;color:#f8fafc;font-size:1.35rem}.global-search-header p{margin:.2rem 0 0;color:#94a3b8;font-size:.82rem}
      .global-search-close{display:grid;place-items:center;min-width:2.75rem;min-height:2.75rem;border:1px solid #475569;border-radius:.75rem;background:#172033;color:#f8fafc;font-size:1.35rem;cursor:pointer}
      .global-search-form{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:.55rem;padding:.75rem 1rem;border-bottom:1px solid #1e293b}
      .global-search-input{box-sizing:border-box;width:100%;min-height:3rem;padding:.72rem .9rem;border:1px solid #475569;border-radius:.8rem;background:#020617;color:#f8fafc;font:inherit;font-size:1rem}
      .global-search-input:focus{border-color:#38bdf8;outline:2px solid rgba(56,189,248,.25)}
      .global-search-clear{min-width:3rem;border:1px solid #475569;border-radius:.8rem;background:#172033;color:#e2e8f0;font-weight:800;cursor:pointer}
      .global-search-scroll{min-height:12rem;overflow-y:auto;overscroll-behavior:contain;padding:.85rem 1rem 1rem}
      .global-search-status{padding:.85rem;border:1px solid #334155;border-radius:.85rem;background:#0f172a;color:#cbd5e1}.global-search-status.error{border-color:#ef4444;background:#450a0a;color:#fecaca}
      .global-search-progress{width:100%;margin-top:.6rem}
      .global-search-group{display:grid;gap:.65rem;margin-bottom:1.1rem}.global-search-group-header{display:flex;align-items:baseline;justify-content:space-between;gap:.7rem}.global-search-group-header h3{margin:0;color:#f8fafc;font-size:1.08rem}.global-search-group-count{color:#93c5fd;font-size:.78rem;font-weight:800}
      .global-search-recordings,.global-search-epg{display:grid;grid-template-columns:repeat(auto-fill,minmax(18rem,1fr));gap:.7rem}
      .global-search-recording-wrap{display:grid;gap:.3rem}.global-search-recording-wrap .recordings2-recording{height:100%}
      .global-search-reason{display:inline-flex;justify-self:start;max-width:100%;padding:.28rem .5rem;border:1px solid #334155;border-radius:999px;background:#0f172a;color:#bae6fd;font-size:.72rem;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
      .global-search-epg-card{display:grid;grid-template-columns:6.5rem minmax(0,1fr);min-height:9rem;padding:0;overflow:hidden;border:1px solid #334155;border-radius:.9rem;background:#0f172a;color:#e2e8f0;text-align:left;cursor:pointer}
      .global-search-epg-image,.global-search-epg-fallback{display:block;width:100%;height:100%;min-height:9rem;object-fit:cover;background:#020617}.global-search-epg-fallback{display:grid;place-items:center;color:#38bdf8;font-size:2rem}
      .global-search-epg-copy{display:grid;align-content:start;gap:.32rem;padding:.7rem}.global-search-epg-title{color:#f8fafc;font-weight:850}.global-search-epg-subtitle{color:#cbd5e1;font-size:.82rem}.global-search-epg-meta{color:#93c5fd;font-size:.78rem}.global-search-epg-reason{color:#bae6fd;font-size:.74rem}
      .global-search-people{display:flex;gap:.5rem;overflow-x:auto;padding-bottom:.25rem}.global-search-person{display:grid;grid-template-columns:2.5rem minmax(0,1fr);align-items:center;gap:.5rem;flex:0 0 auto;min-width:13rem;padding:.4rem .6rem .4rem .4rem;border:1px solid #334155;border-radius:.85rem;background:#0f172a;color:#dbeafe;font-size:.78rem}.global-search-person-image,.global-search-person-placeholder{display:grid;place-items:center;width:2.5rem;height:2.5rem;border-radius:.7rem;background:#172554;object-fit:cover;color:#7dd3fc;font-size:1.15rem}.global-search-person-copy{display:grid;gap:.12rem;min-width:0}.global-search-person-name{overflow:hidden;color:#f8fafc;font-weight:850;text-overflow:ellipsis;white-space:nowrap}.global-search-person-meta{color:#93c5fd;font-size:.7rem}
      @media(max-width:720px){.global-search-dialog{width:calc(100vw - .5rem);max-height:96dvh;border-radius:.85rem}.global-search-shell{max-height:96dvh}.global-search-header{padding:.8rem}.global-search-form{padding:.65rem .8rem}.global-search-scroll{padding:.7rem .8rem 1rem}.global-search-recordings,.global-search-epg{grid-template-columns:1fr}.global-search-epg-card{grid-template-columns:5.6rem minmax(0,1fr)}.global-search-header p{display:none}}
    `;
    document.head.appendChild(style);
  }

  function formatDateTime(value) {
    const raw = Number(value);
    const date = Number.isFinite(raw) && raw > 0
      ? new Date(raw * 1000)
      : new Date(String(value || ''));
    if (!Number.isFinite(date.getTime())) return '–';
    return new Intl.DateTimeFormat('de-DE', {
      weekday: 'short', day: '2-digit', month: '2-digit',
      hour: '2-digit', minute: '2-digit'
    }).format(date);
  }

  function matchReason(item) {
    if (item && item.matchedPerson) {
      return 'Treffer über ' + item.matchedPerson +
        (item.matchedRole ? ' · ' + item.matchedRole : '');
    }
    return item && item.matchReason === 'title-and-person'
      ? 'Titel- und Personentreffer'
      : 'Treffer im Titel oder Untertitel';
  }

  function ensureDetailRuntimes() {
    const tasks = [];
    if (typeof global.loadVdrSuiteRecordings2Runtime === 'function') {
      tasks.push(global.loadVdrSuiteRecordings2Runtime());
    }
    if (typeof global.loadVdrSuiteGenresRuntime === 'function') {
      tasks.push(global.loadVdrSuiteGenresRuntime());
    }
    return Promise.all(tasks);
  }

  function rememberScroll() {
    if (scroll) state.scrollTop = scroll.scrollTop;
  }

  function openRecording(recording) {
    rememberScroll();
    closeDialog(false);
    ensureDetailRuntimes().then(function () {
      const owner = global.VdrSuiteRecordings2;
      if (!owner || typeof owner.openRecording !== 'function') {
        throw new Error('Recordings 2 Detail-Owner ist nicht verfügbar.');
      }
      owner.openRecording(recording, {
        backendId: selectedBackendId(),
        backLabel: '← Zurück zur Suche',
        onClose: openDialog
      });
    }).catch(showNavigationError);
  }

  function openEpg(event) {
    rememberScroll();
    closeDialog(false);
    ensureDetailRuntimes().then(function () {
      const owner = global.VdrSuiteEpgDetailOwner;
      if (!owner || typeof owner.open !== 'function') {
        throw new Error('EPG Detail-Owner ist nicht verfügbar.');
      }
      owner.open(event, {
        id: event.channelId,
        channelId: event.channelId,
        name: event.channelName
      }, {
        backLabel: '← Zurück zur Suche',
        onClose: openDialog
      });
    }).catch(showNavigationError);
  }

  function showNavigationError(error) {
    state.error = error;
    state.status = 'error';
    openDialog();
    render();
  }

  function publicPersonImageUrl(value) {
    const url = text(value);
    return url.startsWith('/api/vdr/recordings/metadata/image?') ||
      url.startsWith('/api/recordings/metadata/image?');
  }

  function personPlaceholder() {
    return node('span', 'global-search-person-placeholder', '●');
  }

  function renderPeople(root, people) {
    if (!Array.isArray(people) || !people.length) return;
    const group = node('section', 'global-search-group');
    const header = node('div', 'global-search-group-header');
    header.append(node('h3', '', 'Personen'), node('span', 'global-search-group-count', people.length + ' Treffergründe'));
    group.appendChild(header);
    const list = node('div', 'global-search-people');
    people.forEach(function (person, index) {
      const entry = node('article', 'global-search-person');
      const imageUrl = person && person.image && person.image.available === true
        ? text(person.image.url)
        : '';
      if (publicPersonImageUrl(imageUrl)) {
        const image = node('img', 'global-search-person-image');
        image.src = imageUrl;
        image.alt = text(person.name, 'Person');
        image.decoding = 'async';
        image.loading = index < 4 ? 'eager' : 'lazy';
        if (index < 2) image.fetchPriority = 'high';
        image.addEventListener('error', function () {
          image.replaceWith(personPlaceholder());
        }, {once: true});
        entry.appendChild(image);
      } else {
        entry.appendChild(personPlaceholder());
      }
      const copy = node('span', 'global-search-person-copy');
      copy.appendChild(node(
        'span',
        'global-search-person-name',
        person.name + (person.role ? ' · ' + person.role : '')
      ));
      copy.appendChild(node(
        'span',
        'global-search-person-meta',
        Number(person.recordingCount || 0) + ' Aufn. / ' +
          Number(person.epgCount || 0) + ' EPG'
      ));
      entry.appendChild(copy);
      list.appendChild(entry);
    });
    group.appendChild(list);
    root.appendChild(group);
  }

  function renderRecordings(root, payload) {
    const recordings = Array.isArray(payload.recordings) ? payload.recordings : [];
    const group = node('section', 'global-search-group');
    const header = node('div', 'global-search-group-header');
    header.append(node('h3', '', 'Aufnahmen'), node('span', 'global-search-group-count', Number(payload.recordingTotal || recordings.length) + ' Treffer'));
    group.appendChild(header);
    if (!recordings.length) {
      group.appendChild(node('div', 'global-search-status', 'Keine passenden Aufnahmen gefunden.'));
      root.appendChild(group);
      return;
    }
    const list = node('div', 'global-search-recordings recordings2-recording-list');
    recordings.forEach(function (recording) {
      const wrap = node('article', 'global-search-recording-wrap');
      const cardOwner = global.VdrSuiteRecordings2BrowserView;
      if (cardOwner && typeof cardOwner.createRecordingCard === 'function') {
        wrap.appendChild(cardOwner.createRecordingCard(recording, openRecording));
      } else {
        wrap.appendChild(button(recording.title || 'Ohne Titel', 'recordings2-recording', function () { openRecording(recording); }));
      }
      wrap.appendChild(node('span', 'global-search-reason', matchReason(recording)));
      list.appendChild(wrap);
    });
    group.appendChild(list);
    root.appendChild(group);
  }

  function renderEpg(root, payload) {
    const events = Array.isArray(payload.epg) ? payload.epg : [];
    const group = node('section', 'global-search-group');
    const header = node('div', 'global-search-group-header');
    header.append(node('h3', '', 'EPG'), node('span', 'global-search-group-count', Number(payload.epgTotal || events.length) + ' Treffer'));
    group.appendChild(header);
    if (!events.length) {
      group.appendChild(node('div', 'global-search-status', 'Keine passenden EPG-Ereignisse im Suchzeitraum gefunden.'));
      root.appendChild(group);
      return;
    }
    const list = node('div', 'global-search-epg');
    events.forEach(function (event) {
      const card = button('', 'global-search-epg-card', function () { openEpg(event); });
      const artwork = event.artwork && event.artwork.available && event.artwork.url;
      if (artwork) {
        const image = node('img', 'global-search-epg-image');
        image.src = artwork;
        image.alt = '';
        image.loading = 'lazy';
        image.decoding = 'async';
        image.addEventListener('error', function () {
          image.replaceWith(node('span', 'global-search-epg-fallback', '◉'));
        }, {once: true});
        card.appendChild(image);
      } else {
        card.appendChild(node('span', 'global-search-epg-fallback', '◉'));
      }
      const copy = node('span', 'global-search-epg-copy');
      copy.appendChild(node('span', 'global-search-epg-title', event.title || 'Ohne Titel'));
      if (event.subtitle) copy.appendChild(node('span', 'global-search-epg-subtitle', event.subtitle));
      copy.appendChild(node('span', 'global-search-epg-meta', (event.channelName || event.channelId || 'Unbekannter Sender') + ' · ' + formatDateTime(event.startTime) + '–' + formatDateTime(event.endTime).split(', ').pop()));
      copy.appendChild(node('span', 'global-search-epg-reason', matchReason(event)));
      card.appendChild(copy);
      list.appendChild(card);
    });
    group.appendChild(list);
    root.appendChild(group);
  }

  function render() {
    if (!results || !status) return;
    results.replaceChildren();
    status.className = 'global-search-status' + (state.status === 'error' ? ' error' : '');
    status.hidden = false;

    if (state.status === 'empty') {
      status.textContent = 'Suche nach Filmen, Sendungen, Aufnahmen oder Personen.';
      return;
    }
    if (state.status === 'too-short') {
      status.textContent = 'Bitte mindestens ' + MINIMUM_QUERY_LENGTH + ' Zeichen eingeben.';
      return;
    }
    if (state.status === 'loading') {
      status.replaceChildren(node('strong', '', 'Suche läuft …'));
      const progress = node('progress', 'global-search-progress');
      progress.setAttribute('aria-label', 'Suche läuft');
      status.appendChild(progress);
      return;
    }
    if (state.status === 'error') {
      status.textContent = 'Die Suche konnte nicht geladen werden: ' + text(state.error && state.error.message, state.error || 'Unbekannter Fehler');
      return;
    }

    const payload = state.result || {};
    const recordings = Array.isArray(payload.recordings) ? payload.recordings : [];
    const epg = Array.isArray(payload.epg) ? payload.epg : [];
    if (!recordings.length && !epg.length) {
      status.textContent = 'Keine Treffer für „' + state.query + '“ gefunden.';
      return;
    }

    status.hidden = true;
    const content = node('div', 'global-search-content');
    renderPeople(content, payload.people);
    renderRecordings(content, payload);
    renderEpg(content, payload);
    results.appendChild(content);
    global.setTimeout(function () {
      if (scroll) scroll.scrollTop = state.scrollTop;
    }, 0);
  }

  function performSearch(query) {
    const normalized = text(query);
    state.query = normalized;
    state.error = null;
    state.scrollTop = 0;
    if (!normalized) {
      coordinator.invalidate();
      state.status = 'empty';
      state.result = null;
      return render();
    }
    if (normalized.length < MINIMUM_QUERY_LENGTH) {
      coordinator.invalidate();
      state.status = 'too-short';
      state.result = null;
      return render();
    }

    state.status = 'loading';
    render();
    coordinator.run(normalized).then(function (outcome) {
      if (!outcome.current) return;
      if (outcome.error) {
        state.error = outcome.error;
        state.status = 'error';
        state.result = null;
      } else {
        state.result = outcome.payload || {};
        state.status = text(state.result.status, 'ready');
        if (state.status !== 'ready') {
          state.status = state.status === 'too-short' ? 'too-short' : 'ready';
        }
      }
      render();
    });
  }

  function scheduleSearch() {
    if (debounceTimer) global.clearTimeout(debounceTimer);
    debounceTimer = global.setTimeout(function () {
      debounceTimer = null;
      performSearch(input ? input.value : '');
    }, DEBOUNCE_MS);
  }

  function buildDialog() {
    installStyles();
    dialog = node('dialog', 'global-search-dialog');
    dialog.id = 'vdr-suite-global-search';
    dialog.setAttribute('aria-labelledby', 'global-search-title');
    const shell = node('section', 'global-search-shell');
    const header = node('header', 'global-search-header');
    const copy = node('div');
    const title = node('h2', '', 'VDR-Suite Suche');
    title.id = 'global-search-title';
    copy.append(title, node('p', '', 'Lokale Suche im ausgewählten Backend · keine Providerabfrage'));
    header.append(copy, button('×', 'global-search-close', function () { closeDialog(true); }));

    const form = node('div', 'global-search-form');
    input = node('input', 'global-search-input');
    input.type = 'search';
    input.autocomplete = 'off';
    input.spellcheck = false;
    input.placeholder = 'Titel oder Person eingeben …';
    input.setAttribute('aria-label', 'Aufnahmen und EPG durchsuchen');
    input.addEventListener('input', scheduleSearch);
    input.addEventListener('keydown', function (event) {
      if (event.key === 'Enter') {
        event.preventDefault();
        if (debounceTimer) global.clearTimeout(debounceTimer);
        debounceTimer = null;
        performSearch(input.value);
      }
    });
    const clear = button('Leeren', 'global-search-clear', function () {
      input.value = '';
      performSearch('');
      input.focus();
    });
    form.append(input, clear);

    scroll = node('div', 'global-search-scroll');
    status = node('div', 'global-search-status');
    status.setAttribute('role', 'status');
    status.setAttribute('aria-live', 'polite');
    results = node('div', 'global-search-results');
    scroll.append(status, results);
    scroll.addEventListener('scroll', rememberScroll, {passive: true});
    shell.append(header, form, scroll);
    dialog.appendChild(shell);
    dialog.addEventListener('close', function () { state.open = false; });
    document.body.appendChild(dialog);
  }

  function openDialog() {
    if (!dialog) buildDialog();
    state.open = true;
    input.value = state.query;
    render();
    if (typeof dialog.showModal === 'function') {
      if (!dialog.open) dialog.showModal();
    } else {
      dialog.setAttribute('open', '');
    }
    global.setTimeout(function () {
      input.focus();
      input.setSelectionRange(input.value.length, input.value.length);
      if (scroll) scroll.scrollTop = state.scrollTop;
    }, 0);
  }

  function closeDialog(invalidate) {
    rememberScroll();
    state.open = false;
    if (invalidate) coordinator.invalidate();
    if (!dialog) return;
    if (typeof dialog.close === 'function' && dialog.open) dialog.close();
    else dialog.removeAttribute('open');
  }

  function installLauncher() {
    const launcher = document.querySelector('[data-brand-module="search"]');
    if (!launcher) return;
    const activate = function (event) {
      if (event && event.type === 'keydown' && event.key !== 'Enter' && event.key !== ' ') return;
      if (event) event.preventDefault();
      openDialog();
    };
    launcher.addEventListener('click', activate);
    launcher.addEventListener('keydown', activate);
  }

  const api = Object.freeze({
    open: openDialog,
    close: function () { closeDialog(true); },
    search: performSearch,
    __test: Object.freeze({
      createRequestCoordinator: createRequestCoordinator,
      publicPersonImageUrl: publicPersonImageUrl,
      renderPeople: renderPeople,
      minimumQueryLength: MINIMUM_QUERY_LENGTH,
      debounceMs: DEBOUNCE_MS,
      requestTimeoutMs: REQUEST_TIMEOUT_MS,
      state: state
    })
  });
  global.VdrSuiteGlobalSearch = api;

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', installLauncher, {once: true});
  } else {
    installLauncher();
  }
}(window));