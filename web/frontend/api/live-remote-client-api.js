(function (global) {
  'use strict';

  const LOGIN_PATH = '/api/security/browser-sessions';
  const LOGOUT_PATH = '/api/security/browser-sessions/logout';
  const CSRF_HEADER = 'X-CSRF-Token';
  const SECURITY_REASONS = Object.freeze({
    authentication_required: true,
    invalid_credentials: true,
    credential_revoked: true,
    credential_expired: true,
    session_revoked: true,
    session_expired: true,
    csrf_validation_failed: true
  });

  let csrfToken = '';
  let expiresAt = '';
  let authenticated = false;
  let lastReason = '';
  let expiryTimer = null;
  const listeners = [];

  function languageIsEnglish() {
    const document = global.document;
    return Boolean(
      document &&
      document.documentElement &&
      String(document.documentElement.lang || '').toLowerCase().startsWith('en')
    );
  }

  function translated(german, english) {
    return languageIsEnglish() ? english : german;
  }

  function securityMessage(reason) {
    const messages = {
      authentication_required: translated(
        'Bitte anmelden, um diese Aktion auszuführen.',
        'Please sign in to perform this action.'
      ),
      invalid_credentials: translated(
        'Die Anmeldung ist nicht mehr gültig. Bitte erneut anmelden.',
        'The sign-in is no longer valid. Please sign in again.'
      ),
      credential_revoked: translated(
        'Die Anmeldung wurde widerrufen. Bitte erneut anmelden.',
        'The sign-in was revoked. Please sign in again.'
      ),
      credential_expired: translated(
        'Die Anmeldung ist abgelaufen. Bitte erneut anmelden.',
        'The sign-in expired. Please sign in again.'
      ),
      session_revoked: translated(
        'Die Browser-Sitzung wurde widerrufen. Bitte erneut anmelden.',
        'The browser session was revoked. Please sign in again.'
      ),
      session_expired: translated(
        'Die Browser-Sitzung ist abgelaufen. Bitte erneut anmelden.',
        'The browser session expired. Please sign in again.'
      ),
      csrf_validation_failed: translated(
        'Die Sicherheitsbestätigung ist nicht mehr gültig. Bitte erneut anmelden.',
        'The security confirmation is no longer valid. Please sign in again.'
      )
    };

    return messages[reason] || '';
  }

  function snapshot() {
    return Object.freeze({
      authenticated: authenticated,
      expiresAt: expiresAt,
      reason: lastReason
    });
  }

  function notify() {
    const state = snapshot();
    listeners.slice().forEach(function (listener) {
      try {
        listener(state);
      } catch (error) {
        if (global.console && typeof global.console.error === 'function') {
          global.console.error(error);
        }
      }
    });
  }

  function cancelExpiryTimer() {
    if (expiryTimer !== null && typeof global.clearTimeout === 'function') {
      global.clearTimeout(expiryTimer);
    }
    expiryTimer = null;
  }

  function clear(reason) {
    cancelExpiryTimer();
    csrfToken = '';
    expiresAt = '';
    authenticated = false;
    lastReason = reason || '';
    notify();
  }

  function scheduleExpiry() {
    cancelExpiryTimer();

    if (!authenticated || !expiresAt || typeof global.setTimeout !== 'function') {
      return;
    }

    const expiryEpoch = Date.parse(expiresAt);
    if (!Number.isFinite(expiryEpoch)) {
      return;
    }

    const delay = expiryEpoch - Date.now();
    if (delay <= 0) {
      clear('session_expired');
      return;
    }

    expiryTimer = global.setTimeout(function () {
      clear('session_expired');
    }, Math.min(delay, 2147483647));
  }

  function subscribe(listener) {
    if (typeof listener !== 'function') {
      return function () {};
    }

    listeners.push(listener);
    listener(snapshot());

    return function () {
      const index = listeners.indexOf(listener);
      if (index >= 0) {
        listeners.splice(index, 1);
      }
    };
  }

  function utf8Base64(value) {
    if (typeof global.TextEncoder !== 'function' || typeof global.btoa !== 'function') {
      throw new Error('UTF-8 Basic authentication encoding is unavailable');
    }

    const bytes = new global.TextEncoder().encode(value);
    let binary = '';
    const chunkSize = 8192;

    for (let offset = 0; offset < bytes.length; offset += chunkSize) {
      const chunk = bytes.subarray(offset, offset + chunkSize);
      binary += String.fromCharCode.apply(null, Array.from(chunk));
    }

    return global.btoa(binary);
  }

  function parseResponse(response) {
    return response.text().then(function (text) {
      if (!text) {
        return null;
      }

      try {
        return JSON.parse(text);
      } catch (error) {
        return null;
      }
    });
  }

  function errorDetails(payload, fallback) {
    if (payload && payload.error && typeof payload.error === 'object') {
      return {
        code: String(payload.error.code || ''),
        message: String(payload.error.message || fallback)
      };
    }

    if (payload && typeof payload === 'object') {
      return {
        code: String(payload.code || ''),
        message: String(payload.message || fallback)
      };
    }

    return {code: '', message: fallback};
  }

  function responseError(response, payload, fallback) {
    const details = errorDetails(payload, fallback);
    const error = new Error(details.message);
    error.status = response.status;
    error.code = details.code;
    error.payload = payload;
    return error;
  }

  function securityReason(payload) {
    const details = errorDetails(payload, '');
    return SECURITY_REASONS[details.code] ? details.code : '';
  }

  function inspectSecurityResponse(response) {
    if (!response || (response.status !== 401 && response.status !== 403) ||
        typeof response.clone !== 'function') {
      return Promise.resolve(response);
    }

    let inspectionResponse;
    try {
      inspectionResponse = response.clone();
    } catch (error) {
      return Promise.resolve(response);
    }

    return parseResponse(inspectionResponse).then(function (payload) {
      const reason = securityReason(payload);
      if (reason) {
        clear(reason);
      }
      return response;
    }, function () {
      return response;
    });
  }

  function installFetchObserver() {
    if (typeof global.fetch !== 'function' || global.fetch.__vdrSuiteSessionObserved) {
      return;
    }

    const nativeFetch = global.fetch.bind(global);
    const observedFetch = function (input, init) {
      return nativeFetch(input, init).then(inspectSecurityResponse);
    };
    observedFetch.__vdrSuiteSessionObserved = true;
    global.fetch = observedFetch;
  }

  function login(username, password) {
    const normalizedUsername = String(username || '').trim();
    let submittedPassword = String(password || '');

    if (!normalizedUsername || !submittedPassword ||
        normalizedUsername.includes(':') ||
        normalizedUsername.length > 128 ||
        submittedPassword.length > 1024) {
      submittedPassword = '';
      return Promise.reject(new Error(translated(
        'Benutzername und Passwort sind erforderlich.',
        'Username and password are required.'
      )));
    }

    let authorization;
    try {
      authorization = 'Basic ' + utf8Base64(
        normalizedUsername + ':' + submittedPassword
      );
    } finally {
      submittedPassword = '';
    }

    return global.fetch(LOGIN_PATH, {
      method: 'POST',
      headers: {
        Accept: 'application/json',
        Authorization: authorization
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function (response) {
      authorization = '';
      return parseResponse(response).then(function (payload) {
        if (!response.ok) {
          throw responseError(
            response,
            payload,
            translated('Anmeldung fehlgeschlagen.', 'Sign-in failed.')
          );
        }

        const nextCsrfToken = payload && String(payload.csrfToken || '');
        const nextExpiresAt = payload && String(payload.expiresAt || '');

        if (nextCsrfToken.length < 32 || nextCsrfToken.length > 256 || !nextExpiresAt) {
          throw new Error(translated(
            'Die Anmeldung lieferte keine gültige Browser-Sitzung.',
            'The sign-in did not return a valid browser session.'
          ));
        }

        csrfToken = nextCsrfToken;
        expiresAt = nextExpiresAt;
        authenticated = true;
        lastReason = '';
        scheduleExpiry();
        notify();
        return snapshot();
      });
    }, function (error) {
      authorization = '';
      throw error;
    });
  }

  function logout() {
    if (!authenticated || !csrfToken) {
      clear('logout');
      return Promise.resolve(null);
    }

    const submittedCsrf = csrfToken;
    return global.fetch(LOGOUT_PATH, {
      method: 'POST',
      headers: {
        Accept: 'application/json',
        [CSRF_HEADER]: submittedCsrf
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function (response) {
      return parseResponse(response).then(function (payload) {
        if (!response.ok) {
          throw responseError(
            response,
            payload,
            translated('Abmeldung fehlgeschlagen.', 'Sign-out failed.')
          );
        }

        clear('logout');
        return null;
      });
    });
  }

  function csrfHeaders() {
    if (!authenticated || !csrfToken) {
      return {};
    }

    return {[CSRF_HEADER]: csrfToken};
  }

  function isAuthenticated() {
    return authenticated;
  }

  function lastSecurityMessage() {
    return securityMessage(lastReason);
  }

  function createElement(tag, className, text) {
    const element = global.document.createElement(tag);
    if (className) {
      element.className = className;
    }
    if (text !== undefined) {
      element.textContent = text;
    }
    return element;
  }

  function installStyles() {
    if (!global.document || global.document.getElementById('vdr-suite-session-style')) {
      return;
    }

    const style = createElement('style');
    style.id = 'vdr-suite-session-style';
    style.textContent =
      '.vss-controls{display:flex;align-items:center;gap:.55rem;margin-left:auto}' +
      '.vss-state{color:#94a3b8;font-size:.72rem;white-space:nowrap}' +
      '.vss-button,.vss-submit,.vss-cancel{min-height:2.45rem;padding:.48rem .85rem;border:1px solid rgba(96,165,250,.42);border-radius:999px;background:rgba(15,23,42,.82);color:#e0f2fe;font:inherit;font-size:.78rem;font-weight:800;cursor:pointer}' +
      '.vss-button.authenticated{border-color:rgba(52,211,153,.5);color:#a7f3d0}' +
      '.vss-dialog{width:min(26rem,calc(100vw - 1.5rem));padding:0;border:0;border-radius:1rem;background:#07111f;color:#e5edf9;box-shadow:0 1.4rem 3.6rem rgba(0,0,0,.65)}' +
      '.vss-dialog::backdrop{background:rgba(2,6,23,.82);backdrop-filter:blur(4px)}' +
      '.vss-panel{display:grid;gap:.85rem;padding:1.1rem}' +
      '.vss-panel h2{margin:0;color:#dbeafe;font-size:1.12rem}' +
      '.vss-panel label{display:grid;gap:.32rem;color:#bfdbfe;font-size:.8rem;font-weight:700}' +
      '.vss-panel input{box-sizing:border-box;width:100%;min-height:2.7rem;padding:.55rem .7rem;border:1px solid #334155;border-radius:.7rem;background:#020617;color:#f8fafc;font:inherit}' +
      '.vss-message{min-height:1.25rem;margin:0;color:#cbd5e1;font-size:.78rem;line-height:1.4}' +
      '.vss-message.error{color:#fca5a5}' +
      '.vss-actions{display:flex;justify-content:flex-end;gap:.55rem}' +
      '.vss-submit{background:linear-gradient(145deg,#0c4a6e,#1e3a8a)}' +
      '.vss-cancel{border-color:#475569;color:#cbd5e1}' +
      '@media(max-width:760px){.vss-state{display:none}.vss-button{min-height:2.25rem;padding:.4rem .7rem}.app-header{flex-direction:row;align-items:center}.app-header-text{display:none}}';
    global.document.head.appendChild(style);
  }

  function mountUi() {
    if (!global.document || typeof global.document.querySelector !== 'function') {
      return;
    }

    const header = global.document.querySelector('.app-header');
    if (!header || global.document.getElementById('vdr-suite-session-button')) {
      return;
    }

    installStyles();

    const controls = createElement('div', 'vss-controls');
    const state = createElement('span', 'vss-state');
    state.setAttribute('aria-live', 'polite');
    const button = createElement('button', 'vss-button');
    button.id = 'vdr-suite-session-button';
    button.type = 'button';
    button.setAttribute('aria-haspopup', 'dialog');
    controls.append(state, button);
    header.appendChild(controls);

    const dialog = createElement('dialog', 'vss-dialog');
    dialog.id = 'vdr-suite-session-dialog';
    dialog.setAttribute('aria-labelledby', 'vdr-suite-session-title');

    const form = createElement('form', 'vss-panel');
    form.noValidate = true;
    const title = createElement(
      'h2',
      '',
      translated('Bei VDR-Suite anmelden', 'Sign in to VDR-Suite')
    );
    title.id = 'vdr-suite-session-title';

    const usernameLabel = createElement(
      'label',
      '',
      translated('Benutzername', 'Username')
    );
    const username = createElement('input');
    username.type = 'text';
    username.name = 'username';
    username.autocomplete = 'username';
    username.required = true;
    username.maxLength = 128;
    usernameLabel.appendChild(username);

    const passwordLabel = createElement(
      'label',
      '',
      translated('Passwort', 'Password')
    );
    const password = createElement('input');
    password.type = 'password';
    password.name = 'password';
    password.autocomplete = 'current-password';
    password.required = true;
    password.maxLength = 1024;
    passwordLabel.appendChild(password);

    const message = createElement('p', 'vss-message');
    message.setAttribute('aria-live', 'assertive');

    const actions = createElement('div', 'vss-actions');
    const cancel = createElement(
      'button',
      'vss-cancel',
      translated('Abbrechen', 'Cancel')
    );
    cancel.type = 'button';
    const submit = createElement(
      'button',
      'vss-submit',
      translated('Anmelden', 'Sign in')
    );
    submit.type = 'submit';
    actions.append(cancel, submit);

    form.append(title, usernameLabel, passwordLabel, message, actions);
    dialog.appendChild(form);
    global.document.body.appendChild(dialog);

    function render(sessionState) {
      const active = Boolean(sessionState && sessionState.authenticated);
      button.classList.toggle('authenticated', active);
      button.textContent = active
        ? translated('Abmelden', 'Sign out')
        : translated('Anmelden', 'Sign in');
      state.textContent = active
        ? translated('Angemeldet', 'Signed in')
        : (securityMessage(sessionState && sessionState.reason) ||
          translated('Nicht angemeldet', 'Not signed in'));
    }

    subscribe(render);

    cancel.addEventListener('click', function () {
      password.value = '';
      message.textContent = '';
      message.classList.remove('error');
      if (typeof dialog.close === 'function') {
        dialog.close();
      } else {
        dialog.removeAttribute('open');
      }
    });

    button.addEventListener('click', function () {
      if (isAuthenticated()) {
        button.disabled = true;
        logout().catch(function (error) {
          state.textContent = String(error && error.message || error);
        }).finally(function () {
          button.disabled = false;
        });
        return;
      }

      message.textContent = securityMessage(lastReason);
      message.classList.toggle('error', Boolean(message.textContent));
      if (typeof dialog.showModal === 'function') {
        dialog.showModal();
      } else {
        dialog.setAttribute('open', '');
      }
      username.focus();
    });

    form.addEventListener('submit', function (event) {
      event.preventDefault();
      submit.disabled = true;
      cancel.disabled = true;
      message.classList.remove('error');
      message.textContent = translated('Anmeldung läuft …', 'Signing in …');

      const loginRequest = login(username.value, password.value);
      password.value = '';

      loginRequest.then(function () {
        username.value = '';
        message.textContent = '';
        if (typeof dialog.close === 'function') {
          dialog.close();
        } else {
          dialog.removeAttribute('open');
        }
      }).catch(function (error) {
        password.value = '';
        message.classList.add('error');
        message.textContent = String(error && error.message || error);
        password.focus();
      }).finally(function () {
        submit.disabled = false;
        cancel.disabled = false;
      });
    });
  }

  installFetchObserver();

  const sessionApi = Object.freeze({
    login: login,
    logout: logout,
    clear: clear,
    subscribe: subscribe,
    snapshot: snapshot,
    isAuthenticated: isAuthenticated,
    csrfHeaders: csrfHeaders,
    lastSecurityMessage: lastSecurityMessage
  });

  Object.defineProperty(global, 'VdrSuiteBrowserSession', {
    configurable: false,
    enumerable: true,
    writable: false,
    value: sessionApi
  });

  if (global.document) {
    if (global.document.readyState === 'loading' &&
        typeof global.document.addEventListener === 'function') {
      global.document.addEventListener('DOMContentLoaded', mountUi, {once: true});
    } else {
      mountUi();
    }
  }

  if (typeof global.addEventListener === 'function') {
    global.addEventListener('pagehide', function () {
      clear('authentication_required');
    });
  }

  const base = global.VdrSuiteClientApi;
  if (!base || typeof base.requestJson !== 'function') {
    return;
  }

  function normalizeOptions(options) {
    return options && typeof options === 'object' ? options : {};
  }

  function fetchClientRemoteAction(options) {
    const value = normalizeOptions(options);
    const payload = value.payload !== undefined ? value.payload : value.body;

    if (!sessionApi.isAuthenticated()) {
      return Promise.reject(new Error(
        sessionApi.lastSecurityMessage() || translated(
          'Bitte anmelden, um die Fernbedienung zu verwenden.',
          'Please sign in to use the remote control.'
        )
      ));
    }

    const headers = Object.assign(
      {'Content-Type': 'application/json'},
      value.headers || {},
      sessionApi.csrfHeaders()
    );

    return base.requestJson('/api/vdr/remote/actions', {
      method: 'POST',
      headers: headers,
      body: payload && typeof payload === 'object' ? JSON.stringify(payload) : payload,
      cache: value.cache || 'no-store',
      credentials: value.credentials || 'same-origin'
    }).catch(function (error) {
      const message = sessionApi.lastSecurityMessage();
      if (message) {
        const securityError = new Error(message);
        securityError.cause = error;
        throw securityError;
      }
      throw error;
    });
  }

  function fetchClientLiveOverlay(options) {
    const value = normalizeOptions(options);
    return base.requestJson('/api/vdr/live/overlay', {
      query: {backend: value.backendId || 'default', _: String(Date.now())},
      cache: value.cache || 'no-store',
      credentials: value.credentials || 'same-origin'
    });
  }

  function createClientLiveUpdateSource() {
    return typeof global.EventSource === 'function'
      ? new global.EventSource('/api/vdr/live', {withCredentials: true})
      : null;
  }

  global.VdrSuiteClientApi = Object.freeze(Object.assign({}, base, {
    fetchClientRemoteAction: fetchClientRemoteAction,
    fetchClientLiveOverlay: fetchClientLiveOverlay,
    createClientLiveUpdateSource: createClientLiveUpdateSource
  }));
}(window));
