# Media Tools add-on

This is the canonical location for **new Media Tools-specific A6 code**.
All development stays on `work/addon-platform-registry-lifecycle`; there
is no new branch, fork or repository.

The already existing C++17 import implementation currently lives in
`modules/rectools/` and stays there until its references and packaging
can be migrated together in a tested change. **Do not copy or fork it.**

Suite Core owns accounts, grants, command reservation, durable operations,
backend generation, capability discovery and recording reconciliation.
Media Tools owns only the import/check/shrink implementation. Import is
first; Check and Shrink are not available or grantable yet.

Current files are inert pre-dispatch contracts and tests. No executable
handler is registered, no service is started, and no manifest capability
is advertised.
