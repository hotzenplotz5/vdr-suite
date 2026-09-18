// Phase 67 domain-first Teletext browser/TV view.
//
// This module owns only Teletext service/page presentation and navigation.
// It never owns Live playback, provider commands, plugin cache state or a
// second MediaSession. Closing Teletext therefore returns to the still-active
// canonical Live-TV owner.
(function(global) {
  'use strict';

  if (global.VdrSuiteTeletextView) return;

  const doc = global.document || (typeof document !== 'undefined' ? document : null);
  const state = {
    open: false,
    backendId: '',
    channelId: '',
    channelName: '',
    pageNumber: 100,
    automaticSubpage: true,
    subpageCode: 0,
    service: null,
    page: null,
    loading: false,
    error: '',
    requestSequence: 0,
    overlay: null
  };

  const COLORS = Object.freeze([
    '#000000', '#ff2b2b', '#38d878', '#ffe34d',
    '#3b82f6', '#d946ef', '#22d3ee', '#f8fafc'
  ]);

  function text(value) {
    return value === undefined || value === null ? '' : String(value).trim();
  }

  function channelId(channel) {
    return text(channel && (channel.channelId || channel.id || channel.nativeId));
  }

  function channelName(channel) {
    return text(channel && (channel.name || channel.channelName || channel.title)) ||
      channelId(channel) || 'Live-TV';
  }

  function clientApi() {
    const platform = global.VdrSuitePlatform || null;
    return platform && typeof platform.getClientApi === 'function'
      ? platform.getClientApi()
      : global.VdrSuiteClientApi;
  }

  function safePageNumber(value, fallback) {
    const number = Number(value);
    if (!Number.isInteger(number) || number < 100 || number > 899) {
      return fallback === undefined ? 100 : fallback;
    }
    return number;
  }

  function safeSubpageCode(value, fallback) {
    const number = Number(value);
    if (!Number.isInteger(number) || number < 0 || number > 65534) {
      return fallback === undefined ? 0 : fallback;
    }
    return number;
  }

  function installStyles() {
    if (!doc || !doc.head || typeof doc.createElement !== 'function') return;
    if (typeof doc.getElementById === 'function' &&
        doc.getElementById('vdr-suite-teletext-view-style')) return;

    const style = doc.createElement('style');
    style.id = 'vdr-suite-teletext-view-style';
    style.textContent = `
.vdr-suite-teletext-launch{min-height:2.5rem;padding:.5rem .8rem;border:1px solid rgba(34,211,238,.65);border-radius:.68rem;background:rgba(8,47,73,.72);color:#cffafe;font-weight:800;cursor:pointer}
.vdr-suite-teletext-launch[data-availability="available"]{border-color:rgba(74,222,128,.72);color:#dcfce7}
.vdr-suite-teletext-launch[data-availability="unavailable"]{border-color:rgba(148,163,184,.35);color:#94a3b8}
.vdr-suite-teletext-overlay{position:fixed;z-index:2147483000;inset:0;display:grid;grid-template-rows:auto 1fr;background:rgba(2,6,23,.96);color:#f8fafc}
@media(min-width:1100px){
  body.vdr-suite-teletext-open{overflow:hidden}
  body.vdr-suite-teletext-open::after{content:"";position:fixed;z-index:2147482400;top:0;right:0;bottom:0;width:32vw;background:linear-gradient(180deg,#020617 0%,#07111f 100%);pointer-events:none}
  body.vdr-suite-teletext-open .vdr-suite-teletext-overlay{right:32vw;background:rgba(2,6,23,.985);box-shadow:1.2rem 0 3.5rem rgba(0,0,0,.42)}
  body.vdr-suite-teletext-open .vdr-suite-live-tv-player{position:fixed;z-index:2147482500;top:0;right:0;width:32vw;height:auto;max-height:100vh;box-sizing:border-box;margin:0!important;padding:0!important;border:0!important;border-radius:0!important;background:transparent!important;overflow:hidden}
  body.vdr-suite-teletext-open .vdr-suite-live-tv-player-head{position:relative;z-index:2;background:rgba(2,6,23,.96);backdrop-filter:blur(10px);padding:.7rem .8rem .8rem;border-top:1px solid rgba(148,163,184,.18)}
  body.vdr-suite-teletext-open .vdr-suite-live-tv-player-slot{display:grid;place-items:start center;min-height:0;background:#000;border-radius:0!important}
  body.vdr-suite-teletext-open .vdr-suite-live-tv-player video{display:block;width:100%;height:auto;max-height:62vh;object-fit:contain;background:#000}
  body.vdr-suite-teletext-open .vdr-suite-teletext-stage{padding:.8rem 1rem 1rem}
  body.vdr-suite-teletext-open .vdr-suite-teletext-page{width:min(100%,70rem);font-size:clamp(.84rem,1.55vw,1.56rem)}
  body.vdr-suite-teletext-open .vdr-suite-teletext-toolbar{justify-content:flex-start;gap:.42rem;padding:.55rem .7rem}
  body.vdr-suite-teletext-open .vdr-suite-teletext-toolbar strong{margin-right:1rem}
}
@media(min-width:1500px){
  body.vdr-suite-teletext-open::after{width:30vw}
  body.vdr-suite-teletext-open .vdr-suite-teletext-overlay{right:30vw}
  body.vdr-suite-teletext-open .vdr-suite-live-tv-player{width:30vw}
  body.vdr-suite-teletext-open .vdr-suite-live-tv-player video{max-height:66vh}
  body.vdr-suite-teletext-open .vdr-suite-teletext-page{width:min(100%,78rem);font-size:clamp(.94rem,1.42vw,1.72rem)}
}
.vdr-suite-teletext-toolbar{display:flex;align-items:center;gap:.55rem;flex-wrap:wrap;padding:.7rem .85rem;border-bottom:1px solid rgba(148,163,184,.26);background:#020617}
.vdr-suite-teletext-toolbar strong{margin-right:auto}
.vdr-suite-teletext-toolbar button,.vdr-suite-teletext-toolbar input{min-height:2.45rem;border:1px solid rgba(148,163,184,.42);border-radius:.55rem;background:#0f172a;color:#f8fafc}
.vdr-suite-teletext-toolbar button{padding:.42rem .68rem;cursor:pointer}.vdr-suite-teletext-toolbar input{width:5.5rem;padding:.35rem .5rem}
.vdr-suite-teletext-toolbar button:focus-visible,.vdr-suite-teletext-toolbar input:focus-visible,.vdr-suite-teletext-launch:focus-visible{outline:2px solid #22d3ee;outline-offset:2px}
.vdr-suite-teletext-status{padding:.45rem .85rem;color:#cbd5e1}.vdr-suite-teletext-status.error{color:#fecaca}
.vdr-suite-teletext-stage{overflow:auto;display:grid;place-items:start center;padding:.75rem}
.vdr-suite-teletext-page{display:grid;grid-template-rows:repeat(25,1fr);width:min(100%,52rem);aspect-ratio:40/25;background:#000;box-shadow:0 1rem 3rem rgba(0,0,0,.46);font-family:"DejaVu Sans Mono","Liberation Mono",monospace;font-size:clamp(.62rem,1.55vw,1.18rem);line-height:1}
.vdr-suite-teletext-row{display:grid;grid-template-columns:repeat(40,minmax(0,1fr))}
.vdr-suite-teletext-cell{display:grid;place-items:center;min-width:0;overflow:hidden;white-space:pre}
.vdr-suite-teletext-empty{padding:1rem;color:#94a3b8}
@media(max-width:620px){.vdr-suite-teletext-toolbar{gap:.4rem}.vdr-suite-teletext-toolbar strong{flex-basis:100%;margin-right:0}.vdr-suite-teletext-stage{padding:.35rem}.vdr-suite-teletext-page{width:100%;font-size:clamp(.46rem,2.35vw,.88rem)}}
`;
    doc.head.appendChild(style);
  }

  function removeOverlay() {
    if (state.overlay && state.overlay.parentNode &&
        typeof state.overlay.parentNode.removeChild === 'function') {
      state.overlay.parentNode.removeChild(state.overlay);
    }
    state.overlay = null;
  }

  function setDesktopCompanionActive(active) {
    if (!doc || !doc.body || !doc.body.classList) return;
    if (active) doc.body.classList.add('vdr-suite-teletext-open');
    else doc.body.classList.remove('vdr-suite-teletext-open');
  }

  function close() {
    state.open = false;
    state.requestSequence += 1;
    state.loading = false;
    state.error = '';
    setDesktopCompanionActive(false);
    removeOverlay();
    return true;
  }

  function pageQuery() {
    return {
      backend: state.backendId,
      channel: state.channelId,
      page: String(state.pageNumber),
      subpage: state.automaticSubpage ? 'auto' : String(state.subpageCode)
    };
  }

  function serviceQuery(backendId, id) {
    return {
      backend: backendId,
      channel: id
    };
  }

  function discoverService(backendId, id) {
    const client = clientApi();
    if (!client || typeof client.fetchClientTeletextService !== 'function') {
      return Promise.reject(new Error('Teletext Client API ist nicht verfügbar.'));
    }
    return client.fetchClientTeletextService({
      query: serviceQuery(backendId, id),
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function readPage() {
    const client = clientApi();
    if (!client || typeof client.fetchClientTeletextPage !== 'function') {
      return Promise.reject(new Error('Teletext Client API ist nicht verfügbar.'));
    }

    const sequence = ++state.requestSequence;
    state.loading = true;
    state.error = '';
    render();

    return client.fetchClientTeletextPage({
      query: pageQuery(),
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function(data) {
      if (!state.open || sequence !== state.requestSequence) return null;
      state.loading = false;
      state.page = data || null;
      if (data && data.page && data.page.number) {
        state.pageNumber = safePageNumber(data.page.number, state.pageNumber);
        state.subpageCode = safeSubpageCode(data.page.subpage, state.subpageCode);
      }
      state.error = '';
      render();
      return data;
    }).catch(function(error) {
      if (!state.open || sequence !== state.requestSequence) return null;
      state.loading = false;
      state.page = null;
      state.error = error && error.message ? error.message : 'Teletext-Seite konnte nicht geladen werden.';
      render();
      return null;
    });
  }

  function open(channel, backendId) {
    const id = channelId(channel);
    const backend = text(backendId) || 'default';
    if (!id) return Promise.resolve(null);

    state.open = true;
    state.backendId = backend;
    state.channelId = id;
    state.channelName = channelName(channel);
    state.pageNumber = 100;
    state.automaticSubpage = true;
    state.subpageCode = 0;
    state.service = null;
    state.page = null;
    state.error = '';
    state.loading = true;
    setDesktopCompanionActive(true);
    const sequence = ++state.requestSequence;
    render();

    return discoverService(backend, id).then(function(data) {
      if (!state.open || sequence !== state.requestSequence) return null;
      state.service = data || null;
      const available = Boolean(data && data.service && data.service.available);
      if (!available) {
        state.loading = false;
        state.error = 'Für diesen Sender ist aktuell kein Teletext verfügbar.';
        render();
        return data;
      }
      return readPage();
    }).catch(function(error) {
      if (!state.open || sequence !== state.requestSequence) return null;
      state.loading = false;
      state.error = error && error.message ? error.message : 'Teletext ist derzeit nicht verfügbar.';
      render();
      return null;
    });
  }

  function navigatePage(delta) {
    state.pageNumber = safePageNumber(state.pageNumber + Number(delta || 0), state.pageNumber);
    state.automaticSubpage = true;
    state.subpageCode = 0;
    return readPage();
  }

  function navigateSubpage(delta) {
    const current = state.page && state.page.page
      ? safeSubpageCode(state.page.page.subpage, state.subpageCode)
      : state.subpageCode;
    state.automaticSubpage = false;
    state.subpageCode = safeSubpageCode(current + Number(delta || 0), current);
    return readPage();
  }

  function submitPage(value) {
    state.pageNumber = safePageNumber(value, state.pageNumber);
    state.automaticSubpage = true;
    state.subpageCode = 0;
    return readPage();
  }

  function cellCharacter(cell) {
    const codepoint = Array.isArray(cell) ? Number(cell[0]) : 32;
    if (!Number.isInteger(codepoint) || codepoint < 0 || codepoint > 0x10ffff) return ' ';
    try {
      const value = String.fromCodePoint(codepoint);
      return value === '\u0000' ? ' ' : value;
    } catch (error) {
      return ' ';
    }
  }

  function createCell(cell) {
    const value = doc.createElement('span');
    value.className = 'vdr-suite-teletext-cell';
    const foreground = Array.isArray(cell) ? Number(cell[3]) : 7;
    const background = Array.isArray(cell) ? Number(cell[4]) : 0;
    const kind = Array.isArray(cell) ? Number(cell[5]) : 0;
    const flags = Array.isArray(cell) ? Number(cell[6]) : 0;
    value.style.color = COLORS[foreground] || COLORS[7];
    value.style.backgroundColor = COLORS[background] || COLORS[0];
    value.dataset.kind = String(Number.isFinite(kind) ? kind : 0);
    value.dataset.flags = String(Number.isFinite(flags) ? flags : 0);
    value.textContent = cellCharacter(cell);
    return value;
  }

  function createPageGrid(data) {
    const cells = data && Array.isArray(data.cells) ? data.cells : [];
    const textRows = data && Array.isArray(data.text) ? data.text : [];
    const grid = doc.createElement('div');
    grid.className = 'vdr-suite-teletext-page';
    grid.setAttribute('role', 'img');
    grid.setAttribute(
      'aria-label',
      'Teletext Seite ' + String(state.pageNumber) + ' von ' + state.channelName
    );

    if (cells.length === 1000) {
      for (let rowIndex = 0; rowIndex < 25; rowIndex += 1) {
        const row = doc.createElement('div');
        row.className = 'vdr-suite-teletext-row';
        for (let column = 0; column < 40; column += 1) {
          row.appendChild(createCell(cells[(rowIndex * 40) + column]));
        }
        grid.appendChild(row);
      }
      return grid;
    }

    for (let rowIndex = 0; rowIndex < 25; rowIndex += 1) {
      const row = doc.createElement('div');
      row.className = 'vdr-suite-teletext-row';
      const source = String(textRows[rowIndex] || '').padEnd(40, ' ').slice(0, 40);
      for (let column = 0; column < 40; column += 1) {
        row.appendChild(createCell([source.codePointAt(column) || 32, 32, 0, 7, 0, 0, 0]));
      }
      grid.appendChild(row);
    }
    return grid;
  }

  function renderToolbar(overlay) {
    const toolbar = doc.createElement('div');
    toolbar.className = 'vdr-suite-teletext-toolbar';

    const title = doc.createElement('strong');
    title.textContent = 'Videotext · ' + state.channelName;
    toolbar.appendChild(title);

    const previous = doc.createElement('button');
    previous.type = 'button';
    previous.textContent = '− Seite';
    previous.setAttribute('aria-label', 'Vorherige Teletext-Seite');
    previous.addEventListener('click', function() { navigatePage(-1); });
    toolbar.appendChild(previous);

    const input = doc.createElement('input');
    input.type = 'number';
    input.min = '100';
    input.max = '899';
    input.value = String(state.pageNumber);
    input.setAttribute('aria-label', 'Teletext-Seitennummer');
    input.addEventListener('keydown', function(event) {
      if (event && event.key === 'Enter') submitPage(input.value);
    });
    toolbar.appendChild(input);

    const go = doc.createElement('button');
    go.type = 'button';
    go.textContent = 'Seite';
    go.addEventListener('click', function() { submitPage(input.value); });
    toolbar.appendChild(go);

    const next = doc.createElement('button');
    next.type = 'button';
    next.textContent = 'Seite +';
    next.setAttribute('aria-label', 'Nächste Teletext-Seite');
    next.addEventListener('click', function() { navigatePage(1); });
    toolbar.appendChild(next);

    const previousSubpage = doc.createElement('button');
    previousSubpage.type = 'button';
    previousSubpage.textContent = 'Unterseite −';
    previousSubpage.addEventListener('click', function() { navigateSubpage(-1); });
    toolbar.appendChild(previousSubpage);

    const nextSubpage = doc.createElement('button');
    nextSubpage.type = 'button';
    nextSubpage.textContent = 'Unterseite +';
    nextSubpage.addEventListener('click', function() { navigateSubpage(1); });
    toolbar.appendChild(nextSubpage);

    const automatic = doc.createElement('button');
    automatic.type = 'button';
    automatic.textContent = 'Unterseite Auto';
    automatic.addEventListener('click', function() {
      state.automaticSubpage = true;
      state.subpageCode = 0;
      readPage();
    });
    toolbar.appendChild(automatic);

    const closeButton = doc.createElement('button');
    closeButton.type = 'button';
    closeButton.textContent = 'Schließen';
    closeButton.setAttribute('aria-label', 'Videotext schließen');
    closeButton.addEventListener('click', close);
    toolbar.appendChild(closeButton);

    overlay.appendChild(toolbar);
  }

  function render() {
    if (!state.open || !doc || !doc.body || typeof doc.createElement !== 'function') {
      setDesktopCompanionActive(false);
      removeOverlay();
      return;
    }

    installStyles();
    removeOverlay();

    const overlay = doc.createElement('section');
    overlay.className = 'vdr-suite-teletext-overlay';
    overlay.setAttribute('role', 'dialog');
    overlay.setAttribute('aria-modal', 'true');
    overlay.setAttribute('aria-label', 'Videotext');
    renderToolbar(overlay);

    const body = doc.createElement('div');
    body.className = 'vdr-suite-teletext-stage';

    if (state.loading) {
      const loading = doc.createElement('div');
      loading.className = 'vdr-suite-teletext-status';
      loading.textContent = 'Videotext wird geladen …';
      body.appendChild(loading);
    } else if (state.error) {
      const error = doc.createElement('div');
      error.className = 'vdr-suite-teletext-status error';
      error.textContent = state.error;
      body.appendChild(error);
    } else if (state.page && state.page.pageAvailable === true) {
      body.appendChild(createPageGrid(state.page));
    } else {
      const empty = doc.createElement('div');
      empty.className = 'vdr-suite-teletext-empty';
      empty.textContent = 'Diese Teletext-Seite ist derzeit nicht verfügbar.';
      body.appendChild(empty);
    }

    overlay.appendChild(body);
    doc.body.appendChild(overlay);
    state.overlay = overlay;
  }

  function createLauncher(channel, backendId) {
    if (!doc || typeof doc.createElement !== 'function') return null;
    const id = channelId(channel);
    const backend = text(backendId) || 'default';
    const launcher = doc.createElement('button');
    launcher.type = 'button';
    launcher.className = 'vdr-suite-teletext-launch';
    launcher.textContent = 'Videotext …';
    launcher.dataset.availability = 'checking';
    launcher.disabled = !id;
    launcher.setAttribute('aria-label', 'Videotext für ' + channelName(channel));

    if (!id) return launcher;

    discoverService(backend, id).then(function(data) {
      const available = Boolean(data && data.service && data.service.available);
      launcher.dataset.availability = available ? 'available' : 'unavailable';
      launcher.textContent = available ? 'Videotext' : 'Kein Videotext';
      launcher.disabled = !available;
    }).catch(function() {
      launcher.dataset.availability = 'unavailable';
      launcher.textContent = 'Videotext nicht verfügbar';
      launcher.disabled = true;
    });

    launcher.addEventListener('click', function() {
      open(channel, backend);
    });

    return launcher;
  }

  if (doc && typeof doc.addEventListener === 'function') {
    doc.addEventListener('keydown', function(event) {
      if (!state.open || !event) return;
      if (event.key === 'Escape') {
        if (typeof event.preventDefault === 'function') event.preventDefault();
        close();
        return;
      }
      if (/^[0-9]$/.test(event.key) && !event.ctrlKey && !event.altKey && !event.metaKey) {
        const current = String(state.pageNumber).padStart(3, '0');
        const next = (current.slice(1) + event.key).slice(-3);
        const number = safePageNumber(Number(next), state.pageNumber);
        state.pageNumber = number;
      }
    });
  }

  const api = Object.freeze({
    createLauncher,
    discoverService,
    open,
    close,
    readPage,
    navigatePage,
    navigateSubpage,
    submitPage,
    snapshot: function() {
      return Object.freeze({
        open: state.open,
        backendId: state.backendId,
        channelId: state.channelId,
        pageNumber: state.pageNumber,
        automaticSubpage: state.automaticSubpage,
        subpageCode: state.subpageCode,
        loading: state.loading,
        error: state.error
      });
    },
    __test: Object.freeze({
      safePageNumber,
      safeSubpageCode,
      cellCharacter,
      serviceQuery,
      pageQuery,
      COLORS,
      setDesktopCompanionActive
    })
  });

  global.VdrSuiteTeletextView = api;
  installStyles();
})(window);
