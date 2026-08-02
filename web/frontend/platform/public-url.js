(function (global) {
  'use strict';

  if (global.VdrSuitePublicUrl) return;

  const SCRIPT_SUFFIX = '/frontend/platform/public-url.js';
  const CANONICAL_ROOTS = Object.freeze([
    '/api',
    '/frontend',
    '/channel-logos',
    '/recording-artwork'
  ]);

  function controlCharacterPresent(value) {
    return /[\u0000-\u001f\u007f]/.test(value);
  }

  function rootMatches(path) {
    return CANONICAL_ROOTS.some(function (root) {
      return path === root ||
        path.startsWith(root + '/') ||
        path.startsWith(root + '?') ||
        path.startsWith(root + '#');
    });
  }

  function unsafeEncodedPath(value) {
    const pathname = value.split(/[?#]/, 1)[0];
    return /%(?:0[0-9a-f]|1[0-9a-f]|7f|2f|5c)/i.test(pathname);
  }

  function containsDotSegment(path) {
    const pathname = path.split(/[?#]/, 1)[0];
    return pathname.split('/').some(function (segment) {
      if (!segment) return false;
      try {
        const decoded = decodeURIComponent(segment);
        return decoded === '.' || decoded === '..';
      } catch (error) {
        return true;
      }
    });
  }

  function validateCanonicalPath(value) {
    if (typeof value !== 'string' || value === '') {
      throw new TypeError('VDR-Suite public URL path must be a non-empty string');
    }
    if (!value.startsWith('/') || value.startsWith('//')) {
      throw new TypeError('VDR-Suite public URL path must be root-relative');
    }
    if (value.includes('\\') || controlCharacterPresent(value)) {
      throw new TypeError('VDR-Suite public URL path contains unsafe characters');
    }
    if (unsafeEncodedPath(value) || containsDotSegment(value)) {
      throw new TypeError('VDR-Suite public URL path contains an unsafe segment');
    }
    if (!rootMatches(value)) {
      throw new TypeError('VDR-Suite public URL path is outside the canonical roots');
    }
    return value;
  }

  function inferBasePath() {
    const currentScript = global.document && global.document.currentScript;
    if (!currentScript || typeof currentScript.src !== 'string' || !currentScript.src) {
      throw new Error('VDR-Suite public URL bootstrap requires document.currentScript.src');
    }

    const scriptUrl = new global.URL(currentScript.src, global.location.href);
    if (scriptUrl.origin !== global.location.origin ||
        !scriptUrl.pathname.endsWith(SCRIPT_SUFFIX)) {
      throw new Error('VDR-Suite public URL bootstrap script has an invalid origin or path');
    }

    const prefix = scriptUrl.pathname.slice(0, -SCRIPT_SUFFIX.length);
    if (!prefix) return '';
    if (!prefix.startsWith('/') || prefix.endsWith('/') ||
        prefix.includes('\\') || prefix.includes('//') ||
        controlCharacterPresent(prefix) || containsDotSegment(prefix)) {
      throw new Error('VDR-Suite public URL bootstrap inferred an invalid base path');
    }
    return prefix;
  }

  const basePath = inferBasePath();

  function resolvePath(path) {
    const canonical = validateCanonicalPath(path);
    if (basePath && canonical.startsWith(basePath + '/')) {
      throw new TypeError('VDR-Suite public URL path is already prefixed');
    }
    return basePath + canonical;
  }

  function isAlreadyPublicPath(path) {
    if (!basePath || !path.startsWith(basePath)) return false;
    const suffix = path.slice(basePath.length);
    return rootMatches(suffix);
  }

  function mapPathString(value) {
    if (typeof value !== 'string' || !value) return value;
    if (isAlreadyPublicPath(value)) return value;
    if (rootMatches(value)) return resolvePath(value);

    let parsed;
    try {
      parsed = new global.URL(value, global.location.href);
    } catch (error) {
      return value;
    }

    if (parsed.origin !== global.location.origin) return value;
    const localPath = parsed.pathname + parsed.search + parsed.hash;
    if (isAlreadyPublicPath(localPath)) return value;
    if (!rootMatches(localPath)) return value;

    const mapped = resolvePath(localPath);
    return /^[a-z][a-z0-9+.-]*:/i.test(value)
      ? parsed.origin + mapped
      : mapped;
  }

  function mapUrlValue(value) {
    if (typeof value === 'string') return mapPathString(value);
    if (typeof global.URL === 'function' && value instanceof global.URL) {
      return new global.URL(mapPathString(value.href), global.location.href);
    }
    return value;
  }

  function mapCssUrls(value) {
    if (typeof value !== 'string' || value.indexOf('url(') < 0) return value;
    return value.replace(/url\(\s*(['"]?)([^'"\)]+)\1\s*\)/gi, function (_, quote, url) {
      const mapped = mapPathString(url.trim());
      const selectedQuote = quote || '"';
      return 'url(' + selectedQuote + mapped + selectedQuote + ')';
    });
  }

  function installFetchAdapter() {
    if (typeof global.fetch !== 'function') return;
    const nativeFetch = global.fetch.bind(global);
    global.fetch = function (input, init) {
      if (typeof global.Request === 'function' && input instanceof global.Request) {
        const mapped = mapPathString(input.url);
        if (mapped !== input.url) input = new global.Request(mapped, input);
      } else {
        input = mapUrlValue(input);
      }
      return nativeFetch(input, init);
    };
  }

  function installEventSourceAdapter() {
    if (typeof global.EventSource !== 'function') return;
    const NativeEventSource = global.EventSource;
    function PublicEventSource(url, configuration) {
      return new NativeEventSource(mapUrlValue(url), configuration);
    }
    PublicEventSource.prototype = NativeEventSource.prototype;
    Object.setPrototypeOf(PublicEventSource, NativeEventSource);
    global.EventSource = PublicEventSource;
  }

  function patchUrlProperty(constructorName, propertyName) {
    const Constructor = global[constructorName];
    if (typeof Constructor !== 'function' || !Constructor.prototype) return;
    const descriptor = Object.getOwnPropertyDescriptor(Constructor.prototype, propertyName);
    if (!descriptor || typeof descriptor.set !== 'function' ||
        typeof descriptor.get !== 'function' || descriptor.configurable === false) return;

    Object.defineProperty(Constructor.prototype, propertyName, {
      configurable: descriptor.configurable,
      enumerable: descriptor.enumerable,
      get: descriptor.get,
      set: function (value) {
        descriptor.set.call(this, mapUrlValue(value));
      }
    });
  }

  function installDomAdapters() {
    [
      ['HTMLScriptElement', 'src'],
      ['HTMLImageElement', 'src'],
      ['HTMLSourceElement', 'src'],
      ['HTMLVideoElement', 'src'],
      ['HTMLVideoElement', 'poster'],
      ['HTMLAudioElement', 'src'],
      ['HTMLLinkElement', 'href'],
      ['HTMLAnchorElement', 'href'],
      ['HTMLIFrameElement', 'src']
    ].forEach(function (entry) {
      patchUrlProperty(entry[0], entry[1]);
    });

    const ElementConstructor = global.Element;
    if (typeof ElementConstructor === 'function' && ElementConstructor.prototype &&
        typeof ElementConstructor.prototype.setAttribute === 'function') {
      const nativeSetAttribute = ElementConstructor.prototype.setAttribute;
      ElementConstructor.prototype.setAttribute = function (name, value) {
        const normalizedName = String(name || '').toLowerCase();
        if (normalizedName === 'src' || normalizedName === 'href' ||
            normalizedName === 'poster' || normalizedName === 'action') {
          value = mapUrlValue(value);
        } else if (normalizedName === 'style') {
          value = mapCssUrls(value);
        }
        return nativeSetAttribute.call(this, name, value);
      };
    }
  }

  function installStyleAdapters() {
    const StyleConstructor = global.CSSStyleDeclaration;
    if (typeof StyleConstructor !== 'function' || !StyleConstructor.prototype) return;
    const prototype = StyleConstructor.prototype;

    if (typeof prototype.setProperty === 'function') {
      const nativeSetProperty = prototype.setProperty;
      prototype.setProperty = function (name, value, priority) {
        return nativeSetProperty.call(this, name, mapCssUrls(value), priority);
      };
    }

    ['background', 'backgroundImage', 'borderImage', 'content', 'cursor', 'listStyle', 'listStyleImage', 'mask', 'maskImage'].forEach(function (propertyName) {
      const descriptor = Object.getOwnPropertyDescriptor(prototype, propertyName);
      if (!descriptor || typeof descriptor.set !== 'function' ||
          typeof descriptor.get !== 'function' || descriptor.configurable === false) return;
      Object.defineProperty(prototype, propertyName, {
        configurable: descriptor.configurable,
        enumerable: descriptor.enumerable,
        get: descriptor.get,
        set: function (value) {
          descriptor.set.call(this, mapCssUrls(value));
        }
      });
    });
  }

  const api = Object.freeze({
    basePath: basePath,
    resolvePath: resolvePath
  });

  Object.defineProperty(global, 'VdrSuitePublicUrl', {
    configurable: false,
    enumerable: true,
    writable: false,
    value: api
  });

  installFetchAdapter();
  installEventSourceAdapter();
  installDomAdapters();
  installStyleAdapters();
}(window));
