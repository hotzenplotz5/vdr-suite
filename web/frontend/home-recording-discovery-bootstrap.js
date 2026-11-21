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
  const RAIL_SELECTOR = '.media-home-discovery-rail, .media-home-series-season-rail, .media-home-live-guide-rail';
  const RAIL_NEAR_END_EVENT = 'vdr-suite-home-rail-near-end';
  const RAIL_NEAR_END_THRESHOLD = 320;
  const PREFETCH_START_DELAY_MS = 500;
  // Optional speculative reads only; initial and user-driven reads keep
  // their existing independent owners.
  // Two bounded owner slots, one speculative HTTP request in flight.
  const pendingRailPrefetch = new Map();
  let railPrefetchScheduled = false;
  let railPrefetchRunning = false;
  const HERO_SELECTOR = '.media-home-hero.media-home-live-hero-active[data-home-zone="hero"]';
  const CATEGORY_SELECTOR = '.media-home-discovery-card.genre, .media-home-discovery-card.folder';
  const DRAG_CLASS = 'media-home-mouse-dragging';
  const INLINE_PAGE_LIMIT = 100;
  const HOME_FOLDER_INITIAL = 12;
  const HOME_FOLDER_PREFETCH = 24;
  const HOME_FOLDER_PREFETCH_MS = 30000;
  const INLINE_METADATA_CONCURRENCY = 4;
  const INLINE_FOLDER_ARTWORK_CONCURRENCY = 3;
  const INLINE_FOLDER_ARTWORK_READS = 2;
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

  function railPrefetchAllowed() {
    const session = global.VdrSuiteBrowserSession;
    const owner = platform();
    const net = global.navigator && global.navigator.connection;
    return Boolean(session && typeof session.isAuthenticated === 'function' &&
      session.isAuthenticated() && owner && typeof owner.getSelectedModule === 'function' &&
      owner.getSelectedModule() === 'overview' && !(doc && doc.hidden) &&
      (!global.navigator || global.navigator.onLine !== false) &&
      (!net || (!net.saveData && !['slow-2g', '2g'].includes(net.effectiveType))));
  }

  function drainRailPrefetch() {
    railPrefetchScheduled = false;
    if (railPrefetchRunning) return;
    const next = pendingRailPrefetch.entries().next();
    if (next.done) return;
    const [key, entry] = next.value;
    pendingRailPrefetch.delete(key);
    if (!railPrefetchAllowed() || !entry.isCurrent()) {
      armRailPrefetch();
      return;
    }
    railPrefetchRunning = true;
    Promise.resolve().then(function () {
      if (railPrefetchAllowed() && entry.isCurrent()) return entry.run();
      return false;
    }).catch(function () { return false; }).then(function () {
      railPrefetchRunning = false;
      armRailPrefetch();
    });
  }

  function armRailPrefetch() {
    if (!pendingRailPrefetch.size || railPrefetchScheduled || railPrefetchRunning ||
        typeof global.setTimeout !== 'function') return;
    railPrefetchScheduled = true;
    global.setTimeout(function () {
      if (typeof global.requestIdleCallback === 'function')
        global.requestIdleCallback(drainRailPrefetch, {timeout: 2000});
      else drainRailPrefetch();
    }, PREFETCH_START_DELAY_MS);
  }

  function scheduleRailPrefetch(isCurrent, run, key = 'movies') {
    if (!railPrefetchAllowed() || typeof isCurrent !== 'function' ||
        typeof run !== 'function' || !isCurrent()) return false;
    if (key !== 'movies' && key !== 'folder') return false;
    pendingRailPrefetch.set(key, {isCurrent, run});
    armRailPrefetch();
    return true;
  }

  function railNearEnd(rail) {
    if (!rail) return false;
    const scrollWidth = Number(rail.scrollWidth) || 0;
    const clientWidth = Number(rail.clientWidth) || 0;
    const scrollLeft = Math.max(0, Number(rail.scrollLeft) || 0);
    if (clientWidth <= 0 || scrollWidth <= clientWidth) return false;
    const classes = String(rail.className || '').split(/\s+/);
    const networkPaged = classes.includes('recent-movies') || classes.includes('media-home-live-guide-rail') ||
      classes.includes('media-home-inline-folder-paged');
    const threshold = networkPaged ? Math.max(RAIL_NEAR_END_THRESHOLD, Math.min(1200, clientWidth)) : RAIL_NEAR_END_THRESHOLD;
    return scrollWidth - scrollLeft - clientWidth <= threshold;
  }

  function prioritizeFolderRail(rail) {
    const discovery = global.VdrSuiteHomeRecordingDiscovery;
    const classes = String(rail && rail.className || '').split(/\s+/);
    // The same bounded lookahead must follow user scrolling in ordinary
    // movie/genre rails, not only inline folders and series episodes.
    // Live-guide images are handled by their own owner.
    if (rail && (classes.includes('media-home-discovery-rail') ||
        (rail.closest && rail.closest('[data-home-inline-kind="folder"]')) ||
        classes.includes('series-episodes') || classes.includes('series')) &&
        discovery && typeof discovery.prioritizeInlineRailArtwork === 'function')
      discovery.prioritizeInlineRailArtwork(rail);
  }

  function handleRailScroll(event) {
    const rail = closestTarget(event && event.target, RAIL_SELECTOR);
    prioritizeFolderRail(rail);
    if (!rail || !railNearEnd(rail) || !doc ||
        typeof doc.dispatchEvent !== 'function' ||
        typeof global.CustomEvent !== 'function') return false;
    doc.dispatchEvent(new global.CustomEvent(RAIL_NEAR_END_EVENT, {detail: {rail: rail}}));
    return true;
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
    if (existing && typeof existing.__stopFolderSeries === 'function') existing.__stopFolderSeries();
    if (existing && typeof existing.__cancelFolderPreview === 'function') existing.__cancelFolderPreview();
    if (existing && typeof existing.__stopFolderArtwork === 'function') existing.__stopFolderArtwork();
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
      state.paged = false;
      state.paging = null;
    }
    if (section) {
      removeExpansion(section);
      setCategoryExpanded(section, null, false);
    }
  }

  function makeInlineShell(section, kind, card, label, backLabel, onBack) {
    const previous = kind === 'folder' ? findExpansion(section) : null;
    const previousHeight = previous && typeof previous.getBoundingClientRect === 'function'
      ? previous.getBoundingClientRect().height : 0;
    removeExpansion(section);
    setCategoryExpanded(section, card, true);
    const expansion = doc.createElement('div');
    if (previousHeight > 0 && expansion.style) {
      expansion.style.boxSizing = 'border-box';
      expansion.style.minHeight = Math.ceil(previousHeight) + 'px';
    }
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

  function releaseFolderHeight(expansion) {
    if (expansion && expansion.style) expansion.style.minHeight = '';
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

  function recordingBackendNativeId(recording) {
    return text(recording && recording.backendNativeId);
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

  function recordingMetadataPosterUrl(value) {
    const helpers = global.VdrSuiteFrontendHelpers;
    if (!helpers || typeof helpers.recordingMetadataPosterUrl !== 'function') return '';
    return text(helpers.recordingMetadataPosterUrl(value));
  }

  function recordingMetadataProjection(recording, richMetadata) {
    if (!recording) return recording;
    const rich = richMetadata && richMetadata.available === true ? richMetadata : {};
    const sourceMetadata = recordingMetadata(recording);
    const sourcePresentation = sourceMetadata.presentation &&
      typeof sourceMetadata.presentation === 'object'
      ? sourceMetadata.presentation
      : {};
    const sourceArtwork = sourceMetadata.artwork &&
      typeof sourceMetadata.artwork === 'object'
      ? sourceMetadata.artwork
      : {};
    const baseTitle = recordingTitle(recording);
    const fallbackTitle = baseTitle.indexOf('/') >= 0
      ? text(baseTitle.split('/').filter(Boolean).pop())
      : baseTitle;
    const richTitle = text(rich.title);
    const richTitleIsPath = Boolean(richTitle && richTitle.indexOf('/') >= 0);
    const title = richTitle && !richTitleIsPath ? richTitle : fallbackTitle;
    const posterUrl = recordingMetadataPosterUrl(rich) || recordingPosterUrl(recording);
    return Object.assign({}, recording, {
      path:'',
      title:title,
      metadata:Object.assign({}, sourceMetadata, {
        presentation:Object.assign({}, sourcePresentation, {
          title:title,
          subtitle:text(rich.episodeName) || recordingSubtitle(recording),
          summary:text(rich.overview) || text(sourcePresentation.summary),
          posterUrl:posterUrl
        }),
        artwork:Object.assign({}, sourceArtwork, {preferredUrl:posterUrl})
      })
    });
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

  function createInlineArtwork(recording, target) {
    const artwork = target || doc.createElement('div');
    artwork.textContent = '';
    artwork.className = 'media-home-discovery-artwork';
    function showNoArtwork() {
      artwork.className = 'media-home-discovery-artwork no-artwork';
      artwork.textContent = 'No Artwork';
    }
    const title = recordingTitle(recording);
    const poster = recordingPosterUrl(recording);
    if (!poster) {
      showNoArtwork();
      return artwork;
    }
    const image = doc.createElement('img');
    const helpers = global.VdrSuiteFrontendHelpers;
    const preview = helpers && typeof helpers.homeArtworkPreviewUrl === 'function'
      ? helpers.homeArtworkPreviewUrl(poster) : poster;
    image.alt = 'Poster zu ' + title;
    image.loading = 'lazy';
    image.decoding = 'async';
    const url = publicPath(preview);
    const discovery = global.VdrSuiteHomeRecordingDiscovery;
    if (discovery && typeof discovery.deferHomeArtworkImage === 'function')
      discovery.deferHomeArtworkImage(image, url);
    else image.src = url;
    image.addEventListener('error', function () {
      image.remove();
      showNoArtwork();
    });
    artwork.appendChild(image);
    if (discovery && typeof discovery.prioritizeVisibleArtwork === 'function') discovery.prioritizeVisibleArtwork(image);
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

  function createInlineRecordingCard(recording, backendId, originalRecording) {
    const openValue = originalRecording || recording;
    const card = doc.createElement('button');
    card.type = 'button';
    card.className = 'media-home-discovery-card recording';
    card.dataset.recordingId = recordingId(openValue);
    card.dataset.backendId = recordingBackendId(openValue, backendId);
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
      openInlineRecording(openValue, backendId);
    });
    return card;
  }

  function folderPath(entry) {
    return text(entry && (entry.path || entry.folderPath || entry.name));
  }

  function folderLabel(entry) {
    const path = folderPath(entry);
    const label = text(entry && (entry.name || entry.title)) ||
      path.split('/').filter(Boolean).pop() || path;
    const shared = global.VdrSuiteRecordings2Shared;
    return shared && typeof shared.decodeDisplayText === 'function'
      ? shared.decodeDisplayText(label) : label.replace(/_/g, ' ');
  }

  function createInlineFolderCard(entry, onOpen, backendId) {
    const path = folderPath(entry);
    const card = doc.createElement('button');
    card.type = 'button';
    card.className = 'media-home-discovery-card media-home-inline-folder';
    card.dataset.inlineFolderPath = path;
    const discovery = global.VdrSuiteHomeRecordingDiscovery;
    const series = discovery && typeof discovery.folderSeriesCard === 'function'
      ? discovery.folderSeriesCard(entry, backendId) : null;
    const artwork = createInlineArtwork({title: series ? series.title : folderLabel(entry),
      metadata:{presentation:{posterUrl: series ? series.posterUrl : text(entry.representativePosterUrl)}}});
    card.appendChild(artwork);

    const copy = doc.createElement('span');
    copy.className = 'media-home-discovery-copy';
    const label = doc.createElement('strong');
    label.textContent = series ? series.title : folderLabel(entry);
    const detail = doc.createElement('span');
    const total = Number(entry.totalCount || entry.count || entry.recordingCount || 0);
    detail.textContent = series ? series.subtitle : total > 0 ? String(total) + ' Aufnahmen' : 'Ordner';
    copy.append(label, detail);
    card.appendChild(copy);
    card.addEventListener('click', function () { onOpen(path, entry); });
    return {card: card, artwork: artwork, entry: entry, sharedSeries:Boolean(series)};
  }

  // The same canonical representative traversal as Recordings2, but only
  // for visible inline folder cards, with a small read budget per card.
  function hydrateInlineFolderArtwork(expansion, cards, backendId, isCurrent) {
    const client = clientApi();
    if (!client || typeof client.fetchClientRecordingFolder !== 'function') return;
    if (typeof expansion.__appendFolderArtwork === 'function') {
      expansion.__appendFolderArtwork(cards);
      return;
    }
    let stopped = false, active = 0, observer = null;
    const pending = [];
    const live = () => !stopped && isCurrent() && expansion.isConnected !== false;
    const paint = (item, recording) => {
      if (!live() || !recording || !recordingId(recording) ||
          recordingBackendId(recording, backendId) !== backendId) return false;
      const projected = recordingMetadataProjection(recording, recording.nativeMetadata);
      if (!recordingPosterUrl(projected)) return false;
      createInlineArtwork(projected, item.artwork);
      return true;
    };
    expansion.__stopFolderArtwork = function () {
      stopped = true;
      pending.length = 0;
      if (observer) observer.disconnect();
    };

    function owner() {
      const existing = global.VdrSuiteRecordings2FolderArtwork;
      if (existing && typeof existing.findRepresentative === 'function') return Promise.resolve(existing);
      const runtime = global.VdrSuiteDeferredFrontendRuntimes;
      return runtime && typeof runtime.loadRecordings2 === 'function'
        ? Promise.resolve(runtime.loadRecordings2()).then(() => global.VdrSuiteRecordings2FolderArtwork)
        : Promise.resolve(null);
    }

    function resolve(item) {
      if (paint(item, item.entry.representativeRecording)) return Promise.resolve();
      const root = folderPath(item.entry);
      let remaining = INLINE_FOLDER_ARTWORK_READS;
      const load = function (path) {
        if (!live() || remaining <= 0 || (path !== root && !path.startsWith(root + '/'))) return Promise.resolve(null);
        remaining--;
        return Promise.resolve(client.fetchClientRecordingFolder({
          backendId: backendId,
          query: {path: path, limit: 1, offset: 0},
          cache: 'no-store',
          credentials: 'same-origin'
        })).then(function (payload) {
          if (!live()) return null;
          const projected = projectFolderEntries(payload, backendId);
          const recordings = canonicalRecordings(payload, backendId).concat(projected.recordings);
          // Prefer an already available poster, not a metadata request.
          const withPoster = recordings.filter(recording =>
            recordingPosterUrl(recordingMetadataProjection(recording, recording.nativeMetadata)));
          return {recordings: withPoster.length ? withPoster : recordings, folders: projected.folders};
        });
      };
      return owner().then(function (api) {
        if (!live() || !api || typeof api.findRepresentative !== 'function') return null;
        return load(root).then(page => live()
          ? api.findRepresentative(page, load, 1, new Set([root])) : null);
      }).then(recording => paint(item, recording));
    }

    function pump() {
      while (live() && active < INLINE_FOLDER_ARTWORK_CONCURRENCY && pending.length) {
        const item = pending.shift();
        active++;
        Promise.resolve().then(() => live() && resolve(item)).catch(() => {}).then(() => {
          active--;
          pump();
        });
      }
    }

    function enqueueVisible(item) {
      // The paginated server projection already contains the selected cover;
      // only older servers need the previous bounded representative traversal.
      if (!live() || item.queued || typeof item.entry.representativePosterUrl === 'string') return;
      item.queued = true;
      pending.push(item);
      pump();
    }

    const items = new Map();
    if (typeof global.IntersectionObserver === 'function') {
      observer = new global.IntersectionObserver(function (entries) {
        if (!live()) { observer.disconnect(); return; }
        entries.forEach(function (entry) {
          if (!entry.isIntersecting) return;
          observer.unobserve(entry.target);
          const item = items.get(entry.target);
          items.delete(entry.target);
          if (item) enqueueVisible(item);
        });
      }, {rootMargin: '160px'});
    }
    expansion.__appendFolderArtwork = function (added) {
      if (!live()) return;
      added = added.filter(item => !item.sharedSeries && typeof item.entry.representativePosterUrl !== 'string');
      if (observer) {
        added.forEach(item => { items.set(item.card, item); observer.observe(item.card); });
      } else {
        added.slice(0, 10).forEach(enqueueVisible);
      }
    };
    expansion.__appendFolderArtwork(cards);
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

  function renderGenreContents(card, section, label, backendId, genreId, recordings, request, client) {
    const state = inline.genre;
    if (state.request !== request || state.key !== genreId || selectedBackendId() !== backendId) return Promise.resolve(false);
    const expansion = makeInlineShell(section, 'genre', card, label);
    if (!recordings.length) {
      renderInlineState(expansion, 'Für dieses Genre sind keine Aufnahmen verfügbar.', false);
      return Promise.resolve(true);
    }
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail media-home-inline-rail';
    expansion.appendChild(rail);

    if (!client || typeof client.requestJson !== 'function') {
      recordings.forEach(function (recording) {
        rail.appendChild(createInlineRecordingCard(recording, backendId));
      });
      prioritizeFolderRail(rail);
      return Promise.resolve(true);
    }

    const settled = new Array(recordings.length);
    recordings.forEach(function (recording, index) {
      const cardMetadata = recording.nativeCardMetadata;
      if (cardMetadata && cardMetadata.available === true) settled[index] = cardMetadata;
    });
    let cursor = 0;
    let published = 0;

    function stillCurrent() {
      return state.request === request &&
        state.key === genreId &&
        state.backendId === backendId &&
        selectedBackendId() === backendId;
    }

    function publishReady() {
      if (!stillCurrent()) return;
      const previous = published;
      while (published < recordings.length && settled[published] !== undefined) {
        const original = recordings[published];
        const rich = settled[published];
        const projected = recordingMetadataProjection(original, rich);
        rail.appendChild(createInlineRecordingCard(projected, backendId, original));
        published += 1;
      }
      // Re-arm only when newly attached cards need an initial visible read.
      if (published > previous) prioritizeFolderRail(rail);
    }

    function worker() {
      if (!stillCurrent()) return Promise.resolve();
      while (cursor < recordings.length && settled[cursor] !== undefined) cursor += 1;
      const index = cursor;
      cursor += 1;
      if (index >= recordings.length) return Promise.resolve();
      const recording = recordings[index];
      const nativeId = recordingBackendNativeId(recording);
      if (!nativeId) {
        settled[index] = null;
        publishReady();
        return worker();
      }
      return Promise.resolve(client.requestJson('/api/vdr/recordings/metadata', {
        query: {
          backend: backendId,
          backendNativeId: nativeId
        },
        cache: 'no-store',
        credentials: 'same-origin'
      })).then(function (value) {
        settled[index] = value && value.available === true ? value : null;
      }).catch(function () {
        settled[index] = null;
      }).then(function () {
        publishReady();
        return worker();
      });
    }

    publishReady();
    const workers = [];
    const workerCount = Math.min(INLINE_METADATA_CONCURRENCY, recordings.length);
    for (let index = 0; index < workerCount; index += 1) workers.push(worker());
    return Promise.all(workers).then(function () {
      publishReady();
      return stillCurrent();
    });
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
      return renderGenreContents(card, section, label, backendId, genreId, recordings, request, client);
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
    releaseFolderHeight(expansion);
    if (!result.folders.length && !result.recordings.length) {
      renderInlineState(expansion, 'Dieser Aufnahmeordner ist leer.', false);
      return true;
    }

    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail media-home-inline-rail';
    const folderCards = result.folders.map(function (entry) {
      const item = createInlineFolderCard(entry, openFolderPathInline, backendId);
      rail.appendChild(item.card);
      return item;
    });
    result.recordings.forEach(function (recording) {
      const projected = recordingMetadataProjection(
        recording,
        recording && recording.nativeMetadata
      );
      rail.appendChild(
        createInlineRecordingCard(projected, backendId, recording)
      );
    });
    expansion.appendChild(rail);
    prioritizeFolderRail(rail);
    hydrateInlineFolderArtwork(expansion, folderCards, backendId, function () {
      return state.request === request && state.key === rootPath && state.path === path &&
        selectedBackendId() === backendId && findExpansion(section) === expansion;
    });
    return true;
  }

  // All inline Home folder navigation uses the opt-in combined card page.
  // Legacy servers continue through the existing complete-query fallback.
  function folderPageCurrent(paging) {
    const state = inline.folder;
    const session = global.VdrSuiteBrowserSession;
    const owner = platform();
    return state.paging === paging && state.request === paging.request &&
      state.backendId === paging.backendId && selectedBackendId() === paging.backendId &&
      state.path === paging.path && findExpansion(state.section) === paging.expansion &&
      paging.expansion.isConnected !== false &&
      (!session || (typeof session.isAuthenticated === 'function' && session.isAuthenticated())) &&
      (!owner || typeof owner.getSelectedModule !== 'function' || owner.getSelectedModule() === 'overview');
  }

  function fetchHomeFolderPage(paging, limit) {
    const offset = paging.offset;
    return Promise.resolve(paging.client.fetchClientRecordingFolder({
      backendId: paging.backendId,
      query: {path: paging.path, limit: limit, offset: offset, homeCards: 1},
      cache: 'no-store',
      credentials: 'same-origin'
    })).then(function (payload) {
      if (!folderPageCurrent(paging)) return null;
      if (payload && payload.recordingFolder === true && payload.homeCardPage !== true && offset === 0)
        return {legacy: true};
      const returned = list(payload, 'folders').length + list(payload, 'recordings').length;
      if (!payload || payload.homeCardPage !== true || Number(payload.offset) !== offset ||
          Number(payload.returnedCards) !== returned || returned > limit ||
          typeof payload.hasMore !== 'boolean' || (!returned && payload.hasMore))
        throw new Error('invalid Home folder card page');
      if (offset === 0 && paging.ticket) paging.ticket.accept(payload);
      const projected = projectFolderEntries(payload, paging.backendId);
      return {folders: projected.folders,
        recordings: projected.recordings.concat(canonicalRecordings(payload, paging.backendId)),
        offset: offset + returned, hasMore: payload.hasMore};
    });
  }

  function scheduleFolderPrefetch(paging) {
    const cache = global.VdrSuiteHomeCache;
    if (!folderPageCurrent(paging) || !paging.hasMore || paging.loading || paging.prefetch ||
        !cache || typeof cache.captureRailFence !== 'function') return;
    const fence = cache.captureRailFence('folders', paging.backendId);
    if (!fence) return;
    const offset = paging.offset;
    const valid = () => folderPageCurrent(paging) && fence() && paging.offset === offset &&
      !paging.loading && !paging.prefetch && paging.hasMore;
    scheduleRailPrefetch(valid, function () {
      if (!valid()) return false;
      const entry = {offset, fence, receivedAt: 0, promise: null};
      paging.prefetch = entry;
      entry.promise = Promise.resolve().then(() => fetchHomeFolderPage(paging, HOME_FOLDER_PREFETCH))
        .then(function (page) {
          if (!page || !folderPageCurrent(paging) || !fence() || paging.prefetch !== entry) return null;
          entry.receivedAt = Date.now();
          return page;
        }).catch(() => null).then(function (page) {
          if (!page && paging.prefetch === entry) paging.prefetch = null;
          return page;
        });
      return entry.promise;
    }, 'folder');
  }

  function folderPreviewCards(page) {
    return page.folders.map(entry => ({
      id:'f:' + folderPath(entry), title:folderLabel(entry),
      subtitle: Number(entry.recordingCount || 0) > 0 ? entry.recordingCount + ' Aufnahmen' : 'Ordner',
      posterUrl: text(entry.representativePosterUrl) ||
        (entry.representativeRecording ? recordingPosterUrl(recordingMetadataProjection(
          entry.representativeRecording, entry.representativeRecording.nativeMetadata)) : '')
    })).concat(page.recordings.map(recording => {
      const projected = recordingMetadataProjection(recording, recording.nativeMetadata);
      return {id:'r:' + recordingId(recording), title:recordingTitle(projected),
        subtitle:recordingSubtitle(projected), posterUrl:recordingPosterUrl(projected)};
    }));
  }

  function paintFolderPreview(paging, items) {
    if (!folderPageCurrent(paging) || !items.length || paging.offset !== 0) return null;
    const preview = doc.createElement('div');
    preview.className = 'media-home-discovery-rail media-home-inline-rail';
    preview.setAttribute('aria-busy', 'true');
    for (const item of items) {
      const card = doc.createElement('div');
      card.className = 'media-home-discovery-card recording';
      card.appendChild(createInlineArtwork({title:item.title,
        metadata:{presentation:{posterUrl:item.posterUrl}}}));
      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const title = doc.createElement('strong');
      title.textContent = item.title;
      const detail = doc.createElement('span');
      detail.textContent = item.subtitle;
      copy.append(title, detail);
      card.appendChild(copy);
      preview.appendChild(card);
    }
    paging.rail.hidden = true;
    paging.more.hidden = true;
    paging.notice.textContent = 'Zuletzt geladen – wird aktualisiert';
    paging.expansion.appendChild(preview);
    releaseFolderHeight(paging.expansion);
    prioritizeFolderRail(preview);
    return () => {
      preview.remove();
      paging.rail.hidden = false;
      if (paging.more) paging.more.hidden = false;
      if (paging.notice) paging.notice.textContent = 'Ordnerinhalt wird geladen …';
    };
  }

  function publishFolderPage(paging, page) {
    if (!page || !folderPageCurrent(paging)) return false;
    if (paging.offset === 0 && paging.ticket) {
      paging.ticket.finish(folderPreviewCards(page));
      paging.ticket = null;
    }
    if (paging.notice) { paging.notice.remove(); paging.notice = null; }
    const addedFolders = [];
    const backendId = paging.backendId;
    page.folders.forEach(function (entry) {
      const key = 'f:' + folderPath(entry);
      if (paging.seen.has(key)) return;
      paging.seen.add(key);
      const item = createInlineFolderCard(entry, openFolderPathInline, backendId);
      paging.rail.appendChild(item.card);
      addedFolders.push(item);
    });
    page.recordings.forEach(function (recording) {
      const key = 'r:' + recordingId(recording);
      if (paging.seen.has(key)) return;
      paging.seen.add(key);
      const projected = recordingMetadataProjection(recording, recording.nativeMetadata);
      paging.rail.appendChild(createInlineRecordingCard(projected, backendId, recording));
    });
    releaseFolderHeight(paging.expansion);
    prioritizeFolderRail(paging.rail);
    paging.offset = page.offset;
    paging.hasMore = page.hasMore;
    if (addedFolders.length)
      hydrateInlineFolderArtwork(paging.expansion, addedFolders, backendId, () => folderPageCurrent(paging));
    if (!paging.hasMore && paging.more) { paging.more.remove(); paging.more = null; }
    if (!paging.seen.size && !paging.hasMore)
      renderInlineState(paging.expansion, 'Dieser Aufnahmeordner ist leer.', false);
    return true;
  }

  function loadMoreFolderCards(paging) {
    if (!folderPageCurrent(paging) || paging.loading || !paging.hasMore) return Promise.resolve(false);
    const buffered = paging.prefetch;
    const fresh = () => buffered && paging.prefetch === buffered && buffered.offset === paging.offset &&
      buffered.fence() && (!buffered.receivedAt || Date.now() - buffered.receivedAt <= HOME_FOLDER_PREFETCH_MS);
    const demand = () => folderPageCurrent(paging)
      ? fetchHomeFolderPage(paging, HOME_FOLDER_INITIAL) : Promise.resolve(null);
    if (!fresh()) paging.prefetch = null;
    const load = Promise.resolve().then(function () {
      return fresh() ? buffered.promise.then(page => page && fresh() ? page : demand()) : demand();
    }).then(function (page) {
      if (paging.prefetch === buffered) paging.prefetch = null;
      if (page && page.legacy && folderPageCurrent(paging)) {
        const state = inline.folder;
        if (paging.ticket) paging.ticket.cancel();
        paging.ticket = null;
        return fetchFolderContents(paging.client, paging.backendId, paging.path).then(result =>
          renderFolderContents(state.card, state.section, paging.backendId, state.rootPath,
            paging.path, result, paging.request));
      }
      return publishFolderPage(paging, page);
    }).catch(function () {
      if (folderPageCurrent(paging)) {
        if (paging.ticket) paging.ticket.cancel();
        paging.ticket = null;
        releaseFolderHeight(paging.expansion);
        if (paging.notice) paging.notice.remove();
        const notice = doc.createElement('p');
        notice.className = 'media-home-inline-state error';
        notice.setAttribute('role', 'status');
        notice.textContent = 'Ordnerinhalt vorübergehend nicht verfügbar. Erneut versuchen.';
        paging.expansion.appendChild(notice);
        paging.notice = notice;
      }
      return false;
    });
    paging.loading = load;
    return load.then(function (result) {
      if (paging.loading === load) paging.loading = null;
      if (result) scheduleFolderPrefetch(paging);
      return result;
    });
  }

  function openPagedFolder(client, backendId, path, request, expansion) {
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail media-home-inline-rail media-home-inline-folder-paged';
    expansion.appendChild(rail);
    const more = doc.createElement('button');
    more.type = 'button';
    more.className = 'media-home-inline-back';
    more.textContent = 'Weitere Aufnahmen';
    expansion.appendChild(more);
    const notice = doc.createElement('p');
    notice.className = 'media-home-inline-state';
    notice.setAttribute('role', 'status');
    notice.textContent = 'Ordnerinhalt wird geladen …';
    expansion.appendChild(notice);
    const paging = {client, backendId, path, request, expansion, rail, more,
      offset: 0, hasMore: true, loading: null, prefetch: null, seen: new Set(), notice};
    inline.folder.paging = paging;
    const cache = global.VdrSuiteHomeCache;
    paging.ticket = cache && typeof cache.beginFolder === 'function'
      ? cache.beginFolder(path, backendId, () => folderPageCurrent(paging),
          items => paintFolderPreview(paging, items)) : null;
    expansion.__cancelFolderPreview = () => {
      if (paging.ticket) paging.ticket.cancel();
      paging.ticket = null;
    };
    more.addEventListener('click', () => loadMoreFolderCards(paging));
    return loadMoreFolderCards(paging);
  }

  function handleFolderRailNearEnd(event) {
    const paging = inline.folder.paging;
    if (!paging || !event || !event.detail || event.detail.rail !== paging.rail) return false;
    return loadMoreFolderCards(paging);
  }

  function openFolderPathInline(path, entry, generic = false) {
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
    state.paging = null;
    const expansion = makeInlineShell(
      section,
      'folder',
      card,
      pathLeaf(nextPath),
      nextPath !== rootPath ? '← ' + pathLeaf(parentPath(nextPath) || rootPath) : '',
      nextPath !== rootPath ? function () { openFolderPathInline(parentPath(nextPath) || rootPath); } : null
    );
    const discovery = global.VdrSuiteHomeRecordingDiscovery;
    const currentEntry = nextPath === rootPath ? () => card.__vdrSuiteFolderEntry : () => entry;
    const count = () => {
      const value = currentEntry();
      return Number(value && (value.totalCount || value.count || value.recordingCount) || 0);
    };
    const stillCurrent = () => state.request === request && state.path === nextPath &&
      selectedBackendId() === backendId && findExpansion(section) === expansion &&
      expansion.isConnected !== false &&
      (!global.VdrSuiteBrowserSession || (typeof global.VdrSuiteBrowserSession.isAuthenticated === 'function' &&
        global.VdrSuiteBrowserSession.isAuthenticated())) &&
      (!platform() || typeof platform().getSelectedModule !== 'function' || platform().getSelectedModule() === 'overview');
    if (!generic && discovery && typeof discovery.mountFolderSeries === 'function' &&
        discovery.mountFolderSeries(expansion, nextPath, backendId, {
          count, current:stillCurrent, direct:nextPath !== rootPath,
          back:() => openFolderPathInline(parentPath(nextPath) || rootPath),
          fallback:() => { if (stillCurrent()) openFolderPathInline(nextPath, entry, true); }
        })) {
      releaseFolderHeight(expansion);
      return Promise.resolve(true);
    }
    if (generic && nextPath === rootPath && /^(?:serien|serie|series)$/i.test(nextPath)) {
      const seriesButton = doc.createElement('button');
      seriesButton.type = 'button';
      seriesButton.className = 'media-home-inline-back';
      seriesButton.textContent = 'Serienansicht';
      seriesButton.addEventListener('click', () => {
        if (stillCurrent()) openFolderPathInline(nextPath, entry);
      });
      expansion.appendChild(seriesButton);
    }
    if (state.paged) return openPagedFolder(client, backendId, nextPath, request, expansion);
    renderInlineState(expansion, 'Ordnerinhalt wird geladen …', false);

    return fetchFolderContents(client, backendId, nextPath).then(function (result) {
      return renderFolderContents(card, section, backendId, rootPath, nextPath, result, request);
    }).catch(function () {
      if (state.request !== request || state.path !== nextPath) return false;
      const current = makeInlineShell(section, 'folder', card, pathLeaf(nextPath));
      releaseFolderHeight(current);
      renderInlineState(current, 'Der Aufnahmeordner ist vorübergehend nicht verfügbar.', true);
      return false;
    });
  }

  function openFolderInline(card, paged = true) {
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
    state.paged = paged;
    return openFolderPathInline(path);
  }

  function openRandomFolderInline(card) {
    const section = categorySection(card, 'folder');
    // The scheduled automatic selection must not replace user navigation.
    if (!section || findExpansion(section)) return Promise.resolve(false);
    const session = global.VdrSuiteBrowserSession;
    if (session && (!session.isAuthenticated || !session.isAuthenticated())) return Promise.resolve(false);
    return openFolderInline(card, true);
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
      '.media-home-discovery-rail,.media-home-series-season-rail,.media-home-live-guide-rail{scrollbar-width:none;-ms-overflow-style:none;cursor:grab}',
      '.media-home-discovery-rail::-webkit-scrollbar,.media-home-series-season-rail::-webkit-scrollbar,.media-home-live-guide-rail::-webkit-scrollbar{display:none;width:0;height:0}',
      '.media-home-live-hero-active{cursor:grab}',
      '.media-home-mouse-dragging{cursor:grabbing!important;user-select:none!important;scroll-snap-type:none!important}',
      '.media-home-mouse-dragging *{cursor:grabbing!important;user-select:none!important}',
      '.media-home-discovery-card.inline-selected{border-color:rgba(125,211,252,.78);box-shadow:0 0 0 1px rgba(125,211,252,.22) inset}',
      '.media-home-inline-expansion{min-width:0;margin:.1rem 0 .65rem;padding-top:.45rem;border-top:1px solid rgba(148,163,184,.16)}',
      '.media-home-inline-heading{display:flex;align-items:center;gap:.65rem;margin:0 1rem .65rem}.media-home-inline-heading h4{margin:0;color:#f8fafc;font-size:1rem}',
      '.media-home-inline-back{border:1px solid rgba(148,163,184,.25);border-radius:.7rem;background:rgba(15,23,42,.78);color:#e2e8f0;padding:.45rem .62rem;cursor:pointer}',
      '.media-home-inline-folder{min-height:7rem;padding:0;background:linear-gradient(145deg,rgba(30,41,59,.88),rgba(2,6,23,.94))}',
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
    doc.addEventListener('scroll', handleRailScroll, true);
    doc.addEventListener(RAIL_NEAR_END_EVENT, handleFolderRailNearEnd);
    const session = global.VdrSuiteBrowserSession;
    if (session && typeof session.subscribe === 'function') {
      session.subscribe(function (value) {
        if (value && value.authenticated) return;
        if (inline.folder.section) resetInline('folder', inline.folder.section);
      });
    }
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
    nearEndEventName: RAIL_NEAR_END_EVENT,
    scheduleRailPrefetch: scheduleRailPrefetch,
    openRandomFolderInline: openRandomFolderInline,
    __test: Object.freeze({
      canonicalSelectedBackendId: canonicalSelectedBackendId,
      railNearEnd: railNearEnd,
      handleRailScroll: handleRailScroll,
      fetchAllGenreRecordings: fetchAllGenreRecordings,
      fetchFolderContents: fetchFolderContents,
      recordingMetadataProjection: recordingMetadataProjection,
      renderGenreContents: renderGenreContents,
      openGenreInline: openGenreInline,
      openFolderInline: openFolderInline,
      openFolderPathInline: openFolderPathInline
    })
  });
  installMouseDrag();
  loadWhenBackendReady();
}(window));
