# Music – optional VDR-Suite add-on

**State:** source/packaging scaffold only; no audio library, player, routes, UI or worker is available.

Scope: music discovery, library identity and metadata, playlists and playback through a stable Suite client API. Music tracks/albums/artists are not VDR recordings; do not inherit recording-destructive operations or create a separate authentication/session owner.

Scanning, metadata parsing, embedded cover art and playlist input are untrusted. Bound file sizes, parse time, source paths and resource consumption; protect per-actor visibility; reuse the existing Suite media session boundary when a later design justifies it.

`addon.json` advertises no runtime capability or entrypoint. Stage metadata only via `make stage-addon MODULE=music DESTDIR=/isolated/root PREFIX=/usr`. See [AGENTS.md](AGENTS.md) and [ADR-0068](../../docs/adr/ADR-0068-optional-addons-and-source-packaging.md).
