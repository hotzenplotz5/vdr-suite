// Person cards and recording-result owner for Recordings 2 metadata details.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  if (!shared) {
    console.error('VDR-Suite Recordings 2 shared runtime is unavailable for person views');
    return;
  }

  const ROLE_LABELS = Object.freeze({
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

  function roleLabel(value) {
    return ROLE_LABELS[shared.text(value).toLowerCase()] || ROLE_LABELS.unknown;
  }

  function isPublicRecordingImageUrl(value) {
    const url = shared.text(value);
    return url.startsWith('/recording-artwork/') ||
      url.startsWith('/api/vdr/recordings/artwork') ||
      url.startsWith('/api/recordings/artwork') ||
      url.startsWith('/api/vdr/recordings/metadata/image?') ||
      url.startsWith('/api/recordings/metadata/image?');
  }

  function status(message, error) {
    const box = shared.node('p', 'recordings2-metadata-status' + (error ? ' error' : ''), message);
    box.setAttribute('role', error ? 'alert' : 'status');
    return box;
  }

  function createRecordingPoster(recording) {
    const poster = shared.node('span', 'recordings2-person-poster');
    const url = shared.recordingPosterUrl(recording);
    if (!isPublicRecordingImageUrl(url)) {
      poster.textContent = '▶';
      return poster;
    }
    const image = document.createElement('img');
    image.src = url;
    image.alt = 'Poster zu ' + shared.recordingTitle(recording);
    image.loading = 'lazy';
    image.decoding = 'async';
    image.addEventListener('error', function () {
      image.remove();
      poster.textContent = '▶';
    }, {once: true});
    poster.appendChild(image);
    return poster;
  }

  function createRecordingResult(recording) {
    const entry = shared.node('article', 'recordings2-person-entry');
    const button = shared.node('button', 'recordings2-person-recording');
    button.type = 'button';
    button.setAttribute('aria-expanded', 'false');
    button.appendChild(createRecordingPoster(recording));

    const copy = shared.node('span', 'recordings2-person-recording-copy');
    copy.appendChild(shared.node('span', 'recordings2-person-recording-title', shared.recordingTitle(recording)));
    const subtitle = shared.recordingSubtitle(recording);
    if (subtitle) copy.appendChild(shared.node('span', 'recordings2-person-recording-subtitle', subtitle));
    copy.appendChild(shared.node(
      'span',
      'recordings2-person-recording-meta',
      shared.formatStart(shared.first(recording, ['startTime'], '')) + ' · ' +
        shared.formatDuration(shared.first(recording, ['durationSeconds'], 0))
    ));
    button.appendChild(copy);
    button.appendChild(shared.node('span', '', '›'));

    const details = shared.node('section', 'recordings2-person-recording-details');
    details.hidden = true;
    details.appendChild(shared.node('p', '', shared.recordingSummary(recording) || 'Keine Beschreibung vorhanden.'));
    details.appendChild(shared.node(
      'p',
      '',
      'Größe: ' + shared.formatSize(shared.first(recording, ['sizeMb'], 0)) +
        ' · Pfad: ' + shared.text(shared.first(recording, ['path'], ''))
    ));
    button.addEventListener('click', function () {
      details.hidden = !details.hidden;
      button.setAttribute('aria-expanded', details.hidden ? 'false' : 'true');
    });
    entry.appendChild(button);
    entry.appendChild(details);
    return entry;
  }

  function searchRecordings(container, person, backendId) {
    const api = shared.clientApi();
    container.hidden = false;
    container.replaceChildren(status('Suche in vorhandenen Aufnahmen …', false));
    if (!api || typeof api.fetchClientRecordingPersons !== 'function') {
      container.replaceChildren(status('Aufnahmensuche ist nicht verfügbar.', true));
      return;
    }

    api.fetchClientRecordingPersons({
      backendId: backendId,
      query: {name: person.name, limit: 20},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function (result) {
      const matches = result && Array.isArray(result.matches) ? result.matches : [];
      container.replaceChildren();
      if (!matches.length) {
        container.appendChild(status('Keine vorhandene Aufnahme mit dieser Person gefunden.', false));
        return;
      }
      container.appendChild(shared.node('p', '', 'Gefundene Aufnahmen: ' + matches.length));
      matches.forEach(function (match) {
        const recording = match && match.recording ? match.recording : null;
        if (recording) container.appendChild(createRecordingResult(recording));
      });
    }).catch(function (error) {
      container.replaceChildren(status(
        'Aufnahmensuche fehlgeschlagen: ' + String(error && error.message ? error.message : error),
        true
      ));
    });
  }

  function personPlaceholder() {
    return shared.node('span', 'recordings2-person-placeholder', '•');
  }

  function renderCast(panel, value, backendId, isPublicMetadataImageUrl) {
    panel.replaceChildren();
    const people = Array.isArray(value.people) ? value.people : [];
    if (!people.length) {
      panel.appendChild(status('Keine Schauspielerdaten verfügbar.', false));
      return;
    }

    const grid = shared.node('div', 'recordings2-metadata-cast');
    people.forEach(function (person, index) {
      const entry = shared.node('article', 'recordings2-person-entry');
      const card = shared.node('button', 'recordings2-person-card');
      card.type = 'button';
      card.title = person.name + ' in vorhandenen Aufnahmen suchen';
      if (person.image && person.image.available === true &&
          isPublicMetadataImageUrl(person.image.url)) {
        const image = document.createElement('img');
        image.className = 'recordings2-person-image';
        image.src = person.image.url;
        image.alt = person.name;
        image.decoding = 'async';
        image.loading = index < 6 ? 'eager' : 'lazy';
        if (index < 2) image.fetchPriority = 'high';
        image.addEventListener('error', function () {
          image.replaceWith(personPlaceholder());
        }, {once: true});
        card.appendChild(image);
      } else {
        card.appendChild(personPlaceholder());
      }

      const copy = shared.node('span', 'recordings2-person-copy');
      copy.appendChild(shared.node('span', 'recordings2-person-name', person.name || 'Unbekannte Person'));
      if (shared.text(person.characterName)) {
        copy.appendChild(shared.node('span', 'recordings2-person-character', person.characterName));
      }
      copy.appendChild(shared.node('span', 'recordings2-person-role', roleLabel(person.role)));
      card.appendChild(copy);

      const results = shared.node('section', 'recordings2-person-results');
      results.hidden = true;
      results.setAttribute('aria-live', 'polite');
      card.addEventListener('click', function () { searchRecordings(results, person, backendId); });
      entry.appendChild(card);
      entry.appendChild(results);
      grid.appendChild(entry);
    });
    panel.appendChild(grid);
  }

  global.VdrSuiteRecordings2PersonSearchView = Object.freeze({
    renderCast,
    roleLabel,
    __test: Object.freeze({
      isPublicRecordingImageUrl: isPublicRecordingImageUrl,
      createRecordingPoster: createRecordingPoster
    })
  });
}(window));