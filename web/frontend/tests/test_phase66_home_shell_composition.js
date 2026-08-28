'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const indexSource = fs.readFileSync(path.join(frontendRoot, 'index.html'), 'utf8');
const appSource = fs.readFileSync(path.join(frontendRoot, 'app.js'), 'utf8');

function occurrences(source, needle) {
  return source.split(needle).length - 1;
}

// Slice 1 must extend the installed browser composition root, not create a
// parallel Home application or a second navigation/state owner.
assert(indexSource.includes('<body class="media-app-shell">'));
assert.strictEqual(occurrences(indexSource, 'id="module-nav"'), 1);
assert.strictEqual(occurrences(indexSource, 'id="detail-data"'), 1);
assert(indexSource.includes('data-brand-module="overview"'));
assert(indexSource.includes('data-brand-module="channels2"'));
assert(indexSource.includes('data-brand-module="recordings2"'));
assert(indexSource.includes('data-brand-module="search"'));
assert(indexSource.includes('data-brand-module="settings"'));
assert(indexSource.includes('class="module-tab active" data-module="overview"'));

// The semantic Home zones are static composition only. Later Phase-66 media
// semantics must not be fabricated in Slice 1.
assert(indexSource.includes('data-home-zone="hero"'));
assert(indexSource.includes('data-home-zone="primary-rail"'));
assert(indexSource.includes('data-home-zone="additional-sections"'));
assert(indexSource.includes('class="media-home-only media-home-hero"'));
assert(indexSource.includes('class="media-home-only media-home-additional-sections"'));
assert(!indexSource.includes('<video'));
assert(!indexSource.includes('<audio'));
assert(!indexSource.includes('continue-watching'));
assert(!indexSource.includes('MediaSession'));
assert(!indexSource.includes('startPositionSeconds'));

// Production app.js remains the sole view/navigation state owner.
assert(appSource.includes("let selectedModule = 'overview';"));
assert(appSource.includes('function selectModule(moduleName)'));
assert(appSource.includes("document.querySelectorAll('.module-tab').forEach(button =>"));
assert(appSource.includes("button.addEventListener('click', () => selectModule(button.dataset.module));"));
assert(appSource.includes("document.querySelectorAll('[data-brand-module]').forEach(button =>"));
assert(appSource.includes('selectModule(moduleName);'));
assert(appSource.includes("button.classList.toggle('active', button.dataset.module === moduleName);"));
assert(appSource.includes("if (selectedModule === 'overview')"));
assert(appSource.includes('renderSnapshotMetrics(data);'));
assert(appSource.includes("createMetric('Kanäle', valueOrZero(data.channelCount))"));
assert(appSource.includes("createMetric('Aufnahmen', valueOrZero(data.recordingCount))"));

// Home composition is driven only by that existing UI state. The primary
// content shell reuses real snapshot data as a horizontal rail rather than
// inventing Continue Watching, preview or demo media data.
assert(indexSource.includes('#detail:has(.module-tab.active[data-module="overview"])'));
assert(indexSource.includes('#detail:has(.module-tab.active[data-module="overview"]) #detail-data'));
assert(indexSource.includes('overflow-x: auto;'));
assert(indexSource.includes('scroll-snap-type: x proximity;'));

// Responsive recomposition: distinct desktop/tablet/mobile/landscape geometry,
// touch-sized controls, no page-wide horizontal scrolling, and reduced motion.
assert(indexSource.includes('@media (min-width: 72.01rem)'));
assert(indexSource.includes('@media (min-width: 46.01rem) and (max-width: 72rem)'));
assert(indexSource.includes('@media (max-width: 46rem)'));
assert(indexSource.includes('@media (max-height: 34rem) and (min-width: 40rem) and (max-width: 64rem)'));
assert(indexSource.includes('@media (prefers-reduced-motion: reduce)'));
assert(indexSource.includes('overflow-x: hidden;'));
assert(indexSource.includes('min-height: 2.75rem;'));
assert(indexSource.includes('min-width: min(78vw, 19rem);'));

// Composition-root regression: build the navigation nodes from the production
// index markup, then execute the exact selectModule() and event-wiring source
// from production app.js. This proves a user-style click/keyboard action reaches
// the canonical production navigation owner and mutates the real module state.
function classList(initial) {
  const values = new Set(String(initial || '').split(/\s+/).filter(Boolean));
  return {
    contains(name) { return values.has(name); },
    add(name) { values.add(name); },
    remove(name) { values.delete(name); },
    toggle(name, enabled) {
      if (enabled === undefined) {
        enabled = !values.has(name);
      }
      if (enabled) values.add(name); else values.delete(name);
      return enabled;
    }
  };
}

function productionNodes(source, tag, attribute) {
  const expression = new RegExp(`<${tag}[^>]*class="([^"]*)"[^>]*${attribute}="([^"]+)"[^>]*>`, 'g');
  const nodes = [];
  let match;
  while ((match = expression.exec(source)) !== null) {
    const listeners = Object.create(null);
    nodes.push({
      dataset: {[attribute.replace('data-', '').replace(/-([a-z])/g, (_, c) => c.toUpperCase())]: match[2]},
      classList: classList(match[1]),
      addEventListener(type, listener) {
        (listeners[type] ||= []).push(listener);
      },
      dispatch(type, event = {}) {
        (listeners[type] || []).forEach(listener => listener(Object.assign({preventDefault() {}}, event)));
      }
    });
  }
  return nodes;
}

const moduleTabs = productionNodes(indexSource, 'button', 'data-module');
const brandButtons = productionNodes(indexSource, 'article', 'data-brand-module');
assert(moduleTabs.length >= 8);
assert(brandButtons.length >= 5);

const selectStart = appSource.indexOf('function selectModule(moduleName)');
const selectEnd = appSource.indexOf('\nfunction markSelected(', selectStart);
const bindingStart = appSource.indexOf("document.querySelectorAll('.module-tab').forEach(button =>", selectEnd);
const bindingEnd = appSource.indexOf('\nrefreshDetailButton.addEventListener(', bindingStart);
assert(selectStart >= 0 && selectEnd > selectStart);
assert(bindingStart >= 0 && bindingEnd > bindingStart);

const detailDataElement = {scrollIntoView() { this.scrolled = true; }};
const document = {
  querySelectorAll(selector) {
    if (selector === '.module-tab') return moduleTabs;
    if (selector === '[data-brand-module]') return brandButtons;
    return [];
  }
};
const context = {
  window: {VdrSuiteChannels2: null},
  document,
  detailDataElement,
  currentSnapshot: null,
  selectedModule: 'overview',
  renderSelectedModule() {}
};
vm.createContext(context);
vm.runInContext(
  `var selectedModule = 'overview';\n${appSource.slice(selectStart, selectEnd)}\n${appSource.slice(bindingStart, bindingEnd)}`,
  context
);

function moduleTab(name) {
  return moduleTabs.find(node => node.dataset.module === name);
}
function brandButton(name) {
  return brandButtons.find(node => node.dataset.brandModule === name);
}

brandButton('recordings2').dispatch('click');
assert(moduleTab('recordings2').classList.contains('active'));
assert(!moduleTab('overview').classList.contains('active'));
assert.strictEqual(detailDataElement.scrolled, true);

brandButton('overview').dispatch('keydown', {key: 'Enter'});
assert(moduleTab('overview').classList.contains('active'));
assert(!moduleTab('recordings2').classList.contains('active'));

brandButton('settings').dispatch('keydown', {key: ' '});
assert(!moduleTab('overview').classList.contains('active'));
assert(moduleTabs.every(node => !node.classList.contains('active')));

console.log('phase66 home shell production composition ok');
