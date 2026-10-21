# Media Tools add-on

This is the canonical location for **new Media Tools-specific A6 code**.
All development stays on `work/addon-platform-registry-lifecycle`; there
is no new branch, fork or repository.

The existing C++17 importer is maintained here under `include/`, `src/`
and `tests/` (moved, not copied, from the legacy source layout).
Manifest and metadata-only packaging remain under `modules/rectools/`.

Suite Core owns accounts, grants, command reservation, durable operations,
backend generation, capability discovery and recording reconciliation.
Media Tools owns only the import/check/shrink implementation. Import is
first; Check and Shrink are not available or grantable yet.

Current files are inert pre-dispatch contracts and tests. No executable
handler is registered, no service is started, and no manifest capability
is advertised.
