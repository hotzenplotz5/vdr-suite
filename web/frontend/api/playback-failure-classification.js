// Phase 65.D ADR-0056 classified playback failure semantics.
//
// This helper is deliberately observational. It converts browser transport and
// HTMLMediaElement failures into the normalized semantic shape but never starts,
// stops, recreates or replaces a MediaSession/transport. Recovery remains an
// explicit decision of the already-authoritative playback owner.
(function (global) {
  'use strict';

  if (!global || global.VdrSuitePlaybackFailureClassification) return;

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function failure(category, origin, stage, terminal, recoveryClass, reasonCode) {
    return Object.freeze({
      category: category,
      origin: origin,
      stage: stage,
      terminal: terminal === true,
      recoveryClass: recoveryClass,
      reasonCode: reasonCode
    });
  }

  function errorName(error) {
    return text(error && error.name).trim();
  }

  function errorMessage(error) {
    return text(error && error.message).trim();
  }

  function classifyClientTransportError(error) {
    const name = errorName(error);
    const message = errorMessage(error);

    if (name === 'QuotaExceededError') {
      return failure(
        'buffer',
        'client-transport',
        'source-buffer-append',
        true,
        'none',
        'client_source_buffer_quota_exceeded'
      );
    }

    if (message.indexOf('kontinuierliche fMP4-Daten nicht verarbeiten') !== -1) {
      return failure(
        'buffer',
        'client-transport',
        'source-buffer-operation',
        true,
        'none',
        'client_source_buffer_error'
      );
    }

    if (message.indexOf('Puffer enthält kein vollständiges Medienfragment') !== -1) {
      return failure(
        'buffer',
        'client-transport',
        'fragment-buffer',
        true,
        'none',
        'client_fragment_buffer_incomplete'
      );
    }

    if (message.indexOf('liefert kein begrenztes Init-Segment') !== -1) {
      return failure(
        'buffer',
        'client-transport',
        'init-buffer',
        true,
        'none',
        'client_init_segment_incomplete'
      );
    }

    if (message.indexOf('Codec-Konfiguration nicht') !== -1 || name === 'NotSupportedError') {
      return failure(
        'client-platform',
        'platform-player',
        'codec-support',
        true,
        'none',
        'client_mse_codec_unsupported'
      );
    }

    if (message.indexOf('Streaming-Reader') !== -1) {
      return failure(
        'client-platform',
        'client-transport',
        'stream-reader',
        true,
        'none',
        'client_stream_reader_unavailable'
      );
    }

    if (message.indexOf('Stream konnte nicht geladen werden') !== -1) {
      return failure(
        'transport',
        'client-transport',
        'stream-fetch',
        true,
        'none',
        'client_stream_fetch_failed'
      );
    }

    if (message.indexOf('MediaSource wurde vor dem Öffnen geschlossen') !== -1) {
      return failure(
        'transport',
        'client-transport',
        'media-source-open',
        true,
        'none',
        'client_media_source_closed'
      );
    }

    return failure(
      'transport',
      'client-transport',
      'streaming',
      true,
      'none',
      'client_transport_failed'
    );
  }

  function classifyPlatformMediaError(mediaError) {
    const code = Number(mediaError && mediaError.code);

    if (code === 1) {
      return failure(
        'client-platform',
        'platform-player',
        'media-playback',
        true,
        'none',
        'client_media_aborted'
      );
    }

    if (code === 2) {
      return failure(
        'transport',
        'platform-player',
        'media-playback',
        true,
        'none',
        'client_media_network_error'
      );
    }

    if (code === 3) {
      return failure(
        'decoder',
        'platform-player',
        'decode',
        true,
        'none',
        'client_media_decode_error'
      );
    }

    if (code === 4) {
      return failure(
        'client-platform',
        'platform-player',
        'source-support',
        true,
        'none',
        'client_media_source_not_supported'
      );
    }

    return failure(
      'client-platform',
      'platform-player',
      'media-playback',
      true,
      'none',
      'client_media_error'
    );
  }

  global.VdrSuitePlaybackFailureClassification = Object.freeze({
    classifyClientTransportError: classifyClientTransportError,
    classifyPlatformMediaError: classifyPlatformMediaError
  });
}(window));
