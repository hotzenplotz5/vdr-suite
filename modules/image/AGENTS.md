# Agent rules: Images (inactive module scaffold)

Inherit all requirements from [root AGENTS.md](../../AGENTS.md) and the
[shared module rules](../AGENTS.md).

- This directory is only an inactive metadata/packaging scaffold:
  `addon.json` has no entrypoints, capabilities or grants. A staged package
  is not an image provider and may not activate itself.
- Keep authentication, actor/backend authorization, source-root confinement,
  filesystem protection and runtime lifecycle with the common Suite owners.
  No arbitrary host scans, file writes, installs or service restarts.
- Any future image files, metadata and thumbnails are untrusted inputs:
  require bounded resource use and explicit authorization and preflight before
  enabling processing or accessing live media.
- Defer detailed Image-specific storage, EXIF/thumbnail, API and UI design
  rules to a separately approved Images workstream. Future edits to this
  `AGENTS.md` require explicit prior maintainer permission.
