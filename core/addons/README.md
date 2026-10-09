# Core Add-on access decisions

This is an internal C++17 source-only policy component, not an optional
module and not a public C++ ABI.

`AddonAccessPolicy` consumes VDR-Suite's existing
`RequestSecurityContext` and `BackendAccessDecision`. It authorizes
global-admin inventory inspection and models the future Media Tools import
admission conjunction. The actual VDR-Suite daemon, package registry,
actor grant administration and Backend Agent do **not** call it yet.
No package, module or capability becomes enabled by compiling the library.

The future `addons.media.import` right is deliberately not yet in the
canonical grant administration vocabulary. Therefore any positive access
example in this unit test is an isolated injected fixture and **not** a
production capability. Do not add an HTTP endpoint, handler launcher or
persisted enable switch on the strength of this evaluator alone.

Run `make test-addon-access-policy` on an isolated build path.
See [registry boundary](../../docs/development/addon-installed-registry-contract.md).

## Persistent activation *intent*, not activation

`AddonActivationIntentRepository` is the next **independently testable**
C++ component. It persists a per-module/per-backend administrative desire
(`desired_enabled`) in SQLite with a monotonic integer revision and
`updated_by_actor_id`. Absent settings are disabled at revision 0. Mutations
use `BEGIN IMMEDIATE` under the canonical Database transaction lease
plus a strict expected revision, and reject invalid module/backend keys.
Only a canonical, resolved `role.admin@*` context may read or write it.

Enabling an intent is limited to the reviewed Media Tools identity with
independent trusted-package/version/registered-healthy-handler evidence and
canonical backend write-policy evidence. Other scaffold modules can only
express disabled intent. A privileged administrator can disable the intent
when the backend or installed handler is unavailable.

**The stored intent is never consulted by the running daemon, backend Agent,
API or worker in this slice.** It cannot register a module, grant execution
rights, or start an import. The positive unit tests inject artificial trusted
evidence; they are not claims that a production handler has been activated.
The SQLite schema is created only in the isolated tests by calling
`ensureSchema()`; no existing database migration was performed.

`make test-addon-activation-intent` compiles and tests the code against a
temporary SQLite DB on an explicitly selected non-live `TMPDIR`.
