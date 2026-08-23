// Stop/restart choice UI for the existing Recording playback owner.
// This module does not create MediaSessions or perform media requests itself.
(function (global) {
  'use strict';

  function formatTime(value) {
    const seconds = Math.max(0, Math.floor(Number(value) || 0));
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const remaining = seconds % 60;
    return String(hours).padStart(2, '0') + ':' +
      String(minutes).padStart(2, '0') + ':' +
      String(remaining).padStart(2, '0');
  }

  function createButton(label, className) {
    const button = global.document.createElement('button');
    button.type = 'button';
    button.textContent = label;
    button.className = className || '';
    return button;
  }

  function install(playback) {
    if (!playback || !playback.element ||
        typeof playback.start !== 'function' ||
        typeof playback.stop !== 'function') return null;
    const panel = playback.element;
    if (panel.__vdrSuiteRestartChoice) return panel.__vdrSuiteRestartChoice;
    if (typeof panel.querySelector !== 'function') return null;

    const startButton = panel.querySelector('button.recordings2-primary');
    const stopButton = panel.querySelector('button[aria-label="Wiedergabe stoppen"]');
    const timeline = panel.querySelector('input[aria-label="Wiedergabeposition"]');
    const status = panel.querySelector('.recordings2-playback-status');
    if (!startButton || !stopButton || typeof stopButton.addEventListener !== 'function') return null;

    const choices = global.document.createElement('div');
    choices.className = 'recordings2-playback-restart-choice';
    choices.hidden = true;
    choices.style.display = 'flex';
    choices.style.flexWrap = 'wrap';
    choices.style.gap = '0.5rem';
    choices.style.marginTop = '0.65rem';

    const resumeButton = createButton('', 'recordings2-primary');
    const fromStartButton = createButton('↺ Wiedergabe von vorn');
    choices.appendChild(resumeButton);
    choices.appendChild(fromStartButton);

    if (typeof panel.insertBefore === 'function') {
      panel.insertBefore(choices, startButton.nextSibling || null);
    }
    else {
      panel.appendChild(choices);
    }

    let stopPosition = 0;
    let canResume = false;
    let stopping = false;

    function showChoices() {
      const time = formatTime(stopPosition);
      startButton.hidden = true;
      choices.hidden = false;
      resumeButton.hidden = !canResume;
      resumeButton.disabled = false;
      fromStartButton.disabled = false;
      resumeButton.textContent = '▶ Wiedergabe ab ' + time + ' fortsetzen';
      if (status) {
        status.textContent = canResume
          ? 'Wiedergabe gestoppt · Wiedergabe ab ' + time + ' fortsetzen?'
          : 'Wiedergabe gestoppt · Wiedergabe von vorn möglich.';
      }
    }

    function begin(position) {
      const target = Math.max(0, Math.floor(Number(position) || 0));
      choices.hidden = true;
      resumeButton.disabled = true;
      fromStartButton.disabled = true;
      startButton.hidden = true;
      return Promise.resolve(playback.start()).then(function (sessionId) {
        if (!sessionId) throw new Error('Neue Recording-MediaSession wurde nicht gestartet.');
        if (target <= 0) return sessionId;
        if (typeof playback.seekAbsolute !== 'function') {
          throw new Error('Fortsetzen ist für diese Aufnahme nicht verfügbar.');
        }
        return Promise.resolve(playback.seekAbsolute(target)).then(function () {
          return sessionId;
        });
      }).catch(function (error) {
        const state = typeof playback.state === 'function' ? playback.state() : '';
        if (state === 'stopped') showChoices();
        throw error;
      });
    }

    stopButton.addEventListener('click', function (event) {
      if (event && typeof event.preventDefault === 'function') event.preventDefault();
      if (event && typeof event.stopImmediatePropagation === 'function') {
        event.stopImmediatePropagation();
      }
      if (stopping) return;

      const position = typeof playback.position === 'function'
        ? Number(playback.position())
        : 0;
      stopPosition = Number.isFinite(position) && position > 0 ? Math.floor(position) : 0;
      canResume = Boolean(
        stopPosition > 0 &&
        timeline && timeline.disabled === false &&
        typeof playback.seekAbsolute === 'function'
      );
      stopping = true;
      choices.hidden = true;
      resumeButton.disabled = true;
      fromStartButton.disabled = true;

      let request;
      try {
        request = playback.stop();
      }
      catch (error) {
        stopping = false;
        startButton.hidden = false;
        return;
      }

      // The Recording owner exposes its generic restart button synchronously
      // while server cleanup is still in flight. Keep that implementation
      // detail hidden and wait for the owner's actual stop promise instead of
      // polling DOM state with a timeout.
      startButton.hidden = true;
      Promise.resolve(request).then(function (stopped) {
        stopping = false;
        const state = typeof playback.state === 'function' ? playback.state() : '';
        if (stopped === false || state !== 'stopped') {
          startButton.hidden = false;
          return;
        }
        showChoices();
      }).catch(function () {
        stopping = false;
        startButton.hidden = false;
      });
    }, true);

    resumeButton.addEventListener('click', function () {
      begin(stopPosition).catch(function () {});
    });
    fromStartButton.addEventListener('click', function () {
      begin(0).catch(function () {});
    });

    const api = Object.freeze({
      choices: choices,
      resumeButton: resumeButton,
      fromStartButton: fromStartButton,
      stopPosition: function () { return stopPosition; },
      canResume: function () { return canResume; },
      stopping: function () { return stopping; }
    });
    try {
      Object.defineProperty(panel, '__vdrSuiteRestartChoice', {
        configurable: false,
        enumerable: false,
        writable: false,
        value: api
      });
    }
    catch (error) {
      panel.__vdrSuiteRestartChoice = api;
    }
    return api;
  }

  global.VdrSuiteRecordingPlaybackRestartChoice = Object.freeze({
    install: install,
    __test: Object.freeze({formatTime: formatTime})
  });
}(window));
