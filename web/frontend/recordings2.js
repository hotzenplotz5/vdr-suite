// Independent mobile-first recording browser. The legacy recording browser remains untouched.
(function (global) {
  'use strict';

  const PAGE_SIZE = 50;
  const CSS = `
#detail-data.recordings2-mount{display:block!important;width:100%!important;max-width:none!important}
.recordings2{display:grid;gap:.8rem;width:100%}.recordings2 *{box-sizing:border-box}.recordings2 h3,.recordings2 h4,.recordings2 p{margin:0}
.recordings2-header{display:grid;gap:.65rem;padding:.85rem;border:1px solid rgba(56,189,248,.28);border-radius:.9rem;background:rgba(2,6,23,.78)}
.recordings2-heading{display:flex;align-items:flex-start;justify-content:space-between;gap:.75rem}.recordings2-heading-copy{display:grid;gap:.2rem;min-width:0}.recordings2-heading h3{color:#f8fafc;font-size:1.15rem}.recordings2-heading p{color:#94a3b8;font-size:.82rem;line-height:1.35}
.recordings2-toolbar{display:flex;flex-wrap:wrap;gap:.45rem}.recordings2 button{min-height:2.75rem;padding:.55rem .8rem;border:1px solid rgba(96,165,250,.52);border-radius:.7rem;background:rgba(30,64,175,.28);color:#e0f2fe;font:inherit;font-weight:750}.recordings2 button:disabled{opacity:.48}.recordings2 button.recordings2-primary{background:#0369a1;color:#fff}
.recordings2-breadcrumbs{display:flex;flex-wrap:wrap;align-items:center;gap:.28rem;color:#94a3b8;font-size:.8rem}.recordings2-breadcrumbs button{min-height:2.2rem;padding:.32rem .5rem;border-radius:.55rem;background:rgba(15,23,42,.75);font-size:.78rem}.recordings2-separator{color:#475569}
.recordings2-status{display:grid;gap:.45rem;padding:.85rem;border:1px solid rgba(148,163,184,.24);border-radius:.8rem;background:rgba(15,23,42,.72);color:#cbd5e1}.recordings2-status strong{color:#f8fafc}.recordings2-status.error{border-color:rgba(248,113,113,.55);background:rgba(69,10,10,.38);color:#fecaca}.recordings2-status-detail{overflow-wrap:anywhere;color:#94a3b8;font-size:.76rem;line-height:1.35}.recordings2-progress{width:100%;height:.55rem}
.recordings2-summary{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:.45rem}.recordings2-metric{display:grid;gap:.1rem;padding:.6rem;border:1px solid rgba(148,163,184,.2);border-radius:.7rem;background:rgba(15,23,42,.64);text-align:center}.recordings2-metric strong{color:#f8fafc;font-size:1.05rem}.recordings2-metric span{color:#94a3b8;font-size:.7rem}
.recordings2-section{display:grid;gap:.55rem}.recordings2-section-title{display:flex;align-items:center;justify-content:space-between;gap:.5rem;color:#e2e8f0}.recordings2-count{padding:.16rem .45rem;border:1px solid rgba(148,163,184,.25);border-radius:999px;color:#94a3b8;font-size:.72rem}
.recordings2-folder-list,.recordings2-recording-list{display:grid;gap:.55rem}.recordings2-folder{display:grid;grid-template-columns:3rem minmax(0,1fr) auto;align-items:center;gap:.7rem;width:100%;padding:.72rem;text-align:left;background:rgba(15,23,42,.78)}.recordings2-folder-icon{display:grid;place-items:center;width:3rem;height:3rem;border-radius:.7rem;background:rgba(14,165,233,.18);font-size:1.5rem}.recordings2-folder-copy{display:grid;gap:.12rem;min-width:0}.recordings2-folder-name{overflow:hidden;color:#f8fafc;font-weight:850;text-overflow:ellipsis;white-space:nowrap}.recordings2-folder-meta{color:#94a3b8;font-size:.78rem}.recordings2-chevron{color:#7dd3fc;font-size:1.35rem}
.recordings2-recording{display:grid;grid-template-columns:5.2rem minmax(0,1fr);gap:.72rem;padding:.68rem;border:1px solid rgba(148,163,184,.2);border-radius:.82rem;background:rgba(15,23,42,.74);color:#f8fafc;text-align:left}.recordings2-recording:hover,.recordings2-recording:focus-visible{border-color:rgba(56,189,248,.65);background:rgba(14,165,233,.12)}.recordings2-poster{display:grid;place-items:center;width:5.2rem;aspect-ratio:2/3;overflow:hidden;border:1px solid rgba(125,211,252,.3);border-radius:.65rem;background:linear-gradient(145deg,#075985,#1e293b 58%,#020617);color:#e0f2fe;font-size:1.5rem}.recordings2-poster[data-variant="1"]{background:linear-gradient(145deg,#9a3412,#4c1d95 58%,#020617)}.recordings2-poster[data-variant="2"]{background:linear-gradient(145deg,#047857,#164e63 58%,#020617)}.recordings2-poster[data-variant="3"]{background:linear-gradient(145deg,#92400e,#7f1d1d 58%,#020617)}.recordings2-poster[data-variant="4"]{background:linear-gradient(145deg,#5b21b6,#1e3a8a 58%,#020617)}.recordings2-poster[data-variant="5"]{background:linear-gradient(145deg,#9d174d,#4338ca 58%,#020617)}.recordings2-poster img{display:block;width:100%;height:100%;object-fit:cover}.recordings2-recording-copy{display:grid;align-content:start;gap:.28rem;min-width:0}.recordings2-title{color:#f8fafc;font-size:.98rem;font-weight:850;line-height:1.25}.recordings2-subtitle{color:#bae6fd;font-size:.8rem;line-height:1.3}.recordings2-meta{color:#94a3b8;font-size:.76rem;line-height:1.38}.recordings2-summary-text{display:-webkit-box;overflow:hidden;color:#cbd5e1;font-size:.79rem;line-height:1.38;-webkit-box-orient:vertical;-webkit-line-clamp:3}
.recordings2-empty{padding:1rem;border:1px dashed rgba(148,163,184,.3);border-radius:.8rem;color:#94a3b8;text-align:center}.recordings2-more{width:100%}
.recordings2-detail{display:grid;gap:.8rem}.recordings2-detail-hero{display:grid;grid-template-columns:minmax(8rem,11rem) minmax(0,1fr);gap:.9rem;padding:.85rem;border:1px solid rgba(56,189,248,.3);border-radius:.9rem;background:rgba(2,6,23,.82)}.recordings2-detail-poster{width:100%;aspect-ratio:2/3}.recordings2-detail-copy{display:grid;align-content:start;gap:.5rem;min-width:0}.recordings2-detail-copy h3{color:#f8fafc;font-size:1.25rem;line-height:1.2}.recordings2-detail-description{color:#dbe4f0;line-height:1.5;white-space:pre-line}.recordings2-detail-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.5rem}.recordings2-detail-field{display:grid;gap:.15rem;padding:.65rem;border:1px solid rgba(148,163,184,.18);border-radius:.7rem;background:rgba(15,23,42,.65)}.recordings2-detail-field span{color:#94a3b8;font-size:.7rem;text-transform:uppercase}.recordings2-detail-field strong{overflow-wrap:anywhere;color:#e2e8f0;font-size:.82rem}
@media(max-width:720px){.recordings2-header{padding:.72rem}.recordings2-heading{display:grid}.recordings2-toolbar{display:grid;grid-template-columns:1fr 1fr}.recordings2-toolbar button{width:100%}.recordings2-recording{grid-template-columns:4.65rem minmax(0,1fr);gap:.62rem;padding:.58rem}.recordings2-poster{width:4.65rem}.recordings2-detail-hero{grid-template-columns:6.7rem minmax(0,1fr);gap:.7rem;padding:.7rem}.recordings2-detail-grid{grid-template-columns:1fr}.recordings2-detail-copy h3{font-size:1.08rem}}
@media(max-width:390px){.recordings2-toolbar{grid-template-columns:1fr}.recordings2-folder{grid-template-columns:2.6rem minmax(0,1fr) auto;padding:.62rem}.recordings2-folder-icon{width:2.6rem;height:2.6rem}.recordings2-detail-hero{grid-template-columns:1fr}.recordings2-detail-poster{max-width:10rem}.recordings2-summary{gap:.3rem}.recordings2-metric{padding:.5rem .3rem}}
`;

  const state = {
    active: false,
    backendId: '',
    path: '',
    parentPath: '',
    data: null,
    recordings: [],
    selectedRecording: null,
    loading: false,
    loadingMore: false,
    error: null,
    requestSequence: 0
  };

  function platform() {
    return global.VdrSuitePlatform || null;
  }

  function clientApi() {
    const boundary = platform();
    if (boundary && typeof boundary.getClientApi === 'function') {
      return boundary.getClientApi();
    }
    return global.VdrSuiteClientApi || null;
  }

  function mountTarget() {
    const boundary = platform();
    if (boundary && typeof boundary.getMountTarget === 'function') {
      return boundary.getMountTarget('recordings2') ||
        boundary.getMountTarget('detail') ||
        document.getElementById('detail-data');
    }
    return document.getElementById('detail-data');
  }

  function selectedBackendId() {
    const boundary = platform();
    if (boundary && typeof boundary.getSelectedBackendId === 'function') {
      const value = String(boundary.getSelectedBackendId() || '').trim();
      if (value) return value;
    }
    const selected = document.querySelector('.backend-card.selected');
    return selected && selected.dataset.backendId ? String(selected.dataset.backendId) : 'default';
  }

  function text(value) {
    return String(value === undefined || value === null ? '' : value).trim();
  }

  function number(value, fallback) {
    const result = Number(value);
    return Number.isFinite(result) ? result : (fallback || 0);
  }

  function first(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
        return object[key];
      }
    }
    return fallback;
  }

  function normalizePath(value) {
    return text(value)
      .replace(/^\/+|\/+$/g, '')
      .split('/')
      .map(part => part.trim())
      .filter(Boolean)
      .join('/');
  }

  function decodeDisplayText(value) {
    let decoded = text(value);
    for (let pass = 0; pass < 6; pass += 1) {
      const next = decoded.replace(/#([0-9A-Fa-f]{2})/g, function (_, hex) {
        return String.fromCharCode(parseInt(hex, 16));
      });
      if (next === decoded) break;
      decoded = next;
    }
    return decoded.replace(/_/g, ' ').replace(/^%+/, '').trim();
  }

  function folderList(data) {
    return data && Array.isArray(data.folders) ? data.folders : [];
  }

  function recordingList(data) {
    return data && Array.isArray(data.recordings) ? data.recordings : [];
  }

  function metadata(recording) {
    return recording && recording.metadata && typeof recording.metadata === 'object'
      ? recording.metadata
      : {};
  }

  function presentation(recording) {
    const value = metadata(recording).presentation;
    return value && typeof value === 'object' ? value : {};
  }

  function provider(recording) {
    const value = metadata(recording).provider;
    return value && typeof value === 'object' ? value : {};
  }

  function nativeMetadata(recording) {
    const value = metadata(recording).native;
    return value && typeof value === 'object' ? value : {};
  }

  function artwork(recording) {
    const value = metadata(recording).artwork;
    return value && typeof value === 'object' ? value : {};
  }

  function recordingTitle(recording) {
    return decodeDisplayText(first(
      presentation(recording),
      ['title'],
      first(provider(recording), ['seriesTitle', 'title'], first(recording, ['title', 'name'], 'Aufnahme'))
    ));
  }

  function recordingSubtitle(recording) {
    return decodeDisplayText(first(
      presentation(recording),
      ['subtitle', 'seasonEpisode'],
      first(provider(recording), ['episodeTitle', 'tagline'], first(nativeMetadata(recording), ['shortText'], ''))
    ));
  }

  function recordingSummary(recording) {
    return text(first(
      presentation(recording),
      ['summary'],
      first(provider(recording), ['overview'], first(nativeMetadata(recording), ['description', 'shortText'], ''))
    ));
  }

  function recordingPosterUrl(recording) {
    return text(first(
      presentation(recording),
      ['posterUrl'],
      first(artwork(recording), ['preferredUrl'], '')
    ));
  }

  function recordingPlaceholderVariant(recording) {
    return Math.abs(number(first(presentation(recording), ['placeholderVariant'], 0), 0)) % 6;
  }

  function formatStart(value) {
    const numeric = Number(value);
    if (Number.isFinite(numeric) && numeric > 1000000000) {
      return new Date(numeric * 1000).toLocaleString('de-DE', {
        day: '2-digit', month: '2-digit', year: 'numeric', hour: '2-digit', minute: '2-digit'
      });
    }
    return text(value) || 'Unbekannt';
  }

  function formatDuration(value) {
    const seconds = number(value, 0);
    if (seconds <= 0) return 'Dauer unbekannt';
    const minutes = Math.round(seconds / 60);
    if (minutes < 60) return String(minutes) + ' min';
    const hours = Math.floor(minutes / 60);
    const rest = minutes % 60;
    return String(hours) + ' h' + (rest ? ' ' + String(rest) + ' min' : '');
  }

  function formatSize(value) {
    const megabytes = number(value, 0);
    if (megabytes <= 0) return 'Größe unbekannt';
    return megabytes >= 1024
      ? (megabytes / 1024).toFixed(1) + ' GB'
      : String(Math.round(megabytes)) + ' MB';
  }

  function ensureStyles() {
    if (document.getElementById('vdr-suite-recordings2-styles')) return;
    const style = document.createElement('style');
    style.id = 'vdr-suite-recordings2-styles';
    style.textContent = CSS;
    document.head.appendChild(style);
  }

  function addText(element, value) {
    element.textContent = String(value);
    return element;
  }

  function createButton(label, action, className) {
    const button = addText(document.createElement('button'), label);
    button.type = 'button';
    if (className) button.className = className;
    button.addEventListener('click', action);
    return button;
  }

  function createPoster(recording, className) {
    const poster = document.createElement('div');
    poster.className = className || 'recordings2-poster';
    poster.dataset.variant = String(recordingPlaceholderVariant(recording));
    const url = recordingPosterUrl(recording);
    if (!url) {
      poster.textContent = '▶';
      return poster;
    }
    const image = document.createElement('img');
    image.src = url;
    image.alt = 'Poster zu ' + recordingTitle(recording);
    image.loading = 'lazy';
    image.addEventListener('error', function () {
      image.remove();
      poster.textContent = '▶';
    });
    poster.appendChild(image);
    return poster;
  }

  function createHeader() {
    const header = document.createElement('header');
    header.className = 'recordings2-header';
    const heading = document.createElement('div');
    heading.className = 'recordings2-heading';
    const copy = document.createElement('div');
    copy.className = 'recordings2-heading-copy';
    copy.appendChild(addText(document.createElement('h3'), state.selectedRecording ? 'Aufnahmedetails' : 'Recordings 2'));
    copy.appendChild(addText(
      document.createElement('p'),
      'Eigenständiger Aufnahmebrowser · Backend ' + (state.backendId || selectedBackendId())
    ));
    heading.appendChild(copy);
    header.appendChild(heading);

    const toolbar = document.createElement('div');
    toolbar.className = 'recordings2-toolbar';
    if (state.selectedRecording) {
      toolbar.appendChild(createButton('← Zum Ordner', function () {
        state.selectedRecording = null;
        render();
      }, 'recordings2-primary'));
    } else if (state.path) {
      toolbar.appendChild(createButton('← Zurück', function () {
        loadFolder(state.parentPath || '');
      }, 'recordings2-primary'));
    }
    toolbar.appendChild(createButton('Neu laden', function () {
      if (state.selectedRecording) state.selectedRecording = null;
      loadFolder(state.path || '');
    }));
    header.appendChild(toolbar);

    if (!state.selectedRecording) header.appendChild(createBreadcrumbs());
    return header;
  }

  function createBreadcrumbs() {
    const breadcrumbs = document.createElement('nav');
    breadcrumbs.className = 'recordings2-breadcrumbs';
    breadcrumbs.setAttribute('aria-label', 'Aufnahmeordner');
    breadcrumbs.appendChild(createButton('Aufnahmen', function () { loadFolder(''); }));
    const parts = normalizePath(state.path).split('/').filter(Boolean);
    let current = '';
    parts.forEach(function (part) {
      current = current ? current + '/' + part : part;
      const target = current;
      breadcrumbs.appendChild(addText(document.createElement('span'), '›')).className = 'recordings2-separator';
      breadcrumbs.appendChild(createButton(decodeDisplayText(part), function () { loadFolder(target); }));
    });
    return breadcrumbs;
  }

  function createStatus(title, message, error, technical) {
    const box = document.createElement('section');
    box.className = 'recordings2-status' + (error ? ' error' : '');
    box.setAttribute('role', error ? 'alert' : 'status');
    box.setAttribute('aria-live', 'polite');
    box.appendChild(addText(document.createElement('strong'), title));
    if (message) box.appendChild(addText(document.createElement('p'), message));
    if (technical) {
      const detail = addText(document.createElement('p'), technical);
      detail.className = 'recordings2-status-detail';
      box.appendChild(detail);
    }
    return box;
  }

  function renderLoading() {
    const target = mountTarget();
    if (!target) return;
    ensureStyles();
    target.classList.add('recordings2-mount');
    target.replaceChildren();
    const root = document.createElement('section');
    root.className = 'recordings2';
    root.appendChild(createHeader());
    const status = createStatus(
      'Aufnahmeordner wird geladen …',
      state.path ? 'Ordner: ' + decodeDisplayText(state.path) : 'Lade den Hauptordner.',
      false,
      '/api/vdr/recordings/folder · Backend ' + state.backendId
    );
    const progress = document.createElement('progress');
    progress.className = 'recordings2-progress';
    progress.setAttribute('aria-label', 'Aufnahmeordner wird geladen');
    status.appendChild(progress);
    root.appendChild(status);
    target.appendChild(root);
  }

  function renderError() {
    const target = mountTarget();
    if (!target) return;
    ensureStyles();
    target.classList.add('recordings2-mount');
    target.replaceChildren();
    const root = document.createElement('section');
    root.className = 'recordings2';
    root.appendChild(createHeader());
    const message = state.error && state.error.message ? state.error.message : String(state.error || 'Unbekannter Fehler');
    const status = createStatus(
      'Aufnahmeordner konnte nicht geladen werden',
      message,
      true,
      'Backend: ' + state.backendId + ' · Pfad: ' + (state.path || '<Hauptordner>') + ' · Endpunkt: /api/vdr/recordings/folder'
    );
    status.appendChild(createButton('Erneut versuchen', function () { loadFolder(state.path || ''); }, 'recordings2-primary'));
    root.appendChild(status);
    target.appendChild(root);
  }

  function createMetric(value, label) {
    const metric = document.createElement('div');
    metric.className = 'recordings2-metric';
    metric.appendChild(addText(document.createElement('strong'), String(value)));
    metric.appendChild(addText(document.createElement('span'), label));
    return metric;
  }

  function createFolderCard(folder) {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'recordings2-folder';
    const icon = addText(document.createElement('span'), '📁');
    icon.className = 'recordings2-folder-icon';
    const copy = document.createElement('span');
    copy.className = 'recordings2-folder-copy';
    const name = addText(document.createElement('span'), decodeDisplayText(first(folder, ['name'], 'Ordner')));
    name.className = 'recordings2-folder-name';
    const count = addText(document.createElement('span'), String(number(folder.recordingCount, 0)) + ' Aufnahme(n)');
    count.className = 'recordings2-folder-meta';
    copy.append(name, count);
    const chevron = addText(document.createElement('span'), '›');
    chevron.className = 'recordings2-chevron';
    button.append(icon, copy, chevron);
    button.addEventListener('click', function () {
      loadFolder(normalizePath(first(folder, ['path'], '')));
    });
    return button;
  }

  function createRecordingCard(recording) {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'recordings2-recording';
    button.appendChild(createPoster(recording));
    const copy = document.createElement('span');
    copy.className = 'recordings2-recording-copy';
    const title = addText(document.createElement('span'), recordingTitle(recording));
    title.className = 'recordings2-title';
    copy.appendChild(title);
    const subtitleValue = recordingSubtitle(recording);
    if (subtitleValue) {
      const subtitle = addText(document.createElement('span'), subtitleValue);
      subtitle.className = 'recordings2-subtitle';
      copy.appendChild(subtitle);
    }
    const meta = addText(
      document.createElement('span'),
      formatStart(first(recording, ['startTime', 'start'], '')) + ' · ' +
      formatDuration(first(recording, ['durationSeconds', 'duration'], 0)) + ' · ' +
      formatSize(first(recording, ['sizeMb'], 0))
    );
    meta.className = 'recordings2-meta';
    copy.appendChild(meta);
    const summaryValue = recordingSummary(recording);
    if (summaryValue) {
      const summary = addText(document.createElement('span'), summaryValue);
      summary.className = 'recordings2-summary-text';
      copy.appendChild(summary);
    }
    button.appendChild(copy);
    button.addEventListener('click', function () {
      state.selectedRecording = recording;
      render();
    });
    return button;
  }

  function createSection(title, count, className) {
    const section = document.createElement('section');
    section.className = 'recordings2-section';
    const heading = document.createElement('div');
    heading.className = 'recordings2-section-title';
    heading.appendChild(addText(document.createElement('h4'), title));
    const badge = addText(document.createElement('span'), String(count));
    badge.className = 'recordings2-count';
    heading.appendChild(badge);
    section.appendChild(heading);
    const list = document.createElement('div');
    list.className = className;
    section.appendChild(list);
    return { section: section, list: list };
  }

  function renderFolder() {
    const target = mountTarget();
    if (!target) return;
    ensureStyles();
    target.classList.add('recordings2-mount');
    target.replaceChildren();
    const root = document.createElement('section');
    root.className = 'recordings2';
    root.appendChild(createHeader());

    const data = state.data || {};
    const folders = folderList(data);
    const recordings = state.recordings;
    const summary = document.createElement('section');
    summary.className = 'recordings2-summary';
    summary.appendChild(createMetric(number(data.totalCount, folders.length + recordings.length), 'Gesamt'));
    summary.appendChild(createMetric(number(data.folderCount, folders.length), 'Ordner'));
    summary.appendChild(createMetric(number(data.recordingCount, recordings.length), 'Aufnahmen'));
    root.appendChild(summary);

    if (text(data.cacheState) && data.cacheReady === false) {
      root.appendChild(createStatus(
        'Recording-Cache wird vorbereitet',
        'Die bereits verfügbaren Einträge werden angezeigt. Ein erneutes Laden aktualisiert den Stand.',
        false,
        'Cache-Zustand: ' + text(data.cacheState)
      ));
    }

    if (folders.length) {
      const folderSection = createSection('Ordner', folders.length, 'recordings2-folder-list');
      folders.forEach(function (folder) { folderSection.list.appendChild(createFolderCard(folder)); });
      root.appendChild(folderSection.section);
    }

    if (recordings.length) {
      const recordingSection = createSection('Aufnahmen', number(data.recordingCount, recordings.length), 'recordings2-recording-list');
      recordings.forEach(function (recording) { recordingSection.list.appendChild(createRecordingCard(recording)); });
      root.appendChild(recordingSection.section);
    }

    if (!folders.length && !recordings.length) {
      const empty = addText(document.createElement('section'), 'Dieser Aufnahmeordner ist leer.');
      empty.className = 'recordings2-empty';
      root.appendChild(empty);
    }

    const returned = number(data.returnedCount, recordingList(data).length);
    const totalRecordings = number(data.recordingCount, recordings.length);
    if (recordings.length < totalRecordings && returned > 0) {
      const more = createButton(
        state.loadingMore ? 'Weitere Aufnahmen werden geladen …' : 'Weitere Aufnahmen laden',
        loadMore,
        'recordings2-more'
      );
      more.disabled = state.loadingMore;
      root.appendChild(more);
    }

    target.appendChild(root);
  }

  function detailField(label, value) {
    const field = document.createElement('div');
    field.className = 'recordings2-detail-field';
    field.appendChild(addText(document.createElement('span'), label));
    field.appendChild(addText(document.createElement('strong'), text(value) || '–'));
    return field;
  }

  function renderDetail() {
    const target = mountTarget();
    if (!target || !state.selectedRecording) return;
    ensureStyles();
    target.classList.add('recordings2-mount');
    target.replaceChildren();
    const root = document.createElement('section');
    root.className = 'recordings2 recordings2-detail';
    root.appendChild(createHeader());

    const recording = state.selectedRecording;
    const hero = document.createElement('article');
    hero.className = 'recordings2-detail-hero';
    hero.appendChild(createPoster(recording, 'recordings2-poster recordings2-detail-poster'));
    const copy = document.createElement('div');
    copy.className = 'recordings2-detail-copy';
    copy.appendChild(addText(document.createElement('h3'), recordingTitle(recording)));
    const subtitle = recordingSubtitle(recording);
    if (subtitle) {
      const subtitleNode = addText(document.createElement('p'), subtitle);
      subtitleNode.className = 'recordings2-subtitle';
      copy.appendChild(subtitleNode);
    }
    const description = addText(document.createElement('p'), recordingSummary(recording) || 'Keine Beschreibung vorhanden.');
    description.className = 'recordings2-detail-description';
    copy.appendChild(description);
    hero.appendChild(copy);
    root.appendChild(hero);

    const providerData = provider(recording);
    const details = document.createElement('section');
    details.className = 'recordings2-detail-grid';
    details.appendChild(detailField('Aufgenommen', formatStart(first(recording, ['startTime', 'start'], ''))));
    details.appendChild(detailField('Dauer', formatDuration(first(recording, ['durationSeconds', 'duration'], 0))));
    details.appendChild(detailField('Größe', formatSize(first(recording, ['sizeMb'], 0))));
    details.appendChild(detailField('Ordner', first(recording, ['path'], state.path || 'Hauptordner')));
    details.appendChild(detailField('Genre', first(providerData, ['genreText'], '')));
    details.appendChild(detailField('Veröffentlichung', first(providerData, ['releaseDate'], '')));
    details.appendChild(detailField('Bewertung', number(first(providerData, ['rating'], 0), 0) > 0 ? String(first(providerData, ['rating'], 0)) : ''));
    details.appendChild(detailField('Metadatenquelle', first(providerData, ['source'], 'VDR')));
    root.appendChild(details);
    target.appendChild(root);
  }

  function render() {
    if (!state.active) return;
    if (state.loading) return renderLoading();
    if (state.error) return renderError();
    if (state.selectedRecording) return renderDetail();
    renderFolder();
  }

  function requestFolder(path, offset) {
    const api = clientApi();
    if (!api || typeof api.fetchClientRecordingFolder !== 'function') {
      return Promise.reject(new Error('Client API für Aufnahmeordner ist nicht verfügbar.'));
    }
    return api.fetchClientRecordingFolder({
      query: {
        backend: state.backendId,
        path: normalizePath(path),
        limit: PAGE_SIZE,
        offset: Math.max(0, number(offset, 0)),
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function applyFolderData(data, append) {
    if (!data || data.recordingFolder !== true) {
      throw new Error('Der Server hat keinen gültigen Aufnahmeordner geliefert.');
    }
    state.data = append ? Object.assign({}, state.data || {}, data) : data;
    state.path = normalizePath(first(data, ['path'], state.path));
    state.parentPath = normalizePath(first(data, ['parentPath'], ''));
    state.recordings = append
      ? state.recordings.concat(recordingList(data))
      : recordingList(data).slice();
  }

  function loadFolder(path) {
    state.active = true;
    state.backendId = selectedBackendId();
    state.path = normalizePath(path);
    state.parentPath = state.path.split('/').slice(0, -1).join('/');
    state.selectedRecording = null;
    state.loading = true;
    state.loadingMore = false;
    state.error = null;
    const sequence = ++state.requestSequence;
    render();
    requestFolder(state.path, 0)
      .then(function (data) {
        if (!state.active || sequence !== state.requestSequence) return;
        applyFolderData(data, false);
        state.loading = false;
        render();
      })
      .catch(function (error) {
        if (!state.active || sequence !== state.requestSequence) return;
        state.loading = false;
        state.error = error;
        render();
      });
  }

  function loadMore() {
    if (state.loadingMore || state.loading) return;
    state.loadingMore = true;
    const sequence = ++state.requestSequence;
    render();
    requestFolder(state.path, state.recordings.length)
      .then(function (data) {
        if (!state.active || sequence !== state.requestSequence) return;
        applyFolderData(data, true);
        state.loadingMore = false;
        render();
      })
      .catch(function (error) {
        if (!state.active || sequence !== state.requestSequence) return;
        state.loadingMore = false;
        state.error = error;
        render();
      });
  }

  const moduleApi = Object.freeze({
    activate: function () {
      const backend = selectedBackendId();
      if (!state.active || state.backendId !== backend || !state.data) {
        state.backendId = backend;
        loadFolder('');
        return;
      }
      state.active = true;
      render();
    },
    deactivate: function () {
      state.active = false;
      state.requestSequence += 1;
      state.selectedRecording = null;
      const target = mountTarget();
      if (target) target.classList.remove('recordings2-mount');
    },
    refresh: function () {
      state.active = true;
      loadFolder(state.path || '');
    },
    openFolder: function (path) {
      loadFolder(path || '');
    },
    __test: Object.freeze({
      normalizePath: normalizePath,
      decodeDisplayText: decodeDisplayText,
      recordingTitle: recordingTitle,
      recordingSubtitle: recordingSubtitle,
      recordingSummary: recordingSummary,
      recordingPosterUrl: recordingPosterUrl,
      formatDuration: formatDuration,
      formatSize: formatSize,
      applyFolderData: applyFolderData
    })
  });

  function ensureNavigationTab() {
    let tab = document.querySelector('[data-module="recordings2"]');
    if (tab) return tab;
    const navigation = document.getElementById('module-nav');
    if (!navigation) return null;
    tab = document.createElement('button');
    tab.type = 'button';
    tab.className = 'module-tab';
    tab.dataset.module = 'recordings2';
    tab.textContent = 'Recordings 2';
    tab.setAttribute('aria-label', 'Recordings 2 öffnen');
    const legacy = navigation.querySelector('[data-module="recordings"]');
    if (legacy && legacy.nextSibling) navigation.insertBefore(tab, legacy.nextSibling);
    else navigation.appendChild(tab);
    return tab;
  }

  function installShellEntry() {
    const tab = ensureNavigationTab();
    if (!tab) return;

    tab.addEventListener('click', function () {
      document.querySelectorAll('.module-tab').forEach(function (button) {
        button.classList.toggle('active', button === tab);
      });
      const channels2 = global.VdrSuiteChannels2;
      if (channels2 && typeof channels2.deactivate === 'function') channels2.deactivate();
      global.setTimeout(function () { moduleApi.activate(); }, 0);
    });

    document.querySelectorAll('.module-tab').forEach(function (button) {
      if (button === tab) return;
      button.addEventListener('click', function () { moduleApi.deactivate(); });
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

    document.addEventListener('click', function (event) {
      const backend = event.target && typeof event.target.closest === 'function'
        ? event.target.closest('.backend-card')
        : null;
      if (backend) moduleApi.deactivate();
    }, true);
  }

  global.VdrSuiteRecordings2 = moduleApi;
  const boundary = platform();
  if (boundary && typeof boundary.registerModule === 'function' &&
      (!boundary.hasModule || !boundary.hasModule('recordings2'))) {
    boundary.registerModule('recordings2', moduleApi);
  }
  installShellEntry();
}(window));
