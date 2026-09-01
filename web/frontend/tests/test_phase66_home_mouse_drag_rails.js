'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery-bootstrap.js'), 'utf8');

assert(source.includes("const RAIL_SELECTOR = '.media-home-discovery-rail, .media-home-series-season-rail, .media-home-live-guide-rail'"));
assert(source.includes('scrollbar-width:none'));
assert(source.includes('::-webkit-scrollbar'));
assert(source.includes("event.pointerType !== 'mouse'"));
assert(source.includes('setPointerCapture'));
assert(source.includes('scrollLeft = activeDrag.startScrollLeft - deltaX'));
assert(source.includes('VdrSuiteHomeLiveHero'));
assert(source.includes('hero.selectOffset(deltaX < 0 ? 1 : -1)'));
assert(source.includes("doc.addEventListener('click', handleClickCapture, true)"));
assert(!source.includes('/api/media/sessions'));
assert(!source.includes('MediaSession'));
assert(!source.includes('navigator.mediaSession'));

function classList(initial) {
  const values = new Set(String(initial || '').split(/\s+/).filter(Boolean));
  return {
    add(...names) { names.forEach(name => values.add(name)); },
    remove(...names) { names.forEach(name => values.delete(name)); },
    contains(name) { return values.has(name); },
    values
  };
}

function matches(element, selector) {
  const value = String(selector || '').trim();
  if (value === '.media-home-discovery-rail') return element.classList.contains('media-home-discovery-rail');
  if (value === '.media-home-series-season-rail') return element.classList.contains('media-home-series-season-rail');
  if (value === '.media-home-live-guide-rail') return element.classList.contains('media-home-live-guide-rail');
  if (value === '.media-home-hero.media-home-live-hero-active[data-home-zone="hero"]') {
    return element.classList.contains('media-home-hero') &&
      element.classList.contains('media-home-live-hero-active') &&
      element.dataset.homeZone === 'hero';
  }
  return false;
}

class FakeElement {
  constructor(tagName, classes) {
    this.tagName = String(tagName || '').toUpperCase();
    this.className = classes || '';
    this.classList = classList(classes);
    this.dataset = {};
    this.children = [];
    this.parentNode = null;
    this.scrollLeft = 0;
    this.id = '';
    this.textContent = '';
    this.captured = [];
    this.released = [];
  }

  appendChild(child) {
    child.parentNode = this;
    this.children.push(child);
    return child;
  }

  closest(selector) {
    const selectors = String(selector || '').split(',').map(value => value.trim());
    let current = this;
    while (current) {
      if (selectors.some(value => matches(current, value))) return current;
      current = current.parentNode;
    }
    return null;
  }

  setPointerCapture(pointerId) { this.captured.push(pointerId); }
  releasePointerCapture(pointerId) { this.released.push(pointerId); }
}

const head = new FakeElement('head');
const listeners = Object.create(null);
const document = {
  head,
  createElement(tagName) { return new FakeElement(tagName); },
  getElementById(id) {
    return head.children.find(child => child.id === id) || null;
  },
  addEventListener(type, listener, options) {
    (listeners[type] ||= []).push({listener, options});
  }
};

function event(target, values) {
  return Object.assign({
    target,
    pointerType: 'mouse',
    button: 0,
    pointerId: 1,
    clientX: 0,
    clientY: 0,
    defaultPrevented: false,
    propagationStopped: false,
    immediatePropagationStopped: false,
    preventDefault() { this.defaultPrevented = true; },
    stopPropagation() { this.propagationStopped = true; },
    stopImmediatePropagation() {
      this.immediatePropagationStopped = true;
      this.propagationStopped = true;
    }
  }, values || {});
}

function dispatch(type, target, values) {
  const current = event(target, values);
  (listeners[type] || []).forEach(entry => entry.listener(current));
  return current;
}

const heroOffsets = [];
let deferredLoadCount = 0;
const window = {
  document,
  console,
  loadVdrSuiteDeferredRuntime(key, url) {
    deferredLoadCount += 1;
    assert.strictEqual(key, 'vdr-suite-home-recording-discovery-runtime');
    assert.strictEqual(url, '/frontend/home-recording-discovery.js');
    return Promise.resolve(true);
  },
  VdrSuiteHomeLiveHero: {
    selectOffset(delta) {
      heroOffsets.push(delta);
      return true;
    }
  }
};
window.window = window;

const context = vm.createContext({window, document, console, Promise, Date, Object, Number, Math, String, Boolean, Array});
vm.runInContext(source, context, {filename: 'web/frontend/home-recording-discovery-bootstrap.js'});

assert(window.VdrSuiteHomeRecordingDiscoveryBootstrap);
// This gesture-only harness intentionally has no canonical backend selection.
assert.strictEqual(deferredLoadCount, 0);
assert(listeners.pointerdown && listeners.pointermove && listeners.pointerup && listeners.pointercancel && listeners.click);
assert.strictEqual(listeners.click[0].options, true);

const style = document.getElementById('vdr-suite-home-mouse-drag-style');
assert(style);
assert(style.textContent.includes('.media-home-discovery-rail::-webkit-scrollbar'));
assert(style.textContent.includes('.media-home-series-season-rail::-webkit-scrollbar'));
assert(style.textContent.includes('.media-home-live-guide-rail::-webkit-scrollbar'));
assert(style.textContent.includes('scrollbar-width:none'));

// A physical discovery rail follows the pointer continuously, like the EPG timeline.
const rail = new FakeElement('div', 'media-home-discovery-rail');
const card = rail.appendChild(new FakeElement('button', 'media-home-discovery-card'));
rail.scrollLeft = 120;
dispatch('pointerdown', card, {pointerId: 10, clientX: 220, clientY: 40});
const railMove = dispatch('pointermove', card, {pointerId: 10, clientX: 150, clientY: 43});
assert.strictEqual(rail.scrollLeft, 190);
assert.strictEqual(railMove.defaultPrevented, true);
assert.strictEqual(rail.classList.contains('media-home-mouse-dragging'), true);
assert.deepStrictEqual(rail.captured, [10]);
dispatch('pointerup', card, {pointerId: 10, clientX: 150, clientY: 43});
assert.strictEqual(rail.classList.contains('media-home-mouse-dragging'), false);
assert.deepStrictEqual(rail.released, [10]);

// The synthetic click browsers emit after a real drag must not open the card.
const draggedClick = dispatch('click', card);
assert.strictEqual(draggedClick.defaultPrevented, true);
assert.strictEqual(draggedClick.immediatePropagationStopped, true);

// A plain click remains a plain click; no drag means no suppression.
dispatch('pointerdown', card, {pointerId: 11, clientX: 150, clientY: 43});
dispatch('pointerup', card, {pointerId: 11, clientX: 150, clientY: 43});
const plainClick = dispatch('click', card);
assert.strictEqual(plainClick.defaultPrevented, false);
assert.strictEqual(plainClick.propagationStopped, false);

// Vertical-dominant mouse movement is abandoned instead of stealing page navigation.
const beforeVertical = rail.scrollLeft;
dispatch('pointerdown', card, {pointerId: 12, clientX: 180, clientY: 40});
dispatch('pointermove', card, {pointerId: 12, clientX: 176, clientY: 92});
dispatch('pointerup', card, {pointerId: 12, clientX: 176, clientY: 92});
assert.strictEqual(rail.scrollLeft, beforeVertical);

// Pointer-touch is deliberately ignored; existing touch/swipe remains the owner.
dispatch('pointerdown', card, {pointerType: 'touch', pointerId: 13, clientX: 200, clientY: 40});
dispatch('pointermove', card, {pointerType: 'touch', pointerId: 13, clientX: 120, clientY: 42});
dispatch('pointerup', card, {pointerType: 'touch', pointerId: 13, clientX: 120, clientY: 42});
assert.strictEqual(rail.scrollLeft, beforeVertical);

// The new Now/Next programme rails reuse the exact same delegated mouse-drag owner.
const liveGuideRail = new FakeElement('div', 'media-home-live-guide-rail');
const liveGuideCard = liveGuideRail.appendChild(new FakeElement('article', 'media-home-live-guide-card'));
liveGuideRail.scrollLeft = 80;
dispatch('pointerdown', liveGuideCard, {pointerId: 14, clientX: 210, clientY: 46});
const liveGuideMove = dispatch('pointermove', liveGuideCard, {pointerId: 14, clientX: 145, clientY: 48});
assert.strictEqual(liveGuideRail.scrollLeft, 145);
assert.strictEqual(liveGuideMove.defaultPrevented, true);
assert.strictEqual(liveGuideRail.classList.contains('media-home-mouse-dragging'), true);
assert.deepStrictEqual(liveGuideRail.captured, [14]);
dispatch('pointerup', liveGuideCard, {pointerId: 14, clientX: 145, clientY: 48});
assert.strictEqual(liveGuideRail.classList.contains('media-home-mouse-dragging'), false);
assert.deepStrictEqual(liveGuideRail.released, [14]);
assert.strictEqual(dispatch('click', liveGuideCard).defaultPrevented, true);

// The projected Live-TV Hero uses the same mouse gesture but delegates selection
// to its existing browse-only API instead of inventing a scroll/playback owner.
const hero = new FakeElement('section', 'media-home-hero media-home-live-hero-active');
hero.dataset.homeZone = 'hero';
const heroCard = hero.appendChild(new FakeElement('article', 'media-home-live-focus'));
dispatch('pointerdown', heroCard, {pointerId: 20, clientX: 260, clientY: 60});
dispatch('pointermove', heroCard, {pointerId: 20, clientX: 185, clientY: 63});
dispatch('pointerup', heroCard, {pointerId: 20, clientX: 185, clientY: 63});
assert.deepStrictEqual(heroOffsets, [1]);
assert.strictEqual(dispatch('click', heroCard).defaultPrevented, true);

dispatch('pointerdown', heroCard, {pointerId: 21, clientX: 130, clientY: 60});
dispatch('pointermove', heroCard, {pointerId: 21, clientX: 205, clientY: 62});
dispatch('pointerup', heroCard, {pointerId: 21, clientX: 205, clientY: 62});
assert.deepStrictEqual(heroOffsets, [1, -1]);

// A short horizontal grab suppresses accidental activation but does not switch channels.
dispatch('pointerdown', heroCard, {pointerId: 22, clientX: 200, clientY: 60});
dispatch('pointermove', heroCard, {pointerId: 22, clientX: 175, clientY: 61});
dispatch('pointerup', heroCard, {pointerId: 22, clientX: 175, clientY: 61});
assert.deepStrictEqual(heroOffsets, [1, -1]);
assert.strictEqual(dispatch('click', heroCard).defaultPrevented, true);

console.log('phase66 Home mouse drag rails production contract ok');
