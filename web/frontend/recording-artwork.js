(function () {
  'use strict';

  const artworkUrlPattern =
    /^\/recording-artwork\/(?:[A-Za-z0-9._~-]|%[0-9A-Fa-f]{2})+\/[0-9A-Fa-f]{32}$/;

  function installRecordingArtworkStyles() {
    if (document.getElementById('vdr-suite-recording-artwork-styles')) {
      return;
    }

    const style = document.createElement('style');
    style.id = 'vdr-suite-recording-artwork-styles';
    style.textContent = [
      '.recording-artwork-image {',
      '  position: absolute;',
      '  z-index: 1;',
      '  display: block;',
      '  object-fit: cover;',
      '  object-position: center;',
      '  pointer-events: none;',
      '  border: 1px solid rgba(125, 211, 252, 0.38);',
      '  border-radius: 0.82rem;',
      '  background: rgba(2, 6, 23, 0.92);',
      '  box-shadow: 0 0.65rem 1.4rem rgba(2, 6, 23, 0.35);',
      '}',
      '.recording-list-item .recording-artwork-image {',
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
      '.recording-list-item.has-recording-artwork::before,',
      '.recording-detail.has-recording-artwork::before {',
      '  opacity: 0;',
      '}',
      '@media (max-width: 760px) {',
      '  .recording-list-item .recording-artwork-image {',
      '    left: 0.65rem;',
      '    top: 0.62rem;',
      '    width: 3.85rem;',
      '    height: 5.5rem;',
      '    border-radius: 0.7rem;',
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

  window.VdrSuiteRecordingArtwork = Object.freeze({
    urlForRecording: recordingArtworkUrl,
    attach: attachRecordingArtwork
  });
})();
