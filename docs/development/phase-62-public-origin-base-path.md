# Phase 62 Slice 3A — Public Origin and Base-Path Integration

## Status

Repository implementation for Phase 62 Slice 3A. Runtime installation and
activation require a separate runtime approval and acceptance pass.

## Purpose

VDR-Suite and the existing yaVDR web application currently use overlapping
internal paths. In particular, both systems have routes below `/api/vdr/`.
VDR-Suite must therefore not claim the public root `/api` namespace.

The Suite-owned public contract is:

```text
/vdr-suite/
/vdr-suite/frontend/...
/vdr-suite/api/...
/vdr-suite/channel-logos/...
/vdr-suite/recording-artwork/...
```

The daemon keeps its existing internal contract:

```text
/frontend/...
/api/...
/channel-logos/...
/recording-artwork/...
```

## Canonical browser entry point

Nginx redirects these paths with HTTP 308:

```text
/vdr-suite
/vdr-suite/
/vdr-suite/frontend
```

The redirect target is always:

```text
/vdr-suite/frontend/
```

The frontend HTML uses relative initial asset paths. The same installed
`index.html` therefore works on the direct daemon routes and below the public
prefix without response rewriting.

## Public URL runtime

`web/frontend/platform/public-url.js` is the first frontend script. It derives
the optional public prefix from its own same-origin script URL and exposes the
immutable API:

```text
window.VdrSuitePublicUrl.basePath
window.VdrSuitePublicUrl.resolvePath(path)
```

Only canonical root-relative Suite paths are accepted. Schemes,
protocol-relative paths, backslashes, control characters, dot segments,
encoded path separators, unknown roots, and accidental double-prefixing are
rejected.

The platform runtime installs narrowly scoped browser adapters for URL-bearing
operations already used by the legacy script frontend:

- `fetch`;
- `EventSource`;
- URL-valued DOM properties and attributes;
- CSS `url(...)` values.

The adapters rewrite only same-origin canonical Suite roots. External URLs,
data URLs, unrelated yaVDR paths, and direct-daemon paths remain unchanged.
No cookie, Authorization, CSRF, password, session, or storage state is owned by
this runtime.

## Nginx contract

`packaging/nginx/vdr-suite.conf` is a server-context snippet. It:

- owns only `/vdr-suite` redirects and the `/vdr-suite/` proxy location;
- strips the prefix through a trailing-slash `proxy_pass` to
  `127.0.0.1:18080`;
- forwards host, client, scheme, and `X-Forwarded-Prefix` context;
- disables proxy buffering and caching for SSE compatibility;
- rewrites the daemon cookie path with
  `proxy_cookie_path / /vdr-suite/;`;
- contains no root `/api` location;
- contains no Uvicorn socket reference;
- uses no `sub_filter` or other response-body rewriting.

Cookie-path rewriting prevents a Suite browser-session cookie from being sent
to the unrelated yaVDR root `/api` namespace.

## Packaging and activation boundary

`mk/public-origin.mk` installs the frontend runtime as part of
`install-runtime`. It installs the Nginx snippet only as part of the full
`install` target or an explicit `install-nginx` call.

Installing the snippet does not activate it. The active yaVDR site must include
it explicitly during a separately approved runtime procedure. This repository
slice does not edit an active site, remove the temporary exact lifecycle
snippet, reload Nginx, restart the daemon, or modify credentials or database
state.

## Validation

The slice adds:

- Node runtime tests for direct-daemon and `/vdr-suite` operation;
- install-staging checks for the resolver and Nginx snippet;
- architecture checks for namespace ownership, prefix stripping, cookie scope,
  static bootstrap order, and forbidden response rewriting;
- continued execution of the existing frontend ownership contracts against a
  normalized direct-daemon view of the relative bootstrap paths.

## Later runtime acceptance

After separate runtime approval:

1. capture current binary, frontend, active-site, and snippet fingerprints;
2. install the new daemon/frontend and repository-managed snippet;
3. replace only the temporary Suite lifecycle include with the new prefix
   include;
4. run `nginx -t`;
5. reload only explicitly approved services;
6. verify yaVDR still owns public `/api/*`;
7. verify unauthenticated route provenance under `/vdr-suite`;
8. perform authenticated browser-session acceptance only through an approved
   credential path;
9. verify cookie path, SSE, logos, artwork, dynamic scripts, and direct-daemon
   compatibility;
10. rollback the binary/frontend and Nginx include state if any check fails.

The earlier managed Basic plaintext password is unavailable. This slice does
not rotate or reprovision it.
