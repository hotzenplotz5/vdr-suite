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
  'function pollHbbtvMedia(sequence)',
  'function startHbbtvMediaPlayback(media, sequence)',
  'function stopHbbtvMediaPlayback(sequence, notifyServer, restoreBroadcast)',
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
  "fetchClientHbbtvMedia",
  "mutateClientHbbtvMedia",
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

for (const token of [
  "button(\n      'Vollbild',\n      'vdr-suite-live-tv-fullscreen'\n    )",
  "typeof slot.requestFullscreen !== 'function'",
  "const request = slot.requestFullscreen();",
  "slot.addEventListener('fullscreenchange'",
  "alignHbbtvCanvas();",
  ".vdr-suite-live-tv-player-slot:fullscreen",
  "max-height:none!important",
  "object-fit:contain!important"
]) {
  assert(live.includes(token), token);
}

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
  'fetchClientHbbtvMedia: fetchClientHbbtvMedia',
  'mutateClientHbbtvMedia: mutateClientHbbtvMedia',
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

for (const token of [
  'hbbtvPresentationInFlight: false',
  'hbbtvPresentationKickPending: false',
  'function kickHbbtvPresentation(sequence)',
  'state.hbbtvPresentationKickPending = true;',
  "scheduleHbbtvPresentation(sequence, 0);",
  'kickHbbtvPresentation(sessionSequence);',
  'inputKickPending'
]) {
  assert(live.includes(token), token);
}

assert(
  live.indexOf('state.hbbtvInputDiagnostic.responseAt = responseAt;') <
  live.indexOf('kickHbbtvPresentation(sessionSequence);')
);

console.log('Phase 67 HbbTV Live-TV overlay contract ok');
