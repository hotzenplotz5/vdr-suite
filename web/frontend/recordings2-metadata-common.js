// Shared Recordings 2 native metadata helpers and presentation primitives.
(function (global) {
  'use strict';

  const STYLE_ID = 'vdr-suite-recordings2-metadata-detail-style';
  const roleLabels = Object.freeze({
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

  function text(value) {
    return String(value === undefined || value === null ? '' : value).trim();
  }

  function first(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
        return object[key];
      }
    }
    return fallback;
  }

  function node(tag, className, value) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (value !== undefined && value !== null) element.textContent = String(value);
    return element;
  }

  function clientApi() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getClientApi === 'function') {
      const api = platform.getClientApi();
      if (api) return api;
    }
    return global.VdrSuiteClientApi || null;
  }

  function roleLabel(value) {
    return roleLabels[text(value).toLowerCase()] || roleLabels.unknown;
  }

  function formatDate(value) {
    const raw = text(value);
    const match = /^(\d{4})-(\d{2})-(\d{2})$/.exec(raw);
    return match ? match[3] + '.' + match[2] + '.' + match[1] : raw;
  }

  function mediaTypeLabel(value) {
    if (value === 'movie') return 'Film';
    if (value === 'series') return 'Serie';
    return text(value) || 'Unbekannt';
  }

  function orientationLabel(value) {
    if (value === 'portrait') return 'Hochformat';
    if (value === 'banner') return 'Banner';
    if (value === 'landscape') return 'Querformat';
    return 'Bild';
  }

  function isPublicMetadataImageUrl(value) {
    return text(value).startsWith('/api/vdr/recordings/metadata/image?');
  }

  function isPublicRecordingImageUrl(value) {
    const url = text(value);
    return url.startsWith('/recording-artwork/') ||
      url.startsWith('/api/vdr/recordings/artwork') ||
      url.startsWith('/api/recordings/artwork');
  }

  function metadata(recording) {
    return recording && recording.metadata && typeof recording.metadata === 'object'
      ? recording.metadata
      : {};
  }

  function presentation(recording) {
    const value = metadata(recording).presentation;
    return value && typeof value === 'object' ? value : {};
  }

  function provider(recording) {
    const value = metadata(recording).provider;
    return value && typeof value === 'object' ? value : {};
  }

  function artwork(recording) {
    const value = metadata(recording).artwork;
    return value && typeof value === 'object' ? value : {};
  }

  function recordingTitle(recording) {
    return text(first(
      presentation(recording),
      ['title'],
      first(provider(recording), ['seriesTitle', 'title'], first(recording, ['title', 'id'], 'Aufnahme'))
    ));
  }

  function recordingSubtitle(recording) {
    return text(first(
      presentation(recording),
      ['subtitle', 'seasonEpisode'],
      first(provider(recording), ['episodeTitle', 'tagline'], '')
    ));
  }

  function recordingSummary(recording) {
    return text(first(
      presentation(recording),
      ['summary'],
      first(provider(recording), ['overview'], '')
    ));
  }

  function recordingPosterUrl(recording) {
    const url = text(first(
      presentation(recording),
      ['posterUrl'],
      first(artwork(recording), ['preferredUrl'], '')
    ));
    return isPublicRecordingImageUrl(url) ? url : '';
  }

  function formatRecordingStart(value) {
    const numeric = Number(value);
    if (Number.isFinite(numeric) && numeric > 1000000000) {
      return new Date(numeric * 1000).toLocaleString('de-DE', {
        day: '2-digit', month: '2-digit', year: 'numeric', hour: '2-digit', minute: '2-digit'
      });
    }
    return text(value) || 'Unbekannt';
  }

  function formatRecordingDuration(value) {
    const seconds = Number(value);
    if (!Number.isFinite(seconds) || seconds <= 0) return 'Dauer unbekannt';
    const minutes = Math.round(seconds / 60);
    if (minutes < 60) return String(minutes) + ' min';
    const hours = Math.floor(minutes / 60);
    const rest = minutes % 60;
    return String(hours) + ' h' + (rest ? ' ' + String(rest) + ' min' : '');
  }

  function formatRecordingSize(value) {
    const megabytes = Number(value);
    if (!Number.isFinite(megabytes) || megabytes <= 0) return 'Größe unbekannt';
    return megabytes >= 1024
      ? (megabytes / 1024).toFixed(1) + ' GB'
      : String(Math.round(megabytes)) + ' MB';
  }

  function ensureStyles() {
    if (document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = [
      '.recordings2-metadata-tabs{display:flex;gap:.4rem;overflow-x:auto;padding:.25rem;border:1px solid rgba(148,163,184,.25);border-radius:.8rem;background:rgba(15,23,42,.82)}',
      '.recordings2-metadata-tab{flex:0 0 auto;min-height:2.45rem!important;padding:.42rem .7rem!important;border-color:transparent!important;background:transparent!important;color:#94a3b8!important;font-size:.78rem!important}',
      '.recordings2-metadata-tab[aria-selected="true"]{background:#1d4ed8!important;color:#eff6ff!important}',
      '.recordings2-metadata-tab:disabled{opacity:.38}',
      '.recordings2-metadata-panel{display:grid;gap:.7rem;scroll-margin-top:5rem}',
      '.recordings2-metadata-panel[hidden],.recordings2-person-results[hidden]{display:none!important}',
      '.recordings2-metadata-status{display:grid;gap:.3rem;padding:.7rem;border:1px solid #334155;border-radius:.7rem;background:#111827;color:#cbd5e1}',
      '.recordings2-metadata-status.error{border-color:#7f1d1d;background:#450a0a;color:#fecaca}',
      '.recordings2-metadata-facts{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.5rem}',
      '.recordings2-metadata-fact{display:grid;gap:.14rem;padding:.62rem;border:1px solid rgba(148,163,184,.2);border-radius:.68rem;background:rgba(15,23,42,.68)}',
      '.recordings2-metadata-fact span{color:#94a3b8;font-size:.66rem;font-weight:800;text-transform:uppercase;letter-spacing:.03em}',
      '.recordings2-metadata-fact strong{overflow-wrap:anywhere;color:#e2e8f0;font-size:.82rem}',
      '.recordings2-metadata-badges{display:flex;flex-wrap:wrap;gap:.38rem}',
      '.recordings2-metadata-badge{display:inline-flex;align-items:center;min-height:1.7rem;padding:.18rem .52rem;border:1px solid #475569;border-radius:999px;background:#172033;color:#dbeafe;font-size:.72rem;font-weight:800}',
      '.recordings2-metadata-copy{display:grid;gap:.35rem;padding:.72rem;border:1px solid rgba(148,163,184,.22);border-radius:.72rem;background:rgba(15,23,42,.68)}',
      '.recordings2-metadata-copy h4{color:#f8fafc}.recordings2-metadata-copy p{color:#dbeafe;line-height:1.5}',
      '.recordings2-metadata-cast{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.65rem}',
      '.recordings2-person-entry{display:grid;gap:.45rem;min-width:0}',
      '.recordings2-person-card{display:grid;grid-template-columns:4.25rem minmax(0,1fr);gap:.65rem;align-items:center;width:100%;padding:.55rem;border:1px solid #334155;border-radius:.75rem;background:#111827;color:inherit;text-align:left}',
      '.recordings2-person-card:hover,.recordings2-person-card:focus-visible{border-color:#60a5fa;background:#172554}',
      '.recordings2-person-image,.recordings2-person-placeholder{width:4.25rem;aspect-ratio:2/3;border-radius:.55rem;object-fit:cover;background:#1e293b}',
      '.recordings2-person-placeholder{display:grid;place-items:center;color:#64748b;font-size:1.3rem;font-weight:900}',
      '.recordings2-person-copy{display:grid;gap:.18rem;min-width:0}',
      '.recordings2-person-name{color:#f8fafc;font-weight:850;line-height:1.25}',
      '.recordings2-person-character{color:#bfdbfe;font-size:.76rem;line-height:1.3}',
      '.recordings2-person-role{color:#94a3b8;font-size:.68rem;font-weight:800;text-transform:uppercase}',
      '.recordings2-person-results{display:grid;gap:.48rem;padding:.55rem;border:1px solid #334155;border-radius:.68rem;background:#020617}',
      '.recordings2-person-recording{display:grid;grid-template-columns:4rem minmax(0,1fr) auto;gap:.6rem;align-items:center;width:100%;padding:.48rem;border:1px solid #334155;border-radius:.68rem;background:#0f172a;color:inherit;text-align:left}',
      '.recordings2-person-recording:hover,.recordings2-person-recording:focus-visible{border-color:#38bdf8;background:#082f49}',
      '.recordings2-person-poster{display:grid;place-items:center;width:4rem;aspect-ratio:2/3;overflow:hidden;border-radius:.5rem;background:#1e293b;color:#e0f2fe}',
      '.recordings2-person-poster img{display:block;width:100%;height:100%;object-fit:cover}',
      '.recordings2-person-recording-copy{display:grid;gap:.16rem;min-width:0}',
      '.recordings2-person-recording-title{color:#f8fafc;font-weight:850}',
      '.recordings2-person-recording-subtitle{color:#bae6fd;font-size:.74rem}',
      '.recordings2-person-recording-meta{color:#94a3b8;font-size:.68rem}',
      '.recordings2-person-recording-details{display:grid;gap:.4rem;padding:.55rem;border:1px solid #1e293b;border-radius:.58rem;background:#0f172a}',
      '.recordings2-person-recording-details[hidden]{display:none!important}',
      '.recordings2-metadata-gallery{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:.58rem}',
      '.recordings2-metadata-image{display:grid;gap:.32rem;margin:0;padding:.36rem;border:1px solid #334155;border-radius:.68rem;background:#111827}',
      '.recordings2-metadata-image img{display:block;width:100%;aspect-ratio:16/9;border-radius:.48rem;object-fit:contain;background:#020617}',
      '.recordings2-metadata-image.portrait img{aspect-ratio:2/3}',
      '.recordings2-metadata-image figcaption{color:#cbd5e1;font-size:.7rem;font-weight:800}',
      '@media(max-width:720px){.recordings2-metadata-facts,.recordings2-metadata-cast{grid-template-columns:1fr}.recordings2-metadata-gallery{grid-template-columns:repeat(2,minmax(0,1fr))}}',
      '@media(max-width:390px){.recordings2-metadata-gallery{grid-template-columns:1fr}.recordings2-person-recording{grid-template-columns:3.5rem minmax(0,1fr) auto}.recordings2-person-poster{width:3.5rem}}'
    ].join('');
    document.head.appendChild(style);
  }

  function status(message, error) {
    const box = node('p', 'recordings2-metadata-status' + (error ? ' error' : ''), message);
    box.setAttribute('role', error ? 'alert' : 'status');
    return box;
  }

  global.VdrSuiteRecordings2MetadataCommon = Object.freeze({
    artwork,
    clientApi,
    ensureStyles,
    first,
    formatDate,
    formatRecordingDuration,
    formatRecordingSize,
    formatRecordingStart,
    isPublicMetadataImageUrl,
    isPublicRecordingImageUrl,
    mediaTypeLabel,
    node,
    orientationLabel,
    presentation,
    provider,
    recordingPosterUrl,
    recordingSubtitle,
    recordingSummary,
    recordingTitle,
    roleLabel,
    status,
    text
  });
}(window));
