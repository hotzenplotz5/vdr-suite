// Genre artwork and single-recording leaf owner for Recordings 2 folder cards.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  if (!shared) {
    console.error('VDR-Suite Recordings 2 shared runtime is unavailable');
    return;
  }

  const STYLE_ID = 'vdr-suite-recordings2-folder-artwork-styles';
  const SPRITE = '/channel-logos/vdr-suite-brand/recording-genre-sprite.svg';
  const LEAF_CONCURRENCY = 4;
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

  function publicPath(path) {
    const resolver = global.VdrSuitePublicUrl;
    return resolver && typeof resolver.resolvePath === 'function'
      ? resolver.resolvePath(path)
      : path;
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
@media(min-width:72rem){.recordings2-folder-list{grid-template-columns:repeat(auto-fit,minmax(18rem,1fr));gap:.7rem}.recordings2-folder.has-genre-artwork{grid-template-columns:5.2rem minmax(0,1fr) auto;min-height:0}.recordings2-folder-artwork{width:5.2rem}}
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
      artwork.style.backgroundImage = 'url("' + publicPath(SPRITE) + '")';
      artwork.style.backgroundPosition = genre.sprite;
    } else {
      artwork.style.backgroundImage = 'url("' + publicPath(
        '/channel-logos/vdr-suite-brand/recording-genre-' + genre.slug + '.svg'
      ) + '")';
    }

    return artwork;
  }

  function isSingleRecordingLeaf(data) {
    const recordings = shared.recordingList(data);
    return Boolean(data && data.recordingFolder === true) &&
      shared.folderList(data).length === 0 && recordings.length === 1;
  }

  function embeddedLeafRecording(folder) {
    if (!folder || folder.singleRecordingLeaf !== true) return null;
    const recording = folder.singleRecording;
    return recording && typeof recording === 'object' ? recording : null;
  }

  function resolveLeaves(data, loader) {
    const folders = shared.folderList(data).slice();
    const resolved = new Array(folders.length);
    const candidates = [];

    folders.forEach(function (folder, folderIndex) {
      const path = shared.normalizePath(shared.first(folder, ['path'], ''));
      const embedded = embeddedLeafRecording(folder);

      if (path && embedded) {
        resolved[folderIndex] = {path: path, recording: embedded};
        return;
      }

      if (shared.number(folder.recordingCount, 0) === 1 && path) {
        candidates.push({folder: folder, folderIndex: folderIndex, path: path});
      }
    });

    if (!candidates.length || typeof loader !== 'function') {
      const matches = resolved.filter(Boolean);
      const paths = new Set(matches.map(function (entry) { return entry.path; }));
      return Promise.resolve({
        folders: folders.filter(function (folder) {
          return !paths.has(shared.normalizePath(shared.first(folder, ['path'], '')));
        }),
        recordings: matches.map(function (entry) { return entry.recording; })
      });
    }

    let cursor = 0;
    function worker() {
      const candidateIndex = cursor++;
      if (candidateIndex >= candidates.length) return Promise.resolve();
      const candidate = candidates[candidateIndex];
      return Promise.resolve(loader(candidate.path, 0)).then(function (page) {
        if (isSingleRecordingLeaf(page)) {
          resolved[candidate.folderIndex] = {
            path: candidate.path,
            recording: shared.recordingList(page)[0]
          };
        }
      }).catch(function (error) {
        console.warn('Recordings 2 leaf could not be resolved:', candidate.path, error);
      }).then(worker);
    }

    const workers = Array.from({length: Math.min(LEAF_CONCURRENCY, candidates.length)}, worker);
    return Promise.all(workers).then(function () {
      const matches = resolved.filter(Boolean);
      const paths = new Set(matches.map(function (entry) { return entry.path; }));
      return {
        folders: folders.filter(function (folder) {
          return !paths.has(shared.normalizePath(shared.first(folder, ['path'], '')));
        }),
        recordings: matches.map(function (entry) { return entry.recording; })
      };
    });
  }

  global.VdrSuiteRecordings2FolderArtwork = Object.freeze({
    normalizeName,
    forFolderName,
    installStyles,
    create,
    isSingleRecordingLeaf,
    embeddedLeafRecording,
    resolveLeaves
  });
}(window));
