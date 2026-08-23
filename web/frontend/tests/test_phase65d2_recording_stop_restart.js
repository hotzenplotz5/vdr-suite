'use strict';

// Focused source-level guard for the Recording stop -> restart lifecycle.
// The behavioral contract remains covered in test_phase65d2_recording_playback_controls.js;
// this guard prevents the UI from returning to the previous dead stopped state.
const assert = require('assert');
const fs = require('fs');
const path = require('path');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'session-frontend-sync.js'),
  'utf8'
);

assert.ok(
  source.includes("startButton.textContent = '▶ Wiedergabe erneut starten';"),
  'Stop must expose an explicit restart action in the same recording panel'
);
assert.ok(
  source.includes("startButton.hidden = false;"),
  'Stop must reveal the restart action'
);
assert.ok(
  source.includes("activeSessionId = '';"),
  'A stopped Recording MediaSession must not remain the panel owner'
);
assert.ok(
  source.includes("if ((started && !stopped) || destroyed) return sessionCreationPromise;"),
  'A stopped panel must be allowed to create a new Recording MediaSession'
);

console.log('phase65d2 recording stop restart source contract ok');
