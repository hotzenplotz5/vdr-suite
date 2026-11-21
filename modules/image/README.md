# Images – optional VDR-Suite add-on

**State:** source/packaging scaffold only; no image library, routes, UI or worker is available.

Scope: photo browsing, albums and slideshow through Suite clients, eventually backed by registered media sources, indexed metadata and safe thumbnail generation. Image identity and collection/album metadata are a distinct domain: do not disguise photos as VDR recordings or require a running VDR.

The daemon owns access control, credentials, library metadata boundaries and APIs. Generated thumbnails and scans must have bounded resource usage, backend/source authorization and path confinement; never recursively scan arbitrary mounts or disclose private filesystem paths through clients.

`addon.json` declares no callable capabilities and no entrypoints. Stage metadata only with `make stage-addon MODULE=image DESTDIR=/isolated/root PREFIX=/usr`. See [AGENTS.md](AGENTS.md) and [ADR-0068](../../docs/adr/ADR-0068-optional-addons-and-source-packaging.md).

To build a distinct, **inactive** metadata-only `.deb` from sources, use the explicit `make build-addon-deb MODULE=image OUTPUT_DIR=/isolated/output ADDON_MAINTAINER='Name <valid@example.org>'` target from the repository root. This does not install or enable any runtime functionality.

The installed catalog recognizes this module metadata only through the [read-only registry](../../docs/development/addon-installed-registry-contract.md). Installation and C++ binaries do **not** activate it, grant privileges or expose API operations.
