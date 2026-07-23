(function (global) {
  'use strict';

  const platform = global.VdrSuitePlatform;
  if (!platform || typeof platform.registerModule !== 'function') {
    console.error('VDR-Suite platform is unavailable for the genres module');
    return;
  }

  const PAGE_SIZE = 48;
  const state = {
    active: false,
    backendId: '',
    scope: 'recordings',
    overview: null,
    selectedGenre: null,
    items: [],
    total: 0,
    offset: 0,
    hasMore: false,
    channels: Object.create(null),
    loading: false,
    loadingMore: false,
    error: null,
    requestSequence: 0
  };

  function clientApi() {
    return platform.getClientApi ? platform.getClientApi() : global.VdrSuiteClientApi;
  }

  function mountTarget() {
    return (platform.getMountTarget && (
      platform.getMountTarget('genres') || platform.getMountTarget('detail')
    )) || document.getElementById('detail-data');
  }

  function selectedBackendId() {
    return String(
      (platform.getSelectedBackendId && platform.getSelectedBackendId()) ||
      state.backendId ||
      'default'
    );
  }

  function text(value, fallback) {
    const normalized = value === undefined || value === null ? '' : String(value).trim();
    return normalized || String(fallback || '');
  }

  function number(value, fallback) {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : Number(fallback || 0);
  }

  function node(tag, className, value) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (value !== undefined) element.textContent = text(value);
    return element;
  }

  function button(label, className, handler) {
    const element = document.createElement('button');
    element.type = 'button';
    element.className = className || '';
    element.textContent = label;
    if (typeof handler === 'function') element.addEventListener('click', handler);
    return element;
  }

  function installStyles() {
    if (document.getElementById('vdr-suite-genres-style')) return;
    const style = document.createElement('style');
    style.id = 'vdr-suite-genres-style';
    style.textContent = `
      .genres-browser{display:grid;gap:1.15rem;grid-column:1/-1;width:100%}
      .genres-header{display:flex;align-items:flex-start;justify-content:space-between;gap:1rem;flex-wrap:wrap}
      .genres-header h3{margin:0;color:#f8fafc;font-size:clamp(1.35rem,2.5vw,2rem)}
      .genres-header p{margin:.3rem 0 0;color:#94a3b8}
      .genres-scope{display:flex;gap:.55rem;padding:.3rem;border:1px solid rgba(148,163,184,.22);border-radius:.9rem;background:rgba(15,23,42,.72)}
      .genres-scope button,.genres-back,.genres-more,.genres-detail-back{border:1px solid rgba(96,165,250,.32);border-radius:.72rem;background:rgba(15,23,42,.88);color:#dbeafe;padding:.65rem .9rem;font-weight:750;cursor:pointer}
      .genres-scope button.active,.genres-back,.genres-more{background:linear-gradient(135deg,#0369a1,#2563eb);color:white}
      .genres-summary{display:flex;gap:.65rem;flex-wrap:wrap;color:#cbd5e1}
      .genres-summary span{padding:.45rem .7rem;border-radius:999px;background:rgba(15,23,42,.76);border:1px solid rgba(148,163,184,.2)}
      .genres-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(15.5rem,1fr));gap:1rem}
      .genre-card{position:relative;min-height:13rem;padding:0;overflow:hidden;border:1px solid rgba(96,165,250,.32);border-radius:1.15rem;background:#0f172a;color:white;text-align:left;cursor:pointer;box-shadow:0 1rem 2rem rgba(2,6,23,.22)}
      .genre-card:hover,.genre-card:focus-visible{transform:translateY(-2px);border-color:rgba(56,189,248,.76);outline:none}
      .genre-card img{position:absolute;inset:0;width:100%;height:100%;object-fit:cover;opacity:.72}
      .genre-card::after{content:"";position:absolute;inset:0;background:linear-gradient(180deg,rgba(2,6,23,.04),rgba(2,6,23,.92))}
      .genre-card-copy{position:absolute;z-index:1;left:1rem;right:1rem;bottom:1rem;display:grid;gap:.22rem}
      .genre-card-title{font-size:1.25rem;font-weight:900;text-shadow:0 .15rem .5rem #020617}
      .genre-card-count{color:#bae6fd;font-weight:800}
      .genre-card-state{color:#cbd5e1;font-size:.8rem}
      .genres-result-header{display:flex;align-items:center;justify-content:space-between;gap:.8rem;flex-wrap:wrap}
      .genres-result-header h4{margin:0;color:#f8fafc;font-size:1.3rem}
      .genres-status{padding:1rem;border:1px solid rgba(148,163,184,.25);border-radius:1rem;background:rgba(15,23,42,.7);color:#cbd5e1}
      .genres-status.error{border-color:rgba(248,113,113,.55);color:#fecaca}
      .genres-recordings{display:grid;grid-template-columns:repeat(auto-fill,minmax(17rem,1fr));gap:1rem}
      .genres-epg-list{display:grid;grid-template-columns:repeat(auto-fill,minmax(19rem,1fr));gap:1rem}
      .genres-epg-card{overflow:hidden;border:1px solid rgba(96,165,250,.28);border-radius:1rem;background:rgba(15,23,42,.82);color:#e2e8f0;text-align:left;padding:0;cursor:pointer}
      .genres-epg-card img{display:block;width:100%;aspect-ratio:16/9;object-fit:cover;background:#020617}
      .genres-epg-copy{display:grid;gap:.35rem;padding:.9rem}
      .genres-epg-title{font-size:1.03rem;font-weight:850;color:#f8fafc}
      .genres-epg-meta{font-size:.82rem;color:#93c5fd}
      .genres-epg-description{font-size:.86rem;color:#cbd5e1;display:-webkit-box;-webkit-line-clamp:3;-webkit-box-orient:vertical;overflow:hidden}
      .genres-owned-detail{display:grid;gap:.8rem;grid-column:1/-1}
      @media(min-width:72rem){.genres-grid{grid-template-columns:repeat(4,minmax(0,1fr))}.genre-card{min-height:17rem}}
      @media(max-width:720px){.genres-grid{grid-template-columns:repeat(2,minmax(0,1fr));gap:.65rem}.genre-card{min-height:10.5rem}.genre-card-copy{left:.7rem;right:.7rem;bottom:.7rem}.genre-card-title{font-size:1rem}.genres-recordings,.genres-epg-list{grid-template-columns:1fr}}
    `;
    document.head.appendChild(style);
  }

  function genreArtworkUrl(genreId) {
    const names = {
      action: 'action',
      documentary: 'doku',
      drama: 'drama',
      music: 'musik',
      musical: 'musical',
      mystery: 'mystery',
      'science-fiction': 'scifi',
      series: 'serien',
      thriller: 'thriller',
      western: 'western'
    };
    const asset = names[genreId];
    return asset
      ? '/channel-logos/vdr-suite-brand/recording-genre-' + asset + '.svg'
      : '/channel-logos/vdr-suite-brand/recording-genre-sprite.svg';
  }

  function channelList(data) {
    if (Array.isArray(data)) return data;
    if (data && Array.isArray(data.channels)) return data.channels;
    if (data && Array.isArray(data.items)) return data.items;
    return [];
  }

  function indexChannels(data) {
    state.channels = Object.create(null);
    channelList(data).forEach(function (channel) {
      const id = text(channel.id || channel.channelId || channel.nativeId);
      if (id) state.channels[id] = channel;
    });
  }

  function channelFor(event) {
    const id = text(event && event.channelId);
    return state.channels[id] || {id: id, channelId: id, name: id || 'Unbekannter Kanal'};
  }

  function channelTitle(channel) {
    return text(channel && (channel.name || channel.title || channel.channelName || channel.id), 'Unbekannter Kanal');
  }

  function formatTime(epoch) {
    const value = number(epoch, 0);
    if (value <= 0) return '–';
    return new Intl.DateTimeFormat('de-DE', {
      weekday: 'short', day: '2-digit', month: '2-digit',
      hour: '2-digit', minute: '2-digit'
    }).format(new Date(value * 1000));
  }

  function overviewGenres() {
    return state.overview && Array.isArray(state.overview.genres)
      ? state.overview.genres
      : [];
  }

  function renderStatus(root) {
    if (state.loading) {
      root.appendChild(node('section', 'genres-status', 'Genres werden aus dem persistenten Index geladen …'));
      return true;
    }
    if (state.error) {
      root.appendChild(node('section', 'genres-status error', state.error.message || state.error));
      return true;
    }
    return false;
  }

  function createHeader(root) {
    const header = document.createElement('header');
    header.className = 'genres-header';
    const copy = document.createElement('div');
    copy.appendChild(node('h3', '', 'Genres'));
    copy.appendChild(node('p', '', 'Persistente Metadatenansicht für Aufnahmen und EPG.'));
    header.appendChild(copy);

    const scope = document.createElement('div');
    scope.className = 'genres-scope';
    ['recordings', 'epg'].forEach(function (name) {
      const switcher = button(name === 'recordings' ? 'Aufnahmen' : 'EPG', '', function () {
        if (state.scope === name) return;
        state.scope = name;
        state.selectedGenre = null;
        state.items = [];
        loadOverview();
      });
      switcher.classList.toggle('active', state.scope === name);
      scope.appendChild(switcher);
    });
    header.appendChild(scope);
    root.appendChild(header);
  }

  function createOverview(root) {
    const summary = document.createElement('div');
    summary.className = 'genres-summary';
    summary.appendChild(node('span', '', String(number(state.overview && state.overview.totalItems, 0)) + ' Einträge'));
    summary.appendChild(node('span', '', String(overviewGenres().length) + ' Genres'));
    summary.appendChild(node('span', '', 'Backend ' + state.backendId));
    root.appendChild(summary);

    const grid = document.createElement('section');
    grid.className = 'genres-grid';
    overviewGenres().forEach(function (genre) {
      const card = button('', 'genre-card', function () { selectGenre(genre); });
      const image = document.createElement('img');
      image.src = genreArtworkUrl(text(genre.id));
      image.alt = '';
      image.loading = 'lazy';
      card.appendChild(image);
      const copy = document.createElement('span');
      copy.className = 'genre-card-copy';
      copy.appendChild(node('span', 'genre-card-title', genre.label || genre.labelDe || genre.id));
      copy.appendChild(node('span', 'genre-card-count', String(number(genre.count, 0)) + ' Treffer'));
      const states = [];
      if (number(genre.staleCount, 0) > 0) states.push(String(genre.staleCount) + ' veraltet');
      if (number(genre.conflictCount, 0) > 0) states.push(String(genre.conflictCount) + ' Konflikte');
      if (!genre.known) states.push('nicht kanonisch');
      if (states.length) copy.appendChild(node('span', 'genre-card-state', states.join(' · ')));
      card.appendChild(copy);
      grid.appendChild(card);
    });
    root.appendChild(grid);
  }

  function createResultHeader(root) {
    const header = document.createElement('div');
    header.className = 'genres-result-header';
    header.appendChild(button('← Alle Genres', 'genres-back', function () {
      state.selectedGenre = null;
      state.items = [];
      state.offset = 0;
      state.total = 0;
      state.hasMore = false;
      render();
    }));
    header.appendChild(node(
      'h4',
      '',
      text(state.selectedGenre && (state.selectedGenre.label || state.selectedGenre.labelDe || state.selectedGenre.id)) +
        ' · ' + String(state.total) + ' Treffer'
    ));
    root.appendChild(header);
  }

  function openRecording(recording) {
    const owner = global.VdrSuiteRecordings2;
    if (!owner || typeof owner.openRecording !== 'function') {
      state.error = new Error('Recordings 2 Detail-Owner ist nicht verfügbar.');
      render();
      return;
    }
    owner.openRecording(recording, {
      backendId: state.backendId,
      backLabel: '← Zurück zu ' + text(state.selectedGenre && state.selectedGenre.label, 'Genre'),
      onClose: function () {
        state.active = true;
        render();
      }
    });
  }

  function createRecordingResults(root) {
    const owner = global.VdrSuiteRecordings2BrowserView;
    const list = document.createElement('section');
    list.className = 'genres-recordings recordings2-recording-list';
    state.items.forEach(function (recording) {
      if (owner && typeof owner.createRecordingCard === 'function') {
        list.appendChild(owner.createRecordingCard(recording, openRecording));
      }
    });
    root.appendChild(list);
  }

  function openEpg(event) {
    const owner = global.VdrSuiteEpgDetailOwner;
    if (!owner || typeof owner.open !== 'function') {
      state.error = new Error('EPG Detail-Owner ist nicht verfügbar.');
      render();
      return;
    }
    owner.open(event, channelFor(event), {
      backLabel: '← Zurück zu ' + text(state.selectedGenre && state.selectedGenre.label, 'Genre'),
      onClose: function () {
        state.active = true;
        render();
      }
    });
  }

  function createEpgResults(root) {
    const list = document.createElement('section');
    list.className = 'genres-epg-list';
    state.items.forEach(function (event) {
      const card = button('', 'genres-epg-card', function () { openEpg(event); });
      if (event.artwork && event.artwork.available && event.artwork.url) {
        const image = document.createElement('img');
        image.src = event.artwork.url;
        image.alt = '';
        image.loading = 'lazy';
        card.appendChild(image);
      }
      const copy = document.createElement('span');
      copy.className = 'genres-epg-copy';
      copy.appendChild(node('span', 'genres-epg-title', event.title || 'Ohne Titel'));
      copy.appendChild(node(
        'span',
        'genres-epg-meta',
        channelTitle(channelFor(event)) + ' · ' + formatTime(event.startTime)
      ));
      if (event.subtitle) copy.appendChild(node('span', 'genres-epg-meta', event.subtitle));
      if (event.description) copy.appendChild(node('span', 'genres-epg-description', event.description));
      card.appendChild(copy);
      list.appendChild(card);
    });
    root.appendChild(list);
  }

  function createResults(root) {
    createResultHeader(root);
    if (state.items.length === 0) {
      root.appendChild(node('section', 'genres-status', 'Für dieses Genre liegen keine Treffer im aktuellen Fenster vor.'));
      return;
    }
    if (state.scope === 'recordings') createRecordingResults(root);
    else createEpgResults(root);

    if (state.hasMore) {
      const more = button(
        state.loadingMore ? 'Weitere Treffer werden geladen …' : 'Weitere Treffer laden',
        'genres-more',
        loadMore
      );
      more.disabled = state.loadingMore;
      root.appendChild(more);
    }
  }

  function render() {
    if (!state.active) return;
    const target = mountTarget();
    if (!target) return;
    installStyles();
    target.classList.remove('recordings2-mount');
    target.replaceChildren();
    const root = document.createElement('section');
    root.className = 'genres-browser';
    createHeader(root);
    if (!renderStatus(root)) {
      if (state.selectedGenre) createResults(root);
      else createOverview(root);
    }
    target.appendChild(root);
  }

  function loadChannels() {
    const api = clientApi();
    if (!api || typeof api.fetchClientChannels !== 'function') return Promise.resolve();
    return api.fetchClientChannels({
      backendId: state.backendId,
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(indexChannels).catch(function () { state.channels = Object.create(null); });
  }

  function loadOverview() {
    const api = clientApi();
    if (!api || typeof api.fetchClientGenres !== 'function') {
      state.error = new Error('Genre Client API ist nicht verfügbar.');
      render();
      return;
    }
    state.loading = true;
    state.error = null;
    const sequence = ++state.requestSequence;
    render();
    const now = Math.floor(Date.now() / 1000);
    const options = {
      backendId: state.backendId,
      scope: state.scope,
      locale: 'de',
      cache: 'no-store',
      credentials: 'same-origin'
    };
    if (state.scope === 'epg') {
      options.from = now;
      options.until = now + (48 * 60 * 60);
    }
    Promise.all([
      api.fetchClientGenres(options),
      state.scope === 'epg' ? loadChannels() : Promise.resolve()
    ]).then(function (results) {
      if (!state.active || sequence !== state.requestSequence) return;
      state.overview = results[0] || {genres: []};
      state.loading = false;
      render();
    }).catch(function (error) {
      if (!state.active || sequence !== state.requestSequence) return;
      state.loading = false;
      state.error = error;
      render();
    });
  }

  function requestItems(offset) {
    const api = clientApi();
    const common = {
      backendId: state.backendId,
      genreId: state.selectedGenre.id,
      limit: PAGE_SIZE,
      offset: offset,
      cache: 'no-store',
      credentials: 'same-origin'
    };
    if (state.scope === 'recordings') {
      return api.fetchClientGenreRecordings(common);
    }
    const now = Math.floor(Date.now() / 1000);
    return api.fetchClientGenreEpg(Object.assign({}, common, {
      from: now,
      until: now + (48 * 60 * 60)
    }));
  }

  function applyItems(data, append) {
    const incoming = data && Array.isArray(data.items) ? data.items : [];
    state.items = append ? state.items.concat(incoming) : incoming;
    state.offset = number(data && data.offset, append ? state.offset : 0);
    state.total = number(data && data.total, state.items.length);
    state.hasMore = Boolean(data && data.hasMore);
  }

  function selectGenre(genre) {
    state.selectedGenre = genre;
    state.items = [];
    state.total = number(genre && genre.count, 0);
    state.offset = 0;
    state.hasMore = false;
    state.loading = true;
    state.error = null;
    const sequence = ++state.requestSequence;
    render();
    requestItems(0).then(function (data) {
      if (!state.active || sequence !== state.requestSequence) return;
      applyItems(data, false);
      state.loading = false;
      render();
    }).catch(function (error) {
      if (!state.active || sequence !== state.requestSequence) return;
      state.loading = false;
      state.error = error;
      render();
    });
  }

  function loadMore() {
    if (state.loadingMore || !state.selectedGenre) return;
    state.loadingMore = true;
    const sequence = ++state.requestSequence;
    render();
    requestItems(state.items.length).then(function (data) {
      if (!state.active || sequence !== state.requestSequence) return;
      applyItems(data, true);
      state.loadingMore = false;
      render();
    }).catch(function (error) {
      if (!state.active || sequence !== state.requestSequence) return;
      state.loadingMore = false;
      state.error = error;
      render();
    });
  }

  const moduleApi = Object.freeze({
    activate: function () {
      const backendId = selectedBackendId();
      const backendChanged = state.backendId !== backendId;
      state.active = true;
      if (backendChanged) {
        state.backendId = backendId;
        state.selectedGenre = null;
        state.items = [];
        state.overview = null;
      }
      if (!state.overview || backendChanged) loadOverview();
      else render();
    },
    deactivate: function () {
      state.active = false;
      state.requestSequence += 1;
    },
    refresh: function () {
      state.active = true;
      if (state.selectedGenre) selectGenre(state.selectedGenre);
      else loadOverview();
    },
    __test: Object.freeze({
      genreArtworkUrl: genreArtworkUrl,
      applyItems: applyItems,
      channelList: channelList
    })
  });

  function installShellEntry() {
    const tab = document.querySelector('[data-module="genres"]');
    if (!tab) return;
    tab.addEventListener('click', function () {
      document.querySelectorAll('.module-tab').forEach(function (entry) {
        entry.classList.toggle('active', entry === tab);
      });
      global.setTimeout(moduleApi.activate, 0);
    });
    document.querySelectorAll('.module-tab').forEach(function (entry) {
      if (entry !== tab) entry.addEventListener('click', moduleApi.deactivate);
    });
    const refresh = document.getElementById('refresh-detail');
    if (refresh) {
      refresh.addEventListener('click', function (event) {
        if (!tab.classList.contains('active')) return;
        event.preventDefault();
        event.stopImmediatePropagation();
        moduleApi.refresh();
      }, true);
    }
  }

  if (!platform.hasModule || !platform.hasModule('genres')) {
    platform.registerModule('genres', moduleApi);
  }
  global.VdrSuiteGenres = moduleApi;
  installShellEntry();
}(window));
