// Genre artwork owner for Recordings 2 folder cards.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  if (!shared) {
    console.error('VDR-Suite Recordings 2 shared runtime is unavailable');
    return;
  }

  const STYLE_ID = 'vdr-suite-recordings2-folder-artwork-styles';
  const SPRITE = '/channel-logos/vdr-suite-brand/recording-genre-sprite.svg';
  const GENRES = Object.freeze({
    horror: {slug: 'horror', sprite: '0% 0%'},
    grusel: {slug: 'horror', sprite: '0% 0%'},
    katastrophe: {slug: 'katastrophenfilm', sprite: '50% 0%'},
    katastrophenfilm: {slug: 'katastrophenfilm', sprite: '50% 0%'},
    fantasy: {slug: 'fantasy', sprite: '100% 0%'},
    historie: {slug: 'historienfilm', sprite: '0% 100%'},
    historienfilm: {slug: 'historienfilm', sprite: '0% 100%'},
    comedy: {slug: 'komoedie', sprite: '50% 100%'},
    komodie: {slug: 'komoedie', sprite: '50% 100%'},
    krieg: {slug: 'krieg', sprite: '100% 100%'},
    kriegsfilm: {slug: 'krieg', sprite: '100% 100%'},
    thriller: {slug: 'thriller'},
    musik: {slug: 'musik'},
    music: {slug: 'musik'},
    drama: {slug: 'drama'},
    mystery: {slug: 'mystery'},
    mysterium: {slug: 'mystery'},
    scifi: {slug: 'scifi'},
    sciencefiction: {slug: 'scifi'},
    serie: {slug: 'serien'},
    serien: {slug: 'serien'},
    western: {slug: 'western'},
    doku: {slug: 'doku'},
    dokumentation: {slug: 'doku'},
    documentary: {slug: 'doku'},
    action: {slug: 'action'},
    musical: {slug: 'musical'}
  });

  function normalizeName(value) {
    return String(value || '')
      .normalize('NFD')
      .replace(/[\u0300-\u036f]/g, '')
      .toLocaleLowerCase('de-DE')
      .replace(/[^a-z0-9]+/g, '');
  }

  function forFolderName(value) {
    const genre = GENRES[normalizeName(value)];
    return genre ? Object.freeze(Object.assign({}, genre)) : null;
  }

  function installStyles() {
    if (document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = `
.recordings2-folder.has-genre-artwork{grid-template-columns:5.2rem minmax(0,1fr) auto}
.recordings2-folder-artwork{display:block;width:5.2rem;aspect-ratio:2/3;overflow:hidden;border:1px solid rgba(125,211,252,.3);border-radius:.65rem;background-color:#020617;background-position:center;background-repeat:no-repeat;background-size:cover;box-shadow:0 .45rem 1rem rgba(2,6,23,.32)}
.recordings2-folder-artwork.is-sprite{background-size:300% 200%}
@media(max-width:720px){.recordings2-folder.has-genre-artwork{grid-template-columns:4.65rem minmax(0,1fr) auto}.recordings2-folder-artwork{width:4.65rem}}
@media(max-width:390px){.recordings2-folder.has-genre-artwork{grid-template-columns:4.1rem minmax(0,1fr) auto}.recordings2-folder-artwork{width:4.1rem}}
@media(min-width:72rem){.recordings2-folder-list{grid-template-columns:repeat(auto-fit,minmax(26rem,1fr));gap:.85rem}.recordings2-folder.has-genre-artwork{grid-template-columns:8.9rem minmax(0,1fr) auto;min-height:13.7rem}.recordings2-folder-artwork{width:8.9rem}}
`;
    document.head.appendChild(style);
  }

  function create(folder) {
    const name = shared.decodeDisplayText(shared.first(folder, ['name'], ''));
    const genre = forFolderName(name);
    if (!genre) return null;

    installStyles();
    const artwork = document.createElement('span');
    artwork.className = 'recordings2-folder-artwork';
    artwork.setAttribute('aria-hidden', 'true');
    artwork.dataset.genre = genre.slug;

    if (genre.sprite) {
      artwork.classList.add('is-sprite');
      artwork.style.backgroundImage = 'url("' + SPRITE + '")';
      artwork.style.backgroundPosition = genre.sprite;
    } else {
      artwork.style.backgroundImage =
        'url("/channel-logos/vdr-suite-brand/recording-genre-' +
        genre.slug + '.svg")';
    }

    return artwork;
  }

  global.VdrSuiteRecordings2FolderArtwork = Object.freeze({
    normalizeName,
    forFolderName,
    installStyles,
    create
  });
}(window));
