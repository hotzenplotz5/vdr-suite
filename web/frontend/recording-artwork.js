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

  let recordingFolderLoader = null;
  let recordingFolderArtworkObserver = null;
  let recordingFolderArtworkEpoch = 0;

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
      '.recording-folder-item .recording-artwork-image,',
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
      '.recording-folder-item.has-recording-artwork,',
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
      '.recording-folder-item.has-recording-artwork::before,',
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
      '.recording-folder-list > .recording-folder-item,',
      '.recording-folder-list > article.recording-list-item {',
      '  box-sizing: border-box;',
      '  min-width: 0;',
      '  animation: vdr-suite-recording-card-reveal 180ms ease-out both;',
      '  transition:',
      '    transform 160ms ease,',
      '    border-color 160ms ease,',
      '    background-color 160ms ease,',
      '    box-shadow 160ms ease;',
      '  -webkit-tap-highlight-color: transparent;',
      '}',
      '.recording-folder-list > .recording-folder-item:focus-visible,',
      '.recording-folder-list > article.recording-list-item:focus-visible {',
      '  border-color: rgba(125, 211, 252, 0.78);',
      '  background-color: rgba(15, 23, 42, 0.98);',
      '  box-shadow: 0 0 0 3px rgba(56, 189, 248, 0.2);',
      '  outline: none;',
      '}',
      '@media (hover: hover) and (pointer: fine) {',
      '  .recording-folder-list > .recording-folder-item:hover,',
      '  .recording-folder-list > article.recording-list-item:hover {',
      '    z-index: 2;',
      '    transform: translateY(-0.22rem);',
      '    border-color: rgba(125, 211, 252, 0.78);',
      '    background-color: rgba(15, 23, 42, 0.98);',
      '    box-shadow:',
      '      0 1rem 2.2rem rgba(2, 6, 23, 0.48),',
      '      0 0 0 1px rgba(56, 189, 248, 0.18);',
      '  }',
      '  .recording-folder-list > .recording-folder-item:hover .recording-artwork-image,',
      '  .recording-folder-list > .recording-folder-item:hover .recording-genre-artwork-image,',
      '  .recording-folder-list > article.recording-list-item:hover .recording-artwork-image {',
      '    border-color: rgba(125, 211, 252, 0.82);',
      '    box-shadow: 0 0.9rem 1.8rem rgba(2, 6, 23, 0.5);',
      '  }',
      '}',
      '@media (hover: none), (pointer: coarse) {',
      '  .recording-folder-list > .recording-folder-item:active,',
      '  .recording-folder-list > article.recording-list-item:active {',
      '    transform: scale(0.985);',
      '    border-color: rgba(125, 211, 252, 0.78);',
      '    background-color: rgba(15, 23, 42, 0.98);',
      '    box-shadow: 0 0.45rem 1.2rem rgba(2, 6, 23, 0.38);',
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
      '    padding-top: 0.55rem;',
      '    padding-bottom: 0.55rem;',
      '    will-change: transform;',
      '  }',
      '  .recording-folder-list > .recording-folder-item.has-recording-artwork {',
      '    min-height: 13.7rem;',
      '    padding-left: 10.45rem !important;',
      '  }',
      '  .recording-folder-list > .recording-folder-item.has-recording-genre-artwork {',
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
      '  .recording-folder-list > .recording-folder-item,',
      '  .recording-folder-list > article.recording-list-item {',
      '    animation: none !important;',
      '    transition: none !important;',
      '  }',
      '}',
      '@media (max-width: 760px) {',
      '  .module-nav {',
      '    flex-wrap: nowrap;',
      '    overflow-x: auto;',
      '    overflow-y: hidden;',
      '    padding: 0.1rem 0 0.45rem;',
      '    scroll-snap-type: x proximity;',
      '    scrollbar-width: thin;',
      '  }',
      '  .module-nav .module-tab {',
      '    flex: 0 0 auto;',
      '    scroll-snap-align: start;',
      '  }',
      '  .recording-list-item .recording-artwork-image,',
      '  .recording-folder-item .recording-artwork-image,',
      '  .recording-folder-item .recording-genre-artwork-image {',
      '    left: 0.65rem;',
      '    top: 0.62rem;',
      '    width: 3.85rem;',
      '    height: 5.5rem;',
      '    border-radius: 0.7rem;',
      '  }',
      '  .recording-folder-item.has-recording-artwork,',
      '  .recording-folder-item.has-recording-genre-artwork {',
      '    min-height: 6.7rem;',
      '    padding-left: 5.25rem !important;',
      '  }',
      '  .recording-detail {',
      '    box-sizing: border-box;',
      '    min-height: 0;',
      '    padding: 10.35rem 0.85rem 0.9rem !important;',
      '  }',
      '  .recording-detail::before,',
      '  .recording-detail .recording-artwork-image {',
      '    left: 50%;',
      '    top: 0.82rem;',
      '    width: 5.8rem;',
      '    height: 8.35rem;',
      '    border-radius: 0.72rem;',
      '    transform: translateX(-50%);',
      '  }',
      '  .recording-detail > h3 {',
      '    margin: 0 0 0.45rem;',
      '    text-align: center;',
      '    font-size: 1.24rem;',
      '    line-height: 1.18;',
      '  }',
      '  .recording-detail > p {',
      '    line-height: 1.34;',
      '  }',
      '  .recording-technical-details,',
      '  .recording-action-panel,',
      '  .recording-action-buttons,',
      '  .recording-action-result {',
      '    box-sizing: border-box;',
      '    width: 100%;',
      '    min-width: 0;',
      '  }',
      '  .recording-technical-details {',
      '    margin-top: 0.65rem;',
      '    padding: 0.62rem 0.72rem;',
      '    border: 1px solid rgba(148, 163, 184, 0.22);',
      '    border-radius: 0.72rem;',
      '    background: rgba(15, 23, 42, 0.62);',
      '  }',
      '  .recording-action-panel {',
      '    margin-top: 0.8rem;',
      '    padding: 0.2rem 0.7rem 0.75rem;',
      '    overflow: hidden;',
      '    border: 1px solid rgba(96, 165, 250, 0.34);',
      '    border-radius: 0.9rem;',
      '    background: rgba(15, 23, 42, 0.72);',
      '  }',
      '  .recording-action-panel > summary {',
      '    cursor: pointer;',
      '    padding: 0.72rem 0.12rem;',
      '    color: #f8fafc;',
      '    font-weight: 850;',
      '    line-height: 1.2;',
      '  }',
      '  .recording-action-panel[open] > summary {',
      '    margin-bottom: 0.6rem;',
      '    border-bottom: 1px solid rgba(148, 163, 184, 0.18);',
      '  }',
      '  .recording-action-panel > p {',
      '    display: none;',
      '  }',
      '  .recording-action-buttons {',
      '    display: grid !important;',
      '    grid-template-columns: minmax(0, 1fr) !important;',
      '    align-items: stretch;',
      '    gap: 0.58rem !important;',
      '  }',
      '  .recording-action-buttons > * {',
      '    box-sizing: border-box;',
      '    width: 100%;',
      '    min-width: 0;',
      '    margin: 0;',
      '  }',
      '  .recording-action-buttons button,',
      '  .recording-move-folder-browser button,',
      '  .recording-detail > button:last-child {',
      '    box-sizing: border-box;',
      '    width: 100%;',
      '    min-height: 2.75rem;',
      '    border-radius: 0.72rem;',
      '    padding: 0.65rem 0.8rem;',
      '    white-space: normal;',
      '    line-height: 1.2;',
      '  }',
      '  .recording-action-buttons details {',
      '    overflow: hidden;',
      '    border: 1px solid rgba(148, 163, 184, 0.22);',
      '    border-radius: 0.78rem;',
      '    background: rgba(2, 6, 23, 0.7);',
      '  }',
      '  .recording-action-buttons details > summary {',
      '    cursor: pointer;',
      '    padding: 0.72rem 0.78rem;',
      '    color: #e2e8f0;',
      '    font-weight: 800;',
      '    line-height: 1.2;',
      '  }',
      '  .recording-action-buttons details[open] > summary {',
      '    border-bottom: 1px solid rgba(148, 163, 184, 0.18);',
      '  }',
      '  .recording-move-editor-body,',
      '  .recording-trash-editor-body {',
      '    box-sizing: border-box;',
      '    width: 100%;',
      '    margin: 0;',
      '    padding: 0.78rem !important;',
      '    border: 0;',
      '    border-radius: 0;',
      '  }',
      '  .recording-move-editor-body label,',
      '  .recording-trash-editor-body label {',
      '    display: grid;',
      '    gap: 0.3rem;',
      '    width: 100%;',
      '    min-width: 0;',
      '  }',
      '  .recording-move-editor-body input,',
      '  .recording-trash-editor-body input {',
      '    box-sizing: border-box;',
      '    width: 100%;',
      '    min-width: 0;',
      '    min-height: 2.65rem;',
      '    border: 1px solid #475569;',
      '    border-radius: 0.62rem;',
      '    padding: 0.55rem 0.65rem;',
      '    background: #111827;',
      '    color: #f8fafc;',
      '    font: inherit;',
      '  }',
      '  .recording-move-validation-status {',
      '    padding: 0.62rem 0.68rem;',
      '    border: 1px solid rgba(96, 165, 250, 0.24);',
      '    border-radius: 0.68rem;',
      '    background: rgba(30, 41, 59, 0.66);',
      '    line-height: 1.3;',
      '  }',
      '  .recording-move-folder-browser {',
      '    display: grid;',
      '    gap: 0.48rem;',
      '    width: 100%;',
      '    min-width: 0;',
      '    margin: 0.6rem 0;',
      '  }',
      '  .recording-action-result:not(:empty) {',
      '    margin-top: 0.65rem;',
      '    padding: 0.68rem 0.72rem;',
      '    border: 1px solid rgba(56, 189, 248, 0.22);',
      '    border-radius: 0.72rem;',
      '    background: rgba(2, 6, 23, 0.72);',
      '  }',
      '  .recording-action-result h4 {',
      '    margin: 0 0 0.35rem;',
      '    color: #f8fafc;',
      '  }',
      '  .recording-action-result p {',
      '    font-size: 0.88rem;',
      '    line-height: 1.28;',
      '  }',
      '  .recording-detail > button:last-child {',
      '    margin-top: 0.78rem;',
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
      return null;
    }

    const url = recordingArtworkUrl(recording);
    if (url === '') {
      return null;
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
      container.dataset.recordingArtworkState = 'ready';
    });

    image.addEventListener('error', () => {
      container.classList.remove('has-recording-artwork');
      container.dataset.recordingArtworkAttached = 'false';
      container.dataset.recordingArtworkState = 'error';
      image.remove();
    });

    container.appendChild(image);
    return image;
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

  function recordingArtworkListFromResponse(data, key) {
    if (Array.isArray(data)) {
      return data;
    }

    if (data && Array.isArray(data[key])) {
      return data[key];
    }

    if (data && Array.isArray(data.items)) {
      return data.items;
    }

    return [];
  }

  function recordingArtworkFirstValue(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined &&
          object[key] !== null && object[key] !== '') {
        return object[key];
      }
    }

    return fallback;
  }

  function recordingArtworkFolderPath(folder) {
    return String(recordingArtworkFirstValue(
      folder,
      ['path', 'folderPath', 'name', 'title'],
      ''
    )).trim();
  }

  function recordingArtworkFolderCount(folder) {
    const value = Number(recordingArtworkFirstValue(
      folder,
      ['recordingCount', 'count', 'total'],
      0
    ));

    return Number.isFinite(value) ? value : 0;
  }

  function recordingArtworkElementConnected(element) {
    return Boolean(element) &&
      (element.isConnected === undefined || element.isConnected === true);
  }

  function recordingArtworkFolderItems() {
    if (!document || typeof document.querySelectorAll !== 'function') {
      return [];
    }

    return Array.from(document.querySelectorAll('.recording-folder-item'));
  }

  function recordingArtworkSingleRecordingFromFolderData(folderData) {
    const recordings = recordingArtworkListFromResponse(
      folderData,
      'recordings'
    );
    const recordingCount = Number(
      folderData && folderData.recordingCount
    ) || recordings.length;

    return recordings.length === 1 && recordingCount === 1
      ? recordings[0]
      : null;
  }

  function requestSingleFolderArtwork(item, folder, epoch) {
    if (!recordingFolderLoader || !recordingArtworkElementConnected(item)) {
      return;
    }

    const folderPath = recordingArtworkFolderPath(folder);
    if (folderPath === '') {
      return;
    }

    item.dataset.recordingFolderArtworkState = 'loading';

    Promise.resolve(recordingFolderLoader(folderPath, 0))
      .then(folderData => {
        if (epoch !== recordingFolderArtworkEpoch ||
            !recordingArtworkElementConnected(item) ||
            item.dataset.recordingFolderPath !== folderPath) {
          return;
        }

        const recording = recordingArtworkSingleRecordingFromFolderData(
          folderData && typeof folderData === 'object' ? folderData : {}
        );

        if (!recording) {
          item.dataset.recordingFolderArtworkState = 'empty';
          return;
        }

        const image = attachRecordingArtwork(
          item,
          recording,
          folderItemTitle(item),
          false
        );

        if (!image) {
          item.dataset.recordingFolderArtworkState = 'unavailable';
          return;
        }

        item.dataset.recordingFolderArtworkState = 'attached';
        image.addEventListener('load', () => {
          item.dataset.recordingFolderArtworkState = 'ready';
        });
        image.addEventListener('error', () => {
          item.dataset.recordingFolderArtworkState = 'error';
        });
      })
      .catch(() => {
        if (epoch === recordingFolderArtworkEpoch &&
            recordingArtworkElementConnected(item)) {
          item.dataset.recordingFolderArtworkState = 'error';
        }
      });
  }

  function observeSingleFolderArtwork(item, folder, epoch) {
    item.__vdrSuiteFolderArtwork = {
      folder: folder,
      epoch: epoch
    };

    if (typeof IntersectionObserver !== 'function') {
      requestSingleFolderArtwork(item, folder, epoch);
      return;
    }

    if (!recordingFolderArtworkObserver) {
      recordingFolderArtworkObserver = new IntersectionObserver(entries => {
        entries.forEach(entry => {
          if (!entry.isIntersecting && entry.intersectionRatio <= 0) {
            return;
          }

          const target = entry.target;
          const request = target && target.__vdrSuiteFolderArtwork;
          recordingFolderArtworkObserver.unobserve(target);

          if (request) {
            requestSingleFolderArtwork(
              target,
              request.folder,
              request.epoch
            );
          }
        });
      }, {
        rootMargin: '320px 0px',
        threshold: 0.01
      });
    }

    recordingFolderArtworkObserver.observe(item);
  }

  function decorateSingleRecordingFolders(folderData) {
    recordingFolderArtworkEpoch += 1;
    const epoch = recordingFolderArtworkEpoch;

    if (recordingFolderArtworkObserver &&
        typeof recordingFolderArtworkObserver.disconnect === 'function') {
      recordingFolderArtworkObserver.disconnect();
      recordingFolderArtworkObserver = null;
    }

    if (!recordingFolderLoader) {
      return;
    }

    const folders = recordingArtworkListFromResponse(folderData, 'folders');
    const items = recordingArtworkFolderItems();

    folders.forEach((folder, index) => {
      const item = items[index];
      if (!item || recordingArtworkFolderCount(folder) !== 1) {
        return;
      }

      const title = folderItemTitle(item);
      if (genreArtworkForFolderName(title)) {
        return;
      }

      const folderPath = recordingArtworkFolderPath(folder);
      if (folderPath === '') {
        return;
      }

      item.dataset.recordingFolderPath = folderPath;

      if (item.dataset.recordingArtworkAttached === 'true' ||
          item.dataset.recordingFolderArtworkState === 'loading') {
        return;
      }

      const embeddedRecording = recordingArtworkFirstValue(
        folder,
        ['recording', 'representativeRecording', 'previewRecording'],
        null
      );

      if (embeddedRecording && typeof embeddedRecording === 'object') {
        attachRecordingArtwork(item, embeddedRecording, title, false);
        return;
      }

      observeSingleFolderArtwork(item, folder, epoch);
    });
  }

  installRecordingArtworkStyles();

  const originalConfigureRecordingBrowserFolderLoader =
    window.configureRecordingBrowserFolderLoader;

  if (typeof originalConfigureRecordingBrowserFolderLoader === 'function') {
    window.configureRecordingBrowserFolderLoader = function (loader) {
      recordingFolderLoader = typeof loader === 'function' ? loader : null;
      return originalConfigureRecordingBrowserFolderLoader.apply(
        this,
        arguments
      );
    };
  }

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
      const result = originalRenderServerRecordingDetail(
        recording,
        folderData,
        options
      );

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
      return result;
    };
  }

  const originalRenderServerRecordingFolder =
    window.renderServerRecordingFolder;

  if (typeof originalRenderServerRecordingFolder === 'function') {
    window.renderServerRecordingFolder = function (folderData) {
      const result = originalRenderServerRecordingFolder.apply(
        this,
        arguments
      );
      decorateSingleRecordingFolders(
        folderData && typeof folderData === 'object' ? folderData : {}
      );
      return result;
    };
  }

  scanGenreArtwork(document);
  const genreArtworkObserver = observeGenreArtwork();

  window.VdrSuiteRecordingArtwork = Object.freeze({
    urlForRecording: recordingArtworkUrl,
    attach: attachRecordingArtwork,
    decorateFolderList: decorateSingleRecordingFolders,
    setFolderLoader(loader) {
      recordingFolderLoader = typeof loader === 'function' ? loader : null;
    }
  });

  window.VdrSuiteRecordingGenreArtwork = Object.freeze({
    normalizeName: normalizeGenreArtworkName,
    forFolderName: genreArtworkForFolderName,
    attach: attachGenreArtwork,
    scan: scanGenreArtwork,
    observer: genreArtworkObserver
  });
})();
