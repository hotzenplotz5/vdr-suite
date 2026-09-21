// Prewarm the existing canonical Recording playback owner while the cinematic Hero stays visible.
(function (global) {
  'use strict';

  const owner = global.VdrSuiteRecordings2HeroDetail;
  if (!owner || typeof owner.enhance !== 'function' || !owner.__test) {
    console.error('VDR-Suite Recordings 2 hero owner is unavailable for playback prewarm');
    return;
  }

  const PREWARM_DELAY_MS = 700;
  const PREWARM_PROMISE = '__vdrSuiteHeroPlaybackPrewarmPromise';
  const PREWARM_TIMER = '__vdrSuiteHeroPlaybackPrewarmTimer';

  function fastPrewarmSupported(playbackOwner) {
    if (!playbackOwner || typeof playbackOwner.start !== 'function' ||
        typeof playbackOwner.play !== 'function') return false;
    if (!global.VdrSuiteRecordingFastPlayback) return false;
    if (global.document && global.document.visibilityState === 'hidden') return false;
    try {
      if (typeof playbackOwner.state === 'function' && playbackOwner.state() !== 'idle') return false;
    } catch (error) {
      return false;
    }

    const MediaSource = global.MediaSource;
    return Boolean(
      MediaSource && typeof MediaSource.isTypeSupported === 'function' &&
      typeof global.fetch === 'function' &&
      typeof global.AbortController === 'function' &&
      typeof global.ReadableStream === 'function' &&
      global.URL && typeof global.URL.createObjectURL === 'function' &&
      typeof global.URL.revokeObjectURL === 'function'
    );
  }

  function prewarmPlayback(root) {
    if (!root || root[PREWARM_PROMISE]) return root ? root[PREWARM_PROMISE] : null;
    if (!root.dataset || root.dataset.recordings2HeroMode !== 'detail') return null;
    if (root.isConnected === false) return null;

    const playbackOwner = root.__vdrSuiteRecordingPlaybackOwner;
    if (!fastPrewarmSupported(playbackOwner)) return null;

    let request = null;
    try {
      request = playbackOwner.start({autoPlay: false});
    } catch (error) {
      return null;
    }

    root[PREWARM_PROMISE] = Promise.resolve(request).catch(function () { return ''; });
    return root[PREWARM_PROMISE];
  }

  function schedulePlaybackPrewarm(root) {
    if (!root || root[PREWARM_PROMISE] || root[PREWARM_TIMER] ||
        typeof global.setTimeout !== 'function') return;

    root[PREWARM_TIMER] = global.setTimeout(function () {
      root[PREWARM_TIMER] = null;
      if (!root.dataset || root.dataset.recordings2HeroMode !== 'detail') return;
      prewarmPlayback(root);
    }, PREWARM_DELAY_MS);
  }

  function playPrepared(root, recording) {
    if (!root) return;
    const playbackOwner = root.__vdrSuiteRecordingPlaybackOwner;
    if (!playbackOwner) return;

    const position = typeof owner.__test.resumePosition === 'function'
      ? owner.__test.resumePosition(recording)
      : 0;

    if (typeof owner.__test.showMode === 'function') {
      owner.__test.showMode(root, 'playback', '.recordings2-volume-owner-shell');
    }

    if (typeof global.setTimeout !== 'function') return;
    global.setTimeout(function () {
      const prepared = root[PREWARM_PROMISE];
      let request = prepared ? Promise.resolve(prepared) : Promise.resolve(null);

      request = request.then(function () {
        if (position > 0 && typeof playbackOwner.startAtAbsolute === 'function') {
          return playbackOwner.startAtAbsolute(position);
        }
        if (position > 0 && typeof playbackOwner.start === 'function' &&
            typeof playbackOwner.seekAbsolute === 'function') {
          return Promise.resolve(playbackOwner.start()).then(function (sessionId) {
            return sessionId ? playbackOwner.seekAbsolute(position) : sessionId;
          });
        }
        if (typeof playbackOwner.play === 'function') return playbackOwner.play();
        if (typeof playbackOwner.start === 'function') return playbackOwner.start();
        return null;
      }).then(function (result) {
        if (position > 0 && typeof playbackOwner.play === 'function') {
          return Promise.resolve(playbackOwner.play()).then(function () { return result; });
        }
        return result;
      });

      request.catch(function () {});
    }, 0);
  }

  function replacePrimaryAction(root, recording) {
    if (!root || typeof root.querySelector !== 'function') return;
    const actions = root.querySelector('.recordings2-hero-actions');
    if (!actions || typeof actions.querySelector !== 'function') return;
    const primary = actions.querySelector('button.primary');
    if (!primary || primary.dataset && primary.dataset.vdrSuitePrewarmAction === 'true' ||
        typeof primary.cloneNode !== 'function') return;

    const replacement = primary.cloneNode(true);
    if (replacement.dataset) replacement.dataset.vdrSuitePrewarmAction = 'true';
    replacement.addEventListener('click', function () { playPrepared(root, recording); });
    primary.replaceWith(replacement);
  }

  function enhance(root, recording, backendId, metadata) {
    const result = owner.enhance(root, recording, backendId, metadata);
    replacePrimaryAction(root, recording);
    schedulePlaybackPrewarm(root);
    return result;
  }

  global.VdrSuiteRecordings2HeroDetail = Object.freeze({
    enhance,
    __test: Object.freeze(Object.assign({}, owner.__test, {
      fastPrewarmSupported,
      prewarmPlayback,
      schedulePlaybackPrewarm,
      playPrepared
    }))
  });
}(window));
