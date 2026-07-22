// Recordings 2 native metadata image gallery renderer.
(function (global) {
  'use strict';

  const common = global.VdrSuiteRecordings2MetadataCommon;
  if (!common) {
    console.error('VDR-Suite Recordings 2 metadata common runtime is missing');
    return;
  }

  const {
    isPublicMetadataImageUrl,
    node,
    orientationLabel,
    recordingTitle,
    status
  } = common;

  function render(panel, value, recording) {
    panel.replaceChildren();
    const images = [];

    if (value.preferredArtwork && value.preferredArtwork.available === true &&
        isPublicMetadataImageUrl(value.preferredArtwork.url)) {
      images.push({
        orientation: 'portrait',
        label: 'Bevorzugtes Bild',
        image: value.preferredArtwork
      });
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

    const gallery = node('div', 'recordings2-metadata-gallery');
    images.forEach(function (entry) {
      const figure = node(
        'figure',
        'recordings2-metadata-image' + (entry.orientation === 'portrait' ? ' portrait' : '')
      );
      const image = document.createElement('img');
      image.src = entry.image.url;
      image.alt = entry.label + ' zu ' + recordingTitle(recording);
      image.loading = 'lazy';
      figure.appendChild(image);
      figure.appendChild(node('figcaption', '', entry.label));
      gallery.appendChild(figure);
    });
    panel.appendChild(gallery);
  }

  global.VdrSuiteRecordings2MetadataImages = Object.freeze({
    render
  });
}(window));
