# VDR-Suite optional add-on source tree

[Architecture decision](../docs/adr/ADR-0068-optional-addons-and-source-packaging.md) · [Packaging guide](../docs/development/addon-source-packaging.md)

This directory owns the *source* of optional product extensions. It is not a runtime plugin loader. The existing browser `VdrSuitePlatform.registerModule()` registers web UI modules only; it does **not** grant backend permissions, register endpoints, or load external executables.

Every subdirectory currently in scope must contain:
- `addon.json`: versioned metadata, package name, scaffold status and declared capabilities;
- `README.md`: the supported and excluded feature set and packaging/runtime status;
- `AGENTS.md`: module-specific repository and safety rules, subordinate to root `AGENTS.md`.

Run `make check-addon-contract` to validate the complete source tree and focused tests.
Run `make stage-addon MODULE=rectools DESTDIR=/path/to/isolated-package-root PREFIX=/usr` to produce a **metadata-only**, reproducible staging payload under `usr/share/vdr-suite/addons/rectools/`. This is package-source readiness, **not** a finished Debian package or a functioning Rectools import.

No add-on is enabled by its presence in the source tree, `make install`, or metadata staging. Activation, public API routes, persistent permissions, protected job execution and web menus require future explicit reviewed implementation. No add-on can bypass the daemon's policy and ownership boundaries.

Current source modules:

| Module | Intended responsibility | State |
| --- | --- | --- |
| [rectools](rectools/README.md) | Media import, read-only checking, optionally guarded shrink | scaffold, no executable integration |
| [image](image/README.md) | Photo library/album/diashow | scaffold |
| [music](music/README.md) | Audio library/playlists | scaffold |
| [tvscraper](tvscraper/README.md) | External scraper metadata integration | scaffold |

The core owns VDR-native marks/cut/rename/move, account rights, common jobs, stable client APIs and reconciliation. Future add-ons implement bounded domain functionality through shared APIs instead of duplicating these owners.
