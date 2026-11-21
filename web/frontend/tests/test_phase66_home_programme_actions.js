'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-live-hero.js'), 'utf8');

function functionSlice(name, nextName) {
  const start = source.indexOf('function ' + name + '(');
  assert(start >= 0, name + ' must exist');
  const end = nextName ? source.indexOf('function ' + nextName + '(', start + 1) : -1;
  return source.slice(start, end > start ? end : source.length);
}

const timerDelegate = functionSlice('createProgrammeTimer', 'programmeDetailOwnerReady');
assert(timerDelegate.includes("typeof createEpgTimerFromDetail !== 'function'"));
assert(timerDelegate.includes('createEpgTimerFromDetail(feedback, entry.event, entry.channel, button);'));
assert(!timerDelegate.includes('fetchClientTimerCreateAction'));
assert(!timerDelegate.includes('/api/vdr/timers/actions/create'));

const detailLoader = functionSlice('loadProgrammeDetailOwner', 'clearProgrammeDetails');
assert(detailLoader.includes('VdrSuiteDeferredFrontendRuntimes'));
assert(detailLoader.includes('runtimes.loadEpgDetail()'));
assert(detailLoader.includes('global.loadVdrSuiteDeferredRuntime'));
assert(detailLoader.includes("'/frontend/epg-detail-owner.js'"));
assert(detailLoader.includes('VdrSuiteEpgDetailOwner'));

const detailOpen = functionSlice('openProgrammeDetail', 'createProgrammeGuideCard');
assert(detailOpen.includes("card.closest('[data-home-live-guide]')"));
assert(detailOpen.includes("shell.dataset.homeLiveGuideDetail = 'true'"));
assert(detailOpen.includes("shell.addEventListener('click'"));
assert(detailOpen.includes('owner.createCard(entry.event, entry.channel)'));
assert(detailOpen.includes('clearProgrammeDetails(card)'));
assert(detailOpen.includes("status.textContent = 'Sendungsdetails werden geladen …'"));

const card = functionSlice('createProgrammeGuideCard', 'renderProgrammeRail');
assert(card.includes("if (current)"));
assert(card.includes("createButton('Live TV'"));
assert(card.includes('watchChannel(entry.channel)'));
assert(card.includes("createButton('Timer erstellen'"));
assert(card.includes('createProgrammeTimer(entry, timer, feedback)'));
assert(card.includes('card.tabIndex = 0'));
assert(card.includes("card.setAttribute('aria-expanded', 'false')"));
assert(card.includes("card.addEventListener('click'"));
assert(card.includes("card.addEventListener('keydown'"));
assert(card.includes("event.key !== 'Enter' && event.key !== ' '"));
assert(card.includes('openProgrammeDetail(entry, card)'));
assert(card.includes('event.stopPropagation'));
assert(card.indexOf("createButton('Live TV'") > card.indexOf('if (current)'));
assert(card.indexOf("createButton('Timer erstellen'") > card.indexOf("createButton('Live TV'"));

const liveDelegate = functionSlice('watchChannel', 'watchLive');
assert(liveDelegate.includes('VdrSuiteLiveTvView'));
assert(liveDelegate.includes('liveOwner.startChannel(channel)'));

assert(source.includes('.media-home-live-guide-actions.single{grid-template-columns:1fr}'));
assert(source.includes('.media-home-live-guide-action.primary'));
assert(source.includes('.media-home-live-guide-card.detail-selected'));
assert(source.includes('.media-home-live-guide-detail{'));
assert(source.includes('.media-home-live-guide-detail>.epg-event-detail'));
assert(!source.includes('/api/media/sessions'));

console.log('phase66 Home programme card action and metadata detail delegation contract ok');
