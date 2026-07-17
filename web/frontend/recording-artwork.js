(function () {
  'use strict';

  const artworkUrlPattern =
    /^\/recording-artwork\/(?:[A-Za-z0-9._~-]|%[0-9A-Fa-f]{2})+\/[0-9A-Fa-f]{32}$/;
  const genreArtworkSpriteUrl =
    '/channel-logos/vdr-suite-brand/recording-genre-sprite.svg';
  const genreArtworkAliases = Object.freeze({
    horror: 'horror',
    grusel: 'horror',
    katastrophe: 'katastrophenfilm',
    katastrophenfilm: 'katastrophenfilm',
    fantasy: 'fantasy',
    historie: 'historienfilm',
    historienfilm: 'historienfilm',
    comedy: 'komoedie',
    komodie: 'komoedie',
    krieg: 'krieg',
    kriegsfilm: 'krieg',
    thriller: 'thriller',
    musik: 'musik',
    music: 'musik',
    drama: 'drama',
    mystery: 'mystery',
    mysterium: 'mystery',
    scifi: 'scifi',
    sciencefiction: 'scifi',
    serie: 'serien',
    serien: 'serien',
    western: 'western',
    doku: 'doku',
    dokumentation: 'doku',
    documentary: 'doku',
    action: 'action',
    musical: 'musical'
  });

  function installRecordingArtworkStyles() {
    if (document.getElementById('vdr-suite-recording-artwork-styles')) {
      return;
    }

    const style = document.createElement('style');
    style.id = 'vdr-suite-recording-artwork-styles';
    style.textContent = [
      '.recording-artwork-image,',
      '.recording-genre-artwork-image {',
      '  position: absolute;',
      '  z-index: 1;',
      '  display: block;',
      '  pointer-events: none;',
      '  border: 1px solid rgba(125, 211, 252, 0.38);',
      '  border-radius: 0.82rem;',
      '  background-color: rgba(2, 6, 23, 0.92);',
      '  box-shadow: 0 0.65rem 1.4rem rgba(2, 6, 23, 0.35);',
      '}',
      '.recording-artwork-image {',
      '  object-fit: cover;',
      '  object-position: center;',
      '}',
      '.recording-list-item .recording-artwork-image,',
      '.recording-folder-item .recording-genre-artwork-image {',
      '  left: 0.8rem;',
      '  top: 0.72rem;',
      '  width: 4.45rem;',
      '  height: 6.35rem;',
      '}',
      '.recording-detail .recording-artwork-image {',
      '  left: 1rem;',
      '  top: 1rem;',
      '  width: 8.6rem;',
      '  height: 12rem;',
      '}',
      '.recording-folder-item.has-recording-genre-artwork {',
      '  position: relative;',
      '  isolation: isolate;',
      '  min-height: 7.5rem;',
      '  padding-left: 6.25rem !important;',
      '}',
      '.recording-genre-artwork-image {',
      '  background-image: url("' + genreArtworkSpriteUrl + '");',
      '  background-repeat: no-repeat;',
      '  background-size: 300% 200%;',
      '}',
      '.recording-genre-artwork-horror {',
      '  background-position: 0% 0%;',
      '}',
      '.recording-genre-artwork-katastrophenfilm {',
      '  background-position: 50% 0%;',
      '}',
      '.recording-genre-artwork-fantasy {',
      '  background-position: 100% 0%;',
      '}',
      '.recording-genre-artwork-historienfilm {',
      '  background-position: 0% 100%;',
      '}',
      '.recording-genre-artwork-komoedie {',
      '  background-position: 50% 100%;',
      '}',
      '.recording-genre-artwork-krieg {',
      '  background-position: 100% 100%;',
      '}',
      '.recording-genre-artwork-thriller {',
      '  background-image: url("/channel-logos/vdr-suite-brand/recording-genre-thriller.svg");',
      '  background-size: cover;',
      '  background-position: center;',
      '}',
      '.recording-genre-artwork-musik {',
      '  background-image: url("/channel-logos/vdr-suite-brand/recording-genre-musik.svg");',
      '  background-size: cover;',
      '  background-position: center;',
      '}',
      '.recording-genre-artwork-drama {',
      '  background-image: url("/channel-logos/vdr-suite-brand/recording-genre-drama.svg");',
      '  background-size: cover;',
      '  background-position: center;',
      '}',
      '.recording-genre-artwork-mystery {',
      '  background-image: url("/channel-logos/vdr-suite-brand/recording-genre-mystery.svg");',
      '  background-size: cover;',
      '  background-position: center;',
      '}',
      '.recording-genre-artwork-scifi {',
      '  background-image: url("/channel-logos/vdr-suite-brand/recording-genre-scifi.svg");',
      '  background-size: cover;',
      '  background-position: center;',
      '}',
      '.recording-genre-artwork-serien {',
      '  background-image: url("/channel-logos/vdr-suite-brand/recording-genre-serien.svg");',
      '  background-size: cover;',
      '  background-position: center;',
      '}',
      '.recording-genre-artwork-western {',
      '  background-image: url("/channel-logos/vdr-suite-brand/recording-genre-western.svg");',
      '  background-size: cover;',
      '  background-position: center;',
      '}',
      '.recording-genre-artwork-doku {',
      '  background-image: url("/channel-logos/vdr-suite-brand/recording-genre-doku.svg");',
      '  background-size: cover;',
      '  background-position: center;',
      '}',
      '.recording-genre-artwork-action {',
      '  background-image: url("/channel-logos/vdr-suite-brand/recording-genre-action.svg");',
      '  background-size: cover;',
      '  background-position: center;',
      '}',
      '.recording-genre-artwork-musical {',
      '  background-image: url("/channel-logos/vdr-suite-brand/recording-genre-musical.svg");',
      '  background-size: cover;',
      '  background-position: center;',
      '}',
      '.recording-list-item.has-recording-artwork::before,',
      '.recording-detail.has-recording-artwork::before {',
      '  opacity: 0;',
      '}',
      '@keyframes vdr-suite-recording-card-reveal {',
      '  from {',
      '    opacity: 0;',
      '    transform: translateY(0.45rem) scale(0.992);',
      '  }',
      '  to {',
      '    opacity: 1;',
      '    transform: translateY(0) scale(1);',
      '  }',
      '}',
      '@media (min-width: 72rem) {',
      '  .recording-folder-list {',
      '    grid-template-columns: repeat(auto-fit, minmax(25rem, 1fr));',
      '    gap: 0.9rem;',
      '  }',
      '  .recording-folder-list > .module-placeholder,',
      '  .recording-folder-list > .recording-list-item:not(.recording-folder-item) {',
      '    grid-column: 1 / -1;',
      '  }',
      '  .recording-folder-list > .recording-folder-item {',
      '    box-sizing: border-box;',
      '    min-width: 0;',
      '    padding-top: 0.55rem;',
      '    padding-bottom: 0.55rem;',
      '    animation: vdr-suite-recording-card-reveal 180ms ease-out both;',
      '    transition:',
      '      transform 160ms ease,',
      '      border-color 160ms ease,',
      '      background-color 160ms ease,',
      '      box-shadow 160ms ease;',
      '    will-change: transform;',
      '  }',
      '  .recording-folder-list > .recording-folder-item:hover,',
      '  .recording-folder-list > .recording-folder-item:focus-visible {',
      '    z-index: 2;',
      '    transform: translateY(-0.22rem);',
      '    border-color: rgba(125, 211, 252, 0.78);',
      '    background-color: rgba(15, 23, 42, 0.98);',
      '    box-shadow:',
      '      0 1rem 2.2rem rgba(2, 6, 23, 0.48),',
      '      0 0 0 1px rgba(56, 189, 248, 0.18);',
      '    outline: none;',
      '  }',
      '  .recording-folder-list > .recording-folder-item:hover .recording-artwork-image,',
      '  .recording-folder-list > .recording-folder-item:hover .recording-genre-artwork-image,',
      '  .recording-folder-list > .recording-folder-item:focus-visible .recording-artwork-image,',
      '  .recording-folder-list > .recording-folder-item:focus-visible .recording-genre-artwork-image {',
      '    border-color: rgba(125, 211, 252, 0.82);',
      '    box-shadow: 0 0.9rem 1.8rem rgba(2, 6, 23, 0.5);',
      '  }',
      '  .recording-folder-list > .recording-folder-item.has-recording-genre-artwork,',
      '  .recording-folder-list > .recording-folder-item.has-recording-artwork {',
      '    min-height: 13.7rem;',
      '    padding-left: 10.45rem !important;',
      '  }',
      '  .recording-folder-list > .recording-folder-item .list-title {',
      '    line-height: 1.12;',
      '  }',
      '  .recording-folder-list > .recording-folder-item .list-meta {',
      '    margin-top: 0.12rem;',
      '    line-height: 1.2;',
      '  }',
      '  .recording-folder-list > .recording-folder-item .recording-artwork-image,',
      '  .recording-folder-list > .recording-folder-item .recording-genre-artwork-image {',
      '    left: 0.75rem;',
      '    top: 0.45rem;',
      '    width: 8.9rem;',
      '    height: 12.7rem;',
      '    transition: border-color 160ms ease, box-shadow 160ms ease;',
      '  }',
      '  .recording-detail {',
      '    min-height: 20rem;',
      '    padding-left: 14.5rem !important;',
      '  }',
      '  .recording-detail::before,',
      '  .recording-detail .recording-artwork-image {',
      '    left: 1rem;',
      '    top: 1rem;',
      '    width: 12rem;',
      '    height: 18rem;',
      '  }',
      '}',
      '@media (prefers-reduced-motion: reduce) {',
      '  .recording-folder-list > .recording-folder-item {',
      '    animation: none !important;',
      '    transition: none !important;',
      '  }',
      '}',
      '@media (max-width: 760px) {',
      '  .recording-list-item .recording-artwork-image,',
      '  .recording-folder-item .recording-genre-artwork-image {',
      '    left: 0.65rem;',
      '    top: 0.62rem;',
      '    width: 3.85rem;',
      '    height: 5.5rem;',
      '    border-radius: 0.7rem;',
      '  }',
      '  .recording-folder-item.has-recording-genre-artwork {',
      '    min-height: 6.7rem;',
      '    padding-left: 5.25rem !important;',
      '  }',
      '  .recording-detail .recording-artwork-image {',
      '    left: 0.75rem;',
      '    top: 0.75rem;',
      '    width: 5.8rem;',
      '    height: 8.35rem;',
      '    border-radius: 0.72rem;',
      '  }',
      '}'
    ].join('\n');
    document.head.appendChild(style);
  }

  function recordingArtworkPresentation(recording) {
    if (!recording || typeof recording !== 'object') {
      return {};
    }

    const metadata = recording.metadata;
    if (!metadata || typeof metadata !== 'object') {
      return {};
    }

    const presentation = metadata.presentation;
    return presentation && typeof presentation === 'object'
      ? presentation
      : {};
  }

  function recordingArtworkSummary(recording) {
    if (!recording || typeof recording !== 'object') {
      return {};
    }

    const metadata = recording.metadata;
    if (!metadata || typeof metadata !== 'object') {
      return {};
    }

    const artwork = metadata.artwork;
    return artwork && typeof artwork === 'object'
      ? artwork
      : {};
  }

  function recordingArtworkUrl(recording) {
    const presentation = recordingArtworkPresentation(recording);
    const artwork = recordingArtworkSummary(recording);
    const candidate = String(
      presentation.posterUrl || artwork.preferredUrl || ''
    ).trim();

    if (!artworkUrlPattern.test(candidate)) {
      return '';
    }

    return candidate;
  }

  function recordingArtworkTitle(recording, fallbackTitle) {
    const presentation = recordingArtworkPresentation(recording);
    return String(
      presentation.title ||
      fallbackTitle ||
      (recording && recording.title) ||
      'Aufnahme'
    );
  }

  function attachRecordingArtwork(container, recording, fallbackTitle, eager) {
    if (!container || container.dataset.recordingArtworkAttached === 'true') {
      return;
    }

    const url = recordingArtworkUrl(recording);
    if (url === '') {
      return;
    }

    container.dataset.recordingArtworkAttached = 'true';

    const image = document.createElement('img');
    image.className = 'recording-artwork-image';
    image.src = url;
    image.alt = 'Poster: ' + recordingArtworkTitle(recording, fallbackTitle);
    image.decoding = 'async';
    image.loading = eager ? 'eager' : 'lazy';
    image.referrerPolicy = 'same-origin';

    image.addEventListener('load', () => {
      container.classList.add('has-recording-artwork');
    });

    image.addEventListener('error', () => {
      container.classList.remove('has-recording-artwork');
      container.dataset.recordingArtworkAttached = 'false';
      image.remove();
    });

    container.appendChild(image);
  }

  function normalizeGenreArtworkName(value) {
    return String(value || '')
      .normalize('NFD')
      .replace(/[\u0300-\u036f]/g, '')
      .toLocaleLowerCase('de-DE')
      .replace(/[^a-z0-9]+/g, '');
  }

  function genreArtworkForFolderName(folderName) {
    const normalized = normalizeGenreArtworkName(folderName);
    const slug = genreArtworkAliases[normalized] || '';

    if (slug === '') {
      return null;
    }

    return Object.freeze({
      slug: slug,
      spriteUrl: genreArtworkSpriteUrl
    });
  }

  function folderItemTitle(item) {
    if (!item || typeof item.querySelector !== 'function') {
      return '';
    }

    const title = item.querySelector('.list-title');
    return title ? String(title.textContent || '').trim() : '';
  }

  function removeCompetingRecordingArtwork(item) {
    if (!item || typeof item.querySelectorAll !== 'function') {
      return;
    }

    item.querySelectorAll('.recording-artwork-image').forEach(image => {
      image.remove();
    });
    item.classList.remove('has-recording-artwork');
    item.dataset.recordingArtworkAttached = 'false';
  }

  function attachGenreArtwork(item) {
    if (!item || !item.classList ||
        !item.classList.contains('recording-folder-item')) {
      return false;
    }

    const artwork = genreArtworkForFolderName(folderItemTitle(item));
    if (!artwork) {
      return false;
    }

    removeCompetingRecordingArtwork(item);

    if (typeof item.querySelector === 'function' &&
        item.querySelector('.recording-genre-artwork-image')) {
      item.classList.add('has-recording-genre-artwork');
      return true;
    }

    const image = document.createElement('span');
    image.className =
      'recording-genre-artwork-image recording-genre-artwork-' + artwork.slug;
    image.setAttribute('aria-hidden', 'true');

    item.dataset.recordingGenreArtworkAttached = 'true';
    item.classList.add('has-recording-genre-artwork');
    item.appendChild(image);
    return true;
  }

  function scanGenreArtwork(root) {
    if (!root) {
      return;
    }

    if (typeof root.closest === 'function') {
      const parentFolder = root.closest('.recording-folder-item');
      if (parentFolder) {
        attachGenreArtwork(parentFolder);
      }
    }

    if (root.nodeType === 1 && typeof root.matches === 'function' &&
        root.matches('.recording-folder-item')) {
      attachGenreArtwork(root);
    }

    if (typeof root.querySelectorAll === 'function') {
      root.querySelectorAll('.recording-folder-item').forEach(item => {
        attachGenreArtwork(item);
      });
    }
  }

  function observeGenreArtwork() {
    if (typeof MutationObserver !== 'function' || !document.body) {
      return null;
    }

    const observer = new MutationObserver(mutations => {
      mutations.forEach(mutation => {
        Array.from(mutation.addedNodes || []).forEach(node => {
          scanGenreArtwork(node);
        });
      });
    });

    observer.observe(document.body, {
      childList: true,
      subtree: true
    });
    return observer;
  }

  installRecordingArtworkStyles();

  const originalCreateServerRecordingItem =
    window.createServerRecordingItem;

  if (typeof originalCreateServerRecordingItem === 'function') {
    window.createServerRecordingItem = function (recording, folderData) {
      const item = originalCreateServerRecordingItem(recording, folderData);
      const fallbackTitle = recording && recording.title
        ? recording.title
        : 'Aufnahme';
      attachRecordingArtwork(
        item,
        recording,
        fallbackTitle,
        false
      );
      return item;
    };
  }

  const originalRenderServerRecordingDetail =
    window.renderServerRecordingDetail;

  if (typeof originalRenderServerRecordingDetail === 'function') {
    window.renderServerRecordingDetail = function (recording, folderData, options) {
      originalRenderServerRecordingDetail(recording, folderData, options);

      const detail = document.querySelector('.recording-detail');
      const fallbackTitle = recording && recording.title
        ? recording.title
        : 'Aufnahme';
      attachRecordingArtwork(
        detail,
        recording,
        fallbackTitle,
        true
      );
    };
  }

  scanGenreArtwork(document);
  const genreArtworkObserver = observeGenreArtwork();

  window.VdrSuiteRecordingArtwork = Object.freeze({
    urlForRecording: recordingArtworkUrl,
    attach: attachRecordingArtwork
  });

  window.VdrSuiteRecordingGenreArtwork = Object.freeze({
    normalizeName: normalizeGenreArtworkName,
    forFolderName: genreArtworkForFolderName,
    attach: attachGenreArtwork,
    scan: scanGenreArtwork,
    observer: genreArtworkObserver
  });
})();
