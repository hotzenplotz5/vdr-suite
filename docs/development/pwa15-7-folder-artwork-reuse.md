# PWA.15.7 – Recording folder artwork DOM reuse

Status: implemented on `main`; real-browser/yaVDR acceptance pending.

## Reported regression

In Home → Aufnahmeordner → Komödie, a decoded “Ace Ventura” poster was
visible, but after a detour via Horror/Historienfilm and return to Komödie,
the card showed a placeholder until the artwork loaded again.

The Recordings 2 view previously called `target.replaceChildren()` on every
folder load and rebuilt every card and `<img>` even when the same authorized
folder contents were returned. PWA.15.6 removed synchronous server-side
preview blocking; it does not preserve destroyed browser image DOM nodes.

## Bounded fix

The existing `recordings2-browser-view.js` DOM owner retains at most three
detached folder roots in an in-memory LRU map, keyed by backend and canonical
folder path. `renderLoading` still performs the canonical API read; **cached
DOM is never shown as a substitute for a successfully authorized response**.
Only after successful folder resolution, `renderFolder` compares the displayed
folder signature (counts, folders, enriched recordings and load-more state).
On equality, the original card and decoded image nodes are reattached without
another `img.src` assignment. On differences, the normal renderer rebuilds
so refreshed metadata, actions, counts and removed recordings remain current.

Errors clear the in-memory cache; backend changes isolate it; module
destruction clears the cache and prevents recapturing an old visible root.
No new request owner, cache persistence, authorization mechanism, SSE/polling
loop, daemon worker, database change or background task is introduced.

## Tests and acceptance

The existing `test_recordings2_detail_back_navigation.js` now exercises:

- Komödie → Horror → Komödie after a successful folder response reuses the
  exact same Recording artwork image DOM object without creating another one.
- Changed Recording metadata forces a fresh presentation.
- Authorization failure prevents reuse of previously cached content.
- Backend changes and module destruction prevent cross-context DOM reuse.

Run `node web/frontend/tests/test_recordings2_detail_back_navigation.js`,
then `make test-recordings2-runtime`. Both are repository-level tests; they
do **not** establish the real browser outcome. Browser acceptance:
Komödie → Horror → Komödie, including “Ace Ventura”, should display
previously decoded posters on the return without a blank/reload phase.
In DevTools Network, verify whether any HTTP image request occurs; DOM reuse
and network caching are distinct observations.

## Deployment

This change affects **only** the source
`web/frontend/recordings2-browser-view.js`, which is one constituent of the
installed **concatenated** `recordings2-browser-view.js` bundle described by
`mk/recordings2.mk`. Never copy only the individual source file onto the
production bundle. Recreate the verified concatenation, replace just that
static production asset, and regenerate/check the installed PWA shell manifest
using `tools/build_pwa_shell.js`. No C++ build or daemon restart is required.
