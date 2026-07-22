// Recordings 2 native SuiteBridge/TVScraper metadata detail orchestrator.
(function (global) {
  'use strict';

  const common = global.VdrSuiteRecordings2MetadataCommon;
  const scraper = global.VdrSuiteRecordings2MetadataScraper;
  const people = global.VdrSuiteRecordings2MetadataPeople;
  const images = global.VdrSuiteRecordings2MetadataImages;

  if (!common || !scraper || !people || !images) {
    console.error('VDR-Suite Recordings 2 metadata renderer dependencies are missing');
    return;
  }

  const {
    clientApi,
    ensureStyles,
    first,
    formatDate,
    isPublicMetadataImageUrl,
    mediaTypeLabel,
    node,
    orientationLabel,
    roleLabel,
    status,
    text
  } = common;

  function fetchMetadata(recording, backendId) {
    const backendNativeId = text(first(recording, ['backendNativeId'], ''));
    if (!backendNativeId) {
      return Promise.reject(new Error('Die Aufnahme besitzt keine stabile Backend-Identität.'));
    }

    const api = clientApi();
    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('Client API für Aufnahme-Metadaten ist nicht verfügbar.'));
    }

    return api.requestJson('/api/vdr/recordings/metadata', {
      query: {
        backend: text(backendId) || 'default',
        backendNativeId: backendNativeId,
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function enhance(root, recording, backendId) {
    if (!root || !recording || root.dataset.recordings2MetadataDetail === 'true') return root;
    ensureStyles();
    root.dataset.recordings2MetadataDetail = 'true';

    const header = root.querySelector('.recordings2-header');
    const hero = root.querySelector('.recordings2-detail-hero');
    const details = root.querySelector('.recordings2-detail-grid');
    if (!hero || !details) return root;

    const tabs = node('nav', 'recordings2-metadata-tabs');
    tabs.setAttribute('role', 'tablist');
    tabs.setAttribute('aria-label', 'Aufnahmedetailbereiche');

    const panels = {
      recording: node('section', 'recordings2-metadata-panel'),
      scraper: node('section', 'recordings2-metadata-panel'),
      cast: node('section', 'recordings2-metadata-panel'),
      images: node('section', 'recordings2-metadata-panel')
    };
    panels.recording.appendChild(hero);
    panels.recording.appendChild(details);
    panels.scraper.hidden = true;
    panels.cast.hidden = true;
    panels.images.hidden = true;

    const buttons = {};
    function addTab(name, label, disabled) {
      const button = node('button', 'recordings2-metadata-tab', label);
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

    fetchMetadata(recording, backendId).then(function (value) {
      if (!value || value.available !== true) {
        loading.textContent = 'Für diese Aufnahme sind keine erweiterten TVScraper-Daten gespeichert.';
        return;
      }

      loading.remove();
      scraper.render(panels.scraper, value);
      people.render(panels.cast, value, text(backendId) || 'default');
      images.render(panels.images, value, recording);
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
    }).catch(function (error) {
      loading.classList.add('error');
      loading.setAttribute('role', 'alert');
      loading.textContent = 'Aufnahme-Metadaten konnten nicht geladen werden: ' +
        String(error && error.message ? error.message : error);
    });

    return root;
  }

  global.VdrSuiteRecordings2MetadataDetail = Object.freeze({
    enhance,
    formatDate,
    isPublicMetadataImageUrl,
    mediaTypeLabel,
    orientationLabel,
    roleLabel
  });
}(window));
