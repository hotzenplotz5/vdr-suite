// Phase 65.D ADR-0056 MediaPlaybackContract compatibility projection.
//
// Existing first-party playback owners keep their accepted lifecycle and
// transport topology. This adapter makes their legacy capability fields derive
// from the normalized server contract whenever contractVersion=1 is present.
(function (global) {
  'use strict';

  const api = global && global.VdrSuiteClientApi;
  if (!api || typeof api.requestJson !== 'function' ||
      api.__vdrSuitePlaybackContractConsumer === true) return;

  function boolean(value) {
    return typeof value === 'boolean' ? value : null;
  }

  function finiteNonNegative(value) {
    const number = Number(value);
    return Number.isFinite(number) && number >= 0 ? number : null;
  }

  function projectPlayback(mediaSession, contract) {
    const normalized = contract && contract.playback;
    if (!normalized || typeof normalized !== 'object') return;
    const legacy = mediaSession.playback;
    if (!legacy || typeof legacy !== 'object') return;

    const position = finiteNonNegative(normalized.positionSeconds);
    if (position !== null) legacy.positionSeconds = Math.floor(position);
    if (normalized.durationSeconds === null) legacy.durationSeconds = null;
    else {
      const duration = finiteNonNegative(normalized.durationSeconds);
      if (duration !== null) legacy.durationSeconds = Math.floor(duration);
    }

    const seekContract = contract.seek;
    if (seekContract && typeof seekContract === 'object') {
      const legacySeek = legacy.seek && typeof legacy.seek === 'object'
        ? legacy.seek
        : {};
      const inSession = seekContract.mode === 'in-session-reposition';
      const seekSupported = boolean(seekContract.supported);
      const seekPreparing = boolean(seekContract.preparing);
      if (seekSupported !== null) legacySeek.supported = inSession && seekSupported;
      if (seekPreparing !== null) legacySeek.preparing = inSession && seekPreparing;
      if (inSession && seekContract.window && typeof seekContract.window === 'object' &&
          seekSupported === true) {
        const start = finiteNonNegative(seekContract.window.startSeconds);
        const end = finiteNonNegative(seekContract.window.endSeconds);
        if (start !== null && end !== null && end > start) {
          legacySeek.window = {startSeconds: Math.floor(start), endSeconds: Math.floor(end)};
        }
      } else if (!inSession) {
        delete legacySeek.window;
      }
      legacy.seek = legacySeek;
    }

    const restart = normalized.restart;
    if (restart && typeof restart === 'object') {
      const legacyResume = legacy.resume && typeof legacy.resume === 'object'
        ? legacy.resume
        : {};
      const supported = boolean(restart.supported);
      const preparing = boolean(restart.preparing);
      if (supported !== null) legacyResume.supported = supported;
      if (preparing !== null) legacyResume.preparing = preparing;
      legacy.resume = legacyResume;
    }
  }

  function projectTracks(mediaSession, contract) {
    const normalized = contract && contract.tracks;
    const tracks = mediaSession.tracks;
    if (!normalized || typeof normalized !== 'object' ||
        !tracks || typeof tracks !== 'object') return;

    const audioSupported = boolean(
      normalized.audioSelection && normalized.audioSelection.supported
    );
    if (audioSupported !== null && tracks.audio && typeof tracks.audio === 'object') {
      tracks.audio.selectionSupported = audioSupported;
    }

    const subtitleSupported = boolean(
      normalized.subtitleSelection && normalized.subtitleSelection.supported
    );
    const subtitleOffSupported = boolean(
      normalized.subtitleOff && normalized.subtitleOff.supported
    );
    if (tracks.subtitles && typeof tracks.subtitles === 'object') {
      if (subtitleSupported !== null) tracks.subtitles.selectionSupported = subtitleSupported;
      if (subtitleOffSupported !== null) tracks.subtitles.offSupported = subtitleOffSupported;
    }
  }

  function consume(payload) {
    const mediaSession = payload && payload.mediaSession;
    const contract = mediaSession && mediaSession.playbackContract;
    if (!mediaSession || !contract || contract.contractVersion !== 1) return payload;
    if (contract.presentationProfileId && mediaSession.presentationProfileId &&
        String(contract.presentationProfileId) !== String(mediaSession.presentationProfileId)) {
      return payload;
    }
    projectPlayback(mediaSession, contract);
    projectTracks(mediaSession, contract);
    return payload;
  }

  const decorated = {};
  Object.keys(api).forEach(function (key) { decorated[key] = api[key]; });
  const requestJson = api.requestJson;
  decorated.requestJson = function () {
    const result = requestJson.apply(api, arguments);
    return result && typeof result.then === 'function'
      ? result.then(consume)
      : Promise.resolve(result).then(consume);
  };
  decorated.__vdrSuitePlaybackContractConsumer = true;

  global.VdrSuiteClientApi = Object.freeze(decorated);
  global.VdrSuitePlaybackContractConsumer = Object.freeze({
    consume: consume
  });
}(window));
