(function(global) {
  'use strict';

  const STORAGE_KEY = 'vdr-suite.frontend.locale';
  const FALLBACK_LOCALE = 'de';
  const catalogs = global.VdrSuiteLocaleCatalogs || Object.create(null);

  function normalizeLocale(value) {
    const candidate = String(value || '')
      .trim()
      .toLowerCase()
      .replace(/_/g, '-');

    if (candidate === '') {
      return '';
    }

    const base = candidate.split('-')[0];
    return Object.prototype.hasOwnProperty.call(catalogs, base)
      ? base
      : '';
  }

  function storedLocale() {
    try {
      return normalizeLocale(global.localStorage.getItem(STORAGE_KEY));
    } catch (error) {
      return '';
    }
  }

  function documentLocale() {
    if (!global.document || !global.document.documentElement) {
      return '';
    }

    return normalizeLocale(global.document.documentElement.lang);
  }

  let activeLocale = storedLocale() || documentLocale() || FALLBACK_LOCALE;

  function messageFor(locale, key) {
    const catalog = catalogs[locale];

    if (!catalog || !Object.prototype.hasOwnProperty.call(catalog, key)) {
      return '';
    }

    return String(catalog[key]);
  }

  function interpolate(message, parameters) {
    const values = parameters && typeof parameters === 'object'
      ? parameters
      : {};

    return String(message).replace(/\{([A-Za-z0-9_]+)\}/g, (match, name) =>
      Object.prototype.hasOwnProperty.call(values, name)
        ? String(values[name])
        : match
    );
  }

  function translate(key, parameters, fallback) {
    const normalizedKey = String(key || '').trim();
    const activeMessage = messageFor(activeLocale, normalizedKey);
    const fallbackMessage = messageFor(FALLBACK_LOCALE, normalizedKey);
    const resolved = activeMessage || fallbackMessage || String(fallback || normalizedKey);
    return interpolate(resolved, parameters);
  }

  function applyElementTranslations(root) {
    if (!root || typeof root.querySelectorAll !== 'function') {
      return;
    }

    root.querySelectorAll('[data-i18n]').forEach(element => {
      element.textContent = translate(element.dataset.i18n);
    });

    root.querySelectorAll('[data-i18n-aria-label]').forEach(element => {
      element.setAttribute(
        'aria-label',
        translate(element.dataset.i18nAriaLabel)
      );
    });

    root.querySelectorAll('[data-i18n-placeholder]').forEach(element => {
      element.setAttribute(
        'placeholder',
        translate(element.dataset.i18nPlaceholder)
      );
    });
  }

  function applyDocumentTranslations() {
    if (!global.document) {
      return;
    }

    if (global.document.documentElement) {
      global.document.documentElement.lang = activeLocale;
    }

    applyElementTranslations(global.document);
  }

  function setLocale(value) {
    const normalized = normalizeLocale(value);

    if (normalized === '') {
      return false;
    }

    activeLocale = normalized;

    try {
      global.localStorage.setItem(STORAGE_KEY, activeLocale);
    } catch (error) {
      /* Browser storage can be unavailable in private or restricted contexts. */
    }

    applyDocumentTranslations();

    if (typeof global.dispatchEvent === 'function' &&
        typeof global.CustomEvent === 'function') {
      global.dispatchEvent(new global.CustomEvent(
        'vdr-suite:locale-changed',
        { detail: { locale: activeLocale } }
      ));
    }

    return true;
  }

  function getLocale() {
    return activeLocale;
  }

  function availableLocales() {
    return Object.keys(catalogs).sort();
  }

  const api = Object.freeze({
    storageKey: STORAGE_KEY,
    fallbackLocale: FALLBACK_LOCALE,
    t: translate,
    getLocale: getLocale,
    setLocale: setLocale,
    availableLocales: availableLocales,
    apply: applyElementTranslations,
    applyDocument: applyDocumentTranslations
  });

  global.VdrSuiteI18n = api;

  if (global.document) {
    if (global.document.readyState === 'loading' &&
        typeof global.document.addEventListener === 'function') {
      global.document.addEventListener(
        'DOMContentLoaded',
        applyDocumentTranslations,
        { once: true }
      );
    } else {
      applyDocumentTranslations();
    }
  }
})(window);
