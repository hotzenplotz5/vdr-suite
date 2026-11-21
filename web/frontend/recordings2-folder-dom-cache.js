// Bounded Recordings 2 folder poster DOM retention; no API or data cache.
(function (global) {
  'use strict';
  const shared = global.VdrSuiteRecordings2Shared;
  if (!shared) return;

  function create(getState) {
    const MAX_CACHED_FOLDERS = 3;
    const MAX_CACHED_RECORDINGS = 200;
    const folderViews = new Map();
    let mounted = null;
    let backend = null;

    function folderKey(value) {
      return String(value.backendId || 'default') + '\u0000' + shared.normalizePath(value.path);
    }

    function recordingIdentity(recording) {
      const owner = String(recording.backendId || getState().backendId || 'default');
      const native = String(shared.first(recording, ['backendNativeId', 'nativePath', 'path'], ''));
      if (native) return owner + '\u0000native:' + native;
      const id = String(shared.first(recording, ['recordingId', 'id', 'nativeId'], ''));
      return id ? owner + '\u0000id:' + id : '';
    }

    function clear() {
      folderViews.clear();
      mounted = null;
      backend = null;
    }

    function remember(target) {
      const old = mounted;
      mounted = null;
      if (!old || old.recordingCount > MAX_CACHED_RECORDINGS ||
          (typeof target.contains === 'function' && !target.contains(old.root))) return;
      old.scrollTop = Number(target.scrollTop) || 0;
      old.scrollY = Number(global.scrollY) || 0;
      folderViews.delete(old.key);
      folderViews.set(old.key, old);
      while (folderViews.size > MAX_CACHED_FOLDERS) {
        folderViews.delete(folderViews.keys().next().value);
      }
    }

    function prepare(target) {
      const currentBackend = String(getState().backendId || 'default');
      if (currentBackend !== backend) {
        clear();
        backend = currentBackend;
      } else {
        remember(target);
      }
    }

    function isMounted(value) {
      return Boolean(mounted && backend === String(value.backendId || 'default') &&
        mounted.key === folderKey(value));
    }

    function consume(value) {
      const key = folderKey(value);
      const previous = folderViews.get(key);
      folderViews.delete(key);
      return {
        key: key, previous: previous, posters: new Map(),
        oldPosters: previous ? new Map(previous.posters) : null
      };
    }

    function poster(snapshot, recording) {
      const id = recordingIdentity(recording);
      const url = shared.publicPath(shared.recordingPosterUrl(recording));
      const previous = id && snapshot.oldPosters && snapshot.oldPosters.get(id);
      if (id && snapshot.oldPosters) snapshot.oldPosters.delete(id);
      let node = null;
      if (previous && url && previous.url === url) {
        const img = previous.node && typeof previous.node.querySelector === 'function'
          ? previous.node.querySelector('img') : null;
        if (img) {
          img.alt = 'Poster zu ' + shared.recordingTitle(recording);
          previous.node.dataset.variant = String(shared.recordingPlaceholderVariant(recording));
          node = previous.node;
        }
      }
      if (!node) node = shared.createPoster(recording);
      if (id) snapshot.posters.set(id, {node: node, url: url});
      return node;
    }

    function attach(target, root, snapshot, recordingCount) {
      mounted = {
        key: snapshot.key, root: root, posters: snapshot.posters,
        recordingCount: recordingCount, scrollTop: 0, scrollY: 0
      };
      const previous = snapshot.previous;
      if (!previous) return;
      if (typeof target.scrollTop === 'number') target.scrollTop = previous.scrollTop || 0;
      if (previous.scrollY > 0 && typeof global.scrollTo === 'function') {
        global.scrollTo({
          top: previous.scrollY, left: Number(global.scrollX) || 0, behavior: 'auto'
        });
      }
    }

    const session = global.VdrSuiteBrowserSession;
    if (session && typeof session.subscribe === 'function') {
      session.subscribe(function (snapshot) {
        if (!snapshot || snapshot.authenticated !== true) clear();
      });
    }

    return Object.freeze({
      clear: clear, prepare: prepare, isMounted: isMounted,
      consume: consume, poster: poster, attach: attach
    });
  }

  global.VdrSuiteRecordings2FolderDomCache = Object.freeze({create: create});
}(window));
