// Full-page hero presentation and local related-recording rail for Recordings 2 details.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  if (!shared) {
    console.error('VDR-Suite Recordings 2 shared runtime is unavailable for hero details');
    return;
  }

  const STYLE_ID = 'vdr-suite-recordings2-hero-detail-style';

  function installStyles() {
    if (document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = `
.recordings2-detail.recordings2-hero-page{position:fixed;inset:0;z-index:1200;overflow-x:hidden;overflow-y:auto;padding:0 0 3rem;background:#090b0f;color:#f8fafc;overscroll-behavior:contain}
.recordings2-hero-page>.recordings2-header{position:absolute;inset:0 0 auto 0;z-index:10;display:block;margin:0;padding:1.25rem clamp(1rem,3vw,3rem);border:0;background:linear-gradient(180deg,rgba(4,6,10,.82),rgba(4,6,10,.24),transparent);pointer-events:none}
.recordings2-hero-page>.recordings2-header .recordings2-heading{display:none}
.recordings2-hero-page>.recordings2-header .recordings2-toolbar{display:flex;gap:.55rem;pointer-events:auto}
.recordings2-hero-page>.recordings2-header .recordings2-toolbar button{min-height:2.65rem;padding:.55rem .9rem;border:1px solid rgba(255,255,255,.2);border-radius:999px;background:rgba(10,13,18,.58);color:#f8fafc;box-shadow:0 .5rem 2rem rgba(0,0,0,.18);backdrop-filter:blur(16px)}
.recordings2-hero-page>.recordings2-header .recordings2-toolbar button:not(.recordings2-primary){display:none}
.recordings2-hero-mode-back{position:fixed;top:1.25rem;left:clamp(1rem,3vw,3rem);z-index:14;min-height:2.65rem;padding:.55rem .9rem;border:1px solid rgba(255,255,255,.2);border-radius:999px;background:rgba(10,13,18,.76);color:#f8fafc;font-weight:850;box-shadow:0 .5rem 2rem rgba(0,0,0,.25);backdrop-filter:blur(16px)}
.recordings2-hero-mode-back[hidden]{display:none!important}
.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-header,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-header,.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-header,.recordings2-hero-page[data-recordings2-hero-mode="actions"]>.recordings2-header{display:none}
.recordings2-hero-page .recordings2-detail-poster{align-self:end;overflow:hidden;border:1px solid rgba(255,255,255,.12);border-radius:.9rem;background:#15181e;box-shadow:0 1.6rem 4rem rgba(0,0,0,.48)}
.recordings2-hero-page .recordings2-detail-poster img{display:block;width:100%;aspect-ratio:2/3;object-fit:cover}
.recordings2-hero-page .recordings2-detail-copy{display:grid;gap:1rem;max-width:62rem;padding-bottom:.25rem;text-shadow:0 .14rem .9rem rgba(0,0,0,.54)}
.recordings2-hero-page .recordings2-detail-copy h3{margin:0;font-size:clamp(2.7rem,5.6vw,5.8rem);line-height:.94;letter-spacing:-.045em;color:#fff}
.recordings2-hero-page .recordings2-subtitle{margin:0;color:#d1d5db;font-size:1rem}
.recordings2-hero-page .recordings2-detail-description{max-width:54rem;margin:0;color:#e5e7eb;font-size:clamp(1rem,1.35vw,1.18rem);line-height:1.58}
.recordings2-hero-eyebrow{color:#c7ddff;font-size:.76rem;font-weight:900;letter-spacing:.14em;text-transform:uppercase}
.recordings2-hero-facts{display:flex;flex-wrap:wrap;align-items:center;gap:.42rem 1rem;color:#e2e8f0;font-size:.86rem;font-weight:700;text-shadow:0 .14rem .9rem rgba(0,0,0,.54)}
.recordings2-hero-fact{display:inline-flex;align-items:baseline;gap:.32rem;white-space:nowrap}
.recordings2-hero-fact-label{color:#94a3b8;font-size:.69rem;font-weight:850;letter-spacing:.035em;text-transform:uppercase}
.recordings2-hero-fact-value{color:#f8fafc}
.recordings2-hero-fact.rating .recordings2-hero-fact-value{color:#fde68a}
.recordings2-hero-actions{display:flex;flex-wrap:wrap;gap:.62rem;padding-top:.3rem}
.recordings2-hero-actions button{min-height:3rem;padding:.66rem 1.15rem;border:1px solid rgba(255,255,255,.25);border-radius:999px;background:rgba(15,18,23,.64);color:#f8fafc;font-weight:850;box-shadow:0 .35rem 1.25rem rgba(0,0,0,.14);backdrop-filter:blur(14px)}
.recordings2-hero-actions button.primary{border-color:#b9d8ff;background:#b9d8ff;color:#10203a}
.recordings2-hero-actions button:hover,.recordings2-hero-actions button:focus-visible{transform:translateY(-1px);border-color:#b9d8ff;outline:none}
.recordings2-media-check{display:grid;gap:.65rem;max-width:52rem;padding:1rem;border:1px solid rgba(185,216,255,.32);border-radius:.8rem;background:rgba(10,13,18,.68)}
.recordings2-media-check[hidden]{display:none!important}
.recordings2-media-check h4{margin:0;font-size:1.1rem}
.recordings2-media-check select,.recordings2-media-check button{min-height:2.6rem;padding:.5rem .85rem;border:1px solid rgba(185,216,255,.4);border-radius:.5rem;background:#182031;color:#f8fafc}
.recordings2-media-check-status{line-height:1.5;overflow-wrap:anywhere}
.recordings2-media-check-report{max-height:18rem;overflow:auto;white-space:pre-wrap;overflow-wrap:anywhere;font-size:.78rem}
.recordings2-hero-cast{display:flex;gap:1rem;overflow-x:auto;padding:.2rem 0 .4rem;scrollbar-width:thin}
.recordings2-hero-page button.recordings2-hero-person{display:grid;grid-template-columns:2.8rem minmax(0,1fr);gap:.58rem;align-items:center;flex:0 0 auto;min-width:10.5rem;min-height:0;padding:.18rem .35rem .18rem .18rem;border:1px solid rgba(255,255,255,.08);border-radius:.75rem;background:transparent;color:inherit;text-align:left;box-shadow:none;appearance:none}
.recordings2-hero-page button.recordings2-hero-person:hover{border-color:rgba(255,255,255,.18);background:rgba(0,0,0,.2);box-shadow:none}
.recordings2-hero-page button.recordings2-hero-person:focus-visible{border-color:rgba(255,255,255,.42);background:rgba(0,0,0,.26);box-shadow:0 0 0 2px rgba(255,255,255,.12);outline:none}
.recordings2-hero-person img,.recordings2-hero-person-placeholder{width:2.8rem;height:2.8rem;border-radius:50%;object-fit:cover;background:#252a33}
.recordings2-hero-person-placeholder{display:grid;place-items:center;color:#6b7280;font-weight:900}
.recordings2-hero-person-copy{display:grid;gap:.08rem}
.recordings2-hero-person-copy strong{font-size:.8rem}
.recordings2-hero-person-copy span{color:#d1d5db;font-size:.68rem}
.recordings2-hero-person-role{color:#b8bec8!important}
.recordings2-hero-related{position:relative;z-index:2;display:grid;gap:.85rem;margin:1.9rem clamp(1.25rem,5vw,5rem) 0}
.recordings2-hero-related-head{display:flex;align-items:end;justify-content:space-between;gap:1rem}
.recordings2-hero-related-head-copy{display:grid;gap:.2rem}
.recordings2-hero-related-head h4{margin:0;color:#fff;font-size:1.35rem}
.recordings2-hero-related-head span{color:#9ca3af;font-size:.78rem}
.recordings2-hero-page button.recordings2-hero-related-back{min-height:2.45rem;padding:.5rem .8rem;border:1px solid rgba(255,255,255,.18);border-radius:999px;background:transparent;color:#f8fafc;font-weight:800;box-shadow:none;appearance:none}
.recordings2-hero-page button.recordings2-hero-related-back:hover,.recordings2-hero-page button.recordings2-hero-related-back:focus-visible{border-color:rgba(255,255,255,.4);background:rgba(0,0,0,.24);box-shadow:none;outline:none}
.recordings2-hero-related-status{margin:0;padding:.9rem 1rem;border:1px solid rgba(148,163,184,.2);border-radius:.7rem;background:rgba(15,18,23,.44);color:#cbd5e1}
.recordings2-hero-related-rail{display:flex;gap:.85rem;overflow-x:auto;padding:.15rem 0 .7rem;scroll-snap-type:x proximity;scrollbar-width:thin}
.recordings2-hero-page button.recordings2-hero-related-card{display:grid;grid-template-rows:auto auto;gap:.48rem;flex:0 0 clamp(8.5rem,13vw,11.5rem);min-height:0;padding:0;border:0;background:transparent;color:inherit;text-align:left;scroll-snap-align:start;box-shadow:none;appearance:none}
.recordings2-hero-related-poster{display:grid;place-items:center;aspect-ratio:2/3;overflow:hidden;border:1px solid rgba(255,255,255,.14);border-radius:.7rem;background:#171a20;color:#d1d5db;font-size:1.3rem;box-shadow:0 .7rem 1.8rem rgba(0,0,0,.2)}
.recordings2-hero-related-poster img{display:block;width:100%;height:100%;object-fit:cover}
.recordings2-hero-page button.recordings2-hero-related-card:hover,.recordings2-hero-page button.recordings2-hero-related-card:focus-visible{background:transparent;box-shadow:none}
.recordings2-hero-page button.recordings2-hero-related-card:hover .recordings2-hero-related-poster{border-color:rgba(255,255,255,.28);box-shadow:0 .7rem 1.8rem rgba(0,0,0,.28)}
.recordings2-hero-page button.recordings2-hero-related-card:focus-visible .recordings2-hero-related-poster{border-color:rgba(255,255,255,.55);box-shadow:0 0 0 2px rgba(255,255,255,.12)}
.recordings2-hero-page button.recordings2-hero-related-card:focus-visible{outline:none}
.recordings2-hero-related-title{color:#f8fafc;font-size:.8rem;font-weight:850;line-height:1.25}
.recordings2-hero-trailer-section{position:relative;z-index:2;display:grid;gap:.85rem;margin:1.25rem clamp(1.25rem,5vw,5rem) 0}
.recordings2-hero-trailer-head{display:flex;align-items:end;justify-content:space-between;gap:1rem}
.recordings2-hero-trailer-head-copy{display:grid;gap:.2rem}
.recordings2-hero-trailer-head h4{margin:0;color:#fff;font-size:1.35rem}
.recordings2-hero-trailer-head span{color:#9ca3af;font-size:.78rem}
.recordings2-hero-page button.recordings2-hero-trailer-close{min-height:2.45rem;padding:.5rem .8rem;border:1px solid rgba(255,255,255,.18);border-radius:999px;background:transparent;color:#f8fafc;font-weight:800;box-shadow:none;appearance:none}
.recordings2-hero-page button.recordings2-hero-trailer-close:hover,.recordings2-hero-page button.recordings2-hero-trailer-close:focus-visible{border-color:rgba(255,255,255,.4);background:rgba(0,0,0,.24);box-shadow:none;outline:none}
.recordings2-hero-trailer-status{margin:0;padding:.9rem 1rem;border:1px solid rgba(148,163,184,.2);border-radius:.7rem;background:rgba(15,18,23,.44);color:#cbd5e1}
.recordings2-hero-trailer-frame{display:block;width:min(100%,76rem);aspect-ratio:16/9;border:0;border-radius:.8rem;background:#000;box-shadow:0 .7rem 1.8rem rgba(0,0,0,.28)}
.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-metadata-tabs{margin-top:5.5rem}
.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-metadata-panel{margin-top:1rem}
.recordings2-hero-page[data-recordings2-hero-mode="actions"]>.recordings2-actions{position:relative;z-index:2;margin:5.5rem clamp(1.25rem,5vw,5rem) 1rem}
@media(max-width:820px){.recordings2-hero-page .recordings2-detail-copy h3{font-size:clamp(2.2rem,8vw,3.8rem)}}
@media(max-width:620px){.recordings2-hero-page .recordings2-detail-poster{width:min(44vw,10rem)}.recordings2-hero-actions{display:grid;grid-template-columns:1fr}.recordings2-hero-actions button{width:100%}}
`;
    document.head.appendChild(style);
  }

  function text(value) { return shared.text(value); }

  function publicImageUrl(value) {
    const url = text(value);
    if (!url) return '';
    return typeof shared.publicPath === 'function' ? shared.publicPath(url) : url;
  }

  function backdropUrl(metadata) {
    const images = Array.isArray(metadata && metadata.images) ? metadata.images : [];
    const preferred = images.find(function (entry) {
      return entry && entry.image && entry.image.available === true &&
        (entry.orientation === 'landscape' || entry.orientation === 'banner') && text(entry.image.url);
    });
    if (preferred) return publicImageUrl(preferred.image.url);
    const fallback = metadata && metadata.preferredArtwork;
    return fallback && fallback.available === true && text(fallback.url)
      ? publicImageUrl(fallback.url)
      : '';
  }

  function actorList(metadata) {
    return (Array.isArray(metadata && metadata.people) ? metadata.people : [])
      .filter(function (person) {
        return person && text(person.role).toLowerCase() === 'actor' && text(person.name);
      });
  }

  function makeButton(label, action, primary) {
    const button = shared.node('button', primary ? 'primary' : '', label);
    button.type = 'button';
    button.addEventListener('click', action);
    return button;
  }

  function focusSection(root, selector, buttonText) {
    const section = root && root.querySelector ? root.querySelector(selector) : null;
    if (!section) return;
    if (typeof section.scrollIntoView === 'function') {
      section.scrollIntoView({behavior: 'smooth', block: 'start'});
    }
    if (!buttonText || typeof section.querySelectorAll !== 'function') return;
    const button = Array.from(section.querySelectorAll('button')).find(function (candidate) {
      return text(candidate && candidate.textContent).indexOf(buttonText) === 0;
    });
    if (button && typeof button.focus === 'function') button.focus();
  }

  function ensureModeBack(root) {
    let button = root.querySelector('.recordings2-hero-mode-back');
    if (button) return button;
    button = makeButton('← Details', function () { showMode(root, 'detail'); });
    button.className = 'recordings2-hero-mode-back';
    button.hidden = true;
    root.appendChild(button);
    return button;
  }

  function showMode(root, mode, selector, buttonText) {
    if (!root || !root.dataset) return;
    const normalized = ['detail', 'playback', 'marks', 'metadata', 'actions'].includes(mode) ? mode : 'detail';
    root.dataset.recordings2HeroMode = normalized;
    const back = ensureModeBack(root);
    back.hidden = normalized === 'detail';
    if (normalized === 'detail') {
      if (typeof root.scrollTo === 'function') root.scrollTo({top: 0, behavior: 'smooth'});
      return;
    }
    global.setTimeout(function () { focusSection(root, selector, buttonText); }, 0);
  }

  function positiveExternalId(value) {
    const raw = text(value).trim();
    return /^[1-9][0-9]{0,15}$/.test(raw) ? raw : '';
  }

  function trailerIdentity(metadata) {
    if (!metadata || metadata.available !== true) return null;
    const manual = metadata.manualAssignment;
    if (manual && manual.active === true &&
        text(manual.providerId).toLowerCase() === 'tmdb') {
      const externalId = positiveExternalId(manual.externalId);
      const externalNamespace = text(manual.externalNamespace).toLowerCase();
      if (externalId && externalNamespace === 'movie') {
        return {mediaType: 'movie', externalId: externalId};
      }
      if (externalId && externalNamespace === 'tv') {
        return {mediaType: 'series', externalId: externalId};
      }
      return null;
    }

    const provider = text(metadata.provider).toLowerCase();
    if (provider !== 'tvscraper' && provider !== 'recording-cache') return null;
    const externalId = positiveExternalId(metadata.providerId);
    if (!externalId) return null;
    const mediaType = text(metadata.mediaType).toLowerCase();
    if (mediaType === 'movie') return {mediaType: 'movie', externalId: externalId};
    if (mediaType === 'series' || mediaType === 'episode') {
      return {mediaType: 'series', externalId: externalId};
    }
    return null;
  }

  function validYoutubeVideoId(value) {
    return /^[A-Za-z0-9_-]{11}$/.test(text(value));
  }

  function closeTrailer(root) {
    const section = root && root.querySelector
      ? root.querySelector('.recordings2-hero-trailer-section')
      : null;
    if (section && typeof section.remove === 'function') section.remove();
  }

  function placeTrailerAfterRelated(root) {
    if (!root || typeof root.querySelector !== 'function') return false;
    const section = root.querySelector('.recordings2-hero-trailer-section');
    if (!section) return false;
    const related = root.querySelector('.recordings2-hero-related');
    if (related && related.parentNode === root && typeof root.insertBefore === 'function') {
      root.insertBefore(section, related.nextSibling);
    } else {
      root.appendChild(section);
    }
    return true;
  }

  function scrollToTrailer(section) {
    if (section && typeof section.scrollIntoView === 'function') {
      section.scrollIntoView({behavior: 'smooth', block: 'start'});
    }
  }

  function showTrailer(root, backendId, identity) {
    if (!root || !identity) return;
    const existing = root.querySelector('.recordings2-hero-trailer-section');
    if (existing) {
      placeTrailerAfterRelated(root);
      scrollToTrailer(existing);
      return;
    }

    const section = shared.node('section', 'recordings2-hero-trailer-section');
    section.setAttribute('aria-label', 'Trailer');
    const head = shared.node('div', 'recordings2-hero-trailer-head');
    const headCopy = shared.node('div', 'recordings2-hero-trailer-head-copy');
    const title = shared.node('h4', '', 'Trailer');
    const subtitle = shared.node('span', '', 'YouTube · wird erst nach Auswahl geladen');
    headCopy.append(title, subtitle);
    const close = shared.node('button', 'recordings2-hero-trailer-close', 'Trailer schließen');
    close.type = 'button';
    close.addEventListener('click', function () { closeTrailer(root); });
    head.append(headCopy, close);
    const status = shared.node('p', 'recordings2-hero-trailer-status', 'Trailer wird geladen …');
    status.setAttribute('role', 'status');
    section.append(head, status);
    root.appendChild(section);
    placeTrailerAfterRelated(root);
    scrollToTrailer(section);

    const api = shared.clientApi();
    if (!api || typeof api.fetchClientRecordingTrailer !== 'function') {
      status.textContent = 'Trailer-Suche ist nicht verfügbar.';
      return;
    }

    api.fetchClientRecordingTrailer({
      backendId: text(backendId) || 'default',
      query: {
        mediaType: identity.mediaType,
        externalId: identity.externalId
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function (result) {
      if (!section.parentNode) return;
      if (!result || result.available !== true) {
        status.textContent = 'Kein Trailer verfügbar.';
        return;
      }
      if (text(result.provider).toLowerCase() !== 'youtube' ||
          !validYoutubeVideoId(result.videoId)) {
        status.textContent = 'Trailer-Antwort ist ungültig.';
        return;
      }

      title.textContent = text(result.title) || 'Trailer';
      subtitle.textContent = 'YouTube · Trailer';
      const iframe = document.createElement('iframe');
      iframe.className = 'recordings2-hero-trailer-frame';
      iframe.src = 'https://www.youtube-nocookie.com/embed/' +
        text(result.videoId) + '?autoplay=0&rel=0';
      iframe.title = title.textContent;
      iframe.loading = 'lazy';
      iframe.referrerPolicy = 'strict-origin-when-cross-origin';
      iframe.allow = 'encrypted-media; picture-in-picture; fullscreen';
      iframe.allowFullscreen = true;
      iframe.setAttribute('allowfullscreen', '');
      iframe.setAttribute(
        'sandbox',
        'allow-scripts allow-same-origin allow-presentation'
      );
      status.remove();
      section.appendChild(iframe);
      placeTrailerAfterRelated(root);
    }).catch(function (error) {
      if (!section.parentNode) return;
      status.textContent = 'Trailer konnte nicht geladen werden: ' +
        text(error && error.message ? error.message : error);
    });
  }

  function resumePosition(recording) {
    const value = Number(shared.first(recording, ['resumePositionSeconds'], 0));
    return Number.isFinite(value) && value > 0 ? Math.floor(value) : 0;
  }

  function startPlayback(root, recording) {
    const position = resumePosition(recording);
    showMode(root, 'playback', '.recordings2-volume-owner-shell');
    global.setTimeout(function () {
      const owner = root && root.__vdrSuiteRecordingPlaybackOwner;
      if (!owner) return;
      let request = null;
      try {
        if (position > 0 && typeof owner.startAtAbsolute === 'function') {
          request = owner.startAtAbsolute(position);
        } else if (position > 0 && typeof owner.start === 'function' &&
                   typeof owner.seekAbsolute === 'function') {
          request = Promise.resolve(owner.start()).then(function (sessionId) {
            return sessionId ? owner.seekAbsolute(position) : sessionId;
          });
        } else if (typeof owner.start === 'function') {
          request = owner.start();
        }
      } catch (error) {
        return;
      }
      if (request && typeof request.catch === 'function') request.catch(function () {});
    }, 0);
  }


  // M2.3: existing Recording Hero owns the Media Check control and report.
  // The web client never submits a recording path or starts local FFmpeg.
  function mediaCheckClient() {
    const factory = global.VdrSuitePublicV1Client;
    if (!factory || typeof factory.createClient !== 'function') return null;
    return factory.createClient({
      baseUrl: global.location && global.location.origin ? global.location.origin : '',
      credentials: 'same-origin'
    });
  }

  function resolveMediaCheckIdentity(client, recording, backendId) {
    const direct = text(recording && recording.recordingId);
    if (/^rec_[0-9a-f]{32}$/.test(direct)) return Promise.resolve(direct);
    const recordedAt = text(shared.first(recording, ['startTime', 'recordedAt'], ''));
    if (!recordedAt) return Promise.reject(
      new Error('Für diese Aufnahme ist keine eindeutige Aufnahme-ID verfügbar.'));
    const expectedTitle = text(recording && recording.title);
    let cursor = '';
    let matches = [];
    let pageCount = 0;
    function page() {
      if (++pageCount > 20) throw new Error(
        'Die Aufnahmenliste ist zu groß; eindeutige Zuordnung nicht möglich.');
      const query = {backendId: backendId, limit: 100};
      if (cursor) query.cursor = cursor;
      return client.getRecordings({query: query, credentials: 'same-origin',
        cache: 'no-store'}).then(function (response) {
        if (!response || !Array.isArray(response.items) || !response.page)
          throw new Error('Ungültige Antwort der Aufnahme-API.');
        // Legacy Recordings 2 can display a decoded or metadata-enriched
        // title. Preserve a unique timestamp fallback instead of rejecting
        // the same recording merely because its public title differs.
        matches = matches.concat(response.items.filter(function (item) {
          return item && item.backendId === backendId &&
            item.recordedAt === recordedAt;
        }));
        if (matches.length > 100) throw new Error(
          'Zu viele Aufnahmen mit gleichem Aufnahmezeitpunkt.');
        if (response.page.hasMore) {
          if (!/^rc1_[a-f0-9]+$/.test(response.page.nextCursor || '') ||
              response.page.nextCursor === cursor)
            throw new Error('Ungültiger Aufnahme-Seitencursor.');
          cursor = response.page.nextCursor;
          return page();
        }
        const byTitle = expectedTitle ? matches.filter(function (item) {
          return item.title === expectedTitle;
        }) : [];
        const candidates = byTitle.length ? byTitle : matches;
        if (candidates.length !== 1 ||
            !/^rec_[0-9a-f]{32}$/.test(candidates[0].recordingId))
          throw new Error('Kanonische Aufnahme-ID nicht eindeutig gefunden.');
        return candidates[0].recordingId;
      });
    }
    return page();
  }

  function mediaCheckSummary(report) {
    if (!report || typeof report !== 'object')
      return 'Unvollständiger Prüfbericht.';
    const label = {ok: 'In Ordnung', warnings: 'Warnungen',
      errors: 'Fehler', incomplete: 'Prüfung unvollständig'};
    const state = label[report.status] || 'Unbekanntes Ergebnis';
    const segments = Number(report.segments);
    const errors = ['syncErrors', 'transportErrors', 'adaptationErrors',
      'trailingBytes', 'continuityWarnings'].map(function (field) {
      return field + ': ' + Number(report[field] || 0);
    }).join(' · ');
    const broken = (Array.isArray(report.segmentReports)
      ? report.segmentReports : []).filter(function (item) {
        return item && item.status !== 'ok';
      }).map(function (item) { return text(item.name) + ' (' + item.status + ')'; });
    let summary = state + ' · ' +
      (report.analysis === 'ts_structure_and_ffmpeg_decode'
        ? 'FFmpeg-Decodierung' : 'TS-Struktur') +
      ' · ' + (Number.isFinite(segments) ? segments : '?') +
      ' TS-Segmente · ' + errors;
    if (broken.length) summary += ' · Betroffene Segmente: ' + broken.join(', ');
    const sidecars = Array.isArray(report.sidecarReports)
      ? report.sidecarReports : [];
    const faultySidecars = sidecars.filter(function (item) {
      return item && item.status !== 'ok';
    }).map(function (item) { return text(item.name) + ' (' + item.status + ')'; });
    if (faultySidecars.length)
      summary += ' · Sidecars: ' + faultySidecars.join(', ');
    if (report.ffprobe && report.ffprobe.status === 'ok' &&
        typeof report.ffprobe.details === 'string') {
      try {
        const found = JSON.parse(report.ffprobe.details);
        const streams = Array.isArray(found.streams) ? found.streams : [];
        const media = streams.filter(function (item) {
          return item && (item.codec_type === 'video' ||
            item.codec_type === 'audio');
        }).map(function (item) {
          const codec = typeof item.codec_name === 'string'
            ? item.codec_name : '?';
          if (item.codec_type === 'video')
            return 'Video: ' + codec + (item.width && item.height
              ? ' ' + Number(item.width) + '×' + Number(item.height) : '');
          return 'Audio: ' + codec + (item.channels
            ? ' (' + Number(item.channels) + ' Kanäle)' : '');
        });
        if (media.length) summary += ' · ' + media.join(' · ');
        if (found.format && typeof found.format.format_name === 'string')
          summary += ' · Container: ' + found.format.format_name;
      } catch (_) {
        summary += ' · FFprobe-Bericht nicht lesbar.';
      }
    } else if (report.ffprobe && report.ffprobe.status !== 'ok') {
      summary += ' · FFprobe unvollständig.';
    }
    if (report.decoder && report.decoder.timedOut)
      summary += ' · FFmpeg-Zeitlimit erreicht.';
    if (report.decoder && report.decoder.diagnostic)
      summary += ' · FFmpeg: ' + text(report.decoder.diagnostic);
    return summary;
  }

  function renderMediaCheck(root, copy, recording, backendId, actions) {
    const client = mediaCheckClient();
    if (!client || !/^[-A-Za-z0-9._]{1,128}$/.test(text(backendId))) return;
    client.getMediaCheckCapability({
      backendId: backendId, credentials: 'same-origin', cache: 'no-store'
    }).then(function (capability) {
      if (!capability || capability.available !== true ||
          capability.backendId !== backendId ||
          capability.capability !== 'addons.media.check' ||
          !actions.parentNode) return;
      const action = makeButton('Aufnahme prüfen', function () {
        panel.hidden = false;
        if (typeof panel.scrollIntoView === 'function')
          panel.scrollIntoView({behavior: 'smooth', block: 'nearest'});
      });
      action.className = 'recordings2-media-check-action';
      const panel = shared.node('section', 'recordings2-media-check');
      panel.hidden = true;
      const heading = shared.node('h4', '', 'Aufnahme prüfen');
      const selector = global.document.createElement('select');
      selector.setAttribute('aria-label', 'Prüfumfang');
      const structure = global.document.createElement('option');
      structure.value = 'structure';
      structure.textContent = 'Schnelle TS-Prüfung';
      selector.appendChild(structure);
      const decode = global.document.createElement('option');
      decode.value = 'decode';
      decode.textContent = 'Vollständige FFmpeg-Prüfung';
      selector.appendChild(decode);
      const launch = makeButton('Prüfung starten', function () {
        launch.disabled = true;
        selector.disabled = true;
        status.textContent = 'Aufnahme wird zugeordnet …';
        output.textContent = '';
        Promise.resolve().then(function () {
          return resolveMediaCheckIdentity(client, recording, backendId);
        }).then(function (recordingId) {
          const session = global.VdrSuiteBrowserSession;
          if (!session || typeof session.restore !== 'function' ||
              typeof session.csrfHeaders !== 'function')
            throw new Error('Anmeldung erforderlich.');
          return Promise.resolve(session.restore()).then(function () {
            return client.submitMediaCheck({
              backendId: backendId, recordingId: recordingId,
              mode: selector.value, headers: session.csrfHeaders(),
              credentials: 'same-origin'
            });
          });
        }).then(function (created) {
          if (!created || !/^chk_[0-9a-f]{32}$/.test(created.checkId) ||
              created.backendId !== backendId ||
              created.state !== 'running')
            throw new Error('Unbekannte Media-Check-Antwort.');
          const checkId = created.checkId;
          let reads = 0;
          function poll() {
            if (root.isConnected === false || !panel.parentNode || reads++ >= 500) {
              status.textContent = 'Statusabfrage beendet. Serverprüfung kann weiterlaufen.';
              return;
            }
            return client.getMediaCheckStatus({
              backendId: backendId, checkId: checkId,
              credentials: 'same-origin', cache: 'no-store'
            }).then(function (result) {
              if (!result || result.checkId !== checkId ||
                  result.backendId !== backendId)
                throw new Error('Unbekannter Prüfstatus.');
              if (result.state === 'running') {
                status.textContent = 'Aufnahmeprüfung läuft …';
                global.setTimeout(poll, 2000);
                return;
              }
              if (result.state !== 'completed' || !result.report)
                throw new Error('Prüfbericht unvollständig.');
              status.textContent = mediaCheckSummary(result.report);
              output.textContent = JSON.stringify(result.report, null, 2);
              launch.disabled = false;
              selector.disabled = false;
            }).catch(function (error) {
              status.textContent = 'Statusabfrage fehlgeschlagen: ' +
                text(error && error.message ? error.message : error);
            });
          }
          status.textContent = 'Aufnahmeprüfung läuft …';
          return poll();
        }).catch(function (error) {
          status.textContent = 'Prüfung nicht gestartet: ' +
            text(error && error.message ? error.message : error);
          launch.disabled = false;
          selector.disabled = false;
        });
      });
      const status = shared.node('p', 'recordings2-media-check-status', 'Prüfumfang auswählen.');
      status.setAttribute('role', 'status');
      const details = global.document.createElement('details');
      const summary = shared.node('summary', '', 'Technischer Prüfbericht (JSON)');
      const output = shared.node('pre', 'recordings2-media-check-report');
      details.appendChild(summary);
      details.appendChild(output);
      panel.appendChild(heading);
      panel.appendChild(selector);
      panel.appendChild(launch);
      panel.appendChild(status);
      panel.appendChild(details);
      actions.appendChild(action);
      copy.appendChild(panel);
    }).catch(function () {
      // Absent or disabled Media Tools must not disrupt the Recording Hero.
    });
  }

  function renderMediaShrinkPlan(root, copy, recording, backendId, actions) {
    const client = mediaCheckClient();
    if (!client || !/^[-A-Za-z0-9._]{1,128}$/.test(text(backendId)) ||
        typeof client.getMediaShrinkCapability !== 'function') return;
    client.getMediaShrinkCapability({
      backendId: backendId, credentials: 'same-origin', cache: 'no-store'
    }).then(function (capability) {
      if (!capability || capability.planningAvailable !== true ||
          (capability.available !== true && capability.available !== false) ||
          capability.capability !== 'addons.media.shrink' ||
          capability.backendId !== backendId || !actions.parentNode) return;

      const panel = shared.node('section', 'recordings2-media-check');
      panel.hidden = true;
      const status = shared.node('p', 'recordings2-media-check-status',
        'HEVC-Vorprüfung liest die Aufnahme nur. Das Original bleibt erhalten.');
      status.setAttribute('role', 'status');
      const launch = makeButton('Vorprüfung starten', function () {
        launch.disabled = true;
        status.textContent = 'Aufnahme wird zugeordnet …';
        Promise.resolve().then(function () {
          return resolveMediaCheckIdentity(client, recording, backendId);
        }).then(function (identity) {
          recordingId = identity;
          const session = global.VdrSuiteBrowserSession;
          if (!session || typeof session.restore !== 'function' ||
              typeof session.csrfHeaders !== 'function')
            throw new Error('Anmeldung erforderlich.');
          return Promise.resolve(session.restore()).then(function () {
            return client.submitMediaShrinkPlan({
              backendId: backendId, recordingId: recordingId,
              headers: session.csrfHeaders(), credentials: 'same-origin'
            });
          });
        }).then(function (plan) {
          if (!plan || plan.backendId !== backendId ||
              plan.recordingId !== recordingId || plan.readOnly !== true ||
              typeof plan.stageAvailable !== 'boolean' ||
              !Number.isSafeInteger(plan.sourceBytes) || plan.sourceBytes <= 0 ||
              !Number.isSafeInteger(plan.segments) || plan.segments <= 0)
            throw new Error('Unbekannte HEVC-Vorprüfung.');
          planReady = plan.stageAvailable === true &&
            capability.available === true;
          status.textContent = 'Original: ' +
            (plan.sourceBytes / 1048576).toFixed(1) + ' MiB, ' +
            plan.segments + ' TS-Segment(e). ' +
            (planReady ? 'Konvertierung als privater Kandidat möglich.' :
              'HEVC-Konvertierung ist auf diesem Backend nicht freigeschaltet.');
          if (planReady) {
            controls.hidden = false;
            restorePending();
          }
        }).catch(function (error) {
          status.textContent = 'HEVC-Vorprüfung fehlgeschlagen: ' +
            text(error && error.message ? error.message : error);
        }).then(function () { launch.disabled = false; });
      });
      let recordingId = '';
      let planReady = false;
      let stageOperationId = '';
      let publishOperationId = '';
      let reviewId = '';
      let stagePolls = 0;
      let publishPolls = 0;

      const controls = shared.node('div', 'recordings2-media-shrink-controls');
      controls.hidden = true;
      const encoderLabel = shared.node('label', '', 'HEVC-Encoder');
      const encoder = shared.node('select', 'recordings2-media-shrink-encoder');
      const permitted = ['software', 'vaapi', 'qsv', 'nvenc'];
      const encoders = Array.isArray(capability.encoders)
        ? capability.encoders.filter(function (value) {
            return permitted.includes(value);
          }) : ['software'];
      encoders.forEach(function (value) {
        const option = shared.node('option', '', value === 'software' ?
          'Software (x265)' : value.toUpperCase());
        option.value = value;
        encoder.appendChild(option);
      });
      encoderLabel.appendChild(encoder);
      const heightLabel = shared.node('label', '', ' Auf 720p begrenzen');
      const height = shared.node('input');
      height.type = 'checkbox';
      heightLabel.prepend(height);
      const nightLabel = shared.node('label', '', ' Night-Mode');
      const night = shared.node('input');
      night.type = 'checkbox';
      nightLabel.prepend(night);

      const stage = makeButton('HEVC-Konvertierung starten', function () {
        if (!planReady || !recordingId || stage.disabled) return;
        const key = newIdempotencyKey();
        if (!key || !remember('stage', {key: key, operationId: ''})) {
          status.textContent = 'Sichere Wiederholsperre nicht verfügbar.';
          return;
        }
        stage.disabled = true;
        status.textContent = 'HEVC-Staging angefordert; keine automatische Wiederholung.';
        mutationHeaders().then(function (headers) {
          return client.submitMediaShrinkStage({
            backendId: backendId, recordingId: recordingId,
            encoder: encoder.value, maxHeight: height.checked ? 720 : 0,
            nightMode: night.checked, idempotencyKey: key,
            headers: headers, credentials: 'same-origin'
          });
        }).then(function (result) {
          if (!result || result.backendId !== backendId ||
              !/^op_[0-9a-f]{32}$/.test(text(result.operationId)))
            throw new Error('Stage-Operation konnte nicht sicher zugeordnet werden.');
          stageOperationId = result.operationId;
          remember('stage', {key: key, operationId: stageOperationId});
          refreshStage.disabled = false;
          refreshOperation('stage');
        }).catch(function (error) {
          // Request may have reached the backend. Never submit again, even
          // if the response was lost or the browser was reloaded.
          status.textContent = 'Stage-Ausgang unklar: ' +
            text(error && error.message ? error.message : error) +
            '. Bitte nicht erneut starten.';
        });
      });
      const refreshStage = makeButton('Stage-Status aktualisieren', function () {
        refreshOperation('stage');
      });
      refreshStage.disabled = true;
      const candidate = shared.node('p', 'recordings2-media-check-status');
      const publish = makeButton('Kandidaten ausdrücklich veröffentlichen', function () {
        if (!/^shk_[0-9a-f]{32}$/.test(reviewId) || publish.disabled ||
            typeof global.confirm !== 'function' ||
            !global.confirm('HEVC-Kandidat als zusätzliche VDR-Aufnahme veröffentlichen? Das Original bleibt erhalten.'))
          return;
        const key = newIdempotencyKey();
        if (!key || !remember('publish', {key: key, operationId: ''})) {
          status.textContent = 'Sichere Veröffentlichungs-Sperre nicht verfügbar.';
          return;
        }
        publish.disabled = true;
        status.textContent = 'Veröffentlichung angefordert; auf Recording-Reconciliation warten.';
        mutationHeaders().then(function (headers) {
          return client.submitMediaShrinkPublish({
            backendId: backendId, reviewId: reviewId, revision: 1,
            idempotencyKey: key, headers: headers, credentials: 'same-origin'
          });
        }).then(function (result) {
          if (!result || result.backendId !== backendId ||
              !/^op_[0-9a-f]{32}$/.test(text(result.operationId)))
            throw new Error('Publish-Operation nicht sicher zugeordnet.');
          publishOperationId = result.operationId;
          remember('publish', {key: key, operationId: publishOperationId});
          refreshPublish.disabled = false;
          refreshOperation('publish');
        }).catch(function (error) {
          status.textContent = 'Veröffentlichung unklar: ' +
            text(error && error.message ? error.message : error) +
            '. Keine automatische Wiederholung.';
        });
      });
      publish.hidden = true;
      const refreshPublish = makeButton('Publish-Status aktualisieren', function () {
        refreshOperation('publish');
      });
      refreshPublish.disabled = true;

      function storageName(action) {
        return 'vdr-suite-hevc-v1.' + backendId + '.' + recordingId +
          '.' + action;
      }
      function remembered(action) {
        try {
          const entry = JSON.parse(global.sessionStorage.getItem(storageName(action)));
          return entry && /^[A-Za-z0-9_.:-]{8,128}$/.test(entry.key) &&
            (entry.operationId === '' ||
              /^op_[0-9a-f]{32}$/.test(entry.operationId)) ? entry : null;
        } catch (_) { return null; }
      }
      function remember(action, value) {
        try {
          if (!global.sessionStorage || !value || !value.key) return false;
          global.sessionStorage.setItem(storageName(action), JSON.stringify(value));
          return true;
        } catch (_) { return false; }
      }
      function newIdempotencyKey() {
        try {
          return global.crypto && typeof global.crypto.randomUUID === 'function'
            ? 'hevc-' + global.crypto.randomUUID() : '';
        } catch (_) { return ''; }
      }
      function mutationHeaders() {
        const session = global.VdrSuiteBrowserSession;
        if (!session || typeof session.restore !== 'function' ||
            typeof session.csrfHeaders !== 'function')
          return Promise.reject(new Error('Anmeldung erforderlich.'));
        return Promise.resolve(session.restore()).then(function () {
          return session.csrfHeaders();
        });
      }
      function restorePending() {
        const savedStage = remembered('stage');
        if (savedStage) {
          stage.disabled = true;
          stageOperationId = savedStage.operationId;
          refreshStage.disabled = !stageOperationId;
          status.textContent = stageOperationId
            ? 'Gespeicherte Stage-Operation gefunden. Status abfragen.'
            : 'Unklarer früherer Stage-Start. Aus Sicherheitsgründen gesperrt.';
          if (stageOperationId) refreshOperation('stage');
        }
        if (!encoders.length) stage.disabled = true;
        const savedPublish = remembered('publish');
        if (savedPublish) {
          publish.disabled = true;
          publishOperationId = savedPublish.operationId;
          refreshPublish.disabled = !publishOperationId;
          if (publishOperationId) refreshOperation('publish');
        }
      }
      function fetchReview(id) {
        if (!/^shk_[0-9a-f]{32}$/.test(id) ||
            typeof client.getMediaShrinkReview !== 'function') return;
        client.getMediaShrinkReview({
          backendId: backendId, reviewId: id, credentials: 'same-origin',
          cache: 'no-store'
        }).then(function (review) {
          if (!review || review.backendId !== backendId ||
              review.recordingId !== recordingId || review.reviewId !== id)
            throw new Error('Kandidat passt nicht zu dieser Aufnahme.');
          const sizes = Number.isSafeInteger(review.candidateBytes) &&
            Number.isSafeInteger(review.sourceBytes)
            ? ' Kandidat: ' + (review.candidateBytes / 1048576).toFixed(1) +
              ' MiB; Einsparung: ' +
              ((review.sourceBytes - review.candidateBytes) / 1048576).toFixed(1) +
              ' MiB.' : '';
          candidate.textContent = 'Review: ' + review.state + '.' + sizes;
          publish.hidden = review.state !== 'staged' ||
            review.revision !== 1 || review.publishAvailable !== true;
          if (remembered('publish')) publish.disabled = true;
        }).catch(function (error) {
          candidate.textContent = 'Kandidatenprüfung fehlgeschlagen: ' +
            text(error && error.message ? error.message : error);
          publish.hidden = true;
        });
      }
      function refreshOperation(action) {
        const id = action === 'stage' ? stageOperationId : publishOperationId;
        if (!/^op_[0-9a-f]{32}$/.test(id) ||
            typeof client.getMediaShrinkOperation !== 'function') return;
        client.getMediaShrinkOperation({
          backendId: backendId, operationId: id, credentials: 'same-origin',
          cache: 'no-store'
        }).then(function (operation) {
          if (!operation || operation.operationId !== id ||
              operation.backendId !== backendId || operation.action !== action)
            throw new Error('HEVC-Operation gehört nicht zur gewählten Aufnahme.');
          status.textContent = 'HEVC-' + action + ': ' + operation.state +
            (operation.resultReconciled === true ? ' (verifiziert)' :
              ' (noch nicht reconciled)');
          // Poll only the canonical read-only status. Never retry Stage or
          // Publish when their outcome is unclear or still reconciling.
          if (operation.resultReconciled !== true &&
              ['accepted', 'queued', 'dispatching', 'executed_unverified',
                'outcome_unknown'].includes(operation.state) &&
              typeof global.setTimeout === 'function') {
            const next = action === 'stage' ? ++stagePolls : ++publishPolls;
            if (next <= 120) global.setTimeout(function () {
              if (panel.isConnected !== false) refreshOperation(action);
            }, 3000);
          }
          if (action === 'stage' && operation.resultReconciled === true &&
              /^shk_[0-9a-f]{32}$/.test(text(operation.reviewId))) {
            reviewId = operation.reviewId;
            fetchReview(reviewId);
          }
          if (action === 'publish' && operation.resultReconciled === true &&
              /^rec_[0-9a-f]{32}$/.test(text(operation.publishedRecordingId))) {
            status.textContent = 'Neue HEVC-Aufnahme bestätigt: ' +
              operation.publishedRecordingId + '. Original erhalten.';
            publish.hidden = true;
          }
        }).catch(function (error) {
          status.textContent = 'HEVC-Status nicht verfügbar: ' +
            text(error && error.message ? error.message : error) +
            '. Keine neue Mutation starten.';
        });
      }

      controls.append(encoderLabel, heightLabel, nightLabel, stage,
        refreshStage, candidate, publish, refreshPublish);
      panel.appendChild(shared.node('h4', '', 'HEVC-Speicherplatzoptimierung'));
      panel.appendChild(launch);
      panel.appendChild(status);
      panel.appendChild(controls);
      const action = makeButton('HEVC-Vorprüfung', function () {
        panel.hidden = false;
        if (typeof panel.scrollIntoView === 'function')
          panel.scrollIntoView({behavior: 'smooth', block: 'nearest'});
      });
      actions.appendChild(action);
      copy.appendChild(panel);
    }).catch(function () {
      // Inactive or inaccessible HEVC never disrupts playback.
    });
  }

  function renderHeroActions(root, copy, recording, backendId, metadata) {
    if (!copy || copy.querySelector('.recordings2-hero-actions')) return;
    const actions = shared.node('div', 'recordings2-hero-actions');
    const position = resumePosition(recording);
    actions.appendChild(makeButton(
      position > 0 ? '▶ Wiedergabe fortsetzen' : '▶ Abspielen',
      function () { startPlayback(root, recording); },
      true
    ));
    const identity = trailerIdentity(metadata);
    if (identity) {
      actions.appendChild(makeButton('▶ Trailer', function () {
        showTrailer(root, backendId, identity);
      }));
    }
    actions.appendChild(makeButton('Schnittmarken', function () {
      showMode(root, 'marks', '.recordings2-marks-detail', 'Marke setzen');
    }));
    actions.appendChild(makeButton('Metadaten', function () {
      showMode(root, 'metadata', '.recordings2-metadata-tabs');
    }));
    actions.appendChild(makeButton('Aufnahmeaktionen', function () {
      showMode(root, 'actions', '.recordings2-actions');
    }));
    copy.appendChild(actions);
    renderMediaCheck(root, copy, recording, backendId, actions);
    renderMediaShrinkPlan(root, copy, recording, backendId, actions);
  }

  function releaseDate(metadata) {
    const value = text(metadata && (metadata.releaseDate || metadata.firstAired));
    const match = /^(\d{4})-(\d{2})-(\d{2})$/.exec(value);
    return match ? match[3] + '.' + match[2] + '.' + match[1] : value;
  }

  function appendFact(container, label, value, className) {
    const normalized = text(value);
    if (!normalized) return;
    const fact = shared.node('span', 'recordings2-hero-fact' + (className ? ' ' + className : ''));
    fact.appendChild(shared.node('span', 'recordings2-hero-fact-label', label));
    fact.appendChild(shared.node('span', 'recordings2-hero-fact-value', normalized));
    container.appendChild(fact);
  }

  function renderHeroFacts(copy, recording, metadata) {
    if (!copy || copy.querySelector('.recordings2-hero-facts')) return;
    copy.insertBefore(shared.node('div', 'recordings2-hero-eyebrow', 'VDR-Suite · Aufnahme'), copy.firstChild);
    const facts = shared.node('div', 'recordings2-hero-facts');
    const rating = Number(metadata && metadata.voteAverage);
    if (Number.isFinite(rating) && rating > 0) {
      appendFact(facts, 'Bewertung', '★ ' + rating.toFixed(1) + ' / 10', 'rating');
    }
    appendFact(facts, 'Veröffentlichung', releaseDate(metadata));
    appendFact(
      facts,
      'Genre',
      (Array.isArray(metadata && metadata.genres) ? metadata.genres : [])
        .map(text).filter(Boolean).slice(0, 3).join(' · ')
    );
    const duration = Number(shared.first(recording, ['durationSeconds', 'duration'], 0));
    if (Number.isFinite(duration) && duration > 0) appendFact(facts, 'Dauer', shared.formatDuration(duration));
    const sizeMb = Number(shared.first(recording, ['sizeMb'], 0));
    if (Number.isFinite(sizeMb) && sizeMb > 0) appendFact(facts, 'Größe', shared.formatSize(sizeMb));
    const description = copy.querySelector('.recordings2-detail-description');
    copy.insertBefore(facts, description || null);
  }

  function personSearchOwner() {
    const owner = global.VdrSuiteRecordings2PersonSearchView;
    return owner && typeof owner.findRecordings === 'function' ? owner : null;
  }

  function personRoleLabel(person) {
    const owner = personSearchOwner();
    return owner && typeof owner.roleLabel === 'function'
      ? owner.roleLabel(person && person.role)
      : 'Schauspiel';
  }

  function renderHeroCast(root, copy, recording, backendId, metadata) {
    if (!copy || copy.querySelector('.recordings2-hero-cast')) return;
    const actors = actorList(metadata).slice(0, 7);
    if (!actors.length) return;
    const cast = shared.node('div', 'recordings2-hero-cast');
    actors.forEach(function (person) {
      const item = shared.node('button', 'recordings2-hero-person');
      item.type = 'button';
      item.title = person.name + ' in vorhandenen Aufnahmen suchen';
      item.setAttribute('aria-label', item.title);
      if (person.image && person.image.available === true && text(person.image.url)) {
        const image = document.createElement('img');
        image.src = publicImageUrl(person.image.url);
        image.alt = person.name;
        image.loading = 'eager';
        item.appendChild(image);
      } else {
        item.appendChild(shared.node('span', 'recordings2-hero-person-placeholder', '•'));
      }
      const personCopy = shared.node('span', 'recordings2-hero-person-copy');
      personCopy.appendChild(shared.node('strong', '', person.name));
      if (text(person.characterName)) personCopy.appendChild(shared.node('span', '', person.characterName));
      personCopy.appendChild(shared.node('span', 'recordings2-hero-person-role', personRoleLabel(person)));
      item.appendChild(personCopy);
      item.addEventListener('click', function () {
        renderPersonRelated(root, recording, backendId, metadata, person);
      });
      cast.appendChild(item);
    });
    copy.appendChild(cast);
  }

  function sameRecording(left, right) {
    const leftId = text(shared.first(left, ['backendNativeId'], ''));
    const rightId = text(shared.first(right, ['backendNativeId'], ''));
    if (leftId && rightId) return leftId === rightId;
    return text(shared.first(left, ['path'], '')) === text(shared.first(right, ['path'], ''));
  }

  function createRelatedCard(recording, currentRecording, backendId) {
    const button = shared.node('button', 'recordings2-hero-related-card');
    button.type = 'button';
    const poster = shared.node('span', 'recordings2-hero-related-poster', '▶');
    const url = shared.recordingPosterUrl(recording);
    if (text(url)) {
      const image = document.createElement('img');
      image.src = publicImageUrl(url);
      image.alt = 'Poster zu ' + shared.recordingTitle(recording);
      image.loading = 'lazy';
      image.addEventListener('error', function () {
        image.remove();
        poster.textContent = '▶';
      }, {once: true});
      poster.replaceChildren(image);
    }
    button.appendChild(poster);
    button.appendChild(shared.node('span', 'recordings2-hero-related-title', shared.recordingTitle(recording)));
    button.addEventListener('click', function () {
      const runtime = global.VdrSuiteRecordings2;
      if (!runtime || typeof runtime.openRecording !== 'function') return;
      runtime.openRecording(recording, {
        backendId: backendId,
        backLabel: '← Zurück zu ' + shared.recordingTitle(currentRecording),
        onClose: function () {
          runtime.openRecording(currentRecording, {
            backendId: backendId,
            backLabel: '← Zurück zu den Aufnahmen'
          });
        }
      });
    });
    return button;
  }

  function nextRelatedGeneration(root) {
    const next = Number(root && root.__vdrSuiteHeroRelatedGeneration || 0) + 1;
    if (root) root.__vdrSuiteHeroRelatedGeneration = next;
    return next;
  }

  function relatedGenerationCurrent(root, generation) {
    return Boolean(root && root.__vdrSuiteHeroRelatedGeneration === generation);
  }

  function replaceRelatedSection(root, section) {
    if (!root || !section) return false;
    const existing = root.querySelector('.recordings2-hero-related');
    if (existing && typeof existing.remove === 'function') existing.remove();
    root.appendChild(section);
    placeTrailerAfterRelated(root);
    return true;
  }

  function createRelatedHead(title, subtitle, onBack) {
    const head = shared.node('div', 'recordings2-hero-related-head');
    const copy = shared.node('div', 'recordings2-hero-related-head-copy');
    copy.appendChild(shared.node('h4', '', title));
    copy.appendChild(shared.node('span', '', subtitle));
    head.appendChild(copy);
    if (typeof onBack === 'function') {
      const back = shared.node('button', 'recordings2-hero-related-back', '← Zurück');
      back.type = 'button';
      back.title = 'Zurück zu den Empfehlungen dieser Aufnahme';
      back.addEventListener('click', onBack);
      head.appendChild(back);
    }
    return head;
  }

  function showRelatedStatus(root, title, subtitle, message, onBack) {
    if (!root) return false;
    const section = shared.node('section', 'recordings2-hero-related');
    section.appendChild(createRelatedHead(title, subtitle, onBack));
    const status = shared.node('p', 'recordings2-hero-related-status', message);
    status.setAttribute('role', 'status');
    section.appendChild(status);
    return replaceRelatedSection(root, section);
  }

  function appendRelatedSection(root, recording, backendId, title, subtitle, matches, onBack) {
    if (!root || !matches.length) return false;
    const section = shared.node('section', 'recordings2-hero-related');
    section.appendChild(createRelatedHead(title, subtitle, onBack));
    const rail = shared.node('div', 'recordings2-hero-related-rail');
    matches.forEach(function (candidate) {
      rail.appendChild(createRelatedCard(candidate, recording, backendId));
    });
    section.appendChild(rail);
    return replaceRelatedSection(root, section);
  }

  function normalizedGenre(value) {
    return text(value).toLocaleLowerCase('de-DE');
  }

  function genreLabels(metadata) {
    return (Array.isArray(metadata && metadata.genres) ? metadata.genres : [])
      .map(text)
      .filter(Boolean);
  }

  function matchingGenre(payload, metadata) {
    const genres = payload && Array.isArray(payload.genres) ? payload.genres : [];
    const wanted = genreLabels(metadata).map(normalizedGenre);
    return genres.find(function (genre) {
      return wanted.indexOf(normalizedGenre(genre && genre.label)) !== -1 ||
        wanted.indexOf(normalizedGenre(genre && genre.id)) !== -1;
    }) || null;
  }

  function renderGenreRelated(root, recording, backendId, metadata, api, generation) {
    if (!root || !relatedGenerationCurrent(root, generation)) return Promise.resolve(false);
    if (!api || typeof api.fetchClientGenres !== 'function' ||
        typeof api.fetchClientGenreRecordings !== 'function') {
      return Promise.resolve(false);
    }
    if (!genreLabels(metadata).length) return Promise.resolve(false);

    return Promise.resolve(api.fetchClientGenres({
      backendId: backendId,
      scope: 'recordings',
      cache: 'no-store',
      credentials: 'same-origin'
    })).then(function (overview) {
      if (!relatedGenerationCurrent(root, generation)) return false;
      const genre = matchingGenre(overview, metadata);
      if (!genre || !text(genre.id)) return false;
      return api.fetchClientGenreRecordings({
        backendId: backendId,
        genreId: genre.id,
        limit: 20,
        offset: 0,
        cache: 'no-store',
        credentials: 'same-origin'
      }).then(function (result) {
        if (!relatedGenerationCurrent(root, generation)) return false;
        if (!root.isConnected && typeof root.isConnected === 'boolean') return false;
        const matches = (result && Array.isArray(result.items) ? result.items : [])
          .filter(Boolean)
          .filter(function (candidate) { return !sameRecording(candidate, recording); })
          .slice(0, 12);
        return appendRelatedSection(
          root,
          recording,
          backendId,
          'Mehr aus ' + text(genre.label || genre.id),
          String(matches.length) + ' Aufnahme(n) im selben Genre',
          matches
        );
      });
    }).catch(function () { return false; });
  }

  function renderRelated(root, recording, backendId, metadata) {
    if (!root) return;
    const generation = nextRelatedGeneration(root);
    const actor = actorList(metadata)[0];
    const api = shared.clientApi();
    if (!api) return;

    const fallback = function () {
      if (!relatedGenerationCurrent(root, generation)) return Promise.resolve(false);
      return renderGenreRelated(root, recording, backendId, metadata, api, generation);
    };

    const personOwner = personSearchOwner();
    if (!actor || !personOwner) {
      fallback();
      return;
    }

    personOwner.findRecordings(actor, backendId, 20).then(function (recordings) {
      if (!relatedGenerationCurrent(root, generation)) return false;
      if (!root.isConnected && typeof root.isConnected === 'boolean') return false;
      const matches = recordings
        .filter(function (candidate) { return !sameRecording(candidate, recording); })
        .slice(0, 12);
      if (matches.length) {
        return appendRelatedSection(
          root,
          recording,
          backendId,
          'Weitere Filme mit ' + actor.name,
          String(matches.length) + ' lokale Aufnahme(n)',
          matches
        );
      }
      return fallback();
    }).catch(fallback);
  }

  function renderPersonRelated(root, recording, backendId, metadata, person) {
    if (!root || !person || !text(person.name)) return;
    const generation = nextRelatedGeneration(root);
    const title = 'Weitere Filme mit ' + text(person.name);
    const restore = function () {
      renderRelated(root, recording, backendId, metadata);
      global.setTimeout(function () {
        const section = root.querySelector('.recordings2-hero-related');
        if (section && typeof section.scrollIntoView === 'function') {
          section.scrollIntoView({behavior: 'smooth', block: 'start'});
        }
      }, 0);
    };

    showRelatedStatus(
      root,
      title,
      'Suche in vorhandenen Aufnahmen',
      'Suche in vorhandenen Aufnahmen …',
      restore
    );

    const personOwner = personSearchOwner();
    if (!personOwner) {
      showRelatedStatus(
        root,
        title,
        'Lokale Aufnahmensuche',
        'Aufnahmensuche ist nicht verfügbar.',
        restore
      );
      return;
    }

    personOwner.findRecordings(person, backendId, 20).then(function (recordings) {
      if (!relatedGenerationCurrent(root, generation)) return false;
      if (!root.isConnected && typeof root.isConnected === 'boolean') return false;
      const matches = recordings
        .filter(function (candidate) { return !sameRecording(candidate, recording); })
        .slice(0, 12);
      if (!matches.length) {
        return showRelatedStatus(
          root,
          title,
          'Lokale Aufnahmensuche',
          'Keine weitere vorhandene Aufnahme mit dieser Person gefunden.',
          restore
        );
      }
      const appended = appendRelatedSection(
        root,
        recording,
        backendId,
        title,
        String(matches.length) + ' lokale Aufnahme(n)',
        matches,
        restore
      );
      global.setTimeout(function () {
        const section = root.querySelector('.recordings2-hero-related');
        if (section && typeof section.scrollIntoView === 'function') {
          section.scrollIntoView({behavior: 'smooth', block: 'start'});
        }
      }, 0);
      return appended;
    }).catch(function () {
      if (!relatedGenerationCurrent(root, generation)) return false;
      return showRelatedStatus(
        root,
        title,
        'Lokale Aufnahmensuche',
        'Aufnahmensuche fehlgeschlagen.',
        restore
      );
    });
  }

  function enhance(root, recording, backendId, metadata) {
    if (!root || !recording) return root;
    installStyles();
    root.classList.add('recordings2-hero-page');
    root.dataset.recordings2HeroMode = 'detail';
    ensureModeBack(root).hidden = true;
    const backdrop = backdropUrl(metadata);
    if (backdrop && root.style && typeof root.style.setProperty === 'function') {
      root.style.setProperty('--recordings2-hero-backdrop', 'url("' + backdrop.replace(/"/g, '%22') + '")');
    }
    const copy = root.querySelector('.recordings2-detail-copy');
    renderHeroFacts(copy, recording, metadata);
    renderHeroCast(root, copy, recording, backendId, metadata);
    renderHeroActions(root, copy, recording, backendId, metadata);
    renderRelated(root, recording, backendId, metadata);
    return root;
  }

  global.VdrSuiteRecordings2HeroDetail = Object.freeze({
    enhance,
    __test: Object.freeze({
      backdropUrl,
      actorList,
      sameRecording,
      showMode,
      resumePosition,
      releaseDate,
      genreLabels,
      matchingGenre,
      trailerIdentity,
      validYoutubeVideoId,
      resolveMediaCheckIdentity,
      mediaCheckSummary,
      placeTrailerAfterRelated
    })
  });
}(window));
