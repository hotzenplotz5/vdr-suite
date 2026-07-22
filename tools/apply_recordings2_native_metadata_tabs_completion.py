#!/usr/bin/env python3

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class PatchError(RuntimeError):
    pass


class Editor:
    def __init__(self) -> None:
        self.files: dict[Path, str] = {}
        self.changed: set[Path] = set()

    def read(self, relative: str) -> str:
        path = ROOT / relative
        if path not in self.files:
            if not path.is_file():
                raise PatchError(f"Datei fehlt: {relative}")
            self.files[path] = path.read_text(encoding="utf-8")
        return self.files[path]

    def replace_once(self, relative: str, old: str, new: str) -> None:
        path = ROOT / relative
        content = self.read(relative)
        count = content.count(old)
        if count != 1:
            raise PatchError(
                f"Anker in {relative} wurde {count}-mal statt einmal gefunden"
            )
        self.files[path] = content.replace(old, new, 1)
        self.changed.add(path)

    def create(self, relative: str, content: str) -> None:
        path = ROOT / relative
        if path.exists():
            existing = path.read_text(encoding="utf-8")
            if existing != content:
                raise PatchError(
                    f"Neue Datei existiert bereits mit anderem Inhalt: {relative}"
                )
            return
        self.files[path] = content
        self.changed.add(path)

    def write(self) -> None:
        for path in sorted(self.changed):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(self.files[path], encoding="utf-8")

    def report(self) -> None:
        for path in sorted(self.changed):
            print(path.relative_to(ROOT))


def git_output(*arguments: str) -> str:
    return subprocess.check_output(
        ["git", *arguments],
        cwd=ROOT,
        text=True,
    ).strip()


def verify_repository() -> None:
    branch = git_output("branch", "--show-current")
    if branch != "agent/recordings2-native-metadata-tabs":
        raise PatchError(
            "Falscher Branch. Erwartet wird "
            "agent/recordings2-native-metadata-tabs"
        )

    status = git_output("status", "--porcelain")
    if status:
        raise PatchError(
            "Arbeitsverzeichnis ist nicht sauber. Vor dem Patch nicht fortfahren."
        )

    required = {
        "api/rest/include/VdrRecordingFolderController.h": "getMetadataImage(",
        "core/vdr/include/VdrRecordingNativeMetadataRepository.h":
            "findByBackendNativeId(",
        "core/vdr/include/VdrRecordingNativeMetadataPublicJsonSerializer.h":
            "class VdrRecordingNativeMetadataPublicJsonSerializer",
    }

    for relative, token in required.items():
        content = (ROOT / relative).read_text(encoding="utf-8")
        if token not in content:
            raise PatchError(
                f"Unvollständiger Ausgangsbranch: {token} fehlt in {relative}"
            )


def patch_router(editor: Editor) -> None:
    old = '''    if (path == "/api/vdr/recordings/query")
    {
'''

    new = '''    if (path == "/api/recordings/metadata/image" ||
        path == "/api/vdr/recordings/metadata/image")
    {
        if (vdrRecordingFolderController_ == nullptr)
        {
            ApiResponse response;
            response.statusCode = 503;
            response.contentType = "application/json";
            response.body =
                "{\\"error\\":\\"recording metadata unavailable\\"}";
            return response;
        }

        return vdrRecordingFolderController_->getMetadataImage(
            normalizeBackendId(queryParameters.get("backend")),
            queryParameters.get("backendNativeId"),
            queryParameters.get("kind"),
            queryParameters.getInt("index", 0));
    }

    if (path == "/api/recordings/metadata" ||
        path == "/api/vdr/recordings/metadata")
    {
        if (vdrRecordingFolderController_ == nullptr)
        {
            ApiResponse response;
            response.statusCode = 503;
            response.contentType = "application/json";
            response.body =
                "{\\"error\\":\\"recording metadata unavailable\\"}";
            return response;
        }

        return vdrRecordingFolderController_->getMetadata(
            normalizeBackendId(queryParameters.get("backend")),
            queryParameters.get("backendNativeId"));
    }

    if (path == "/api/vdr/recordings/query")
    {
'''

    editor.replace_once("api/rest/src/ApiRouter.cpp", old, new)


def patch_daemon_runtime(editor: Editor) -> None:
    old = '''    vdrRecordingFolderController_ = std::make_unique<VdrRecordingFolderController>(
        *vdrRecordingCacheRepository_);
'''

    new = '''    vdrRecordingFolderController_ =
        std::make_unique<VdrRecordingFolderController>(
            *vdrRecordingCacheRepository_,
            [this](
                const std::string& backendId,
                const std::string& backendNativeId)
            {
                const std::string normalizedBackendId =
                    backendId.empty()
                        ? "default"
                        : backendId;

                for (const auto& backendRuntimeContext :
                     backendRuntimeContexts_)
                {
                    if (!backendRuntimeContext ||
                        backendRuntimeContext->backendId !=
                            normalizedBackendId ||
                        !backendRuntimeContext
                             ->recordingMetadataRepository)
                    {
                        continue;
                    }

                    return backendRuntimeContext
                        ->recordingMetadataRepository
                        ->findByBackendNativeId(
                            normalizedBackendId,
                            backendNativeId);
                }

                return VdrRecordingNativeMetadataRecord{};
            });
'''

    editor.replace_once("core/daemon/src/DaemonRuntime.cpp", old, new)


def patch_vdr_sources(editor: Editor) -> None:
    old = '''        core/vdr/src/VdrRecordingMetadataJsonSerializer.cpp \\
        core/vdr/src/EpgArtworkRepository.cpp \\
'''

    new = '''        core/vdr/src/VdrRecordingMetadataJsonSerializer.cpp \\
        core/vdr/src/VdrRecordingNativeMetadataPublicJsonSerializer.cpp \\
        core/vdr/src/EpgArtworkRepository.cpp \\
'''

    editor.replace_once("mk/vdr-sources.mk", old, new)


def create_metadata_detail_module(editor: Editor) -> None:
    content = r'''// Recordings 2 native SuiteBridge/TVScraper metadata detail owner.
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

  function renderScraper(panel, value) {
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

  function renderCast(panel, value, backendId) {
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

  function renderImages(panel, value, recording) {
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

  function fetchMetadata(recording, backendId) {
    const backendNativeId = text(first(recording, ['backendNativeId'], ''));
    if (!backendNativeId) {
      return Promise.reject(new Error('Die Aufnahme besitzt keine stabile Backend-Identität.'));
    }

    const api = clientApi();
    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('Client API für Aufnahme-Metadaten ist nicht verfügbar.'));
    }

    return api.requestJson('/api/vdr/recordings/metadata', {
      query: {
        backend: text(backendId) || 'default',
        backendNativeId: backendNativeId,
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function enhance(root, recording, backendId) {
    if (!root || !recording || root.dataset.recordings2MetadataDetail === 'true') return root;
    ensureStyles();
    root.dataset.recordings2MetadataDetail = 'true';

    const header = root.querySelector('.recordings2-header');
    const hero = root.querySelector('.recordings2-detail-hero');
    const details = root.querySelector('.recordings2-detail-grid');
    if (!hero || !details) return root;

    const tabs = node('nav', 'recordings2-metadata-tabs');
    tabs.setAttribute('role', 'tablist');
    tabs.setAttribute('aria-label', 'Aufnahmedetailbereiche');

    const panels = {
      recording: node('section', 'recordings2-metadata-panel'),
      scraper: node('section', 'recordings2-metadata-panel'),
      cast: node('section', 'recordings2-metadata-panel'),
      images: node('section', 'recordings2-metadata-panel')
    };
    panels.recording.appendChild(hero);
    panels.recording.appendChild(details);
    panels.scraper.hidden = true;
    panels.cast.hidden = true;
    panels.images.hidden = true;

    const buttons = {};
    function addTab(name, label, disabled) {
      const button = node('button', 'recordings2-metadata-tab', label);
      button.type = 'button';
      button.disabled = Boolean(disabled);
      button.setAttribute('role', 'tab');
      button.setAttribute('aria-selected', name === 'recording' ? 'true' : 'false');
      buttons[name] = button;
      tabs.appendChild(button);
    }

    addTab('recording', 'Aufnahme', false);
    addTab('scraper', 'Scraper', true);
    addTab('cast', 'Schauspieler', true);
    addTab('images', 'Bilder', true);

    function activate(name) {
      Object.keys(panels).forEach(function (panelName) {
        panels[panelName].hidden = panelName !== name;
      });
      Object.keys(buttons).forEach(function (buttonName) {
        buttons[buttonName].setAttribute('aria-selected', buttonName === name ? 'true' : 'false');
      });
    }

    Object.keys(buttons).forEach(function (name) {
      buttons[name].addEventListener('click', function () {
        if (!buttons[name].disabled) activate(name);
      });
    });

    if (header && header.nextSibling) root.insertBefore(tabs, header.nextSibling);
    else root.insertBefore(tabs, root.firstChild || null);

    const loading = status('Persistierte SuiteBridge-/TVScraper-Metadaten werden geladen …', false);
    root.insertBefore(loading, panels.recording);
    root.appendChild(panels.recording);
    root.appendChild(panels.scraper);
    root.appendChild(panels.cast);
    root.appendChild(panels.images);

    fetchMetadata(recording, backendId).then(function (value) {
      if (!value || value.available !== true) {
        loading.textContent = 'Für diese Aufnahme sind keine erweiterten TVScraper-Daten gespeichert.';
        return;
      }

      loading.remove();
      renderScraper(panels.scraper, value);
      renderCast(panels.cast, value, text(backendId) || 'default');
      renderImages(panels.images, value, recording);
      buttons.scraper.disabled = false;
      buttons.cast.disabled = !(Array.isArray(value.people) && value.people.length);
      const preferredAvailable = value.preferredArtwork &&
        value.preferredArtwork.available === true &&
        isPublicMetadataImageUrl(value.preferredArtwork.url);
      const galleryAvailable = Array.isArray(value.images) && value.images.some(function (entry) {
        return entry && entry.image && entry.image.available === true &&
          isPublicMetadataImageUrl(entry.image.url);
      });
      buttons.images.disabled = !(preferredAvailable || galleryAvailable);
      root.dataset.recordings2MetadataAvailable = 'true';
    }).catch(function (error) {
      loading.classList.add('error');
      loading.setAttribute('role', 'alert');
      loading.textContent = 'Aufnahme-Metadaten konnten nicht geladen werden: ' +
        String(error && error.message ? error.message : error);
    });

    return root;
  }

  global.VdrSuiteRecordings2MetadataDetail = Object.freeze({
    enhance: enhance,
    formatDate: formatDate,
    isPublicMetadataImageUrl: isPublicMetadataImageUrl,
    mediaTypeLabel: mediaTypeLabel,
    orientationLabel: orientationLabel,
    roleLabel: roleLabel
  });
}(window));
'''

    editor.create("web/frontend/recordings2-metadata-detail.js", content)


def patch_recordings2(editor: Editor) -> None:
    old = '''    root.appendChild(details);
    target.appendChild(root);
  }

  function render() {
'''

    new = '''    root.appendChild(details);
    target.appendChild(root);

    const metadataDetail = global.VdrSuiteRecordings2MetadataDetail;
    if (metadataDetail && typeof metadataDetail.enhance === 'function') {
      metadataDetail.enhance(root, recording, state.backendId);
    }
  }

  function render() {
'''

    editor.replace_once("web/frontend/recordings2.js", old, new)


def patch_loader(editor: Editor) -> None:
    old = '''function startVdrSuiteDeferredFrontendRuntimes() {
'''

    new = '''function loadVdrSuiteRecordings2Runtime() {
  return loadVdrSuiteDeferredRuntime(
    'vdr-suite-recordings2-metadata-detail-runtime',
    '/frontend/recordings2-metadata-detail.js',
    () => Boolean(window.VdrSuiteRecordings2MetadataDetail)
  ).then(() => loadVdrSuiteDeferredRuntime(
    'vdr-suite-recordings2-runtime',
    '/frontend/recordings2.js',
    () => Boolean(window.VdrSuiteRecordings2)
  ));
}

function startVdrSuiteDeferredFrontendRuntimes() {
'''

    editor.replace_once(
        "web/frontend/platform/deferred-runtime-loader.js",
        old,
        new,
    )

    old = '''  loadVdrSuiteDeferredRuntime(
    'vdr-suite-recordings2-runtime',
    '/frontend/recordings2.js',
    () => Boolean(window.VdrSuiteRecordings2)
  ).catch(error => {
    console.error('VDR-Suite Recordings 2 runtime failed', error);
  });
'''

    new = '''  loadVdrSuiteRecordings2Runtime().catch(error => {
    console.error('VDR-Suite Recordings 2 runtime failed', error);
  });
'''

    editor.replace_once(
        "web/frontend/platform/deferred-runtime-loader.js",
        old,
        new,
    )

    old = '''    start: startVdrSuiteDeferredFrontendRuntimes,
    loadEpgDetail: loadVdrSuiteEpgDetailRuntime
'''

    new = '''    start: startVdrSuiteDeferredFrontendRuntimes,
    loadEpgDetail: loadVdrSuiteEpgDetailRuntime,
    loadRecordings2: loadVdrSuiteRecordings2Runtime
'''

    editor.replace_once(
        "web/frontend/platform/deferred-runtime-loader.js",
        old,
        new,
    )


def patch_http_server(editor: Editor) -> None:
    old = '''        path == "/frontend/recordings2.js" ||
        path == "/frontend/recording-trash-ux.js" ||
'''

    new = '''        path == "/frontend/recordings2.js" ||
        path == "/frontend/recordings2-metadata-detail.js" ||
        path == "/frontend/recording-trash-ux.js" ||
'''

    editor.replace_once("core/http/src/TestHttpServer.cpp", old, new)

    old = '''    if (path == "/frontend/recordings2.js")
    {
        return makeFrontendAssetResponse(
            "recordings2.js",
            "application/javascript; charset=utf-8");
    }

    if (path == "/frontend/recording-trash-ux.js")
'''

    new = '''    if (path == "/frontend/recordings2.js")
    {
        return makeFrontendAssetResponse(
            "recordings2.js",
            "application/javascript; charset=utf-8");
    }

    if (path == "/frontend/recordings2-metadata-detail.js")
    {
        return makeFrontendAssetResponse(
            "recordings2-metadata-detail.js",
            "application/javascript; charset=utf-8");
    }

    if (path == "/frontend/recording-trash-ux.js")
'''

    editor.replace_once("core/http/src/TestHttpServer.cpp", old, new)


def patch_recordings2_makefile(editor: Editor) -> None:
    old = '''\t$(INSTALL) -m 0644 web/frontend/recordings2.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2.js
'''

    new = '''\t$(INSTALL) -m 0644 web/frontend/recordings2.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2.js
\t$(INSTALL) -m 0644 web/frontend/recordings2-metadata-detail.js $(DESTDIR)$(DATADIR)/web/frontend/recordings2-metadata-detail.js
'''

    editor.replace_once("mk/recordings2.mk", old, new)

    old = '''\tnode --check web/frontend/recordings2.js
\tnode web/frontend/tests/test_recordings2_runtime.js
'''

    new = '''\tnode --check web/frontend/recordings2.js
\tnode --check web/frontend/recordings2-metadata-detail.js
\tnode web/frontend/tests/test_recordings2_runtime.js
\tnode web/frontend/tests/test_recordings2_metadata_detail.js
'''

    editor.replace_once("mk/recordings2.mk", old, new)

    old = '''\ttest -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2.js
\tnode --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2.js
'''

    new = '''\ttest -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2.js
\ttest -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-metadata-detail.js
\tnode --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2.js
\tnode --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/recordings2-metadata-detail.js
'''

    editor.replace_once("mk/recordings2.mk", old, new)


def create_frontend_test(editor: Editor) -> None:
    content = r''' 'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const document = {
  getElementById() { return null; },
  head: { appendChild() {} },
  createElement() {
    return {
      className: '',
      dataset: {},
      hidden: false,
      children: [],
      classList: { add() {} },
      setAttribute() {},
      addEventListener() {},
      appendChild(child) { this.children.push(child); return child; },
      replaceChildren() { this.children = Array.from(arguments); },
      remove() {},
      querySelector() { return null; }
    };
  }
};

const window = {};
const context = vm.createContext({
  window,
  document,
  console,
  Date,
  Number,
  String,
  Object,
  Array,
  Promise
});

const source = fs.readFileSync(
  'web/frontend/recordings2-metadata-detail.js',
  'utf8'
);
vm.runInContext(source, context, {
  filename: 'recordings2-metadata-detail.js'
});

const api = window.VdrSuiteRecordings2MetadataDetail;
assert.ok(api);
assert.strictEqual(api.roleLabel('actor'), 'Schauspiel');
assert.strictEqual(api.roleLabel('director'), 'Regie');
assert.strictEqual(api.mediaTypeLabel('movie'), 'Film');
assert.strictEqual(api.mediaTypeLabel('series'), 'Serie');
assert.strictEqual(api.orientationLabel('portrait'), 'Hochformat');
assert.strictEqual(api.formatDate('2026-07-22'), '22.07.2026');
assert.strictEqual(
  api.isPublicMetadataImageUrl(
    '/api/vdr/recordings/metadata/image?backend=default'
  ),
  true
);
assert.strictEqual(
  api.isPublicMetadataImageUrl(
    'https://image.tmdb.org/example.jpg'
  ),
  false
);

console.log('recordings2 metadata detail ok');
'''

    editor.create(
        "web/frontend/tests/test_recordings2_metadata_detail.js",
        content,
    )


def patch_runtime_checker(editor: Editor) -> None:
    old = '''recordings2 = (ROOT / 'web/frontend/recordings2.js').read_text(encoding='utf-8')
loader = (ROOT / 'web/frontend/platform/deferred-runtime-loader.js').read_text(encoding='utf-8')
'''

    new = '''recordings2 = (ROOT / 'web/frontend/recordings2.js').read_text(encoding='utf-8')
metadata_detail = (ROOT / 'web/frontend/recordings2-metadata-detail.js').read_text(encoding='utf-8')
loader = (ROOT / 'web/frontend/platform/deferred-runtime-loader.js').read_text(encoding='utf-8')
router = (ROOT / 'api/rest/src/ApiRouter.cpp').read_text(encoding='utf-8')
daemon = (ROOT / 'core/daemon/src/DaemonRuntime.cpp').read_text(encoding='utf-8')
'''

    editor.replace_once(
        "tools/check_recordings2_runtime_wiring.py",
        old,
        new,
    )

    old = '''    'renderDetail',
)
'''

    new = '''    'renderDetail',
    'VdrSuiteRecordings2MetadataDetail',
    'metadataDetail.enhance',
)
'''

    editor.replace_once(
        "tools/check_recordings2_runtime_wiring.py",
        old,
        new,
    )

    old = '''for forbidden in ('VdrSuiteRecordingBrowser', 'configureRecordingBrowser', 'renderRecordingsThroughModule'):
'''

    new = '''required_metadata_tokens = (
    'global.VdrSuiteRecordings2MetadataDetail',
    "'/api/vdr/recordings/metadata'",
    "'Scraper'",
    "'Schauspieler'",
    "'Bilder'",
    'fetchClientRecordingPersons',
    'isPublicMetadataImageUrl',
)
for token in required_metadata_tokens:
    if token not in metadata_detail:
        raise SystemExit(f'missing Recordings 2 metadata detail contract: {token}')

required_backend_tokens = (
    'path == "/api/vdr/recordings/metadata"',
    'path == "/api/vdr/recordings/metadata/image"',
    'getMetadataImage(',
)
for token in required_backend_tokens:
    if token not in router:
        raise SystemExit(f'missing recording metadata route contract: {token}')

for token in ('findByBackendNativeId(', 'recordingMetadataRepository'):
    if token not in daemon:
        raise SystemExit(f'missing recording metadata daemon wiring: {token}')

for forbidden in ('VdrSuiteRecordingBrowser', 'configureRecordingBrowser', 'renderRecordingsThroughModule'):
'''

    editor.replace_once(
        "tools/check_recordings2_runtime_wiring.py",
        old,
        new,
    )

    old = '''required_loader_tokens = (
    "'/frontend/recordings2.js'",
    'window.VdrSuiteRecordings2',
    "'vdr-suite-recordings2-runtime'",
)
'''

    new = '''required_loader_tokens = (
    "'/frontend/recordings2-metadata-detail.js'",
    'window.VdrSuiteRecordings2MetadataDetail',
    "'vdr-suite-recordings2-metadata-detail-runtime'",
    "'/frontend/recordings2.js'",
    'window.VdrSuiteRecordings2',
    "'vdr-suite-recordings2-runtime'",
)
'''

    editor.replace_once(
        "tools/check_recordings2_runtime_wiring.py",
        old,
        new,
    )

    old = '''if 'path == "/frontend/recordings2.js"' not in server:
    raise SystemExit('HTTP server does not allow /frontend/recordings2.js')
if '"recordings2.js"' not in server:
    raise SystemExit('HTTP server does not serve recordings2.js')
'''

    new = '''if 'path == "/frontend/recordings2.js"' not in server:
    raise SystemExit('HTTP server does not allow /frontend/recordings2.js')
if '"recordings2.js"' not in server:
    raise SystemExit('HTTP server does not serve recordings2.js')
if 'path == "/frontend/recordings2-metadata-detail.js"' not in server:
    raise SystemExit('HTTP server does not allow Recordings 2 metadata detail')
if '"recordings2-metadata-detail.js"' not in server:
    raise SystemExit('HTTP server does not serve Recordings 2 metadata detail')
'''

    editor.replace_once(
        "tools/check_recordings2_runtime_wiring.py",
        old,
        new,
    )


def create_serializer_test(editor: Editor) -> None:
    content = r'''#include "VdrRecordingNativeMetadataPublicJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    VdrRecordingNativeMetadataRecord record;
    record.backendId = "default";
    record.backendNativeId =
        "/srv/vdr/video/Filme/Inferno/2026-07-20.20.15.1-0.rec";
    record.recordingKey = "recording-key";
    record.contentState = "found";

    record.metadata.found = true;
    record.metadata.provider = "tvscraper";
    record.metadata.mediaType = "movie";
    record.metadata.providerId = 207932;
    record.metadata.title = "Inferno";
    record.metadata.overview =
        "Robert Langdon folgt einer gefährlichen Spur.";
    record.metadata.releaseDate = "2016-10-13";
    record.metadata.imdbId = "tt3062096";
    record.metadata.voteAverage = 6.1;
    record.metadata.voteCount = 6400;
    record.metadata.genres = {"Thriller", "Mystery"};

    record.metadata.preferredArtwork.available = true;
    record.metadata.preferredArtwork.provider = "tvscraper";
    record.metadata.preferredArtwork.path =
        "/var/cache/vdr/plugins/tvscraper/movies/207932/poster.jpg";
    record.metadata.preferredArtwork.width = 1000;
    record.metadata.preferredArtwork.height = 1500;

    VdrRecordingNativePerson person;
    person.role = "actor";
    person.name = "Tom Hanks";
    person.characterName = "Robert Langdon";
    person.image.available = true;
    person.image.provider = "tvscraper";
    person.image.path =
        "/var/cache/vdr/plugins/tvscraper/actors/tom-hanks.jpg";
    record.metadata.people.push_back(person);

    VdrRecordingNativeArtwork image;
    image.available = true;
    image.provider = "tvscraper";
    image.path =
        "/var/cache/vdr/plugins/tvscraper/movies/207932/fanart.jpg";
    image.orientation = "landscape";
    record.metadata.images.push_back(image);

    const std::string json =
        VdrRecordingNativeMetadataPublicJsonSerializer().serialize(record);

    assert(json.find("\\"available\\":true") != std::string::npos);
    assert(json.find("\\"provider\\":\\"tvscraper\\"") != std::string::npos);
    assert(json.find("\\"title\\":\\"Inferno\\"") != std::string::npos);
    assert(json.find("\\"name\\":\\"Tom Hanks\\"") != std::string::npos);
    assert(json.find("\\"characterName\\":\\"Robert Langdon\\"") !=
        std::string::npos);
    assert(json.find(
        "/api/vdr/recordings/metadata/image?backend=default") !=
        std::string::npos);
    assert(json.find(
        "%2Fsrv%2Fvdr%2Fvideo%2FFilme%2FInferno") !=
        std::string::npos);
    assert(json.find(
        "/var/cache/vdr/plugins/tvscraper") ==
        std::string::npos);

    const VdrRecordingNativeMetadataRecord missing;
    assert(
        VdrRecordingNativeMetadataPublicJsonSerializer()
            .serialize(missing) ==
        "{\\"available\\":false,\\"status\\":\\"not-found\\"}");

    std::cout
        << "test_vdr_recording_native_metadata_public_json_serializer passed"
        << std::endl;

    return 0;
}
'''

    editor.create(
        "core/vdr/tests/"
        "test_vdr_recording_native_metadata_public_json_serializer.cpp",
        content,
    )


def patch_folder_controller_test(editor: Editor) -> None:
    old = '''    VdrRecordingFolderController controller(repository);
'''

    new = '''    VdrRecordingFolderController controller(
        repository,
        [](
            const std::string& backendId,
            const std::string& backendNativeId)
        {
            VdrRecordingNativeMetadataRecord record;
            if (backendId != "default" ||
                backendNativeId.find("Movie") == std::string::npos)
            {
                return record;
            }

            record.backendId = backendId;
            record.backendNativeId = backendNativeId;
            record.recordingKey = "movie-key";
            record.contentState = "found";
            record.metadata.found = true;
            record.metadata.provider = "tvscraper";
            record.metadata.mediaType = "movie";
            record.metadata.title = "Movie";

            VdrRecordingNativePerson person;
            person.role = "actor";
            person.name = "Tom Hanks";
            person.characterName = "Robert Langdon";
            record.metadata.people.push_back(person);

            return record;
        });
'''

    editor.replace_once(
        "api/rest/tests/test_vdr_recording_folder_controller.cpp",
        old,
        new,
    )

    old = '''    assert(contains(series.body, "\\"metadata\\":{"));

    std::cout
'''

    new = '''    assert(contains(series.body, "\\"metadata\\":{"));

    const ApiResponse nativeMetadata = controller.getMetadata(
        "default",
        "/srv/vdr/video/Movies/Movie/2026-07-02.20.15.1-0.rec");
    assert(nativeMetadata.statusCode == 200);
    assert(contains(nativeMetadata.body, "\\"available\\":true"));
    assert(contains(nativeMetadata.body, "\\"provider\\":\\"tvscraper\\""));
    assert(contains(nativeMetadata.body, "\\"name\\":\\"Tom Hanks\\""));
    assert(contains(nativeMetadata.body, "\\"characterName\\":\\"Robert Langdon\\""));

    const ApiResponse missingMetadata = controller.getMetadata(
        "default",
        "/srv/vdr/video/Movies/Missing.rec");
    assert(missingMetadata.statusCode == 200);
    assert(contains(missingMetadata.body, "\\"available\\":false"));

    std::cout
'''

    editor.replace_once(
        "api/rest/tests/test_vdr_recording_folder_controller.cpp",
        old,
        new,
    )


def patch_recording_metadata_tests(editor: Editor) -> None:
    old = '''\ttest-vdr-recording-metadata-json-serializer \\
\ttest-vdr-recording-artwork-service \\
'''

    new = '''\ttest-vdr-recording-metadata-json-serializer \\
\ttest-vdr-recording-native-metadata-public-json-serializer \\
\ttest-vdr-recording-artwork-service \\
'''

    editor.replace_once("mk/recording-metadata-tests.mk", old, new)
    editor.replace_once("mk/recording-metadata-tests.mk", old, new)

    old = '''test-vdr-recording-artwork-service:
'''

    new = '''test-vdr-recording-native-metadata-public-json-serializer:
\t$(BUILD_CXX) $(CXXFLAGS) \\
\t\tcore/vdr/src/VdrRecordingNativeMetadataPublicJsonSerializer.cpp \\
\t\tcore/vdr/tests/test_vdr_recording_native_metadata_public_json_serializer.cpp \\
\t\t-o $(BUILD_DIR)/test_vdr_recording_native_metadata_public_json_serializer
\t$(BUILD_DIR)/test_vdr_recording_native_metadata_public_json_serializer

test-vdr-recording-artwork-service:
'''

    editor.replace_once("mk/recording-metadata-tests.mk", old, new)

    old = '''test-vdr-recording-folder-controller: CXXFLAGS += \\
\tcore/vdr/src/VdrRecordingMetadataCacheCodec.cpp \\
\tcore/vdr/src/VdrRecordingArtworkIdentity.cpp
'''

    new = '''test-vdr-recording-folder-controller: CXXFLAGS += \\
\tcore/vdr/src/VdrRecordingMetadataCacheCodec.cpp \\
\tcore/vdr/src/VdrRecordingArtworkIdentity.cpp \\
\tcore/vdr/src/VdrRecordingNativeMetadataPublicJsonSerializer.cpp \\
\tcore/vdr/src/EpgArtworkRepository.cpp \\
\tapi/rest/src/EpgArtworkController.cpp
'''

    editor.replace_once("mk/recording-metadata-tests.mk", old, new)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--check",
        action="store_true",
        help="Nur Ausgangsstand und Patchanker prüfen",
    )
    arguments = parser.parse_args()

    verify_repository()
    editor = Editor()

    patch_router(editor)
    patch_daemon_runtime(editor)
    patch_vdr_sources(editor)
    create_metadata_detail_module(editor)
    patch_recordings2(editor)
    patch_loader(editor)
    patch_http_server(editor)
    patch_recordings2_makefile(editor)
    create_frontend_test(editor)
    patch_runtime_checker(editor)
    create_serializer_test(editor)
    patch_folder_controller_test(editor)
    patch_recording_metadata_tests(editor)

    print("Betroffene Dateien:")
    editor.report()

    if arguments.check:
        print("DRY-RUN OK")
        return 0

    editor.write()
    print("PATCH ANGEWENDET")
    print()
    print("Danach ausführen:")
    print("  git diff --check")
    print("  make test-vdr-recording-native-metadata-public-json-serializer")
    print("  make test-vdr-recording-folder-controller")
    print("  make test-recordings2-runtime")
    print("  make test-api-router")
    print("  make daemon")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except PatchError as error:
        print(f"FEHLER: {error}")
        raise SystemExit(1)
