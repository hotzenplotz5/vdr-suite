# Optional Add-ons: source packaging and module rules

[ADR-0068](../adr/ADR-0068-optional-addons-and-source-packaging.md) · [Module inventory](../../modules/README.md)

## Current implementation

The following are **implemented now**: versioned declarative manifest validation, per-module developer rules, offline package staging, optional nonfunctional Debian scaffold-package building, and focused regression tests. Everything staged today is inert metadata: no daemon load, no systemd unit, no browser menu, no API route, no database migration and no media worker.

Source modules: `rectools`, `image`, `music`, `tvscraper`. The `rectools` source module's future package is called `vdr-suite-addon-media-tools`, reflecting its bounded functionality.

## Commands (repository checkout only)

```bash
cd /home/yavdr/vdr-suite
make check-addon-contract
make stage-addon MODULE=rectools DESTDIR=/path/to/isolated-package-root PREFIX=/usr
```

The staging script rejects unset/unsafe `DESTDIR` values and refuses to overwrite an existing package payload. It creates exactly:

```text
<DESTDIR>/usr/share/vdr-suite/addons/<MODULE>/addon.json
<DESTDIR>/usr/share/vdr-suite/addons/<MODULE>/README.md
<DESTDIR>/usr/share/vdr-suite/addons/<MODULE>/AGENTS.md
```

To build a **metadata-only Debian package** from the same sources (requires `dpkg-deb`, an existing isolated output directory, and an explicit, real package maintainer identity):

```bash
cd /home/yavdr/vdr-suite
make build-addon-deb MODULE=rectools OUTPUT_DIR=/absolute/package-output ADDON_MAINTAINER='Your Name <your-address@example.org>'
```

The tool refuses common system-root output targets, symlinked stage roots, invalid maintainer metadata, overwriting existing packages and insufficient free space. Choose a fresh, explicitly isolated output directory; the tool does not know which other directories your host treats as live data. It stages through an isolated temporary package root, constructs a control file, and publishes the `_all.deb` artifact without executing package installation. The Debian version suffix `~scaffold1` and package description explicitly identify this **nonfunctional scaffold**. Do **not** deploy it as a usable Media Tools extension; no runtime feature will appear.

An external Debian source-package build can likewise own a fresh `debian/<package>/` staging root and invoke the same stage target; full `debian/` source package metadata, runtime Depends, maintainer scripts, upgrades and service installation remain future per-module work. Each optional add-on must have its own package/runtime dependency list and be installable/removable without modifying Suite core packaging. Installing a metadata-only scaffold must not be presented to users as enabling functionality.

The source validator rejects malformed IDs, unknown keys, missing/invalid module-specific `AGENTS.md`, symlinks and runtime capability declarations while `state=scaffold`. Read these rules before changing a module; the repository root `AGENTS.md` remains authoritative.

## Validation gates

- `make check-addon-contract`: validates all manifests and tests non-destructive metadata staging without invoking `dpkg-deb`.
- `make check-addon-deb`: builds and inspects four inert packages in temporary directories (requires `dpkg-deb`, capacity preflight, never installs them).
- The existing `test-docs` and `test-install-staging` targets pick up the corresponding focused checks without executing a full build merely for metadata edits.

## Next bounded implementation slices

1. Versioned activation/capability/permission contract with an explicitly trusted package boundary and tests (not an arbitrary backend loader).
2. Rectools safe media preview/import/check CLI audited in its own repository; repair-free, source-preserving, check non-mutating.
3. Suite protected backend-bound import workflow, job and verify/reconciliation; focused integration tests and stageable runtime files with exact dependencies.
4. Optional frontend UI and advertised capability only after authorization, worker and install tests.
5. Optional shrink only after proving non-destructive staged replacement, verification, resource budgets, and failure handling.
6. Image/Music use the same addon contract to demonstrate independent media domains; TVScraper provider remains optional.

These slices require normal AGENTS.md preflight and fresh main evidence. No automatic switch-on or live host operations are implied.

## Installed module registry (read-only)

[Registry and activation-denial contract](addon-registry-activation-contract.md) defines local package metadata discovery and the distinction between an administrator's requested state and **effective server-granted capabilities**. Run `make check-addon-registry` for isolated policy, ownership and malformed-manifest regressions. This does not start the daemon or install packages; the local CLI does not expose any API route or grant permissions. The existing scaffold packages remain inactive.
