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
.recordings2-detail.recordings2-hero-page{position:relative;overflow:hidden;padding-bottom:2rem;background:#020617;color:#f8fafc}
.recordings2-hero-page>.recordings2-header{position:relative;z-index:5;margin-bottom:0;padding:1rem 1.1rem;background:linear-gradient(180deg,rgba(2,6,23,.92),rgba(2,6,23,.28),transparent);border:0}
.recordings2-hero-page>.recordings2-detail-hero{position:relative;z-index:2;display:grid;grid-template-columns:minmax(11rem,17rem) minmax(0,1fr);gap:2rem;align-items:end;min-height:min(66vh,46rem);margin:0;padding:clamp(4rem,10vw,8rem) clamp(1rem,4vw,4rem) clamp(2rem,5vw,4rem);border:0;border-radius:0;background:linear-gradient(90deg,rgba(2,6,23,.98) 0%,rgba(2,6,23,.86) 34%,rgba(2,6,23,.36) 68%,rgba(2,6,23,.7) 100%)}
.recordings2-hero-page>.recordings2-detail-hero::before{content:'';position:absolute;inset:0;z-index:-2;background-image:var(--recordings2-hero-backdrop);background-size:cover;background-position:center 22%;opacity:.78;transform:scale(1.015)}
.recordings2-hero-page>.recordings2-detail-hero::after{content:'';position:absolute;inset:0;z-index:-1;background:linear-gradient(180deg,rgba(2,6,23,.16),rgba(2,6,23,.22) 55%,#020617 100%)}
.recordings2-hero-page .recordings2-detail-poster{align-self:end;box-shadow:0 1.4rem 3.2rem rgba(0,0,0,.48);border-radius:1rem;overflow:hidden;background:#111827}
.recordings2-hero-page .recordings2-detail-poster img{display:block;width:100%;aspect-ratio:2/3;object-fit:cover}
.recordings2-hero-page .recordings2-detail-copy{display:grid;gap:1rem;max-width:58rem;padding-bottom:.25rem;text-shadow:0 .14rem .8rem rgba(0,0,0,.45)}
.recordings2-hero-page .recordings2-detail-copy h3{margin:0;font-size:clamp(2.25rem,5vw,5rem);line-height:.98;letter-spacing:-.04em;color:#fff}
.recordings2-hero-page .recordings2-detail-description{max-width:52rem;margin:0;color:#e2e8f0;font-size:clamp(.98rem,1.45vw,1.15rem);line-height:1.55}
.recordings2-hero-eyebrow{color:#bae6fd;font-size:.76rem;font-weight:900;letter-spacing:.12em;text-transform:uppercase}
.recordings2-hero-badges{display:flex;flex-wrap:wrap;gap:.45rem}.recordings2-hero-badge{display:inline-flex;align-items:center;min-height:1.9rem;padding:.24rem .62rem;border:1px solid rgba(226,232,240,.34);border-radius:999px;background:rgba(15,23,42,.62);color:#f8fafc;font-size:.76rem;font-weight:850;backdrop-filter:blur(10px)}
.recordings2-hero-actions{display:flex;flex-wrap:wrap;gap:.55rem;padding-top:.15rem}.recordings2-hero-actions button{min-height:2.75rem;padding:.6rem 1rem;border-radius:.78rem;border:1px solid rgba(148,163,184,.38);background:rgba(15,23,42,.78);color:#f8fafc;font-weight:850}.recordings2-hero-actions button.primary{border-color:#7dd3fc;background:#e0f2fe;color:#0f172a}.recordings2-hero-actions button:hover,.recordings2-hero-actions button:focus-visible{transform:translateY(-1px);border-color:#7dd3fc;outline:none}
.recordings2-hero-cast{display:flex;gap:.8rem;overflow-x:auto;padding:.2rem 0 .35rem;scrollbar-width:thin}.recordings2-hero-person{display:grid;grid-template-columns:2.7rem minmax(0,1fr);gap:.55rem;align-items:center;flex:0 0 auto;min-width:10rem}.recordings2-hero-person img,.recordings2-hero-person-placeholder{width:2.7rem;height:2.7rem;border-radius:50%;object-fit:cover;background:#1e293b}.recordings2-hero-person-placeholder{display:grid;place-items:center;color:#64748b;font-weight:900}.recordings2-hero-person-copy{display:grid;gap:.08rem}.recordings2-hero-person-copy strong{font-size:.78rem}.recordings2-hero-person-copy span{color:#cbd5e1;font-size:.68rem}
.recordings2-hero-page>.recordings2-detail-grid,.recordings2-hero-page>.recordings2-playback,.recordings2-hero-page>.recordings2-actions,.recordings2-hero-page>.recordings2-metadata-tabs,.recordings2-hero-page>.recordings2-metadata-panel,.recordings2-hero-related{position:relative;z-index:2;margin-left:clamp(1rem,4vw,4rem);margin-right:clamp(1rem,4vw,4rem)}
.recordings2-hero-page>.recordings2-detail-grid{grid-template-columns:repeat(auto-fit,minmax(10rem,1fr));margin-top:1.2rem}
.recordings2-hero-related{display:grid;gap:.8rem;margin-top:1.6rem}.recordings2-hero-related-head{display:flex;align-items:end;justify-content:space-between;gap:1rem}.recordings2-hero-related-head h4{margin:0;color:#fff;font-size:1.25rem}.recordings2-hero-related-head span{color:#94a3b8;font-size:.78rem}.recordings2-hero-related-rail{display:flex;gap:.8rem;overflow-x:auto;padding:.15rem 0 .6rem;scroll-snap-type:x proximity;scrollbar-width:thin}.recordings2-hero-related-card{display:grid;grid-template-rows:auto auto;gap:.45rem;flex:0 0 clamp(8.2rem,13vw,11rem);padding:0;border:0;background:transparent;color:inherit;text-align:left;scroll-snap-align:start}.recordings2-hero-related-poster{display:grid;place-items:center;aspect-ratio:2/3;overflow:hidden;border:1px solid rgba(148,163,184,.28);border-radius:.75rem;background:#111827;color:#cbd5e1;font-size:1.3rem}.recordings2-hero-related-poster img{display:block;width:100%;height:100%;object-fit:cover}.recordings2-hero-related-card:hover .recordings2-hero-related-poster,.recordings2-hero-related-card:focus-visible .recordings2-hero-related-poster{border-color:#7dd3fc;box-shadow:0 0 0 2px rgba(125,211,252,.22)}.recordings2-hero-related-card:focus-visible{outline:none}.recordings2-hero-related-title{color:#f8fafc;font-size:.78rem;font-weight:850;line-height:1.25}
@media(max-width:820px){.recordings2-hero-page>.recordings2-detail-hero{grid-template-columns:8.5rem minmax(0,1fr);gap:1rem;min-height:auto;padding-top:4rem}.recordings2-hero-page .recordings2-detail-copy h3{font-size:clamp(2rem,8vw,3.5rem)}}
@media(max-width:620px){.recordings2-hero-page>.recordings2-detail-hero{grid-template-columns:1fr;padding-top:3rem}.recordings2-hero-page .recordings2-detail-poster{width:min(44vw,10rem)}.recordings2-hero-actions{display:grid;grid-template-columns:repeat(2,minmax(0,1fr))}.recordings2-hero-actions button{width:100%}}
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
    return preferred ? publicImageUrl(preferred.image.url) : '';
  }

  function releaseYear(metadata) {
    const value = text(metadata && (metadata.releaseDate || metadata.firstAired));
    const match = /^(\d{4})/.exec(value);
    return match ? match[1] : '';
  }

  function actorList(metadata) {
    return (Array.isArray(metadata && metadata.people) ? metadata.people : [])
      .filter(function (person) { return person && text(person.role).toLowerCase() === 'actor' && text(person.name); });
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
    if (typeof section.scrollIntoView === 'function') section.scrollIntoView({behavior: 'smooth', block: 'start'});
    if (!buttonText || typeof section.querySelectorAll !== 'function') return;
    const button = Array.from(section.querySelectorAll('button')).find(function (candidate) {
      return text(candidate && candidate.textContent).indexOf(buttonText) === 0;
    });
    if (button && typeof button.focus === 'function') button.focus();
  }

  function renderHeroActions(root, copy) {
    if (!copy || copy.querySelector('.recordings2-hero-actions')) return;
    const actions = shared.node('div', 'recordings2-hero-actions');
    actions.appendChild(makeButton('▶ Abspielen', function () {
      const owner = root.__vdrSuiteRecordingPlaybackOwner;
      if (owner && typeof owner.start === 'function') {
        Promise.resolve(owner.start()).catch(function () { focusSection(root, '.recordings2-playback'); });
        return;
      }
      focusSection(root, '.recordings2-playback');
    }, true));
    actions.appendChild(makeButton('Schnittmarken', function () {
      focusSection(root, '.recordings2-marks-detail', 'Marke setzen');
    }));
    actions.appendChild(makeButton('Schneiden', function () {
      focusSection(root, '.recordings2-marks-detail', 'Schneiden');
    }));
    actions.appendChild(makeButton('Metadaten', function () {
      focusSection(root, '.recordings2-metadata-tabs');
    }));
    copy.appendChild(actions);
  }

  function renderHeroMetadata(copy, metadata) {
    if (!copy || copy.querySelector('.recordings2-hero-badges')) return;
    const eyebrow = shared.node('div', 'recordings2-hero-eyebrow', 'VDR-Suite · Aufnahme');
    copy.insertBefore(eyebrow, copy.firstChild);
    const badges = shared.node('div', 'recordings2-hero-badges');
    appendBadge(badges, releaseYear(metadata));
    (Array.isArray(metadata && metadata.genres) ? metadata.genres : []).slice(0, 3).forEach(function (genre) {
      appendBadge(badges, genre);
    });
    if (Number(metadata && metadata.voteAverage) > 0) appendBadge(badges, '★ ' + Number(metadata.voteAverage).toFixed(1) + ' / 10');
    appendBadge(badges, 'Aufnahme');
    const description = copy.querySelector('.recordings2-detail-description');
    copy.insertBefore(badges, description || null);
  }

  function renderHeroCast(copy, metadata) {
    if (!copy || copy.querySelector('.recordings2-hero-cast')) return;
    const actors = actorList(metadata).slice(0, 5);
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
      if (text(person.characterName)) personCopy.appendChild(shared.node('span', '', person.characterName));
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
      image.addEventListener('error', function () { image.remove(); poster.textContent = '▶'; }, {once: true});
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
          runtime.openRecording(currentRecording, {backendId: backendId, backLabel: '← Zurück zu den Aufnahmen'});
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
      matches.forEach(function (candidate) { rail.appendChild(createRelatedCard(candidate, recording, backendId)); });
      section.appendChild(rail);
      root.appendChild(section);
    }).catch(function () {
      // Related recordings are an optional enhancement; keep the detail page usable on lookup failure.
    });
  }

  function enhance(root, recording, backendId, metadata) {
    if (!root || !recording || !metadata || metadata.available !== true) return root;
    installStyles();
    root.classList.add('recordings2-hero-page');
    const backdrop = backdropUrl(metadata);
    if (backdrop && root.style && typeof root.style.setProperty === 'function') {
      root.style.setProperty('--recordings2-hero-backdrop', 'url("' + backdrop.replace(/"/g, '%22') + '")');
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
    __test: Object.freeze({backdropUrl, releaseYear, actorList, sameRecording})
  });
}(window));
