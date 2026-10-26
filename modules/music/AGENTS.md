# Agent rules: Music (inactive module scaffold)

Inherit all requirements from [root AGENTS.md](../../AGENTS.md) and the
[shared module rules](../AGENTS.md).

- This directory is only an inactive metadata/packaging scaffold:
  `addon.json` has no entrypoints, capabilities or grants. A staged package
  is not a music provider and may not activate itself.
- Keep authentication, actor/backend permissions, media-session ownership,
  source-root confinement and runtime lifecycle with the common Suite owners.
  No unreviewed scans, host file writes, installs or service restarts.
- Future audio files, playlists, tags, cover art and source URLs are
  untrusted; preserve actor-scoped visibility and require bounded processing,
  explicit authorization and preflight before enabling any media operation.
- Defer detailed Music-specific catalog, playlists, playback, API and UI
  design rules to a separately approved Music workstream. Future edits to
  this `AGENTS.md` require explicit prior maintainer permission.
