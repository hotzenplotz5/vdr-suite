// Recordings 2 scraper metadata renderer.
(function (global) {
  'use strict';

  const common = global.VdrSuiteRecordings2MetadataCommon;
  if (!common) {
    console.error('VDR-Suite Recordings 2 metadata common runtime is missing');
    return;
  }

  const {
    formatDate,
    mediaTypeLabel,
    node,
    text
  } = common;

  function appendFact(container, label, value) {
    const normalized = text(value);
    if (!normalized) return;
    const fact = node('div', 'recordings2-metadata-fact');
    fact.appendChild(node('span', '', label));
    fact.appendChild(node('strong', '', normalized));
    container.appendChild(fact);
  }

  function appendCopy(container, title, value) {
    const normalized = text(value);
    if (!normalized) return;
    const box = node('section', 'recordings2-metadata-copy');
    box.appendChild(node('h4', '', title));
    box.appendChild(node('p', '', normalized));
    container.appendChild(box);
  }

  function render(panel, value) {
    panel.replaceChildren();
    const facts = node('div', 'recordings2-metadata-facts');
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

    const badges = node('div', 'recordings2-metadata-badges');
    [].concat(value.genres || [], value.productionCountries || [], value.networks || []).forEach(function (entry) {
      if (text(entry)) badges.appendChild(node('span', 'recordings2-metadata-badge', entry));
    });
    if (badges.children.length) panel.appendChild(badges);

    appendCopy(panel, 'Tagline', value.tagline);
    appendCopy(panel, 'TVScraper-Beschreibung', value.overview);
  }

  global.VdrSuiteRecordings2MetadataScraper = Object.freeze({
    render
  });
}(window));
