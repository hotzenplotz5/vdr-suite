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
.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-header,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-header,.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-header{display:none}
.recordings2-hero-page>.recordings2-detail-hero{position:relative;z-index:2;display:grid;grid-template-columns:minmax(12rem,18rem) minmax(0,1fr);gap:clamp(1.5rem,3vw,3rem);align-items:end;min-height:min(78vh,58rem);margin:0;padding:clamp(6rem,11vw,10rem) clamp(1.25rem,5vw,5rem) clamp(3rem,6vw,5.5rem);border:0;border-radius:0;background:linear-gradient(90deg,rgba(7,9,12,.98) 0%,rgba(7,9,12,.9) 32%,rgba(7,9,12,.46) 63%,rgba(7,9,12,.2) 100%)}
.recordings2-hero-page>.recordings2-detail-hero::before{content:'';position:absolute;inset:0;z-index:-3;background-image:var(--recordings2-hero-backdrop);background-size:cover;background-position:center 24%;opacity:.62;filter:saturate(.92);transform:scale(1.018)}
.recordings2-hero-page>.recordings2-detail-hero::after{content:'';position:absolute;inset:0;z-index:-2;background:linear-gradient(90deg,rgba(7,9,12,.96) 0%,rgba(7,9,12,.82) 33%,rgba(7,9,12,.34) 68%,rgba(7,9,12,.16) 100%),linear-gradient(180deg,rgba(7,9,12,.08) 0%,rgba(7,9,12,.04) 58%,#090b0f 100%)}
.recordings2-hero-page .recordings2-detail-poster{align-self:end;overflow:hidden;border:1px solid rgba(255,255,255,.12);border-radius:.9rem;background:#15181e;box-shadow:0 1.6rem 4rem rgba(0,0,0,.48)}
.recordings2-hero-page .recordings2-detail-poster img{display:block;width:100%;aspect-ratio:2/3;object-fit:cover}
.recordings2-hero-page .recordings2-detail-copy{display:grid;gap:1rem;max-width:62rem;padding-bottom:.25rem;text-shadow:0 .14rem .9rem rgba(0,0,0,.54)}
.recordings2-hero-page .recordings2-detail-copy h3{margin:0;font-size:clamp(2.7rem,5.6vw,5.8rem);line-height:.94;letter-spacing:-.045em;color:#fff}
.recordings2-hero-page .recordings2-subtitle{margin:0;color:#d1d5db;font-size:1rem}
.recordings2-hero-page .recordings2-detail-description{max-width:54rem;margin:0;color:#e5e7eb;font-size:clamp(1rem,1.35vw,1.18rem);line-height:1.58}
.recordings2-hero-eyebrow{color:#c7ddff;font-size:.76rem;font-weight:900;letter-spacing:.14em;text-transform:uppercase}
.recordings2-hero-badges{display:flex;flex-wrap:wrap;gap:.48rem}.recordings2-hero-badge{display:inline-flex;align-items:center;min-height:1.95rem;padding:.24rem .66rem;border:1px solid rgba(255,255,255,.24);border-radius:.4rem;background:rgba(18,20,25,.58);color:#f8fafc;font-size:.76rem;font-weight:850;backdrop-filter:blur(12px)}
.recordings2-hero-actions{display:flex;flex-wrap:wrap;gap:.62rem;padding-top:.3rem}.recordings2-hero-actions button{min-height:3rem;padding:.66rem 1.15rem;border:1px solid rgba(255,255,255,.25);border-radius:999px;background:rgba(15,18,23,.64);color:#f8fafc;font-weight:850;box-shadow:0 .35rem 1.25rem rgba(0,0,0,.14);backdrop-filter:blur(14px)}.recordings2-hero-actions button.primary{border-color:#b9d8ff;background:#b9d8ff;color:#10203a}.recordings2-hero-actions button:hover,.recordings2-hero-actions button:focus-visible{transform:translateY(-1px);border-color:#b9d8ff;outline:none}
.recordings2-hero-cast{display:flex;gap:1rem;overflow-x:auto;padding:.2rem 0 .4rem;scrollbar-width:thin}.recordings2-hero-person{display:grid;grid-template-columns:2.8rem minmax(0,1fr);gap:.58rem;align-items:center;flex:0 0 auto;min-width:10.5rem}.recordings2-hero-person img,.recordings2-hero-person-placeholder{width:2.8rem;height:2.8rem;border-radius:50%;object-fit:cover;background:#252a33}.recordings2-hero-person-placeholder{display:grid;place-items:center;color:#6b7280;font-weight:900}.recordings2-hero-person-copy{display:grid;gap:.08rem}.recordings2-hero-person-copy strong{font-size:.8rem}.recordings2-hero-person-copy span{color:#d1d5db;font-size:.68rem}
.recordings2-hero-page>.recordings2-detail-grid,.recordings2-hero-page>.recordings2-playback,.recordings2-hero-page>.recordings2-actions,.recordings2-hero-page>.recordings2-metadata-tabs,.recordings2-hero-page>.recordings2-metadata-panel,.recordings2-hero-related{position:relative;z-index:2;margin-left:clamp(1.25rem,5vw,5rem);margin-right:clamp(1.25rem,5vw,5rem)}
.recordings2-hero-page>.recordings2-detail-grid{grid-template-columns:repeat(auto-fit,minmax(11rem,1fr));gap:.65rem;margin-top:1rem}.recordings2-hero-page>.recordings2-detail-grid .recordings2-detail-field{border-color:rgba(255,255,255,.1);background:rgba(18,21,27,.68)}
.recordings2-hero-related{display:grid;gap:.85rem;margin-top:1.9rem}.recordings2-hero-related-head{display:flex;align-items:end;justify-content:space-between;gap:1rem}.recordings2-hero-related-head h4{margin:0;color:#fff;font-size:1.35rem}.recordings2-hero-related-head span{color:#9ca3af;font-size:.78rem}.recordings2-hero-related-rail{display:flex;gap:.85rem;overflow-x:auto;padding:.15rem 0 .7rem;scroll-snap-type:x proximity;scrollbar-width:thin}.recordings2-hero-related-card{display:grid;grid-template-rows:auto auto;gap:.48rem;flex:0 0 clamp(8.5rem,13vw,11.5rem);padding:0;border:0;background:transparent;color:inherit;text-align:left;scroll-snap-align:start}.recordings2-hero-related-poster{display:grid;place-items:center;aspect-ratio:2/3;overflow:hidden;border:1px solid rgba(255,255,255,.14);border-radius:.7rem;background:#171a20;color:#d1d5db;font-size:1.3rem;box-shadow:0 .7rem 1.8rem rgba(0,0,0,.2)}.recordings2-hero-related-poster img{display:block;width:100%;height:100%;object-fit:cover}.recordings2-hero-related-card:hover .recordings2-hero-related-poster,.recordings2-hero-related-card:focus-visible .recordings2-hero-related-poster{border-color:#b9d8ff;box-shadow:0 0 0 2px rgba(185,216,255,.2)}.recordings2-hero-related-card:focus-visible{outline:none}.recordings2-hero-related-title{color:#f8fafc;font-size:.8rem;font-weight:850;line-height:1.25}
.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-playback,.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-actions,.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-metadata-tabs,.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-metadata-panel{display:none!important}
.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-detail-hero,.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-detail-grid,.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-actions,.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-metadata-tabs,.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-metadata-panel,.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-hero-related{display:none!important}
.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-detail-hero,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-detail-grid,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-actions,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-metadata-tabs,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-metadata-panel,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-hero-related{display:none!important}
.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-detail-hero,.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-detail-grid,.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-playback,.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-actions,.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-hero-related{display:none!important}
.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-playback,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-playback{margin-top:5.5rem;margin-bottom:2rem;padding:clamp(1rem,3vw,2rem);border-radius:1rem;background:#05070b;box-shadow:0 0 0 1px rgba(255,255,255,.08)}
.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-metadata-tabs{margin-top:5.5rem}.recordings2-hero-page[data-recordings2-hero-mode="metadata"]>.recordings2-metadata-panel{margin-top:1rem}
@media(max-width:820px){.recordings2-hero-page>.recordings2-detail-hero{grid-template-columns:9rem minmax(0,1fr);gap:1.2rem;min-height:auto;padding-top:6rem}.recordings2-hero-page .recordings2-detail-copy h3{font-size:clamp(2.2rem,8vw,3.8rem)}}
@media(max-width:620px){.recordings2-hero-page>.recordings2-detail-hero{grid-template-columns:1fr;padding-top:5.5rem}.recordings2-hero-page .recordings2-detail-poster{width:min(44vw,10rem)}.recordings2-hero-actions{display:grid;grid-template-columns:repeat(2,minmax(0,1fr))}.recordings2-hero-actions button{width:100%}.recordings2-hero-page>.recordings2-detail-grid{grid-template-columns:1fr 1fr}}
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

  function releaseYear(metadata) {
    const value = text(metadata && (metadata.releaseDate || metadata.firstAired));
    const match = /^(\d{4})/.exec(value);
    return match ? match[1] : '';
  }

  function actorList(metadata) {
    return (Array.isArray(metadata && metadata.people) ? metadata.people : [])
      .filter(function (person) {
        return person && text(person.role).toLowerCase() === 'actor' && text(person.name);
      });
  }

  function appendBadge(container, value) {
    const normalized = text(value);
    if (normalized) container.appendChild(shared.node('span', 'recordings2-hero-badge', normalized));
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
    const normalized = ['detail', 'playback', 'marks', 'metadata'].includes(mode) ? mode : 'detail';
    root.dataset.recordings2HeroMode = normalized;
    const back = ensureModeBack(root);
    back.hidden = normalized === 'detail';
    if (normalized === 'detail') {
      if (typeof root.scrollTo === 'function') root.scrollTo({top: 0, behavior: 'smooth'});
      return;
    }
    global.setTimeout(function () { focusSection(root, selector, buttonText); }, 0);
  }

  function renderHeroActions(root, copy) {
    if (!copy || copy.querySelector('.recordings2-hero-actions')) return;
    const actions = shared.node('div', 'recordings2-hero-actions');
    actions.appendChild(makeButton('▶ Abspielen', function () {
      showMode(root, 'playback', '.recordings2-playback');
    }, true));
    actions.appendChild(makeButton('Schnittmarken', function () {
      showMode(root, 'marks', '.recordings2-marks-detail', 'Marke setzen');
    }));
    actions.appendChild(makeButton('Schneiden', function () {
      showMode(root, 'marks', '.recordings2-marks-detail', 'Schneiden');
    }));
    actions.appendChild(makeButton('Metadaten', function () {
      showMode(root, 'metadata', '.recordings2-metadata-tabs');
    }));
    copy.appendChild(actions);
  }

  function renderHeroMetadata(copy, metadata) {
    if (!copy || copy.querySelector('.recordings2-hero-badges')) return;
    const eyebrow = shared.node('div', 'recordings2-hero-eyebrow', 'VDR-Suite · Aufnahme');
    copy.insertBefore(eyebrow, copy.firstChild);
    const badges = shared.node('div', 'recordings2-hero-badges');
    appendBadge(badges, releaseYear(metadata));
    (Array.isArray(metadata && metadata.genres) ? metadata.genres : []).slice(0, 3)
      .forEach(function (genre) { appendBadge(badges, genre); });
    if (Number(metadata && metadata.voteAverage) > 0) {
      appendBadge(badges, '★ ' + Number(metadata.voteAverage).toFixed(1) + ' / 10');
    }
    appendBadge(badges, 'Aufnahme');
    const description = copy.querySelector('.recordings2-detail-description');
    copy.insertBefore(badges, description || null);
  }

  function renderHeroCast(copy, metadata) {
    if (!copy || copy.querySelector('.recordings2-hero-cast')) return;
    const actors = actorList(metadata).slice(0, 7);
    if (!actors.length) return;
    const cast = shared.node('div', 'recordings2-hero-cast');
    actors.forEach(function (person) {
      const item = shared.node('div', 'recordings2-hero-person');
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
      if (text(person.characterName)) {
        personCopy.appendChild(shared.node('span', '', person.characterName));
      }
      item.appendChild(personCopy);
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
    button.appendChild(shared.node(
      'span', 'recordings2-hero-related-title', shared.recordingTitle(recording)
    ));
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

  function renderRelated(root, recording, backendId, metadata) {
    if (!root || root.querySelector('.recordings2-hero-related')) return;
    const actor = actorList(metadata)[0];
    const api = shared.clientApi();
    if (!actor || !api || typeof api.fetchClientRecordingPersons !== 'function') return;
    api.fetchClientRecordingPersons({
      backendId: backendId,
      query: {name: actor.name, limit: 20},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function (result) {
      if (!root.isConnected && typeof root.isConnected === 'boolean') return;
      const matches = (result && Array.isArray(result.matches) ? result.matches : [])
        .map(function (match) { return match && match.recording ? match.recording : null; })
        .filter(Boolean)
        .filter(function (candidate) { return !sameRecording(candidate, recording); });
      if (!matches.length) return;
      const section = shared.node('section', 'recordings2-hero-related');
      const head = shared.node('div', 'recordings2-hero-related-head');
      head.appendChild(shared.node('h4', '', 'Weitere Filme mit ' + actor.name));
      head.appendChild(shared.node('span', '', String(matches.length) + ' lokale Aufnahme(n)'));
      section.appendChild(head);
      const rail = shared.node('div', 'recordings2-hero-related-rail');
      matches.forEach(function (candidate) {
        rail.appendChild(createRelatedCard(candidate, recording, backendId));
      });
      section.appendChild(rail);
      root.appendChild(section);
    }).catch(function () {
      // Related recordings are optional; the primary detail page remains fully usable.
    });
  }

  function enhance(root, recording, backendId, metadata) {
    if (!root || !recording || !metadata || metadata.available !== true) return root;
    installStyles();
    root.classList.add('recordings2-hero-page');
    root.dataset.recordings2HeroMode = 'detail';
    ensureModeBack(root).hidden = true;
    const backdrop = backdropUrl(metadata);
    if (backdrop && root.style && typeof root.style.setProperty === 'function') {
      root.style.setProperty(
        '--recordings2-hero-backdrop',
        'url("' + backdrop.replace(/"/g, '%22') + '")'
      );
    }
    const copy = root.querySelector('.recordings2-detail-copy');
    renderHeroMetadata(copy, metadata);
    renderHeroCast(copy, metadata);
    renderHeroActions(root, copy);
    renderRelated(root, recording, backendId, metadata);
    return root;
  }

  global.VdrSuiteRecordings2HeroDetail = Object.freeze({
    enhance,
    __test: Object.freeze({backdropUrl, releaseYear, actorList, sameRecording, showMode})
  });
}(window));
