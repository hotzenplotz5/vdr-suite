// Shared presentation and DOM helpers for the independent Recordings 2 runtime.
(function (global) {
  'use strict';

  const PAGE_SIZE = 50;
  const STYLE_ID = 'vdr-suite-recordings2-styles';
  const CSS = `
#detail-data.recordings2-mount{display:block!important;width:100%!important;max-width:none!important}
.recordings2{display:grid;gap:.8rem;width:100%}.recordings2 *{box-sizing:border-box}.recordings2 h3,.recordings2 h4,.recordings2 p{margin:0}
.recordings2-header{display:grid;gap:.65rem;padding:.85rem;border:1px solid rgba(56,189,248,.28);border-radius:.9rem;background:rgba(2,6,23,.78)}
.recordings2-heading{display:flex;align-items:flex-start;justify-content:space-between;gap:.75rem}.recordings2-heading-copy{display:grid;gap:.2rem;min-width:0}.recordings2-heading h3{color:#f8fafc;font-size:1.15rem}.recordings2-heading p{color:#94a3b8;font-size:.82rem;line-height:1.35}
.recordings2-toolbar{display:flex;flex-wrap:wrap;gap:.45rem}.recordings2 button{min-height:2.75rem;padding:.55rem .8rem;border:1px solid rgba(96,165,250,.52);border-radius:.7rem;background:rgba(30,64,175,.28);color:#e0f2fe;font:inherit;font-weight:750}.recordings2 button:disabled{opacity:.48}.recordings2 button.recordings2-primary{background:#0369a1;color:#fff}
.recordings2-breadcrumbs{display:flex;flex-wrap:wrap;align-items:center;gap:.28rem;color:#94a3b8;font-size:.8rem}.recordings2-breadcrumbs button{min-height:2.2rem;padding:.32rem .5rem;border-radius:.55rem;background:rgba(15,23,42,.75);font-size:.78rem}.recordings2-separator{color:#475569}
.recordings2-status{display:grid;gap:.45rem;padding:.85rem;border:1px solid rgba(148,163,184,.24);border-radius:.8rem;background:rgba(15,23,42,.72);color:#cbd5e1}.recordings2-status strong{color:#f8fafc}.recordings2-status.error{border-color:rgba(248,113,113,.55);background:rgba(69,10,10,.38);color:#fecaca}.recordings2-status-detail{overflow-wrap:anywhere;color:#94a3b8;font-size:.76rem;line-height:1.35}.recordings2-progress{width:100%;height:.55rem}
.recordings2-summary{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:.45rem}.recordings2-metric{display:grid;gap:.1rem;padding:.6rem;border:1px solid rgba(148,163,184,.2);border-radius:.7rem;background:rgba(15,23,42,.64);text-align:center}.recordings2-metric strong{color:#f8fafc;font-size:1.05rem}.recordings2-metric span{color:#94a3b8;font-size:.7rem}
.recordings2-section{display:grid;gap:.55rem}.recordings2-section-title{display:flex;align-items:center;justify-content:space-between;gap:.5rem;color:#e2e8f0}.recordings2-count{padding:.16rem .45rem;border:1px solid rgba(148,163,184,.25);border-radius:999px;color:#94a3b8;font-size:.72rem}
.recordings2-folder-list,.recordings2-recording-list{display:grid;gap:.55rem}.recordings2-folder{display:grid;grid-template-columns:3rem minmax(0,1fr) auto;align-items:center;gap:.7rem;width:100%;padding:.72rem;text-align:left;background:rgba(15,23,42,.78)}.recordings2-folder-icon{display:grid;place-items:center;width:3rem;height:3rem;border-radius:.7rem;background:rgba(14,165,233,.18);font-size:1.5rem}.recordings2-folder-copy{display:grid;gap:.12rem;min-width:0}.recordings2-folder-name{overflow:hidden;color:#f8fafc;font-weight:850;text-overflow:ellipsis;white-space:nowrap}.recordings2-folder-meta{color:#94a3b8;font-size:.78rem}.recordings2-chevron{color:#7dd3fc;font-size:1.35rem}
.recordings2-recording{display:grid;grid-template-columns:5.2rem minmax(0,1fr);gap:.72rem;padding:.68rem;border:1px solid rgba(148,163,184,.2);border-radius:.82rem;background:rgba(15,23,42,.74);color:#f8fafc;text-align:left}.recordings2-recording:hover,.recordings2-recording:focus-visible{border-color:rgba(56,189,248,.65);background:rgba(14,165,233,.12)}.recordings2-poster{display:grid;place-items:center;width:5.2rem;aspect-ratio:2/3;overflow:hidden;border:1px solid rgba(125,211,252,.3);border-radius:.65rem;background:linear-gradient(145deg,#075985,#1e293b 58%,#020617);color:#e0f2fe;font-size:1.5rem}.recordings2-poster[data-variant="1"]{background:linear-gradient(145deg,#9a3412,#4c1d95 58%,#020617)}.recordings2-poster[data-variant="2"]{background:linear-gradient(145deg,#047857,#164e63 58%,#020617)}.recordings2-poster[data-variant="3"]{background:linear-gradient(145deg,#92400e,#7f1d1d 58%,#020617)}.recordings2-poster[data-variant="4"]{background:linear-gradient(145deg,#5b21b6,#1e3a8a 58%,#020617)}.recordings2-poster[data-variant="5"]{background:linear-gradient(145deg,#9d174d,#4338ca 58%,#020617)}.recordings2-poster img{display:block;width:100%;height:100%;object-fit:cover}.recordings2-recording-copy{display:grid;align-content:start;gap:.28rem;min-width:0}.recordings2-title{color:#f8fafc;font-size:.98rem;font-weight:850;line-height:1.25}.recordings2-subtitle{color:#bae6fd;font-size:.8rem;line-height:1.3}.recordings2-meta{color:#94a3b8;font-size:.76rem;line-height:1.38}.recordings2-summary-text{display:-webkit-box;overflow:hidden;color:#cbd5e1;font-size:.79rem;line-height:1.38;-webkit-box-orient:vertical;-webkit-line-clamp:3}
.recordings2-empty{padding:1rem;border:1px dashed rgba(148,163,184,.3);border-radius:.8rem;color:#94a3b8;text-align:center}.recordings2-more{width:100%}
.recordings2-detail{display:grid;gap:.8rem}.recordings2-detail-hero{display:grid;grid-template-columns:minmax(8rem,11rem) minmax(0,1fr);gap:.9rem;padding:.85rem;border:1px solid rgba(56,189,248,.3);border-radius:.9rem;background:rgba(2,6,23,.82)}.recordings2-detail-poster{width:100%;aspect-ratio:2/3}.recordings2-detail-copy{display:grid;align-content:start;gap:.5rem;min-width:0}.recordings2-detail-copy h3{color:#f8fafc;font-size:1.25rem;line-height:1.2}.recordings2-detail-description{color:#dbe4f0;line-height:1.5;white-space:pre-line}.recordings2-detail-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.5rem}.recordings2-detail-field{display:grid;gap:.15rem;padding:.65rem;border:1px solid rgba(148,163,184,.18);border-radius:.7rem;background:rgba(15,23,42,.65)}.recordings2-detail-field span{color:#94a3b8;font-size:.7rem;text-transform:uppercase}.recordings2-detail-field strong{overflow-wrap:anywhere;color:#e2e8f0;font-size:.82rem}
@media(max-width:720px){.recordings2-header{padding:.72rem}.recordings2-heading{display:grid}.recordings2-toolbar{display:grid;grid-template-columns:1fr 1fr}.recordings2-toolbar button{width:100%}.recordings2-recording{grid-template-columns:4.65rem minmax(0,1fr);gap:.62rem;padding:.58rem}.recordings2-poster{width:4.65rem}.recordings2-detail-hero{grid-template-columns:6.7rem minmax(0,1fr);gap:.7rem;padding:.7rem}.recordings2-detail-grid{grid-template-columns:1fr}.recordings2-detail-copy h3{font-size:1.08rem}}
@media(max-width:390px){.recordings2-toolbar{grid-template-columns:1fr}.recordings2-folder{grid-template-columns:2.6rem minmax(0,1fr) auto;padding:.62rem}.recordings2-folder-icon{width:2.6rem;height:2.6rem}.recordings2-detail-hero{grid-template-columns:1fr}.recordings2-detail-poster{max-width:10rem}.recordings2-summary{gap:.3rem}.recordings2-metric{padding:.5rem .3rem}}
`;

  function platform() {
    return global.VdrSuitePlatform || null;
  }

  function clientApi() {
    const boundary = platform();
    if (boundary && typeof boundary.getClientApi === 'function') {
      return boundary.getClientApi();
    }
    return global.VdrSuiteClientApi || null;
  }

  function mountTarget() {
    const boundary = platform();
    if (boundary && typeof boundary.getMountTarget === 'function') {
      return boundary.getMountTarget('recordings2') ||
        boundary.getMountTarget('detail') ||
        document.getElementById('detail-data');
    }
    return document.getElementById('detail-data');
  }

  function selectedBackendId() {
    const boundary = platform();
    if (boundary && typeof boundary.getSelectedBackendId === 'function') {
      const value = String(boundary.getSelectedBackendId() || '').trim();
      if (value) return value;
    }
    const selected = document.querySelector('.backend-card.selected');
    return selected && selected.dataset.backendId ? String(selected.dataset.backendId) : 'default';
  }

  function publicPath(path) {
    const value = text(path);
    const resolver = global.VdrSuitePublicUrl;
    return value && resolver && typeof resolver.resolvePath === 'function'
      ? resolver.resolvePath(value)
      : value;
  }

  function text(value) {
    return String(value === undefined || value === null ? '' : value).trim();
  }

  function number(value, fallback) {
    const result = Number(value);
    return Number.isFinite(result) ? result : (fallback || 0);
  }

  function first(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
        return object[key];
      }
    }
    return fallback;
  }

  function normalizePath(value) {
    return text(value)
      .replace(/^\/+|\/+$/g, '')
      .split('/')
      .map(part => part.trim())
      .filter(Boolean)
      .join('/');
  }

  function decodeDisplayText(value) {
    let decoded = text(value);
    for (let pass = 0; pass < 6; pass += 1) {
      const next = decoded.replace(/#([0-9A-Fa-f]{2})/g, function (_, hex) {
        return String.fromCharCode(parseInt(hex, 16));
      });
      if (next === decoded) break;
      decoded = next;
    }
    return decoded.replace(/_/g, ' ').replace(/^%+/, '').trim();
  }

  function folderList(data) {
    return data && Array.isArray(data.folders) ? data.folders : [];
  }

  function recordingList(data) {
    return data && Array.isArray(data.recordings) ? data.recordings : [];
  }

  function metadata(recording) {
    return recording && recording.metadata && typeof recording.metadata === 'object'
      ? recording.metadata
      : {};
  }

  function nestedMetadata(recording, name) {
    const value = metadata(recording)[name];
    return value && typeof value === 'object' ? value : {};
  }

  function presentation(recording) { return nestedMetadata(recording, 'presentation'); }
  function provider(recording) { return nestedMetadata(recording, 'provider'); }
  function nativeMetadata(recording) { return nestedMetadata(recording, 'native'); }
  function artwork(recording) { return nestedMetadata(recording, 'artwork'); }
  function manualAssignment(recording) { return nestedMetadata(recording, 'manualAssignment'); }

  function recordingPathTitle(recording) {
    const raw = text(first(
      recording,
      ['path', 'backendNativeId', 'nativePath', 'fileName', 'directory'],
      ''
    )).replace(/\\/g, '/').replace(/~+/g, '/').replace(/\/+$/g, '');
    const parts = raw.split('/').map(function (part) { return part.trim(); }).filter(Boolean);
    if (parts.length && /\.rec$/i.test(parts[parts.length - 1])) parts.pop();
    return parts.length ? decodeDisplayText(parts[parts.length - 1]) : '';
  }

  function recordingNativeTitle(recording) {
    const raw = text(first(recording, ['title', 'name', 'displayName'], ''))
      .replace(/\\/g, '/')
      .replace(/~/g, '/');
    const parts = raw.split('/').map(function (part) { return part.trim(); }).filter(Boolean);
    return parts.length ? decodeDisplayText(parts[parts.length - 1]) : '';
  }

  function recordingMetadataTitle(recording) {
    return decodeDisplayText(first(
      presentation(recording),
      ['title'],
      first(provider(recording), ['seriesTitle', 'title'], '')
    ));
  }

  function recordingTitle(recording) {
    const manualTitle = manualAssignment(recording).active === true
      ? recordingMetadataTitle(recording)
      : '';
    return manualTitle ||
      recordingPathTitle(recording) ||
      recordingNativeTitle(recording) ||
      recordingMetadataTitle(recording) ||
      'Aufnahme';
  }

  function recordingSubtitle(recording) {
    return decodeDisplayText(first(
      presentation(recording),
      ['subtitle', 'seasonEpisode'],
      first(provider(recording), ['episodeTitle', 'tagline'], first(nativeMetadata(recording), ['shortText'], ''))
    ));
  }

  function recordingSummary(recording) {
    return text(first(
      presentation(recording),
      ['summary'],
      first(provider(recording), ['overview'], first(nativeMetadata(recording), ['description', 'shortText'], ''))
    ));
  }

  function recordingPosterUrl(recording) {
    return text(first(
      presentation(recording),
      ['posterUrl'],
      first(artwork(recording), ['preferredUrl'], '')
    ));
  }

  function recordingPlaceholderVariant(recording) {
    return Math.abs(number(first(presentation(recording), ['placeholderVariant'], 0), 0)) % 6;
  }

  function formatStart(value) {
    const numeric = Number(value);
    if (Number.isFinite(numeric) && numeric > 1000000000) {
      return new Date(numeric * 1000).toLocaleString('de-DE', {
        day: '2-digit', month: '2-digit', year: 'numeric', hour: '2-digit', minute: '2-digit'
      });
    }
    return text(value) || 'Unbekannt';
  }

  function formatDuration(value) {
    const seconds = number(value, 0);
    if (seconds <= 0) return 'Dauer unbekannt';
    const minutes = Math.round(seconds / 60);
    if (minutes < 60) return String(minutes) + ' min';
    const hours = Math.floor(minutes / 60);
    const rest = minutes % 60;
    return String(hours) + ' h' + (rest ? ' ' + String(rest) + ' min' : '');
  }

  function formatSize(value) {
    const megabytes = number(value, 0);
    if (megabytes <= 0) return 'Größe unbekannt';
    return megabytes >= 1024
      ? (megabytes / 1024).toFixed(1) + ' GB'
      : String(Math.round(megabytes)) + ' MB';
  }

  function installStyles() {
    if (document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = CSS;
    document.head.appendChild(style);
  }

  function addText(element, value) {
    element.textContent = String(value);
    return element;
  }

  function node(tag, className, value) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (value !== undefined && value !== null) element.textContent = String(value);
    return element;
  }

  function createButton(label, action, className) {
    const button = node('button', className || '', label);
    button.type = 'button';
    button.addEventListener('click', action);
    return button;
  }

  function createPoster(recording, className) {
    const poster = document.createElement('div');
    poster.className = className || 'recordings2-poster';
    poster.dataset.variant = String(recordingPlaceholderVariant(recording));
    const url = recordingPosterUrl(recording);
    if (!url) {
      poster.textContent = '▶';
      return poster;
    }
    const image = document.createElement('img');
    image.src = publicPath(url);
    image.alt = 'Poster zu ' + recordingTitle(recording);
    image.loading = 'lazy';
    image.addEventListener('error', function () {
      image.remove();
      poster.textContent = '▶';
    });
    poster.appendChild(image);
    return poster;
  }

  global.VdrSuiteRecordings2Shared = Object.freeze({
    PAGE_SIZE,
    platform,
    clientApi,
    mountTarget,
    selectedBackendId,
    publicPath,
    text,
    number,
    first,
    normalizePath,
    decodeDisplayText,
    folderList,
    recordingList,
    metadata,
    presentation,
    provider,
    nativeMetadata,
    artwork,
    manualAssignment,
    recordingPathTitle,
    recordingNativeTitle,
    recordingMetadataTitle,
    recordingTitle,
    recordingSubtitle,
    recordingSummary,
    recordingPosterUrl,
    recordingPlaceholderVariant,
    formatStart,
    formatDuration,
    formatSize,
    installStyles,
    addText,
    node,
    createButton,
    createPoster
  });
}(window));