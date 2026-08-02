(function (global) {
  'use strict';

  const platform = global.VdrSuitePlatform;
  if (!platform || typeof platform.registerModule !== 'function') {
    console.error('VDR-Suite platform is unavailable for the genres module');
    return;
  }

  const PAGE_SIZE = 48;
  const state = {
    active:false,
    backendId:'',
    scope:'recordings',
    overview:null,
    view:'overview',
    selectedCategory:null,
    selectedGenre:null,
    items:[],
    total:0,
    hasMore:false,
    loading:false,
    loadingMore:false,
    error:null,
    requestSequence:0
  };

  const text = (value, fallback) => String(
    value === undefined || value === null ? (fallback || '') : value
  ).trim();
  const number = (value, fallback) => Number.isFinite(Number(value))
    ? Number(value)
    : Number(fallback || 0);
  const node = (tag, className, value) => {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (value !== undefined) element.textContent = text(value);
    return element;
  };
  const button = (label, className, handler) => {
    const element = node('button', className, label);
    element.type = 'button';
    if (handler) element.addEventListener('click', handler);
    return element;
  };

  function api() {
    return platform.getClientApi
      ? platform.getClientApi()
      : global.VdrSuiteClientApi;
  }

  function target() {
    return (platform.getMountTarget && (
      platform.getMountTarget('genres') ||
      platform.getMountTarget('detail')
    )) || document.getElementById('detail-data');
  }

  function backendId() {
    return text(
      platform.getSelectedBackendId && platform.getSelectedBackendId(),
      state.backendId || 'default'
    );
  }

  function installStyles() {
    const recordingsShared = global.VdrSuiteRecordings2Shared;
    if (recordingsShared &&
        typeof recordingsShared.installStyles === 'function') {
      recordingsShared.installStyles();
    }
    if (document.getElementById('vdr-suite-genres-style')) return;
    const style = node('style');
    style.id = 'vdr-suite-genres-style';
    style.textContent = `
      .genres-browser{display:grid;gap:1.15rem;grid-column:1/-1;width:100%}
      .genres-header,.genres-result-header{display:flex;align-items:flex-start;justify-content:space-between;gap:1rem;flex-wrap:wrap}
      .genres-header h3,.genres-result-header h4{margin:0;color:#f8fafc;font-size:clamp(1.3rem,2.5vw,2rem)}
      .genres-header p{margin:.3rem 0 0;color:#94a3b8}
      .genres-scope{display:flex;gap:.5rem;padding:.3rem;border:1px solid #334155;border-radius:.9rem;background:#0f172a}
      .genres-scope button,.genres-back,.genres-more,.genres-detail-back{border:1px solid #475569;border-radius:.7rem;background:#172033;color:#e2e8f0;padding:.65rem .9rem;font-weight:750;cursor:pointer}
      .genres-scope button.active,.genres-back,.genres-more{background:linear-gradient(135deg,#0369a1,#2563eb);color:#fff}
      .genres-summary{display:flex;gap:.65rem;flex-wrap:wrap}
      .genres-summary span{padding:.45rem .7rem;border:1px solid #334155;border-radius:999px;background:#0f172a;color:#cbd5e1}
      .genres-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(15.5rem,1fr));gap:1rem}
      .genre-card{position:relative;min-height:13rem;padding:0;overflow:hidden;border:1px solid rgba(96,165,250,.35);border-radius:1.15rem;background:#0f172a;color:#fff;text-align:left;cursor:pointer;box-shadow:0 1rem 2rem rgba(2,6,23,.24)}
      .genre-card:hover,.genre-card:focus-visible{transform:translateY(-2px);border-color:#38bdf8;outline:none}
      .genre-card img{position:absolute;inset:0;width:100%;height:100%;object-fit:cover;opacity:.72}
      .genre-card:after{content:"";position:absolute;inset:0;background:linear-gradient(180deg,transparent,rgba(2,6,23,.94))}
      .genre-card-copy{position:absolute;z-index:1;left:1rem;right:1rem;bottom:1rem;display:grid;gap:.22rem}
      .genre-card-title{font-size:1.25rem;font-weight:900}
      .genre-card-count{color:#bae6fd;font-weight:800}
      .genre-card-state{color:#cbd5e1;font-size:.8rem}
      .genres-status{padding:1rem;border:1px solid #334155;border-radius:1rem;background:#0f172a;color:#cbd5e1}
      .genres-status.error{border-color:#ef4444;color:#fecaca}
      .genres-recordings{display:grid;grid-template-columns:repeat(auto-fill,minmax(17rem,1fr));gap:1rem}
      .genres-epg-list{display:grid;grid-template-columns:repeat(auto-fill,minmax(19rem,1fr));gap:1rem}
      .genres-epg-card{overflow:hidden;border:1px solid #334155;border-radius:1rem;background:#0f172a;color:#e2e8f0;text-align:left;padding:0;cursor:pointer}
      .genres-epg-card img{display:block;width:100%;aspect-ratio:16/9;object-fit:cover;background:#020617}
      .genres-epg-copy{display:grid;gap:.35rem;padding:.9rem}
      .genres-epg-title{font-size:1.03rem;font-weight:850;color:#f8fafc}
      .genres-epg-meta{font-size:.82rem;color:#93c5fd}
      .genres-epg-description{font-size:.86rem;color:#cbd5e1;display:-webkit-box;-webkit-line-clamp:3;-webkit-box-orient:vertical;overflow:hidden}
      .genres-owned-detail{display:grid;gap:.8rem;grid-column:1/-1}
      @media(min-width:72rem){.genres-grid{grid-template-columns:repeat(4,minmax(0,1fr))}.genre-card{min-height:17rem}}
      @media(max-width:720px){.genres-grid{grid-template-columns:repeat(2,minmax(0,1fr));gap:.65rem}.genre-card{min-height:10.5rem}.genre-card-title{font-size:1rem}.genres-recordings,.genres-epg-list{grid-template-columns:1fr}}
    `;
    document.head.appendChild(style);
  }

  function genreArtworkUrl(id) {
    const assets = {
      action:'action',
      documentary:'doku',
      drama:'drama',
      music:'musik',
      musical:'musical',
      mystery:'mystery',
      'science-fiction':'scifi',
      series:'serien',
      thriller:'thriller',
      western:'western'
    };
    return assets[id]
      ? '/channel-logos/vdr-suite-brand/recording-genre-' +
          assets[id] + '.svg'
      : '/channel-logos/vdr-suite-brand/recording-genre-sprite.svg';
  }

  function channelFor(event) {
    const id = text(event && event.channelId);
    return {
      id:id,
      channelId:id,
      name:text(
        event && (event.channelName || event.channelTitle || event.channel),
        id || 'Unbekannter Kanal'
      )
    };
  }

  function formatTime(epoch) {
    const value = number(epoch, 0);
    if (value <= 0) return '–';
    return new Intl.DateTimeFormat('de-DE', {
      weekday:'short',
      day:'2-digit',
      month:'2-digit',
      hour:'2-digit',
      minute:'2-digit'
    }).format(new Date(value * 1000));
  }

  function recordingGenres() {
    return state.overview && Array.isArray(state.overview.genres)
      ? state.overview.genres
      : [];
  }

  function epgCategories() {
    if (!state.overview) return [];
    if (Array.isArray(state.overview.categories)) {
      return state.overview.categories;
    }
    return Array.isArray(state.overview.genres)
      ? state.overview.genres
      : [];
  }

  function epgWindow() {
    const from = number(state.overview && state.overview.from, 0);
    const until = number(state.overview && state.overview.until, 0);
    if (from > 0 && until > from) {
      return {from:from, until:until};
    }
    const now = Math.floor(Date.now() / 1000);
    return {from:now, until:now + 172800};
  }

  function synchronizeSelectedCount(total) {
    if (state.scope !== 'epg' || !state.overview) return;
    const normalizedTotal = number(total, 0);
    if (state.selectedGenre) {
      state.selectedGenre.count = normalizedTotal;
      return;
    }
    if (!state.selectedCategory) return;
    state.selectedCategory.count = normalizedTotal;
    state.overview.totalItems = epgCategories().reduce(
      (sum, entry) => sum + number(entry && entry.count, 0),
      0
    );
  }

  function overviewEntries() {
    return state.scope === 'epg'
      ? epgCategories()
      : recordingGenres();
  }

  function resetNavigation() {
    state.view = 'overview';
    state.selectedCategory = null;
    state.selectedGenre = null;
    state.items = [];
    state.total = 0;
    state.hasMore = false;
    state.loadingMore = false;
  }

  function renderHeader(root) {
    const header = node('header', 'genres-header');
    const copy = node('div');
    copy.append(
      node('h3', '', 'Genres'),
      node('p', '', state.scope === 'epg'
        ? 'EPG nach Sendungsart und Filmgenre.'
        : 'Persistente Metadatenansicht für Aufnahmen.')
    );
    const scope = node('div', 'genres-scope');
    [['recordings','Aufnahmen'],['epg','EPG']].forEach(entry => {
      const item = button(
        entry[1],
        state.scope === entry[0] ? 'active' : '',
        () => {
          if (state.scope === entry[0]) return;
          state.scope = entry[0];
          resetNavigation();
          loadOverview();
        }
      );
      scope.appendChild(item);
    });
    header.append(copy, scope);
    root.appendChild(header);
  }

  function appendGenreCard(grid, entry, handler) {
    const card = button('', 'genre-card', handler);
    const image = node('img');
    image.src = genreArtworkUrl(text(entry.id));
    image.alt = '';
    image.loading = 'lazy';
    const copy = node('span', 'genre-card-copy');
    copy.append(
      node('span', 'genre-card-title', entry.label || entry.labelDe || entry.id),
      node('span', 'genre-card-count', number(entry.count, 0) + ' Treffer')
    );
    if (state.scope === 'recordings') {
      const conditions = [];
      if (number(entry.staleCount, 0)) {
        conditions.push(entry.staleCount + ' veraltet');
      }
      if (number(entry.conflictCount, 0)) {
        conditions.push(entry.conflictCount + ' Konflikte');
      }
      if (!entry.known) conditions.push('nicht kanonisch');
      if (conditions.length) {
        copy.appendChild(
          node('span', 'genre-card-state', conditions.join(' · '))
        );
      }
    }
    card.append(image, copy);
    grid.appendChild(card);
  }

  function renderOverview(root) {
    const entries = overviewEntries();
    const summary = node('div', 'genres-summary');
    summary.append(
      node('span', '', number(state.overview && state.overview.totalItems, 0) + ' Einträge'),
      node('span', '', state.scope === 'epg'
        ? entries.length + ' Hauptkategorien'
        : entries.length + ' Genres'),
      node('span', '', 'Backend ' + state.backendId)
    );
    root.appendChild(summary);

    const grid = node('section', 'genres-grid');
    entries.forEach(entry => {
      appendGenreCard(grid, entry, () => selectOverviewEntry(entry));
    });
    root.appendChild(grid);
  }

  function renderMovieGenres(root) {
    const category = state.selectedCategory;
    const header = node('div', 'genres-result-header');
    header.append(
      button('← EPG-Hauptkategorien', 'genres-back', () => {
        resetNavigation();
        render();
      }),
      node('h4', '', 'Filmgenres')
    );
    root.appendChild(header);

    const children = category && Array.isArray(category.children)
      ? category.children.filter(child => number(child.count, 0) > 0)
      : [];
    const grid = node('section', 'genres-grid');
    appendGenreCard(grid, {
      id:'movie',
      label:'Alle Filme',
      count:number(category && category.count, 0)
    }, () => selectEpgResult(category, null));
    children.forEach(child => {
      appendGenreCard(grid, child, () => selectEpgResult(category, child));
    });
    root.appendChild(grid);
  }

  function resultLabel() {
    if (state.scope === 'recordings') {
      return text(
        state.selectedGenre &&
          (state.selectedGenre.label || state.selectedGenre.id),
        'Genre'
      );
    }
    return text(
      (state.selectedGenre &&
        (state.selectedGenre.label || state.selectedGenre.id)) ||
      (state.selectedCategory &&
        (state.selectedCategory.label || state.selectedCategory.id)),
      'EPG'
    );
  }

  function closeLabel() {
    return '← Zurück zu ' + resultLabel();
  }

  function openRecording(recording) {
    const owner = global.VdrSuiteRecordings2;
    if (!owner || typeof owner.openRecording !== 'function') {
      state.error = new Error(
        'Recordings 2 Detail-Owner ist nicht verfügbar.'
      );
      return render();
    }
    owner.openRecording(recording, {
      backendId:state.backendId,
      backLabel:closeLabel(),
      onClose:() => {
        state.active = true;
        render();
      }
    });
  }

  function openEpg(event) {
    const owner = global.VdrSuiteEpgDetailOwner;
    if (!owner || typeof owner.open !== 'function') {
      state.error = new Error('EPG Detail-Owner ist nicht verfügbar.');
      return render();
    }
    owner.open(event, channelFor(event), {
      backLabel:closeLabel(),
      onClose:() => {
        state.active = true;
        render();
      }
    });
  }

  function backFromResults() {
    state.items = [];
    state.total = 0;
    state.hasMore = false;
    state.selectedGenre = null;
    if (state.scope === 'epg' &&
        state.selectedCategory &&
        state.selectedCategory.id === 'movie') {
      state.view = 'movie-genres';
    } else {
      resetNavigation();
    }
    render();
  }

  function renderResults(root) {
    const header = node('div', 'genres-result-header');
    header.append(
      button(
        state.scope === 'epg' &&
          state.selectedCategory &&
          state.selectedCategory.id === 'movie'
          ? '← Filmgenres'
          : state.scope === 'epg'
            ? '← EPG-Hauptkategorien'
            : '← Alle Genres',
        'genres-back',
        backFromResults
      ),
      node('h4', '', resultLabel() + ' · ' + state.total + ' Treffer')
    );
    root.appendChild(header);

    if (!state.items.length) {
      root.appendChild(node(
        'section',
        'genres-status',
        'Für diese Auswahl liegen keine Treffer im aktuellen Fenster vor.'
      ));
      return;
    }

    if (state.scope === 'recordings') {
      const cardOwner = global.VdrSuiteRecordings2BrowserView;
      const list = node(
        'section',
        'genres-recordings recordings2-recording-list'
      );
      state.items.forEach(recording => {
        if (cardOwner &&
            typeof cardOwner.createRecordingCard === 'function') {
          list.appendChild(
            cardOwner.createRecordingCard(recording, openRecording)
          );
        }
      });
      root.appendChild(list);
    } else {
      const list = node('section', 'genres-epg-list');
      state.items.forEach(event => {
        const card = button('', 'genres-epg-card', () => openEpg(event));
        if (event.artwork &&
            event.artwork.available &&
            event.artwork.url) {
          const image = node('img');
          image.src = event.artwork.url;
          image.alt = '';
          image.loading = 'lazy';
          card.appendChild(image);
        }
        const channel = channelFor(event);
        const copy = node('span', 'genres-epg-copy');
        copy.append(
          node('span', 'genres-epg-title', event.title || 'Ohne Titel'),
          node(
            'span',
            'genres-epg-meta',
            text(channel.name, channel.id || 'Unbekannter Kanal') +
              ' · ' + formatTime(event.startTime)
          )
        );
        if (event.subtitle) {
          copy.appendChild(node('span', 'genres-epg-meta', event.subtitle));
        }
        if (event.description) {
          copy.appendChild(
            node('span', 'genres-epg-description', event.description)
          );
        }
        card.appendChild(copy);
        list.appendChild(card);
      });
      root.appendChild(list);
    }

    if (state.hasMore) {
      const more = button(
        state.loadingMore
          ? 'Weitere Treffer werden geladen …'
          : 'Weitere Treffer laden',
        'genres-more',
        loadMore
      );
      more.disabled = state.loadingMore;
      root.appendChild(more);
    }
  }

  function render() {
    if (!state.active) return;
    const mount = target();
    if (!mount) return;
    installStyles();
    mount.classList.remove('recordings2-mount');
    mount.replaceChildren();
    const root = node('section', 'genres-browser');
    renderHeader(root);

    if (state.loading) {
      root.appendChild(node(
        'section',
        'genres-status',
        'Genres werden aus dem persistenten Index geladen …'
      ));
    } else if (state.error) {
      root.appendChild(node(
        'section',
        'genres-status error',
        state.error.message || state.error
      ));
    } else if (state.view === 'movie-genres') {
      renderMovieGenres(root);
    } else if (state.view === 'results') {
      renderResults(root);
    } else {
      renderOverview(root);
    }
    mount.appendChild(root);
  }

  function loadOverview() {
    const client = api();
    if (!client || typeof client.fetchClientGenres !== 'function') {
      state.error = new Error('Genre Client API ist nicht verfügbar.');
      return render();
    }
    state.loading = true;
    state.error = null;
    const sequence = ++state.requestSequence;
    const requestedScope = state.scope;
    render();
    const now = Math.floor(Date.now() / 1000);
    const options = {
      backendId:state.backendId,
      scope:requestedScope,
      locale:'de',
      cache:'no-store',
      credentials:'same-origin'
    };
    if (requestedScope === 'epg') {
      options.from = now;
      options.until = now + 172800;
    }
    client.fetchClientGenres(options)
      .then(result => {
        if (!state.active ||
            sequence !== state.requestSequence ||
            state.scope !== requestedScope) {
          return;
        }
        state.overview = result || {genres:[], categories:[]};
        if (state.selectedCategory) {
          const replacement = epgCategories().find(
            entry => entry.id === state.selectedCategory.id
          );
          if (replacement) state.selectedCategory = replacement;
        }
        state.loading = false;
        render();
      })
      .catch(error => {
        if (!state.active ||
            sequence !== state.requestSequence ||
            state.scope !== requestedScope) {
          return;
        }
        state.loading = false;
        state.error = error;
        render();
      });
  }

  function requestItems(offset) {
    const client = api();
    const options = {
      backendId:state.backendId,
      genreId:text(state.selectedGenre && state.selectedGenre.id),
      limit:PAGE_SIZE,
      offset:offset,
      cache:'no-store',
      credentials:'same-origin'
    };
    if (state.scope === 'recordings') {
      return client.fetchClientGenreRecordings(options);
    }
    const window = epgWindow();
    return client.fetchClientGenreEpg(Object.assign({}, options, {
      contentClass:text(
        state.selectedCategory && state.selectedCategory.id
      ),
      from:window.from,
      until:window.until
    }));
  }

  function applyItems(data, append) {
    const incoming = data && Array.isArray(data.items) ? data.items : [];
    state.items = append ? state.items.concat(incoming) : incoming;
    state.total = number(data && data.total, state.items.length);
    state.hasMore = Boolean(data && data.hasMore);
    synchronizeSelectedCount(state.total);
  }

  function beginResultLoad() {
    state.items = [];
    state.total = number(
      (state.selectedGenre && state.selectedGenre.count) ||
      (state.selectedCategory && state.selectedCategory.count),
      0
    );
    state.hasMore = false;
    state.loading = true;
    state.error = null;
    state.view = 'results';
    const sequence = ++state.requestSequence;
    const requestedScope = state.scope;
    render();
    requestItems(0).then(data => {
      if (!state.active ||
          sequence !== state.requestSequence ||
          state.scope !== requestedScope) {
        return;
      }
      applyItems(data, false);
      state.loading = false;
      render();
    }).catch(error => {
      if (!state.active ||
          sequence !== state.requestSequence ||
          state.scope !== requestedScope) {
        return;
      }
      state.loading = false;
      state.error = error;
      render();
    });
  }

  function selectOverviewEntry(entry) {
    if (state.scope === 'recordings') {
      state.selectedCategory = null;
      state.selectedGenre = entry;
      beginResultLoad();
      return;
    }

    state.selectedCategory = entry;
    state.selectedGenre = null;
    if (entry.id === 'movie') {
      state.view = 'movie-genres';
      state.items = [];
      render();
      return;
    }
    selectEpgResult(entry, null);
  }

  function selectEpgResult(category, genre) {
    state.selectedCategory = category;
    state.selectedGenre = genre;
    beginResultLoad();
  }

  function loadMore() {
    if (state.loadingMore || state.view !== 'results') return;
    state.loadingMore = true;
    const sequence = ++state.requestSequence;
    const requestedScope = state.scope;
    render();
    requestItems(state.items.length).then(data => {
      if (!state.active ||
          sequence !== state.requestSequence ||
          state.scope !== requestedScope) {
        return;
      }
      applyItems(data, true);
      state.loadingMore = false;
      render();
    }).catch(error => {
      if (!state.active ||
          sequence !== state.requestSequence ||
          state.scope !== requestedScope) {
        return;
      }
      state.loadingMore = false;
      state.error = error;
      render();
    });
  }

  const moduleApi = Object.freeze({
    activate: function () {
      const nextBackend = backendId();
      const changed = state.backendId !== nextBackend;
      state.active = true;
      state.backendId = nextBackend;
      installStyles();
      if (changed) {
        state.overview = null;
        resetNavigation();
      }
      if (!state.overview || changed || state.scope === 'epg') loadOverview();
      else render();
    },
    deactivate: function () {
      state.active = false;
      state.requestSequence += 1;
    },
    refresh: function () {
      state.active = true;
      if (state.view === 'results') beginResultLoad();
      else loadOverview();
    },
    __test: Object.freeze({
      genreArtworkUrl:genreArtworkUrl,
      applyItems:applyItems,
      channelFor:channelFor,
      epgCategories:epgCategories,
      epgWindow:epgWindow
    })
  });

  const tab = document.querySelector('[data-module="genres"]');
  if (tab) {
    tab.addEventListener('click', () => {
      document.querySelectorAll('.module-tab').forEach(entry => {
        entry.classList.toggle('active', entry === tab);
      });
      global.setTimeout(moduleApi.activate, 0);
    });
    document.querySelectorAll('.module-tab').forEach(entry => {
      if (entry !== tab) {
        entry.addEventListener('click', moduleApi.deactivate);
      }
    });
  }

  document.addEventListener('click', event => {
    const element = event.target && event.target.closest
      ? event.target.closest('.backend-card,[data-brand-module]')
      : null;
    if (element) moduleApi.deactivate();
  }, true);

  const refresh = document.getElementById('refresh-detail');
  if (refresh) {
    refresh.addEventListener('click', event => {
      if (!tab || !tab.classList.contains('active')) return;
      event.preventDefault();
      event.stopImmediatePropagation();
      moduleApi.refresh();
    }, true);
  }

  if (!platform.hasModule || !platform.hasModule('genres')) {
    platform.registerModule('genres', moduleApi);
  }
  global.VdrSuiteGenres = moduleApi;
}(window));
