// Mobile-friendly numeric entry for Recording direct time seek.
// Android numeric keyboards often do not expose ':'. Keep the existing
// HH:MM:SS contract, but format digit-only entry as HH:MM[:SS] and default
// omitted minutes/seconds immediately before the existing seek handler runs.
(function (global) {
  'use strict';

  const marker = '__vdrSuiteRecordingTimeInputMaskBound';
  const DIRECT_TIME_LABEL = 'Direkte Wiedergabezeit';
  const DIRECT_SEEK_LABEL = 'Zur eingegebenen Wiedergabezeit springen';
  if (!global || !global.document || global[marker] === true) return;

  const document = global.document;
  if (typeof document.addEventListener !== 'function') return;

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function ariaLabel(node) {
    if (!node) return '';
    if (typeof node.getAttribute === 'function') return text(node.getAttribute('aria-label'));
    return text(node['aria-label']);
  }

  function isDirectTimeInput(node) {
    return Boolean(node && text(node.tagName).toUpperCase() === 'INPUT' && ariaLabel(node) === DIRECT_TIME_LABEL);
  }

  function isDirectSeekButton(node) {
    return Boolean(node && text(node.tagName).toUpperCase() === 'BUTTON' && ariaLabel(node) === DIRECT_SEEK_LABEL);
  }

  function digitsOnly(value) {
    return text(value).replace(/\D/g, '').slice(0, 6);
  }

  function formatDigits(value) {
    const digits = digitsOnly(value);
    if (digits.length <= 1) return digits;
    if (digits.length === 2) return digits + ':';
    if (digits.length <= 4) return digits.slice(0, 2) + ':' + digits.slice(2);
    return digits.slice(0, 2) + ':' + digits.slice(2, 4) + ':' + digits.slice(4);
  }

  function masked(node) {
    return Boolean(node && node.dataset && node.dataset.vdrSuiteTimeMask === 'true');
  }

  function markMasked(node) {
    if (node && node.dataset) node.dataset.vdrSuiteTimeMask = 'true';
  }

  function formatInput(node, inputType) {
    if (!isDirectTimeInput(node)) return;
    const value = text(node.value);
    const alreadyMasked = masked(node);
    const deleting = text(inputType).indexOf('delete') === 0;

    // Never fight Android/desktop backspace. In particular, deleting the
    // automatically inserted ':' after HH must be able to reach HH and H.
    if (deleting) return;

    // Respect explicit desktop/pasted HH:MM:SS input. Once digit-only entry has
    // entered mask mode, inserted separators are ours and can be rebuilt safely.
    if (!alreadyMasked && value.indexOf(':') !== -1) return;

    const digits = digitsOnly(value);
    if (!alreadyMasked && digits.length < 2) return;
    markMasked(node);
    node.value = formatDigits(digits);
    try { node.maxLength = 8; } catch (error) {}
    try { node.autocomplete = 'off'; } catch (error) {}
    try { node.enterKeyHint = 'go'; } catch (error) {}
  }

  function normalizeMaskedValue(node) {
    if (!isDirectTimeInput(node) || !masked(node)) return;
    const digits = digitsOnly(node.value);
    if (digits.length < 2) return;

    if (digits.length === 2) {
      node.value = digits + ':00:00';
      return;
    }

    if (digits.length <= 4) {
      const hours = digits.slice(0, 2);
      const minutes = digits.slice(2).padStart(2, '0');
      node.value = hours + ':' + minutes + ':00';
      return;
    }

    const hours = digits.slice(0, 2);
    const minutes = digits.slice(2, 4);
    const seconds = digits.slice(4).padStart(2, '0');
    node.value = hours + ':' + minutes + ':' + seconds;
  }

  function inputForButton(button) {
    const row = button && button.parentNode;
    if (row && typeof row.querySelector === 'function') {
      return row.querySelector('input[aria-label="' + DIRECT_TIME_LABEL + '"]');
    }
    return null;
  }

  document.addEventListener('input', function (event) {
    formatInput(event && event.target, event && event.inputType);
  }, true);

  // Capture runs before the existing target click/keydown listeners parse the
  // value, so HHMM becomes HH:MM:00 without changing either playback owner.
  document.addEventListener('click', function (event) {
    const target = event && event.target;
    if (!isDirectSeekButton(target)) return;
    normalizeMaskedValue(inputForButton(target));
  }, true);

  document.addEventListener('keydown', function (event) {
    const target = event && event.target;
    if (!isDirectTimeInput(target) || !event || event.key !== 'Enter') return;
    normalizeMaskedValue(target);
  }, true);

  document.addEventListener('blur', function (event) {
    normalizeMaskedValue(event && event.target);
  }, true);

  global[marker] = true;
  global.VdrSuiteRecordingTimeInputMask = Object.freeze({
    __test: Object.freeze({
      formatDigits: formatDigits,
      normalizeMaskedValue: normalizeMaskedValue
    })
  });
}(window));
