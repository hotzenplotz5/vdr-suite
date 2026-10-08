# Media Tools (Rectools) – optional VDR-Suite add-on

**State:** source/packaging scaffold only. **No import, checking or shrink action is wired or enabled.**

Source location: `modules/rectools/`; prospective binary package: `vdr-suite-addon-media-tools`. Rectools remains an external tool and independent repository. Integration uses an explicit restricted Suite adapter/worker on the owning backend rather than shell execution from the browser or a VDR plugin.

## Allowed scope

1. **Import** local media from administrator-registered source roots into VDR recording format, with preview, codec analysis, identity/duplicate check, staged commit, index verification, provenance and library reconciliation.
2. **Check** existing recordings for media/integrity information, strictly non-destructively; report findings through Suite.
3. **Shrink** optionally; separately reviewed, explicit consent, staged result, verification and original preservation. Never a silent rewrite.

## Explicit exclusions

- `repair` and `smart_repair`: **not permitted**, including indirect invocation during import.
- PES-to-TS: handled by current VDR; do not add a Rectools adapter for it.
- cut/marks, rename, move: canonical VDR-Suite functionality; no duplicate Rectools actions.
- No use of the legacy world-writable `/tmp/vdr-rectools-jobs` worker as the Suite trust boundary.
- Do not treat today's Rectools `import`, `check_single`, or `shrink_single` as a safe public API. Import currently removes the source on success and may invoke repair. Check currently writes diagnostic markers, mail/status artifacts. Shrink currently replaces TS segments and index. These contracts require hardening, isolated verification and tests **in Rectools** before execution wiring in Suite.

## Runtime boundaries

Actor authorization and backend write policy are Suite-owned. Backend Agent generation fencing and job/idempotency guarantees apply to real asynchronous operations; no arbitrary user-supplied filesystem paths, unbounded transcoding or direct client execution. Imported VDR recordings must be re-read through the existing recording reconciliation owner. Minimize impact on live TV and existing recordings; preflight both staging and destination capacity.

## Packaging

`addon.json` is a disabled scaffold contract with zero advertised runtime capabilities. `make stage-addon MODULE=rectools DESTDIR=/isolated/root PREFIX=/usr` installs only manifest, README and AGENTS into staging. Do not claim or add runtime dependencies until the handler and worker boundary exist. The server remains functional when this add-on is absent.

See [module rules](AGENTS.md), [ADR-0068](../../docs/adr/ADR-0068-optional-addons-and-source-packaging.md) and [packaging guide](../../docs/development/addon-source-packaging.md).

To build a distinct, **inactive** metadata-only `.deb` from sources, use the explicit `make build-addon-deb MODULE=rectools OUTPUT_DIR=/isolated/output ADDON_MAINTAINER='Name <valid@example.org>'` target from the repository root. This does not install or enable any runtime functionality.
