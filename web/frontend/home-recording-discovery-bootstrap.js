// Phase 66.5 follow-up: deferred recording discovery plus shared Home browse affordances.
//
// The recording rails are rendered lazily, so presentation-only interactions are
// delegated from this eager Home bootstrap. Mouse drag moves existing Home rails
// and the Live-TV Hero delegates completed drags to its existing browse API.
// Genre/folder category taps stay on Home and expand their canonical contents
// directly below the category rail. Playback/session ownership remains unchanged.
(function (global) {
  'use strict';

  const doc = global && global.document ? global.document : null;
  const RAIL_SELECTOR = '.media-home-discovery-rail, .media-home-series-season-rail';
  const HERO_SELECTOR = '.media-home-hero.media-home-live-hero-active[data-home-zone="hero"]';
  const CATEGORY_SELECTOR = '.media-home-discovery-card.genre, .media-home-discovery-card.folder';
  const DRAG_CLASS = 'media-home-mouse-dragging';
  const INLINE_PAGE_LIMIT = 100;
  const START_THRESHOLD = 8;
  const HERO_SWITCH_THRESHOLD = 48;
  const HORIZONTAL_DOMINANCE = 1.15;
  const CLICK_SUPPRESS_MS = 420;

  let documentBound = false;
  let activeDrag = null;
  let suppressedTarget = null;
  let suppressClickUntil = 0;
  let backendReadyObserver = null;
  const inline = {
    genre: {key: '', backendId: '', request: 0, section: null, card: null},
    folder: {key: '', rootPath: '', path: '', backendId: '', request: 0, section: null, card: null}
  };

  function text(value) {
    return value === undefined || value === null ? '' : String(value).trim();
  }

  function list(value, key) {
    if (Array.isArray(value)) return value;
    if (value && Array.isArray(value[key])) return value[key];
    if (value && Array.isArray(value.items)) return value.items;
    return [];
  }

  function platform() {
    return global.VdrSuitePlatform || null;
  }

  function clientApi() {
    const owner = platform();
    if (owner && typeof owner.getClientApi === 'function') {
      const client = owner.getClientApi();
      if (client) return client;
    }
    return global.VdrSuiteClientApi || null;
  }

  function selectedBackendId() {
    const owner = platform();
    if (owner && typeof owner.getSelectedBackendId === 'function') {
      return text(owner.getSelectedBackendId()) || 'default';
    }
    return 'default';
  }

  function canonicalSelectedBackendId() {
    const selected = doc && typeof doc.querySelector === 'function'
      ? doc.querySelector('#backends .backend-card.selected, #backends [aria-selected="true"]')
      : null;
    return text(selected && selected.dataset && selected.dataset.backendId);
  }

  function discoveryReady() {
    return Boolean(
      global.VdrSuiteHomeRecordingDiscovery &&
      typeof global.VdrSuiteHomeRecordingDiscovery.install === 'function'
    );
  }

  function closestTarget(target, selector) {
    return target && typeof target.closest === 'function' ? target.closest(selector) : null;
  }

  function dragTarget(event) {
    const target = event && event.target;
    const rail = closestTarget(target, RAIL_SELECTOR);
    if (rail) return {kind: 'rail', element: rail};
    const hero = closestTarget(target, HERO_SELECTOR);
    if (hero) return {kind: 'hero', element: hero};
    return null;
  }

  function addDraggingClass(element) {
    if (element && element.classList && typeof element.classList.add === 'function') {
      element.classList.add(DRAG_CLASS);
    }
  }

  function removeDraggingClass(element) {
    if (element && element.classList && typeof element.classList.remove === 'function') {
      element.classList.remove(DRAG_CLASS);
    }
  }

  function capturePointer(element, pointerId) {
    if (!element || typeof element.setPointerCapture !== 'function') return;
    try { element.setPointerCapture(pointerId); } catch (_) {}
  }

  function releasePointer(element, pointerId) {
    if (!element || typeof element.releasePointerCapture !== 'function') return;
    try { element.releasePointerCapture(pointerId); } catch (_) {}
  }

  function resetDrag(release) {
    if (!activeDrag) return;
    const current = activeDrag;
    activeDrag = null;
    removeDraggingClass(current.element);
    if (release) releasePointer(current.element, current.pointerId);
  }

  function horizontalGesture(deltaX, deltaY, threshold) {
    return Math.abs(deltaX) >= threshold &&
      Math.abs(deltaX) > Math.abs(deltaY) * HORIZONTAL_DOMINANCE;
  }

  function suppressNextClick(element) {
    suppressedTarget = element;
    suppressClickUntil = Date.now() + CLICK_SUPPRESS_MS;
  }

  function handlePointerDown(event) {
    if (!event || event.pointerType !== 'mouse' || Number(event.button) !== 0) return;
    const target = dragTarget(event);
    if (!target) return;
    activeDrag = {
      kind: target.kind,
      element: target.element,
      pointerId: event.pointerId,
      startX: Number(event.clientX),
      startY: Number(event.clientY),
      startScrollLeft: Number(target.element.scrollLeft) || 0,
      dragging: false
    };
  }

  function handlePointerMove(event) {
    if (!activeDrag || !event || event.pointerId !== activeDrag.pointerId) return;
    const deltaX = Number(event.clientX) - activeDrag.startX;
    const deltaY = Number(event.clientY) - activeDrag.startY;

    if (!activeDrag.dragging) {
      if (Math.abs(deltaX) < START_THRESHOLD && Math.abs(deltaY) < START_THRESHOLD) return;
      if (!horizontalGesture(deltaX, deltaY, START_THRESHOLD)) {
        resetDrag(false);
        return;
      }
      activeDrag.dragging = true;
      addDraggingClass(activeDrag.element);
      capturePointer(activeDrag.element, activeDrag.pointerId);
    }

    if (activeDrag.kind === 'rail') {
      activeDrag.element.scrollLeft = activeDrag.startScrollLeft - deltaX;
    }
    if (typeof event.preventDefault === 'function') event.preventDefault();
  }

  function finishPointer(event, cancelled) {
    if (!activeDrag || !event || event.pointerId !== activeDrag.pointerId) return;
    const current = activeDrag;
    const deltaX = Number(event.clientX) - current.startX;
    const deltaY = Number(event.clientY) - current.startY;
    const dragged = current.dragging;
    resetDrag(true);

    if (!dragged || cancelled) return;
    suppressNextClick(current.element);
    if (typeof event.preventDefault === 'function') event.preventDefault();

    if (current.kind === 'hero' && horizontalGesture(deltaX, deltaY, HERO_SWITCH_THRESHOLD)) {
      const hero = global.VdrSuiteHomeLiveHero;
      if (hero && typeof hero.selectOffset === 'function') {
        hero.selectOffset(deltaX < 0 ? 1 : -1);
      }
    }
  }

  function handlePointerUp(event) { finishPointer(event, false); }
  function handlePointerCancel(event) { finishPointer(event, true); }

  function handleNativeDragStart(event) {
    if (!activeDrag || !event) return;
    const target = dragTarget(event);
    if (!target || target.element !== activeDrag.element) return;
    if (typeof event.preventDefault === 'function') event.preventDefault();
  }

  function consumeSuppressedClick(event) {
    if (!suppressedTarget || Date.now() > suppressClickUntil) {
      suppressedTarget = null;
      suppressClickUntil = 0;
      return false;
    }
    const target = event && event.target;
    const rail = closestTarget(target, RAIL_SELECTOR);
    const hero = closestTarget(target, HERO_SELECTOR);
    if (rail !== suppressedTarget && hero !== suppressedTarget) return false;
    suppressedTarget = null;
    suppressClickUntil = 0;
    if (typeof event.preventDefault === 'function') event.preventDefault();
    if (typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation();
    else if (typeof event.stopPropagation === 'function') event.stopPropagation();
    return true;
  }

  function categoryKind(card) {
    if (!card || !card.classList) return '';
    if (card.classList.contains('genre')) return 'genre';
    if (card.classList.contains('folder')) return 'folder';
    return '';
  }

  function categoryKey(card, kind) {
    if (!card || !card.dataset) return '';
    return kind === 'genre' ? text(card.dataset.genreId) : text(card.dataset.folderPath);
  }

  function categoryLabel(card, fallback) {
    const strong = card && typeof card.querySelector === 'function' ? card.querySelector('strong') : null;
    return text(strong && strong.textContent) || text(fallback);
  }

  function categorySection(card, kind) {
    const key = kind === 'genre' ? 'genres' : 'folders';
    return closestTarget(card, '[data-home-discovery-rail="' + key + '"]');
  }

  function inlineId(kind) {
    return 'vdr-suite-home-inline-' + (kind === 'genre' ? 'genres' : 'folders');
  }

  function findExpansion(section) {
    return section && typeof section.querySelector === 'function'
      ? section.querySelector('.media-home-inline-expansion')
      : null;
  }

  function setCategoryExpanded(section, selectedCard, expanded) {
    if (!section || typeof section.querySelectorAll !== 'function') return;
    const cards = section.querySelectorAll(CATEGORY_SELECTOR);
    Array.prototype.forEach.call(cards, function (card) {
      const selected = expanded && card === selectedCard;
      if (card.classList && typeof card.classList.toggle === 'function') {
        card.classList.toggle('inline-selected', selected);
      }
      if (typeof card.setAttribute === 'function') {
        card.setAttribute('aria-expanded', selected ? 'true' : 'false');
        if (selected) card.setAttribute('aria-controls', inlineId(categoryKind(card)));
        else card.removeAttribute('aria-controls');
      }
    });
  }

  function removeExpansion(section) {
    const existing = findExpansion(section);
    if (existing && typeof existing.remove === 'function') existing.remove();
  }

  function resetInline(kind, section) {
    const state = inline[kind];
    state.request += 1;
    state.key = '';
    state.backendId = '';
    state.card = null;
    state.section = null;
    if (kind === 'folder') {
      state.rootPath = '';
      state.path = '';
    }
    if (section) {
      removeExpansion(section);
      setCategoryExpanded(section, null, false);
    }
  }

  function makeInlineShell(section, kind, card, label, backLabel, onBack) {
    removeExpansion(section);
    setCategoryExpanded(section, card, true);
    const expansion = doc.createElement('div');
    expansion.id = inlineId(kind);
    expansion.className = 'media-home-inline-expansion';
    expansion.setAttribute('data-home-inline-kind', kind);

    const heading = doc.createElement('div');
    heading.className = 'media-home-inline-heading';
    if (backLabel && typeof onBack === 'function') {
      const back = doc.createElement('button');
      back.type = 'button';
      back.className = 'media-home-inline-back';
      back.textContent = backLabel;
      back.addEventListener('click', onBack);
      heading.appendChild(back);
    }
    const title = doc.createElement('h4');
    title.textContent = label;
    heading.appendChild(title);
    expansion.appendChild(heading);
    section.appendChild(expansion);
    return expansion;
  }

  function renderInlineState(expansion, message, error) {
    if (!expansion) return;
    const state = doc.createElement('div');
    state.className = 'media-home-inline-state' + (error ? ' error' : '');
    state.setAttribute('role', 'status');
    state.textContent = message;
    expansion.appendChild(state);
  }

  function recordingId(recording) {
    return text(recording && (recording.recordingId || recording.id));
  }

  function recordingBackendId(recording, fallback) {
    return text(recording && recording.backendId) || text(fallback);
  }

  function recordingMetadata(recording) {
    return recording && recording.metadata && typeof recording.metadata === 'object'
      ? recording.metadata
      : {};
  }

  function recordingTitle(recording) {
    const metadata = recordingMetadata(recording);
    const presentation = metadata.presentation && typeof metadata.presentation === 'object'
      ? metadata.presentation
      : {};
    const provider = metadata.provider && typeof metadata.provider === 'object'
      ? metadata.provider
      : {};
    return text(
      presentation.title ||
      provider.seriesTitle ||
      provider.title ||
      (recording && recording.title)
    ) || 'Aufnahme';
  }

  function recordingSubtitle(recording) {
    const metadata = recordingMetadata(recording);
    const presentation = metadata.presentation && typeof metadata.presentation === 'object'
      ? metadata.presentation
      : {};
    const provider = metadata.provider && typeof metadata.provider === 'object'
      ? metadata.provider
      : {};
    return text(presentation.subtitle || provider.episodeTitle);
  }

  function recordingPosterUrl(recording) {
    const metadata = recordingMetadata(recording);
    const presentation = metadata.presentation && typeof metadata.presentation === 'object'
      ? metadata.presentation
      : {};
    const artwork = metadata.artwork && typeof metadata.artwork === 'object'
      ? metadata.artwork
      : {};
    return text(presentation.posterUrl || artwork.preferredUrl);
  }

  function publicPath(path) {
    const value = text(path);
    const resolver = global.VdrSuitePublicUrl;
    return value && resolver && typeof resolver.resolvePath === 'function'
      ? resolver.resolvePath(value)
      : value;
  }

  function canonicalRecordings(payload, backendId) {
    return list(payload, 'recordings').filter(function (recording) {
      return Boolean(
        recordingId(recording) &&
        recordingBackendId(recording, backendId) === backendId
      );
    });
  }

  function canonicalFolders(payload) {
    return list(payload, 'folders').filter(function (entry) {
      return Boolean(entry && text(entry.path || entry.folderPath || entry.name));
    });
  }

  function embeddedLeafRecording(entry, backendId) {
    if (!entry || entry.singleRecordingLeaf !== true) return null;
    const recording = entry.singleRecording;
    if (!recording || typeof recording !== 'object') return null;
    if (!recordingId(recording) || recordingBackendId(recording, backendId) !== backendId) return null;
    return recording;
  }

  function projectFolderEntries(payload, backendId) {
    const folders = [];
    const recordings = [];
    canonicalFolders(payload).forEach(function (entry) {
      const embedded = embeddedLeafRecording(entry, backendId);
      if (embedded) recordings.push(embedded);
      else folders.push(entry);
    });
    return {folders: folders, recordings: recordings};
  }

  function createInlineArtwork(recording) {
    const artwork = doc.createElement('div');
    artwork.className = 'media-home-discovery-artwork';
    const title = recordingTitle(recording);
    const poster = recordingPosterUrl(recording);
    if (!poster) {
      artwork.textContent = title.slice(0, 1).toUpperCase() || '▶';
      return artwork;
    }
    const image = doc.createElement('img');
    image.src = publicPath(poster);
    image.alt = 'Poster zu ' + title;
    image.loading = 'lazy';
    image.addEventListener('error', function () {
      image.remove();
      artwork.textContent = title.slice(0, 1).toUpperCase() || '▶';
    });
    artwork.appendChild(image);
    return artwork;
  }

  function ensureRecordings2() {
    if (global.VdrSuiteRecordings2 &&
        typeof global.VdrSuiteRecordings2.openRecording === 'function') {
      return Promise.resolve(true);
    }
    const runtimes = global.VdrSuiteDeferredFrontendRuntimes;
    if (!runtimes || typeof runtimes.loadRecordings2 !== 'function') return Promise.resolve(false);
    return Promise.resolve(runtimes.loadRecordings2()).then(function () {
      return Boolean(global.VdrSuiteRecordings2 &&
        typeof global.VdrSuiteRecordings2.openRecording === 'function');
    }).catch(function () {
      return false;
    });
  }

  function returnHome() {
    if (typeof global.selectModule === 'function') global.selectModule('overview');
  }

  function openInlineRecording(recording, backendId) {
    const scopedBackend = recordingBackendId(recording, backendId);
    if (!recordingId(recording) || !scopedBackend) return Promise.resolve(false);
    const preview = global.VdrSuiteHomeLivePreview;
    if (preview && typeof preview.cancel === 'function') {
      preview.cancel('Recording Discovery Inline-Aufnahme geöffnet');
    }
    return ensureRecordings2().then(function (ready) {
      if (!ready || typeof global.selectModule !== 'function') return false;
      global.selectModule('recordings2');
      global.VdrSuiteRecordings2.openRecording(recording, {
        backendId: scopedBackend,
        backLabel: '← Zurück zu Home',
        onClose: returnHome
      });
      return true;
    });
  }

  function createInlineRecordingCard(recording, backendId) {
    const card = doc.createElement('button');
    card.type = 'button';
    card.className = 'media-home-discovery-card recording';
    card.dataset.recordingId = recordingId(recording);
    card.dataset.backendId = recordingBackendId(recording, backendId);
    card.appendChild(createInlineArtwork(recording));

    const copy = doc.createElement('span');
    copy.className = 'media-home-discovery-copy';
    const title = doc.createElement('strong');
    title.textContent = recordingTitle(recording);
    copy.appendChild(title);
    const subtitle = recordingSubtitle(recording);
    if (subtitle) {
      const detail = doc.createElement('span');
      detail.textContent = subtitle;
      copy.appendChild(detail);
    }
    card.appendChild(copy);
    card.addEventListener('click', function () {
      openInlineRecording(recording, backendId);
    });
    return card;
  }

  function folderPath(entry) {
    return text(entry && (entry.path || entry.folderPath || entry.name));
  }

  function folderLabel(entry) {
    const path = folderPath(entry);
    return text(entry && (entry.name || entry.title)) ||
      path.split('/').filter(Boolean).pop() ||
      path;
  }

  function createInlineFolderCard(entry, onOpen) {
    const path = folderPath(entry);
    const card = doc.createElement('button');
    card.type = 'button';
    card.className = 'media-home-discovery-card media-home-inline-folder';
    card.dataset.inlineFolderPath = path;

    const copy = doc.createElement('span');
    copy.className = 'media-home-discovery-copy';
    const label = doc.createElement('strong');
    label.textContent = folderLabel(entry);
    const detail = doc.createElement('span');
    const total = Number(entry.totalCount || entry.count || entry.recordingCount || 0);
    detail.textContent = total > 0 ? String(total) + ' Aufnahmen' : 'Ordner';
    copy.append(label, detail);
    card.appendChild(copy);
    card.addEventListener('click', function () { onOpen(path); });
    return card;
  }

  function pageHasMore(payload, nextOffset, returnedCount) {
    if (payload && typeof payload.hasMore === 'boolean') return payload.hasMore;
    const candidates = [
      payload && payload.total,
      payload && payload.totalCount,
      payload && payload.recordingCount
    ];
    for (let index = 0; index < candidates.length; index += 1) {
      const value = Number(candidates[index]);
      if (Number.isFinite(value) && value >= 0) return nextOffset < Math.floor(value);
    }
    return Number(returnedCount || 0) >= INLINE_PAGE_LIMIT;
  }

  function folderHasMoreRecordings(payload, nextOffset, returnedCount) {
    if (payload && typeof payload.hasMore === 'boolean') return payload.hasMore;
    const recordingCount = Number(payload && payload.recordingCount);
    if (Number.isFinite(recordingCount) && recordingCount >= 0) {
      return nextOffset < Math.floor(recordingCount);
    }
    return Number(returnedCount || 0) >= INLINE_PAGE_LIMIT;
  }

  function fetchAllGenreRecordings(client, backendId, genreId) {
    const recordings = [];

    function page(offset) {
      return Promise.resolve(client.fetchClientGenreRecordings({
        backendId: backendId,
        genreId: genreId,
        limit: INLINE_PAGE_LIMIT,
        offset: offset,
        cache: 'no-store',
        credentials: 'same-origin'
      })).then(function (payload) {
        const raw = list(payload, 'recordings');
        Array.prototype.push.apply(recordings, canonicalRecordings(payload, backendId));
        const nextOffset = offset + raw.length;
        const hasMore = pageHasMore(payload, nextOffset, raw.length);
        if (!hasMore) return recordings;
        if (!raw.length || nextOffset <= offset) throw new Error('genre inline pagination made no progress');
        return page(nextOffset);
      });
    }

    return page(0);
  }

  function fetchFolderContents(client, backendId, path) {
    const recordings = [];
    let folders = [];

    function page(offset) {
      return Promise.resolve(client.fetchClientRecordingFolder({
        backendId: backendId,
        query: {
          path: path,
          limit: INLINE_PAGE_LIMIT,
          offset: offset
        },
        cache: 'no-store',
        credentials: 'same-origin'
      })).then(function (payload) {
        if (offset === 0) {
          const projected = projectFolderEntries(payload, backendId);
          folders = projected.folders;
          Array.prototype.push.apply(recordings, projected.recordings);
        }
        const raw = list(payload, 'recordings');
        Array.prototype.push.apply(recordings, canonicalRecordings(payload, backendId));
        const nextOffset = offset + raw.length;
        const hasMore = folderHasMoreRecordings(payload, nextOffset, raw.length);
        if (!hasMore) return {folders: folders, recordings: recordings, path: path};
        if (!raw.length || nextOffset <= offset) throw new Error('folder inline pagination made no progress');
        return page(nextOffset);
      });
    }

    return page(0);
  }

  function renderGenreContents(card, section, label, backendId, genreId, recordings, request) {
    const state = inline.genre;
    if (state.request !== request || state.key !== genreId || selectedBackendId() !== backendId) return false;
    const expansion = makeInlineShell(section, 'genre', card, label);
    if (!recordings.length) {
      renderInlineState(expansion, 'Für dieses Genre sind keine Aufnahmen verfügbar.', false);
      return true;
    }
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail media-home-inline-rail';
    recordings.forEach(function (recording) {
      rail.appendChild(createInlineRecordingCard(recording, backendId));
    });
    expansion.appendChild(rail);
    return true;
  }

  function openGenreInline(card) {
    const section = categorySection(card, 'genre');
    const genreId = categoryKey(card, 'genre');
    const backendId = selectedBackendId();
    if (!section || !genreId || !backendId) return Promise.resolve(false);

    const existing = findExpansion(section);
    if (inline.genre.key === genreId && existing) {
      resetInline('genre', section);
      return Promise.resolve(true);
    }

    const client = clientApi();
    if (!client || typeof client.fetchClientGenreRecordings !== 'function') return Promise.resolve(false);

    const state = inline.genre;
    state.request += 1;
    const request = state.request;
    state.key = genreId;
    state.backendId = backendId;
    state.section = section;
    state.card = card;
    const label = categoryLabel(card, genreId);
    const expansion = makeInlineShell(section, 'genre', card, label);
    renderInlineState(expansion, 'Aufnahmen werden geladen …', false);

    return fetchAllGenreRecordings(client, backendId, genreId).then(function (recordings) {
      return renderGenreContents(card, section, label, backendId, genreId, recordings, request);
    }).catch(function () {
      if (state.request !== request || state.key !== genreId) return false;
      const current = makeInlineShell(section, 'genre', card, label);
      renderInlineState(current, 'Die Aufnahmen dieses Genres sind vorübergehend nicht verfügbar.', true);
      return false;
    });
  }

  function parentPath(path) {
    return text(path).split('/').filter(Boolean).slice(0, -1).join('/');
  }

  function pathLeaf(path) {
    const parts = text(path).split('/').filter(Boolean);
    return parts.length ? parts[parts.length - 1] : 'Aufnahmeordner';
  }

  function renderFolderContents(card, section, backendId, rootPath, path, result, request) {
    const state = inline.folder;
    if (state.request !== request || state.key !== rootPath || state.path !== path || selectedBackendId() !== backendId) {
      return false;
    }

    const nested = path !== rootPath;
    const expansion = makeInlineShell(
      section,
      'folder',
      card,
      pathLeaf(path),
      nested ? '← ' + pathLeaf(parentPath(path) || rootPath) : '',
      nested ? function () { openFolderPathInline(parentPath(path) || rootPath); } : null
    );
    if (!result.folders.length && !result.recordings.length) {
      renderInlineState(expansion, 'Dieser Aufnahmeordner ist leer.', false);
      return true;
    }

    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail media-home-inline-rail';
    result.folders.forEach(function (entry) {
      rail.appendChild(createInlineFolderCard(entry, openFolderPathInline));
    });
    result.recordings.forEach(function (recording) {
      rail.appendChild(createInlineRecordingCard(recording, backendId));
    });
    expansion.appendChild(rail);
    return true;
  }

  function openFolderPathInline(path) {
    const state = inline.folder;
    const client = clientApi();
    const backendId = state.backendId || selectedBackendId();
    const section = state.section;
    const card = state.card;
    const rootPath = state.rootPath;
    const nextPath = text(path);
    if (!client || typeof client.fetchClientRecordingFolder !== 'function' ||
        !backendId || !section || !card || !rootPath || !nextPath) {
      return Promise.resolve(false);
    }

    state.request += 1;
    const request = state.request;
    state.path = nextPath;
    const expansion = makeInlineShell(
      section,
      'folder',
      card,
      pathLeaf(nextPath),
      nextPath !== rootPath ? '← ' + pathLeaf(parentPath(nextPath) || rootPath) : '',
      nextPath !== rootPath ? function () { openFolderPathInline(parentPath(nextPath) || rootPath); } : null
    );
    renderInlineState(expansion, 'Ordnerinhalt wird geladen …', false);

    return fetchFolderContents(client, backendId, nextPath).then(function (result) {
      return renderFolderContents(card, section, backendId, rootPath, nextPath, result, request);
    }).catch(function () {
      if (state.request !== request || state.path !== nextPath) return false;
      const current = makeInlineShell(section, 'folder', card, pathLeaf(nextPath));
      renderInlineState(current, 'Der Aufnahmeordner ist vorübergehend nicht verfügbar.', true);
      return false;
    });
  }

  function openFolderInline(card) {
    const section = categorySection(card, 'folder');
    const path = categoryKey(card, 'folder');
    const backendId = selectedBackendId();
    if (!section || !path || !backendId) return Promise.resolve(false);

    const existing = findExpansion(section);
    if (inline.folder.key === path && existing) {
      resetInline('folder', section);
      return Promise.resolve(true);
    }

    const state = inline.folder;
    state.request += 1;
    state.key = path;
    state.rootPath = path;
    state.path = path;
    state.backendId = backendId;
    state.section = section;
    state.card = card;
    return openFolderPathInline(path);
  }

  function handleInlineCategoryClick(event) {
    const card = closestTarget(event && event.target, CATEGORY_SELECTOR);
    if (!card) return false;
    const kind = categoryKind(card);
    if (!kind) return false;

    if (typeof event.preventDefault === 'function') event.preventDefault();
    if (typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation();
    else if (typeof event.stopPropagation === 'function') event.stopPropagation();

    if (kind === 'genre') openGenreInline(card);
    else openFolderInline(card);
    return true;
  }

  function handleClickCapture(event) {
    if (consumeSuppressedClick(event)) return;
    handleInlineCategoryClick(event);
  }

  function installStyles() {
    if (!doc || !doc.head || typeof doc.createElement !== 'function') return false;
    if (typeof doc.getElementById === 'function' && doc.getElementById('vdr-suite-home-mouse-drag-style')) return true;
    const style = doc.createElement('style');
    style.id = 'vdr-suite-home-mouse-drag-style';
    style.textContent = [
      '.media-home-discovery-rail,.media-home-series-season-rail{scrollbar-width:none;-ms-overflow-style:none;cursor:grab}',
      '.media-home-discovery-rail::-webkit-scrollbar,.media-home-series-season-rail::-webkit-scrollbar{display:none;width:0;height:0}',
      '.media-home-live-hero-active{cursor:grab}',
      '.media-home-mouse-dragging{cursor:grabbing!important;user-select:none!important;scroll-snap-type:none!important}',
      '.media-home-mouse-dragging *{cursor:grabbing!important;user-select:none!important}',
      '.media-home-discovery-card.inline-selected{border-color:rgba(125,211,252,.78);box-shadow:0 0 0 1px rgba(125,211,252,.22) inset}',
      '.media-home-inline-expansion{min-width:0;margin:.1rem 0 .65rem;padding-top:.45rem;border-top:1px solid rgba(148,163,184,.16)}',
      '.media-home-inline-heading{display:flex;align-items:center;gap:.65rem;margin:0 1rem .65rem}.media-home-inline-heading h4{margin:0;color:#f8fafc;font-size:1rem}',
      '.media-home-inline-back{border:1px solid rgba(148,163,184,.25);border-radius:.7rem;background:rgba(15,23,42,.78);color:#e2e8f0;padding:.45rem .62rem;cursor:pointer}',
      '.media-home-inline-folder{min-height:7rem;padding:.35rem;background:linear-gradient(145deg,rgba(30,41,59,.88),rgba(2,6,23,.94))}',
      '.media-home-inline-state{margin:0 1rem 1rem;padding:.8rem 1rem;border:1px solid rgba(148,163,184,.16);border-radius:.8rem;color:#94a3b8;background:rgba(15,23,42,.5)}',
      '.media-home-inline-state.error{border-color:rgba(239,68,68,.48);color:#fecaca}',
      '@media(max-width:46rem){.media-home-inline-heading,.media-home-inline-state{margin-left:.78rem;margin-right:.78rem}}'
    ].join('');
    doc.head.appendChild(style);
    return true;
  }

  function installMouseDrag() {
    if (documentBound || !doc || typeof doc.addEventListener !== 'function') return false;
    documentBound = true;
    installStyles();
    doc.addEventListener('pointerdown', handlePointerDown);
    doc.addEventListener('pointermove', handlePointerMove, {passive: false});
    doc.addEventListener('pointerup', handlePointerUp);
    doc.addEventListener('pointercancel', handlePointerCancel);
    doc.addEventListener('dragstart', handleNativeDragStart, true);
    doc.addEventListener('click', handleClickCapture, true);
    return true;
  }

  function load() {
    if (discoveryReady()) return Promise.resolve(true);
    if (typeof global.loadVdrSuiteDeferredRuntime !== 'function') {
      return Promise.resolve(false);
    }
    return Promise.resolve(global.loadVdrSuiteDeferredRuntime(
      'vdr-suite-home-recording-discovery-runtime',
      '/frontend/home-recording-discovery.js',
      discoveryReady
    )).then(discoveryReady).catch(function (error) {
      if (global.console && typeof global.console.error === 'function') {
        global.console.error('VDR-Suite Home Recording Discovery runtime failed', error);
      }
      return false;
    });
  }

  function loadWhenBackendReady() {
    if (discoveryReady()) return Promise.resolve(true);
    if (canonicalSelectedBackendId()) return load();
    if (!doc ||
        typeof doc.getElementById !== 'function' ||
        typeof global.MutationObserver !== 'function') {
      return Promise.resolve(false);
    }

    const backends = doc.getElementById('backends');
    if (!backends) return Promise.resolve(false);

    if (!backendReadyObserver) {
      backendReadyObserver = new global.MutationObserver(function () {
        if (!canonicalSelectedBackendId()) return;
        backendReadyObserver.disconnect();
        backendReadyObserver = null;
        load();
      });
      backendReadyObserver.observe(backends, {
        childList: true,
        subtree: true,
        attributes: true,
        attributeFilter: ['class', 'aria-selected']
      });
    }

    return Promise.resolve(false);
  }

  global.VdrSuiteHomeRecordingDiscoveryBootstrap = Object.freeze({
    load: load,
    loadWhenBackendReady: loadWhenBackendReady,
    installMouseDrag: installMouseDrag,
    __test: Object.freeze({
      canonicalSelectedBackendId: canonicalSelectedBackendId,
      fetchAllGenreRecordings: fetchAllGenreRecordings,
      fetchFolderContents: fetchFolderContents,
      openGenreInline: openGenreInline,
      openFolderInline: openFolderInline,
      openFolderPathInline: openFolderPathInline
    })
  });
  installMouseDrag();
  loadWhenBackendReady();
}(window));
