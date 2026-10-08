# Agent rules: Music

Inherit all requirements from [root AGENTS.md](../../AGENTS.md).

1. Own optional music catalog, artist/album identity, playlists and music client presentation; do not store music as pseudo-VDR recordings.
2. Treat tags, cover art, playlists, external source URLs and filenames as untrusted. Prevent traversal, unsolicited remote fetches and unbounded decode/transcode.
3. Reuse Suite authentication, grants, common client API, MediaSession/playback strategy and job safety where applicable. Never create a shadow player/session control plane.
4. Playlist visibility and shared libraries are actor-scoped; clients cannot access raw host paths or other users' media without authorization.
5. Keep install optional: no mandatory audio runtime dependencies in core and no startup failure when this module is missing.
6. Keep this AGENTS file, README, manifest and staging/package tests consistent; no capabilities/entrypoints before implementation and proof.
7. No media mutation, live installations, external network scans or expensive encoding without explicit authorization and preflight.
