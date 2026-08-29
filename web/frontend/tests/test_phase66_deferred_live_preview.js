'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontend = path.resolve(__dirname, '..');
const root = path.resolve(frontend, '..', '..');
const previewSource = fs.readFileSync(path.join(frontend, 'home-live-preview.js'), 'utf8');
const heroSource = fs.readFileSync(path.join(frontend, 'home-live-hero.js'), 'utf8');
const registrySource = fs.readFileSync(path.join(root, 'core/http/src/TestHttpServerPaths.inc'), 'utf8');

function requires(source, expression, message) {
  assert(expression.test(source), message);
}

// Architecture: Home owns only a deferred intent controller. MediaSession and
// stream lifecycle remain behind the already-authoritative playback facade.
requires(previewSource, /focusToken/, '66.3 rapid-focus race must be fenced by a focus token');
requires(previewSource, /previewSettleMs\s*=\s*\d+/, '66.3 preview startup must use a bounded private settle timer');
requires(previewSource, /cancelPendingPreview/, '66.3 superseded pending preview must be cancellable');
requires(previewSource, /VdrSuitePlaybackShell/, '66.3 must consult canonical shell ownership before preview startup');
requires(previewSource, /VdrSuiteRecordings2Playback/, '66.3 must use the existing canonical Live playback facade');
requires(previewSource, /ownerIntent\s*:\s*['"]preview['"]/, '66.3 preview intent must be explicit at the existing playback boundary');
assert(!previewSource.includes('/api/media/sessions'), '66.3 Home preview must not call MediaSession REST directly');
assert(!previewSource.includes('createLiveSession('), '66.3 Home preview must not own MediaSession creation');
assert(!previewSource.includes("createElement('video')"), '66.3 Home preview must not create a second media element');
assert(!previewSource.includes('video.src ='), '66.3 Home preview must not invent a provider/native stream path');
requires(heroSource, /VdrSuiteLiveTvView/, '66.3 must preserve the existing explicit Live-TV owner path');
requires(heroSource, /startChannel\(channel\)/, '66.3 Watch Live must remain an explicit startChannel intent');
requires(
  registrySource,
  /channel-day-program-compat\.js[^\n]+home-live-preview\.js/,
  '66.3 production composition must load preview after the canonical playback-shell runtime'
);
requires(previewSource, /@media\(min-width:46\.01rem\)/, '66.3 desktop preview must have a secondary responsive composition');
requires(previewSource, /@media\(max-width:46rem\)/, '66.3 mobile preview must be hero-integrated');
assert(!previewSource.includes('position:fixed'), '66.3 Home preview must not create a floating mini-player');

function makeHarness(options) {
  const settings = Object.assign({deferredStart: false, fullOwner: false, failStart: false}, options || {});
  const hero = {active: true, backendId: 'backend-a', selectedChannelId: '1'};
  const shell = {
    active: Boolean(settings.fullOwner),
    backendId: settings.fullOwner ? 'backend-a' : '',
    channelId: settings.fullOwner ? 'full-owner-channel' : '',
    channelName: settings.fullOwner ? 'Explicit Live' : '',
    sessionId: settings.fullOwner ? 'full-session' : '',
    miniVisible: false,
    lastStopReason: ''
  };
  const metrics = {creates: 0, starts: 0, destroys: 0};
  const timers = new Map();
  let nextTimerId = 1;
  let deferredResolve = null;
  let deferredReject = null;

  function setTimeoutFake(callback, delay) {
    const id = nextTimerId++;
    timers.set(id, {callback, delay: Number(delay) || 0});
    return id;
  }

  function clearTimeoutFake(id) {
    timers.delete(id);
  }

  function runTimers() {
    const pending = Array.from(timers.entries());
    pending.forEach(([id]) => timers.delete(id));
    pending.forEach(([, entry]) => entry.callback());
    return pending.length;
  }

  const document = {
    readyState: 'loading',
    addEventListener() {},
    querySelector() { return null; }
  };

  const context = {
    console,
    Promise,
    document,
    setTimeout: setTimeoutFake,
    clearTimeout: clearTimeoutFake,
    VdrSuiteHomeLiveHero: {
      snapshot() { return Object.assign({}, hero); }
    },
    VdrSuitePlaybackShell: {
      snapshot() { return Object.assign({}, shell); }
    },
    VdrSuiteRecordings2Playback: {
      createLivePanel(channel, backendId, panelOptions) {
        metrics.creates += 1;
        assert.strictEqual(panelOptions && panelOptions.ownerIntent, 'preview', 'preview adapter intent');
        assert.strictEqual(String(channel.channelId || channel.id), hero.selectedChannelId, 'preview must target current focused channel');
        shell.active = true;
        shell.backendId = backendId;
        shell.channelId = String(channel.channelId || channel.id);
        shell.channelName = String(channel.name || shell.channelId);
        shell.sessionId = '';

        let destroyed = false;
        return {
          element: {querySelector() { return null; }},
          start() {
            metrics.starts += 1;
            if (settings.failStart) return Promise.reject(new Error('preview failed'));
            if (settings.deferredStart && metrics.starts === 1) {
              return new Promise((resolve, reject) => {
                deferredResolve = resolve;
                deferredReject = reject;
              });
            }
            shell.sessionId = 'preview-session-' + metrics.starts;
            return Promise.resolve(shell.sessionId);
          },
          destroy() {
            if (destroyed) return;
            destroyed = true;
            metrics.destroys += 1;
            if (shell.channelId === String(channel.channelId || channel.id)) {
              shell.active = false;
              shell.backendId = '';
              shell.channelId = '';
              shell.channelName = '';
              shell.sessionId = '';
              shell.lastStopReason = 'preview-destroyed';
            }
          }
        };
      }
    }
  };
  context.window = context;
  vm.createContext(context);
  vm.runInContext(previewSource, context, {filename: 'home-live-preview.js'});

  return {
    api: context.VdrSuiteHomeLivePreview,
    hero,
    shell,
    metrics,
    timers,
    runTimers,
    resolveDeferred(value) {
      assert(deferredResolve, 'expected a deferred preview start');
      shell.sessionId = value || 'preview-session-deferred';
      deferredResolve(shell.sessionId);
    },
    rejectDeferred(error) {
      assert(deferredReject, 'expected a deferred preview start');
      deferredReject(error || new Error('deferred preview failed'));
    }
  };
}

async function flushPromises() {
  await Promise.resolve();
  await Promise.resolve();
  await Promise.resolve();
}

async function startSettled(harness) {
  harness.api.sync();
  assert.strictEqual(harness.metrics.creates, 0, 'settle period must not create a session immediately');
  assert.strictEqual(harness.api.snapshot().pending, true, 'settled focus must queue one preview intent');
  assert.strictEqual(harness.runTimers(), 1, 'exactly one settle timer should fire');
  await flushPromises();
}

(async () => {
  // Rapid focus movement: no MediaSession/player creation before the final
  // bounded settle timer. Only the last focused channel may start.
  {
    const h = makeHarness();
    h.api.sync();
    h.hero.selectedChannelId = '2';
    h.api.sync();
    h.hero.selectedChannelId = '3';
    h.api.sync();
    assert.strictEqual(h.metrics.creates, 0, 'rapid focus changes before settle must create ZERO previews');
    assert.strictEqual(h.timers.size, 1, 'rapid focus changes must replace the pending settle timer');
    h.runTimers();
    await flushPromises();
    assert.strictEqual(h.metrics.creates, 1, 'settled focus may create at most one preview');
    assert.strictEqual(h.metrics.starts, 1, 'settled focus may start at most one preview');
    assert.strictEqual(h.api.snapshot().channelId, '3', 'preview must match the latest focused channel');
  }

  // Active preview: browsing relinquishes immediately through the canonical
  // adapter and only then queues the next deferred intent.
  {
    const h = makeHarness();
    await startSettled(h);
    assert.strictEqual(h.api.snapshot().active, true, 'settled preview should become active');
    h.hero.selectedChannelId = '2';
    h.api.sync();
    assert.strictEqual(h.metrics.destroys, 1, 'active preview must relinquish on browse movement');
    assert.strictEqual(h.metrics.creates, 1, 'browse movement must not synchronously create replacement playback');
    assert.strictEqual(h.api.snapshot().pending, true, 'replacement preview must be deferred again');
  }

  // Stale in-flight preview: selection may move while MediaSession creation is
  // unresolved. The old request stays the sole shell owner until it resolves,
  // is destroyed, and only then may a new settled preview start.
  {
    const h = makeHarness({deferredStart: true});
    h.api.sync();
    h.runTimers();
    assert.strictEqual(h.metrics.creates, 1, 'first settled preview should create one adapter');
    assert.strictEqual(h.metrics.starts, 1, 'first settled preview should begin one asynchronous start');
    assert.strictEqual(h.api.snapshot().starting, true, 'first preview should be in-flight');

    h.hero.selectedChannelId = '2';
    h.api.sync();
    assert.strictEqual(h.metrics.destroys, 0, 'in-flight adapter must not be destroyed before its session result is known');
    assert.strictEqual(h.metrics.creates, 1, 'stale in-flight owner must fence a competing preview');
    assert.strictEqual(h.timers.size, 0, 'no second settle timer may race the unresolved canonical owner');

    h.resolveDeferred('preview-session-old');
    await flushPromises();
    assert.strictEqual(h.metrics.destroys, 1, 'stale in-flight preview must be destroyed once startup resolves');
    assert.strictEqual(h.api.snapshot().active, false, 'stale in-flight preview must never attach as current');
    assert.strictEqual(h.api.snapshot().pending, true, 'latest focus should be scheduled only after stale owner cleanup');

    h.runTimers();
    await flushPromises();
    assert.strictEqual(h.metrics.creates, 2, 'latest settled focus may start after stale cleanup');
    assert.strictEqual(h.api.snapshot().channelId, '2', 'replacement preview must target latest focus');
  }

  // Existing explicit full playback is foreign ownership and blocks preview
  // startup without blocking browse state changes.
  {
    const h = makeHarness({fullOwner: true});
    h.api.sync();
    h.runTimers();
    await flushPromises();
    assert.strictEqual(h.metrics.creates, 0, 'active explicit full playback must block Home preview startup');
    h.hero.selectedChannelId = '2';
    assert.doesNotThrow(() => h.api.sync(), 'browse must remain usable while full playback owns the shell');
    h.runTimers();
    await flushPromises();
    assert.strictEqual(h.metrics.creates, 0, 'Home preview must never displace an explicit full owner');
  }

  // Leaving Home and switching backend deterministically relinquish an active
  // preview. These are ownership boundaries, not DOM-presence heuristics.
  {
    const h = makeHarness();
    await startSettled(h);
    h.hero.active = false;
    h.api.sync();
    assert.strictEqual(h.metrics.destroys, 1, 'leaving Home must relinquish active preview');
  }
  {
    const h = makeHarness();
    await startSettled(h);
    h.hero.backendId = 'backend-b';
    h.api.sync();
    assert.strictEqual(h.metrics.destroys, 1, 'backend change must relinquish active preview');
    assert.strictEqual(h.api.snapshot().pending, true, 'backend change may queue only a newly settled preview');
  }

  // Watch Live promotion is intentionally different from browse replacement:
  // Home drops its preview intent without destroying the canonical same-channel
  // owner, allowing VdrSuiteLiveTvView.startChannel() to adopt the same proxy.
  {
    const h = makeHarness();
    await startSettled(h);
    assert.strictEqual(h.shell.active, true, 'preview should own canonical shell before promotion');
    h.api.__test.promotePreviewToFull();
    assert.strictEqual(h.api.snapshot().active, false, 'Home must cease preview ownership on explicit Watch Live');
    assert.strictEqual(h.metrics.destroys, 0, 'promotion must not tear down the same canonical playback owner');
    assert.strictEqual(h.shell.active, true, 'canonical shell owner must remain available for explicit adoption');
  }

  // Preview failure remains local evidence. It must neither throw into browsing
  // nor create fallback providers/profiles or seize another owner.
  {
    const h = makeHarness({failStart: true});
    await startSettled(h);
    assert.strictEqual(h.api.snapshot().active, false, 'failed preview must not become active');
    assert.strictEqual(h.metrics.destroys, 1, 'failed preview adapter must be cleaned up');
    h.hero.selectedChannelId = '2';
    assert.doesNotThrow(() => h.api.sync(), 'preview failure must not block further browsing');
  }

  console.log('phase66 deferred live preview ownership races: PASS');
})().catch(error => {
  console.error(error && error.stack ? error.stack : error);
  process.exitCode = 1;
});
