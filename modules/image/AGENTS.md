# Agent rules: Images

Inherit all requirements from [root AGENTS.md](../../AGENTS.md).

1. This module owns optional photos, albums and slideshow features, not VDR recordings or recording actions.
2. Treat EXIF, thumbnails and imported image bytes as untrusted data. Defend against malformed files, decompression bombs, metadata privacy leaks and oversized decode/cache jobs.
3. Restrict scanning and writes to administrator-registered storage roots. Validate real paths and symlinks on the owning backend; no arbitrary actor-controlled filesystem paths.
4. Use Suite actor grants, backend capabilities, API error semantics and resource constraints. Do not create independent accounts, authentication or a second global SQLite owner.
5. UI is an optional client of stable Suite APIs, never a direct image filesystem endpoint without policy. Design for absent VDR and disabled module.
6. Each change updates this file, README, addon.json capability declarations and staged package contracts when needed; no active capability until tested.
7. No live filesystem scans, package installation or service restarts without prior authorization and resource preflight.
