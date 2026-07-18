(function () {
  'use strict';

  const URL_PATTERN = /^\/recording-artwork\/(?:[A-Za-z0-9._~-]|%[0-9A-Fa-f]{2})+\/[0-9A-Fa-f]{32}$/;
  const SPRITE = '/channel-logos/vdr-suite-brand/recording-genre-sprite.svg';
  const GENRES = Object.freeze({
    horror:'horror',grusel:'horror',katastrophe:'katastrophenfilm',
    katastrophenfilm:'katastrophenfilm',fantasy:'fantasy',
    historie:'historienfilm',historienfilm:'historienfilm',
    comedy:'komoedie',komodie:'komoedie',krieg:'krieg',
    kriegsfilm:'krieg',thriller:'thriller',musik:'musik',music:'musik',
    drama:'drama',mystery:'mystery',mysterium:'mystery',
    scifi:'scifi',sciencefiction:'scifi',serie:'serien',serien:'serien',
    western:'western',doku:'doku',dokumentation:'doku',
    documentary:'doku',action:'action',musical:'musical'
  });

  let folderLoader = null;
  let folderRequest = null;
  let folderObserver = null;
  let folderEpoch = 0;

  function installStyles() {
    if (document.getElementById('vdr-suite-recording-artwork-styles')) return;
    const style = document.createElement('style');
    style.id = 'vdr-suite-recording-artwork-styles';
    style.textContent = `
.recording-artwork-image,.recording-genre-artwork-image{position:absolute;z-index:1;display:block;pointer-events:none;border:1px solid rgba(125,211,252,.38);border-radius:.82rem;background:#020617;box-shadow:0 .65rem 1.4rem rgba(2,6,23,.35)}
.recording-artwork-image{object-fit:cover;object-position:center}
.recording-list-item .recording-artwork-image,.recording-folder-item .recording-artwork-image,.recording-folder-item .recording-genre-artwork-image{left:.8rem;top:.72rem;width:4.45rem;height:6.35rem}
.recording-detail .recording-artwork-image{left:1rem;top:1rem;width:8.6rem;height:12rem}
.recording-folder-item.has-recording-artwork,.recording-folder-item.has-recording-genre-artwork{position:relative;isolation:isolate;min-height:7.5rem;padding-left:6.25rem!important}
.recording-genre-artwork-image{background-image:url("${SPRITE}");background-repeat:no-repeat;background-size: 300% 200%;}
.recording-genre-artwork-horror{background-position:0 0}.recording-genre-artwork-katastrophenfilm{background-position:50% 0}.recording-genre-artwork-fantasy{background-position:100% 0}.recording-genre-artwork-historienfilm{background-position:0 100%}.recording-genre-artwork-komoedie{background-position:50% 100%}.recording-genre-artwork-krieg{background-position:100% 100%}
.recording-genre-artwork-thriller{background-image:url("/channel-logos/vdr-suite-brand/recording-genre-thriller.svg");background-size:cover;background-position:center}
.recording-genre-artwork-musik{background-image:url("/channel-logos/vdr-suite-brand/recording-genre-musik.svg");background-size:cover;background-position:center}
.recording-genre-artwork-drama{background-image:url("/channel-logos/vdr-suite-brand/recording-genre-drama.svg");background-size:cover;background-position:center}
.recording-genre-artwork-mystery{background-image:url("/channel-logos/vdr-suite-brand/recording-genre-mystery.svg");background-size:cover;background-position:center}
.recording-genre-artwork-scifi{background-image:url("/channel-logos/vdr-suite-brand/recording-genre-scifi.svg");background-size:cover;background-position:center}
.recording-genre-artwork-serien{background-image:url("/channel-logos/vdr-suite-brand/recording-genre-serien.svg");background-size:cover;background-position:center}
.recording-genre-artwork-western{background-image:url("/channel-logos/vdr-suite-brand/recording-genre-western.svg");background-size:cover;background-position:center}
.recording-genre-artwork-doku{background-image:url("/channel-logos/vdr-suite-brand/recording-genre-doku.svg");background-size:cover;background-position:center}
.recording-genre-artwork-action{background-image:url("/channel-logos/vdr-suite-brand/recording-genre-action.svg");background-size:cover;background-position:center}
.recording-genre-artwork-musical{background-image:url("/channel-logos/vdr-suite-brand/recording-genre-musical.svg");background-size:cover;background-position:center}
.recording-list-item.has-recording-artwork::before,.recording-folder-item.has-recording-artwork::before,.recording-detail.has-recording-artwork::before{opacity:0}
@keyframes vdr-suite-recording-card-reveal{from{opacity:0;transform:translateY(.45rem) scale(.992)}to{opacity:1;transform:none}}
@keyframes vdr-suite-recording-tap-confirm{from{opacity:.95}to{opacity:0}}
.recording-folder-list>.recording-folder-item,.recording-folder-list>article.recording-list-item{position:relative;isolation:isolate;box-sizing:border-box;min-width:0;animation:vdr-suite-recording-card-reveal 180ms ease-out both;transition:transform 160ms ease,border-color 160ms ease,background-color 160ms ease,box-shadow 160ms ease;-webkit-tap-highlight-color:transparent}
.recording-folder-list>.recording-folder-item:focus-visible,.recording-folder-list>article.recording-list-item:focus-visible{border-color:rgba(125,211,252,.78);background:#0f172a;box-shadow:0 0 0 3px rgba(56,189,248,.2);outline:0}
.recording-folder-list>.recording-card-pressed,.recording-folder-list>.recording-card-activated{z-index:5;transform:scale(.972)!important;border-color:#38bdf8!important;background:rgba(3,105,161,.34)!important;box-shadow:0 0 0 3px rgba(56,189,248,.34),0 .75rem 1.6rem rgba(2,6,23,.5)!important}
.recording-folder-list>.recording-card-activated::after{content:"";position:absolute;z-index:8;inset:0;pointer-events:none;border-radius:inherit;background:radial-gradient(circle,rgba(125,211,252,.3),transparent 58%);animation:vdr-suite-recording-tap-confirm 180ms ease-out both}
@media (hover: none), (pointer: coarse){.recording-folder-list>.recording-folder-item:active,.recording-folder-list>article.recording-list-item:active{transform:scale(.985);border-color:rgba(125,211,252,.78);background:#0f172a;box-shadow:0 .45rem 1.2rem rgba(2,6,23,.38)}}
@media (hover: hover) and (pointer: fine){.recording-folder-list>.recording-folder-item:hover,.recording-folder-list>article.recording-list-item:hover{z-index:2;transform:translateY(-.22rem);border-color:rgba(125,211,252,.78);background:#0f172a;box-shadow:0 1rem 2.2rem rgba(2,6,23,.48),0 0 0 1px rgba(56,189,248,.18)}}
@media (min-width: 72rem){
.recording-folder-list {grid-template-columns: repeat(auto-fit, minmax(25rem, 1fr));gap:.9rem}
.recording-folder-list > .module-placeholder,.recording-folder-list > .recording-list-item:not(.recording-folder-item) {grid-column:1/-1}
.recording-folder-list>.recording-folder-item{box-sizing: border-box;padding-top: 0.55rem;padding-bottom: 0.55rem;}
.recording-folder-list > .recording-folder-item.has-recording-artwork {min-height: 13.7rem;padding-left: 10.45rem !important;}
.recording-folder-list>.recording-folder-item.has-recording-genre-artwork{min-height:13.7rem;padding-left:10.45rem!important}
.recording-folder-list>.recording-folder-item .list-title{line-height: 1.12;}
.recording-folder-list>.recording-folder-item .list-meta{margin-top: 0.12rem;line-height: 1.2;}
.recording-folder-list > .recording-folder-item .recording-artwork-image,.recording-folder-list>.recording-folder-item .recording-genre-artwork-image{left: 0.75rem;top: 0.45rem;width: 8.9rem;height: 12.7rem;}
.recording-detail{min-height:20rem;padding-left:14.5rem!important}.recording-detail::before,.recording-detail .recording-artwork-image{left:1rem;top:1rem;width: 12rem;height: 18rem;}}
@media (prefers-reduced-motion: reduce){.recording-folder-list>.recording-folder-item,.recording-folder-list>article.recording-list-item{animation:none!important;transition:none!important}}
@media (max-width: 760px){
.module-nav{flex-wrap:nowrap;overflow-x:auto;overflow-y:hidden;padding:.1rem 0 .45rem;scroll-snap-type:x proximity;scrollbar-width:thin}.module-nav .module-tab{flex:0 0 auto;scroll-snap-align:start}
.recording-list-item .recording-artwork-image,.recording-folder-item .recording-artwork-image,.recording-folder-item .recording-genre-artwork-image{left:.65rem;top:.62rem;width:3.85rem;height:5.5rem;border-radius:.7rem}
.recording-folder-item.has-recording-artwork,.recording-folder-item.has-recording-genre-artwork{min-height:6.7rem;padding-left:5.25rem!important}
.recording-detail{box-sizing:border-box;min-height:0;padding: 13rem 0.85rem 0.9rem !important;}
.recording-detail::before,.recording-detail .recording-artwork-image{left:50%;top:.82rem;width: 7.4rem;height: 10.65rem;border-radius:.78rem;transform:translateX(-50%)}
.recording-detail>h3{margin:0 0 .45rem;text-align:center;font-size:1.28rem;line-height:1.18}.recording-detail>p{line-height:1.34}
.recording-technical-details,.recording-action-panel,.recording-action-buttons,.recording-action-result{box-sizing:border-box;width:100%;min-width:0}
.recording-technical-details{margin-top:.65rem;padding:.62rem .72rem;border:1px solid rgba(148,163,184,.22);border-radius:.72rem;background:rgba(15,23,42,.62)}
.recording-action-panel{margin-top:.8rem;padding:.2rem .7rem .75rem;overflow:hidden;border:1px solid rgba(96,165,250,.34);border-radius:.9rem;background:rgba(15,23,42,.72)}
.recording-action-panel>summary{cursor:pointer;padding:.72rem .12rem;color:#f8fafc;font-weight:850;line-height:1.2}.recording-action-panel[open]>summary{margin-bottom:.6rem;border-bottom:1px solid rgba(148,163,184,.18)}
.recording-action-panel > p {display:none}
.recording-action-buttons{display:grid!important;grid-template-columns:minmax(0,1fr)!important;align-items:stretch;gap:.58rem!important}.recording-action-buttons>*{box-sizing:border-box;width:100%;min-width:0;margin:0}
.recording-action-buttons button,.recording-move-folder-browser button,.recording-detail>button:last-child{box-sizing:border-box;width:100%;min-height:2.75rem;border-radius:.72rem;padding:.65rem .8rem;white-space:normal;line-height:1.2}
.recording-action-buttons details{overflow:hidden;border:1px solid rgba(148,163,184,.22);border-radius:.78rem;background:rgba(2,6,23,.7)}.recording-action-buttons details>summary{cursor:pointer;padding:.72rem .78rem;color:#e2e8f0;font-weight:800;line-height:1.2}
.recording-move-editor-body,.recording-trash-editor-body{box-sizing:border-box;width:100%;margin:0;padding:.78rem!important;border:0;border-radius:0}
.recording-move-editor-body label,.recording-trash-editor-body label{display:grid;gap:.3rem;width:100%;min-width:0}
.recording-move-editor-body input,.recording-trash-editor-body input{box-sizing:border-box;width:100%;min-width:0;min-height:2.65rem;border:1px solid #475569;border-radius:.62rem;padding:.55rem .65rem;background:#111827;color:#f8fafc;font:inherit}
.recording-move-validation-status{padding:.62rem .68rem;border:1px solid rgba(96,165,250,.24);border-radius:.68rem;background:rgba(30,41,59,.66);line-height:1.3}
.recording-move-folder-browser{display:grid;gap:.48rem;width:100%;min-width:0;margin:.6rem 0}.recording-action-result:not(:empty){margin-top:.65rem;padding:.68rem .72rem;border:1px solid rgba(56,189,248,.22);border-radius:.72rem;background:rgba(2,6,23,.72)}
.recording-detail>button:last-child{margin-top:.78rem}}
@media (max-width: 340px){.recording-detail{padding: 10.35rem 0.85rem 0.9rem !important;}.recording-detail::before,.recording-detail .recording-artwork-image{width: 5.8rem;height: 8.35rem;}}
`;
    document.head.appendChild(style);
  }

  function presentation(recording) {
    const metadata = recording && typeof recording === 'object' ? recording.metadata : null;
    const value = metadata && typeof metadata === 'object' ? metadata.presentation : null;
    return value && typeof value === 'object' ? value : {};
  }

  function artwork(recording) {
    const metadata = recording && typeof recording === 'object' ? recording.metadata : null;
    const value = metadata && typeof metadata === 'object' ? metadata.artwork : null;
    return value && typeof value === 'object' ? value : {};
  }

  function urlForRecording(recording) {
    const candidate = String(
      presentation(recording).posterUrl || artwork(recording).preferredUrl || ''
    ).trim();
    return URL_PATTERN.test(candidate) ? candidate : '';
  }

  function titleForRecording(recording, fallback) {
    return String(
      presentation(recording).title || fallback ||
      (recording && recording.title) || 'Aufnahme'
    );
  }

  function attach(container, recording, fallback, eager) {
    if (!container || container.dataset.recordingArtworkAttached === 'true') return null;
    const url = urlForRecording(recording);
    if (!url) return null;

    container.dataset.recordingArtworkAttached = 'true';
    const image = document.createElement('img');
    image.className = 'recording-artwork-image';
    image.src = url;
    image.alt = 'Poster: ' + titleForRecording(recording, fallback);
    image.decoding = 'async';
    image.loading = eager ? 'eager' : 'lazy';
    image.referrerPolicy = 'same-origin';
    image.addEventListener('load', () => {
      container.classList.add('has-recording-artwork');
      container.dataset.recordingArtworkState = 'ready';
    });
    image.addEventListener('error', () => {
      container.classList.remove('has-recording-artwork');
      container.dataset.recordingArtworkAttached = 'false';
      container.dataset.recordingArtworkState = 'error';
      image.remove();
    });
    container.appendChild(image);
    return image;
  }

  function normalizeGenre(value) {
    return String(value || '').normalize('NFD')
      .replace(/[\u0300-\u036f]/g, '')
      .toLocaleLowerCase('de-DE').replace(/[^a-z0-9]+/g, '');
  }

  function genreForFolder(name) {
    const slug = GENRES[normalizeGenre(name)] || '';
    return slug ? Object.freeze({slug, spriteUrl:SPRITE}) : null;
  }

  function folderTitle(item) {
    const node = item && typeof item.querySelector === 'function'
      ? item.querySelector('.list-title') : null;
    return node ? String(node.textContent || '').trim() : '';
  }

  function attachGenre(item) {
    if (!item || !item.classList || !item.classList.contains('recording-folder-item')) return false;
    const genre = genreForFolder(folderTitle(item));
    if (!genre) return false;

    if (typeof item.querySelectorAll === 'function') {
      item.querySelectorAll('.recording-artwork-image').forEach(node => node.remove());
    }
    item.classList.remove('has-recording-artwork');
    item.dataset.recordingArtworkAttached = 'false';

    if (item.querySelector && item.querySelector('.recording-genre-artwork-image')) {
      item.classList.add('has-recording-genre-artwork');
      return true;
    }

    const image = document.createElement('span');
    image.className = 'recording-genre-artwork-image recording-genre-artwork-' + genre.slug;
    image.setAttribute('aria-hidden', 'true');
    item.dataset.recordingGenreArtworkAttached = 'true';
    item.classList.add('has-recording-genre-artwork');
    item.appendChild(image);
    return true;
  }

  function scanGenres(root) {
    if (!root) return;
    if (typeof root.closest === 'function') {
      const parent = root.closest('.recording-folder-item');
      if (parent) attachGenre(parent);
    }
    if (root.nodeType === 1 && root.matches && root.matches('.recording-folder-item')) attachGenre(root);
    if (root.querySelectorAll) root.querySelectorAll('.recording-folder-item').forEach(attachGenre);
  }

  function list(data, key) {
    if (Array.isArray(data)) return data;
    if (data && Array.isArray(data[key])) return data[key];
    if (data && Array.isArray(data.items)) return data.items;
    return [];
  }

  function first(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') return object[key];
    }
    return fallback;
  }

  function folderPath(folder) {
    return String(first(folder, ['path','folderPath','name','title'], '')).trim();
  }

  function folderCount(folder) {
    const value = Number(first(folder, ['recordingCount','count','total'], 0));
    return Number.isFinite(value) ? value : 0;
  }

  function connected(node) {
    return Boolean(node) && (node.isConnected === undefined || node.isConnected === true);
  }

  function currentFolderItems() {
    const holder = document.querySelector && document.querySelector('.recording-folder-list');
    return holder && holder.querySelectorAll
      ? Array.from(holder.querySelectorAll('.recording-folder-item')) : [];
  }

  function singleRecording(data) {
    const recordings = list(data, 'recordings');
    const count = Number(data && data.recordingCount) || recordings.length;
    return recordings.length === 1 && count === 1 ? recordings[0] : null;
  }

  function requestFolderPoster(item, folder) {
    const path = folderPath(folder);
    if (!folderLoader || !connected(item) || !path) return;
    item.dataset.recordingFolderArtworkState = 'loading';

    Promise.resolve(folderLoader(path, 0)).then(data => {
      if (!connected(item) || item.dataset.recordingFolderPath !== path) return;
      const recording = singleRecording(data && typeof data === 'object' ? data : {});
      if (!recording) {
        item.dataset.recordingFolderArtworkState = 'empty';
        return;
      }
      const image = attach(item, recording, folderTitle(item), false);
      item.dataset.recordingFolderArtworkState = image ? 'attached' : 'unavailable';
      if (image) {
        image.addEventListener('load', () => { item.dataset.recordingFolderArtworkState = 'ready'; });
        image.addEventListener('error', () => { item.dataset.recordingFolderArtworkState = 'error'; });
      }
    }).catch(() => {
      if (connected(item) && item.dataset.recordingFolderPath === path) {
        item.dataset.recordingFolderArtworkState = 'error';
      }
    });
  }

  function observeFolderPoster(item, folder) {
    if (typeof IntersectionObserver !== 'function') {
      requestFolderPoster(item, folder);
      return;
    }
    item.__vdrSuiteFolderArtwork = folder;
    if (!folderObserver) {
      folderObserver = new IntersectionObserver(entries => {
        entries.forEach(entry => {
          if (!entry.isIntersecting && entry.intersectionRatio <= 0) return;
          const target = entry.target;
          const requestedFolder = target && target.__vdrSuiteFolderArtwork;
          folderObserver.unobserve(target);
          if (requestedFolder) requestFolderPoster(target, requestedFolder);
        });
      }, {rootMargin:'420px 0px', threshold:.01});
    }
    folderObserver.observe(item);
  }

  function decorateFolders(data) {
    folderEpoch += 1;
    if (folderObserver && folderObserver.disconnect) {
      folderObserver.disconnect();
      folderObserver = null;
    }
    if (!folderLoader) return;

    const folders = list(data, 'folders');
    const items = currentFolderItems();
    folders.forEach((folder, index) => {
      const item = items[index];
      if (!item || folderCount(folder) !== 1 || genreForFolder(folderTitle(item))) return;
      const path = folderPath(folder);
      if (!path) return;
      item.dataset.recordingFolderPath = path;
      if (item.dataset.recordingArtworkAttached === 'true' ||
          item.dataset.recordingFolderArtworkState === 'loading') return;
      const embedded = first(folder, ['recording','representativeRecording','previewRecording'], null);
      if (embedded && typeof embedded === 'object') {
        attach(item, embedded, folderTitle(item), false);
      } else {
        observeFolderPoster(item, folder);
      }
    });
  }

  function copyOptions(options) {
    const value = options && typeof options === 'object' ? options : {};
    const copy = Object.assign({}, value);
    copy.query = Object.assign({}, value.query || {});
    return copy;
  }

  function interceptFolderApi() {
    const api = window.VdrSuiteClientApi;
    if (!api || typeof api.fetchClientRecordingFolder !== 'function' ||
        api.__recordingArtworkFolderInterceptor === true) return;

    const original = api.fetchClientRecordingFolder.bind(api);
    const wrapped = Object.assign({}, api);
    wrapped.fetchClientRecordingFolder = function (options) {
      folderRequest = copyOptions(options);
      folderLoader = function (path, offset) {
        const request = copyOptions(folderRequest);
        request.cache = 'no-store';
        request.query.path = path;
        request.query.offset = Number(offset) || 0;
        return original(request);
      };
      return Promise.resolve(original(options)).then(data => {
        window.setTimeout(() => decorateFolders(
          data && typeof data === 'object' ? data : {}
        ), 0);
        return data;
      });
    };
    Object.defineProperty(wrapped, '__recordingArtworkFolderInterceptor', {value:true});
    window.VdrSuiteClientApi = Object.freeze(wrapped);
  }

  function eventCard(event) {
    if (!event || !event.target || !event.target.closest) return null;
    const card = event.target.closest(
      '.recording-folder-list > .recording-folder-item, ' +
      '.recording-folder-list > article.recording-list-item'
    );
    return connected(card) ? card : null;
  }

  function installTapFeedback() {
    if (!document.body || !document.body.addEventListener ||
        document.body.dataset.vdrSuiteRecordingTapFeedback === 'true') return;

    document.body.dataset.vdrSuiteRecordingTapFeedback = 'true';
    const replay = new WeakSet();

    document.body.addEventListener('pointerdown', event => {
      const card = eventCard(event);
      if (card) card.classList.add('recording-card-pressed');
    }, true);

    const release = event => {
      const card = eventCard(event);
      if (!card) return;
      window.setTimeout(() => {
        if (connected(card)) card.classList.remove('recording-card-pressed');
      }, 220);
    };
    document.body.addEventListener('pointerup', release, true);
    document.body.addEventListener('pointercancel', release, true);

    document.body.addEventListener('click', event => {
      const card = eventCard(event);
      if (!card) return;
      if (replay.has(card)) {
        replay.delete(card);
        card.classList.remove('recording-card-activated');
        return;
      }
      event.preventDefault();
      event.stopImmediatePropagation();
      card.classList.remove('recording-card-pressed');
      card.classList.add('recording-card-activated');
      window.setTimeout(() => {
        if (!connected(card)) return;
        replay.add(card);
        if (card.click) card.click();
      }, 170);
    }, true);
  }

  function observeDom() {
    if (typeof MutationObserver !== 'function' || !document.body) return null;
    const observer = new MutationObserver(mutations => {
      mutations.forEach(mutation => {
        Array.from(mutation.addedNodes || []).forEach(scanGenres);
      });
    });
    observer.observe(document.body, {childList:true, subtree:true});
    return observer;
  }

  installStyles();
  interceptFolderApi();
  installTapFeedback();

  const originalConfigureLoader = window.configureRecordingBrowserFolderLoader;
  if (typeof originalConfigureLoader === 'function') {
    window.configureRecordingBrowserFolderLoader = function (loader) {
      folderLoader = typeof loader === 'function' ? loader : null;
      return originalConfigureLoader.apply(this, arguments);
    };
  }

  const originalCreateItem = window.createServerRecordingItem;
  if (typeof originalCreateItem === 'function') {
    window.createServerRecordingItem = function (recording, folderData) {
      const item = originalCreateItem(recording, folderData);
      attach(item, recording, recording && recording.title ? recording.title : 'Aufnahme', false);
      return item;
    };
  }

  const originalRenderDetail = window.renderServerRecordingDetail;
  if (typeof originalRenderDetail === 'function') {
    window.renderServerRecordingDetail = function (recording, folderData, options) {
      const result = originalRenderDetail(recording, folderData, options);
      attach(
        document.querySelector('.recording-detail'),
        recording,
        recording && recording.title ? recording.title : 'Aufnahme',
        true
      );
      return result;
    };
  }

  const originalRenderFolder = window.renderServerRecordingFolder;
  if (typeof originalRenderFolder === 'function') {
    window.renderServerRecordingFolder = function (data) {
      const result = originalRenderFolder.apply(this, arguments);
      decorateFolders(data && typeof data === 'object' ? data : {});
      return result;
    };
  }

  scanGenres(document);
  const observer = observeDom();

  window.VdrSuiteRecordingArtwork = Object.freeze({
    urlForRecording,
    attach,
    decorateFolderList:decorateFolders,
    installTapFeedback,
    setFolderLoader(loader) { folderLoader = typeof loader === 'function' ? loader : null; },
    observer
  });

  window.VdrSuiteRecordingGenreArtwork = Object.freeze({
    normalizeName:normalizeGenre,
    forFolderName:genreForFolder,
    attach:attachGenre,
    scan:scanGenres,
    observer
  });
})();
