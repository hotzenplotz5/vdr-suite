# VDR-Suite optional add-on source tree

[Architecture decision](../docs/adr/ADR-0068-optional-addons-and-source-packaging.md) · [Packaging guide](../docs/development/addon-source-packaging.md)

This directory owns the *source* of optional product extensions. It is not a runtime plugin loader. The existing browser `VdrSuitePlatform.registerModule()` registers web UI modules only; it does **not** grant backend permissions, register endpoints, or load external executables.

Every subdirectory currently in scope must contain:
- `addon.json`: versioned metadata, package name, scaffold status and declared capabilities;
- `README.md`: the supported and excluded feature set and packaging/runtime status;
- `AGENTS.md`: module-specific repository and safety rules, subordinate to root `AGENTS.md`.

Run `make check-addon-contract` to validate the complete source tree and focused tests.
Run `make stage-addon MODULE=rectools DESTDIR=/path/to/isolated-package-root PREFIX=/usr` to produce a **metadata-only**, reproducible staging payload under `usr/share/vdr-suite/addons/rectools/`. This is package-source readiness, **not** a functioning Rectools import. A separate opt-in `make build-addon-deb` target builds a distinctly labelled, inert `~scaffold1` Debian metadata package. Runnable binary and source package policies follow only after the handler exists.

No add-on is enabled by its presence in the source tree, `make install`, or metadata staging. Activation, public API routes, persistent permissions, protected job execution and web menus require future explicit reviewed implementation. No add-on can bypass the daemon's policy and ownership boundaries.

Current source modules:

| Module | Intended responsibility | State |
| --- | --- | --- |
| [rectools](rectools/README.md) | Media import, read-only checking, optionally guarded shrink | scaffold, no executable integration |
| [image](image/README.md) | Photo library/album/diashow | scaffold |
| [music](music/README.md) | Audio library/playlists | scaffold |
| [tvscraper](tvscraper/README.md) | External scraper metadata integration | scaffold |

The core owns VDR-native marks/cut/rename/move, account rights, common jobs, stable client APIs and reconciliation. Future add-ons implement bounded domain functionality through shared APIs instead of duplicating these owners.

## Installed inventory (read-only)

A source manifest and independently installed metadata do not imply activation.
The [registry contract](../docs/development/addon-installed-registry-contract.md)
adds `make check-addon-registry` and a deterministic `installed_registry.py`
inventory/enable-decision preview. Installed module manifests must be complete
and match a core-owned package name; unknown packages, symlinked entries and
malformed metadata are rejected. **All listed modules remain disabled**, and
the preview never writes activation state, installs packages or invokes media.
This is not a server API, menu or backend module loader.

## C++ access decision contract (offline)

A standalone Suite-Core policy under `core/addons/` now checks authenticated actor context, **resolved** permission grants, exact reviewed module/package binding, package provenance, administrator intent, handler health and canonical backend write policy. `make test-addon-access-policy` validates these gates without a daemon or media access. This contract is not yet connected to API/Agent execution. No live module is enabled; `addons.media.import` is deliberately **not** added to current administrable rights until the backend operation is ready.

## Administrator desire is not execution

The [SQLite activation-intent proof](../docs/development/addon-installed-registry-contract.md)
models administrator preference separately from installed and effective
capabilities. It is **not connected to the daemon** and does not activate
any of these four packages. No automatic add-on enable or client discovery
is permitted from persisted preference alone.
