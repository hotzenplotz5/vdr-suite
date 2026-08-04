// View owner for persisted SuiteBridge/TVScraper metadata in Recordings 2.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  const personView = global.VdrSuiteRecordings2PersonSearchView;
  if (!shared || !personView) {
    console.error('VDR-Suite Recordings 2 metadata view dependencies are unavailable');
    return;
  }

  const STYLE_ID = 'vdr-suite-recordings2-metadata-detail-style';
  const CSS = [
    '.recordings2-metadata-tabs{display:flex;gap:.4rem;overflow-x:auto;padding:.25rem;border:1px solid rgba(148,163,184,.25);border-radius:.8rem;background:rgba(15,23,42,.82)}',
    '.recordings2-metadata-tab{flex:0 0 auto;min-height:2.45rem!important;padding:.42rem .7rem!important;border-color:transparent!important;background:transparent!important;color:#94a3b8!important;font-size:.78rem!important}',
    '.recordings2-metadata-tab[aria-selected="true"]{background:#1d4ed8!important;color:#eff6ff!important}',
    '.recordings2-metadata-tab:disabled{opacity:.38}',
    '.recordings2-metadata-panel{display:grid;gap:.7rem;scroll-margin-top:5rem}',
    '.recordings2-metadata-panel[hidden],.recordings2-person-results[hidden]{display:none!important}',
    '.recordings2-metadata-status{display:grid;gap:.3rem;padding:.7rem;border:1px solid #334155;border-radius:.7rem;background:#111827;color:#cbd5e1}',
    '.recordings2-metadata-status.error{border-color:#7f1d1d;background:#450a0a;color:#fecaca}',
    '.recordings2-metadata-facts{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.5rem}',
    '.recordings2-metadata-fact{display:grid;gap:.14rem;padding:.62rem;border:1px solid rgba(148,163,184,.2);border-radius:.68rem;background:rgba(15,23,42,.68)}',
    '.recordings2-metadata-fact span{color:#94a3b8;font-size:.66rem;font-weight:800;text-transform:uppercase;letter-spacing:.03em}',
    '.recordings2-metadata-fact strong{overflow-wrap:anywhere;color:#e2e8f0;font-size:.82rem}',
    '.recordings2-metadata-badges{display:flex;flex-wrap:wrap;gap:.38rem}',
    '.recordings2-metadata-badge{display:inline-flex;align-items:center;min-height:1.7rem;padding:.18rem .52rem;border:1px solid #475569;border-radius:999px;background:#172033;color:#dbeafe;font-size:.72rem;font-weight:800}',
    '.recordings2-metadata-copy{display:grid;gap:.35rem;padding:.72rem;border:1px solid rgba(148,163,184,.22);border-radius:.72rem;background:rgba(15,23,42,.68)}',
    '.recordings2-metadata-copy h4{color:#f8fafc}.recordings2-metadata-copy p{color:#dbeafe;line-height:1.5}',
    '.recordings2-metadata-cast{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.65rem}',
    '.recordings2-person-entry{display:grid;gap:.45rem;min-width:0}',
    '.recordings2-person-card{display:grid;grid-template-columns:4.25rem minmax(0,1fr);gap:.65rem;align-items:center;width:100%;padding:.55rem;border:1px solid #334155;border-radius:.75rem;background:#111827;color:inherit;text-align:left}',
    '.recordings2-person-card:hover,.recordings2-person-card:focus-visible{border-color:#60a5fa;background:#172554}',
    '.recordings2-person-image,.recordings2-person-placeholder{width:4.25rem;aspect-ratio:2/3;border-radius:.55rem;object-fit:cover;background:#1e293b}',
    '.recordings2-person-placeholder{display:grid;place-items:center;color:#64748b;font-size:1.3rem;font-weight:900}',
    '.recordings2-person-copy{display:grid;gap:.18rem;min-width:0}',
    '.recordings2-person-name{color:#f8fafc;font-weight:850;line-height:1.25}',
    '.recordings2-person-character{color:#bfdbfe;font-size:.76rem;line-height:1.3}',
    '.recordings2-person-role{color:#94a3b8;font-size:.68rem;font-weight:800;text-transform:uppercase}',
    '.recordings2-person-results{display:grid;gap:.48rem;padding:.55rem;border:1px solid #334155;border-radius:.68rem;background:#020617}',
    '.recordings2-person-recording{display:grid;grid-template-columns:4rem minmax(0,1fr) auto;gap:.6rem;align-items:center;width:100%;padding:.48rem;border:1px solid #334155;border-radius:.68rem;background:#0f172a;color:inherit;text-align:left}',
    '.recordings2-person-recording:hover,.recordings2-person-recording:focus-visible{border-color:#38bdf8;background:#082f49}',
    '.recordings2-person-poster{display:grid;place-items:center;width:4rem;aspect-ratio:2/3;overflow:hidden;border-radius:.5rem;background:#1e293b;color:#e0f2fe}',
    '.recordings2-person-poster img{display:block;width:100%;height:100%;object-fit:cover}',
    '.recordings2-person-recording-copy{display:grid;gap:.16rem;min-width:0}',
    '.recordings2-person-recording-title{color:#f8fafc;font-weight:850}',
    '.recordings2-person-recording-subtitle{color:#bae6fd;font-size:.74rem}',
    '.recordings2-person-recording-meta{color:#94a3b8;font-size:.68rem}',
    '.recordings2-person-recording-details{display:grid;gap:.4rem;padding:.55rem;border:1px solid #1e293b;border-radius:.58rem;background:#0f172a}',
    '.recordings2-person-recording-details[hidden]{display:none!important}',
    '.recordings2-metadata-gallery{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:.58rem}',
    '.recordings2-metadata-image{display:grid;gap:.32rem;margin:0;padding:.36rem;border:1px solid #334155;border-radius:.68rem;background:#111827}',
    '.recordings2-metadata-image img{display:block;width:100%;aspect-ratio:16/9;border-radius:.48rem;object-fit:contain;background:#020617}',
    '.recordings2-metadata-image.portrait img{aspect-ratio:2/3}',
    '.recordings2-metadata-image figcaption{color:#cbd5e1;font-size:.7rem;font-weight:800}',
    '@media(max-width:720px){.recordings2-metadata-facts,.recordings2-metadata-cast{grid-template-columns:1fr}.recordings2-metadata-gallery{grid-template-columns:repeat(2,minmax(0,1fr))}}',
    '@media(max-width:390px){.recordings2-metadata-gallery{grid-template-columns:1fr}.recordings2-person-recording{grid-template-columns:3.5rem minmax(0,1fr) auto}.recordings2-person-poster{width:3.5rem}}'
  ].join('');

  function formatDate(value) {
    const raw = shared.text(value);
    const match = /^(\d{4})-(\d{2})-(\d{2})$/.exec(raw);
    return match ? match[3] + '.' + match[2] + '.' + match[1] : raw;
  }

  function mediaTypeLabel(value) {
    if (value === 'movie') return 'Film';
    if (value === 'series') return 'Serie';
    return shared.text(value) || 'Unbekannt';
  }

  function orientationLabel(value) {
    if (value === 'portrait') return 'Hochformat';
    if (value === 'banner') return 'Banner';
    if (value === 'landscape') return 'Querformat';
    return 'Bild';
  }

  function isPublicMetadataImageUrl(value) {
    return shared.text(value).startsWith('/api/vdr/recordings/metadata/image?');
  }

  function preferredArtworkUrl(value) {
    const artwork = value && value.preferredArtwork;
    return artwork && artwork.available === true ? shared.text(artwork.url) : '';
  }

  function applyToDetail(root, value) {
    if (!root || !value || value.available !== true ||
        typeof root.querySelector !== 'function') return;
    const manual = value.manualAssignment && value.manualAssignment.active === true;
    const title = shared.text(value.title || value.episodeName);
    const description = shared.text(value.overview || value.tagline);
    const heading = manual ? root.querySelector('.recordings2-detail-copy h3') : null;
    const summary = manual ? root.querySelector('.recordings2-detail-description') : null;
    if (heading && title) heading.textContent = title;
    if (summary && description) summary.textContent = description;
    const url = preferredArtworkUrl(value);
    const poster = root.querySelector('.recordings2-detail-poster');
    if (!url || !poster || typeof poster.replaceChildren !== 'function') return;
    const image = document.createElement('img');
    image.src = typeof shared.publicPath === 'function' ? shared.publicPath(url) : url;
    image.alt = 'Poster zu ' + (title || 'Aufnahme');
    image.loading = 'lazy';
    image.addEventListener('error', function () {
      image.remove();
      if (!poster.children || poster.children.length === 0) poster.textContent = '▶';
    });
    poster.replaceChildren(image);
  }

  function installStyles() {
    if (document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = CSS;
    document.head.appendChild(style);
  }

  function status(message, error) {
    const box = shared.node('p', 'recordings2-metadata-status' + (error ? ' error' : ''), message);
    box.setAttribute('role', error ? 'alert' : 'status');
    return box;
  }

  function appendFact(container, label, value) {
    const normalized = shared.text(value);
    if (!normalized) return;
    const fact = shared.node('div', 'recordings2-metadata-fact');
    fact.appendChild(shared.node('span', '', label));
    fact.appendChild(shared.node('strong', '', normalized));
    container.appendChild(fact);
  }

  function appendCopy(container, title, value) {
    const normalized = shared.text(value);
    if (!normalized) return;
    const box = shared.node('section', 'recordings2-metadata-copy');
    box.appendChild(shared.node('h4', '', title));
    box.appendChild(shared.node('p', '', normalized));
    container.appendChild(box);
  }

  function renderScraper(panel, value) {
    panel.replaceChildren();
    const facts = shared.node('div', 'recordings2-metadata-facts');
    appendFact(facts, 'Provider', value.provider);
    appendFact(facts, 'Typ', mediaTypeLabel(value.mediaType));
    appendFact(facts, 'Titel', value.title);
    appendFact(facts, 'Originaltitel', value.originalTitle);
    appendFact(facts, 'Folge', value.episodeName);
    if (Number(value.seasonNumber) > 0) appendFact(facts, 'Staffel', value.seasonNumber);
    if (Number(value.episodeNumber) > 0) appendFact(facts, 'Episode', value.episodeNumber);
    if (Number(value.absoluteEpisodeNumber) > 0) appendFact(facts, 'Folge gesamt', value.absoluteEpisodeNumber);
    if (Number(value.runtimeMinutes) > 0) appendFact(facts, 'Laufzeit', value.runtimeMinutes + ' Minuten');
    appendFact(facts, 'Veröffentlichung', formatDate(value.releaseDate));
    appendFact(facts, 'Erstausstrahlung', formatDate(value.firstAired));
    appendFact(facts, 'Status', value.statusText);
    appendFact(facts, 'Collection', value.collectionName);
    appendFact(facts, 'IMDb', value.imdbId);
    if (Number(value.voteAverage) > 0) {
      appendFact(
        facts,
        'Bewertung',
        value.voteAverage + ' / 10' +
          (Number(value.voteCount) > 0 ? ' · ' + value.voteCount + ' Stimmen' : '')
      );
    }
    if (facts.children.length) panel.appendChild(facts);

    const badges = shared.node('div', 'recordings2-metadata-badges');
    [].concat(value.genres || [], value.productionCountries || [], value.networks || []).forEach(function (entry) {
      if (shared.text(entry)) badges.appendChild(shared.node('span', 'recordings2-metadata-badge', entry));
    });
    if (badges.children.length) panel.appendChild(badges);
    appendCopy(panel, 'Tagline', value.tagline);
    appendCopy(panel, 'TVScraper-Beschreibung', value.overview);
  }

  function renderImages(panel, value, recording) {
    panel.replaceChildren();
    const images = [];
    if (value.preferredArtwork && value.preferredArtwork.available === true &&
        isPublicMetadataImageUrl(value.preferredArtwork.url)) {
      images.push({orientation: 'portrait', label: 'Bevorzugtes Bild', image: value.preferredArtwork});
    }
    (Array.isArray(value.images) ? value.images : []).forEach(function (entry) {
      if (!entry || !entry.image || entry.image.available !== true ||
          !isPublicMetadataImageUrl(entry.image.url)) return;
      images.push({
        orientation: entry.orientation || '',
        label: orientationLabel(entry.orientation),
        image: entry.image
      });
    });
    if (!images.length) {
      panel.appendChild(status('Keine weiteren Bilder verfügbar.', false));
      return;
    }
    const gallery = shared.node('div', 'recordings2-metadata-gallery');
    images.forEach(function (entry) {
      const figure = shared.node(
        'figure',
        'recordings2-metadata-image' + (entry.orientation === 'portrait' ? ' portrait' : '')
      );
      const image = document.createElement('img');
      image.src = entry.image.url;
      image.alt = entry.label + ' zu ' + shared.recordingTitle(recording);
      image.loading = 'lazy';
      figure.appendChild(image);
      figure.appendChild(shared.node('figcaption', '', entry.label));
      gallery.appendChild(figure);
    });
    panel.appendChild(gallery);
  }

  function mount(root, recording, backendId) {
    installStyles();
    const header = root.querySelector('.recordings2-header');
    const hero = root.querySelector('.recordings2-detail-hero');
    const details = root.querySelector('.recordings2-detail-grid');
    if (!hero || !details) return null;

    const tabs = shared.node('nav', 'recordings2-metadata-tabs');
    tabs.setAttribute('role', 'tablist');
    tabs.setAttribute('aria-label', 'Aufnahmedetailbereiche');
    const panels = {
      recording: shared.node('section', 'recordings2-metadata-panel'),
      scraper: shared.node('section', 'recordings2-metadata-panel'),
      cast: shared.node('section', 'recordings2-metadata-panel'),
      images: shared.node('section', 'recordings2-metadata-panel')
    };
    panels.recording.appendChild(hero);
    panels.recording.appendChild(details);
    panels.scraper.hidden = true;
    panels.cast.hidden = true;
    panels.images.hidden = true;

    const buttons = {};
    function addTab(name, label, disabled) {
      const button = shared.node('button', 'recordings2-metadata-tab', label);
      button.type = 'button';
      button.disabled = Boolean(disabled);
      button.setAttribute('role', 'tab');
      button.setAttribute('aria-selected', name === 'recording' ? 'true' : 'false');
      buttons[name] = button;
      tabs.appendChild(button);
    }
    addTab('recording', 'Aufnahme', false);
    addTab('scraper', 'Scraper', true);
    addTab('cast', 'Schauspieler', true);
    addTab('images', 'Bilder', true);

    function activate(name) {
      Object.keys(panels).forEach(function (panelName) {
        panels[panelName].hidden = panelName !== name;
      });
      Object.keys(buttons).forEach(function (buttonName) {
        buttons[buttonName].setAttribute('aria-selected', buttonName === name ? 'true' : 'false');
      });
    }
    Object.keys(buttons).forEach(function (name) {
      buttons[name].addEventListener('click', function () {
        if (!buttons[name].disabled) activate(name);
      });
    });

    if (header) {
      if (header.nextSibling) root.insertBefore(tabs, header.nextSibling);
      else root.appendChild(tabs);
    } else {
      root.insertBefore(tabs, root.firstChild || null);
    }
    const loading = status('Persistierte SuiteBridge-/TVScraper-Metadaten werden geladen …', false);
    root.appendChild(panels.recording);
    root.insertBefore(loading, panels.recording);
    root.appendChild(panels.scraper);
    root.appendChild(panels.cast);
    root.appendChild(panels.images);

    return Object.freeze({
      setMetadata: function (value) {
        if (!value || value.available !== true) {
          loading.textContent = 'Für diese Aufnahme sind keine erweiterten TVScraper-Daten gespeichert.';
          return;
        }
        loading.remove();
        renderScraper(panels.scraper, value);
        personView.renderCast(
          panels.cast,
          value,
          shared.text(backendId) || 'default',
          isPublicMetadataImageUrl
        );
        renderImages(panels.images, value, recording);
        buttons.scraper.disabled = false;
        buttons.cast.disabled = !(Array.isArray(value.people) && value.people.length);
        const preferredAvailable = value.preferredArtwork &&
          value.preferredArtwork.available === true &&
          isPublicMetadataImageUrl(value.preferredArtwork.url);
        const galleryAvailable = Array.isArray(value.images) && value.images.some(function (entry) {
          return entry && entry.image && entry.image.available === true &&
            isPublicMetadataImageUrl(entry.image.url);
        });
        buttons.images.disabled = !(preferredAvailable || galleryAvailable);
        root.dataset.recordings2MetadataAvailable = 'true';
      },
      setError: function (error) {
        loading.classList.add('error');
        loading.setAttribute('role', 'alert');
        loading.textContent = 'Aufnahme-Metadaten konnten nicht geladen werden: ' +
          String(error && error.message ? error.message : error);
      }
    });
  }

  global.VdrSuiteRecordings2MetadataView = Object.freeze({
    mount,
    formatDate,
    isPublicMetadataImageUrl,
    preferredArtworkUrl,
    applyToDetail,
    mediaTypeLabel,
    orientationLabel,
    roleLabel: personView.roleLabel
  });
}(window));