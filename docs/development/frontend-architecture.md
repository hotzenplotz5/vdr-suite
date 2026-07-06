# Frontend Architecture and Ownership Contracts

## Navigation

- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Phase Map](../planning/phase-map.md)

---

## Purpose

This document defines the ownership boundaries for the current web frontend.

It is intentionally narrow and practical. Its main goal is to prevent accidental cross-module patches such as loading a timer module from the channel-logo helper.

---

## Current State

The Phase 58 web frontend is still a lightweight script-based frontend, not yet a fully bundled module system.

Current files:

```text
web/frontend/index.html
web/frontend/app.js
web/frontend/channel-logos.js
web/frontend/channel-browser.js
web/frontend/style.css
```

The current architecture is therefore a legacy-compatible ownership model:

- `index.html` is the static bootstrap and document shell.
- `app.js` owns the backend selection, module navigation and generic module loading.
- `channel-logos.js` owns channel-logo normalization, logo path candidates and logo fallback elements only.
- `channel-browser.js` owns the channel list/browser rendering only.
- `style.css` owns shared and module-specific visual styling.

---

## Ownership Table

| Area | Owner file | Notes |
| --- | --- | --- |
| Static HTML shell and script order | `web/frontend/index.html` | No feature logic beyond legacy inline blocks until those are extracted. |
| Backend selection | `web/frontend/app.js` | Includes backend loading, active backend state and module routing. |
| Module navigation | `web/frontend/app.js` | Owns the `data-module` tab dispatch and refresh behavior. |
| Timer list | `web/frontend/app.js` | `loadTimers()` and `renderTimerList()` are the Timer module boundary while the frontend is still script-based. |
| Timer conflict panel | `web/frontend/app.js` | Must be integrated through the Timer module boundary, not through unrelated helper files. |
| Channel logo lookup | `web/frontend/channel-logos.js` | Helper-only file. It must not load scripts or implement feature modules. |
| Channel browser | `web/frontend/channel-browser.js` | May depend on `app.js` helpers and `channel-logos.js`, but must not become a general bootstrap. |
| Visual state | `web/frontend/style.css` | Module-specific classes need explicit prefixes, for example `timer-conflict-*`. |

---

## Hard Rules

### `index.html`

Allowed:

- define the document shell
- load CSS
- load the official frontend scripts in explicit order
- keep existing legacy inline code until it is extracted in a planned phase

Not allowed:

- hiding new feature behavior in arbitrary inline blocks
- introducing dynamic script loaders
- silently changing script order without a reason documented in the phase notes

### `app.js`

Allowed:

- backend selection
- module router and refresh behavior
- load/render functions for current legacy modules
- Timer-owned follow-up calls such as loading timer conflicts after rendering timers

Not allowed:

- channel-logo normalization
- image/logo path fallback logic
- one-off DOM hacks that should be in a module-specific function

### `channel-logos.js`

Allowed:

- `normalizeChannelLogoName()`
- logo aliases
- candidate logo paths
- `createChannelLogoElement()`

Not allowed:

- `document.createElement('script')`
- `fetch()`
- timer logic
- recording logic
- SearchTimer logic
- EPG loading logic
- module bootstrapping

### `channel-browser.js`

Allowed:

- channel list/browser rendering
- grouped channel navigation
- channel agenda prefetching when needed for the channel browser

Not allowed:

- timer conflict rendering
- SearchTimer workflow logic
- global script bootstrapping

### `style.css`

Allowed:

- shared UI classes
- module-specific classes with explicit prefixes

Preferred prefixes:

```text
timer-*
timer-conflict-*
channel-*
epg-*
recording-*
searchtimer-*
backend-*
```

---

## Patch Placement Decision Tree

```text
Is it about Timer list or Timer conflicts?
  -> web/frontend/app.js for current legacy frontend
  -> future: web/frontend/modules/timers.js

Is it about channel logo lookup or fallback initials?
  -> web/frontend/channel-logos.js

Is it about channel list, groups or channel agenda cards?
  -> web/frontend/channel-browser.js

Is it only visual styling?
  -> web/frontend/style.css

Is it script order or static shell?
  -> web/frontend/index.html

Does it need a new frontend file?
  -> document the owner first and add/update the ownership contract test
```

---

## Timer Conflict Integration Rule

Timer conflicts are part of the Timer module.

Correct integration:

```text
loadTimers()
  -> fetch timer list
  -> renderTimerList(data)
  -> loadTimerConflictPanel(listFromResponse(data, 'timers'))
```

Incorrect integration:

```text
channel-logos.js
  -> dynamically loads timer-conflicts.js
```

Reason:

`channel-logos.js` is a helper file. Loading feature modules from a helper couples unrelated concerns and makes future patches hard to place correctly.

---

## Migration Direction

The desired long-term layout is:

```text
web/frontend/
  index.html
  style.css
  app.js
  modules/
    timers.js
    epg.js
    recordings.js
    searchtimers.js
    channels.js
  helpers/
    channel-logos.js
    dom.js
    format.js
```

This migration should be incremental. Until then, the ownership rules in this document are the source of truth.

---

## Guard Test

The ownership contract is guarded by:

```text
python3 tools/check_frontend_ownership_contracts.py
```

The test must fail if a helper file becomes a hidden bootstrap or if the script order in `index.html` drifts away from the documented frontend ownership model.

---

## Client API Wrapper Ownership

The web Client API wrapper is owned by:

- web/frontend/api/client-api.js

This file is the DOM-free HTTP and JSON seam for the web
frontend.

Allowed responsibilities:

- fetch calls
- URL and query construction
- JSON parsing
- HTTP error normalization
- backend id propagation
- read-only and write capability propagation

Forbidden responsibilities:

- DOM rendering
- CSS class decisions
- module routing
- channel-logo rendering
- Timer card rendering
- direct access to detailDataElement

UI modules may call the Client API wrapper.

Helper files must not become API bootstraps.

