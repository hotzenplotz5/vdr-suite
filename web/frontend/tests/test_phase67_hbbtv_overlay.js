'use strict';

const assert = require('assert');
const fs = require('fs');

const live = fs.readFileSync('web/frontend/live-tv-view.js', 'utf8');
const client = fs.readFileSync('web/frontend/api/client-api.js', 'utf8');
const qoi = fs.readFileSync('web/frontend/hbbtv-qoi.js', 'utf8');

for (const token of [
  'function openHbbtvSession()',
  'function closeHbbtvSession()',
  'function pollHbbtvPresentation(sequence)',
  'function sendHbbtvInput(action)',
  'function alignHbbtvCanvas()',
  'function drawHbbtvPresentation(frame)',
  'function keyboardHbbtvAction(event)',
  "hbbtvOverlay.className = 'vdr-suite-hbbtv-overlay'",
  "hbbtvOverlay.setAttribute('role', 'application')",
  "fetchClientHbbtvSessionLaunch",
  "fetchClientHbbtvSessionStatus",
  "fetchClientHbbtvSessionInput",
  "fetchClientHbbtvSessionClose",
  "fetchClientHbbtvPresentation",
  "state.hbbtvFrameRevision = 0",
  "releaseHbbtvSessionBestEffort();",
  "alignHbbtvCanvas();"
]) {
  assert(live.includes(token), token);
}

assert(live.includes(
  "slot.appendChild(state.playback.element);"
));
assert(live.includes(
  "slot.appendChild(hbbtvOverlay);"
));
assert(live.indexOf("slot.appendChild(state.playback.element);") <
       live.indexOf("slot.appendChild(hbbtvOverlay);"));

for (const action of [
  "'up'", "'down'", "'left'", "'right'", "'ok'", "'back'",
  "'red'", "'green'", "'yellow'", "'blue'"
]) {
  assert(live.includes(action), action);
}

for (const token of [
  'VdrSuiteQoi',
  'Uint8ClampedArray',
  'hbbtv_qoi_magic_invalid'
]) {
  assert(qoi.includes(token), token);
}

for (const token of [
  'fetchClientHbbtvSessionLaunch: fetchClientHbbtvSessionLaunch',
  'fetchClientHbbtvSessionStatus: fetchClientHbbtvSessionStatus',
  'fetchClientHbbtvSessionInput: fetchClientHbbtvSessionInput',
  'fetchClientHbbtvSessionClose: fetchClientHbbtvSessionClose',
  'fetchClientHbbtvPresentation: fetchClientHbbtvPresentation'
]) {
  assert(client.includes(token), token);
}

for (const forbidden of [
  'HBBAPPS',
  'HBBRUN',
  'HBBPRES',
  'RedButton',
  'LoadUrl',
  'ProcessKey',
  'executeJavascript',
  'VK_LEFT',
  'VK_ENTER'
]) {
  assert(!live.includes(forbidden), forbidden);
  assert(!client.includes(forbidden), forbidden);
}

assert(!live.includes('fetch('));

console.log('Phase 67 HbbTV Live-TV overlay contract ok');
