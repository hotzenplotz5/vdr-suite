(function () {
  'use strict';

  const artworkUrlPattern =
    /^\/recording-artwork\/(?:[A-Za-z0-9._~-]|%[0-9A-Fa-f]{2})+\/[0-9A-Fa-f]{32}$/;

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
