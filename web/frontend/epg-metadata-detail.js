'use strict';

(function (global) {
  const STYLE_ID = 'vdr-suite-epg-metadata-detail-style';
  const requestCache = new Map();
  const roleLabels = Object.freeze({
    actor: 'Schauspiel',
    director: 'Regie',
    writer: 'Drehbuch',
    producer: 'Produktion',
    moderator: 'Moderation',
    guest: 'Gast',
    composer: 'Musik',
    other: 'Mitwirkung',
    unknown: 'Mitwirkung'
  });

  function firstValue(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
        return object[key];
      }
    }
    return fallback;
  }

  function node(tag, className, text) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (text !== undefined && text !== null) element.textContent = String(text);
    return element;
  }

  function selectedBackendId() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedBackendId === 'function') {
      const value = String(platform.getSelectedBackendId() || '').trim();
      if (value) return value;
    }
    return 'default';
  }

  function eventChannelId(event, channel) {
    return String(firstValue(
      event,
      ['channelId', 'channel', 'channel_id'],
      firstValue(channel, ['id', 'channelId', 'nativeId'], '')
    )).trim();
  }

  function eventId(event) {
    return String(firstValue(event, ['eventId', 'id', 'nativeId'], '')).trim();
  }

  function isPublicImageUrl(value) {
    const url = String(value || '').trim();
    return url.startsWith('/api/epg/cache/metadata/image?') ||
      url.startsWith('/api/epg/cache/artwork?');
  }

  function formatDate(value) {
    const text = String(value || '').trim();
    const match = /^(\d{4})-(\d{2})-(\d{2})$/.exec(text);
    return match ? match[3] + '.' + match[2] + '.' + match[1] : text;
  }

  function roleLabel(value) {
    return roleLabels[String(value || 'unknown').toLowerCase()] || roleLabels.unknown;
  }

  function mediaTypeLabel(value) {
    if (value === 'movie') return 'Film';
    if (value === 'series') return 'Serie';
    return 'Unbekannt';
  }

  function orientationLabel(value) {
    if (value === 'landscape') return 'Querformat';
    if (value === 'banner') return 'Banner';
    if (value === 'portrait') return 'Hochformat';
    return 'Bild';
  }

  function ensureStyles() {
    if (document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = [
      '.epg-metadata-tabs{display:flex;gap:.4rem;overflow-x:auto;margin:.15rem 0 .85rem;padding:.2rem;border:1px solid #334155;border-radius:.75rem;background:#0f172a}',
      '.epg-metadata-tab{flex:0 0 auto;border:0;border-radius:.58rem;padding:.55rem .75rem;background:transparent;color:#94a3b8;font:inherit;font-size:.82rem;font-weight:800;cursor:pointer}',
      '.epg-metadata-tab[aria-selected="true"]{background:#1d4ed8;color:#eff6ff}',
      '.epg-metadata-tab:disabled{opacity:.42;cursor:not-allowed}',
      '.epg-metadata-panel{display:grid;gap:.8rem;margin-bottom:.85rem}',
      '.epg-metadata-status,.epg-person-search-result{margin:.35rem 0 .8rem;padding:.7rem .8rem;border:1px solid #334155;border-radius:.7rem;background:#111827;color:#cbd5e1;font-size:.86rem}',
      '.epg-metadata-facts{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.55rem}',
      '.epg-metadata-fact{display:grid;gap:.15rem;padding:.55rem .65rem;border:1px solid #334155;border-radius:.65rem;background:#0f172a}',
      '.epg-metadata-fact-label{color:#94a3b8;font-size:.68rem;font-weight:800;text-transform:uppercase;letter-spacing:.04em}',
      '.epg-metadata-fact-value{color:#f8fafc;font-size:.88rem;overflow-wrap:anywhere}',
      '.epg-metadata-copy{padding:.75rem;border:1px solid #334155;border-radius:.7rem;background:#111827}',
      '.epg-metadata-copy h4,.epg-person-search-result h4{margin:0 0 .4rem;color:#f8fafc}',
      '.epg-metadata-copy p{margin:0;color:#dbeafe;line-height:1.5}',
      '.epg-metadata-badges{display:flex;flex-wrap:wrap;gap:.4rem}',
      '.epg-metadata-badge{display:inline-flex;align-items:center;min-height:1.75rem;padding:.2rem .55rem;border:1px solid #475569;border-radius:999px;background:#172033;color:#dbeafe;font-size:.74rem;font-weight:800}',
      '.epg-metadata-cast{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.65rem}',
      '.epg-person-card{display:grid;grid-template-columns:4.4rem minmax(0,1fr);gap:.65rem;align-items:center;width:100%;padding:.55rem;border:1px solid #334155;border-radius:.75rem;background:#111827;color:inherit;text-align:left;cursor:pointer}',
      '.epg-person-card:hover,.epg-person-card:focus-visible{border-color:#60a5fa;background:#172554;outline:none}',
      '.epg-person-image,.epg-person-placeholder{width:4.4rem;aspect-ratio:2/3;border-radius:.55rem;object-fit:cover;background:#1e293b}',
      '.epg-person-placeholder{display:grid;place-items:center;color:#64748b;font-size:1.4rem;font-weight:900}',
      '.epg-person-name{display:block;color:#f8fafc;font-weight:850;line-height:1.25}',
      '.epg-person-character{display:block;margin-top:.2rem;color:#bfdbfe;font-size:.78rem;line-height:1.3}',
      '.epg-person-role{display:block;margin-top:.25rem;color:#94a3b8;font-size:.7rem;font-weight:800;text-transform:uppercase}',
      '.epg-person-search-result ul{margin:.4rem 0 0;padding-left:1.15rem}',
      '.epg-metadata-gallery-feature{width:100%;max-height:22rem;border-radius:.75rem;object-fit:contain;background:#020617}',
      '.epg-metadata-gallery-thumbs{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:.55rem}',
      '.epg-gallery-thumb{display:grid;gap:.3rem;padding:.35rem;border:1px solid #334155;border-radius:.65rem;background:#111827;color:#cbd5e1;cursor:pointer}',
      '.epg-gallery-thumb[aria-current="true"]{border-color:#60a5fa;background:#172554}',
      '.epg-gallery-thumb img{width:100%;aspect-ratio:16/9;border-radius:.45rem;object-fit:cover;background:#020617}',
      '.epg-gallery-thumb.portrait img{aspect-ratio:2/3;object-fit:contain}',
      '.epg-gallery-thumb span{font-size:.68rem;font-weight:800}',
      '.epg-metadata-link{color:#93c5fd;font-weight:800;text-decoration:none}',
      '@media(max-width:720px){.epg-metadata-facts,.epg-metadata-cast{grid-template-columns:1fr}.epg-metadata-gallery-thumbs{grid-template-columns:repeat(2,minmax(0,1fr))}}'
    ].join('');
    document.head.appendChild(style);
  }

  function fetchMetadata(backendId, channelId, nativeEventId) {
    const key = backendId + '\n' + channelId + '\n' + nativeEventId;
    if (requestCache.has(key)) return requestCache.get(key);

    const client = global.VdrSuiteClientApi;
    if (!client || typeof client.requestJson !== 'function') {
      return Promise.reject(new Error('EPG-Metadaten-Client ist nicht verfügbar.'));
    }

    const request = client.requestJson('/api/epg/cache/metadata', {
      query: {
        backend: backendId,
        channelId: channelId,
        eventId: nativeEventId
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).catch(function (error) {
      requestCache.delete(key);
      throw error;
    });

    requestCache.set(key, request);
    return request;
  }

  function appendFact(container, label, value) {
    const text = String(value === undefined || value === null ? '' : value).trim();
    if (!text) return;
    const fact = node('div', 'epg-metadata-fact');
    fact.appendChild(node('span', 'epg-metadata-fact-label', label));
    fact.appendChild(node('span', 'epg-metadata-fact-value', text));
    container.appendChild(fact);
  }

  function renderScraper(panel, metadata) {
    panel.replaceChildren();
    const facts = node('div', 'epg-metadata-facts');
    appendFact(facts, 'Typ', mediaTypeLabel(metadata.mediaType));
    appendFact(facts, 'Titel', metadata.title);
    appendFact(facts, 'Originaltitel', metadata.originalTitle);
    appendFact(facts, 'Folge', metadata.episodeName);
    if (Number(metadata.seasonNumber) > 0) appendFact(facts, 'Staffel', metadata.seasonNumber);
    if (Number(metadata.episodeNumber) > 0) appendFact(facts, 'Episode', metadata.episodeNumber);
    if (Number(metadata.absoluteEpisodeNumber) > 0) appendFact(facts, 'Folge gesamt', metadata.absoluteEpisodeNumber);
    if (Number(metadata.runtimeMinutes) > 0) appendFact(facts, 'Laufzeit', metadata.runtimeMinutes + ' Minuten');

    if (metadata.mediaType === 'series') {
      appendFact(facts, 'Serienstart', formatDate(metadata.releaseDate));
      appendFact(facts, 'Erstausstrahlung Folge', formatDate(metadata.firstAired));
    } else {
      appendFact(facts, 'Veröffentlichung', formatDate(metadata.releaseDate));
    }

    appendFact(facts, 'Sender / Netzwerk', (metadata.networks || []).join(', '));
    appendFact(facts, 'Produktionsland', (metadata.productionCountries || []).join(', '));
    appendFact(facts, 'Status', metadata.status);
    if (Number(metadata.voteAverage) > 0) {
      appendFact(
        facts,
        'Bewertung',
        metadata.voteAverage + ' / 10' + (Number(metadata.voteCount) > 0 ? ' · ' + metadata.voteCount + ' Stimmen' : '')
      );
    }

    if (metadata.imdbId && /^tt\d+$/.test(String(metadata.imdbId))) {
      const fact = node('div', 'epg-metadata-fact');
      fact.appendChild(node('span', 'epg-metadata-fact-label', 'IMDb'));
      const link = node('a', 'epg-metadata-fact-value epg-metadata-link', metadata.imdbId);
      link.href = 'https://www.imdb.com/title/' + encodeURIComponent(metadata.imdbId) + '/';
      link.target = '_blank';
      link.rel = 'noopener noreferrer';
      fact.appendChild(link);
      facts.appendChild(fact);
    }
    if (facts.children.length) panel.appendChild(facts);

    const badges = node('div', 'epg-metadata-badges');
    (metadata.genres || []).forEach(function (genre) {
      if (genre) badges.appendChild(node('span', 'epg-metadata-badge', genre));
    });
    const hints = metadata.providerHints || {};
    if (Number(hints.hd) > 0) {
      badges.appendChild(node('span', 'epg-metadata-badge', 'HD-Hinweis · TVScraper'));
    }
    if (Number(hints.language) > 0) {
      badges.appendChild(node('span', 'epg-metadata-badge', 'Sprachcode ' + hints.language + ' · TVScraper'));
    }
    if (badges.children.length) panel.appendChild(badges);

    if (metadata.tagline) {
      const box = node('div', 'epg-metadata-copy');
      box.appendChild(node('h4', '', 'Tagline'));
      box.appendChild(node('p', '', metadata.tagline));
      panel.appendChild(box);
    }
    if (metadata.overview) {
      const box = node('div', 'epg-metadata-copy');
      box.appendChild(node('h4', '', 'TVScraper-Beschreibung'));
      box.appendChild(node('p', '', metadata.overview));
      panel.appendChild(box);
    }
  }

  function searchRecordings(target, person, metadata, backendId) {
    target.hidden = false;
    target.replaceChildren();
    target.appendChild(node('h4', '', person.name));
    target.appendChild(node('p', '', 'Aktuelle EPG-Sendung: ' + (metadata.episodeName || metadata.title || 'Sendung')));
    target.appendChild(node('p', '', 'Suche in vorhandenen Aufnahmen …'));

    const client = global.VdrSuiteClientApi;
    if (!client || typeof client.fetchClientRecordingPersons !== 'function') {
      target.appendChild(node('p', '', 'Aufnahmensuche ist nicht verfügbar.'));
      return;
    }

    client.fetchClientRecordingPersons({
      backendId: backendId,
      query: {name: person.name, limit: 20},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function (result) {
      const matches = result && Array.isArray(result.matches) ? result.matches : [];
      target.replaceChildren();
      target.appendChild(node('h4', '', person.name));
      target.appendChild(node('p', '', 'Aktuelle EPG-Sendung: ' + (metadata.episodeName || metadata.title || 'Sendung')));
      if (!matches.length) {
        target.appendChild(node('p', '', 'Keine vorhandene Aufnahme mit dieser Person gefunden.'));
        return;
      }
      target.appendChild(node('p', '', 'Gefundene Aufnahmen: ' + matches.length));
      const list = document.createElement('ul');
      matches.forEach(function (match) {
        const recording = match && match.recording ? match.recording : {};
        list.appendChild(node('li', '', (recording.title || recording.id || 'Aufnahme') + ' · ' + (recording.backendId || backendId)));
      });
      target.appendChild(list);
    }).catch(function (error) {
      target.replaceChildren();
      target.appendChild(node('h4', '', person.name));
      target.appendChild(node('p', '', 'Aufnahmensuche fehlgeschlagen: ' + String(error && error.message ? error.message : error)));
    });
  }

  function renderCast(panel, metadata, backendId) {
    panel.replaceChildren();
    const people = Array.isArray(metadata.people) ? metadata.people : [];
    if (!people.length) {
      panel.appendChild(node('p', 'epg-metadata-status', 'Keine Besetzungsdaten verfügbar.'));
      return;
    }

    const grid = node('div', 'epg-metadata-cast');
    const result = node('div', 'epg-person-search-result');
    result.hidden = true;

    people.forEach(function (person) {
      const card = node('button', 'epg-person-card');
      card.type = 'button';
      card.title = person.name + ' in vorhandenen Aufnahmen suchen';

      if (person.image && person.image.available === true && isPublicImageUrl(person.image.url)) {
        const image = document.createElement('img');
        image.className = 'epg-person-image';
        image.src = person.image.url;
        image.alt = person.name;
        image.loading = 'lazy';
        card.appendChild(image);
      } else {
        card.appendChild(node('span', 'epg-person-placeholder', '•'));
      }

      const copy = document.createElement('span');
      copy.appendChild(node('span', 'epg-person-name', person.name));
      if (person.characterName) copy.appendChild(node('span', 'epg-person-character', person.characterName));
      copy.appendChild(node('span', 'epg-person-role', roleLabel(person.role)));
      card.appendChild(copy);
      card.addEventListener('click', function () {
        searchRecordings(result, person, metadata, backendId);
      });
      grid.appendChild(card);
    });

    panel.appendChild(grid);
    panel.appendChild(result);
  }

  function renderGallery(panel, metadata) {
    panel.replaceChildren();
    const images = Array.isArray(metadata.images)
      ? metadata.images.filter(function (entry) {
          return entry && entry.image && entry.image.available === true && isPublicImageUrl(entry.image.url);
        })
      : [];

    if (!images.length) {
      panel.appendChild(node('p', 'epg-metadata-status', 'Keine weiteren Bilder verfügbar.'));
      return;
    }

    const feature = document.createElement('img');
    feature.className = 'epg-metadata-gallery-feature';
    feature.loading = 'lazy';
    panel.appendChild(feature);

    const thumbs = node('div', 'epg-metadata-gallery-thumbs');
    const buttons = [];
    function select(index) {
      feature.src = images[index].image.url;
      feature.alt = orientationLabel(images[index].orientation) + ' zu ' + (metadata.title || 'Sendung');
      buttons.forEach(function (button, currentIndex) {
        button.setAttribute('aria-current', currentIndex === index ? 'true' : 'false');
      });
    }

    images.forEach(function (entry, index) {
      const button = node('button', 'epg-gallery-thumb ' + (entry.orientation === 'portrait' ? 'portrait' : ''));
      button.type = 'button';
      const image = document.createElement('img');
      image.src = entry.image.url;
      image.alt = orientationLabel(entry.orientation);
      image.loading = 'lazy';
      button.appendChild(image);
      button.appendChild(node('span', '', orientationLabel(entry.orientation)));
      button.addEventListener('click', function () { select(index); });
      buttons.push(button);
      thumbs.appendChild(button);
    });

    panel.appendChild(thumbs);
    select(0);
  }

  function ensurePreferredArtwork(detail, metadata) {
    if (detail.querySelector('.epg-detail-artwork')) return;
    const artwork = metadata && metadata.preferredArtwork;
    const hero = detail.querySelector('.epg-detail-hero');
    if (!hero || !artwork || artwork.available !== true || !isPublicImageUrl(artwork.url)) return;

    const image = document.createElement('div');
    image.className = 'epg-detail-artwork';
    image.setAttribute('role', 'img');
    image.setAttribute('aria-label', 'Bild zu ' + (metadata.episodeName || metadata.title || 'Sendung'));
    image.style.backgroundImage = 'url("' + String(artwork.url).replace(/["\\\r\n]/g, '') + '")';
    detail.classList.add('epg-has-artwork');
    detail.insertBefore(image, hero);
  }

  function enhance(detail, event, channel) {
    if (!detail || detail.dataset.epgMetadataDetail === 'true') return detail;

    const backendId = selectedBackendId();
    const channelId = eventChannelId(event, channel);
    const nativeEventId = eventId(event);
    if (!channelId || !nativeEventId) return detail;

    ensureStyles();
    detail.dataset.epgMetadataDetail = 'true';

    const baseSections = [
      detail.querySelector('.epg-detail-hero'),
      detail.querySelector('.epg-detail-meta-grid'),
      detail.querySelector('.epg-detail-description')
    ].filter(Boolean);
    const actions = detail.querySelector('.epg-detail-actions');
    const tabs = node('div', 'epg-metadata-tabs');
    tabs.setAttribute('role', 'tablist');
    tabs.setAttribute('aria-label', 'EPG Detailbereiche');

    const buttons = {};
    const panels = {};
    function addTab(name, label, disabled) {
      const button = node('button', 'epg-metadata-tab', label);
      button.type = 'button';
      button.disabled = Boolean(disabled);
      button.setAttribute('role', 'tab');
      button.setAttribute('aria-selected', name === 'epg' ? 'true' : 'false');
      buttons[name] = button;
      tabs.appendChild(button);
    }

    addTab('epg', 'EPG', false);
    addTab('scraper', 'Scraper', true);
    addTab('cast', 'Besetzung', true);
    addTab('images', 'Bilder', true);

    ['scraper', 'cast', 'images'].forEach(function (name) {
      const panel = node('section', 'epg-metadata-panel');
      panel.hidden = true;
      panels[name] = panel;
      detail.insertBefore(panel, actions || null);
    });

    const status = node('p', 'epg-metadata-status', 'TVScraper-Metadaten werden geladen …');
    detail.insertBefore(status, actions || null);

    function activate(name) {
      baseSections.forEach(function (section) { section.hidden = name !== 'epg'; });
      Object.keys(panels).forEach(function (panelName) { panels[panelName].hidden = panelName !== name; });
      Object.keys(buttons).forEach(function (buttonName) {
        buttons[buttonName].setAttribute('aria-selected', buttonName === name ? 'true' : 'false');
      });
    }

    Object.keys(buttons).forEach(function (name) {
      buttons[name].addEventListener('click', function () {
        if (!buttons[name].disabled) activate(name);
      });
    });

    const hero = detail.querySelector('.epg-detail-hero');
    detail.insertBefore(tabs, hero || detail.firstChild || null);

    fetchMetadata(backendId, channelId, nativeEventId).then(function (metadata) {
      if (!metadata || metadata.available !== true) {
        status.textContent = 'Für diese Sendung sind keine erweiterten TVScraper-Daten verfügbar.';
        return;
      }

      status.remove();
      ensurePreferredArtwork(detail, metadata);
      renderScraper(panels.scraper, metadata);
      renderCast(panels.cast, metadata, backendId);
      renderGallery(panels.images, metadata);
      buttons.scraper.disabled = false;
      buttons.cast.disabled = !(Array.isArray(metadata.people) && metadata.people.length);
      buttons.images.disabled = !(Array.isArray(metadata.images) && metadata.images.length);
      detail.dataset.epgMetadataAvailable = 'true';
    }).catch(function (error) {
      status.textContent = 'TVScraper-Metadaten konnten nicht geladen werden: ' + String(error && error.message ? error.message : error);
    });

    return detail;
  }

  ensureStyles();
  global.VdrSuiteEpgMetadataDetail = Object.freeze({
    enhance: enhance,
    formatDate: formatDate,
    isPublicImageUrl: isPublicImageUrl,
    mediaTypeLabel: mediaTypeLabel,
    roleLabel: roleLabel
  });
}(window));
