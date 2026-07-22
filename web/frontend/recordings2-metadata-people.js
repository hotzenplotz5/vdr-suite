// Recordings 2 people metadata renderer and persistent recording-person search UI.
(function (global) {
  'use strict';

  const common = global.VdrSuiteRecordings2MetadataCommon;
  if (!common) {
    console.error('VDR-Suite Recordings 2 metadata common runtime is missing');
    return;
  }

  const {
    clientApi,
    first,
    formatRecordingDuration,
    formatRecordingSize,
    formatRecordingStart,
    isPublicMetadataImageUrl,
    node,
    recordingPosterUrl,
    recordingSubtitle,
    recordingSummary,
    recordingTitle,
    roleLabel,
    status,
    text
  } = common;

  function createRecordingPoster(recording) {
    const poster = node('span', 'recordings2-person-poster');
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

  function createRecordingResult(recording) {
    const entry = node('article', 'recordings2-person-entry');
    const button = node('button', 'recordings2-person-recording');
    button.type = 'button';
    button.setAttribute('aria-expanded', 'false');
    button.appendChild(createRecordingPoster(recording));

    const copy = node('span', 'recordings2-person-recording-copy');
    copy.appendChild(node('span', 'recordings2-person-recording-title', recordingTitle(recording)));
    const subtitle = recordingSubtitle(recording);
    if (subtitle) copy.appendChild(node('span', 'recordings2-person-recording-subtitle', subtitle));
    copy.appendChild(node(
      'span',
      'recordings2-person-recording-meta',
      formatRecordingStart(first(recording, ['startTime'], '')) + ' · ' +
        formatRecordingDuration(first(recording, ['durationSeconds'], 0))
    ));
    button.appendChild(copy);
    button.appendChild(node('span', '', '›'));

    const details = node('section', 'recordings2-person-recording-details');
    details.hidden = true;
    details.appendChild(node('p', '', recordingSummary(recording) || 'Keine Beschreibung vorhanden.'));
    details.appendChild(node(
      'p',
      '',
      'Größe: ' + formatRecordingSize(first(recording, ['sizeMb'], 0)) +
        ' · Pfad: ' + text(first(recording, ['path'], ''))
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
    const api = clientApi();
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
      container.appendChild(node('p', '', 'Gefundene Aufnahmen: ' + matches.length));
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

  function render(panel, value, backendId) {
    panel.replaceChildren();
    const people = Array.isArray(value.people) ? value.people : [];
    if (!people.length) {
      panel.appendChild(status('Keine Schauspielerdaten verfügbar.', false));
      return;
    }

    const grid = node('div', 'recordings2-metadata-cast');
    people.forEach(function (person) {
      const entry = node('article', 'recordings2-person-entry');
      const card = node('button', 'recordings2-person-card');
      card.type = 'button';
      card.title = person.name + ' in vorhandenen Aufnahmen suchen';

      if (person.image && person.image.available === true && isPublicMetadataImageUrl(person.image.url)) {
        const image = document.createElement('img');
        image.className = 'recordings2-person-image';
        image.src = person.image.url;
        image.alt = person.name;
        image.loading = 'lazy';
        card.appendChild(image);
      } else {
        card.appendChild(node('span', 'recordings2-person-placeholder', '•'));
      }

      const copy = node('span', 'recordings2-person-copy');
      copy.appendChild(node('span', 'recordings2-person-name', person.name || 'Unbekannte Person'));
      if (text(person.characterName)) {
        copy.appendChild(node('span', 'recordings2-person-character', person.characterName));
      }
      copy.appendChild(node('span', 'recordings2-person-role', roleLabel(person.role)));
      card.appendChild(copy);

      const results = node('section', 'recordings2-person-results');
      results.hidden = true;
      results.setAttribute('aria-live', 'polite');
      card.addEventListener('click', function () {
        searchRecordings(results, person, backendId);
      });

      entry.appendChild(card);
      entry.appendChild(results);
      grid.appendChild(entry);
    });
    panel.appendChild(grid);
  }

  global.VdrSuiteRecordings2MetadataPeople = Object.freeze({
    render
  });
}(window));
