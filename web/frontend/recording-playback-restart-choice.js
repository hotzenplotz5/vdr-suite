// Stop/restart choice UI for the existing Recording playback owner.
// This module does not create MediaSessions or perform media requests itself.
(function (global) {
  'use strict';

  const POLL_DELAY_MS = 25;
  const MAX_POLL_ATTEMPTS = 240;

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
    if (!playback || !playback.element || typeof playback.start !== 'function') return null;
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
    let stopGeneration = 0;

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

    function waitForStopped(generation, attempt) {
      if (generation !== stopGeneration) return;
      const state = typeof playback.state === 'function' ? playback.state() : '';
      if (state === 'destroyed' || state === 'fallback') return;
      if (state === 'stopped' && startButton.hidden === false && startButton.disabled === false) {
        showChoices();
        return;
      }
      if (attempt >= MAX_POLL_ATTEMPTS || typeof global.setTimeout !== 'function') return;
      global.setTimeout(function () {
        waitForStopped(generation, attempt + 1);
      }, POLL_DELAY_MS);
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

    stopButton.addEventListener('click', function () {
      const position = typeof playback.position === 'function'
        ? Number(playback.position())
        : 0;
      stopPosition = Number.isFinite(position) && position > 0 ? Math.floor(position) : 0;
      canResume = Boolean(
        stopPosition > 0 &&
        timeline && timeline.disabled === false &&
        typeof playback.seekAbsolute === 'function'
      );
      const generation = ++stopGeneration;
      if (typeof global.setTimeout === 'function') {
        global.setTimeout(function () { waitForStopped(generation, 0); }, 0);
      }
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
      canResume: function () { return canResume; }
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
