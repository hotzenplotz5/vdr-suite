// Manual movie, series, season and episode assignment for Recordings 2.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  const mountedRoots = typeof WeakSet === 'function' ? new WeakSet() : null;

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function node(tag, className, value) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (value !== undefined && value !== null) element.textContent = text(value);
    return element;
  }

  function button(label, action, className) {
    const element = node('button', className || '', label);
    element.type = 'button';
    element.addEventListener('click', action);
    return element;
  }

  function backendPath(backendId, operation) {
    return '/api/backends/' + encodeURIComponent(text(backendId) || 'default') +
      '/recordings/metadata/' + operation;
  }

  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function api() {
    const client = global.VdrSuiteClientApi;
    if (!client || typeof client.requestJson !== 'function') {
      throw new Error('Client API für manuelle Metadaten ist nicht verfügbar.');
    }
    return client;
  }

  function post(backendId, operation, payload) {
    return api().requestJson(backendPath(backendId, operation), {
      method: 'POST',
      headers: Object.assign({
        'Content-Type': 'application/json'
      }, csrfHeaders()),
      body: JSON.stringify(payload || {}),
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function installStyles() {
    if (document.getElementById('recordings2-metadata-assignment-styles')) return;
    const style = document.createElement('style');
    style.id = 'recordings2-metadata-assignment-styles';
    style.textContent = [
      '.recordings2-metadata-assignment{margin-top:1rem;padding:1rem;border:1px solid rgba(148,163,184,.35);border-radius:.8rem;background:rgba(15,23,42,.45)}',
      '.recordings2-metadata-assignment h4{margin:0 0 .4rem}',
      '.recordings2-metadata-assignment p{margin:.35rem 0}',
      '.recordings2-metadata-assignment-toolbar,.recordings2-metadata-search-row{display:flex;gap:.6rem;flex-wrap:wrap;align-items:center;margin-top:.8rem}',
      '.recordings2-metadata-search-row input,.recordings2-metadata-search-row select{min-height:2.5rem;padding:.5rem .7rem;border-radius:.5rem;border:1px solid #64748b;background:#0f172a;color:#f8fafc}',
      '.recordings2-metadata-search-row input{flex:1 1 15rem}',
      '.recordings2-metadata-assignment button{min-height:2.4rem;padding:.45rem .8rem;border-radius:.5rem;border:1px solid #64748b;background:#1e293b;color:#f8fafc;cursor:pointer}',
      '.recordings2-metadata-assignment button.primary{background:#2563eb;border-color:#3b82f6}',
      '.recordings2-metadata-assignment button.danger{background:#7f1d1d;border-color:#b91c1c}',
      '.recordings2-metadata-assignment button:disabled{opacity:.55;cursor:wait}',
      '.recordings2-metadata-candidates{display:grid;grid-template-columns:repeat(auto-fill,minmax(14rem,1fr));gap:.75rem;margin-top:.9rem}',
      '.recordings2-metadata-candidate{display:flex;flex-direction:column;gap:.4rem;text-align:left;padding:.8rem!important;min-height:10rem!important}',
      '.recordings2-metadata-candidate strong{font-size:1rem}',
      '.recordings2-metadata-candidate small{color:#cbd5e1}',
      '.recordings2-metadata-candidate p{font-size:.88rem;line-height:1.35;color:#e2e8f0}',
      '.recordings2-metadata-candidate-actions{display:flex;gap:.4rem;flex-wrap:wrap;margin-top:auto}',
      '.recordings2-metadata-assignment-status{margin-top:.7rem;padding:.55rem .7rem;border-radius:.5rem;background:rgba(30,41,59,.8)}',
      '.recordings2-metadata-assignment-status.error{background:rgba(127,29,29,.7)}',
      '.recordings2-manual-badge{display:inline-flex;padding:.25rem .55rem;border-radius:999px;background:#1d4ed8;color:white;font-size:.78rem;font-weight:700}'
    ].join('');
    document.head.appendChild(style);
  }

  function candidateLabel(candidate) {
    if (candidate.kind === 'episode') {
      return 'S' + Number(candidate.seasonNumber || 0) +
        ' E' + Number(candidate.episodeNumber || 0);
    }
    if (candidate.kind === 'season') {
      return 'Staffel ' + Number(candidate.seasonNumber || 0);
    }
    return candidate.kind === 'series' ? 'Serie' : 'Film';
  }

  function assignmentPayload(recording, candidate, revision, mediaType) {
    return {
      resourceKey: text(recording && recording.backendNativeId),
      providerId: text(candidate.providerId),
      externalNamespace: text(candidate.externalNamespace),
      externalId: text(candidate.externalId),
      mediaType: mediaType || text(candidate.kind),
      title: text(candidate.title),
      originalTitle: text(candidate.originalTitle),
      overview: text(candidate.overview),
      releaseDate: text(candidate.releaseDate),
      posterReference: text(candidate.posterReference),
      seasonNumber: Number(candidate.seasonNumber || 0),
      episodeNumber: Number(candidate.episodeNumber || 0),
      expectedRevision: Number(revision || 0)
    };
  }

  function mount(root, recording, backendId, metadata) {
    if (!root || !recording) return null;
    if (mountedRoots && mountedRoots.has(root)) return null;
    if (mountedRoots) mountedRoots.add(root);
    installStyles();

    const section = node('section', 'recordings2-metadata-assignment');
    const heading = node('h4', '', 'Metadaten suchen oder korrigieren');
    section.appendChild(heading);

    const current = metadata && metadata.manualAssignment &&
      metadata.manualAssignment.active === true
      ? metadata.manualAssignment
      : null;
    let revision = current ? Number(current.revision || 0) : 0;

    if (current) {
      const badge = node('span', 'recordings2-manual-badge', 'Manuell zugeordnet');
      section.appendChild(badge);
      section.appendChild(node(
        'p',
        '',
        'Diese Auswahl ist gesperrt und wird nicht automatisch von TVScraper überschrieben.'
      ));
    } else {
      section.appendChild(node(
        'p',
        '',
        'Für nicht oder falsch erkannte Aufnahmen kann ein Film oder eine Serie gezielt ausgewählt werden.'
      ));
    }

    const status = node('div', 'recordings2-metadata-assignment-status');
    status.hidden = true;
    section.appendChild(status);

    function setStatus(message, error) {
      status.hidden = !message;
      status.textContent = message || '';
      status.className = 'recordings2-metadata-assignment-status' + (error ? ' error' : '');
    }

    function refreshDetail() {
      const runtime = global.VdrSuiteRecordings2;
      if (runtime && typeof runtime.refreshDetailAddon === 'function') {
        runtime.refreshDetailAddon();
      }
    }

    const toolbar = node('div', 'recordings2-metadata-assignment-toolbar');
    const openButton = button(
      current ? 'Andere Metadaten auswählen' : 'Metadaten suchen',
      openSearch,
      'primary'
    );
    toolbar.appendChild(openButton);

    if (current) {
      toolbar.appendChild(button('Manuelle Zuordnung entfernen', function () {
        setStatus('Zuordnung wird entfernt …', false);
        post(backendId, 'withdraw', {
          resourceKey: text(recording.backendNativeId),
          expectedRevision: revision
        }).then(function () {
          setStatus('Manuelle Zuordnung wurde entfernt.', false);
          refreshDetail();
        }).catch(function (error) {
          setStatus(error.message || String(error), true);
        });
      }, 'danger'));
    }
    section.appendChild(toolbar);

    const searchArea = node('div', 'recordings2-metadata-search');
    searchArea.hidden = true;
    section.appendChild(searchArea);

    function renderCandidates(candidates, mode, parentSeries) {
      const list = node('div', 'recordings2-metadata-candidates');
      const values = Array.isArray(candidates) ? candidates : [];
      if (!values.length) {
        list.appendChild(node('p', '', 'Keine passenden Treffer gefunden.'));
        searchArea.appendChild(list);
        return;
      }

      values.forEach(function (candidate) {
        const card = node('article', 'recordings2-metadata-candidate');
        card.appendChild(node('strong', '', text(candidate.title) || 'Ohne Titel'));
        const meta = [candidateLabel(candidate), text(candidate.releaseDate)]
          .filter(Boolean).join(' · ');
        if (meta) card.appendChild(node('small', '', meta));
        if (candidate.originalTitle && candidate.originalTitle !== candidate.title) {
          card.appendChild(node('small', '', 'Original: ' + candidate.originalTitle));
        }
        if (candidate.overview) {
          card.appendChild(node('p', '', candidate.overview));
        }
        const actions = node('div', 'recordings2-metadata-candidate-actions');

        if (mode === 'series') {
          actions.appendChild(button('Serie verwenden', function () {
            assign(candidate, 'series');
          }, 'primary'));
          actions.appendChild(button('Staffeln anzeigen', function () {
            loadSeasons(candidate);
          }));
        } else if (mode === 'season') {
          actions.appendChild(button('Folgen anzeigen', function () {
            loadEpisodes(parentSeries, candidate);
          }, 'primary'));
        } else {
          actions.appendChild(button('Diesen Treffer verwenden', function () {
            assign(candidate, mode === 'episode' ? 'episode' : 'movie');
          }, 'primary'));
        }
        card.appendChild(actions);
        list.appendChild(card);
      });
      searchArea.appendChild(list);
    }

    function clearResults() {
      Array.from(searchArea.querySelectorAll('.recordings2-metadata-candidates'))
        .forEach(function (element) { element.remove(); });
    }

    function assign(candidate, mediaType) {
      setStatus('Metadaten werden zugeordnet …', false);
      post(
        backendId,
        'assign',
        assignmentPayload(recording, candidate, revision, mediaType)
      ).then(function (result) {
        revision = Number(result && result.revision || revision + 1);
        setStatus('Metadaten wurden dauerhaft zugeordnet.', false);
        refreshDetail();
      }).catch(function (error) {
        setStatus(error.message || String(error), true);
      });
    }

    function loadSeasons(series) {
      clearResults();
      setStatus('Staffeln werden geladen …', false);
      post(backendId, 'seasons', {
        seriesExternalId: text(series.externalId),
        limit: 20
      }).then(function (result) {
        setStatus('', false);
        renderCandidates(result && result.candidates, 'season', series);
      }).catch(function (error) {
        setStatus(error.message || String(error), true);
      });
    }

    function loadEpisodes(series, season) {
      clearResults();
      setStatus('Folgen werden geladen …', false);
      post(backendId, 'episodes', {
        seriesExternalId: text(series.externalId),
        seasonNumber: Number(season.seasonNumber || 0),
        limit: 20
      }).then(function (result) {
        setStatus('', false);
        renderCandidates(result && result.candidates, 'episode', series);
      }).catch(function (error) {
        setStatus(error.message || String(error), true);
      });
    }

    function openSearch() {
      searchArea.hidden = false;
      if (searchArea.dataset.ready === 'true') return;
      searchArea.dataset.ready = 'true';
      const row = node('div', 'recordings2-metadata-search-row');
      const input = document.createElement('input');
      input.type = 'search';
      input.value = text(
        (recording && recording.title) ||
        (shared && shared.recordingTitle && shared.recordingTitle(recording))
      );
      input.placeholder = 'Titel suchen';
      input.setAttribute('aria-label', 'Metadatentitel');
      const kind = document.createElement('select');
      kind.setAttribute('aria-label', 'Medientyp');
      [['movie', 'Film'], ['series', 'Serie']].forEach(function (entry) {
        const option = document.createElement('option');
        option.value = entry[0];
        option.textContent = entry[1];
        kind.appendChild(option);
      });
      const searchButton = button('Suchen', function () {
        const query = input.value.trim();
        if (query.length < 2) {
          setStatus('Bitte mindestens zwei Zeichen eingeben.', true);
          return;
        }
        clearResults();
        searchButton.disabled = true;
        setStatus('Kandidaten werden gesucht …', false);
        post(backendId, 'search', {
          query: query,
          kind: kind.value,
          limit: 12
        }).then(function (result) {
          searchButton.disabled = false;
          setStatus('', false);
          renderCandidates(
            result && result.candidates,
            kind.value === 'series' ? 'series' : 'movie'
          );
        }).catch(function (error) {
          searchButton.disabled = false;
          setStatus(error.message || String(error), true);
        });
      }, 'primary');
      row.append(input, kind, searchButton);
      searchArea.appendChild(row);
    }

    root.appendChild(section);
    return section;
  }

  global.VdrSuiteRecordings2MetadataAssignment = Object.freeze({
    mount: mount,
    __test: Object.freeze({
      backendPath: backendPath,
      assignmentPayload: assignmentPayload,
      candidateLabel: candidateLabel
    })
  });
}(window));
