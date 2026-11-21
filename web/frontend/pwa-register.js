(function (global) {
  'use strict';

  // Transport state is not an authorization decision. Ordinary SSE reconnects
  // leave the online UI alone; a known outage discards transient previews.
  function createConnection(options) {
    let state = 'ready', generation = 0, pending = null;
    const blocked = () => state === 'offline' || state === 'unavailable' || state === 'checking' || state === 'recovering';
    function publish(next) {
      if (state !== next) state = next;
      options.render(state);
    }
    function unavailable() {
      if (state === 'checking' || state === 'recovering') return;
      if (!blocked()) { generation++; options.suspend(); }
      publish(options.online() ? 'unavailable' : 'offline');
    }
    function offline() {
      generation++;
      pending = null;
      options.suspend();
      publish('offline');
    }
    function retry() {
      if (!blocked() || state === 'recovering') return Promise.resolve(false);
      if (!options.online()) { offline(); return Promise.resolve(false); }
      if (pending) return pending;
      const attempt = generation;
      publish('checking');
      const work = Promise.resolve().then(() => options.verify()).then(verified => {
        if (attempt !== generation) return false;
        if (!verified || !options.online()) {
          publish(options.online() ? 'unavailable' : 'offline');
          return false;
        }
        // A genuine outage may span a changed account/grants or deployment.
        // Re-enter the normal, authoritative bootstrap once, not on SSE churn.
        publish('recovering');
        options.navigate();
        return true;
      }).catch(() => {
        if (attempt === generation) publish(options.online() ? 'unavailable' : 'offline');
        return false;
      }).finally(() => { if (pending === work) pending = null; });
      pending = work;
      return work;
    }
    function live(type) {
      if (type === 'error') {
        if (!options.online()) { if (state !== 'offline') offline(); }
        else if (!blocked()) publish('reconnecting');
      } else if (type === 'open') {
        if (blocked()) { if (options.online()) retry(); }
        else publish('ready');
      }
    }
    if (!options.online()) offline();
    else publish('ready');
    return Object.freeze({offline, unavailable, retry, live, state: () => state});
  }

  function mountConnection() {
    const doc = global.document;
    if (!doc || !doc.body || typeof global.addEventListener !== 'function') return;
    const panel = doc.getElementById('vdr-suite-connection-status');
    if (!panel) return;
    const message = doc.getElementById('vdr-suite-connection-message');
    const button = doc.getElementById('vdr-suite-connection-retry');
    if (!message || !button) return;
    const copy = {
      reconnecting: ['Live-Verbindung unterbrochen. Wiederverbindung läuft …', 'Live connection interrupted. Reconnecting …'],
      offline: ['Offline – keine Serververbindung. Geschützte Inhalte benötigen eine aktuelle Verbindung.', 'Offline – no server connection. Protected content requires a current connection.'],
      unavailable: ['Server nicht erreichbar oder Sitzung nicht überprüfbar. Bitte erneut verbinden.', 'Server unavailable or session could not be verified. Please reconnect.'],
      checking: ['Serververbindung und Sitzung werden überprüft …', 'Checking server connection and session …'],
      recovering: ['Verbindung bestätigt. VDR-Suite wird neu verbunden …', 'Connection verified. Reconnecting VDR-Suite …']
    };
    const english = () => String(doc.documentElement.lang || '').toLowerCase().startsWith('en');
    const connection = createConnection({
      online: () => !global.navigator || global.navigator.onLine !== false,
      suspend: () => {
        const cache = global.VdrSuiteHomeCache;
        if (cache && typeof cache.suspend === 'function') cache.suspend();
      },
      verify: () => {
        const session = global.VdrSuiteBrowserSession;
        return session && typeof session.verifyConnection === 'function' ? session.verifyConnection() : false;
      },
      navigate: () => global.location.reload(),
      render: state => {
        panel.hidden = state === 'ready';
        doc.body.classList.toggle('vss-connection-unavailable', state !== 'ready' && state !== 'reconnecting');
        message.textContent = copy[state] ? copy[state][english() ? 1 : 0] : '';
        button.textContent = english() ? 'Reconnect' : 'Erneut verbinden';
        button.hidden = state === 'ready' || state === 'reconnecting' || state === 'checking' || state === 'recovering';
      }
    });
    global.VdrSuiteConnection = connection;
    global.addEventListener('offline', connection.offline);
    global.addEventListener('online', connection.retry);
    button.addEventListener('click', connection.retry);
  }

  // Optional browser-native installation. The browser alone decides whether
  // the app is installable; standalone mode never changes auth or API access.
  function mountInstall() {
    const doc = global.document;
    if (!doc || !doc.body || typeof global.addEventListener !== 'function') return;
    const row = doc.getElementById('vdr-suite-pwa-install');
    const button = doc.getElementById('vdr-suite-pwa-install-button');
    if (!row || !button) return;
    let display = null;
    try {
      if (typeof global.matchMedia === 'function') display = global.matchMedia('(display-mode: standalone)');
    } catch (error) { /* Browser without media-query support. */ }
    let offer = null, busy = false, installed = false;
    const standalone = () => Boolean((display && display.matches) ||
      (global.navigator && global.navigator.standalone === true));
    function render() {
      const appMode = standalone();
      doc.body.classList.toggle('vss-pwa-standalone', appMode);
      if (appMode || installed) offer = null;
      row.hidden = !global.isSecureContext || appMode || installed || busy || !offer;
    }
    function receive(event) {
      if (!global.isSecureContext || standalone() || installed ||
          !event || typeof event.prompt !== 'function') return;
      if (typeof event.preventDefault === 'function') event.preventDefault();
      offer = event;
      render();
    }
    async function install() {
      if (!offer || busy || installed || standalone() || !global.isSecureContext) return;
      const selected = offer;
      offer = null;
      busy = true;
      render();
      try {
        // Invoke directly within the actual click's user activation.
        // A consumed offer is never retried automatically, even on dismissal.
        const choice = await selected.prompt();
        if ((!choice || !choice.outcome) && selected.userChoice) await selected.userChoice;
      } catch (error) { /* Browser declined/unavailable; ordinary UI remains. */ }
      finally {
        busy = false;
        render();
      }
    }
    global.addEventListener('beforeinstallprompt', receive);
    global.addEventListener('appinstalled', () => {
      installed = true;
      offer = null;
      render();
    });
    if (display) {
      if (typeof display.addEventListener === 'function') display.addEventListener('change', render);
      else if (typeof display.addListener === 'function') display.addListener(render);
    }
    button.addEventListener('click', install);
    render();
  }

  global.VdrSuiteConnectionRuntime = Object.freeze({create: createConnection});
  mountConnection();
  mountInstall();

  // Optional enhancement, also in an ordinary browser without SW support.
  if (!global.isSecureContext || !global.navigator || !global.navigator.serviceWorker || !global.VdrSuitePublicUrl) return;
  try {
    const worker = global.VdrSuitePublicUrl.resolvePath('/frontend/service-worker.js');
    const scope = global.VdrSuitePublicUrl.resolvePath('/frontend/');
    global.navigator.serviceWorker.register(worker, { scope, updateViaCache: 'none' })
      .catch(function () { /* Normal browser operation remains available. */ });
  } catch (error) { /* Unsupported or restricted browser. */ }
  if (typeof global.addEventListener === 'function') {
    global.addEventListener('offline', function () {
      const controller = global.navigator.serviceWorker.controller;
      if (controller) {
        try { controller.postMessage({type: 'vdr-suite-artwork-proof-reset'}); } catch (error) {}
      }
    });
  }
  function bindSession() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.subscribe !== 'function') return;
    let previouslyAuthenticated = false;
    session.subscribe(function (state) {
      const authenticated = Boolean(state && state.authenticated);
      if (previouslyAuthenticated && !authenticated) {
        const controller = global.navigator.serviceWorker.controller;
        if (controller) {
          try { controller.postMessage({type: 'vdr-suite-artwork-clear'}); } catch (error) {}
        }
      }
      previouslyAuthenticated = authenticated;
    });
  }
  if (global.document && global.document.readyState === 'loading') {
    global.document.addEventListener('DOMContentLoaded', bindSession, {once: true});
  } else bindSession();
}(window));
