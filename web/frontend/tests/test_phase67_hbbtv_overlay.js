'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

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
  'function hbbtvMediaCompositionTarget(',
  'function applyHbbtvMediaVideoComposition(canvas, video, baseVideoRect)',
  'function restoreHbbtvMediaVideoComposition()',
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
  'hbbtvMediaAttached: false',
  'hbbtvMediaVideoStyle: null',
  "video.style.setProperty('z-index', '11')",
  'hbbtvMediaPresentationBaselineRevision: 0',
  'hbbtvMediaPresentationReady: false',
  'function clearHbbtvMediaPresentationHole()',
  "video.style.setProperty('pointer-events', 'none')",
  "video.style.setProperty('transform-origin', '0 0')",
  'state.hbbtvMediaAttached = true;',
  'state.hbbtvMediaGeometry',
  'applyHbbtvMediaVideoComposition(canvas, video, videoRect);'
]) {
  assert(live.includes(token), token);
}

const compositionStart = live.indexOf('function hbbtvMediaCompositionTarget(');
const compositionEnd = live.indexOf(
  '\n  function snapshotHbbtvMediaVideoStyle',
  compositionStart
);
assert(compositionStart >= 0 && compositionEnd > compositionStart);
const compositionTarget = vm.runInNewContext(
  '(' + live.slice(compositionStart, compositionEnd).trim() + ')'
);

assert.strictEqual(
  JSON.stringify(compositionTarget(
    1280,
    720,
    true,
    {x: 100, y: 50, width: 640, height: 360}
  )),
  JSON.stringify({x: 0, y: 0, width: 1280, height: 720}),
  'fullscreen HbbTV broadband video must occupy the whole presentation viewport'
);
assert.strictEqual(
  JSON.stringify(compositionTarget(
    1280,
    720,
    false,
    {x: 100, y: 50, width: 640, height: 360}
  )),
  JSON.stringify({x: 100, y: 50, width: 640, height: 360}),
  'windowed HbbTV broadband video must honor provider presentation geometry'
);
assert.strictEqual(
  compositionTarget(
    1280,
    720,
    false,
    {x: -1, y: 50, width: 640, height: 360}
  ),
  null,
  'invalid provider geometry must fail closed instead of covering application UI'
);

const attachPosition = live.indexOf('state.hbbtvMediaAttached = true;');
const switchSuccessPosition = live.indexOf(
  "if (!switched) throw new Error('hbbtv_media_player_switch_failed');"
);
assert(
  switchSuccessPosition >= 0 && attachPosition > switchSuccessPosition,
  'the browser video plane may only be composed after canonical external-stream attachment succeeds'
);
assert(
  live.indexOf('clearHbbtvMediaPresentationHole();', attachPosition) > attachPosition,
  'external-media attach must clear the stale presentation media rectangle'
);
assert(
  live.includes('revision <= state.hbbtvMediaPresentationBaselineRevision'),
  'stale in-flight presentation frames must not repaint the cleared media rectangle'
);
assert(
  live.includes('state.hbbtvMediaPresentationReady = true;'),
  'a newer provider frame must release the presentation fence'
);

const measurePosition = live.indexOf('restoreHbbtvMediaVideoComposition();', live.indexOf('function alignHbbtvCanvas()'));
const videoRectPosition = live.indexOf('const videoRect = video.getBoundingClientRect();', live.indexOf('function alignHbbtvCanvas()'));
assert(
  measurePosition >= 0 && videoRectPosition > measurePosition,
  'canvas alignment must measure the untransformed canonical video viewport'
);

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
